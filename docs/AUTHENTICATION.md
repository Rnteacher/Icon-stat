# Authentication and account linking

Icon Stat uses [Argon](https://github.com/GlobedGD/argon), the open-source GD authentication system maintained by the Globed team.

1. The mod reads the already logged-in account identity from Geometry Dash locally.
2. Argon performs its account challenge inside the mod and returns an account-scoped authentication token. Icon Stat never prompts for a GD password.
3. The mod sends that token only in the HTTPS `Authorization` header to your Worker.
4. The Worker forwards it to Argon's `check_strong` endpoint with account ID, user ID, and username.
5. Only a response with both `valid` and `valid_weak` true authorizes a write or delete.

The Worker does not store tokens. It stores only verified identity, `auth_provider = 'argon'`, verification time, synchronization time, and the published numerical IDs. The `.geode` contains no backend secret. `RATE_LIMIT_SALT` is a Worker secret used only to pseudonymize source IPs for short-lived rate-limit buckets.

Argon is an independent community service, not an official RobTop authentication API. Its public instance is the practical secure ownership proof available without sending GD passwords to this project. Operators who do not want that external dependency may self-host Argon and change both the client library configuration and Worker `ARGON_BASE_URL`; that is an advanced fork and is not silently enabled here.
