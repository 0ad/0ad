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

/**
 * @author Gustav Larsson
 *
 * The main object that includes the whole GUI. Is singleton
 * and accessed by g_GUI.
 */
class CGUI : public Singleton<CGUI>
{
	// Only CGUIObject's leaf functions uses CGUI
	//  freely.
	friend class CGUIObject;
	friend class CInternalCGUIAccessorBase;

private:
	// Private typedefs
	typedef CGUIObject *(*ConstructObjectFunction)();

public:
	CGUI();
	~CGUI();

	/**
	 * Initializes the GUI, needs to be called before the GUI is used
	 */
	void Initialize();
	
	/**
	 * @deprecated Will be removed
	 */
	void Process();

	/**
	 * Displays the whole GUI
	 */
	void Draw();
	
	/**
	 * Clean up, call this to clean up all memory allocated
	 * within the GUI.
	 */
	void Destroy();

	/**
	 * The replacement of Process(), handles an SDL_Event
	 *
	 * @param ev SDL Event, like mouse/keyboard input
	 */
	bool HandleEvent(const SDL_Event& ev);

	/**
	 * Load a GUI XML file into the GUI.
	 *
	 * @param Filename Name of file
	 */
	void LoadXMLFile(const std::string &Filename);

	/**
	 * Checks if object exists and return true or false accordingly
	 *
	 * @param Name String name of object
	 * @return true if object exists
	 */
	bool ObjectExists(const CStr &Name) const;

	/**
	 * The GUI needs to have all object types inputted and
	 * their constructors. Also it needs to associate a type
	 * by a string name of the type.
	 * 
	 * To add a type:<br>
	 * AddObjectType("button", &CButton::ConstructObject);
	 *
	 * @param str Reference name of object type
	 * @param pFunc Pointer of function ConstuctObject() in the object
	 *
	 * @see CGUI#ConstructObject()
	 */
	void AddObjectType(const CStr &str, ConstructObjectFunction pFunc) { m_ObjectTypes[str] = pFunc; }

private:
	/**
	 * Updates the object pointers, needs to be called each
	 * time an object has been added or removed.
	 */
	void UpdateObjects();

	/**
	 * Adds an object to the GUI's object database
	 * Private, since you can only add objects through 
	 * XML files. Why? Becasue it enables the GUI to
	 * be much more encapsulated and safe.
	 */
	void AddObject(CGUIObject* pObject);

	/**
	 * Report an XML parsing error
	 *
	 * @param str Error message
	 */
	void ReportParseError(const CStr &str, ...);

	/**
	 * You input the name of the object type, and let's
	 * say you input "button", then it will construct a
	 * CGUIObjet* as a CButton.
	 *
	 * @param str Name of object type
	 * @return Newly constructed CGUIObject (but constructed as a subclass)
	 */
	CGUIObject *ConstructObject(const CStr &str);

	//--------------------------------------------------------
	// XML Reading Xerces C++ specific subroutines
	//--------------------------------------------------------
	/** 
	 * @name Xerces_* Read Function 
	 *
	 * These does not throw!
	 * Because when reading in XML files, it won't be fatal
	 * if an error occurs, perhaps one particular object
	 * fails, but it'll still continue reading in the next.
	 * All Error are reported with ReportParseError
	 */
	//@{

	/*
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

	// Read roots
	void Xerces_ReadRootObjects(XERCES_CPP_NAMESPACE::DOMElement *pElement);
	void Xerces_ReadRootSprites(XERCES_CPP_NAMESPACE::DOMElement *pElement);

	// Read subs
	void Xerces_ReadObject(XERCES_CPP_NAMESPACE::DOMElement *, CGUIObject *pParent);
	void Xerces_ReadSprite(XERCES_CPP_NAMESPACE::DOMElement *);
	void Xerces_ReadImage(XERCES_CPP_NAMESPACE::DOMElement *, CGUISprite &parent);

	//@}

private:

	// Variables

	//--------------------------------------------------------
	//	Misc
	//--------------------------------------------------------
	/** @name Miscellaneous */
	//@{

	/**
	 * don't want to pass this around with the 
	 * ChooseMouseOverAndClosest broadcast -
	 * we'd need to pack this and pNearest in a struct
	 */
	u16 m_MouseX, m_MouseY;

	/// Used when reading in XML files
	int										m_Errors;

	//@}
	//--------------------------------------------------------
	//	Objects
	//--------------------------------------------------------
	/** @name Objects */
	//@{

	/**
	 * Base Object, all its children are considered parentless
	 * because this is no real object per se.
	 */
	CGUIObject*								m_BaseObject;

	/** 
	 * Just pointers for fast name access, each object
	 * is really constructed within its parent for easy
	 * recursive management.
	 * Notice m_BaseObject won't belong here since it's
	 * not considered a real object.
	 */
	map_pObjects							m_pAllObjects;

	/**
	 * Function pointers to functions that constructs
	 * CGUIObjects by name... For instance m_ObjectTypes["button"]
	 * is filled with a function that will "return new CButton();"
	 */
	std::map<CStr, ConstructObjectFunction>	m_ObjectTypes;

	//@}
	//--------------------------------------------------------
	//	Sprites
	//--------------------------------------------------------
	/** @name Sprites */
	//@{

	std::map<CStr, CGUISprite>				m_Sprites;

	//@}
};

#endif