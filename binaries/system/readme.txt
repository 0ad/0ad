COMMAND LINE OPTIONS

Basic gameplay:
-autostart=...      load a map instead of showing main menu (see below)
-editor             launch the Atlas scenario editor
-mod=NAME           start the game using NAME mod
-quickstart         load faster (disables audio and some system info logging)

Autostart:
-autostart="TYPEDIR/MAPNAME"    enables autostart and sets MAPNAME; TYPEDIR is skirmishes, scenarios, or random
-autostart-ai=PLAYER:AI         sets the AI for PLAYER (e.g. 2:petra)
-autostart-aidiff=PLAYER:DIFF   sets the DIFFiculty of PLAYER's AI (0: sandbox, 5: very hard)
-autostart-aiseed=AISEED        sets the seed used for the AI random generator (default 0, use -1 for random)
-autostart-civ=PLAYER:CIV       sets PLAYER's civilisation to CIV (skirmish and random maps only)
-autostart-team=PLAYER:TEAM     sets the team for PLAYER (e.g. 2:2).
Multiplayer:
-autostart-playername=NAME      sets local player NAME (default 'anonymous')
-autostart-host                 sets multiplayer host mode
-autostart-host-players=NUMBER  sets NUMBER of human players for multiplayer game (default 2)
-autostart-client=IP            sets multiplayer client to join host at given IP address
 Random maps only:
-autostart-seed=SEED            sets random map SEED value (default 0, use -1 for random)
-autostart-size=TILES           sets random map size in TILES (default 192)
-autostart-players=NUMBER       sets NUMBER of players on random map (default 2)

Examples:
1) "Bob" will host a 2 player game on the Arcadia map:
 -autostart="scenarios/Arcadia 02" -autostart-host -autostart-host-players=2 -autostart-playername="Bob"
2) Load Alpine Lakes random map with random seed, 2 players (Athens and Britons), and player 2 is PetraBot:
 -autostart="random/alpine_lakes" -autostart-seed=-1 -autostart-players=2 -autostart-civ=1:athen -autostart-civ=2:brit -autostart-ai=2:petra

Configuration:
-conf=KEY:VALUE     set a config value
-g=F                set the gamma correction to 'F' (default 1.0)
-nosound            disable audio
-noUserMod          disable loading of the user mod
-shadows            enable shadows
-vsync              enable VSync, i.e. lock FPS to monitor refresh rate
-xres=N             set screen X resolution to 'N'
-yres=N             set screen Y resolution to 'N'

Advanced / diagnostic:
-version            print the version of the engine and exit
-dumpSchema         creates a file entity.rng in the working directory, containing
                      complete entity XML schema, used by various analysis tools
-entgraph           (disabled)
-listfiles          (disabled)
-profile=NAME       (disabled)
-replay=PATH        non-visual replay of a previous game, used for analysis purposes
                      PATH is system path to commands.txt containing simulation log
-replay-visual=PATH visual replay of a previous game, used for analysis purposes
                      PATH is system path to commands.txt containing simulation log
-writableRoot       store runtime game data in root data directory
                      (only use if you have write permissions on that directory)
-ooslog             dumps simulation state in binary and ASCII representations each turn,
                    files created in sim_log within the game's log folder. NOTE: game will
                    run much slower with this option!
-serializationtest  checks simulation state each turn for serialization errors; on test
                    failure, error is displayed and logs created in oos_log within the
                    game's log folder. NOTE: game will run much slower with this option!

Windows-specific:
-wQpcTscSafe        allow timing via QueryPerformanceCounter despite the fact
                    that it's using TSC and it may be unsafe. has no effect if
                    a better timer (i.e. the HPET) is available.
                    should only be specified if:
                    - you are sure your system does not engage in
                      thermal throttling (including STPCLK) OR
                    - an "RDTSC patch" is installed
                    this flag is also useful if all other alternatives are worse
                    than a potentially risky or slightly broken TSC-based QPC.

-wNoMahaf           prevent any physical memory mapping or direct port I/O.
                    this disables all ACPI-related code and thus some of the
                    timer backends. specify this if problems are observed with
                    one of the abovementioned subsystems.

Archive builder:
-archivebuild=PATH            system PATH of the base directory containing mod data to be archived/precached
                                specify all mods it depends on with -mod=NAME
-archivebuild-output=PATH     system PATH to output of the resulting .zip archive (use with archivebuild)
-archivebuild-compress        enable deflate compression in the .zip
                                (no zip compression by default since it hurts compression of release packages)
