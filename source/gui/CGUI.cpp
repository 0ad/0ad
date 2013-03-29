/* Copyright (C) 2013 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
CGUI
*/

#include "precompiled.h"

#include <string>

#include <stdarg.h>

#include "GUI.h"

// Types - when including them into the engine.
#include "CButton.h"
#include "CImage.h"
#include "CText.h"
#include "CCheckBox.h"
#include "CRadioButton.h"
#include "CInput.h"
#include "CList.h"
#include "CDropDown.h"
#include "CProgressBar.h"
#include "CTooltip.h"
#include "MiniMap.h"
#include "scripting/JSInterface_GUITypes.h"

#include "graphics/ShaderManager.h"
#include "graphics/TextRenderer.h"
#include "lib/input.h"
#include "lib/bits.h"
#include "lib/timer.h"
#include "lib/sysdep/sysdep.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/Font.h"
#include "ps/Hotkey.h"
#include "ps/Globals.h"
#include "ps/Overlay.h"
#include "ps/Profile.h"
#include "ps/Pyrogenesis.h"
#include "ps/XML/Xeromyces.h"
#include "renderer/Renderer.h"
#include "scripting/ScriptingHost.h"
#include "scriptinterface/ScriptInterface.h"

extern int g_yres;

const double SELECT_DBLCLICK_RATE = 0.5;

void CGUI::ScriptingInit()
{
	JSI_IGUIObject::init();
	JSI_GUITypes::init();
}

InReaction CGUI::HandleEvent(const SDL_Event_* ev)
{
	InReaction ret = IN_PASS;

	if (ev->ev.type == SDL_HOTKEYDOWN)
	{
		const char* hotkey = static_cast<const char*>(ev->ev.user.data1);
		std::map<CStr, std::vector<IGUIObject*> >::iterator it = m_HotkeyObjects.find(hotkey);
		if (it != m_HotkeyObjects.end())
		{
			for (size_t i = 0; i < it->second.size(); ++i)
			{
				it->second[i]->SendEvent(GUIM_PRESSED, "press");
			}
		}
	}

	else if (ev->ev.type == SDL_MOUSEMOTION)
	{
		// Yes the mouse position is stored as float to avoid
		//  constant conversions when operating in a
		//  float-based environment.
		m_MousePos = CPos((float)ev->ev.motion.x, (float)ev->ev.motion.y);

		SGUIMessage msg(GUIM_MOUSE_MOTION);
		GUI<SGUIMessage>::RecurseObject(GUIRR_HIDDEN | GUIRR_GHOST, m_BaseObject, 
										&IGUIObject::HandleMessage, 
										msg);
	}

	// Update m_MouseButtons. (BUTTONUP is handled later.)
	else if (ev->ev.type == SDL_MOUSEBUTTONDOWN)
	{
		switch (ev->ev.button.button)
		{
		case SDL_BUTTON_LEFT:
		case SDL_BUTTON_RIGHT:
		case SDL_BUTTON_MIDDLE:
			m_MouseButtons |= Bit<unsigned int>(ev->ev.button.button);
			break;
		default:
			break;
		}
	}

	// Update m_MousePos (for delayed mouse button events)
	CPos oldMousePos = m_MousePos;
	if (ev->ev.type == SDL_MOUSEBUTTONDOWN || ev->ev.type == SDL_MOUSEBUTTONUP)
	{
		m_MousePos = CPos((float)ev->ev.button.x, (float)ev->ev.button.y);
	}

	// Only one object can be hovered
	IGUIObject *pNearest = NULL;

	// TODO Gee: (2004-09-08) Big TODO, don't do the below if the SDL_Event is something like a keypress!
	try
	{
		PROFILE( "mouse events" );
		// TODO Gee: Optimizations needed!
		//  these two recursive function are quite overhead heavy.

		// pNearest will after this point at the hovered object, possibly NULL
		pNearest = FindObjectUnderMouse();

		// Is placed in the UpdateMouseOver function
		//if (ev->ev.type == SDL_MOUSEMOTION && pNearest)
		//	pNearest->ScriptEvent("mousemove");

		// Now we'll call UpdateMouseOver on *all* objects,
		//  we'll input the one hovered, and they will each
		//  update their own data and send messages accordingly
		
		GUI<IGUIObject*>::RecurseObject(GUIRR_HIDDEN | GUIRR_GHOST, m_BaseObject, 
										&IGUIObject::UpdateMouseOver, 
										pNearest);

		if (ev->ev.type == SDL_MOUSEBUTTONDOWN)
		{
			switch (ev->ev.button.button)
			{
			case SDL_BUTTON_LEFT:
				// Focus the clicked object (or focus none if nothing clicked on)
				SetFocusedObject(pNearest);

				if (pNearest)
					ret = pNearest->SendEvent(GUIM_MOUSE_PRESS_LEFT, "mouseleftpress");
				break;

			case SDL_BUTTON_RIGHT:
				if (pNearest)
					ret = pNearest->SendEvent(GUIM_MOUSE_PRESS_RIGHT, "mouserightpress");
				break;

#if !SDL_VERSION_ATLEAST(2, 0, 0)
			case SDL_BUTTON_WHEELDOWN: // wheel down
				if (pNearest)
					ret = pNearest->SendEvent(GUIM_MOUSE_WHEEL_DOWN, "mousewheeldown");
				break;

			case SDL_BUTTON_WHEELUP: // wheel up
				if (pNearest)
					ret = pNearest->SendEvent(GUIM_MOUSE_WHEEL_UP, "mousewheelup");
				break;
#endif
			default:
				break;
			}
		}
#if SDL_VERSION_ATLEAST(2, 0, 0)
		else if (ev->ev.type == SDL_MOUSEWHEEL)
		{
			if (ev->ev.wheel.y < 0)
			{
				if (pNearest)
					ret = pNearest->SendEvent(GUIM_MOUSE_WHEEL_DOWN, "mousewheeldown");
			}
			else if (ev->ev.wheel.y > 0)
			{
				if (pNearest)
					ret = pNearest->SendEvent(GUIM_MOUSE_WHEEL_UP, "mousewheelup");
			}
		}
#endif
		else if (ev->ev.type == SDL_MOUSEBUTTONUP)
		{
			switch (ev->ev.button.button)
			{
			case SDL_BUTTON_LEFT:
				if (pNearest)
				{
					double timeElapsed = timer_Time() - pNearest->m_LastClickTime[SDL_BUTTON_LEFT];
					pNearest->m_LastClickTime[SDL_BUTTON_LEFT] = timer_Time();
					
					//Double click?
					if (timeElapsed < SELECT_DBLCLICK_RATE)
					{
						ret = pNearest->SendEvent(GUIM_MOUSE_DBLCLICK_LEFT, "mouseleftdoubleclick");
					}
					else
					{
						ret = pNearest->SendEvent(GUIM_MOUSE_RELEASE_LEFT, "mouseleftrelease");
					}
				}
				break;
			case SDL_BUTTON_RIGHT:
				if (pNearest)
				{
					double timeElapsed = timer_Time() - pNearest->m_LastClickTime[SDL_BUTTON_RIGHT];
					pNearest->m_LastClickTime[SDL_BUTTON_RIGHT] = timer_Time();
					
					//Double click?
					if (timeElapsed < SELECT_DBLCLICK_RATE)
					{
						ret = pNearest->SendEvent(GUIM_MOUSE_DBLCLICK_RIGHT, "mouserightdoubleclick");
					}
					else
					{
						ret = pNearest->SendEvent(GUIM_MOUSE_RELEASE_RIGHT, "mouserightrelease");
					}
				}
				break;
			}

			// Reset all states on all visible objects
			GUI<>::RecurseObject(GUIRR_HIDDEN, m_BaseObject, 
									&IGUIObject::ResetStates);

			// It will have reset the mouse over of the current hovered, so we'll
			//  have to restore that
			if (pNearest)
				pNearest->m_MouseHovering = true;
		}
	}
	catch (PSERROR_GUI& e)
	{
		UNUSED2(e);
		debug_warn(L"CGUI::HandleEvent error");
		// TODO Gee: Handle
	}

	// BUTTONUP's effect on m_MouseButtons is handled after
	// everything else, so that e.g. 'press' handlers (activated
	// on button up) see which mouse button had been pressed.
	if (ev->ev.type == SDL_MOUSEBUTTONUP)
	{
		switch (ev->ev.button.button)
		{
		case SDL_BUTTON_LEFT:
		case SDL_BUTTON_RIGHT:
		case SDL_BUTTON_MIDDLE:
			m_MouseButtons &= ~Bit<unsigned int>(ev->ev.button.button);
			break;
		default:
			break;
		}
	}

	// Restore m_MousePos (for delayed mouse button events)
	if (ev->ev.type == SDL_MOUSEBUTTONDOWN || ev->ev.type == SDL_MOUSEBUTTONUP)
	{
		m_MousePos = oldMousePos;
	}

	// Handle keys for input boxes
	if (GetFocusedObject())
	{
		if (
			(ev->ev.type == SDL_KEYDOWN &&
				ev->ev.key.keysym.sym != SDLK_ESCAPE &&
				!g_keys[SDLK_LCTRL] && !g_keys[SDLK_RCTRL] &&
				!g_keys[SDLK_LALT] && !g_keys[SDLK_RALT]) 
			|| ev->ev.type == SDL_HOTKEYDOWN
			)
		{
			ret = GetFocusedObject()->ManuallyHandleEvent(ev);
		}
		// else will return IN_PASS because we never used the button.
	}

	return ret;
}

