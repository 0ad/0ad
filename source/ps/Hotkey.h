// Hotkey.h
// 
// Constant definitions and a couple of exports for the hotkey processor
//
// Mark Thompson (mot20@cam.ac.uk / mark@wildfiregames.com)
//
// Hotkeys can be mapped onto SDL events (for use internal to the engine),
// or used to trigger activation of GUI buttons.
//
// Adding a hotkey (SDL event type):
//
// - Define your constant in the enum, just below;
// - Add an entry to hotkeyInfo[], in Hotkey.cpp
//     first column is this constant, second is the config string (minus 'hotkey.') it maps to
//     third and fourth are the default keys, used if the config file doesn't contain that string.
// - Create an input handler for SDL_HOTKEYDOWN, SDL_HOTKEYUP, or poll the hotkeys[] array.
//     For SDL_HOTKEYDOWN, SDL_HOTKEYUP, the constant is passed in as ev->user.code.
// - Add some bindings to the config file.

#include "precompiled.h"
#include "sdl.h"
#include "CStr.h"

const int SDL_HOTKEYDOWN = SDL_USEREVENT;
const int SDL_HOTKEYUP = SDL_USEREVENT + 1;
const int SDL_GUIHOTKEYPRESS = SDL_USEREVENT + 2;

enum
{
	HOTKEY_EXIT,
	HOTKEY_SCREENSHOT,
	HOTKEY_WIREFRAME,
	HOTKEY_CAMERA_RESET,
	HOTKEY_CAMERA_ZOOM_IN,
	HOTKEY_CAMERA_ZOOM_OUT,
	HOTKEY_CAMERA_ZOOM_WHEEL_IN,
	HOTKEY_CAMERA_ZOOM_WHEEL_OUT,
	HOTKEY_CAMERA_ROTATE,
	HOTKEY_CAMERA_ROTATE_ABOUT_TARGET,
	HOTKEY_CAMERA_PAN,
	HOTKEY_CAMERA_PAN_LEFT,
	HOTKEY_CAMERA_PAN_RIGHT,
	HOTKEY_CAMERA_PAN_FORWARD,
	HOTKEY_CAMERA_PAN_BACKWARD,
	HOTKEY_CAMERA_BOOKMARK_MODIFIER,
	HOTKEY_CAMERA_BOOKMARK_SAVE,
	HOTKEY_CAMERA_BOOKMARK_SNAP,
	HOTKEY_CONSOLE_TOGGLE,
	HOTKEY_SELECTION_ADD,
	HOTKEY_SELECTION_REMOVE,
	HOTKEY_SELECTION_GROUP_ADD,
	HOTKEY_SELECTION_GROUP_SAVE,
	HOTKEY_SELECTION_GROUP_SNAP,
	HOTKEY_SELECTION_SNAP,
	HOTKEY_ORDER_QUEUE,
	HOTKEY_CONTEXTORDER_NEXT,
	HOTKEY_CONTEXTORDER_PREVIOUS,
	HOTKEY_HIGHLIGHTALL,
	HOTKEY_LAST,
};

void loadHotkeys();
int hotkeyInputHandler( const SDL_Event* ev );
void hotkeyRegisterGUIObject( const CStr& objName, const CStr& hotkeyName );

void initKeyNameMap();
CStr getKeyName( int keycode );
int getKeyCode( CStr keyname );

extern bool hotkeys[HOTKEY_LAST];