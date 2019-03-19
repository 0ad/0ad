/* Copyright (C) 2019 Wildfire Games.
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

#include "precompiled.h"

#include <stdarg.h>
#include <string>

#include "GUI.h"

// Types - when including them into the engine.
#include "CButton.h"
#include "CChart.h"
#include "CCheckBox.h"
#include "CDropDown.h"
#include "CImage.h"
#include "CInput.h"
#include "CList.h"
#include "COList.h"
#include "CProgressBar.h"
#include "CRadioButton.h"
#include "CSlider.h"
#include "CText.h"
#include "CTooltip.h"
#include "MiniMap.h"

#include "graphics/FontMetrics.h"
#include "graphics/ShaderManager.h"
#include "graphics/TextRenderer.h"
#include "i18n/L10n.h"
#include "lib/bits.h"
#include "lib/input.h"
#include "lib/sysdep/sysdep.h"
#include "lib/timer.h"
#include "lib/utf8.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/GameSetup/Config.h"
#include "ps/Globals.h"
#include "ps/Hotkey.h"
#include "ps/Profile.h"
#include "ps/Pyrogenesis.h"
#include "ps/XML/Xeromyces.h"
#include "renderer/Renderer.h"
#include "scripting/ScriptFunctions.h"
#include "scriptinterface/ScriptInterface.h"

extern int g_yres;

const double SELECT_DBLCLICK_RATE = 0.5;
const u32 MAX_OBJECT_DEPTH = 100; // Max number of nesting for GUI includes. Used to detect recursive inclusion

InReaction CGUI::HandleEvent(const SDL_Event_* ev)
{
	InReaction ret = IN_PASS;

	if (ev->ev.type == SDL_HOTKEYDOWN || ev->ev.type == SDL_HOTKEYUP)
	{
		const char* hotkey = static_cast<const char*>(ev->ev.user.data1);

		std::map<CStr, std::vector<IGUIObject*> >::iterator it = m_HotkeyObjects.find(hotkey);
		if (it != m_HotkeyObjects.end())
			for (IGUIObject* const& obj : it->second)
			{
				// Update hotkey status before sending the event,
				// else the status will be outdated when processing the GUI event.
				HotkeyInputHandler(ev);
				ret = IN_HANDLED;

				if (ev->ev.type == SDL_HOTKEYDOWN)
					obj->SendEvent(GUIM_PRESSED, "press");
				else
					obj->SendEvent(GUIM_RELEASED, "release");
			}
	}

	else if (ev->ev.type == SDL_MOUSEMOTION)
	{
		// Yes the mouse position is stored as float to avoid
		//  constant conversions when operating in a
		//  float-based environment.
		m_MousePos = CPos((float)ev->ev.motion.x / g_GuiScale, (float)ev->ev.motion.y / g_GuiScale);

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
		m_MousePos = CPos((float)ev->ev.button.x / g_GuiScale, (float)ev->ev.button.y / g_GuiScale);
	}

	// Only one object can be hovered
	IGUIObject* pNearest = NULL;

	// TODO Gee: (2004-09-08) Big TODO, don't do the below if the SDL_Event is something like a keypress!
	try
	{
		PROFILE("mouse events");
		// TODO Gee: Optimizations needed!
		//  these two recursive function are quite overhead heavy.

		// pNearest will after this point at the hovered object, possibly NULL
		pNearest = FindObjectUnderMouse();

		// Now we'll call UpdateMouseOver on *all* objects,
		//  we'll input the one hovered, and they will each
		//  update their own data and send messages accordingly
		GUI<IGUIObject*>::RecurseObject(GUIRR_HIDDEN | GUIRR_GHOST,
			m_BaseObject, &IGUIObject::UpdateMouseOver, pNearest);

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

			default:
				break;
			}
		}
		else if (ev->ev.type == SDL_MOUSEWHEEL && pNearest)
		{
			if (ev->ev.wheel.y < 0)
				ret = pNearest->SendEvent(GUIM_MOUSE_WHEEL_DOWN, "mousewheeldown");
			else if (ev->ev.wheel.y > 0)
				ret = pNearest->SendEvent(GUIM_MOUSE_WHEEL_UP, "mousewheelup");
		}
		else if (ev->ev.type == SDL_MOUSEBUTTONUP)
		{
			switch (ev->ev.button.button)
			{
			case SDL_BUTTON_LEFT:
				if (pNearest)
				{
					double timeElapsed = timer_Time() - pNearest->m_LastClickTime[SDL_BUTTON_LEFT];
					pNearest->m_LastClickTime[SDL_BUTTON_LEFT] = timer_Time();

					if (timeElapsed < SELECT_DBLCLICK_RATE)
						ret = pNearest->SendEvent(GUIM_MOUSE_DBLCLICK_LEFT, "mouseleftdoubleclick");
					else
						ret = pNearest->SendEvent(GUIM_MOUSE_RELEASE_LEFT, "mouseleftrelease");
				}
				break;
			case SDL_BUTTON_RIGHT:
				if (pNearest)
				{
					double timeElapsed = timer_Time() - pNearest->m_LastClickTime[SDL_BUTTON_RIGHT];
					pNearest->m_LastClickTime[SDL_BUTTON_RIGHT] = timer_Time();

					if (timeElapsed < SELECT_DBLCLICK_RATE)
						ret = pNearest->SendEvent(GUIM_MOUSE_DBLCLICK_RIGHT, "mouserightdoubleclick");
					else
						ret = pNearest->SendEvent(GUIM_MOUSE_RELEASE_RIGHT, "mouserightrelease");
				}
				break;
			}

			// Reset all states on all visible objects
			GUI<>::RecurseObject(GUIRR_HIDDEN, m_BaseObject,
									&IGUIObject::ResetStates);

			// Since the hover state will have been reset, we reload it.
			GUI<IGUIObject*>::RecurseObject(GUIRR_HIDDEN | GUIRR_GHOST,
				m_BaseObject, &IGUIObject::UpdateMouseOver, pNearest);
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
		m_MousePos = oldMousePos;

	// Handle keys for input boxes
	if (GetFocusedObject())
	{
		if ((ev->ev.type == SDL_KEYDOWN &&
		     ev->ev.key.keysym.sym != SDLK_ESCAPE &&
		     !g_keys[SDLK_LCTRL] && !g_keys[SDLK_RCTRL] &&
		     !g_keys[SDLK_LALT] && !g_keys[SDLK_RALT]) ||
		    ev->ev.type == SDL_HOTKEYDOWN ||
		    ev->ev.type == SDL_TEXTINPUT ||
		    ev->ev.type == SDL_TEXTEDITING)
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

CGUI::CGUI(const shared_ptr<ScriptRuntime>& runtime)
	: m_MouseButtons(0), m_FocusedObject(NULL), m_InternalNameNumber(0)
{
	m_ScriptInterface.reset(new ScriptInterface("Engine", "GUIPage", runtime));
	GuiScriptingInit(*m_ScriptInterface);
	m_ScriptInterface->LoadGlobalScripts();
	m_BaseObject = new CGUIDummyObject;
	m_BaseObject->SetGUI(this);
}

CGUI::~CGUI()
{
	Destroy();

	if (m_BaseObject)
		delete m_BaseObject;
}

IGUIObject* CGUI::ConstructObject(const CStr& str)
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
	AddObjectType("olist",			&COList::ConstructObject);
	AddObjectType("dropdown",		&CDropDown::ConstructObject);
	AddObjectType("tooltip",		&CTooltip::ConstructObject);
	AddObjectType("chart",			&CChart::ConstructObject);
	AddObjectType("slider",			&CSlider::ConstructObject);
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
		LOGERROR("GUI draw error: %s", e.what());
	}
}

void CGUI::DrawSprite(const CGUISpriteInstance& Sprite, int CellID, const float& Z, const CRect& Rect, const CRect& UNUSED(Clipping))
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
	for (const std::pair<CStr, IGUIObject*>& p : m_pAllObjects)
	{
		try
		{
			p.second->Destroy();
		}
		catch (PSERROR_GUI& e)
		{
			UNUSED2(e);
			debug_warn(L"CGUI::Destroy error");
			// TODO Gee: Handle
		}

		delete p.second;
	}
	m_pAllObjects.clear();

	for (const std::pair<CStr, CGUISprite*>& p : m_Sprites)
		delete p.second;
	m_Sprites.clear();
	m_Icons.clear();
}

void CGUI::UpdateResolution()
{
	// Update ALL cached
	GUI<>::RecurseObject(0, m_BaseObject, &IGUIObject::UpdateCachedSize);
}

void CGUI::AddObject(IGUIObject* pObject)
{
	try
	{
		// Add CGUI pointer
		GUI<CGUI*>::RecurseObject(0, pObject, &IGUIObject::SetGUI, this);

		m_BaseObject->AddChild(pObject);

		// Cache tree
		GUI<>::RecurseObject(0, pObject, &IGUIObject::UpdateCachedSize);

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
	// We'll fill a temporary map until we know everything succeeded
	map_pObjects AllObjects;

	try
	{
		// Fill freshly
		GUI<map_pObjects>::RecurseObject(0, m_BaseObject, &IGUIObject::AddToPointersMap, AllObjects);
	}
	catch (PSERROR_GUI&)
	{
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
		&IGUIObject::ChooseMouseOverAndClosest, pNearest);

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

const SGUIScrollBarStyle* CGUI::GetScrollBarStyle(const CStr& style) const
{
	std::map<CStr, SGUIScrollBarStyle>::const_iterator it = m_ScrollBarStyles.find(style);
	if (it == m_ScrollBarStyles.end())
		return nullptr;

 	return &it->second;
}

// private struct used only in GenerateText(...)
struct SGenerateTextImage
{
	float m_YFrom,			// The image's starting location in Y
		  m_YTo,			// The image's end location in Y
		  m_Indentation;	// The image width in other words

	// Some help functions
	// TODO Gee: CRect => CPoint ?
	void SetupSpriteCall(const bool Left, SGUIText::SSpriteCall& SpriteCall,
						 const float width, const float y,
						 const CSize& Size, const CStr& TextureName,
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

SGUIText CGUI::GenerateText(const CGUIString& string, const CStrW& FontW, const float& Width, const float& BufferZone, const IGUIObject* pObject)
{
	SGUIText Text;

	CStrIntern Font(FontW.ToUTF8());

	if (string.m_Words.empty())
		return Text;

	float x = BufferZone, y = BufferZone; // drawing pointer
	int from = 0;
	bool done = false;

	bool FirstLine = true;	// Necessary because text in the first line is shorter
							// (it doesn't count the line spacing)

	// Images on the left or the right side.
	std::vector<SGenerateTextImage> Images[2];
	int pos_last_img = -1;	// Position in the string where last img (either left or right) were encountered.
							//  in order to avoid duplicate processing.

	// Easier to read.
	bool WordWrapping = (Width != 0);

	// get the alignment type for the control we are computing the text for since
	// we are computing the horizontal alignment in this method in order to not have
	// to run through the TextCalls a second time in the CalculateTextPosition method again
	EAlign align = EAlign_Left;
	if (pObject->SettingExists("text_align"))
		GUI<EAlign>::GetSetting(pObject, "text_align", align);

	// Go through string word by word
	for (int i = 0; i < (int)string.m_Words.size()-1 && !done; ++i)
	{
		// Pre-process each line one time, so we know which floating images
		//  will be added for that line.

		// Generated stuff is stored in Feedback.
		CGUIString::SFeedback Feedback;

		// Preliminary line_height, used for word-wrapping with floating images.
		float prelim_line_height = 0.f;

		// Width and height of all text calls generated.
		string.GenerateTextCall(this, Feedback, Font,
								string.m_Words[i], string.m_Words[i+1],
								FirstLine);

		// Loop through our images queues, to see if images has been added.

		// Check if this has already been processed.
		//  Also, floating images are only applicable if Word-Wrapping is on
		if (WordWrapping && i > pos_last_img)
		{
			// Loop left/right
			for (int j = 0; j < 2; ++j)
			{
				for (const CStr& imgname : Feedback.m_Images[j])
				{
					SGUIText::SSpriteCall SpriteCall;
					SGenerateTextImage Image;

					// Y is if no other floating images is above, y. Else it is placed
					//  after the last image, like a stack downwards.
					float _y;
					if (!Images[j].empty())
						_y = std::max(y, Images[j].back().m_YTo);
					else
						_y = y;

					// Get Size from Icon database
					SGUIIcon icon = GetIcon(imgname);

					CSize size = icon.m_Size;
					Image.SetupSpriteCall((j == CGUIString::SFeedback::Left), SpriteCall, Width, _y, size, icon.m_SpriteName, BufferZone, icon.m_CellID);

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

			static const int From = 0, To = 1;
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
				for (int j = 0; j < 2; ++j)
				{
					for (const SGenerateTextImage& img : Images[j])
					{
						// We're working with two intervals here, the image's and the line height's.
						//  let's find the union of these two.
						float union_from, union_to;

						union_from = std::max(y, img.m_YFrom);
						union_to = std::min(y+prelim_line_height, img.m_YTo);

						// The union is not empty
						if (union_to > union_from)
						{
							if (j == From)
								width_range[From] = std::max(width_range[From], img.m_Indentation);
							else
								width_range[To] = std::min(width_range[To], Width - img.m_Indentation);
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
			float line_height = 0.f;
			float line_width = 0.f;
			for (int j = temp_from; j <= i; ++j)
			{
				// We don't want to use Feedback now, so we'll have to use
				//  another.
				CGUIString::SFeedback Feedback2;

				// Don't attach object, it'll suppress the errors
				//  we want them to be reported in the final GenerateTextCall()
				//  so that we don't get duplicates.
				string.GenerateTextCall(this, Feedback2, Font,
										string.m_Words[j], string.m_Words[j+1],
										FirstLine);

				// Append X value.
				x += Feedback2.m_Size.cx;

				if (WordWrapping && x > width_range[To] && j!=temp_from && !Feedback2.m_NewLine)
				{
					// The calculated width of each word includes the space between the current
					// word and the next. When we're wrapping, we need subtract the width of the
					// space after the last word on the line before the wrap.
					CFontMetrics currentFont(Font);
					line_width -= currentFont.GetCharacterWidth(*L" ");
					break;
				}

				// Let line_height be the maximum m_Height we encounter.
				line_height = std::max(line_height, Feedback2.m_Size.cy);

				// If the current word is an explicit new line ("\n"),
				// break now before adding the width of this character.
				// ("\n" doesn't have a glyph, thus is given the same width as
				// the "missing glyph" character by CFont::GetCharacterWidth().)
				if (WordWrapping && Feedback2.m_NewLine)
					break;

				line_width += Feedback2.m_Size.cx;
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
			for (int j = temp_from; j <= i; ++j)
			{
				// We don't want to use Feedback now, so we'll have to use
				//  another one.
				CGUIString::SFeedback Feedback2;

				// Defaults
				string.GenerateTextCall(this, Feedback2, Font,
										string.m_Words[j], string.m_Words[j+1],
										FirstLine, pObject);

				// Iterate all and set X/Y values
				// Since X values are not set, we need to make an internal
				//  iteration with an increment that will append the internal
				//  x, that is what x_pointer is for.
				float x_pointer = 0.f;

				for (SGUIText::STextCall& tc : Feedback2.m_TextCalls)
				{
					tc.m_Pos = CPos(dx + x + x_pointer, y);

					x_pointer += tc.m_Size.cx;

					if (tc.m_pSpriteCall)
						tc.m_pSpriteCall->m_Area += tc.m_Pos - CSize(0, tc.m_pSpriteCall->m_Area.GetHeight());
				}

				// Append X value.
				x += Feedback2.m_Size.cx;

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
					else if (x > width_range[To] && j == temp_from)
					{
						from = j+1;
						// do not break, since we want it to be added to m_TextCalls
					}
					else if (x > width_range[To])
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
			x = BufferZone;

			// Update dimensions
			Text.m_Size.cx = std::max(Text.m_Size.cx, line_width + BufferZone * 2);
			Text.m_Size.cy = std::max(Text.m_Size.cy, y + BufferZone);

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

void CGUI::DrawText(SGUIText& Text, const CColor& DefaultColor, const CPos& pos, const float& z, const CRect& clipping)
{
	CShaderTechniquePtr tech = g_Renderer.GetShaderManager().LoadEffect(str_gui_text);

	tech->BeginPass();

	bool isClipped = (clipping != CRect());
	if (isClipped)
	{
		glEnable(GL_SCISSOR_TEST);
		glScissor(
			clipping.left * g_GuiScale,
			g_yres - clipping.bottom * g_GuiScale,
			clipping.GetWidth() * g_GuiScale,
			clipping.GetHeight() * g_GuiScale);
	}

	CTextRenderer textRenderer(tech->GetShader());
	textRenderer.SetClippingRect(clipping);
	textRenderer.Translate(0.0f, 0.0f, z);

	for (const SGUIText::STextCall& tc : Text.m_TextCalls)
	{
		// If this is just a placeholder for a sprite call, continue
		if (tc.m_pSpriteCall)
			continue;

		CColor color = tc.m_UseCustomColor ? tc.m_Color : DefaultColor;

		textRenderer.Color(color);
		textRenderer.Font(tc.m_Font);
		textRenderer.Put((float)(int)(pos.x + tc.m_Pos.x), (float)(int)(pos.y + tc.m_Pos.y), &tc.m_String);
	}

	textRenderer.Render();

	for (const SGUIText::SSpriteCall& sc : Text.m_SpriteCalls)
		DrawSprite(sc.m_Sprite, sc.m_CellID, z, sc.m_Area + pos);

	if (isClipped)
		glDisable(GL_SCISSOR_TEST);

	tech->EndPass();
}

bool CGUI::GetPreDefinedColor(const CStr& name, CColor& Output) const
{
	std::map<CStr, CColor>::const_iterator cit = m_PreDefinedColors.find(name);
	if (cit == m_PreDefinedColors.end())
		return false;

	Output = cit->second;
	return true;
}

/**
 * @callgraph
 */
