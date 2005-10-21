extern bool g_active;

extern bool g_keys[SDLK_LAST];
extern int g_mouse_x, g_mouse_y;


/**
 * g_mouse_buttons: Mouse buttons states, indexed by SDL_BUTTON_* constants. If an entry 
 * is true, it represents a pressed button.
 * 
 * Be aware that SDL_BUTTON_* constants start at 1. Therefore, g_mouse_buttons[0] is unused.
 * Thus, the order of entries is { unused, left, right, middle, wheel up, wheel down }
 */
extern bool g_mouse_buttons[6];

extern InReaction GlobalsInputHandler(const SDL_Event* ev);