void CGUI::TickObjects()
{
	CStr action = "tick";
	GUI<CStr>::RecurseObject(0, m_BaseObject, 
							&IGUIObject::ScriptEvent, action);

	// Also update tooltips:
	m_Tooltip.Update(FindObjectUnderMouse(), m_MousePos, this);
}

void CGUI::SendEventToAll(const CStr& EventName)
{
	// janwas 2006-03-03: spoke with Ykkrosh about EventName case.
	// when registering, case is converted to lower - this avoids surprise
	// if someone were to get the case wrong and then not notice their
	// handler is never called. however, until now, the other end
	// (sending events here) wasn't converting to lower case,
	// leading to a similar problem.
	// now fixed; case is irrelevant since all are converted to lower.
	GUI<CStr>::RecurseObject(0, m_BaseObject, 
		&IGUIObject::ScriptEvent, EventName.LowerCase());
}

//-------------------------------------------------------------------
//  Constructor / Destructor
//-------------------------------------------------------------------

// To isolate the vars declared by each GUI page, we need to create
// a pseudo-global object to declare them in. In particular, it must
// have no parent object, so it must be declared with JS_NewGlobalObject.
// But GUI scripts should have access to the real global's properties
// (Array, undefined, getGUIObjectByName, etc), so we add a custom resolver
// that defers to the real global object when necessary.

static JSBool GetGlobalProperty(JSContext* cx, JSObject* UNUSED(obj), jsid id, jsval* vp)
{
	return JS_GetPropertyById(cx, g_ScriptingHost.GetGlobalObject(), id, vp);
}

static JSBool SetGlobalProperty(JSContext* cx, JSObject* UNUSED(obj), jsid id, JSBool UNUSED(strict), jsval* vp)
{
	return JS_SetPropertyById(cx, g_ScriptingHost.GetGlobalObject(), id, vp);
}

static JSBool ResolveGlobalProperty(JSContext* cx, JSObject* obj, jsid id, uintN flags, JSObject** objp)
{
	// This gets called when the property can't be resolved in the page_global object.

	// Warning: The interaction between this resolution stuff and the JITs appears
	// to be quite fragile, and I don't quite understand what the constraints are.
	// If changing it, be careful to test with each JIT to make sure it works.
	// (This code is somewhat based on GPSEE's module system.)

	// Declarations and assignments shouldn't affect the real global
	if (flags & (JSRESOLVE_DECLARING | JSRESOLVE_ASSIGNING))
	{
		// Can't be resolved - return NULL
		*objp = NULL;
		return JS_TRUE;
	}

	// Check whether the real global object defined this property
	uintN attrs;
	JSBool found;
	if (!JS_GetPropertyAttrsGetterAndSetterById(cx, g_ScriptingHost.GetGlobalObject(), id, &attrs, &found, NULL, NULL))
		return JS_FALSE;

	if (!found)
	{
		// Not found on real global, so can't be resolved - return NULL
		*objp = NULL;
		return JS_TRUE;
	}

	// Retrieve the property value from the global
	jsval v;
	if (!JS_GetPropertyById(cx, g_ScriptingHost.GetGlobalObject(), id, &v))
		return JS_FALSE;

	// Add the global's property value onto this object, with getter/setter that will
	// update the global's copy instead of this copy, and then return this object
	if (!JS_DefinePropertyById(cx, obj, id, v, GetGlobalProperty, SetGlobalProperty, attrs))
		return JS_FALSE;

	*objp = obj;
	return JS_TRUE;
}

static JSClass page_global_class = {
	"page_global", JSCLASS_GLOBAL_FLAGS | JSCLASS_NEW_RESOLVE,
	JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
	JS_EnumerateStub, (JSResolveOp)ResolveGlobalProperty, JS_ConvertStub, JS_FinalizeStub,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL
};

CGUI::CGUI() : m_MouseButtons(0), m_FocusedObject(NULL), m_InternalNameNumber(0)
{
	m_BaseObject = new CGUIDummyObject;
	m_BaseObject->SetGUI(this);

	// Construct the root object for all GUI JavaScript things
	m_ScriptObject = JS_NewGlobalObject(g_ScriptingHost.getContext(), &page_global_class);
	JS_AddObjectRoot(g_ScriptingHost.getContext(), &m_ScriptObject);
}

CGUI::~CGUI()
{
	Destroy();

	if (m_BaseObject)
		delete m_BaseObject;

	if (m_ScriptObject)
	{
		// Let it be garbage-collected
		JS_RemoveObjectRoot(g_ScriptingHost.getContext(), &m_ScriptObject);
	}
}

//-------------------------------------------------------------------
//  Functions
//-------------------------------------------------------------------
IGUIObject *CGUI::ConstructObject(const CStr& str)
{
	if (m_ObjectTypes.count(str) > 0)
		return (*m_ObjectTypes[str])();
	else
	{
		// Error reporting will be handled with the NULL return.
		return NULL;
	}
}

void CGUI::Initialize()
{
	// Add base types!
	//  You can also add types outside the GUI to extend the flexibility of the GUI.
	//  Pyrogenesis though will have all the object types inserted from here.
	AddObjectType("empty",			&CGUIDummyObject::ConstructObject);
	AddObjectType("button",			&CButton::ConstructObject);
	AddObjectType("image",			&CImage::ConstructObject);
	AddObjectType("text",			&CText::ConstructObject);
	AddObjectType("checkbox",		&CCheckBox::ConstructObject);
	AddObjectType("radiobutton",	&CRadioButton::ConstructObject);
	AddObjectType("progressbar",	&CProgressBar::ConstructObject);
	AddObjectType("minimap",        &CMiniMap::ConstructObject);
	AddObjectType("input",			&CInput::ConstructObject);
	AddObjectType("list",			&CList::ConstructObject);
	AddObjectType("dropdown",		&CDropDown::ConstructObject);
	AddObjectType("tooltip",		&CTooltip::ConstructObject);
}