void CGUI::LoadXmlFile(const VfsPath& Filename, boost::unordered_set<VfsPath>& Paths)
{
	Paths.insert(Filename);

	CXeromyces XeroFile;
	if (XeroFile.Load(g_VFS, Filename, "gui") != PSRETURN_OK)
		return;

	XMBElement node = XeroFile.GetRoot();

	CStr root_name(XeroFile.GetElementString(node.GetNodeName()));
	try
	{
		if (root_name == "objects")
		{
			Xeromyces_ReadRootObjects(node, &XeroFile, Paths);

			// Re-cache all values so these gets cached too.
			//UpdateResolution();
		}
		else if (root_name == "sprites")
			Xeromyces_ReadRootSprites(node, &XeroFile);
		else if (root_name == "styles")
			Xeromyces_ReadRootStyles(node, &XeroFile);
		else if (root_name == "setup")
			Xeromyces_ReadRootSetup(node, &XeroFile);
		else
			debug_warn(L"CGUI::LoadXmlFile error");
	}
	catch (PSERROR_GUI& e)
	{
		LOGERROR("Errors loading GUI file %s (%u)", Filename.string8(), e.getCode());
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
	for (XMBElement child : Element.GetChildNodes())
	{
		if (child.GetNodeName() == el_script)
			// Execute the inline script
			Xeromyces_ReadScript(child, pFile, Paths);
		else
			// Read in this whole object into the GUI
			Xeromyces_ReadObject(child, pFile, m_BaseObject, subst, Paths, 0);
	}
}

void CGUI::Xeromyces_ReadRootSprites(XMBElement Element, CXeromyces* pFile)
{
	for (XMBElement child : Element.GetChildNodes())
		Xeromyces_ReadSprite(child, pFile);
}

void CGUI::Xeromyces_ReadRootStyles(XMBElement Element, CXeromyces* pFile)
{
	for (XMBElement child : Element.GetChildNodes())
		Xeromyces_ReadStyle(child, pFile);
}

void CGUI::Xeromyces_ReadRootSetup(XMBElement Element, CXeromyces* pFile)
{
	for (XMBElement child : Element.GetChildNodes())
	{
		CStr name(pFile->GetElementString(child.GetNodeName()));

		if (name == "scrollbar")
			Xeromyces_ReadScrollBarStyle(child, pFile);
		else if (name == "icon")
			Xeromyces_ReadIcon(child, pFile);
		else if (name == "tooltip")
			Xeromyces_ReadTooltip(child, pFile);
		else if (name == "color")
			Xeromyces_ReadColor(child, pFile);
		else
			debug_warn(L"Invalid data - DTD shouldn't allow this");
	}
}

void CGUI::Xeromyces_ReadObject(XMBElement Element, CXeromyces* pFile, IGUIObject* pParent, std::vector<std::pair<CStr, CStr> >& NameSubst, boost::unordered_set<VfsPath>& Paths, u32 nesting_depth)
{
	ENSURE(pParent);

	XMBAttributeList attributes = Element.GetAttributes();

	CStr type(attributes.GetNamedItem(pFile->GetAttributeID("type")));
	if (type.empty())
		type = "empty";

	// Construct object from specified type
	//  henceforth, we need to do a rollback before aborting.
	//  i.e. releasing this object
	IGUIObject* object = ConstructObject(type);

	if (!object)
	{
		LOGERROR("GUI: Unrecognized object type \"%s\"", type.c_str());
		return;
	}

	// Cache some IDs for element attribute names, to avoid string comparisons
	#define ELMT(x) int elmt_##x = pFile->GetElementID(#x)
	#define ATTR(x) int attr_##x = pFile->GetAttributeID(#x)
	ELMT(object);
	ELMT(action);
	ELMT(repeat);
	ELMT(translatableAttribute);
	ELMT(translate);
	ELMT(attribute);
	ELMT(keep);
	ELMT(include);
	ATTR(style);
	ATTR(type);
	ATTR(name);
	ATTR(hotkey);
	ATTR(z);
	ATTR(on);
	ATTR(file);
	ATTR(directory);
	ATTR(id);
	ATTR(context);

	//
	//	Read Style and set defaults
	//
	//	If the setting "style" is set, try loading that setting.
	//
	//	Always load default (if it's available) first!
	//
	CStr argStyle(attributes.GetNamedItem(attr_style));

	if (m_Styles.count("default") == 1)
		object->LoadStyle(*this, "default");

	if (!argStyle.empty())
	{
		if (m_Styles.count(argStyle) == 0)
			LOGERROR("GUI: Trying to use style '%s' that doesn't exist.", argStyle.c_str());
		else
			object->LoadStyle(*this, argStyle);
	}

	bool NameSet = false;
	bool ManuallySetZ = false;

	CStrW inclusionPath;
	CStr hotkeyTag;

	for (XMBAttribute attr : attributes)
	{
		// If value is "null", then it is equivalent as never being entered
		if (CStr(attr.Value) == "null")
			continue;

		// Ignore "type" and "style", we've already checked it
		if (attr.Name == attr_type || attr.Name == attr_style)
			continue;

		if (attr.Name == attr_name)
		{
			CStr name(attr.Value);

			for (const std::pair<CStr, CStr>& sub : NameSubst)
				name.Replace(sub.first, sub.second);

			object->SetName(name);
			NameSet = true;
			continue;
		}

		if (attr.Name == attr_hotkey)
			hotkeyTag = attr.Value;

		if (attr.Name == attr_z)
			ManuallySetZ = true;

		if (object->SetSetting(pFile->GetAttributeString(attr.Name), attr.Value.FromUTF8(), true) != PSRETURN_OK)
			LOGERROR("GUI: (object: %s) Can't set \"%s\" to \"%s\"", object->GetPresentableName(), pFile->GetAttributeString(attr.Name), attr.Value);
	}

	// Check if name isn't set, generate an internal name in that case.
	if (!NameSet)
	{
		object->SetName("__internal(" + CStr::FromInt(m_InternalNameNumber) + ")");
		++m_InternalNameNumber;
	}

	if (!hotkeyTag.empty())
		m_HotkeyObjects[hotkeyTag].push_back(object);

	CStrW caption(Element.GetText().FromUTF8());
	if (!caption.empty())
		object->SetSetting("caption", caption, true);

	for (XMBElement child : Element.GetChildNodes())
	{
		// Check what name the elements got
		int element_name = child.GetNodeName();

		if (element_name == elmt_object)
		{
			// Call this function on the child
			Xeromyces_ReadObject(child, pFile, object, NameSubst, Paths, nesting_depth);
		}
		else if (element_name == elmt_action)
		{
			// Scripted <action> element

			// Check for a 'file' parameter
			CStrW filename(child.GetAttributes().GetNamedItem(attr_file).FromUTF8());

			CStr code;

			// If there is a file, open it and use it as the code
			if (!filename.empty())
			{
				Paths.insert(filename);
				CVFSFile scriptfile;
				if (scriptfile.Load(g_VFS, filename) != PSRETURN_OK)
				{
					LOGERROR("Error opening GUI script action file '%s'", utf8_from_wstring(filename));
					throw PSERROR_GUI_JSOpenFailed();
				}

				code = scriptfile.DecodeUTF8(); // assume it's UTF-8
			}

			XMBElementList grandchildren = child.GetChildNodes();
			if (!grandchildren.empty()) // The <action> element contains <keep> and <translate> tags.
				for (XMBElement grandchild : grandchildren)
				{
					if (grandchild.GetNodeName() == elmt_translate)
						code += g_L10n.Translate(grandchild.GetText());
					else if (grandchild.GetNodeName() == elmt_keep)
						code += grandchild.GetText();
				}
			else // It’s pure JavaScript code.
				// Read the inline code (concatenating to the file code, if both are specified)
				code += CStr(child.GetText());

			CStr action = CStr(child.GetAttributes().GetNamedItem(attr_on));

			// We need to set the GUI this object belongs to because RegisterScriptHandler requires an associated GUI.
			object->SetGUI(this);
			object->RegisterScriptHandler(action.LowerCase(), code, this);
		}
		else if (element_name == elmt_repeat)
		{
			Xeromyces_ReadRepeat(child, pFile, object, NameSubst, Paths, nesting_depth);
		}
		else if (element_name == elmt_translatableAttribute)
		{
			// This is an element in the form “<translatableAttribute id="attributeName">attributeValue</translatableAttribute>”.
			CStr attributeName(child.GetAttributes().GetNamedItem(attr_id)); // Read the attribute name.
			if (attributeName.empty())
			{
				LOGERROR("GUI: ‘translatableAttribute’ XML element with empty ‘id’ XML attribute found. (object: %s)", object->GetPresentableName().c_str());
				continue;
			}

			CStr value(child.GetText());
			if (value.empty())
				continue;

			CStr context(child.GetAttributes().GetNamedItem(attr_context)); // Read the context if any.
			if (!context.empty())
			{
				CStr translatedValue(g_L10n.TranslateWithContext(context, value));
				object->SetSetting(attributeName, translatedValue.FromUTF8(), true);
			}
			else
			{
				CStr translatedValue(g_L10n.Translate(value));
				object->SetSetting(attributeName, translatedValue.FromUTF8(), true);
			}
		}
		else if (element_name == elmt_attribute)
		{
			// This is an element in the form “<attribute id="attributeName"><keep>Don’t translate this part
			// </keep><translate>but translate this one.</translate></attribute>”.
			CStr attributeName(child.GetAttributes().GetNamedItem(attr_id)); // Read the attribute name.
			if (attributeName.empty())
			{
				LOGERROR("GUI: ‘attribute’ XML element with empty ‘id’ XML attribute found. (object: %s)", object->GetPresentableName().c_str());
				continue;
			}

			CStr translatedValue;

			for (XMBElement grandchild : child.GetChildNodes())
			{
				if (grandchild.GetNodeName() == elmt_translate)
					translatedValue += g_L10n.Translate(grandchild.GetText());
				else if (grandchild.GetNodeName() == elmt_keep)
					translatedValue += grandchild.GetText();
			}
			object->SetSetting(attributeName, translatedValue.FromUTF8(), true);
		}
		else if (element_name == elmt_include)
		{
			CStrW filename(child.GetAttributes().GetNamedItem(attr_file).FromUTF8());
			CStrW directory(child.GetAttributes().GetNamedItem(attr_directory).FromUTF8());
			if (!filename.empty())
			{
				if (!directory.empty())
					LOGWARNING("GUI: Include element found with file name (%s) and directory name (%s). Only the file will be processed.", utf8_from_wstring(filename), utf8_from_wstring(directory));

				Paths.insert(filename);

				CXeromyces XeroIncluded;
				if (XeroIncluded.Load(g_VFS, filename, "gui") != PSRETURN_OK)
				{
					LOGERROR("GUI: Error reading included XML: '%s'", utf8_from_wstring(filename));
					continue;
				}

				XMBElement node = XeroIncluded.GetRoot();
				if (node.GetNodeName() != XeroIncluded.GetElementID("object"))
				{
					LOGERROR("GUI: Error reading included XML: '%s', root element must have be of type 'object'.", utf8_from_wstring(filename));
					continue;
				}

				if (nesting_depth+1 >= MAX_OBJECT_DEPTH)
				{
					LOGERROR("GUI: Too many nested GUI includes. Probably caused by a recursive include attribute. Abort rendering '%s'.", utf8_from_wstring(filename));
					continue;
				}

				Xeromyces_ReadObject(node, &XeroIncluded, object, NameSubst, Paths, nesting_depth+1);
			}
			else if (!directory.empty())
			{
				if (nesting_depth+1 >= MAX_OBJECT_DEPTH)
				{
					LOGERROR("GUI: Too many nested GUI includes. Probably caused by a recursive include attribute. Abort rendering '%s'.", utf8_from_wstring(directory));
					continue;
				}

				VfsPaths pathnames;
				vfs::GetPathnames(g_VFS, directory, L"*.xml", pathnames);
				for (const VfsPath& path : pathnames)
				{
					// as opposed to loading scripts, don't care if it's loaded before
					// one might use the same parts of the GUI in different situations
					Paths.insert(path);
					CXeromyces XeroIncluded;
					if (XeroIncluded.Load(g_VFS, path, "gui") != PSRETURN_OK)
					{
						LOGERROR("GUI: Error reading included XML: '%s'", path.string8());
						continue;
					}

					XMBElement node = XeroIncluded.GetRoot();
					if (node.GetNodeName() != XeroIncluded.GetElementID("object"))
					{
						LOGERROR("GUI: Error reading included XML: '%s', root element must have be of type 'object'.", path.string8());
						continue;
					}
					Xeromyces_ReadObject(node, &XeroIncluded, object, NameSubst, Paths, nesting_depth+1);
				}

			}
			else
				LOGERROR("GUI: 'include' XML element must have valid 'file' or 'directory' attribute found. (object %s)", object->GetPresentableName().c_str());
		}
		else
		{
			// Try making the object read the tag.
			if (!object->HandleAdditionalChildren(child, pFile))
				LOGERROR("GUI: (object: %s) Reading unknown children for its type", object->GetPresentableName().c_str());
		}
	}

	if (!ManuallySetZ)
	{
		// Set it automatically to 10 plus its parents
		bool absolute;
		GUI<bool>::GetSetting(object, "absolute", absolute);

		if (absolute)
			// If the object is absolute, we'll have to get the parent's Z buffered,
			// and add to that!
			GUI<float>::SetSetting(object, "z", pParent->GetBufferedZ() + 10.f, true);
		else
			// If the object is relative, then we'll just store Z as "10"
			GUI<float>::SetSetting(object, "z", 10.f, true);
	}

	try
	{
		if (pParent == m_BaseObject)
			AddObject(object);
		else
			pParent->AddChild(object);
	}
	catch (PSERROR_GUI& e)
	{
		LOGERROR("GUI error: %s", e.what());
	}
}

void CGUI::Xeromyces_ReadRepeat(XMBElement Element, CXeromyces* pFile, IGUIObject* pParent, std::vector<std::pair<CStr, CStr> >& NameSubst, boost::unordered_set<VfsPath>& Paths, u32 nesting_depth)
{
	#define ELMT(x) int elmt_##x = pFile->GetElementID(#x)
	#define ATTR(x) int attr_##x = pFile->GetAttributeID(#x)
	ELMT(object);
	ATTR(count);
	ATTR(var);

	XMBAttributeList attributes = Element.GetAttributes();

	int count = CStr(attributes.GetNamedItem(attr_count)).ToInt();
	CStr var("["+attributes.GetNamedItem(attr_var)+"]");
	if (var.size() < 3)
		var = "[n]";

	for (int n = 0; n < count; ++n)
	{
		NameSubst.emplace_back(var, "[" + CStr::FromInt(n) + "]");

		XERO_ITER_EL(Element, child)
		{
			if (child.GetNodeName() == elmt_object)
				Xeromyces_ReadObject(child, pFile, pParent, NameSubst, Paths, nesting_depth);
		}
		NameSubst.pop_back();
	}
}

void CGUI::Xeromyces_ReadScript(XMBElement Element, CXeromyces* pFile, boost::unordered_set<VfsPath>& Paths)
{
	// Check for a 'file' parameter
	CStrW file(Element.GetAttributes().GetNamedItem(pFile->GetAttributeID("file")).FromUTF8());

	// If there is a file specified, open and execute it
	if (!file.empty())
	{
		Paths.insert(file);
		try
		{
			m_ScriptInterface->LoadGlobalScriptFile(file);
		}
		catch (PSERROR_Scripting& e)
		{
			LOGERROR("GUI: Error executing script %s: %s", utf8_from_wstring(file), e.what());
		}
	}

	// If it has a directory attribute, read all JS files in that directory
	CStrW directory(Element.GetAttributes().GetNamedItem(pFile->GetAttributeID("directory")).FromUTF8());
	if (!directory.empty())
	{
		VfsPaths pathnames;
		vfs::GetPathnames(g_VFS, directory, L"*.js", pathnames);
		for (const VfsPath& path : pathnames)
		{
			// Only load new files (so when the insert succeeds)
			if (Paths.insert(path).second)
				try
				{
					m_ScriptInterface->LoadGlobalScriptFile(path);
				}
				catch (PSERROR_Scripting& e)
				{
					LOGERROR("GUI: Error executing script %s: %s", path.string8(), e.what());
				}
		}
	}

	// Execute inline scripts
	try
	{
		CStr code(Element.GetText());
		if (!code.empty())
			m_ScriptInterface->LoadGlobalScript(L"Some XML file", code.FromUTF8());
	}
	catch (PSERROR_Scripting& e)
	{
		LOGERROR("GUI: Error executing inline script: %s", e.what());
	}
}

void CGUI::Xeromyces_ReadSprite(XMBElement Element, CXeromyces* pFile)
{
	CGUISprite* Sprite = new CGUISprite;

	// Get name, we know it exists because of DTD requirements
	CStr name = Element.GetAttributes().GetNamedItem(pFile->GetAttributeID("name"));

	if (m_Sprites.find(name) != m_Sprites.end())
		LOGWARNING("GUI sprite name '%s' used more than once; first definition will be discarded", name.c_str());

	SGUIImageEffects* effects = NULL;

	for (XMBElement child : Element.GetChildNodes())
	{
		CStr ElementName(pFile->GetElementString(child.GetNodeName()));

		if (ElementName == "image")
			Xeromyces_ReadImage(child, pFile, *Sprite);
		else if (ElementName == "effect")
		{
			if (effects)
				LOGERROR("GUI <sprite> must not have more than one <effect>");
			else
			{
				effects = new SGUIImageEffects;
				Xeromyces_ReadEffects(child, pFile, *effects);
			}
		}
		else
			debug_warn(L"Invalid data - DTD shouldn't allow this");
	}

	// Apply the effects to every image (unless the image overrides it with
	// different effects)
	if (effects)
		for (SGUIImage* const& img : Sprite->m_Images)
			if (!img->m_Effects)
				img->m_Effects = new SGUIImageEffects(*effects); // do a copy just so it can be deleted correctly later

	delete effects;

	m_Sprites[name] = Sprite;
}

void CGUI::Xeromyces_ReadImage(XMBElement Element, CXeromyces* pFile, CGUISprite& parent)
{
	SGUIImage* Image = new SGUIImage;

	Image->m_TextureSize = CClientArea(CRect(0, 0, 0, 0), CRect(0, 0, 100, 100));
	Image->m_Size = CClientArea(CRect(0, 0, 0, 0), CRect(0, 0, 100, 100));

	// TODO Gee: Setup defaults here (or maybe they are in the SGUIImage ctor)

	for (XMBAttribute attr : Element.GetAttributes())
	{
		CStr attr_name(pFile->GetAttributeString(attr.Name));
		CStrW attr_value(attr.Value.FromUTF8());

		if (attr_name == "texture")
		{
			Image->m_TextureName = VfsPath("art/textures/ui") / attr_value;
		}
		else if (attr_name == "size")
		{
			CClientArea ca;
			if (!GUI<CClientArea>::ParseString(attr_value, ca))
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, utf8_from_wstring(attr_value));
			else
				Image->m_Size = ca;
		}
		else if (attr_name == "texture_size")
		{
			CClientArea ca;
			if (!GUI<CClientArea>::ParseString(attr_value, ca))
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, utf8_from_wstring(attr_value));
			else
				Image->m_TextureSize = ca;
		}
		else if (attr_name == "real_texture_placement")
		{
			CRect rect;
			if (!GUI<CRect>::ParseString(attr_value, rect))
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, utf8_from_wstring(attr_value));
			else
				Image->m_TexturePlacementInFile = rect;
		}
		else if (attr_name == "cell_size")
		{
			CSize size;
			if (!GUI<CSize>::ParseString(attr_value, size))
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, utf8_from_wstring(attr_value));
			else
				Image->m_CellSize = size;
		}
		else if (attr_name == "fixed_h_aspect_ratio")
		{
			float val;
			if (!GUI<float>::ParseString(attr_value, val))
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, utf8_from_wstring(attr_value));
			else
				Image->m_FixedHAspectRatio = val;
		}
		else if (attr_name == "round_coordinates")
		{
			bool b;
			if (!GUI<bool>::ParseString(attr_value, b))
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, utf8_from_wstring(attr_value));
			else
				Image->m_RoundCoordinates = b;
		}
		else if (attr_name == "wrap_mode")
		{
			if (attr_value == L"repeat")
				Image->m_WrapMode = GL_REPEAT;
			else if (attr_value == L"mirrored_repeat")
				Image->m_WrapMode = GL_MIRRORED_REPEAT;
			else if (attr_value == L"clamp_to_edge")
				Image->m_WrapMode = GL_CLAMP_TO_EDGE;
			else
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, utf8_from_wstring(attr_value));
		}
		else if (attr_name == "z_level")
		{
			float z_level;
			if (!GUI<float>::ParseString(attr_value, z_level))
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, utf8_from_wstring(attr_value));
			else
				Image->m_DeltaZ = z_level/100.f;
		}
		else if (attr_name == "backcolor")
		{
			CColor color;
			if (!GUI<CColor>::ParseString(attr_value, color))
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, utf8_from_wstring(attr_value));
			else
				Image->m_BackColor = color;
		}
		else if (attr_name == "bordercolor")
		{
			CColor color;
			if (!GUI<CColor>::ParseString(attr_value, color))
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, utf8_from_wstring(attr_value));
			else
				Image->m_BorderColor = color;
		}
		else if (attr_name == "border")
		{
			bool b;
			if (!GUI<bool>::ParseString(attr_value, b))
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, utf8_from_wstring(attr_value));
			else
				Image->m_Border = b;
		}
		else
			debug_warn(L"Invalid data - DTD shouldn't allow this");
	}

	// Look for effects
	for (XMBElement child : Element.GetChildNodes())
	{
		CStr ElementName(pFile->GetElementString(child.GetNodeName()));
		if (ElementName == "effect")
		{
			if (Image->m_Effects)
				LOGERROR("GUI <image> must not have more than one <effect>");
			else
			{
				Image->m_Effects = new SGUIImageEffects;
				Xeromyces_ReadEffects(child, pFile, *Image->m_Effects);
			}
		}
		else
			debug_warn(L"Invalid data - DTD shouldn't allow this");
	}

	parent.AddImage(Image);
}

