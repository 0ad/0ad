
//
// GUI integration
//

extern void GUI_Init();

extern void GUI_Shutdown();

extern void GUI_ShowMainMenu();

// display progress / description in loading screen
extern void GUI_DisplayLoadProgress(int percent, const wchar_t* pending_task);



extern void Render();
extern void RenderActor();


/**
 * initialize global modules that are be needed before Init.
 * must be called from the very beginning of main.
 **/
extern void EarlyInit();

enum InitFlags
{
	// avoid setting a video mode / initializing OpenGL; assume that has
	// already been done and everything is ready for rendering.
	// needed by map editor because it creates its own window.
	INIT_HAVE_VMODE = 1,

	// skip initializing the in-game GUI.
	// needed by map editor because it uses its own GUI.
	INIT_NO_GUI = 2,

	// skip initializing the simulation.
	// used by actor viewer because it doesn't need the simulation code.
	INIT_NO_SIM = 4
};

class CmdLineArgs;
extern void Init(const CmdLineArgs& args, int flags);
extern void Shutdown(int flags);
