#include "precompiled.h"

#include "Hotkey.h"
#include "input.h"
#include "ConfigDB.h"

bool hotkeys[HOTKEY_LAST];
extern bool keys[SDLK_LAST];
extern bool mouseButtons[5];

struct SHotkeyMapping
{
	int mapsTo;
	std::vector<int> requires;
};

static std::vector<SHotkeyMapping> hotkeyMap[SDLK_LAST + 5];

const int MOUSE_LEFT = SDLK_LAST + SDL_BUTTON_LEFT;
const int MOUSE_RIGHT = SDLK_LAST + SDL_BUTTON_RIGHT;
const int MOUSE_MIDDLE = SDLK_LAST + SDL_BUTTON_MIDDLE;
const int MOUSE_WHEELUP = SDLK_LAST + SDL_BUTTON_WHEELUP;
const int MOUSE_WHEELDOWN = SDLK_LAST + SDL_BUTTON_WHEELDOWN;

struct SHotkeyInfo
{
	int code;
	const char* name;
	int defaultmapping1, defaultmapping2;
};

static SHotkeyInfo hotkeyInfo[] =
{
	{ HOTKEY_EXIT, "exit", SDLK_ESCAPE, 0 },
	{ HOTKEY_SCREENSHOT, "screenshot", SDLK_PRINT, 0 },
	{ HOTKEY_WIREFRAME, "wireframe", SDLK_w, 0 },
	{ HOTKEY_CAMERA_RESET, "camera.reset", SDLK_h, 0 },
	{ HOTKEY_CAMERA_ZOOM_IN, "camera.zoom.in", SDLK_PLUS, SDLK_KP_PLUS },
	{ HOTKEY_CAMERA_ZOOM_OUT, "camera.zoom.out", SDLK_MINUS, SDLK_KP_MINUS },
	{ HOTKEY_CAMERA_ZOOM_WHEEL_IN, "camera.zoom.wheel.in", MOUSE_WHEELUP, 0 },
	{ HOTKEY_CAMERA_ZOOM_WHEEL_OUT, "camera.zoom.wheel.out", MOUSE_WHEELDOWN, 0 },
	{ HOTKEY_CAMERA_ROTATE, "camera.rotate", 0, 0 },
	{ HOTKEY_CAMERA_PAN, "camera.pan", MOUSE_MIDDLE, 0 },
	{ HOTKEY_CAMERA_PAN_LEFT, "camera.pan.left", SDLK_LEFT, 0 },
	{ HOTKEY_CAMERA_PAN_RIGHT, "camera.pan.right", SDLK_RIGHT, 0 },
	{ HOTKEY_CAMERA_PAN_FORWARD, "camera.pan.forward", SDLK_UP, 0 },
	{ HOTKEY_CAMERA_PAN_BACKWARD, "camera.pan.backward", SDLK_DOWN, 0 },
	{ HOTKEY_CONSOLE_TOGGLE, "console.toggle", SDLK_F1, 0 },
	{ HOTKEY_SELECTION_ADD, "selection.add", SDLK_LSHIFT, SDLK_RSHIFT },
	{ HOTKEY_SELECTION_REMOVE, "selection.remove", SDLK_LCTRL, SDLK_RCTRL },
	{ HOTKEY_SELECTION_GROUP_ADD, "selection.group.add", SDLK_LSHIFT, SDLK_RSHIFT },
	{ HOTKEY_SELECTION_GROUP_SAVE, "selection.group.save", SDLK_LCTRL, SDLK_RCTRL },
	{ HOTKEY_SELECTION_GROUP_SNAP, "selection.group.snap", SDLK_LALT, SDLK_RALT },
	{ HOTKEY_SELECTION_SNAP, "selection.snap", SDLK_HOME, 0 },
	{ HOTKEY_CONTEXTORDER_NEXT, "contextorder.next", SDLK_RIGHTBRACKET, 0 },
	{ HOTKEY_CONTEXTORDER_PREVIOUS, "contextorder.previous", SDLK_LEFTBRACKET, 0 },
	{ HOTKEY_HIGHLIGHTALL, "highlightall", SDLK_o, 0 }
};

