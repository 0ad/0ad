/*
CGUI
by Gustav Larsson
gee@pyro.nu
*/

#include "precompiled.h"
#undef new // if it was redefined for leak detection, since xerces doesn't like it

#include "GUI.h"

// Types - when including them into the engine.
#include "CButton.h"
#include "CText.h"
#include "CCheckBox.h"
#include "CRadioButton.h"

#include "XML.h"
//#include <xercesc/dom/DOM.hpp>
//#include <xercesc/parsers/XercesDOMParser.hpp>
//#include <xercesc/framework/LocalFileInputSource.hpp>
//#include <xercesc/util/XMLString.hpp>
//#include <xercesc/util/PlatformUtils.hpp>

#include "XercesErrorHandler.h"
#include "Prometheus.h"
#include "input.h"
#include "OverlayText.h"
// TODO Gee: Whatever include CRect/CPos/CSize
#include "Overlay.h"

#include <string>
#include <assert.h>
#include <stdarg.h>

// namespaces used
XERCES_CPP_NAMESPACE_USE
using namespace std;


extern int g_xres, g_yres;

// TODO Gee: how to draw overlays?
void render(COverlayText* overlaytext)
{

}


//-------------------------------------------------------------------
//	called from main loop when (input) events are received.
//	event is passed to other handlers if false is returned.
//	trampoline: we don't want to make the implementation (in CGUI) static
//-------------------------------------------------------------------
bool gui_handler(const SDL_Event& ev)
{
	return g_GUI.HandleEvent(ev);
}

