/*
CGUI
by Gustav Larsson
gee@pyro.nu
*/

#include "precompiled.h"

#include "GUI.h"

// Types - when including them into the engine.
#include "CButton.h"
#include "CText.h"
#include "CCheckBox.h"
#include "CRadioButton.h"

#include "ps/Xeromyces.h"

#include "Prometheus.h"
#include "input.h"
#include "OverlayText.h"
// TODO Gee: Whatever include CRect/CPos/CSize
#include "Overlay.h"

#include "scripting/ScriptingHost.h"
#include "Hotkey.h"

#include <string>
#include <assert.h>
#include <stdarg.h>

// namespaces used
using namespace std;

#include "ps/CLogger.h"
#define XERO_TIME

// Class for global JavaScript object
JSClass GUIClass = {
	"GUIClass", 0,
	JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
};


extern int g_xres, g_yres;


//-------------------------------------------------------------------
//	called from main loop when (input) events are received.
//	event is passed to other handlers if false is returned.
//	trampoline: we don't want to make the implementation (in CGUI) static
//-------------------------------------------------------------------
int gui_handler(const SDL_Event* ev)
{
	return g_GUI.HandleEvent(ev);
}

int CGUI::HandleEvent(const SDL_Event* ev)
{
	// MT: If something's gone wrong, check this block... (added for hotkey support)

	if( ev->type == SDL_GUIHOTKEYPRESS )
	{
		const CStr& objectName = *( (CStr*)ev->user.code );
		IGUIObject* object = FindObjectByName( objectName );
		object->HandleMessage( SGUIMessage( GUIM_PRESSED ) );
		object->ScriptEvent( "press" );
	}

	// -- MT

	if(ev->type == SDL_MOUSEMOTION)
	{
		m_MousePos = CPos(ev->motion.x, ev->motion.y);

		GUI<SGUIMessage>::RecurseObject(GUIRR_HIDDEN | GUIRR_GHOST, m_BaseObject, 
										&IGUIObject::HandleMessage, 
										SGUIMessage(GUIM_MOUSE_MOTION));
	}

	// TODO Gee: temp-stuff
//	char buf[30];
//	sprintf(buf, "type = %d", ev->type);
	//TEMPmessage = buf;

	// Update m_MouseButtons. (BUTTONUP is handled later.)
	if (ev->type == SDL_MOUSEBUTTONDOWN)
	{
		// (0,1,2) = (LMB,RMB,MMB)
		if (ev->button.button < 3)
			m_MouseButtons |= (1 << ev->button.button);
	}

// JW: (pre|post)process omitted; what're they for? why would we need any special button_released handling?

	// Only one object can be hovered
	IGUIObject *pNearest = NULL;

	try
	{
		// TODO Gee: Optimizations needed!
		//  these two recursive function are quite overhead heavy.

		// pNearest will after this point at the hovered object, possibly NULL
		GUI<IGUIObject*>::RecurseObject(GUIRR_HIDDEN | GUIRR_GHOST, m_BaseObject, 
										&IGUIObject::ChooseMouseOverAndClosest, 
										pNearest);

		if (ev->type == SDL_MOUSEMOTION && pNearest)
			pNearest->ScriptEvent("mousemove");

		// Now we'll call UpdateMouseOver on *all* objects,
		//  we'll input the one hovered, and they will each
		//  update their own data and send messages accordingly
		
		GUI<IGUIObject*>::RecurseObject(GUIRR_HIDDEN | GUIRR_GHOST, m_BaseObject, 
										&IGUIObject::UpdateMouseOver, 
										pNearest);

		if (ev->type == SDL_MOUSEBUTTONDOWN)
		{
			switch (ev->button.button)
			{
			case SDL_BUTTON_LEFT:
				if (pNearest)
				{
					pNearest->HandleMessage(SGUIMessage(GUIM_MOUSE_PRESS_LEFT));
					pNearest->ScriptEvent("mouseleftpress");
				}
				break;

			case SDL_BUTTON_WHEELDOWN: // wheel down
				if (pNearest)
				{
					pNearest->HandleMessage(SGUIMessage(GUIM_MOUSE_WHEEL_DOWN));
					pNearest->ScriptEvent("mousewheeldown");
				}
				break;

			case SDL_BUTTON_WHEELUP: // wheel up
				if (pNearest)
				{
					pNearest->HandleMessage(SGUIMessage(GUIM_MOUSE_WHEEL_UP));
					pNearest->ScriptEvent("mousewheelup");
				}
				break;

			default:
				break;
			}
			
		}
		else 
		if (ev->type == SDL_MOUSEBUTTONUP)
		{
			if (ev->button.button == SDL_BUTTON_LEFT)
			{
				if (pNearest)
				{
					pNearest->HandleMessage(SGUIMessage(GUIM_MOUSE_RELEASE_LEFT));
					pNearest->ScriptEvent("mouseleftrelease");
				}
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
	catch (PS_RESULT e)
	{
		UNUSED(e);
		// TODO Gee: Handle
	}
// JW: what's the difference between mPress and mDown? what's the code below responsible for?
/*
	// Generally if just mouse is clicked
	if (m_pInput->mDown(NEMM_BUTTON1) && pNearest)
	{
		pNearest->HandleMessage(GUIM_MOUSE_DOWN_LEFT);
	}
*/

	// BUTTONUP's effect on m_MouseButtons is handled after
	// everything else, so that e.g. 'press' handlers (activated
	// on button up) see which mouse button had been pressed.
	if (ev->type == SDL_MOUSEBUTTONUP)
	{
		// (0,1,2) = (LMB,RMB,MMB)
		if (ev->button.button < 3)
			m_MouseButtons &= ~(1 << ev->button.button);
	}

	return EV_PASS;
}

void CGUI::TickObjects()
{
	CStr action = "tick";
	GUI<CStr>::RecurseObject(0, m_BaseObject, 
							&IGUIObject::ScriptEvent, action);
}

//-------------------------------------------------------------------
//  Constructor / Destructor
//-------------------------------------------------------------------
CGUI::CGUI() : m_InternalNameNumber(0), m_MouseButtons(0)
{
	m_BaseObject = new CGUIDummyObject;
	m_BaseObject->SetGUI(this);

	// Construct the parent object for all GUI JavaScript things
	m_ScriptObject = (void*)JS_NewObject(g_ScriptingHost.getContext(), &GUIClass, NULL, NULL);
	assert(m_ScriptObject != NULL); // How should it handle errors?
	JS_AddRoot(g_ScriptingHost.getContext(), &m_ScriptObject);

	// This will make this invisible, not add
	//m_BaseObject->SetName(BASE_OBJECT_NAME);
}

CGUI::~CGUI()
{
	if (m_BaseObject)
		delete m_BaseObject;

	if (m_ScriptObject)
		// Let it be garbage-collected
		JS_RemoveRoot(g_ScriptingHost.getContext(), &m_ScriptObject);

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
		// TODO Gee: Report in log
		return NULL;
	}
}

void CGUI::Initialize()
{
	// Add base types!
	//  You can also add types outside the GUI to extend the flexibility of the GUI.
	//  Prometheus though will have all the object types inserted from here.
	AddObjectType("empty",			&CGUIDummyObject::ConstructObject);
	AddObjectType("button",			&CButton::ConstructObject);
	AddObjectType("text",			&CText::ConstructObject);
	AddObjectType("checkbox",		&CCheckBox::ConstructObject);
	AddObjectType("radiobutton",	&CRadioButton::ConstructObject);
}

void CGUI::Process()
{
/*

	// TODO Gee: check if m_pInput is valid, otherwise return
///	assert(m_pInput);

	// Pre-process all objects
	try
	{
		GUI<EGUIMessage>::RecurseObject(0, m_BaseObject, &IGUIObject::HandleMessage, GUIM_PREPROCESS);
	}
	catch (PS_RESULT e)
	{
		return;
	}

	// Check mouse over
	try
	{
		// Only one object can be hovered
		//  check which one it is, if any !
		IGUIObject *pNearest = NULL;

		GUI<IGUIObject*>::RecurseObject(GUIRR_HIDDEN, m_BaseObject, &IGUIObject::ChooseMouseOverAndClosest, pNearest);
		
		// Now we'll call UpdateMouseOver on *all* objects,
		//  we'll input the one hovered, and they will each
		//  update their own data and send messages accordingly
		GUI<IGUIObject*>::RecurseObject(GUIRR_HIDDEN, m_BaseObject, &IGUIObject::UpdateMouseOver, pNearest);

		// If pressed
		if (m_pInput->mPress(NEMM_BUTTON1) && pNearest)
		{
			pNearest->HandleMessage(GUIM_MOUSE_PRESS_LEFT);
		}
		else
		// If released
		if (m_pInput->mRelease(NEMM_BUTTON1) && pNearest)
		{
			pNearest->HandleMessage(GUIM_MOUSE_RELEASE_LEFT);
		}

		// Generally if just mouse is clicked
		if (m_pInput->mDown(NEMM_BUTTON1) && pNearest)
		{
			pNearest->HandleMessage(GUIM_MOUSE_DOWN_LEFT);
		}

	}
	catch (PS_RESULT e)
	{
		return;
	}

	// Post-process all objects
	try
	{
		GUI<EGUIMessage>::RecurseObject(0, m_BaseObject, &IGUIObject::HandleMessage, GUIM_POSTPROCESS);
	}
	catch (PS_RESULT e)
	{
		return;
	}
*/
}

void CGUI::Draw()
{
	// Clear the depth buffer, so the GUI is
	// drawn on top of everything else
	glClear(GL_DEPTH_BUFFER_BIT);

	glPushMatrix();
	glLoadIdentity();

	// Adapt (origio) to being in top left corner and down
	//  just like the mouse position
	glTranslatef(0.0f, (GLfloat)g_yres, -1000.0f);
	glScalef(1.0f, -1.f, 1.0f);

	try
	{
		// Recurse IGUIObject::Draw() with restriction: hidden
		//  meaning all hidden objects won't call Draw (nor will it recurse its children)
		GUI<>::RecurseObject(GUIRR_HIDDEN, m_BaseObject, &IGUIObject::Draw);
	}
	catch (PS_RESULT e)
	{
		UNUSED(e);
		glPopMatrix();

		// TODO Gee: Report error.
		return;
	}
	glPopMatrix();
}

void CGUI::DrawSprite(const CStr& SpriteName, 
					  const float &Z, 
					  const CRect &Rect, 
					  const CRect &Clipping)
{
	// This is not an error, it's just a choice not to draw any sprite.
	if (SpriteName == CStr())
		return;

	// TODO: Clipping?
	bool DoClipping = (Clipping != CRect());

	CGUISprite Sprite;

	// Fetch real sprite from name
	if (m_Sprites.count(SpriteName) == 0)
	{
		// TODO Gee: Report error
		return;
	}
	else Sprite = m_Sprites[SpriteName];

	glPushMatrix();
	glTranslatef(0.0f, 0.0f, Z);

	// Iterate all images and request them being drawn be the
	//  CRenderer
	std::vector<SGUIImage>::const_iterator cit;
	for (cit=Sprite.m_Images.begin(); cit!=Sprite.m_Images.end(); ++cit)
	{
		if (cit->m_Texture)
		{
			// TODO: Handle the GL state in a nicer way

			glEnable(GL_TEXTURE_2D);
			glColor3f(1.0f, 1.0f, 1.0f);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

			int fmt;
			tex_info(cit->m_Texture, NULL, NULL, &fmt, NULL, NULL);
			if (fmt == GL_RGBA || fmt == GL_BGRA)
			{
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glEnable(GL_BLEND);
			}
			else
			{
				glDisable(GL_BLEND);
			}

			tex_bind(cit->m_Texture);

			CRect real = cit->m_Size.GetClientArea(Rect);

			// Get the screen position/size of a single tiling of the texture
			CRect TexSize = cit->m_TextureSize.GetClientArea(real);

			float TexLeft = (float)(TexSize.left - real.left) / (float)TexSize.GetWidth();
			float TexRight = TexLeft + (float)real.GetWidth() / (float)TexSize.GetWidth();

			// 'Bottom' is actually the top in screen-space (I think),
			// because the GUI puts (0,0) at the top-left
			float TexBottom = (float)(TexSize.bottom - real.bottom) / (float)TexSize.GetHeight();
			float TexTop = TexBottom + (float)real.GetHeight() / (float)TexSize.GetHeight();

			glBegin(GL_QUADS);
			glTexCoord2f(TexRight,	TexBottom);	glVertex3f((float)real.right,	(float)real.bottom,	cit->m_DeltaZ);
			glTexCoord2f(TexLeft,	TexBottom);	glVertex3f((float)real.left,	(float)real.bottom,	cit->m_DeltaZ);
			glTexCoord2f(TexLeft,	TexTop);	glVertex3f((float)real.left,	(float)real.top,	cit->m_DeltaZ);
			glTexCoord2f(TexRight,	TexTop);	glVertex3f((float)real.right,	(float)real.top,	cit->m_DeltaZ);
			glEnd();

			glDisable(GL_TEXTURE_2D);

		}
		else
		{
			glColor3f(cit->m_BackColor.r, cit->m_BackColor.g, cit->m_BackColor.b);

			CRect real = cit->m_Size.GetClientArea(Rect);

			glBegin(GL_QUADS);
			glVertex3f((float)real.right,	(float)real.bottom,	cit->m_DeltaZ);
			glVertex3f((float)real.left,	(float)real.bottom,	cit->m_DeltaZ);
			glVertex3f((float)real.left,	(float)real.top,	cit->m_DeltaZ);
			glVertex3f((float)real.right,	(float)real.top,	cit->m_DeltaZ);
			glEnd();
		}
	}
	glPopMatrix();
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
		catch (PS_RESULT e)
		{
			UNUSED(e);
			// TODO Gee: Handle
		}
		
		delete it->second;
	}

	// Clear all
	m_pAllObjects.clear();
	m_Sprites.clear();
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
		m_BaseObject->AddChild(pObject);

		// Cache tree
		GUI<>::RecurseObject(0, pObject, &IGUIObject::UpdateCachedSize);

		// Loaded
		GUI<SGUIMessage>::RecurseObject(0, pObject, &IGUIObject::HandleMessage, SGUIMessage(GUIM_LOAD));
		GUI<CStr>::RecurseObject(0, pObject, &IGUIObject::ScriptEvent, "load");
	}
	catch (PS_RESULT e)
	{
		throw e;
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
	catch (PS_RESULT e)
	{
		// Throw the same error
		throw e;
	}

	// Else actually update the real one
	m_pAllObjects = AllObjects;
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


// private struct used only in GenerateText(...)
struct SGenerateTextImage
{
	int m_YFrom,		// The images starting location in Y
		m_YTo,			// The images end location in Y
		m_Indentation;	// The image width in other words

	// Some help functions
	// TODO Gee: CRect => CPoint ?
	void SetupSpriteCall(const bool &Left, SGUIText::SSpriteCall &SpriteCall, 
						 const int &width, const int &y,
						 const CSize &Size, const CStr& TextureName, 
						 const int &BufferZone)
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

		SpriteCall.m_TextureName = TextureName;

		m_YFrom = SpriteCall.m_Area.top-BufferZone;
		m_YTo = SpriteCall.m_Area.bottom+BufferZone;
		m_Indentation = Size.cx+BufferZone*2;
	}
};

SGUIText CGUI::GenerateText(const CGUIString &string, /*const CColor &Color, */
							const CStr& Font, const int &Width, const int &BufferZone)
{
	SGUIText Text; // object we're generating
	
	if (string.m_Words.size() == 0)
		return Text;

	int x=BufferZone, y=BufferZone; // drawing pointer
	int from=0;
	bool done=false;

	// Images on the left or the right side.
	vector<SGenerateTextImage> Images[2];
	int pos_last_img=-1;	// Position in the string where last img (either left or right) were encountered.
							//  in order to avoid duplicate processing.

	// Easier to read.
	bool WordWrapping = (Width != 0);

	// Go through string word by word
	for (int i=0; i<(int)string.m_Words.size()-1 && !done; ++i)
	{
		// Pre-process each line one time, so we know which floating images
		//  will be added for that line.

		// Generated stuff is stored in Feedback.
		CGUIString::SFeedback Feedback;

		// Preliminary line_height, used for word-wrapping with floating images.
		int prelim_line_height=0;

		// Width and height of all text calls generated.
		string.GenerateTextCall(Feedback, Font, /*CColor(),*/
								string.m_Words[i], string.m_Words[i+1]);

		// Loop through our images queues, to see if images has been added.
		
		// Check if this has already been processed.
		//  Also, floating images are only applicable if Word-Wrapping is on
		if (WordWrapping && i > pos_last_img)
		{
			// Loop left/right
			for (int j=0; j<2; ++j)
			{
				for (vector<CStr>::const_iterator it = Feedback.m_Images[j].begin(); 
					it != Feedback.m_Images[j].end(); 
					++it)
				{
					SGUIText::SSpriteCall SpriteCall;
					SGenerateTextImage Image;

					// Y is if no other floating images is above, y. Else it is placed
					//  after the last image, like a stack downwards.
					int _y;
					if (Images[j].size() > 0)
						_y = max(y, Images[j].back().m_YTo);
					else
						_y = y; 

					// TODO Gee: CSize temp
					CSize size; size.cx = 100; size.cy = 100;
					Image.SetupSpriteCall((j==CGUIString::SFeedback::Left), SpriteCall, Width, _y, size, CStr("white-border"), BufferZone);

					// Check if image is the lowest thing.
					Text.m_Size.cy = max(Text.m_Size.cy, Image.m_YTo);

					Images[j].push_back(Image);
					Text.m_SpriteCalls.push_back(SpriteCall);
				}
			}
		}

		pos_last_img = max(pos_last_img, i);

		x += Feedback.m_Size.cx;
		prelim_line_height = max(prelim_line_height, Feedback.m_Size.cy);

		// If Width is 0, then there's no word-wrapping, disable NewLine.
		if ((WordWrapping && (x > Width-BufferZone || Feedback.m_NewLine)) || i == (int)string.m_Words.size()-2)
		{
			// Change from to i, but first keep a copy of its value.
			int temp_from = from;
			from = i;

			static const int From=0, To=1;
			//int width_from=0, width_to=width;
			int width_range[2];
			width_range[From] = BufferZone;
			width_range[To] = Width - BufferZone;

			// Floating images are only appicable if word-wrapping is enabled.
			if (WordWrapping)
			{
				// Decide width of the line. We need to iterate our floating images.
				//  this won't be exact because we're assuming the line_height
				//  will be as our preliminary calculation said. But that may change,
				//  although we'd have to add a couple of more loops to try straightening
				//  this problem out, and it is very unlikely to happen noticably if one
				//  stuctures his text in a stylistically pure fashion. Even if not, it
				//  is still quite unlikely it will happen.
				// Loop through left and right side, from and to.
				for (int j=0; j<2; ++j)
				{
					for (vector<SGenerateTextImage>::const_iterator it = Images[j].begin(); 
						it != Images[j].end(); 
						++it)
					{
						// We're working with two intervals here, the image's and the line height's.
						//  let's find the union of these two.
						int union_from, union_to;

						union_from = max(y, it->m_YFrom);
						union_to = min(y+prelim_line_height, it->m_YTo);
						
						// The union is not empty
						if (union_to > union_from)
						{
							if (j == From)
								width_range[From] = max(width_range[From], it->m_Indentation);
							else
								width_range[To] = min(width_range[To], Width - it->m_Indentation);
						}
					}
				}
			}

			// Reset X for the next loop
			x = width_range[From];

			// Now we'll do another loop to figure out the height of
			//  the line (the height of the largest character). This
			//  couldn't be determined in the first loop (main loop)
			//  because it didn't regard images, so we don't know
			//  if all characters processed, will actually be involved
			//  in that line.
			int line_height=0;
			for (int j=temp_from; j<=i; ++j)
			{
				// We don't want to use Feedback now, so we'll have to use
				//  another.
				CGUIString::SFeedback Feedback2;

				string.GenerateTextCall(Feedback2, Font, /*CColor(),*/
										string.m_Words[j], string.m_Words[j+1]);

				// Append X value.
				x += Feedback2.m_Size.cx;

				if (WordWrapping && x > width_range[To] && j!=temp_from && !Feedback2.m_NewLine)
					break;

				// Let line_height be the maximum m_Height we encounter.
				line_height = max(line_height, Feedback2.m_Size.cy);

				if (WordWrapping && Feedback2.m_NewLine)
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
				//  another.
				CGUIString::SFeedback Feedback2;

				// Defaults
				string.GenerateTextCall(Feedback2, Font, /*Color, */
										string.m_Words[j], string.m_Words[j+1]);

				// Iterate all and set X/Y values
				// Since X values are not set, we need to make an internal
				//  iteration with an increment that will append the internal
				//  x, that is what x_pointer is for.
				int x_pointer=0;

				vector<SGUIText::STextCall>::iterator it;
				for (it = Feedback2.m_TextCalls.begin(); it != Feedback2.m_TextCalls.end(); ++it)
				{
					it->m_Pos = CPos(x + x_pointer, y);

					x_pointer += it->m_Size.cx;

					if (it->m_pSpriteCall)
					{
						it->m_pSpriteCall->m_Area = 
							it->m_pSpriteCall->m_Area + it->m_Pos;
					}
				}

				// Append X value.
				x += Feedback2.m_Size.cx;

				Text.m_Size.cx = max(Text.m_Size.cx, x+BufferZone);

				// The first word overrides the width limit, that we
				//  do in those cases, are just draw that word even
				//  though it'll extend the object.
				if (WordWrapping) // only if word-wrapping is applicable
				{
					if (Feedback2.m_NewLine)
					{
						from = j+1;
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
			x = 0;

			// Update height of all
			Text.m_Size.cy = max(Text.m_Size.cy, y+BufferZone);

			// Now if we entered as from = i, then we want
			//  i being one minus that, so that it will become
			//  the same i in the next loop. The difference is that
			//  we're on a new line now.
			i = from-1;
		}
	}

	return Text;
}

void CGUI::DrawText(const SGUIText &Text, const CColor &DefaultColor, 
					const CPos &pos, const float &z)
{
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	Handle font = 0;
	CStr LastFontName;

	for (vector<SGUIText::STextCall>::const_iterator it = Text.m_TextCalls.begin(); 
		 it != Text.m_TextCalls.end(); 
		 ++it)
	{
		if (it->m_pSpriteCall)
			continue;

		// Switch fonts when necessary, but remember the last one used
		if (it->m_Font != LastFontName)
		{
			if (font)
				unifont_unload(font);
			font = unifont_load(it->m_Font);
			unifont_bind(font);
			LastFontName = it->m_Font;
		}

		CColor color = it->m_UseCustomColor ? it->m_Color : DefaultColor;

		glPushMatrix();

		glTranslatef((float)pos.x+it->m_Pos.x, (float)pos.y+it->m_Pos.y, (float)z);
		glColor4f(color.r, color.g, color.b, color.a);
		glwprintf(L"%hs", it->m_String.c_str());

		glPopMatrix();

	}

	if (font)
		unifont_unload(font);

	for (vector<SGUIText::SSpriteCall>::const_iterator it=Text.m_SpriteCalls.begin(); 
		 it!=Text.m_SpriteCalls.end(); 
		 ++it)
	{
		DrawSprite(it->m_TextureName, z, it->m_Area + pos);
	}
}

void CGUI::ReportParseError(const CStr& str)
{
	// Print header
	if (m_Errors==0)
	{
		LOG(ERROR, "*** GUI Tree Creation Errors:");
	}

	// Important, set ParseError to true
	++m_Errors;

	LOG(ERROR, str);
}

/**
 * @callgraph
 */
void CGUI::LoadXMLFile(const string &Filename)
{
	// Reset parse error
	//  we can later check if this has increased
	m_Errors = 0;

	CXeromyces XeroFile;
	if (XeroFile.Load(Filename.c_str()) != PSRETURN_OK)
		// Fail silently
		return;

	XMBElement node = XeroFile.getRoot();

	// Check root element's (node) name so we know what kind of
	//  data we'll be expecting
	std::string root_name = XeroFile.getElementString(node.getNodeName());

	try
	{

		if (root_name == "objects")
		{
			Xeromyces_ReadRootObjects(node, &XeroFile);

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
			// TODO Gee: Output in log
		}
	}
	catch (PSERROR_GUI)
	{
		LOG(ERROR, "Errors loading GUI file %s", Filename.c_str());
		return;
	}

	// Now report if any other errors occured
	if (m_Errors > 0)
	{
///		g_console.submit("echo GUI Tree Creation Reports %d errors", m_Errors);
	}

}

//===================================================================
//	XML Reading Xeromyces Specific Sub-Routines
//===================================================================

void CGUI::Xeromyces_ReadRootObjects(XMBElement Element, CXeromyces* pFile)
{
	int el_script = pFile->getElementID("script");

	// Iterate main children
	//  they should all be <object> or <script> elements
	XMBElementList children = Element.getChildNodes();
	for (int i=0; i<children.Count; ++i)
	{
		//debug_out("Object %d\n", i);
		XMBElement child = children.item(i);

		if (child.getNodeName() == el_script)
			// Execute the inline script
			Xeromyces_ReadScript(child, pFile);
		else
			// Read in this whole object into the GUI
			Xeromyces_ReadObject(child, pFile, m_BaseObject);
	}
}

void CGUI::Xeromyces_ReadRootSprites(XMBElement Element, CXeromyces* pFile)
{
	// Iterate main children
	//  they should all be <sprite> elements
	XMBElementList children = Element.getChildNodes();
	for (int i=0; i<children.Count; ++i)
	{
		XMBElement child = children.item(i);

		// Read in this whole object into the GUI
		Xeromyces_ReadSprite(child, pFile);
	}
}

void CGUI::Xeromyces_ReadRootStyles(XMBElement Element, CXeromyces* pFile)
{
	// Iterate main children
	//  they should all be <styles> elements
	XMBElementList children = Element.getChildNodes();
	for (int i=0; i<children.Count; ++i)
	{
		XMBElement child = children.item(i);

		// Read in this whole object into the GUI
		Xeromyces_ReadStyle(child, pFile);
	}
}

void CGUI::Xeromyces_ReadRootSetup(XMBElement Element, CXeromyces* pFile)
{
	// Iterate main children
	//  they should all be <icon>, <scrollbar> or <tooltip>.
	XMBElementList children = Element.getChildNodes();
	for (int i=0; i<children.Count; ++i)
	{
		XMBElement child = children.item(i);

		// Read in this whole object into the GUI

		std::string name = pFile->getElementString(child.getNodeName());

		if (name == "scrollbar")
		{
			Xeromyces_ReadScrollBarStyle(child, pFile);
		}
		// No need for else, we're using DTD.
	}
}

void CGUI::Xeromyces_ReadObject(XMBElement Element, CXeromyces* pFile, IGUIObject *pParent)
{
	assert(pParent);
	int i;

	// Our object we are going to create
	IGUIObject *object = NULL;

	XMBAttributeList attributes = Element.getAttributes();

	// Well first of all we need to determine the type
	utf16string type = attributes.getNamedItem( pFile->getAttributeID("type") );

	// Construct object from specified type
	//  henceforth, we need to do a rollback before aborting.
	//  i.e. releasing this object
	object = ConstructObject((CStr)type);

	if (!object)
	{
		// Report error that object was unsuccessfully loaded
		ReportParseError(CStr("Unrecognized type: ") + CStr(type));

		delete object;
		return;
	}

	// Cache some IDs for element attribute names, to avoid string comparisons
	#define ELMT(x) int elmt_##x = pFile->getElementID(#x)
	#define ATTR(x) int attr_##x = pFile->getAttributeID(#x)
	ELMT(object);
	ELMT(action);
	ATTR(style);
	ATTR(type);
	ATTR(name);
// MT - temp tag
	ATTR(hotkey);
// -- MT
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
	CStr argStyle = (CStr)attributes.getNamedItem(attr_style);

	if (m_Styles.count(CStr("default")) == 1)
		object->LoadStyle(*this, CStr("default"));

    if (argStyle != CStr())
	{
		// additional check
		if (m_Styles.count(argStyle) == 0)
		{
			// TODO Gee: Error
		}
		else object->LoadStyle(*this, argStyle);
	}
	

	//
	//	Read Attributes
	//

	bool NameSet = false;
	bool ManuallySetZ = false; // if z has been manually set, this turn true

// MT - temp tag
	CStr hotkeyTag( "" );
// -- MT

	// Now we can iterate all attributes and store
	for (i=0; i<attributes.Count; ++i)
	{
		XMBAttribute attr = attributes.item(i);

		// If value is "null", then it is equivalent as never being entered
		if ((CStr)attr.Value == (CStr)"null")
			continue;

		// Ignore "type" and "style", we've already checked it
		if (attr.Name == attr_type || attr.Name == attr_style)
			continue;

		// Also the name needs some special attention
		if (attr.Name == attr_name)
		{
			object->SetName((CStr)attr.Value);
			NameSet = true;
			continue;
		}

// MT - temp tag
		// Wire up the hotkey tag, if it has one
		if( attr.Name == attr_hotkey )
			hotkeyTag = attr.Value;
// -- MT

		if (attr.Name == attr_z)
			ManuallySetZ = true;

		
		// Generate "stretched:filename" sprites.
		//
		// Check whether it's actually one of the many sprite... parameters.
		if (pFile->getAttributeString(attr.Name).substr(0, 6) == "sprite")
		{
			// Check whether it's a special stretched one
			std::string SpriteName = CStr8(attr.Value).c_str();
			if (SpriteName.substr(0, 10) == "stretched:" &&
				m_Sprites.find(SpriteName) == m_Sprites.end() )
			{

				CGUISprite sprite;
				SGUIImage image;

				CStr DefaultSize ("0 0 100% 100%");
				image.m_TextureSize = CClientArea(DefaultSize);
				image.m_Size = CClientArea(DefaultSize);

				std::string TexFilename = "art/textures/ui/";
				TexFilename += SpriteName.substr(10);

				image.m_TextureName = TexFilename;
				Handle tex = tex_load(TexFilename.c_str());
				if (tex <= 0)
				{
					LOG(ERROR, "Error opening texture '%s': %lld", TexFilename.c_str(), tex);
					throw PSERROR_GUI_TextureLoadFailed();
				}
				image.m_Texture = tex;
				// TODO: more error handling
				tex_upload(tex);

				sprite.AddImage(image);	
				m_Sprites[SpriteName] = sprite;
			}
		}

		// Try setting the value
		try
		{
			object->SetSetting(pFile->getAttributeString(attr.Name), (CStr)attr.Value);
		}
		catch (PS_RESULT e)
		{
			UNUSED(e);
			ReportParseError(CStr("Can't set \"") + pFile->getAttributeString(attr.Name) + CStr("\" to \"") + (CStr)attr.Value + CStr("\""));

			// This is not a fatal error
		}
	}

	// Check if name isn't set, generate an internal name in that case.
	if (!NameSet)
	{
		object->SetName(CStr("__internal(") + CStr(m_InternalNameNumber) + CStr(")"));
		++m_InternalNameNumber;
	}

// MT - temp tag
	// Attempt to register the hotkey tag, if one was provided
	if( hotkeyTag.Length() )
		hotkeyRegisterGUIObject( object->GetName(), hotkeyTag );
// -- MT

	CStr caption = (CStr)Element.getText();
	caption.Trim(PS_TRIM_BOTH);
	if (caption.Length())
	{
		try
		{
			// Set the setting caption to this
			object->SetSetting("caption", caption);
		}
		catch (PS_RESULT)
		{
			// There is no harm if the object didn't have a "caption"
		}
	}


	//
	//	Read Children
	//

	// Iterate children
	XMBElementList children = Element.getChildNodes();

	for (i=0; i<children.Count; ++i)
	{
		// Get node
		XMBElement child = children.item(i);

		// Check what name the elements got
		int element_name = child.getNodeName();

		if (element_name == elmt_object)
		{
			// TODO Gee: REPORT ERROR

			// Call this function on the child
			Xeromyces_ReadObject(child, pFile, object);
		}
		else if (element_name == elmt_action)
		{
			// Scripted <action> element

			// Check for a 'file' parameter
			CStr file (child.getAttributes().getNamedItem(attr_file));

			CStr code;

			// If there is a file, open it and use it as the code
			if (file.Length())
			{
				CVFSFile scriptfile;
				if (scriptfile.Load(file) != PSRETURN_OK)
				{
					LOG(ERROR, "Error opening action file '%s'", file.c_str());
					throw PSERROR_GUI_JSOpenFailed();
				}

				code = scriptfile.GetAsString();
			}

			// Read the inline code (concatenating to the file code, if both are specified)
			code += (CStr)child.getText();

			CStr action = (CStr)child.getAttributes().getNamedItem(attr_on);
			object->RegisterScriptHandler(action.LowerCase(), code, this);
		}
	} 

	//
	//	Check if Z wasn't manually set
	//
	if (!ManuallySetZ)
	{
		// Set it automatically to 10 plus its parents
		if (pParent==NULL)
		{
			// TODO Gee: Report error
		}
		else
		{
			bool absolute;
			GUI<bool>::GetSetting(object, "absolute", absolute);

			// If the object is absolute, we'll have to get the parent's Z buffered,
			//  and add to that!
			if (absolute)
			{
				GUI<float>::SetSetting(object, "z", pParent->GetBufferedZ() + 10.f);
			}
			else
			// If the object is relative, then we'll just store Z as "10"
			{
				GUI<float>::SetSetting(object, "z", 10.f);
			}
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
	catch (PS_RESULT e)
	{ 
		ReportParseError(e);
	}
}

void CGUI::Xeromyces_ReadScript(XMBElement Element, CXeromyces* pFile)
{

	// Check for a 'file' parameter
	CStr file (Element.getAttributes().getNamedItem( pFile->getAttributeID("file") ));

	// If there is a file specified, open and execute it
	if (file.Length())
	{
		CVFSFile scriptfile;
		if (scriptfile.Load(file) != PSRETURN_OK)
		{
			LOG(ERROR, "Error opening script file '%s'", file.c_str());
			throw PSERROR_GUI_JSOpenFailed();
		}

		jsval result;
		JS_EvaluateScript(g_ScriptingHost.getContext(), (JSObject*)m_ScriptObject, (const char*)scriptfile.GetBuffer(), (int)scriptfile.GetBufferSize(), file, 1, &result);
	}

	// Execute inline scripts
	CStr code (Element.getText());

	if (code.Length())
	{
		jsval result;
		// TODO: Report the filename
		JS_EvaluateScript(g_ScriptingHost.getContext(), (JSObject*)m_ScriptObject, code.c_str(), (int)code.Length(), "Some XML file", Element.getLineNumber(), &result);
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
	name = (CStr) Element.getAttributes().getNamedItem( pFile->getAttributeID("name") );

	//
	//	Read Children (the images)
	//

	// Iterate children
	XMBElementList children = Element.getChildNodes();

	for (int i=0; i<children.Count; ++i)
	{
		// Get node
		XMBElement child = children.item(i);

		// All Elements will be of type "image" by DTD law

		// Call this function on the child
		Xeromyces_ReadImage(child, pFile, sprite);
	}

	//
	//	Add Sprite
	//

	m_Sprites[name] = sprite;
}

void CGUI::Xeromyces_ReadImage(XMBElement Element, CXeromyces* pFile, CGUISprite &parent)
{

	// Image object we're adding
	SGUIImage image;
	
	CStr DefaultTextureSize = "0 0 100% 100%";
	image.m_TextureSize = CClientArea(DefaultTextureSize);
	
	// TODO Gee: Setup defaults here (or maybe they are in the SGUIImage ctor)

	//
	//	Read Attributes
	//

	// Now we can iterate all attributes and store
	XMBAttributeList attributes = Element.getAttributes();
	for (int i=0; i<attributes.Count; ++i)
	{
		XMBAttribute attr = attributes.item(i);
		std::string attr_name = pFile->getAttributeString(attr.Name);
		CStr attr_value (attr.Value);

		// This is the only attribute we want
		if (attr_name == "texture")
		{
			// Load the texture from disk now, because now's as good a time as any
			std::string TexFilename = "art/textures/ui/";
			TexFilename += attr_value;

			image.m_TextureName = TexFilename;
			Handle tex = tex_load(TexFilename.c_str());
			if (tex <= 0)
			{
				LOG(ERROR, "Error opening texture '%s': %lld", TexFilename.c_str(), tex);
				throw PSERROR_GUI_TextureLoadFailed();
			}
			image.m_Texture = tex;
			tex_upload(tex);
		}
		else
		if (attr_name == "size")
		{
			CClientArea ca;
			if (!GUI<CClientArea>::ParseString(attr_value, ca))
			{
				// TODO Gee: Error
			}
			else image.m_Size = ca;
		}
		else
		if (attr_name == "texture-size")
		{
			CClientArea ca;
			if (!GUI<CClientArea>::ParseString(attr_value, ca))
			{
				// TODO Gee: Error
			}
			else image.m_TextureSize = ca;
		}
		else
		if (attr_name == "z-level")
		{
			int z_level;
			if (!GUI<int>::ParseString(attr_value, z_level))
			{
				// TODO Gee: Error
			}
			else image.m_DeltaZ = (float)z_level/100.f;
		}
		else
		if (attr_name == "backcolor")
		{
			CColor color;
			if (!GUI<CColor>::ParseString(attr_value, color))
			{
				// TODO Gee: Error
			}
			else image.m_BackColor = color;
		}
		else
		{
			// TODO Gee: Log
			//g_console.submit("echo Error attribute " + attr_name + " is not expected in <image>");
			return;
		}
	}

	//
	//	Input
	//

	parent.AddImage(image);	
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
	XMBAttributeList attributes = Element.getAttributes();
	for (int i=0; i<attributes.Count; ++i)
	{
		XMBAttribute attr = attributes.item(i);
		std::string attr_name = pFile->getAttributeString(attr.Name);
		CStr attr_value (attr.Value);

		// The "name" setting is actually the name of the style
		//  and not a new default
		if (attr_name == "name")
			name = attr_value;
		else
			style.m_SettingsDefaults[attr_name] = attr_value;
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
	XMBAttributeList attributes = Element.getAttributes();
	for (int i=0; i<attributes.Count; ++i)
	{
		XMBAttribute attr = attributes.item(i);
		std::string attr_name = pFile->getAttributeString(attr.Name);
		CStr attr_value (attr.Value);

		if (attr_value == CStr("null"))
			continue;

		if (attr_name == "name")
			name = attr_value;
		else
		if (attr_name == "width")
		{
			int i;
			if (!GUI<int>::ParseString(attr_value, i))
			{
				// TODO Gee: Report in log file
			}
			scrollbar.m_Width = i;
		}
		else
		if (attr_name == "minimum-bar-size")
		{
			int i;
			if (!GUI<int>::ParseString(attr_value, i))
			{
				// TODO Gee: Report in log file
			}
			scrollbar.m_MinimumBarSize = i;
		}
		else
		if (attr_name == "sprite-button-top")
			scrollbar.m_SpriteButtonTop = attr_value;
		else
		if (attr_name == "sprite-button-top-pressed")
			scrollbar.m_SpriteButtonTopPressed = attr_value;
		else
		if (attr_name == "sprite-button-top-disabled")
			scrollbar.m_SpriteButtonTopDisabled = attr_value;
		else
		if (attr_name == "sprite-button-top-over")
			scrollbar.m_SpriteButtonTopOver = attr_value;
		else
		if (attr_name == "sprite-button-bottom")
			scrollbar.m_SpriteButtonBottom = attr_value;
		else
		if (attr_name == "sprite-button-bottom-pressed")
			scrollbar.m_SpriteButtonBottomPressed = attr_value;
		else
		if (attr_name == "sprite-button-bottom-disabled")
			scrollbar.m_SpriteButtonBottomDisabled = attr_value;
		else
		if (attr_name == "sprite-button-bottom-over")
			scrollbar.m_SpriteButtonBottomOver = attr_value;
		else
		if (attr_name == "sprite-back-vertical")
			scrollbar.m_SpriteBackVertical = attr_value;
		else
		if (attr_name == "sprite-bar-vertical")
			scrollbar.m_SpriteBarVertical = attr_value;
		else
		if (attr_name == "sprite-bar-vertical-over")
			scrollbar.m_SpriteBarVerticalOver = attr_value;
		else
		if (attr_name == "sprite-bar-vertical-pressed")
			scrollbar.m_SpriteBarVerticalPressed = attr_value;

/*
	CStr m_SpriteButtonTop;
	CStr m_SpriteButtonTopPressed;
	CStr m_SpriteButtonTopDisabled;

	CStr m_SpriteButtonBottom;
	CStr m_SpriteButtonBottomPressed;
	CStr m_SpriteButtonBottomDisabled;

	CStr m_SpriteScrollBackHorizontal;
	CStr m_SpriteScrollBarHorizontal;


	CStr m_SpriteButtonLeft;
	CStr m_SpriteButtonLeftPressed;
	CStr m_SpriteButtonLeftDisabled;

	CStr m_SpriteButtonRight;
	CStr m_SpriteButtonRightPressed;
	CStr m_SpriteButtonRightDisabled;

	CStr m_SpriteScrollBackVertical;
	CStr m_SpriteScrollBarVertical;

*/

	}

	//
	//	Add to CGUI
	//

	m_ScrollBarStyles[name] = scrollbar;
}
