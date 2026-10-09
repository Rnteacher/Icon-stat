# Icon Stat

Icon Stat is a Windows x64 Geode mod for Geometry Dash 2.2081 and Geode 5.10.1, plus a Cloudflare Worker/D1 backend. It publishes a verified player's exact unlocked icon IDs and adds a clickable total to profiles for other users of the mod.

The valid mod ID is `axolodlaat.icon-stat`: current Geode IDs allow lowercase ASCII letters, periods, underscores, and hyphens.

## Architecture

```text
GD 2.2081 + Geode mod
  ├─ GameManager::countForType + isIconUnlocked  ──> exact local collection
  ├─ Argon challenge ──> account-scoped token
  └─ async Geode WebRequest over HTTPS
             │
             ▼
Cloudflare Worker (validation, strong Argon check, rate limits)
             │
             ▼
Cloudflare D1
  ├─ players (verified identity, totals, timestamps, schema)
  ├─ player_icons (account, category, numerical icon ID)
  └─ equipped_icons (account, category, selected icon ID)
```

Public reads are cached locally by the mod and briefly at the HTTP edge. The profile stat is not created for a `404 not_published`, invalid response, or expired cache with a network failure.

## Implemented

- Exact local unlock enumeration for Cube, Ship, Ball, UFO, Wave, Robot, Spider, Swing, and Jetpack using the 2.2081 `GameManager` APIs.
- Async profile lookup and TTL cache with no game-thread blocking.
- Native `GJItemIcon` gallery cells and native `ItemInfoPopup` on click.
- Native selection highlights identify the profile owner's equipped icon in every gallery.
- Nine-category breakdown, counts, total, scrolling gallery, View All, empty state, and popup back/close behavior.
- Settings for enable, publishing, automatic sync, profile visibility, cache TTL, server URL, manual sync, deletion, and cache clearing.
- Argon-authenticated write/delete operations; no GD password and no shared client secret.
- D1 migration, prepared statements, normalized icon rows, indexes, validation, request limits, and privacy-conscious error logging.
- Optional compatibility declarations for Better Unlock Info and Colorful Icons.

## Compatibility rationale

