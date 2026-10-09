import { describe, expect, it, vi } from "vitest";
import { bearerToken, verifyArgon } from "../src/security";

describe("authentication", () => {
  it("requires a bearer token", () => {
    expect(() => bearerToken(new Request("https://example.test"))).toThrow(/Authentication/);
  });
  it("uses strong Argon validation and rejects weak-only identity", async () => {
    vi.stubGlobal("fetch", vi.fn(async () => Response.json({ valid: false, valid_weak: true })));
    const env = { ARGON_BASE_URL: "https://argon.example/v1" } as never;
    await expect(verifyArgon(env, { accountId: 1, userId: 2, username: "Player" }, "abcdefghijklmnop"))
      .rejects.toThrow(/Refresh/);
  });
  it("accepts strong Argon validation", async () => {
    vi.stubGlobal("fetch", vi.fn(async () => Response.json({ valid: true, valid_weak: true })));
    const env = { ARGON_BASE_URL: "https://argon.example/v1" } as never;
    await expect(verifyArgon(env, { accountId: 1, userId: 2, username: "Player" }, "abcdefghijklmnop"))
      .resolves.toBeUndefined();
  });
  it("converts authentication network failures to a safe service error", async () => {
    vi.stubGlobal("fetch", vi.fn(async () => { throw new Error("socket details must not leak"); }));
    const env = { ARGON_BASE_URL: "https://argon.example/v1" } as never;
    await expect(verifyArgon(env, { accountId: 1, userId: 2, username: "Player" }, "abcdefghijklmnop"))
      .rejects.toThrow("Account verification is temporarily unavailable");
  });
  it("rejects an upstream server error without trusting its body", async () => {
    vi.stubGlobal("fetch", vi.fn(async () => new Response("sensitive upstream detail", { status: 500 })));
    const env = { ARGON_BASE_URL: "https://argon.example/v1" } as never;
    await expect(verifyArgon(env, { accountId: 1, userId: 2, username: "Player" }, "abcdefghijklmnop"))
      .rejects.toThrow("Account verification is temporarily unavailable");
  });
});
