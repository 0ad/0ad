/*
CGUI
by Gustav Larsson
gee@pyro.nu

--Overview--

	This is the top class of the whole GUI, all objects
	and settings are stored within this class.

--More info--

	Check GUI.h

*/

#ifndef CGUI_H
#define CGUI_H

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------
#include "GUI.h"
#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>

///// janwas: yeah I don't know how the including etiquette is really
#include "../ps/Singleton.h"
#include "input.h"	// JW: grr, classes suck in this case :P

class XERCES_CPP_NAMESPACE::DOMElement;


//--------------------------------------------------------
//  Macros
//--------------------------------------------------------

//--------------------------------------------------------
//  Types
//--------------------------------------------------------

//--------------------------------------------------------
//  Error declarations
//--------------------------------------------------------

//--------------------------------------------------------
//  Declarations
//--------------------------------------------------------

class CGUI : public Singleton<CGUI>
{

	// Only CGUIObject's leaf functions uses CGUI
	//  freely.
	friend class CGUIObject;
	friend class CInternalCGUIAccessorBase;

private:
	// Private typedefs
	typedef CGUIObject *(*ConstructObjectFunction)();

	// don't want to pass this around with the ChooseMouseOverAndClosest broadcast -
	// we'd need to pack this and pNearest in a struct
	u16 m_MouseX, m_MouseY;

public:
	CGUI();
	~CGUI();

	// Initialize
	void Initialize(/*/*nemInput *pInput*/);
	
	// Process
	void Process();

	// Draw
	void Draw();
	
	// Shutdown
	void Destroy();

	bool HandleEvent(const SDL_Event& ev);

	// Load a GUI XML file
	void LoadXMLFile(const CStr &Filename);

	// Checks if object exists and return true or false accordingly
	bool ObjectExists(const CStr &Name) const;

	// Get pInput
///	CInput *GetInput() { return m_pInput; }

	// to add a type:
	//  AddObjecType("button", &CButton::ConstructObject);
	void AddObjectType(const CStr &str, ConstructObjectFunction pFunc) { m_ObjectTypes[str] = pFunc; }

private:
	void UpdateObjects();

	// Adds an object to the GUI's object database
	//  Private, you can only add objects through XML files.
	void AddObject(CGUIObject* pObject);

	// Report a XML parsing error
	void ReportParseError(const CStr &str, ...);

	// Construct an object
	CGUIObject *ConstructObject(const CStr &str);

	//
	// XML Reading Xerces C++ specific subroutines
	//

	/**
		Xerces_* functions tree

		==========================

		<objects> (ReadRootObjects)
		 |
		 +-<object> (ReadObject)
			|
			+-<action>
			|
			+-Optional Type Extensions (CGUIObject::ReadExtendedElement) TODO
			|
			+-«object» *recursive*


		<styles> (ReadRootStyles)
		 |
		 +-<style> (ReadStyle)


		<sprites> (ReadRootSprites)
		 |
		 +-<sprite> (ReadSprite)
			|
			+-<image> (ReadImage)


		<setup> (ReadRootSetup)
		 |
		 +-<tooltip> (ReadToolTip)
		 |
		 +-<scrollbar> (ReadScrollBar)
		 |
		 +-<icon> (ReadIcon)

		==========================
	*/

	// These does not throw!
	//  Because when reading in XML files, it won't be fatal
	//  if an error occurs, perhaps one particular object
	//  fails, but it'll still continue reading in the next
	// All Error are reported with ReportParseError

	// Read roots
	void Xerces_ReadRootObjects(XERCES_CPP_NAMESPACE::DOMElement *pElement);
	void Xerces_ReadRootSprites(XERCES_CPP_NAMESPACE::DOMElement *pElement);

	// Read subs
	void Xerces_ReadObject(XERCES_CPP_NAMESPACE::DOMElement *, CGUIObject *pParent);
	void Xerces_ReadSprite(XERCES_CPP_NAMESPACE::DOMElement *);
	void Xerces_ReadImage(XERCES_CPP_NAMESPACE::DOMElement *, CGUISprite &parent);

private:

	// Variables

	// Pointer to input module
///	CInput									*m_pInput;

	//
	//	Objects
	//

	// Base Object, all its children are considered parentless
	//  because this is no real object per se.
	CGUIObject*								m_BaseObject;

	// Just pointers for fast name access, each object
	//  is really constructed within its parent for easy
	//  recursive management.
	// Notice m_BaseObject won't belong here since it's
	//  not considered a real object.
	map_pObjects							m_pAllObjects;

	// Function pointers to functions that constructs
	//  CGUIObjects by name... For instance m_ObjectTypes["button"]
	//  is filled with a function that will "return new CButton();"
	std::map<CStr, ConstructObjectFunction>	m_ObjectTypes;

	// Used when reading in XML files
	int										m_Errors;

	//
	//	Sprites
	//
	
	std::map<CStr, CGUISprite>				m_Sprites;
};

#endif