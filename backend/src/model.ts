export const CATEGORIES = [
  "cube", "ship", "ball", "ufo", "wave", "robot", "spider", "swing", "jetpack",
] as const;

export type Category = (typeof CATEGORIES)[number];

// Geometry Dash 2.2081 desktop. The client independently obtains these through
// GameManager::countForType; these caps are a server-side anti-abuse boundary.
export const MAX_ICON_ID: Readonly<Record<Category, number>> = {
  cube: 485,
  ship: 169,
  ball: 118,
  ufo: 149,
  wave: 96,
  robot: 68,
  spider: 69,
  swing: 43,
  jetpack: 8,
};

export interface IconCollectionInput {
  schemaVersion: 1;
  gameVersion: "2.2081";
  accountId: number;
  userId: number;
  username: string;
  icons: Record<Category, number[]>;
  equipped: Record<Category, number>;
}

export interface PublicCollection {
  schemaVersion: 1;
  gameVersion: "2.2081";
  accountId: number;
  username: string;
  total: number;
  icons: Record<Category, number[]>;
  counts: Record<Category, number>;
  equipped: Record<Category, number>;
  synchronizedAt: string;
}

export interface Env {
  DB: D1Database;
  ARGON_BASE_URL: string;
  ALLOWED_ORIGIN: string;
  RATE_LIMIT_SALT: string;
}
