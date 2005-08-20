
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

extern void Init(int argc, char* argv[], bool setup_gfx, bool setup_gui);