void CGUI::Xeromyces_ReadEffects(XMBElement Element, CXeromyces* pFile, SGUIImageEffects& effects)
{
	for (XMBAttribute attr : Element.GetAttributes())
	{
		CStr attr_name(pFile->GetAttributeString(attr.Name));
		CStrW attr_value(attr.Value.FromUTF8());

		if (attr_name == "add_color")
		{
			CColor color;
			if (!GUI<int>::ParseColor(attr_value, color, 0))
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, utf8_from_wstring(attr_value));
			else effects.m_AddColor = color;
		}
		else if (attr_name == "grayscale")
			effects.m_Greyscale = true;
		else
			debug_warn(L"Invalid data - DTD shouldn't allow this");
	}
}

void CGUI::Xeromyces_ReadStyle(XMBElement Element, CXeromyces* pFile)
{
	SGUIStyle style;
	CStr name;

	for (XMBAttribute attr : Element.GetAttributes())
	{
		CStr attr_name(pFile->GetAttributeString(attr.Name));

		// The "name" setting is actually the name of the style
		//  and not a new default
		if (attr_name == "name")
			name = attr.Value;
		else
			style.m_SettingsDefaults[attr_name] = attr.Value.FromUTF8();
	}

	m_Styles[name] = style;
}

void CGUI::Xeromyces_ReadScrollBarStyle(XMBElement Element, CXeromyces* pFile)
{
	SGUIScrollBarStyle scrollbar;
	CStr name;

	// Setup some defaults.
	scrollbar.m_MinimumBarSize = 0.f;
	// Using 1.0e10 as a substitute for infinity
	scrollbar.m_MaximumBarSize = 1.0e10;
	scrollbar.m_UseEdgeButtons = false;

	for (XMBAttribute attr : Element.GetAttributes())
	{
		CStr attr_name = pFile->GetAttributeString(attr.Name);
		CStr attr_value(attr.Value);

		if (attr_value == "null")
			continue;

		if (attr_name == "name")
			name = attr_value;
		else if (attr_name == "show_edge_buttons")
		{
			bool b;
			if (!GUI<bool>::ParseString(attr_value.FromUTF8(), b))
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, attr_value);
			else
				scrollbar.m_UseEdgeButtons = b;
		}
		else if (attr_name == "width")
		{
			float f;
			if (!GUI<float>::ParseString(attr_value.FromUTF8(), f))
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, attr_value);
			else
				scrollbar.m_Width = f;
		}
		else if (attr_name == "minimum_bar_size")
		{
			float f;
			if (!GUI<float>::ParseString(attr_value.FromUTF8(), f))
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, attr_value);
			else
				scrollbar.m_MinimumBarSize = f;
		}
		else if (attr_name == "maximum_bar_size")
		{
			float f;
			if (!GUI<float>::ParseString(attr_value.FromUTF8(), f))
				LOGERROR("GUI: Error parsing '%s' (\"%s\")", attr_name, attr_value);
			else
				scrollbar.m_MaximumBarSize = f;
		}
		else if (attr_name == "sprite_button_top")
			scrollbar.m_SpriteButtonTop = attr_value;
		else if (attr_name == "sprite_button_top_pressed")
			scrollbar.m_SpriteButtonTopPressed = attr_value;
		else if (attr_name == "sprite_button_top_disabled")
			scrollbar.m_SpriteButtonTopDisabled = attr_value;
		else if (attr_name == "sprite_button_top_over")
			scrollbar.m_SpriteButtonTopOver = attr_value;
		else if (attr_name == "sprite_button_bottom")
			scrollbar.m_SpriteButtonBottom = attr_value;
		else if (attr_name == "sprite_button_bottom_pressed")
			scrollbar.m_SpriteButtonBottomPressed = attr_value;
		else if (attr_name == "sprite_button_bottom_disabled")
			scrollbar.m_SpriteButtonBottomDisabled = attr_value;
		else if (attr_name == "sprite_button_bottom_over")
			scrollbar.m_SpriteButtonBottomOver = attr_value;
		else if (attr_name == "sprite_back_vertical")
			scrollbar.m_SpriteBackVertical = attr_value;
		else if (attr_name == "sprite_bar_vertical")
			scrollbar.m_SpriteBarVertical = attr_value;
		else if (attr_name == "sprite_bar_vertical_over")
			scrollbar.m_SpriteBarVerticalOver = attr_value;
		else if (attr_name == "sprite_bar_vertical_pressed")
			scrollbar.m_SpriteBarVerticalPressed = attr_value;
	}

	m_ScrollBarStyles[name] = scrollbar;
}