void loadHotkeys()
{
	initKeyNameMap();

	CParser multikeyParser;
	multikeyParser.InputTaskType( "multikey", "<_$value_+_>_$value" );

	for( int i = 0; i < HOTKEY_LAST; i++ )
	{
		CStr hotkeyname( "hotkey." );
		hotkeyname += hotkeyInfo[i].name;

		CConfigValueSet* binding = g_ConfigDB.GetValues( CFG_SYSTEM, hotkeyname );

		if( binding )
		{
			int mapping;
			
			CConfigValueSet::iterator it;

			// Iterate through the bindings for this event...

			for( it = binding->begin(); it != binding->end(); it++ )
			{
				std::string hotkey;
				if( it->GetString( hotkey ) )
				{
					std::vector<int> keyCombination;

					CParserLine multikeyIdentifier;
					multikeyIdentifier.ParseString( multikeyParser, hotkey );

					// Iterate through multiple-key bindings (e.g. Ctrl+I)

					for( size_t t = 0; t < multikeyIdentifier.GetArgCount(); t++ )
					{
						
						if( multikeyIdentifier.GetArgString( (int)t, hotkey ) )
						{
							mapping = getKeyCode( hotkey );		// Attempt decode as key name
							if( !mapping )
								if( !it->GetInt( mapping ) )	// Attempt decode as key code
									continue;
							keyCombination.push_back( mapping );
						}
					}
					
					std::vector<int>::iterator itKey, itKey2;

					SHotkeyMapping bind;

					for( itKey = keyCombination.begin(); itKey != keyCombination.end(); itKey++ )
					{
						bind.mapsTo = i;
						bind.requires.clear();
						for( itKey2 = keyCombination.begin(); itKey2 != keyCombination.end(); itKey2++ )
						{
							// Push any auxiliary keys.
							if( itKey != itKey2 )
								bind.requires.push_back( *itKey2 );
						}
						hotkeyMap[*itKey].push_back( bind );
					}
				}
			}
		}
		else
		{
			SHotkeyMapping bind[2];
			bind[0].mapsTo = i;
			bind[1].mapsTo = i;
			bind[0].requires.clear();
			bind[1].requires.clear();
			hotkeyMap[ hotkeyInfo[i].defaultmapping1 ].push_back( bind[0] );
			if( hotkeyInfo[i].defaultmapping2 )
				hotkeyMap[ hotkeyInfo[i].defaultmapping2 ].push_back( bind[1] );
		}
	}
}

int hotkeyInputHandler( const SDL_Event* ev )
{
	int keycode;

	switch( ev->type )
	{
	case SDL_KEYDOWN:
	case SDL_KEYUP:
		keycode = (int)ev->key.keysym.sym;
		break;
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		keycode = SDLK_LAST + (int)ev->button.button;
		break;
	default:
		return( EV_PASS );
	}

	std::vector<SHotkeyMapping>::iterator it;
	SDL_Event hotkeyNotification;

	if( ( ev->type == SDL_KEYDOWN ) || ( ev->type == SDL_MOUSEBUTTONDOWN ) )
	{
		for( it = hotkeyMap[keycode].begin(); it < hotkeyMap[keycode].end(); it++ )
		{			
			// Check to see if all auxiliary keys are down

			std::vector<int>::iterator itKey;
			bool accept = true;

			for( itKey = it->requires.begin(); itKey != it->requires.end(); itKey++ )
			{
				if( *itKey < SDLK_LAST )
				{
					if( !keys[*itKey] ) accept = false;
				}
				else
				{
					if( !mouseButtons[(*itKey)-SDLK_LAST] ) accept = false;
				}
			}

			if( accept )
			{
				hotkeys[it->mapsTo] = true;
				hotkeyNotification.type = SDL_HOTKEYDOWN;
				hotkeyNotification.user.code = it->mapsTo;
				SDL_PushEvent( &hotkeyNotification );
			}
		}
	}
	else
	{
		for( it = hotkeyMap[keycode].begin(); it < hotkeyMap[keycode].end(); it++ )
		{
			// Check to see if all auxiliary keys are down

			std::vector<int>::iterator itKey;
			bool accept = true;

			for( itKey = it->requires.begin(); itKey != it->requires.end(); itKey++ )
			{
				if( *itKey < SDLK_LAST )
				{
					if( !keys[*itKey] ) accept = false;
				}
				else
				{
					if( !mouseButtons[(*itKey)-SDLK_LAST] ) accept = false;
				}
			}

			if( accept )
			{
				hotkeys[it->mapsTo] = false;
				hotkeyNotification.type = SDL_HOTKEYUP;
				hotkeyNotification.user.code = it->mapsTo;
				SDL_PushEvent( &hotkeyNotification );
			}
		}
	}
	return( EV_PASS );
}
