COMMAND LINE OPTIONS

Basic gameplay:
-autostart			load a map instead of showing main menu (see below)
-editor				launch the Atlas scenario editor
-mod NAME			start the game using NAME mod
-quickstart			load faster (disables audio and some system info logging)

Autostart:
-autostart=NAME					map NAME for scenario, or rms name for random map
-autostart-ai=PLAYER:AI			adds named AI to the given PLAYER (e.g. 2:testbot)
Multiplayer:
-autostart-playername=NAME		multiplayer local player NAME (default 'anonymous')
-autostart-host					multiplayer host mode
-autostart-players=NUMBER		multiplayer host: NUMBER of client players (default 2)
-autostart-client				multiplayer client mode
-autostart-ip=IP				multiplayer client: connect to this host IP
Random maps only:
-autostart-random				random map
-autostart-random=SEED			random map with SEED value (default 0, use -1 for random)
-autostart-size=TILES			random map SIZE in tiles (default 192)
-autostart-players=NUMBER		NUMBER of players on random map

Configuration:
-conf:KEY=VALUE		set a config value (overrides the contents of system.cfg)
-g=F				set the gamma correction to 'F' (default 1.0)
-nosound			disable audio
-onlyPublicFiles	force game to use only the public (default) mod
-shadows			enable shadows
-vsync				enable VSync, i.e. lock FPS to monitor refresh rate
-xres=N				set screen X resolution to 'N'
-yres=N				set screen Y resolution to 'N'

Advanced / diagnostic:
-dumpSchema			creates a file entity.rng in the working directory, containing
					  complete entity XML schema, used by various analysis tools
-entgraph			(disabled)
-listfiles			(disabled)
-profile=NAME		(disabled)
-replay=PATH		non-visual replay of a previous game, used for analysis purposes
					  PATH is system path to commands.txt containing simulation log
-writableRoot		store runtime game data in root data directory
					  (only use if you have write permissions on that directory)

Windows-specific:
-wQpcTscSafe		allow timing via QueryPerformanceCounter despite the fact
					that it's using TSC and it may be unsafe. has no effect if
					a better timer (i.e. the HPET) is available.
					should only be specified if:
					- you are sure your system does not engage in
					  thermal throttling (including STPCLK) OR
					- an "RDTSC patch" is installed
					this flag is also useful if all other alternatives are worse
					than a potentially risky or slightly broken TSC-based QPC.

-wNoMahaf			prevent any physical memory mapping or direct port I/O.
					this disables all ACPI-related code and thus some of the
					timer backends. specify this if problems are observed with
					one of the abovementioned subsystems.

Archive builder:
-archivebuild=PATH				system PATH of the base directory containing mod data to be archived/precached
-archivebuild-output=PATH		system PATH to output of the resulting .zip archive (use with archivebuild)
-buildarchive					(disabled)