void CGUI::Draw()
{
	// Clear the depth buffer, so the GUI is
	// drawn on top of everything else
	glClear(GL_DEPTH_BUFFER_BIT);

	try
	{
		// Recurse IGUIObject::Draw() with restriction: hidden
		//  meaning all hidden objects won't call Draw (nor will it recurse its children)
		GUI<>::RecurseObject(GUIRR_HIDDEN, m_BaseObject, &IGUIObject::Draw);
	}
	catch (PSERROR_GUI& e)
	{
		LOGERROR(L"GUI draw error: %hs", e.what());
	}
}

void CGUI::DrawSprite(const CGUISpriteInstance& Sprite,
					  int CellID,
					  const float& Z,
					  const CRect& Rect,
					  const CRect& UNUSED(Clipping))
{
	// If the sprite doesn't exist (name == ""), don't bother drawing anything
	if (Sprite.IsEmpty())
		return;

	// TODO: Clipping?

	Sprite.Draw(Rect, CellID, m_Sprites, Z);
}

void CGUI::Destroy()
{
	// We can use the map to delete all
	//  now we don't want to cancel all if one Destroy fails
	map_pObjects::iterator it;
	for (it = m_pAllObjects.begin(); it != m_pAllObjects.end(); ++it)
	{
		try
		{
			it->second->Destroy();
		}
		catch (PSERROR_GUI& e)
		{
			UNUSED2(e);
			debug_warn(L"CGUI::Destroy error");
			// TODO Gee: Handle
		}
		
		delete it->second;
	}

	for (std::map<CStr, CGUISprite>::iterator it2 = m_Sprites.begin(); it2 != m_Sprites.end(); ++it2)
		for (std::vector<SGUIImage>::iterator it3 = it2->second.m_Images.begin(); it3 != it2->second.m_Images.end(); ++it3)
			delete it3->m_Effects;

	// Clear all
	m_pAllObjects.clear();
	m_Sprites.clear();
	m_Icons.clear();
}

void CGUI::UpdateResolution()
{
	// Update ALL cached
	GUI<>::RecurseObject(0, m_BaseObject, &IGUIObject::UpdateCachedSize );
}

void CGUI::AddObject(IGUIObject* pObject)
{
	try
	{
		// Add CGUI pointer
		GUI<CGUI*>::RecurseObject(0, pObject, &IGUIObject::SetGUI, this);

		// Add child to base object
		m_BaseObject->AddChild(pObject); // can throw

		// Cache tree
		GUI<>::RecurseObject(0, pObject, &IGUIObject::UpdateCachedSize);

		// Loaded
		SGUIMessage msg(GUIM_LOAD);
		GUI<SGUIMessage>::RecurseObject(0, pObject, &IGUIObject::HandleMessage, msg);
	}
	catch (PSERROR_GUI&)
	{
		throw;
	}
}

void CGUI::UpdateObjects()
{
	// We'll fill a temporary map until we know everything
	//  succeeded
	map_pObjects AllObjects;

	try
	{
		// Fill freshly
		GUI< map_pObjects >::RecurseObject(0, m_BaseObject, &IGUIObject::AddToPointersMap, AllObjects );
	}
	catch (PSERROR_GUI&)
	{
		// Throw the same error
		throw;
	}

	// Else actually update the real one
	m_pAllObjects.swap(AllObjects);
}

bool CGUI::ObjectExists(const CStr& Name) const
{
	return m_pAllObjects.count(Name) != 0;
}

IGUIObject* CGUI::FindObjectByName(const CStr& Name) const
{
	map_pObjects::const_iterator it = m_pAllObjects.find(Name);
	if (it == m_pAllObjects.end())
		return NULL;
	else
		return it->second;
}

IGUIObject* CGUI::FindObjectUnderMouse() const
{
	IGUIObject* pNearest = NULL;
	GUI<IGUIObject*>::RecurseObject(GUIRR_HIDDEN | GUIRR_GHOST, m_BaseObject,
									&IGUIObject::ChooseMouseOverAndClosest,
									pNearest);
	return pNearest;
}

void CGUI::SetFocusedObject(IGUIObject* pObject)
{
	if (pObject == m_FocusedObject)
		return;

	if (m_FocusedObject)
	{
		SGUIMessage msg(GUIM_LOST_FOCUS);
		m_FocusedObject->HandleMessage(msg);
	}

	m_FocusedObject = pObject;

	if (m_FocusedObject)
	{
		SGUIMessage msg(GUIM_GOT_FOCUS);
		m_FocusedObject->HandleMessage(msg);
	}
}

// private struct used only in GenerateText(...)
struct SGenerateTextImage
{
	float m_YFrom,			// The image's starting location in Y
		  m_YTo,			// The image's end location in Y
		  m_Indentation;	// The image width in other words

	// Some help functions
	// TODO Gee: CRect => CPoint ?
	void SetupSpriteCall(const bool Left, SGUIText::SSpriteCall &SpriteCall, 
						 const float width, const float y,
						 const CSize &Size, const CStr& TextureName, 
						 const float BufferZone, const int CellID)
	{
		// TODO Gee: Temp hardcoded values
		SpriteCall.m_Area.top = y+BufferZone;
		SpriteCall.m_Area.bottom = y+BufferZone + Size.cy;
		
		if (Left)
		{
			SpriteCall.m_Area.left = BufferZone;
			SpriteCall.m_Area.right = Size.cx+BufferZone;
		}
		else
		{
			SpriteCall.m_Area.left = width-BufferZone - Size.cx;
			SpriteCall.m_Area.right = width-BufferZone;
		}

		SpriteCall.m_CellID = CellID;
		SpriteCall.m_Sprite = TextureName;

		m_YFrom = SpriteCall.m_Area.top-BufferZone;
		m_YTo = SpriteCall.m_Area.bottom+BufferZone;
		m_Indentation = Size.cx+BufferZone*2;
	}
};

