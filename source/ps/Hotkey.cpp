#include "precompiled.h"

#include "Hotkey.h"
#include "input.h"
#include "ConfigDB.h"
#include "CConsole.h"
#include "CStr.h"

extern CConsole* g_Console;
extern bool keys[SDLK_LAST];
extern bool mouseButtons[5];

/* SDL-type */

struct SHotkeyMapping
{
	int mapsTo;
	std::vector<int> requires;
};

typedef std::vector<SHotkeyMapping> KeyMapping;

// A mapping of keycodes onto sets of SDL event codes
static KeyMapping hotkeyMap[SDLK_LAST + 5];

// An array of the status of virtual keys
bool hotkeys[HOTKEY_LAST];

// 'Keycodes' for the mouse buttons
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
	{ HOTKEY_CAMERA_ROTATE_ABOUT_TARGET, "camera.rotate.abouttarget", 0, 0 },
	{ HOTKEY_CAMERA_PAN, "camera.pan", MOUSE_MIDDLE, 0 },
	{ HOTKEY_CAMERA_PAN_LEFT, "camera.pan.left", SDLK_LEFT, 0 },
	{ HOTKEY_CAMERA_PAN_RIGHT, "camera.pan.right", SDLK_RIGHT, 0 },
	{ HOTKEY_CAMERA_PAN_FORWARD, "camera.pan.forward", SDLK_UP, 0 },
	{ HOTKEY_CAMERA_PAN_BACKWARD, "camera.pan.backward", SDLK_DOWN, 0 },
	{ HOTKEY_CAMERA_BOOKMARK_MODIFIER, "camera.bookmark.modifier", 0, 0 },
	{ HOTKEY_CAMERA_BOOKMARK_SAVE, "camera.bookmark.save", 0, 0 },
	{ HOTKEY_CAMERA_BOOKMARK_SNAP, "camera.bookmark.snap", 0, 0 },
	{ HOTKEY_CONSOLE_TOGGLE, "console.toggle", SDLK_F1, 0 },
	{ HOTKEY_SELECTION_ADD, "selection.add", SDLK_LSHIFT, SDLK_RSHIFT },
	{ HOTKEY_SELECTION_REMOVE, "selection.remove", SDLK_LCTRL, SDLK_RCTRL },
	{ HOTKEY_SELECTION_GROUP_ADD, "selection.group.add", SDLK_LSHIFT, SDLK_RSHIFT },
	{ HOTKEY_SELECTION_GROUP_SAVE, "selection.group.save", SDLK_LCTRL, SDLK_RCTRL },
	{ HOTKEY_SELECTION_GROUP_SNAP, "selection.group.snap", SDLK_LALT, SDLK_RALT },
	{ HOTKEY_SELECTION_SNAP, "selection.snap", SDLK_HOME, 0 },
	{ HOTKEY_ORDER_QUEUE, "order.queue", SDLK_LSHIFT, SDLK_RSHIFT },
	{ HOTKEY_CONTEXTORDER_NEXT, "contextorder.next", SDLK_RIGHTBRACKET, 0 },
	{ HOTKEY_CONTEXTORDER_PREVIOUS, "contextorder.previous", SDLK_LEFTBRACKET, 0 },
	{ HOTKEY_HIGHLIGHTALL, "highlightall", SDLK_o, 0 }
};

/* SDL-type ends */

/* GUI-type */

struct SHotkeyMappingGUI
{
	CStr mapsTo;
	std::vector<int> requires;
};

typedef std::vector<SHotkeyMappingGUI> GuiMapping;

// A mapping of keycodes onto sets of hotkey name strings (e.g. '[hotkey.]camera.reset')
static GuiMapping hotkeyMapGUI[SDLK_LAST + 5];

typedef std::vector<CStr> GUIObjectList; // A list of GUI objects
typedef std::map<CStr,GUIObjectList> GUIHotkeyMap; // A mapping of name strings to lists of GUI objects that they trigger

static GUIHotkeyMap guiHotkeyMap;

// Look up a key binding in the config file and set the mappings for
// all key combinations that trigger it.

