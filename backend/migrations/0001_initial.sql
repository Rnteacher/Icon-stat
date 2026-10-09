PRAGMA foreign_keys = ON;

CREATE TABLE players (
    account_id INTEGER PRIMARY KEY CHECK (account_id > 0),
    user_id INTEGER NOT NULL CHECK (user_id > 0),
    username TEXT NOT NULL CHECK (length(username) BETWEEN 3 AND 20),
    total_icons INTEGER NOT NULL CHECK (total_icons >= 0),
    game_version TEXT NOT NULL,
    schema_version INTEGER NOT NULL CHECK (schema_version = 1),
    auth_provider TEXT NOT NULL CHECK (auth_provider = 'argon'),
    verified_at TEXT NOT NULL,
    synchronized_at TEXT NOT NULL
);

CREATE TABLE player_icons (
    account_id INTEGER NOT NULL REFERENCES players(account_id) ON DELETE CASCADE,
    category TEXT NOT NULL CHECK (category IN ('cube','ship','ball','ufo','wave','robot','spider','swing','jetpack')),
    icon_id INTEGER NOT NULL CHECK (icon_id > 0),
    PRIMARY KEY (account_id, category, icon_id)
);

CREATE INDEX idx_player_icons_account_category ON player_icons(account_id, category, icon_id);
CREATE INDEX idx_players_sync ON players(synchronized_at);

CREATE TABLE rate_limits (
    key TEXT NOT NULL,
    window_start INTEGER NOT NULL,
    request_count INTEGER NOT NULL CHECK (request_count > 0),
    PRIMARY KEY (key, window_start)
);
CREATE INDEX idx_rate_limits_window ON rate_limits(window_start);
