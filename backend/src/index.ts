import { CATEGORIES, type Category, type Env, type IconCollectionInput, type PublicCollection } from "./model";
import { ApiError, assertBodySize, parseAccountId, validateCollection } from "./validation";
import { bearerToken, rateLimit, verifyArgon } from "./security";

const JSON_HEADERS = { "content-type": "application/json; charset=utf-8", "x-content-type-options": "nosniff" };

export default {
  async fetch(request: Request, env: Env): Promise<Response> {
    try {
      const url = new URL(request.url);
      if (request.method === "GET" && url.pathname === "/health") {
        return json({ ok: true, service: "icon-stat-api", schemaVersion: 1 });
      }
      const match = /^\/api\/v1\/players\/(\d+)\/icons$/.exec(url.pathname);
      if (request.method === "GET" && match?.[1]) {
        await rateLimit(env, request, "read", 120, 60);
        return await getCollection(env, parseAccountId(match[1]), request);
      }
      if (request.method === "PUT" && url.pathname === "/api/v1/me/icons") {
        await rateLimit(env, request, "write", 6, 60);
        return await putCollection(env, request);
      }
      if (request.method === "DELETE" && url.pathname === "/api/v1/me/icons") {
        await rateLimit(env, request, "delete", 3, 300);
        return await deleteCollection(env, request);
      }
      if (request.method === "OPTIONS") return new Response(null, { status: 204, headers: cors(env) });
      throw new ApiError(404, "not_found", "Endpoint not found");
    } catch (error) {
      if (error instanceof ApiError) return json({ error: error.code, message: error.message }, error.status);
      console.error("request_failed", error instanceof Error ? error.name : "unknown");
      return json({ error: "internal_error", message: "Request failed" }, 500);
    }
  },
} satisfies ExportedHandler<Env>;

async function getCollection(env: Env, accountId: number, request: Request): Promise<Response> {
  const player = await env.DB.prepare(
    "SELECT account_id, username, total_icons, game_version, schema_version, synchronized_at FROM players WHERE account_id = ?",
  ).bind(accountId).first<{
    account_id: number; username: string; total_icons: number; game_version: string; schema_version: number; synchronized_at: string;
  }>();
  if (!player) throw new ApiError(404, "not_published", "This player has not published an icon collection");
  const rows = await env.DB.prepare(
    "SELECT category, icon_id FROM player_icons WHERE account_id = ? ORDER BY category, icon_id",
  ).bind(accountId).all<{ category: Category; icon_id: number }>();
  const equippedRows = await env.DB.prepare(
    "SELECT category, icon_id FROM equipped_icons WHERE account_id = ? ORDER BY category",
  ).bind(accountId).all<{ category: Category; icon_id: number }>();
  const icons = emptyIcons();
  for (const row of rows.results) if (CATEGORIES.includes(row.category)) icons[row.category].push(row.icon_id);
  const equipped = emptyEquipped();
  for (const row of equippedRows.results) if (CATEGORIES.includes(row.category)) equipped[row.category] = row.icon_id;
  const counts = Object.fromEntries(CATEGORIES.map((category) => [category, icons[category].length])) as Record<Category, number>;
  const payload: PublicCollection = {
    schemaVersion: 1,
    gameVersion: "2.2081",
    accountId: player.account_id,
    username: player.username,
    total: player.total_icons,
    icons,
    counts,
    equipped,
    synchronizedAt: player.synchronized_at,
  };
  const etag = `W/\"${accountId}-${Date.parse(player.synchronized_at)}\"`;
  if (request.headers.get("if-none-match") === etag) return new Response(null, { status: 304, headers: { etag } });
  return json(payload, 200, { "cache-control": "public, max-age=60, stale-while-revalidate=300", etag });
}

