#include "lib/input.h"
#include "lib/external_libraries/sdl.h"

// thin abstraction layer on top of SDL.
// game code should use it instead of SDL_GetMouseState etc. because
// Atlas does not completely emulate SDL (it can only send events).

extern bool g_app_minimized;
extern bool g_app_has_focus;

extern int g_mouse_x, g_mouse_y;

/**
 * g_keys: Key states, indexed by SDLK* constants. If an entry is true,
 * it represents a pressed key.
 * Updated by GlobalsInputHandler in response to key press/release events.
 */
extern bool g_keys[SDLK_LAST];

/**
 * g_mouse_buttons: Mouse buttons states, indexed by SDL_BUTTON_* constants.
 * If an entry is true, it represents a pressed button.
 * Updated by GlobalsInputHandler in response to mouse button up/down events.
 * 
 * Be aware that SDL_BUTTON_* constants start at 1. Therefore,
 * g_mouse_buttons[0] is unused. The order of entries is:
 * { unused, left, right, middle, wheel up, wheel down }
 */
extern bool g_mouse_buttons[6];

extern InReaction GlobalsInputHandler(const SDL_Event_* ev);