void CGUI::Xeromyces_ReadIcon(XMBElement Element, CXeromyces* pFile)
{
	SGUIIcon icon;
	CStr name;

	for (XMBAttribute attr : Element.GetAttributes())
	{
		CStr attr_name(pFile->GetAttributeString(attr.Name));
		CStr attr_value(attr.Value);

		if (attr_value == "null")
			continue;

		if (attr_name == "name")
			name = attr_value;
		else if (attr_name == "sprite")
			icon.m_SpriteName = attr_value;
		else if (attr_name == "size")
		{
			CSize size;
			if (!GUI<CSize>::ParseString(attr_value.FromUTF8(), size))
				LOGERROR("Error parsing '%s' (\"%s\") inside <icon>.", attr_name, attr_value);
			else
				icon.m_Size = size;
		}
		else if (attr_name == "cell_id")
		{
			int cell_id;
			if (!GUI<int>::ParseString(attr_value.FromUTF8(), cell_id))
				LOGERROR("GUI: Error parsing '%s' (\"%s\") inside <icon>.", attr_name, attr_value);
			else
				icon.m_CellID = cell_id;
		}
		else
			debug_warn(L"Invalid data - DTD shouldn't allow this");
	}

	m_Icons[name] = icon;
}

void CGUI::Xeromyces_ReadTooltip(XMBElement Element, CXeromyces* pFile)
{
	IGUIObject* object = new CTooltip;

	for (XMBAttribute attr : Element.GetAttributes())
	{
		CStr attr_name(pFile->GetAttributeString(attr.Name));
		CStr attr_value(attr.Value);

		if (attr_name == "name")
			object->SetName("__tooltip_" + attr_value);
		else
			object->SetSetting(attr_name, attr_value.FromUTF8());
	}

	AddObject(object);
}

void CGUI::Xeromyces_ReadColor(XMBElement Element, CXeromyces* pFile)
{
	XMBAttributeList attributes = Element.GetAttributes();

	CColor color;
	CStr name = attributes.GetNamedItem(pFile->GetAttributeID("name"));

	// Try parsing value
	CStr value(Element.GetText());
	if (value.empty())
		return;

	// Try setting color to value
	if (!color.ParseString(value))
	{
		LOGERROR("GUI: Unable to create custom color '%s'. Invalid color syntax.", name.c_str());
		return;
	}

	m_PreDefinedColors[name] = color;
}
