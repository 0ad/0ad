/*
CGUI
by Gustav Larsson
gee@pyro.nu
*/

//#include "stdafx."
#include "GUI.h"
#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>

#include "XercesErrorHandler.h"
#include "Prometheus.h"
#include "input.h"

#include <string>
#include <assert.h>
#include <stdarg.h>

// namespaces used
XERCES_CPP_NAMESPACE_USE
using namespace std;

#ifdef _MSC_VER
#pragma comment(lib, "xerces-c_2.lib")
#endif


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
		m_MouseX = ev.motion.x, m_MouseY = ev.motion.y;

		// pNearest will after this point at the hovered object, possibly NULL
		GUI<SGUIMessage>::RecurseObject(GUIRR_HIDDEN | GUIRR_GHOST, m_BaseObject, 
										&IGUIObject::HandleMessage, 
										SGUIMessage(GUIM_MOUSE_MOTION));
	}

	char buf[30];
	sprintf(buf, "type = %d", ev.type);
	TEMPmessage = buf;

	if (ev.type == SDL_MOUSEBUTTONDOWN)
	{
		sprintf(buf, "button = %d", ev.button.button);
		TEMPmessage = buf;
	}

// JW: (pre|post)process omitted; what're they for? why would we need any special button_released handling?

	// Only one object can be hovered
	IGUIObject *pNearest = NULL;

	try
	{
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

		//if (ev.type == SDL_MOUSEBUTTONDOWN)
		{
			if (ev.type == SDL_MOUSEBUTTONDOWN)
			{
				switch (ev.button.button)
				{
				case SDL_BUTTON_LEFT:
					if (pNearest)
					{
						pNearest->HandleMessage(SGUIMessage(GUIM_MOUSE_PRESS_LEFT));

						// some temp
						CClientArea ca;
						bool hidden;

						/*GUI<CClientArea>::GetSetting(*this, CStr("backdrop"), CStr("size"), ca);
						GUI<bool>::GetSetting(*this, CStr("backdrop"), CStr("hidden"), hidden);
					
						//hidden = !hidden;
						ca.pixel.right += 3;
						ca.pixel.bottom += 3;

						GUI<CClientArea>::SetSetting(*this, CStr("backdrop"), CStr("size"), ca);
						GUI<bool>::SetSetting(*this, CStr("backdrop"), CStr("hidden"), hidden);
		*/			}
					break;

				case 3: // wheel down
					if (pNearest)
					{
						pNearest->HandleMessage(SGUIMessage(GUIM_MOUSE_WHEEL_DOWN));
					}
					break;

				case 4: // wheel up
					if (pNearest)
					{
						pNearest->HandleMessage(SGUIMessage(GUIM_MOUSE_WHEEL_UP));
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
	}
	catch (PS_RESULT e)
	{
		// TODO Gee: Handle
	}
// JW: what's the difference between mPress and mDown? what's the code below responsible for?
/*/* // Generally if just mouse is clicked
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
CGUI::CGUI()
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
IGUIObject *CGUI::ConstructObject(const CStr &str)
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
	AddObjectType("button", &CButton::ConstructObject);
	AddObjectType("text", &CText::ConstructObject);
}

void CGUI::Process()
{
/*/*

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
	glTranslatef(0.0f, g_yres, -1000.0f);
	glScalef(1.0f, -1.f, 1.0f);

	try
	{
		// Recurse IGUIObject::Draw() with restriction: hidden
		//  meaning all hidden objects won't call Draw (nor will it recurse its children)
		GUI<>::RecurseObject(GUIRR_HIDDEN, m_BaseObject, &IGUIObject::Draw);
	}
	catch (PS_RESULT e)
	{
		glPopMatrix();

		// TODO Gee: Report error.
		return;
	}
	glPopMatrix();
}

void CGUI::DrawSprite(const CStr &SpriteName, 
					  const float &Z, 
					  const CRect &Rect, 
					  const CRect &Clipping)
{
	// This is not an error, it's just a choice not to draw any sprite.
	if (SpriteName == CStr("null") || SpriteName == CStr())
		return;

	bool DoClipping = (Clipping != CRect(0,0,0,0));
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

			glColor3f(cit->m_BackColor.r , cit->m_BackColor.g, cit->m_BackColor.b);
			//glColor3f((float)real.right/1000.f, 0.5f, 0.5f);

			// Do this
			glBegin(GL_QUADS);
				glVertex2i(real.right,			real.bottom);
				glVertex2i(real.left,			real.bottom);
				glVertex2i(real.left,			real.top);
				glVertex2i(real.right,			real.top);
			glEnd();
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
			// TODO Gee: Handle
		}
		
		delete it->second;
		it->second = NULL;
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

bool CGUI::ObjectExists(const CStr &Name) const
{
	if (m_pAllObjects.count(Name))
		return true;
	else
		return false;
}

void CGUI::ReportParseError(const CStr &str, ...)
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
			LocalFileInputSource source( XMLString::transcode( Filename.c_str() ) );

			// parse
			parser->parse(source);

			// Check how many errors
			ParseFailed = parser->getErrorCount() != 0;
			if (ParseFailed)
			{
				// TODO Gee: Report for real!
	///				g_console.submit("echo Xerces XML Parsing Reports %d errors", parser->getErrorCount());
			}
			
			// Parse Failed?
			if (!ParseFailed)
			{
				DOMDocument *doc = parser->getDocument();
				DOMElement *node = doc->getDocumentElement();

				// Check root element's (node) name so we know what kind of
				//  data we'll be expecting
				string root_name = XMLString::transcode( node->getNodeName() );

				if (root_name == "objects")
				{
					Xerces_ReadRootObjects(node);

					// Re-cache all values so these gets cached too.
					//UpdateResolution();
				}
				else
				if (root_name == "sprites")
				{
					Xerces_ReadRootSprites(node);
				}
				else
				if (root_name == "styles")
				{
					Xerces_ReadRootStyles(node);
				}
				else
				if (root_name == "setup")
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

void CGUI::Xerces_ReadRootObjects(XERCES_CPP_NAMESPACE::DOMElement *pElement)
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

void CGUI::Xerces_ReadRootSprites(XERCES_CPP_NAMESPACE::DOMElement *pElement)
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

void CGUI::Xerces_ReadRootStyles(XERCES_CPP_NAMESPACE::DOMElement *pElement)
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

void CGUI::Xerces_ReadRootSetup(XERCES_CPP_NAMESPACE::DOMElement *pElement)
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

			CStr name = XMLString::transcode( element->getNodeName() );

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
	CStr type = XMLString::transcode( pElement->getAttribute( XMLString::transcode("type") ) );

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
	CStr argStyle = XMLString::transcode( pElement->getAttribute( XMLString::transcode("style") ) );

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
		CStr attr_name = XMLString::transcode( attr->getName() );
		CStr attr_value = XMLString::transcode( attr->getValue() );

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
			ReportParseError(CStr("Can't set \"") + attr_name + CStr("\" to \"") + attr_value + CStr("\""));

			// This is not a fatal error
		}
	}

	// Check if name isn't set, report error in that case
	if (!NameSet)
	{
		// TODO Gee: Generate internal name!
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
			string element_name = XMLString::transcode( child->getNodeName() );

			if (element_name == "object")
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
			CStr caption = XMLString::transcode( child->getNodeValue() );

			// Text is only okay if it's the first element i.e. <object>caption ... </object>
			if (i==0)
			{
				// Thank you CStr =)
				caption.Trim(PS_TRIM_BOTH);

				// Set the setting caption to this
				GUI<CStr>::SetSetting(object, "caption", caption);
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
			// If the object is absolute, we'll have to get the parent's Z buffered,
			//  and add to that!
			if (object->GetBaseSettings().m_Absolute)
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

void CGUI::Xerces_ReadSprite(XERCES_CPP_NAMESPACE::DOMElement *pElement)
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
	name = XMLString::transcode( pElement->getAttribute( XMLString::transcode("name") )  );

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

void CGUI::Xerces_ReadImage(XERCES_CPP_NAMESPACE::DOMElement *pElement, CGUISprite &parent)
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
		CStr attr_name = XMLString::transcode( attr->getName() );
		CStr attr_value(XMLString::transcode( attr->getValue() ));

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

void CGUI::Xerces_ReadStyle(XERCES_CPP_NAMESPACE::DOMElement *pElement)
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
		CStr attr_name = XMLString::transcode( attr->getName() );
		CStr attr_value = XMLString::transcode( attr->getValue() );

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

void CGUI::Xerces_ReadScrollBarStyle(XERCES_CPP_NAMESPACE::DOMElement *pElement)
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
		CStr attr_name = XMLString::transcode( attr->getName() );
		CStr attr_value = XMLString::transcode( attr->getValue() );

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
	}

	//
	//	Add to CGUI
	//

	m_ScrollBarStyles[name] = scrollbar;
}
