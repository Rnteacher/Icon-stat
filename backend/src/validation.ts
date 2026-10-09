import { CATEGORIES, MAX_ICON_ID, type Category, type IconCollectionInput } from "./model";

const MAX_BODY_BYTES = 32 * 1024;
const USERNAME = /^[A-Za-z0-9_]{3,20}$/;

export class ApiError extends Error {
  constructor(public readonly status: number, public readonly code: string, message: string) {
    super(message);
  }
}

export function assertBodySize(request: Request): void {
  const raw = request.headers.get("content-length");
  if (raw && Number(raw) > MAX_BODY_BYTES) {
    throw new ApiError(413, "body_too_large", "Request body exceeds 32 KiB");
  }
}

export function parseAccountId(raw: string): number {
  if (!/^\d{1,10}$/.test(raw)) throw new ApiError(400, "invalid_account_id", "Invalid account ID");
  const value = Number(raw);
  if (!Number.isSafeInteger(value) || value <= 0 || value > 2_147_483_647) {
    throw new ApiError(400, "invalid_account_id", "Invalid account ID");
  }
  return value;
}

export function validateCollection(value: unknown): IconCollectionInput {
  if (!value || typeof value !== "object" || Array.isArray(value)) {
    throw new ApiError(400, "invalid_body", "Expected a JSON object");
  }
  const body = value as Record<string, unknown>;
  if (body.schemaVersion !== 1 || body.gameVersion !== "2.2081") {
    throw new ApiError(400, "unsupported_schema", "Expected schemaVersion 1 for game 2.2081");
  }
  const accountId = integer(body.accountId, "accountId");
  const userId = integer(body.userId, "userId");
  if (accountId <= 0 || userId <= 0) throw new ApiError(400, "invalid_identity", "Invalid account identity");
  if (typeof body.username !== "string" || !USERNAME.test(body.username)) {
    throw new ApiError(400, "invalid_username", "Invalid Geometry Dash username");
  }
  if (!body.icons || typeof body.icons !== "object" || Array.isArray(body.icons)) {
    throw new ApiError(400, "invalid_icons", "icons must be an object");
  }
  if (!body.equipped || typeof body.equipped !== "object" || Array.isArray(body.equipped)) {
    throw new ApiError(400, "invalid_equipped", "equipped must be an object");
  }

  const source = body.icons as Record<string, unknown>;
  const selected = body.equipped as Record<string, unknown>;
  const icons = {} as Record<Category, number[]>;
  const equipped = {} as Record<Category, number>;
  for (const category of CATEGORIES) {
    const list = source[category];
    if (!Array.isArray(list)) throw new ApiError(400, "missing_category", `Missing ${category} icon list`);
    if (list.length > MAX_ICON_ID[category]) throw new ApiError(400, "too_many_icons", `Too many ${category} icons`);
    const ids = list.map((id) => integer(id, `${category} icon ID`));
    const unique = new Set(ids);
    if (unique.size !== ids.length) throw new ApiError(400, "duplicate_icon", `Duplicate ${category} icon ID`);
    if (ids.some((id) => id < 1 || id > MAX_ICON_ID[category])) {
      throw new ApiError(400, "invalid_icon_id", `${category} icon ID is outside the 2.2081 range`);
    }
    icons[category] = ids.sort((a, b) => a - b);
    const selectedId = integer(selected[category], `equipped ${category} icon ID`);
    if (selectedId < 1 || selectedId > MAX_ICON_ID[category]) {
      throw new ApiError(400, "invalid_equipped_icon", `Equipped ${category} icon is outside the 2.2081 range`);
    }
    if (!unique.has(selectedId)) {
      throw new ApiError(400, "equipped_icon_not_owned", `Equipped ${category} icon is not in the unlocked collection`);
    }
    equipped[category] = selectedId;
  }
  const unknown = Object.keys(source).filter((key) => !CATEGORIES.includes(key as Category));
  if (unknown.length) throw new ApiError(400, "unknown_category", "Unknown icon category");
  const unknownEquipped = Object.keys(selected).filter((key) => !CATEGORIES.includes(key as Category));
  if (unknownEquipped.length) throw new ApiError(400, "unknown_equipped_category", "Unknown equipped icon category");

  return {
    schemaVersion: 1,
    gameVersion: "2.2081",
    accountId,
    userId,
    username: body.username,
    icons,
    equipped,
  };
}

function integer(value: unknown, name: string): number {
  if (typeof value !== "number" || !Number.isSafeInteger(value)) {
    throw new ApiError(400, "invalid_number", `${name} must be an integer`);
  }
  return value;
}
