
//----------------------------------------------------------------------------
// GUI integration
//----------------------------------------------------------------------------


extern void GUI_Init();

extern void GUI_Shutdown();

extern void GUI_ShowMainMenu();

// display progress / description in loading screen
extern void GUI_DisplayLoadProgress(int percent, const wchar_t* pending_task);



extern void Render();

extern void Shutdown();

// If setup_videmode is false, it is assumed that the video mode has already
// been set up and is ready for rendering.
extern void Init(int argc, char* argv[], bool setup_videomode, bool setup_gui);