SGUIText CGUI::GenerateText(const CGUIString &string,
							const CStrW& Font, const float &Width, const float &BufferZone,
							const IGUIObject *pObject)
{
	SGUIText Text; // object we're generating
	
	if (string.m_Words.size() == 0)
		return Text;

	float x=BufferZone, y=BufferZone; // drawing pointer
	int from=0;
	bool done=false;

	bool FirstLine = true;	// Necessary because text in the first line is shorter
							// (it doesn't count the line spacing)

	// Images on the left or the right side.
	std::vector<SGenerateTextImage> Images[2];
	int pos_last_img=-1;	// Position in the string where last img (either left or right) were encountered.
							//  in order to avoid duplicate processing.

	// Easier to read.
	bool WordWrapping = (Width != 0);

	// get the alignment type for the control we are computing the text for since
	// we are computing the horizontal alignment in this method in order to not have
	// to run through the TextCalls a second time in the CalculateTextPosition method again
	EAlign align;
	GUI<EAlign>::GetSetting(pObject, "text_align", align);

	// Go through string word by word
	for (int i=0; i<(int)string.m_Words.size()-1 && !done; ++i)
	{
		// Pre-process each line one time, so we know which floating images
		//  will be added for that line.

		// Generated stuff is stored in Feedback.
		CGUIString::SFeedback Feedback;

		// Preliminary line_height, used for word-wrapping with floating images.
		float prelim_line_height=0.f;

		// Width and height of all text calls generated.
		string.GenerateTextCall(Feedback, Font,
								string.m_Words[i], string.m_Words[i+1],
								FirstLine);

		// Loop through our images queues, to see if images has been added.
		
		// Check if this has already been processed.
		//  Also, floating images are only applicable if Word-Wrapping is on
		if (WordWrapping && i > pos_last_img)
		{
			// Loop left/right
			for (int j=0; j<2; ++j)
			{
				for (std::vector<CStr>::const_iterator it = Feedback.m_Images[j].begin(); 
					it != Feedback.m_Images[j].end();
					++it)
				{
					SGUIText::SSpriteCall SpriteCall;
					SGenerateTextImage Image;

					// Y is if no other floating images is above, y. Else it is placed
					//  after the last image, like a stack downwards.
					float _y;
					if (Images[j].size() > 0)
						_y = std::max(y, Images[j].back().m_YTo);
					else
						_y = y; 

					// Get Size from Icon database
					SGUIIcon icon = GetIcon(*it);

					CSize size = icon.m_Size;
					Image.SetupSpriteCall((j==CGUIString::SFeedback::Left), SpriteCall, Width, _y, size, icon.m_SpriteName, BufferZone, icon.m_CellID);

					// Check if image is the lowest thing.
					Text.m_Size.cy = std::max(Text.m_Size.cy, Image.m_YTo);

					Images[j].push_back(Image);
					Text.m_SpriteCalls.push_back(SpriteCall);
				}
			}
		}

		pos_last_img = std::max(pos_last_img, i);

		x += Feedback.m_Size.cx;
		prelim_line_height = std::max(prelim_line_height, Feedback.m_Size.cy);

		// If Width is 0, then there's no word-wrapping, disable NewLine.
		if ((WordWrapping && (x > Width-BufferZone || Feedback.m_NewLine)) || i == (int)string.m_Words.size()-2)
		{
			// Change 'from' to 'i', but first keep a copy of its value.
			int temp_from = from;
			from = i;

			static const int From=0, To=1;
			//int width_from=0, width_to=width;
			float width_range[2];
			width_range[From] = BufferZone;
			width_range[To] = Width - BufferZone;

			// Floating images are only applicable if word-wrapping is enabled.
			if (WordWrapping)
			{
				// Decide width of the line. We need to iterate our floating images.
				//  this won't be exact because we're assuming the line_height
				//  will be as our preliminary calculation said. But that may change,
				//  although we'd have to add a couple of more loops to try straightening
				//  this problem out, and it is very unlikely to happen noticeably if one
				//  structures his text in a stylistically pure fashion. Even if not, it
				//  is still quite unlikely it will happen.
				// Loop through left and right side, from and to.
				for (int j=0; j<2; ++j)
				{
					for (std::vector<SGenerateTextImage>::const_iterator it = Images[j].begin(); 
						it != Images[j].end(); 
						++it)
					{
						// We're working with two intervals here, the image's and the line height's.
						//  let's find the union of these two.
						float union_from, union_to;

						union_from = std::max(y, it->m_YFrom);
						union_to = std::min(y+prelim_line_height, it->m_YTo);
						
						// The union is not empty
						if (union_to > union_from)
						{
							if (j == From)
								width_range[From] = std::max(width_range[From], it->m_Indentation);
							else
								width_range[To] = std::min(width_range[To], Width - it->m_Indentation);
						}
					}
				}
			}

			// Reset X for the next loop
			x = width_range[From];

			// Now we'll do another loop to figure out the height and width of
			//  the line (the height of the largest character and the width is
			//  the sum of all of the individual widths). This
			//  couldn't be determined in the first loop (main loop)
			//  because it didn't regard images, so we don't know
			//  if all characters processed, will actually be involved
			//  in that line.
			float line_height=0.f;
			float line_width=0.f;
			for (int j=temp_from; j<=i; ++j)
			{
				// We don't want to use Feedback now, so we'll have to use
				//  another.
				CGUIString::SFeedback Feedback2;

				// Don't attach object, it'll suppress the errors
				//  we want them to be reported in the final GenerateTextCall()
				//  so that we don't get duplicates.
				string.GenerateTextCall(Feedback2, Font,
										string.m_Words[j], string.m_Words[j+1],
										FirstLine);

				// Append X value.
				x += Feedback2.m_Size.cx;

				if (WordWrapping && x > width_range[To] && j!=temp_from && !Feedback2.m_NewLine)
					break;

				// Let line_height be the maximum m_Height we encounter.
				line_height = std::max(line_height, Feedback2.m_Size.cy);

				line_width += Feedback2.m_Size.cx;

				if (WordWrapping && Feedback2.m_NewLine)
					break;
			}

			float dx = 0.f;
			// compute offset based on what kind of alignment
			switch (align)
			{
			case EAlign_Left:
				// don't add an offset
				dx = 0.f;
				break;

			case EAlign_Center:
				dx = ((width_range[To] - width_range[From]) - line_width) / 2;
				break;

			case EAlign_Right:
				dx = width_range[To] - line_width;
				break;

			default:
				debug_warn(L"Broken EAlign in CGUI::GenerateText()");
				break;
			}
			// Reset x once more
			x = width_range[From];
			// Move down, because font drawing starts from the baseline
			y += line_height;

			// Do the real processing now
			for (int j=temp_from; j<=i; ++j)
			{
				// We don't want to use Feedback now, so we'll have to use
				//  another one.
				CGUIString::SFeedback Feedback2;

				// Defaults
				string.GenerateTextCall(Feedback2, Font,
										string.m_Words[j], string.m_Words[j+1], 
										FirstLine, pObject);

				// Iterate all and set X/Y values
				// Since X values are not set, we need to make an internal
				//  iteration with an increment that will append the internal
				//  x, that is what x_pointer is for.
				float x_pointer=0.f;

				std::vector<SGUIText::STextCall>::iterator it;
				for (it = Feedback2.m_TextCalls.begin(); it != Feedback2.m_TextCalls.end(); ++it)
				{
					it->m_Pos = CPos(dx + x + x_pointer, y);

					x_pointer += it->m_Size.cx;

					if (it->m_pSpriteCall)
					{
						it->m_pSpriteCall->m_Area += it->m_Pos - CSize(0,it->m_pSpriteCall->m_Area.GetHeight());
					}
				}

				// Append X value.
				x += Feedback2.m_Size.cx;

				Text.m_Size.cx = std::max(Text.m_Size.cx, x+BufferZone);

				// The first word overrides the width limit, what we
				//  do, in those cases, are just drawing that word even
				//  though it'll extend the object.
				if (WordWrapping) // only if word-wrapping is applicable
				{
					if (Feedback2.m_NewLine)
					{
						from = j+1;

						// Sprite call can exist within only a newline segment,
						//  therefore we need this.
						Text.m_SpriteCalls.insert(Text.m_SpriteCalls.end(), Feedback2.m_SpriteCalls.begin(), Feedback2.m_SpriteCalls.end());
						break;
					}
					else
					if (x > width_range[To] && j==temp_from)
					{
						from = j+1;
						// do not break, since we want it to be added to m_TextCalls
					}
					else
					if (x > width_range[To])
					{
						from = j;
						break;
					}
				}

				// Add the whole Feedback2.m_TextCalls to our m_TextCalls.
				Text.m_TextCalls.insert(Text.m_TextCalls.end(), Feedback2.m_TextCalls.begin(), Feedback2.m_TextCalls.end());
				Text.m_SpriteCalls.insert(Text.m_SpriteCalls.end(), Feedback2.m_SpriteCalls.begin(), Feedback2.m_SpriteCalls.end());

				if (j == (int)string.m_Words.size()-2)
					done = true;
			}

			// Reset X
			x = 0.f;

			// Update height of all
			Text.m_Size.cy = std::max(Text.m_Size.cy, y+BufferZone);

			FirstLine = false;

			// Now if we entered as from = i, then we want
			//  i being one minus that, so that it will become
			//  the same i in the next loop. The difference is that
			//  we're on a new line now.
			i = from-1;
		}
	}

	return Text;
}

