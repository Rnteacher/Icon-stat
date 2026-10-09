import { ApiError } from "./validation";
import type { Env, IconCollectionInput } from "./model";

export function bearerToken(request: Request): string {
  const header = request.headers.get("authorization") ?? "";
  const match = /^Bearer ([A-Za-z0-9._~-]{16,512})$/.exec(header);
  if (!match?.[1]) throw new ApiError(401, "missing_auth", "Authentication is required");
  return match[1];
}

export async function verifyArgon(env: Env, identity: Pick<IconCollectionInput, "accountId" | "userId" | "username">, token: string): Promise<void> {
  const base = env.ARGON_BASE_URL.replace(/\/$/, "");
  if (!base.startsWith("https://") && !base.startsWith("http://127.0.0.1:") && !base.startsWith("http://localhost:")) {
    throw new ApiError(503, "auth_misconfigured", "Account verification is unavailable");
  }
  const query = new URLSearchParams({
    account_id: String(identity.accountId),
    user_id: String(identity.userId),
    username: identity.username,
    authtoken: token,
  });
  let response: Response;
  try {
    response = await fetch(`${base}/validation/check_strong?${query}`, {
      headers: { accept: "application/json", "user-agent": "icon-stat-api/1.0" },
      cf: { cacheTtl: 0 },
    });
  } catch {
    throw new ApiError(503, "auth_unavailable", "Account verification is temporarily unavailable");
  }
  if (!response.ok) throw new ApiError(503, "auth_unavailable", "Account verification is temporarily unavailable");
  const result = await response.json<{ valid?: boolean; valid_weak?: boolean }>();
  if (!result.valid_weak) throw new ApiError(401, "invalid_auth", "Account authentication failed");
  if (!result.valid) throw new ApiError(401, "identity_mismatch", "Refresh your Geometry Dash login and try again");
}

export async function rateLimit(env: Env, request: Request, bucket: string, limit: number, windowSeconds: number): Promise<void> {
  const ip = request.headers.get("cf-connecting-ip") ?? "unknown";
  const data = new TextEncoder().encode(`${env.RATE_LIMIT_SALT}:${ip}`);
  const digest = await crypto.subtle.digest("SHA-256", data);
  const key = `${bucket}:${Array.from(new Uint8Array(digest), (b) => b.toString(16).padStart(2, "0")).join("")}`;
  const now = Math.floor(Date.now() / 1000);
  const windowStart = now - (now % windowSeconds);
  await env.DB.prepare(
    `INSERT INTO rate_limits (key, window_start, request_count) VALUES (?, ?, 1)
     ON CONFLICT(key, window_start) DO UPDATE SET request_count = request_count + 1`,
  ).bind(key, windowStart).run();
  const row = await env.DB.prepare(
    "SELECT request_count FROM rate_limits WHERE key = ? AND window_start = ?",
  ).bind(key, windowStart).first<{ request_count: number }>();
  if ((row?.request_count ?? 0) > limit) throw new ApiError(429, "rate_limited", "Too many requests; try again later");
  if (Math.random() < 0.01) {
    await env.DB.prepare("DELETE FROM rate_limits WHERE window_start < ?").bind(now - 86_400).run();
  }
}
