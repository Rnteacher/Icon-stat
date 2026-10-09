# Isolated in-game test checklist

No installed Geometry Dash or Geode files were used during development. Perform these checks only by manually installing the artifact into a disposable test profile.

- Start with publishing off; confirm no upload occurs.
- Open an unpublished player's profile; confirm no Icon Stat node appears.
- Enable publishing and manually synchronize; complete Argon authentication and verify all nine totals against the garage.
- Confirm a locked ID never appears and a selected-but-locked test condition is not treated as owned.
- Open own and remote profiles; confirm async loading does not stall UI and the stat animates in once.
- Click every category and View All; verify scrolling, empty state, large cube collection, resize/fullscreen transitions, and back/close behavior.
- Click an icon the viewer does not own; confirm `ItemInfoPopup` describes the clicked remote ID.
- Repeat with Better Unlock Info 1.3.2 and Colorful Icons 1.0.10 independently and together.
- Disconnect the network; verify cached data appears only within the configured TTL and failures do not create a stat.
- Force HTTP 404, 429, 500, malformed JSON, and timeout responses against a local Worker.
- Synchronize twice within 30 seconds; expect a safe 429.
- Attempt PUT/DELETE with no token, wrong account, weak-only identity, and invalid IDs; expect rejection with no database change.
- Delete published data, then verify GET returns 404 and the stat disappears after cache clear/expiry.