void CGUI::DrawText(SGUIText &Text, const CColor &DefaultColor, 
					const CPos &pos, const float &z, const CRect &clipping)
{
	CShaderTechniquePtr tech = g_Renderer.GetShaderManager().LoadEffect("gui_text");

	tech->BeginPass();

	if (clipping != CRect())
	{
		glEnable(GL_SCISSOR_TEST);
		glScissor(clipping.left, g_yres - clipping.bottom, clipping.GetWidth(), clipping.GetHeight());
	}

	CTextRenderer textRenderer(tech->GetShader());
	textRenderer.Translate(0.0f, 0.0f, z);

	for (std::vector<SGUIText::STextCall>::const_iterator it = Text.m_TextCalls.begin(); 
		 it != Text.m_TextCalls.end(); 
		 ++it)
	{
		// If this is just a placeholder for a sprite call, continue
		if (it->m_pSpriteCall)
			continue;

		CColor color = it->m_UseCustomColor ? it->m_Color : DefaultColor;

		textRenderer.Color(color);
		textRenderer.Font(it->m_Font);
		textRenderer.Put((float)(int)(pos.x+it->m_Pos.x), (float)(int)(pos.y+it->m_Pos.y), it->m_String.c_str());
	}

	textRenderer.Render();

	for (std::list<SGUIText::SSpriteCall>::iterator it=Text.m_SpriteCalls.begin(); 
		 it!=Text.m_SpriteCalls.end(); 
		 ++it)
	{
		DrawSprite(it->m_Sprite, it->m_CellID, z, it->m_Area + pos);
	}

	if (clipping != CRect())
		glDisable(GL_SCISSOR_TEST);

	tech->EndPass();
}

bool CGUI::GetPreDefinedColor(const CStr& name, CColor &Output)
{
	if (m_PreDefinedColors.count(name) == 0)
	{
		return false;
	}
	else
	{
		Output = m_PreDefinedColors[name];
		return true;
	}
}

/**
 * @callgraph
 */
void CGUI::LoadXmlFile(const VfsPath& Filename, boost::unordered_set<VfsPath>& Paths)
{
	Paths.insert(Filename);

	CXeromyces XeroFile;
	if (XeroFile.Load(g_VFS, Filename) != PSRETURN_OK)
		// Fail silently
		return;

	XMBElement node = XeroFile.GetRoot();

	// Check root element's (node) name so we know what kind of
	//  data we'll be expecting
	CStr root_name (XeroFile.GetElementString(node.GetNodeName()));

	try
	{

		if (root_name == "objects")
		{
			Xeromyces_ReadRootObjects(node, &XeroFile, Paths);

			// Re-cache all values so these gets cached too.
			//UpdateResolution();
		}
		else
		if (root_name == "sprites")
		{
			Xeromyces_ReadRootSprites(node, &XeroFile);
		}
		else
		if (root_name == "styles")
		{
			Xeromyces_ReadRootStyles(node, &XeroFile);
		}
		else
		if (root_name == "setup")
		{
			Xeromyces_ReadRootSetup(node, &XeroFile);
		}
		else
		{
			debug_warn(L"CGUI::LoadXmlFile error");
			// TODO Gee: Output in log
		}
	}
	catch (PSERROR_GUI& e)
	{
		LOGERROR(L"Errors loading GUI file %ls (%u)", Filename.string().c_str(), e.getCode());
		return;
	}
}

//===================================================================
//	XML Reading Xeromyces Specific Sub-Routines
//===================================================================

void CGUI::Xeromyces_ReadRootObjects(XMBElement Element, CXeromyces* pFile, boost::unordered_set<VfsPath>& Paths)
{
	int el_script = pFile->GetElementID("script");

	std::vector<std::pair<CStr, CStr> > subst;

	// Iterate main children
	//  they should all be <object> or <script> elements
	XMBElementList children = Element.GetChildNodes();
	for (int i=0; i<children.Count; ++i)
	{
		//debug_printf(L"Object %d\n", i);
		XMBElement child = children.Item(i);

		if (child.GetNodeName() == el_script)
			// Execute the inline script
			Xeromyces_ReadScript(child, pFile, Paths);
		else
			// Read in this whole object into the GUI
			Xeromyces_ReadObject(child, pFile, m_BaseObject, subst, Paths);
	}
}

void CGUI::Xeromyces_ReadRootSprites(XMBElement Element, CXeromyces* pFile)
{
	// Iterate main children
	//  they should all be <sprite> elements
	XMBElementList children = Element.GetChildNodes();
	for (int i=0; i<children.Count; ++i)
	{
		XMBElement child = children.Item(i);

		// Read in this whole object into the GUI
		Xeromyces_ReadSprite(child, pFile);
	}
}

void CGUI::Xeromyces_ReadRootStyles(XMBElement Element, CXeromyces* pFile)
{
	// Iterate main children
	//  they should all be <styles> elements
	XMBElementList children = Element.GetChildNodes();
	for (int i=0; i<children.Count; ++i)
	{
		XMBElement child = children.Item(i);

		// Read in this whole object into the GUI
		Xeromyces_ReadStyle(child, pFile);
	}
}

void CGUI::Xeromyces_ReadRootSetup(XMBElement Element, CXeromyces* pFile)
{
	// Iterate main children
	//  they should all be <icon>, <scrollbar> or <tooltip>.
	XMBElementList children = Element.GetChildNodes();
	for (int i=0; i<children.Count; ++i)
	{
		XMBElement child = children.Item(i);

		// Read in this whole object into the GUI

		CStr name (pFile->GetElementString(child.GetNodeName()));

		if (name == "scrollbar")
		{
			Xeromyces_ReadScrollBarStyle(child, pFile);
		}
		else
		if (name == "icon")
		{
			Xeromyces_ReadIcon(child, pFile);
		}
		else
		if (name == "tooltip")
		{
			Xeromyces_ReadTooltip(child, pFile);
		}
		else
		if (name == "color")
		{
			Xeromyces_ReadColor(child, pFile);
		}
		else
		{
			debug_warn(L"Invalid data - DTD shouldn't allow this");
		}
	}
}