bool CGUI::HandleEvent(const SDL_Event& ev)
{
	if(ev.type == SDL_MOUSEMOTION)
	{
		m_MousePos = CPos(ev.motion.x, ev.motion.y);

		// pNearest will after this point at the hovered object, possibly NULL
		GUI<SGUIMessage>::RecurseObject(GUIRR_HIDDEN | GUIRR_GHOST, m_BaseObject, 
										&IGUIObject::HandleMessage, 
										SGUIMessage(GUIM_MOUSE_MOTION));
	}

	// TODO Gee: temp-stuff
//	char buf[30];
//	sprintf(buf, "type = %d", ev.type);
	//TEMPmessage = buf;

	if (ev.type == SDL_MOUSEBUTTONDOWN)
	{
	//	sprintf(buf, "button = %d", ev.button.button);
		//TEMPmessage = buf;
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
		
		// Now we'll call UpdateMouseOver on *all* objects,
		//  we'll input the one hovered, and they will each
		//  update their own data and send messages accordingly
		GUI<IGUIObject*>::RecurseObject(GUIRR_HIDDEN | GUIRR_GHOST, m_BaseObject, 
										&IGUIObject::UpdateMouseOver, 
										pNearest);

		if (ev.type == SDL_MOUSEBUTTONDOWN)
		{
			switch (ev.button.button)
			{
			case SDL_BUTTON_LEFT:
				if (pNearest)
				{
					pNearest->HandleMessage(SGUIMessage(GUIM_MOUSE_PRESS_LEFT));
				}

			{
				// some temp
/*				CClientArea ca;
				bool hidden;

				GUI<CClientArea>::GetSetting(*this, CStr("backdrop43"), CStr("size"), ca);
			
				//hidden = !hidden;
				ca.pixel.right -= 3;

				GUI<CClientArea>::SetSetting(*this, CStr("backdrop43"), CStr("size"), ca);
*/			}
			break;

			case SDL_BUTTON_WHEELDOWN: // wheel down
				if (pNearest)
				{
					pNearest->HandleMessage(SGUIMessage(GUIM_MOUSE_WHEEL_DOWN));
				}
				break;

			case SDL_BUTTON_WHEELUP: // wheel up
				if (pNearest)
				{
					pNearest->HandleMessage(SGUIMessage(GUIM_MOUSE_WHEEL_UP));
				}
				break;

			// TODO Gee: Just temp
			case SDL_BUTTON_RIGHT:
			{
				CClientArea ca;
				GUI<CClientArea>::GetSetting(*this, CStr("backdrop43"), CStr("size"), ca);
			
				//hidden = !hidden;
				ca.pixel.right -= 3;

				GUI<CClientArea>::SetSetting(*this, CStr("backdrop43"), CStr("size"), ca);
			}
			break;

			default:
				break;
			}
			
		}
		else 
		if (ev.type == SDL_MOUSEBUTTONUP)
		{
			if (ev.button.button == SDL_BUTTON_LEFT)
			{
				if (pNearest)
					pNearest->HandleMessage(SGUIMessage(GUIM_MOUSE_RELEASE_LEFT));
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

	return false;
}

//-------------------------------------------------------------------
//  Constructor / Destructor
//-------------------------------------------------------------------
CGUI::CGUI() : m_InternalNameNumber(0)
{
	m_BaseObject = new CGUIDummyObject;
	m_BaseObject->SetGUI(this);

	// This will make this invisible, not add
	//m_BaseObject->SetName(BASE_OBJECT_NAME);
}

CGUI::~CGUI()
{
	if (m_BaseObject)
		delete m_BaseObject;
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

	const char * buf = SpriteName;

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
			CRect real = cit->m_Size.GetClientArea(Rect);

			glPushMatrix();
			glTranslatef(0.f, 0.f, cit->m_DeltaZ);

			glColor3f(cit->m_BackColor.r , cit->m_BackColor.g, cit->m_BackColor.b);
			//glColor3f((float)real.right/1000.f, 0.5f, 0.5f);

			// Do this
			glBegin(GL_QUADS);
				glVertex2i(real.right,			real.bottom);
				glVertex2i(real.left,			real.bottom);
				glVertex2i(real.left,			real.top);
				glVertex2i(real.right,			real.top);
			glEnd();

			glPopMatrix();
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
		if ((WordWrapping && (x > Width-BufferZone || Feedback.m_NewLine)) || i == string.m_Words.size()-2)
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
						
						// The union is not ø
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
					it->m_Pos = CPos(x + x_pointer, y + line_height - it->m_Size.cy);

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

				if (j == string.m_Words.size()-2)
					done = true;
			}
		
			// Reset X, and append Y.
			x = 0;
			y += line_height;

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
	for (vector<SGUIText::STextCall>::const_iterator it=Text.m_TextCalls.begin(); 
		 it!=Text.m_TextCalls.end(); 
		 ++it)
	{
		if (it->m_pSpriteCall)
			continue;

		COverlayText txt((float)pos.x+it->m_Pos.x, (float)pos.y+it->m_Pos.y, 
						 (int)z, it->m_Font, it->m_String, 
						 (it->m_UseCustomColor?it->m_Color:DefaultColor));
		render(&txt);
	}

	for (vector<SGUIText::SSpriteCall>::const_iterator it=Text.m_SpriteCalls.begin(); 
		 it!=Text.m_SpriteCalls.end(); 
		 ++it)
	{
		DrawSprite(it->m_TextureName, z, it->m_Area + pos);
	}
}

void CGUI::ReportParseError(const CStr& str, ...)
{
	// Print header
	if (m_Errors==0)
	{
///		g_nemLog("*** GUI Tree Creation Errors");
	}



	// Important, set ParseError to true
	++m_Errors;
/*	TODO Gee: (MEGA)
	char buffer[512];
	va_list args;

	// get arguments
	va_start(args, str);
		vsprintf(buffer, str.c_str(), args);
	va_end(args);
*/	
///	g_nemLog(" %s", buffer);
}

/**
 * @callgraph
 */
void CGUI::LoadXMLFile(const string &Filename)
{
	// Reset parse error
	//  we can later check if this has increased
	m_Errors = 0;

	// Initialize XML library
	XMLPlatformUtils::Initialize();
	{
		// Create parser instance
		XercesDOMParser *parser = new XercesDOMParser();

 		bool ParseFailed = false;

		if (parser)
		{
			// Setup parser
			parser->setValidationScheme(XercesDOMParser::Val_Auto);
			parser->setDoNamespaces(false);
			parser->setDoSchema(false);
			parser->setCreateEntityReferenceNodes(false);

			// Set cosutomized error handler
			CXercesErrorHandler *errorHandler = new CXercesErrorHandler();
			parser->setErrorHandler(errorHandler);
			
	///		g_nemLog("*** Xerces XML Parsing Errors");

			// Get main node
			XMLCh *fname=XMLString::transcode(Filename.c_str());
			LocalFileInputSource source(fname);
			XMLString::release(&fname);

			// parse
			parser->parse(source);

			// Check how many errors
			ParseFailed = parser->getErrorCount() != 0;
			
			// Parse Failed?
			if (!ParseFailed)
			{
				DOMDocument *doc = parser->getDocument();
				DOMElement *node = doc->getDocumentElement();

				// Check root element's (node) name so we know what kind of
				//  data we'll be expecting
				CStr root_name = XMLTranscode( node->getNodeName() );

				if (root_name == CStr("objects"))
				{
					Xerces_ReadRootObjects(node);

					// Re-cache all values so these gets cached too.
					//UpdateResolution();
				}
				else
				if (root_name == CStr("sprites"))
				{
					Xerces_ReadRootSprites(node);
				}
				else
				if (root_name == CStr("styles"))
				{
					Xerces_ReadRootStyles(node);
				}
				else
				if (root_name == CStr("setup"))
				{
					Xerces_ReadRootSetup(node);
				}
				else
				{
					// TODO Gee: Output in log
				}

			}
		}

		// Now report if any other errors occured
		if (m_Errors > 0)
		{
	///		g_console.submit("echo GUI Tree Creation Reports %d errors", m_Errors);
		}

		delete parser;
	}
	XMLPlatformUtils::Terminate();
}

//===================================================================
//	XML Reading Xerces Specific Sub-Routines
//===================================================================

void CGUI::Xerces_ReadRootObjects(DOMElement *pElement)
{
	// Iterate main children
	//  they should all be <object> elements
	DOMNodeList *children = pElement->getChildNodes();
	for (u16 i=0; i<children->getLength(); ++i)
	{
		DOMNode *child = children->item(i);

		if (child->getNodeType() == DOMNode::ELEMENT_NODE)
		{
			// Read in this whole object into the GUI
			DOMElement *element = (DOMElement*)child;
			Xerces_ReadObject(element, m_BaseObject);
		}
	}
}

void CGUI::Xerces_ReadRootSprites(DOMElement *pElement)
{
	// Iterate main children
	//  they should all be <sprite> elements
	DOMNodeList *children = pElement->getChildNodes();
	for (u16 i=0; i<children->getLength(); ++i)
	{
		DOMNode *child = children->item(i);

		if (child->getNodeType() == DOMNode::ELEMENT_NODE)
		{
			// Read in this whole object into the GUI
			DOMElement *element = (DOMElement*)child;
			Xerces_ReadSprite(element);
		}
	}
}

void CGUI::Xerces_ReadRootStyles(DOMElement *pElement)
{
	// Iterate main children
	//  they should all be <styles> elements
	DOMNodeList *children = pElement->getChildNodes();
	for (u16 i=0; i<children->getLength(); ++i)
	{
		DOMNode *child = children->item(i);

		if (child->getNodeType() == DOMNode::ELEMENT_NODE)
		{
			// Read in this whole object into the GUI
			DOMElement *element = (DOMElement*)child;
			Xerces_ReadStyle(element);
		}
	}
}

void CGUI::Xerces_ReadRootSetup(DOMElement *pElement)
{
	// Iterate main children
	//  they should all be <icon>, <scrollbar> or <tooltip>.
	DOMNodeList *children = pElement->getChildNodes();
	for (u16 i=0; i<children->getLength(); ++i)
	{
		DOMNode *child = children->item(i);

		if (child->getNodeType() == DOMNode::ELEMENT_NODE)
		{
			// Read in this whole object into the GUI
			DOMElement *element = (DOMElement*)child;

			CStr name = XMLTranscode( element->getNodeName() );

			if (name == CStr("scrollbar"))
			{
				Xerces_ReadScrollBarStyle(element);
			}
			// No need for else, we're using DTD.
		}
	}
}

void CGUI::Xerces_ReadObject(DOMElement *pElement, IGUIObject *pParent)
{
	assert(pParent && pElement);
	u16 i;

	// Our object we are going to create
	IGUIObject *object = NULL;

	// Well first of all we need to determine the type
	CStr type = XMLTranscode( pElement->getAttribute( L"type" ) );

	// Construct object from specified type
	//  henceforth, we need to do a rollback before aborting.
	//  i.e. releasing this object
	object = ConstructObject(type);

	if (!object)
	{
		// Report error that object was unsuccessfully loaded
		ReportParseError(CStr("Unrecognized type: ") + type);

		delete object;
		return;
	}

	//
	//	Read Style and set defaults
	//
	//	If the setting "style" is set, try loading that setting.
	//
	//	Always load default (if it's available) first!
	//
	CStr argStyle = XMLTranscode( pElement->getAttribute( L"style" ) );

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

	// Now we can iterate all attributes and store
	DOMNamedNodeMap *attributes = pElement->getAttributes();
	for (i=0; i<attributes->getLength(); ++i)
	{
		DOMAttr *attr = (DOMAttr*)attributes->item(i);
		CStr attr_name = XMLTranscode( attr->getName() );
		CStr attr_value = XMLTranscode( attr->getValue() );

		// If value is "null", then it is equivalent as never being entered
		if (attr_value == CStr("null"))
			continue;

		// Ignore "type" and "style", we've already checked it
		if (attr_name == CStr("type") || attr_name == CStr("style") )
			continue;

		// Also the name needs some special attention
		if (attr_name == CStr("name"))
		{
			object->SetName(attr_value);
			NameSet = true;
			continue;
		}

		if (attr_name == CStr("z"))
			ManuallySetZ = true;

		// Try setting the value
		try
		{
			object->SetSetting(attr_name, attr_value);
		}
		catch (PS_RESULT e)
		{
			UNUSED(e);
			ReportParseError(CStr("Can't set \"") + attr_name + CStr("\" to \"") + attr_value + CStr("\""));

			// This is not a fatal error
		}
	}

	// Check if name isn't set, generate an internal name in that case.
	if (!NameSet)
	{
		object->SetName(CStr("__internal(") + CStr(m_InternalNameNumber) + CStr(")"));
		++m_InternalNameNumber;
	}

	//
	//	Read Children
	//

	// Iterate children
	DOMNodeList *children = pElement->getChildNodes();

	for (i=0; i<children->getLength(); ++i)
	{
		// Get node
		DOMNode *child = children->item(i);

		// Check type (it's probably text or element)
		
		// A child element
		if (child->getNodeType() == DOMNode::ELEMENT_NODE)
		{
			// Check what name the elements got
			CStr element_name = XMLTranscode( child->getNodeName() );

			if (element_name == CStr("object"))
			{
				// First get element and not node
				DOMElement *element = (DOMElement*)child;

				// TODO Gee: REPORT ERROR

				// Call this function on the child
				Xerces_ReadObject(element, object);
			}
		}
		else 
		if (child->getNodeType() == DOMNode::TEXT_NODE)
		{
			CStr caption = XMLTranscode( child->getNodeValue() );

			// Text is only okay if it's the first element i.e. <object>caption ... </object>
			if (i==0)
			{
				// Thank you CStr =)
				caption.Trim(PS_TRIM_BOTH);

				try
				{
					// Set the setting caption to this
					object->SetSetting("caption", caption);
				}
				catch (...)
				{
					// There is no harm if the object didn't have a "caption"
				}
			}
			// else 
			// TODO Gee: give warning
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

void CGUI::Xerces_ReadSprite(DOMElement *pElement)
{
	assert(pElement);

	// Sprite object we're adding
	CGUISprite sprite;
	
	// and what will be its reference name
	CStr name;

	//
	//	Read Attributes
	//

	// Get name, we know it exists because of DTD requirements
	name = XMLTranscode( pElement->getAttribute( L"name" )  );

	//
	//	Read Children (the images)
	//

	// Iterate children
	DOMNodeList *children = pElement->getChildNodes();

	for (u16 i=0; i<children->getLength(); ++i)
	{
		// Get node
		DOMNode *child = children->item(i);

		// Check type (it's probably text or element)
		
		// A child element
		if (child->getNodeType() == DOMNode::ELEMENT_NODE)
		{
			// All Elements will be of type "image" by DTD law

			// First get element and not node
			DOMElement *element = (DOMElement*)child;

			// Call this function on the child
			Xerces_ReadImage(element, sprite);
		}
	}

	//
	//	Add Sprite
	//

	m_Sprites[name] = sprite;
}

void CGUI::Xerces_ReadImage(DOMElement *pElement, CGUISprite &parent)
{
	assert(pElement);

	// Image object we're adding
	SGUIImage image;
	
	// TODO Gee: Setup defaults here (or maybe they are in the SGUIImage ctor)

	//
	//	Read Attributes
	//

	// Now we can iterate all attributes and store
	DOMNamedNodeMap *attributes = pElement->getAttributes();
	for (u16 i=0; i<attributes->getLength(); ++i)
	{
		DOMAttr *attr = (DOMAttr*)attributes->item(i);
		CStr attr_name = XMLTranscode( attr->getName() );
		CStr attr_value(XMLTranscode( attr->getValue() ));

		// This is the only attribute we want
		if (attr_name == CStr("texture"))
		{
			image.m_Texture = attr_value;
		}
		else
		if (attr_name == CStr("size"))
		{
			CClientArea ca;
			if (!GUI<CClientArea>::ParseString(attr_value, ca))
			{
				// TODO Gee: Error
			}
			else image.m_Size = ca;
		}
		else
		if (attr_name == CStr("z-level"))
		{
			int z_level;
			if (!GUI<int>::ParseString(attr_value, z_level))
			{
				// TODO Gee: Error
			}
			else image.m_DeltaZ = (float)z_level/100.f;
		}
		else
		if (attr_name == CStr("backcolor"))
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

void CGUI::Xerces_ReadStyle(DOMElement *pElement)
{
	assert(pElement);

	// style object we're adding
	SGUIStyle style;
	CStr name;
	
	//
	//	Read Attributes
	//

	// Now we can iterate all attributes and store
	DOMNamedNodeMap *attributes = pElement->getAttributes();
	for (u16 i=0; i<attributes->getLength(); ++i)
	{
		DOMAttr *attr = (DOMAttr*)attributes->item(i);
		CStr attr_name = XMLTranscode( attr->getName() );
		CStr attr_value = XMLTranscode( attr->getValue() );

		// The "name" setting is actually the name of the style
		//  and not a new default
		if (attr_name == CStr("name"))
			name = attr_value;
		else
			style.m_SettingsDefaults[attr_name] = attr_value;
	}

	//
	//	Add to CGUI
	//

	m_Styles[name] = style;
}

void CGUI::Xerces_ReadScrollBarStyle(DOMElement *pElement)
{
	assert(pElement);

	// style object we're adding
	SGUIScrollBarStyle scrollbar;
	CStr name;
	
	//
	//	Read Attributes
	//

	// Now we can iterate all attributes and store
	DOMNamedNodeMap *attributes = pElement->getAttributes();
	for (u16 i=0; i<attributes->getLength(); ++i)
	{
		DOMAttr *attr = (DOMAttr*)attributes->item(i);
		CStr attr_name = XMLTranscode( attr->getName() );
		CStr attr_value = XMLTranscode( attr->getValue() );

		if (attr_value == CStr("null"))
			continue;

		if (attr_name == CStr("name"))
			name = attr_value;
		else
		if (attr_name == CStr("width"))
		{
			int i;
			if (!GUI<int>::ParseString(attr_value, i))
			{
				// TODO Gee: Report in log file
			}
			scrollbar.m_Width = i;
		}
		else
		if (attr_name == CStr("minimum-bar-size"))
		{
			int i;
			if (!GUI<int>::ParseString(attr_value, i))
			{
				// TODO Gee: Report in log file
			}
			scrollbar.m_MinimumBarSize = i;
		}
		else
		if (attr_name == CStr("sprite-button-top"))
			scrollbar.m_SpriteButtonTop = attr_value;
		else
		if (attr_name == CStr("sprite-button-top-pressed"))
			scrollbar.m_SpriteButtonTopPressed = attr_value;
		else
		if (attr_name == CStr("sprite-button-top-disabled"))
			scrollbar.m_SpriteButtonTopDisabled = attr_value;
		else
		if (attr_name == CStr("sprite-button-top-over"))
			scrollbar.m_SpriteButtonTopOver = attr_value;
		else
		if (attr_name == CStr("sprite-button-bottom"))
			scrollbar.m_SpriteButtonBottom = attr_value;
		else
		if (attr_name == CStr("sprite-button-bottom-pressed"))
			scrollbar.m_SpriteButtonBottomPressed = attr_value;
		else
		if (attr_name == CStr("sprite-button-bottom-disabled"))
			scrollbar.m_SpriteButtonBottomDisabled = attr_value;
		else
		if (attr_name == CStr("sprite-button-bottom-over"))
			scrollbar.m_SpriteButtonBottomOver = attr_value;
		else
		if (attr_name == CStr("sprite-back-vertical"))
			scrollbar.m_SpriteBackVertical = attr_value;
		else
		if (attr_name == CStr("sprite-bar-vertical"))
			scrollbar.m_SpriteBarVertical = attr_value;
		else
		if (attr_name == CStr("sprite-bar-vertical-over"))
			scrollbar.m_SpriteBarVerticalOver = attr_value;
		else
		if (attr_name == CStr("sprite-bar-vertical-pressed"))
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
