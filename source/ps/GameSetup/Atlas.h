// free reference to Atlas UI SO (avoids resource leak report)
extern void ATLAS_Shutdown();

// starts the Atlas UI if an "-editor" switch is found on the command line.
// this is the alternative to starting the main menu and clicking on
// the editor button; it is much faster because it's called during early
// init and therefore skips GUI setup.
extern void ATLAS_RunIfOnCmdLine(int argc, char* argv[]);
