CREATE TABLE logentry (
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	revision INTEGER UNIQUE,
	author TEXT,
	date TEXT,
	msg TEXT
);

CREATE TABLE paths (
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	logentry INTEGER REFERENCES logentry,
	action TEXT,
	copyfrom_path TEXT,
	copyfrom_rev INTEGER,
	path TEXT
);

CREATE TABLE public_message (
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	logentry INTEGER REFERENCES logentry,
	msg TEXT
);
CREATE INDEX public_message_logentry ON public_message (logentry);

