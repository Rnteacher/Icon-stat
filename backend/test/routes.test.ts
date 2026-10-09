import { describe, expect, it } from "vitest";
import worker from "../src/index";
import type { Env } from "../src/model";

function envWithEmptyDatabase(): Env {
  const db = {
    prepare(sql: string) {
      const statement = {
        bind() { return statement; },
        async run() { return { success: true, meta: { changes: 1 } }; },
        async first() {
          if (sql.includes("request_count FROM rate_limits")) return { request_count: 1 };
          if (sql.includes("FROM players")) return null;
          return null;
        },
        async all() { return { success: true, results: [] }; },
      };
      return statement;
    },
  };
  return {
    DB: db as unknown as D1Database,
    ARGON_BASE_URL: "https://argon.example/v1",
    ALLOWED_ORIGIN: "null",
    RATE_LIMIT_SALT: "test-only-salt",
  };
}

describe("HTTP route contract", () => {
  it("returns the documented health payload", async () => {
    const response = await worker.fetch(new Request("https://example.test/health"), envWithEmptyDatabase());
    expect(response.status).toBe(200);
    await expect(response.json()).resolves.toEqual({ ok: true, service: "icon-stat-api", schemaVersion: 1 });
  });

  it("returns JSON 404 for an unpublished player", async () => {
    const response = await worker.fetch(new Request("https://example.test/api/v1/players/123/icons"), envWithEmptyDatabase());
    expect(response.status).toBe(404);
    await expect(response.json()).resolves.toMatchObject({ error: "not_published" });
  });

  it.each(["PUT", "DELETE"])("protects %s /api/v1/me/icons", async (method) => {
    const response = await worker.fetch(new Request("https://example.test/api/v1/me/icons", {
      method,
      headers: { "content-type": "application/json" },
      body: "{}",
    }), envWithEmptyDatabase());
    expect(response.status).toBe(401);
    await expect(response.json()).resolves.toMatchObject({ error: "missing_auth" });
  });
});