async function putCollection(env: Env, request: Request): Promise<Response> {
  assertBodySize(request);
  const token = bearerToken(request);
  let raw: unknown;
  try { raw = await request.json(); } catch { throw new ApiError(400, "invalid_json", "Malformed JSON body"); }
  const body = validateCollection(raw);
  await verifyArgon(env, body, token);
  const existing = await env.DB.prepare("SELECT synchronized_at FROM players WHERE account_id = ?").bind(body.accountId).first<{ synchronized_at: string }>();
  if (existing && Date.now() - Date.parse(existing.synchronized_at) < 30_000) {
    throw new ApiError(429, "sync_too_frequent", "Wait 30 seconds between synchronizations");
  }
  const total = CATEGORIES.reduce((sum, category) => sum + body.icons[category].length, 0);
  const now = new Date().toISOString();
  const statements: D1PreparedStatement[] = [
    env.DB.prepare(
      `INSERT INTO players (account_id, user_id, username, total_icons, game_version, schema_version, auth_provider, verified_at, synchronized_at)
       VALUES (?, ?, ?, ?, '2.2081', 1, 'argon', ?, ?)
       ON CONFLICT(account_id) DO UPDATE SET user_id=excluded.user_id, username=excluded.username,
       total_icons=excluded.total_icons, game_version=excluded.game_version, schema_version=excluded.schema_version,
       auth_provider='argon', verified_at=excluded.verified_at, synchronized_at=excluded.synchronized_at`,
    ).bind(body.accountId, body.userId, body.username, total, now, now),
    env.DB.prepare("DELETE FROM player_icons WHERE account_id = ?").bind(body.accountId),
  ];
  for (const category of CATEGORIES) {
    for (let offset = 0; offset < body.icons[category].length; offset += 30) {
      const chunk = body.icons[category].slice(offset, offset + 30);
      const placeholders = chunk.map(() => "(?, ?, ?)").join(",");
      const bindings = chunk.flatMap((id) => [body.accountId, category, id]);
      statements.push(env.DB.prepare(`INSERT INTO player_icons (account_id, category, icon_id) VALUES ${placeholders}`).bind(...bindings));
    }
  }
  for (const category of CATEGORIES) {
    statements.push(env.DB.prepare(
      "INSERT INTO equipped_icons (account_id, category, icon_id) VALUES (?, ?, ?)",
    ).bind(body.accountId, category, body.equipped[category]));
  }
  await env.DB.batch(statements);
  return json({ ok: true, accountId: body.accountId, total, synchronizedAt: now });
}

async function deleteCollection(env: Env, request: Request): Promise<Response> {
  assertBodySize(request);
  const token = bearerToken(request);
  let raw: unknown;
  try { raw = await request.json(); } catch { throw new ApiError(400, "invalid_json", "Malformed JSON body"); }
  const identity = validateIdentity(raw);
  await verifyArgon(env, identity, token);
  const result = await env.DB.prepare("DELETE FROM players WHERE account_id = ?").bind(identity.accountId).run();
  return json({ ok: true, deleted: (result.meta.changes ?? 0) > 0 });
}

function validateIdentity(value: unknown): Pick<IconCollectionInput, "accountId" | "userId" | "username"> {
  if (!value || typeof value !== "object") throw new ApiError(400, "invalid_body", "Expected identity object");
  const body = value as Record<string, unknown>;
  const dummy = Object.fromEntries(CATEGORIES.map((c) => [c, [1]]));
  const equipped = Object.fromEntries(CATEGORIES.map((c) => [c, 1]));
  const parsed = validateCollection({ schemaVersion: 1, gameVersion: "2.2081", ...body, icons: dummy, equipped });
  return { accountId: parsed.accountId, userId: parsed.userId, username: parsed.username };
}

function emptyIcons(): Record<Category, number[]> {
  return Object.fromEntries(CATEGORIES.map((category) => [category, [] as number[]])) as unknown as Record<Category, number[]>;
}

function emptyEquipped(): Record<Category, number> {
  return Object.fromEntries(CATEGORIES.map((category) => [category, 0])) as Record<Category, number>;
}

function cors(env: Env): HeadersInit {
  return {
    "access-control-allow-origin": env.ALLOWED_ORIGIN || "null",
    "access-control-allow-methods": "GET,PUT,DELETE,OPTIONS",
    "access-control-allow-headers": "authorization,content-type,if-none-match",
    "access-control-max-age": "86400",
  };
}

function json(value: unknown, status = 200, extra: HeadersInit = {}): Response {
  return new Response(JSON.stringify(value), { status, headers: { ...JSON_HEADERS, ...extra } });
}
