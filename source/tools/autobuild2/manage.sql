CREATE TABLE activity (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp DEFAULT CURRENT_TIMESTAMP NOT NULL,
    user TEXT NOT NULL,
    ip TEXT NOT NULL,
    ua TEXT NOT NULL,
    action TEXT NOT NULL, -- like "index", "start", "stop", "console"
    params TEXT -- action-dependent - typically just the instance ID
);

CREATE TABLE instances (
    instance_id TEXT NOT NULL PRIMARY KEY,
    local_launch_time TEXT NOT NULL
);

CREATE TABLE state (
    key TEXT NOT NULL PRIMARY KEY,
    value TEXT NOT NULL
);
INSERT INTO state VALUES ('process_mutex', '');
INSERT INTO state VALUES ('last_start', '0');