void CGUI::Xeromyces_ReadObject(XMBElement Element, CXeromyces* pFile, IGUIObject *pParent, const std::vector<std::pair<CStr, CStr> >& NameSubst, boost::unordered_set<VfsPath>& Paths)
{
	ENSURE(pParent);
	int i;

	// Our object we are going to create
	IGUIObject *object = NULL;

	XMBAttributeList attributes = Element.GetAttributes();

	// Well first of all we need to determine the type
	CStr type (attributes.GetNamedItem(pFile->GetAttributeID("type")));
	if (type.empty())
		type = "empty";

	// Construct object from specified type
	//  henceforth, we need to do a rollback before aborting.
	//  i.e. releasing this object
	object = ConstructObject(type);

	if (!object)
	{
		// Report error that object was unsuccessfully loaded
		LOGERROR(L"GUI: Unrecognized object type \"%hs\"", type.c_str());
		return;
	}

	// Cache some IDs for element attribute names, to avoid string comparisons
	#define ELMT(x) int elmt_##x = pFile->GetElementID(#x)
	#define ATTR(x) int attr_##x = pFile->GetAttributeID(#x)
	ELMT(object);
	ELMT(action);
	ELMT(repeat);
	ATTR(style);
	ATTR(type);
	ATTR(name);
	ATTR(hotkey);
	ATTR(z);
	ATTR(on);
	ATTR(file);

	//
	//	Read Style and set defaults
	//
	//	If the setting "style" is set, try loading that setting.
	//
	//	Always load default (if it's available) first!
	//
	CStr argStyle (attributes.GetNamedItem(attr_style));

	if (m_Styles.count("default") == 1)
		object->LoadStyle(*this, "default");

	if (! argStyle.empty())
	{
		// additional check
		if (m_Styles.count(argStyle) == 0)
		{
			LOGERROR(L"GUI: Trying to use style '%hs' that doesn't exist.", argStyle.c_str());
		}
		else object->LoadStyle(*this, argStyle);
	}
	

	//
	//	Read Attributes
	//

	bool NameSet = false;
	bool ManuallySetZ = false; // if z has been manually set, this turn true

	CStr hotkeyTag;

	// Now we can iterate all attributes and store
	for (i=0; i<attributes.Count; ++i)
	{
		XMBAttribute attr = attributes.Item(i);

		// If value is "null", then it is equivalent as never being entered
		if (CStr(attr.Value) == "null")
			continue;

		// Ignore "type" and "style", we've already checked it
		if (attr.Name == attr_type || attr.Name == attr_style)
			continue;

		// Also the name needs some special attention
		if (attr.Name == attr_name)
		{
			CStr name (attr.Value);

			// Apply the requested substitutions
			for (size_t j = 0; j < NameSubst.size(); ++j)
				name.Replace(NameSubst[j].first, NameSubst[j].second);

			object->SetName(name);
			NameSet = true;
			continue;
		}

		// Wire up the hotkey tag, if it has one
		if (attr.Name == attr_hotkey)
			hotkeyTag = attr.Value;

		if (attr.Name == attr_z)
			ManuallySetZ = true;

		// Try setting the value
		if (object->SetSetting(pFile->GetAttributeString(attr.Name), attr.Value.FromUTF8(), true) != PSRETURN_OK)
		{
			LOGERROR(L"GUI: (object: %hs) Can't set \"%hs\" to \"%ls\"", object->GetPresentableName().c_str(), pFile->GetAttributeString(attr.Name).c_str(), attr.Value.FromUTF8().c_str());

			// This is not a fatal error
		}
	}

	// Check if name isn't set, generate an internal name in that case.
	if (!NameSet)
	{
		object->SetName("__internal(" + CStr::FromInt(m_InternalNameNumber) + ")");
		++m_InternalNameNumber;
	}

	// Attempt to register the hotkey tag, if one was provided
	if (! hotkeyTag.empty())
		m_HotkeyObjects[hotkeyTag].push_back(object);

	CStrW caption (Element.GetText().FromUTF8());
	if (! caption.empty())
	{
		// Set the setting caption to this
		object->SetSetting("caption", caption, true);

		// There is no harm if the object didn't have a "caption"
	}


	//
	//	Read Children
	//

	// Iterate children
	XMBElementList children = Element.GetChildNodes();

	for (i=0; i<children.Count; ++i)
	{
		// Get node
		XMBElement child = children.Item(i);

		// Check what name the elements got
		int element_name = child.GetNodeName();

		if (element_name == elmt_object)
		{
			// Call this function on the child
			Xeromyces_ReadObject(child, pFile, object, NameSubst, Paths);
		}
		else if (element_name == elmt_action)
		{
			// Scripted <action> element

			// Check for a 'file' parameter
			CStrW filename (child.GetAttributes().GetNamedItem(attr_file).FromUTF8());

			CStr code;

			// If there is a file, open it and use it as the code
			if (! filename.empty())
			{
				Paths.insert(filename);
				CVFSFile scriptfile;
				if (scriptfile.Load(g_VFS, filename) != PSRETURN_OK)
				{
					LOGERROR(L"Error opening GUI script action file '%ls'", filename.c_str());
					throw PSERROR_GUI_JSOpenFailed();
				}

				code = scriptfile.DecodeUTF8(); // assume it's UTF-8
			}

			// Read the inline code (concatenating to the file code, if both are specified)
			code += CStr(child.GetText());

			CStr action = CStr(child.GetAttributes().GetNamedItem(attr_on));
			object->RegisterScriptHandler(action.LowerCase(), code, this);
		}
		else if (element_name == elmt_repeat)
		{
			Xeromyces_ReadRepeat(child, pFile, object, Paths);
		}
		else
		{
			// Try making the object read the tag.
			if (!object->HandleAdditionalChildren(child, pFile))
			{
				LOGERROR(L"GUI: (object: %hs) Reading unknown children for its type", object->GetPresentableName().c_str());
			}
		}
	} 

	//
	//	Check if Z wasn't manually set
	//
	if (!ManuallySetZ)
	{
		// Set it automatically to 10 plus its parents
		bool absolute;
		GUI<bool>::GetSetting(object, "absolute", absolute);

		// If the object is absolute, we'll have to get the parent's Z buffered,
		//  and add to that!
		if (absolute)
		{
			GUI<float>::SetSetting(object, "z", pParent->GetBufferedZ() + 10.f, true);
		}
		else
		// If the object is relative, then we'll just store Z as "10"
		{
			GUI<float>::SetSetting(object, "z", 10.f, true);
		}
	}


	//
	//	Input Child
	//

	try
	{
		if (pParent == m_BaseObject)
			AddObject(object);
		else
			pParent->AddChild(object);
	}
	catch (PSERROR_GUI& e)
	{
		LOGERROR(L"GUI error: %hs", e.what());
	}
}

void CGUI::Xeromyces_ReadRepeat(XMBElement Element, CXeromyces* pFile, IGUIObject *pParent, boost::unordered_set<VfsPath>& Paths)
{
	#define ELMT(x) int elmt_##x = pFile->GetElementID(#x)
	#define ATTR(x) int attr_##x = pFile->GetAttributeID(#x)
	ELMT(object);
	ATTR(count);

	XMBAttributeList attributes = Element.GetAttributes();

	int count = CStr(attributes.GetNamedItem(attr_count)).ToInt();

	for (int n = 0; n < count; ++n)
	{
		std::vector<std::pair<CStr, CStr> > subst;
		subst.push_back(std::make_pair(CStr("[n]"), "[" + CStr::FromInt(n) + "]"));

		XERO_ITER_EL(Element, child)
		{
			if (child.GetNodeName() == elmt_object)
			{
				Xeromyces_ReadObject(child, pFile, pParent, subst, Paths);
			}
		}
	}
}

void CGUI::Xeromyces_ReadScript(XMBElement Element, CXeromyces* pFile, boost::unordered_set<VfsPath>& Paths)
{
	// Check for a 'file' parameter
	CStrW file (Element.GetAttributes().GetNamedItem( pFile->GetAttributeID("file") ).FromUTF8());

	// If there is a file specified, open and execute it
	if (! file.empty())
	{
		Paths.insert(file);
		try
		{
			g_ScriptingHost.RunScript(file, m_ScriptObject);
		}
		catch (PSERROR_Scripting& e)
		{
			LOGERROR(L"GUI: Error executing script %ls: %hs", file.c_str(), e.what());
		}
	}

	// Execute inline scripts
	try
	{
		CStr code (Element.GetText());
		if (! code.empty())
			g_ScriptingHost.RunMemScript(code.c_str(), code.length(), "Some XML file", Element.GetLineNumber(), m_ScriptObject);
	}
	catch (PSERROR_Scripting& e)
	{
		LOGERROR(L"GUI: Error executing inline script: %hs", e.what());
	}
}

