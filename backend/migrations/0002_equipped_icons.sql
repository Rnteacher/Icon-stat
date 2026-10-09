PRAGMA foreign_keys = ON;

CREATE TABLE equipped_icons (
    account_id INTEGER NOT NULL,
    category TEXT NOT NULL CHECK (category IN ('cube','ship','ball','ufo','wave','robot','spider','swing','jetpack')),
    icon_id INTEGER NOT NULL CHECK (icon_id > 0),
    PRIMARY KEY (account_id, category),
    FOREIGN KEY (account_id, category, icon_id)
        REFERENCES player_icons(account_id, category, icon_id)
        ON DELETE CASCADE
);

CREATE INDEX idx_equipped_icons_account ON equipped_icons(account_id);
