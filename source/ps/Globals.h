extern bool g_active;

extern bool g_keys[SDLK_LAST];
extern int g_mouse_x, g_mouse_y;

// { left, right, middle, wheel up, wheel down }
// (order is given by SDL_BUTTON_* constants).
extern bool g_mouse_buttons[5];

extern InReaction GlobalsInputHandler(const SDL_Event* ev);