Research was performed against the public sources for [Better Unlock Info](https://github.com/Rynat86/Better-Unlock-Info) (`rynat.better_unlock_info`) and [Colorful Icons](https://github.com/flurrybun/colorful-icons) (`ninkaz.colorful-icons`).

Icon Stat does not replace `ItemInfoPopup::init`, `GJItemIcon::init`, or `SimplePlayer`. Gallery items use `GJItemIcon::createBrowserItem`, so Colorful Icons receives its normal rendering hook. Clicks call `ItemInfoPopup::create`, so Better Unlock Info receives its normal enhancement hook. The profile addition has a namespaced node ID and does not replace or reparent vanilla statistics.

## Repository layout

```text
backend/       Worker TypeScript, tests, Wrangler config, D1 migration
mod/           Geode C++ mod, metadata, settings, build output
docs/          API, authentication, and manual in-game test documentation
scripts/       isolated dependency bootstrap
vendor/        reproducible upstream checkouts and CMake cache (gitignored)
build.ps1      isolated Windows mod build
test.ps1       backend checks
```

## Backend setup

Prerequisites: Node.js, pnpm, and a Cloudflare account with Wrangler authenticated. Nothing in these steps deploys until you explicitly run the final deploy command.

1. Install packages:

   ```powershell
   cd backend
   pnpm install --frozen-lockfile
   ```

2. Create D1 (this changes your Cloudflare account but does not deploy the Worker):

   ```powershell
   pnpm exec wrangler d1 create icon-stat
   ```

3. Copy the returned database ID into `backend/wrangler.toml` in place of `REPLACE_WITH_YOUR_D1_DATABASE_ID`.

4. Create the rate-limit hashing secret:

   ```powershell
   pnpm exec wrangler secret put RATE_LIMIT_SALT
   ```

   Enter a randomly generated value of at least 32 bytes. Do not put it in source control.

5. Apply the migration locally and test:

   ```powershell
   pnpm run db:migrate:local
   pnpm run dev
   ```

6. When ready to change the remote D1 database:

   ```powershell
   pnpm run db:migrate:remote
   ```

7. Deployment is deliberately manual and was **not performed**:

   ```powershell
   pnpm run deploy
   ```

8. Put the resulting `https://icon-stat-api.<subdomain>.workers.dev` URL in the mod's Server URL setting. For a public native mod, `ALLOWED_ORIGIN = "null"` is intentional; CORS is not an authentication boundary. Argon authentication protects mutations.

See [API.md](docs/API.md) and [AUTHENTICATION.md](docs/AUTHENTICATION.md).

## Mod build

Prerequisites: Visual Studio 2022 Build Tools with Desktop C++, CMake 3.29+, Ninja, Git, and Geode CLI. The bootstrap downloads only public upstream sources into this project and installs SDK binaries into `vendor/geode`.

```powershell
.\scripts\bootstrap.ps1
.\build.ps1
```

The reproducible build uses `RelWithDebInfo` with precompiled headers disabled. MSVC 19.44 encounters an internal compiler error while optimizing Argon/Arc templates, so CMake applies `/Od` only to the small Argon dependency; Icon Stat and the remaining SDK code stay optimized. Clang remains recommended when available.

Before distribution, replace the placeholder source URL in `mod/mod.json` if the project has a public repository. The default Worker URL is `https://icon-stat-api.dlaat-nw.workers.dev` and remains user-configurable at runtime.

## Tests

```powershell
.\test.ps1
```

The automated suite covers schema/category/range validation, duplicate IDs, account IDs, missing auth, weak-only identity rejection, and strong authentication acceptance. TypeScript strict checking covers the Worker/D1 contract. The C++ source is compiled against the actual Geode 5.10.1 SDK and generated GD 2.2081 bindings.

No in-game test was claimed or performed. Follow [MANUAL_TESTING.md](docs/MANUAL_TESTING.md) in a disposable manually configured installation.

## Privacy and security

- Publishing defaults to off.
- The Icon Stat Worker never asks for or receives a GD password.
- The Worker never stores an Argon token.
- Only numerical icon IDs and minimal verified profile metadata are public.
- Source IPs are salted and hashed solely for expiring D1 rate-limit buckets; raw IPs are not intentionally logged.
- All SQL values use D1 prepared-statement bindings.
- Keep Cloudflare secrets in Wrangler's secret store, never in `mod.json` or the `.geode` package.

## Known limitations

- Game icon counts are authoritative at collection time, while the backend's defensive caps are fixed to known 2.2081 ranges. A future GD icon update requires matching client/backend changes.
- Native page artwork is constructed through the game's native `GJItemIcon` renderer with the default icon for each `UnlockType`; no copyrighted game textures are redistributed.
- The View All gallery uses one lightweight native node per unlocked icon. It scrolls correctly for current 2.2081 counts, but it is not a virtualized list.
- A precise in-game overlap/resize and compatibility pass requires manual testing in an isolated GD installation, which was intentionally not accessed or launched.
- Argon is a community authentication service, not an official RobTop OAuth service; details and self-hosting implications are documented in `docs/AUTHENTICATION.md`.
- The provided artifact is `RelWithDebInfo`; only Argon is unoptimized due to the local MSVC optimizer crash noted above.

## Research references

- [Geode 5.10.1 release](https://github.com/geode-sdk/geode/releases/tag/v5.10.1)
- [Geode 2.2081 bindings](https://github.com/geode-sdk/bindings)
- [Geode async web requests](https://docs.geode-sdk.org/tutorials/fetch/)
- [Argon authentication](https://github.com/GlobedGD/argon)
- [Better Unlock Info](https://github.com/Rynat86/Better-Unlock-Info)
- [Colorful Icons](https://github.com/flurrybun/colorful-icons)
