COMMAND LINE OPTIONS:
	
-autostart=MAP	load the file 'MAP' instead of showing the main menu
-buildarchive	?
-conf:KEY=VALUE	set a config value (overrides the contents of system.cfg)
-entgraph		?
-fixedframe		enable fixed frame timing; ?
-g=F			set the gamma correction to 'F' (default 1.0)
-listfiles		?
-profile=NAME	?
-quickstart		load faster (disables audio and some system info logging)
-shadows		enable shadows
-vsync			enable VSync, i.e. lock FPS to monitor refresh rate
-xres=N			set screen X resolution to 'N'
-yres=N			set screen Y resolution to 'N'

-editor			launch the Atlas scenario editor


Windows-specific:
-wQpcTscSafe	allow timing via QueryPerformanceCounter despite the fact
				that it's using TSC and it may be unsafe. has no effect if
				a better timer (i.e. the HPET) is available.
				should only be specified if:
				- you are sure your system does not engage in
				  thermal throttling (including STPCLK) OR
				- an "RDTSC patch" is installed
				this flag is also useful if all other alternatives are worse
				than a potentially risky or slightly broken TSC-based QPC.

-wNoMahaf		prevent any physical memory mapping or direct port I/O.
				this disables all ACPI-related code and thus some of the
				timer backends. specify this if problems are observed with
				one of the abovementioned subsystems.
