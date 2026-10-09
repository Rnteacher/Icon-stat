# Icon Stat API v1

All responses are JSON except `304 Not Modified`. Errors use `{ "error": "machine_code", "message": "safe description" }`.

## `GET /health`

Returns service and schema status. No authentication.

## `GET /api/v1/players/:accountId/icons`

Returns a published 2.2081 collection, including `total`, per-category `counts`, numerical `icons`, the owner's per-category `equipped` icon IDs, and `synchronizedAt`. Returns `404 not_published` if the account has never synchronized or deleted its data. Supports `If-None-Match`; successful data is publicly cacheable for 60 seconds.

## `PUT /api/v1/me/icons`

Requires `Authorization: Bearer <Argon authtoken>` and a JSON body containing:

```json
{
  "schemaVersion": 1,
  "gameVersion": "2.2081",
  "accountId": 123,
  "userId": 456,
  "username": "Player",
  "icons": {
    "cube": [1], "ship": [1], "ball": [1], "ufo": [1], "wave": [1],
    "robot": [1], "spider": [1], "swing": [1], "jetpack": [1]
  },
  "equipped": {
    "cube": 1, "ship": 1, "ball": 1, "ufo": 1, "wave": 1,
    "robot": 1, "spider": 1, "swing": 1, "jetpack": 1
  }
}
```

The Worker calls Argon's strong-validation endpoint and only writes when account ID, user ID, and username match the token. Every equipped ID must also appear in that category's unlocked list. Collections are replaced atomically with a D1 batch. Repeated synchronization is limited to once per 30 seconds.

## `DELETE /api/v1/me/icons`

Requires the same bearer token and `{ "accountId", "userId", "username" }`. The Worker revalidates ownership before deletion. D1 foreign-key cascade removes icon rows.

## Limits

- Request body: 32 KiB.
- Reads: 120/minute/IP hash.
- Writes: 6/minute/IP hash plus 30-second per-account synchronization spacing.
- Deletes: 3/5 minutes/IP hash.
- IDs must be unique and within the known GD 2.2081 counts for all nine categories.