void setBindings( const CStr& hotkeyName, int integerMapping = -1 )
{
	CConfigValueSet* binding = g_ConfigDB.GetValues( CFG_SYSTEM, CStr( "hotkey." ) + hotkeyName );
	if( binding )
	{
		int mapping;
		
		CConfigValueSet::iterator it;
		CParser multikeyParser;
		multikeyParser.InputTaskType( "multikey", "<_$value_+_>_$value" );

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

				SHotkeyMapping bindCode;
				SHotkeyMappingGUI bindName;

				for( itKey = keyCombination.begin(); itKey != keyCombination.end(); itKey++ )
				{
					bindName.mapsTo = hotkeyName;
					bindName.requires.clear();
					if( integerMapping != -1 )
					{
						bindCode.mapsTo = integerMapping;
						bindCode.requires.clear();
					}
					for( itKey2 = keyCombination.begin(); itKey2 != keyCombination.end(); itKey2++ )
					{
						// Push any auxiliary keys.
						if( itKey != itKey2 )
						{
							bindName.requires.push_back( *itKey2 );
							if( integerMapping != -1 )
								bindCode.requires.push_back( *itKey2 );
						}
					}
					hotkeyMapGUI[*itKey].push_back( bindName );
					if( integerMapping != -1 )
						hotkeyMap[*itKey].push_back( bindCode );
				}
			}
		}
	}
	else if( integerMapping != -1 )
	{
		SHotkeyMapping bind[2];
		bind[0].mapsTo = integerMapping;
		bind[1].mapsTo = integerMapping;
		bind[0].requires.clear();
		bind[1].requires.clear();
		hotkeyMap[ hotkeyInfo[integerMapping].defaultmapping1 ].push_back( bind[0] );
		if( hotkeyInfo[integerMapping].defaultmapping2 )
			hotkeyMap[ hotkeyInfo[integerMapping].defaultmapping2 ].push_back( bind[1] );
	}
}
void loadHotkeys()
{
	initKeyNameMap();

	for( int i = 0; i < HOTKEY_LAST; i++ )
		setBindings( hotkeyInfo[i].name, i );
	
}

void hotkeyRegisterGUIObject( const CStr& objName, const CStr& hotkeyName )
{
	GUIObjectList& boundTo = guiHotkeyMap[hotkeyName];
	if( boundTo.empty() )
	{
		// Load keybindings from the config file
		setBindings( hotkeyName );
	}
	boundTo.push_back( objName );
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

	// Inhibit the dispatch of hotkey events caused by printable or control keys
	// while the console is up. 

	if( g_Console->IsActive() && (
	    ( keycode == 8 ) || ( keycode == 9 ) || ( keycode == 13 ) || /* Editing */
		( ( keycode >= 32 ) && ( keycode < 282 ) ) ) )				 /* Printable (<128), 'World' (<256) */
		return( EV_PASS );											 /* Numeric keypad (<273) and Navigation (<282) */

	std::vector<SHotkeyMapping>::iterator it;
	std::vector<SHotkeyMappingGUI>::iterator itGUI;

	SDL_Event hotkeyNotification;

	if( ( ev->type == SDL_KEYDOWN ) || ( ev->type == SDL_MOUSEBUTTONDOWN ) )
	{
		// SDL-events bit
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
		// GUI bit... could do with some optimization later.
		for( itGUI = hotkeyMapGUI[keycode].begin(); itGUI != hotkeyMapGUI[keycode].end(); itGUI++ )
		{			
			// Check to see if all auxiliary keys are down

			std::vector<int>::iterator itKey;
			bool accept = true;

			for( itKey = itGUI->requires.begin(); itKey != itGUI->requires.end(); itKey++ )
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
				// GUI-objects bit
				// This fragment is an obvious candidate for rewriting when speed becomes an issue.
				GUIHotkeyMap::iterator map_it;
				GUIObjectList::iterator obj_it;
				map_it = guiHotkeyMap.find( itGUI->mapsTo );
				if( map_it != guiHotkeyMap.end() )
				{
					GUIObjectList& targets = map_it->second;
					for( obj_it = targets.begin(); obj_it != targets.end(); obj_it++ )
					{
						hotkeyNotification.type = SDL_GUIHOTKEYPRESS;
						hotkeyNotification.user.code = (intptr_t)&(*obj_it);
						SDL_PushEvent( &hotkeyNotification );
					}
				}	
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