void CGUI::Xeromyces_ReadSprite(XMBElement Element, CXeromyces* pFile)
{
	// Sprite object we're adding
	CGUISprite sprite;
	
	// and what will be its reference name
	CStr name;

	//
	//	Read Attributes
	//

	// Get name, we know it exists because of DTD requirements
	name = Element.GetAttributes().GetNamedItem( pFile->GetAttributeID("name") );

	if (m_Sprites.find(name) != m_Sprites.end())
		LOGWARNING(L"GUI sprite name '%hs' used more than once; first definition will be discarded", name.c_str());

	//
	//	Read Children (the images)
	//

	SGUIImageEffects* effects = NULL;

	// Iterate children
	XMBElementList children = Element.GetChildNodes();

	for (int i=0; i<children.Count; ++i)
	{
		// Get node
		XMBElement child = children.Item(i);

		CStr ElementName (pFile->GetElementString(child.GetNodeName()));

		if (ElementName == "image")
		{
			Xeromyces_ReadImage(child, pFile, sprite);
		}
		else if (ElementName == "effect")
		{
			if (effects)
			{
				LOGERROR(L"GUI <sprite> must not have more than one <effect>");
			}
			else
			{
				effects = new SGUIImageEffects;
				Xeromyces_ReadEffects(child, pFile, *effects);
			}
		}
		else
		{
			debug_warn(L"Invalid data - DTD shouldn't allow this");
		}
	}

	// Apply the effects to every image (unless the image overrides it with
	// different effects)
	if (effects)
		for (std::vector<SGUIImage>::iterator it = sprite.m_Images.begin(); it != sprite.m_Images.end(); ++it)
			if (! it->m_Effects)
				it->m_Effects = new SGUIImageEffects(*effects); // do a copy just so it can be deleted correctly later

	delete effects;

	//
	//	Add Sprite
	//

	m_Sprites[name] = sprite;
}

void CGUI::Xeromyces_ReadImage(XMBElement Element, CXeromyces* pFile, CGUISprite &parent)
{

	// Image object we're adding
	SGUIImage image;
	
	// Set defaults to "0 0 100% 100%"
	image.m_TextureSize = CClientArea(CRect(0, 0, 0, 0), CRect(0, 0, 100, 100));
	image.m_Size = CClientArea(CRect(0, 0, 0, 0), CRect(0, 0, 100, 100));
	
	// TODO Gee: Setup defaults here (or maybe they are in the SGUIImage ctor)

	//
	//	Read Attributes
	//

	// Now we can iterate all attributes and store
	XMBAttributeList attributes = Element.GetAttributes();
	for (int i=0; i<attributes.Count; ++i)
	{
		XMBAttribute attr = attributes.Item(i);
		CStr attr_name (pFile->GetAttributeString(attr.Name));
		CStrW attr_value (attr.Value.FromUTF8());

		if (attr_name == "texture")
		{
			image.m_TextureName = VfsPath("art/textures/ui") / attr_value;
		}
		else
		if (attr_name == "size")
		{
			CClientArea ca;
			if (!GUI<CClientArea>::ParseString(attr_value, ca))
				LOGERROR(L"GUI: Error parsing '%hs' (\"%ls\")", attr_name.c_str(), attr_value.c_str());
			else image.m_Size = ca;
		}
		else
		if (attr_name == "texture_size")
		{
			CClientArea ca;
			if (!GUI<CClientArea>::ParseString(attr_value, ca))
				LOGERROR(L"GUI: Error parsing '%hs' (\"%ls\")", attr_name.c_str(), attr_value.c_str());
			else image.m_TextureSize = ca;
		}
		else
		if (attr_name == "real_texture_placement")
		{
			CRect rect;
			if (!GUI<CRect>::ParseString(attr_value, rect))
				LOGERROR(L"GUI: Error parsing '%hs' (\"%ls\")", attr_name.c_str(), attr_value.c_str());
			else image.m_TexturePlacementInFile = rect;
		}
		else
		if (attr_name == "cell_size")
		{
			CSize size;
			if (!GUI<CSize>::ParseString(attr_value, size))
				LOGERROR(L"GUI: Error parsing '%hs' (\"%ls\")", attr_name.c_str(), attr_value.c_str());
			else image.m_CellSize = size;
		}
		else
		if (attr_name == "fixed_h_aspect_ratio")
		{
			float val;
			if (!GUI<float>::ParseString(attr_value, val))
				LOGERROR(L"GUI: Error parsing '%hs' (\"%ls\")", attr_name.c_str(), attr_value.c_str());
			else image.m_FixedHAspectRatio = val;
		}
		else
		if (attr_name == "round_coordinates")
		{
			bool b;
			if (!GUI<bool>::ParseString(attr_value, b))
				LOGERROR(L"GUI: Error parsing '%hs' (\"%ls\")", attr_name.c_str(), attr_value.c_str());
			else image.m_RoundCoordinates = b;
		}
		else
		if (attr_name == "wrap_mode")
		{
			if (attr_value == L"repeat")
				image.m_WrapMode = GL_REPEAT;
			else if (attr_value == L"mirrored_repeat")
				image.m_WrapMode = GL_MIRRORED_REPEAT;
			else if (attr_value == L"clamp_to_edge")
				image.m_WrapMode = GL_CLAMP_TO_EDGE;
			else
				LOGERROR(L"GUI: Error parsing '%hs' (\"%ls\")", attr_name.c_str(), attr_value.c_str());
		}
		else
		if (attr_name == "z_level")
		{
			float z_level;
			if (!GUI<float>::ParseString(attr_value, z_level))
				LOGERROR(L"GUI: Error parsing '%hs' (\"%ls\")", attr_name.c_str(), attr_value.c_str());
			else image.m_DeltaZ = z_level/100.f;
		}
		else
		if (attr_name == "backcolor")
		{
			CColor color;
			if (!GUI<CColor>::ParseString(attr_value, color))
				LOGERROR(L"GUI: Error parsing '%hs' (\"%ls\")", attr_name.c_str(), attr_value.c_str());
			else image.m_BackColor = color;
		}
		else
		if (attr_name == "bordercolor")
		{
			CColor color;
			if (!GUI<CColor>::ParseString(attr_value, color))
				LOGERROR(L"GUI: Error parsing '%hs' (\"%ls\")", attr_name.c_str(), attr_value.c_str());
			else image.m_BorderColor = color;
		}
		else
		if (attr_name == "border")
		{
			bool b;
			if (!GUI<bool>::ParseString(attr_value, b))
				LOGERROR(L"GUI: Error parsing '%hs' (\"%ls\")", attr_name.c_str(), attr_value.c_str());
			else image.m_Border = b;
		}
		else
		{
			debug_warn(L"Invalid data - DTD shouldn't allow this");
		}
	}

	// Look for effects
	XMBElementList children = Element.GetChildNodes();
	for (int i=0; i<children.Count; ++i)
	{
		XMBElement child = children.Item(i);
		CStr ElementName (pFile->GetElementString(child.GetNodeName()));
		if (ElementName == "effect")
		{
			if (image.m_Effects)
			{
				LOGERROR(L"GUI <image> must not have more than one <effect>");
			}
			else
			{
				image.m_Effects = new SGUIImageEffects;
				Xeromyces_ReadEffects(child, pFile, *image.m_Effects);
			}
		}
		else
		{
			debug_warn(L"Invalid data - DTD shouldn't allow this");
		}
	}

	//
	//	Input
	//

	parent.AddImage(image);	
}

