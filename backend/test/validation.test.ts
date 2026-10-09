import { describe, expect, it } from "vitest";
import { CATEGORIES } from "../src/model";
import { parseAccountId, validateCollection } from "../src/validation";

const valid = () => ({
  schemaVersion: 1,
  gameVersion: "2.2081",
  accountId: 123,
  userId: 456,
  username: "Player_1",
  icons: Object.fromEntries(CATEGORIES.map((category) => [category, [1]])),
  equipped: Object.fromEntries(CATEGORIES.map((category) => [category, 1])),
});

describe("collection validation", () => {
  it("accepts all nine categories and normalizes order", () => {
    const input = valid();
    input.icons.cube = [3, 1, 2];
    expect(validateCollection(input).icons.cube).toEqual([1, 2, 3]);
  });
  it("rejects duplicate IDs", () => {
    const input = valid();
    input.icons.wave = [1, 1];
    expect(() => validateCollection(input)).toThrow(/Duplicate wave/);
  });
  it("rejects out-of-range IDs", () => {
    const input = valid();
    input.icons.jetpack = [9];
    expect(() => validateCollection(input)).toThrow(/outside the 2.2081 range/);
  });
  it("accepts the final 2.2081 cube and rejects IDs beyond it", () => {
    const lastCube = valid();
    lastCube.icons.cube = [485];
    lastCube.equipped.cube = 485;
    expect(validateCollection(lastCube).icons.cube).toEqual([485]);

    const beyondCube = valid();
    beyondCube.icons.cube = [486];
    expect(() => validateCollection(beyondCube)).toThrow(/outside the 2.2081 range/);
  });
  it("accepts all eight jetpacks added through Update 2.207", () => {
    const input = valid();
    input.icons.jetpack = [1, 2, 3, 4, 5, 6, 7, 8];
    expect(validateCollection(input).icons.jetpack).toEqual(input.icons.jetpack);
  });
  it("requires each equipped icon to be owned", () => {
    const input = valid();
    input.icons.cube = [1, 2];
    input.equipped.cube = 3;
    expect(() => validateCollection(input)).toThrow(/not in the unlocked collection/);
  });
  it("rejects missing and unknown categories", () => {
    const missing = valid() as ReturnType<typeof valid> & { icons: Record<string, number[]> };
    delete missing.icons.robot;
    expect(() => validateCollection(missing)).toThrow(/Missing robot/);
    const unknown = valid() as ReturnType<typeof valid> & { icons: Record<string, number[]> };
    unknown.icons.car = [1];
    expect(() => validateCollection(unknown)).toThrow(/Unknown icon category/);
  });
  it("validates account IDs strictly", () => {
    expect(parseAccountId("42")).toBe(42);
    expect(() => parseAccountId("42x")).toThrow(/Invalid account ID/);
    expect(() => parseAccountId("0")).toThrow(/Invalid account ID/);
  });
});