void CGUI::Xeromyces_ReadEffects(XMBElement Element, CXeromyces* pFile, SGUIImageEffects &effects)
{
	XMBAttributeList attributes = Element.GetAttributes();
	for (int i=0; i<attributes.Count; ++i)
	{
		XMBAttribute attr = attributes.Item(i);
		CStr attr_name (pFile->GetAttributeString(attr.Name));
		CStrW attr_value (attr.Value.FromUTF8());

		if (attr_name == "add_color")
		{
			CColor color;
			if (!GUI<int>::ParseColor(attr_value, color, 0.f))
				LOGERROR(L"GUI: Error parsing '%hs' (\"%ls\")", attr_name.c_str(), attr_value.c_str());
			else effects.m_AddColor = color;
		}
		else if (attr_name == "grayscale")
		{
			effects.m_Greyscale = true;
		}
		else
		{
			debug_warn(L"Invalid data - DTD shouldn't allow this");
		}
	}
}

void CGUI::Xeromyces_ReadStyle(XMBElement Element, CXeromyces* pFile)
{
	// style object we're adding
	SGUIStyle style;
	CStr name;
	
	//
	//	Read Attributes
	//

	// Now we can iterate all attributes and store
	XMBAttributeList attributes = Element.GetAttributes();
	for (int i=0; i<attributes.Count; ++i)
	{
		XMBAttribute attr = attributes.Item(i);
		CStr attr_name (pFile->GetAttributeString(attr.Name));

		// The "name" setting is actually the name of the style
		//  and not a new default
		if (attr_name == "name")
			name = attr.Value;
		else
			style.m_SettingsDefaults[attr_name] = attr.Value.FromUTF8();
	}

	//
	//	Add to CGUI
	//

	m_Styles[name] = style;
}

void CGUI::Xeromyces_ReadScrollBarStyle(XMBElement Element, CXeromyces* pFile)
{
	// style object we're adding
	SGUIScrollBarStyle scrollbar;
	CStr name;
	
	//
	//	Read Attributes
	//

	// Now we can iterate all attributes and store
	XMBAttributeList attributes = Element.GetAttributes();
	for (int i=0; i<attributes.Count; ++i)
	{
		XMBAttribute attr = attributes.Item(i);
		CStr attr_name = pFile->GetAttributeString(attr.Name);
		CStr attr_value (attr.Value);

		if (attr_value == "null")
			continue;

		if (attr_name == "name")
			name = attr_value;
		else
		if (attr_name == "width")
		{
			float f;
			if (!GUI<float>::ParseString(attr_value.FromUTF8(), f))
				LOGERROR(L"GUI: Error parsing '%hs' (\"%hs\")", attr_name.c_str(), attr_value.c_str());
			else
				scrollbar.m_Width = f;
		}
		else
		if (attr_name == "minimum_bar_size")
		{
			float f;
			if (!GUI<float>::ParseString(attr_value.FromUTF8(), f))
				LOGERROR(L"GUI: Error parsing '%hs' (\"%hs\")", attr_name.c_str(), attr_value.c_str());
			else
				scrollbar.m_MinimumBarSize = f;
		}
		else
		if (attr_name == "sprite_button_top")
			scrollbar.m_SpriteButtonTop = attr_value;
		else
		if (attr_name == "sprite_button_top_pressed")
			scrollbar.m_SpriteButtonTopPressed = attr_value;
		else
		if (attr_name == "sprite_button_top_disabled")
			scrollbar.m_SpriteButtonTopDisabled = attr_value;
		else
		if (attr_name == "sprite_button_top_over")
			scrollbar.m_SpriteButtonTopOver = attr_value;
		else
		if (attr_name == "sprite_button_bottom")
			scrollbar.m_SpriteButtonBottom = attr_value;
		else
		if (attr_name == "sprite_button_bottom_pressed")
			scrollbar.m_SpriteButtonBottomPressed = attr_value;
		else
		if (attr_name == "sprite_button_bottom_disabled")
			scrollbar.m_SpriteButtonBottomDisabled = attr_value;
		else
		if (attr_name == "sprite_button_bottom_over")
			scrollbar.m_SpriteButtonBottomOver = attr_value;
		else
		if (attr_name == "sprite_back_vertical")
			scrollbar.m_SpriteBackVertical = attr_value;
		else
		if (attr_name == "sprite_bar_vertical")
			scrollbar.m_SpriteBarVertical = attr_value;
		else
		if (attr_name == "sprite_bar_vertical_over")
			scrollbar.m_SpriteBarVerticalOver = attr_value;
		else
		if (attr_name == "sprite_bar_vertical_pressed")
			scrollbar.m_SpriteBarVerticalPressed = attr_value;
	}

	//
	//	Add to CGUI 
	//

	m_ScrollBarStyles[name] = scrollbar;
}

void CGUI::Xeromyces_ReadIcon(XMBElement Element, CXeromyces* pFile)
{
	// Icon we're adding
	SGUIIcon icon;
	CStr name;

	XMBAttributeList attributes = Element.GetAttributes();
	for (int i=0; i<attributes.Count; ++i)
	{
		XMBAttribute attr = attributes.Item(i);
		CStr attr_name (pFile->GetAttributeString(attr.Name));
		CStr attr_value (attr.Value);

		if (attr_value == "null")
			continue;

		if (attr_name == "name")
			name = attr_value;
		else
		if (attr_name == "sprite")
			icon.m_SpriteName = attr_value;
		else
		if (attr_name == "size")
		{
			CSize size;
			if (!GUI<CSize>::ParseString(attr_value.FromUTF8(), size))
				LOGERROR(L"Error parsing '%hs' (\"%hs\") inside <icon>.", attr_name.c_str(), attr_value.c_str());
			else
				icon.m_Size = size;
		}
		else
		if (attr_name == "cell_id")
		{
			int cell_id;
			if (!GUI<int>::ParseString(attr_value.FromUTF8(), cell_id))
				LOGERROR(L"GUI: Error parsing '%hs' (\"%hs\") inside <icon>.", attr_name.c_str(), attr_value.c_str());
			else
				icon.m_CellID = cell_id;
		}
		else
		{
			debug_warn(L"Invalid data - DTD shouldn't allow this");
		}
	}

	m_Icons[name] = icon;
}

void CGUI::Xeromyces_ReadTooltip(XMBElement Element, CXeromyces* pFile)
{
	// Read the tooltip, and store it as a specially-named object

	IGUIObject* object = new CTooltip;

	XMBAttributeList attributes = Element.GetAttributes();
	for (int i=0; i<attributes.Count; ++i)
	{
		XMBAttribute attr = attributes.Item(i);
		CStr attr_name (pFile->GetAttributeString(attr.Name));
		CStr attr_value (attr.Value);

		if (attr_name == "name")
		{
			object->SetName("__tooltip_" + attr_value);
		}
		else
		{
			object->SetSetting(attr_name, attr_value.FromUTF8());
		}
	}

	AddObject(object);
}

// Reads Custom Color
void CGUI::Xeromyces_ReadColor(XMBElement Element, CXeromyces* pFile)
{
	// Read the color and stor in m_PreDefinedColors

	XMBAttributeList attributes = Element.GetAttributes();

	//IGUIObject* object = new CTooltip;
	CColor color;
	CStr name = attributes.GetNamedItem(pFile->GetAttributeID("name"));

	// Try parsing value 
	CStr value (Element.GetText());
	if (! value.empty())
	{
		// Try setting color to value
		if (!color.ParseString(value, 255.f))
		{
			LOGERROR(L"GUI: Unable to create custom color '%hs'. Invalid color syntax.", name.c_str());
		}
		else
		{
			// input color
			m_PreDefinedColors[name] = color;
		}
	}
}
