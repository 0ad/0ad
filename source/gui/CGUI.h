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
	 *
	 * @throws PS_RESULT from CGUIObject::Draw().
	 */
	void Draw();
	
	/**
	 * Clean up, call this to clean up all memory allocated
	 * within the GUI.
	 *
	 * @throws PS_RESULT from CGUIObject::Destroy().
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
	 *
	 * This function is atomic, meaning if it throws anything, it will
	 * have seen it through that nothing was ultimately changed.
	 *
	 * @throws PS_RESULT that is thrown from CGUIObject::AddToPointersMap().
	 */
	void UpdateObjects();

	/**
	 * Adds an object to the GUI's object database
	 * Private, since you can only add objects through 
	 * XML files. Why? Becasue it enables the GUI to
	 * be much more encapsulated and safe.
	 *
	 * @throws	Rethrows PS_RESULT from CGUIObject::SetGUI() and
	 *			CGUIObject::AddChild().
	 */
	void AddObject(CGUIObject* pObject);

	/**
	 * Report XML Reading Error, should be called from within the
	 * Xerces_* functions.
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
	/** @name XML Reading Xerces C++ specific subroutines
	 *
	 * These does not throw!
	 * Because when reading in XML files, it won't be fatal
	 * if an error occurs, perhaps one particular object
	 * fails, but it'll still continue reading in the next.
	 * All Error are reported with ReportParseError
	 */
	//--------------------------------------------------------

	/**
		Xerces_* functions tree
		<code>
		\<objects\> (ReadRootObjects)
		 |
		 +-\<object\> (ReadObject)
			|
			+-\<action\>
			|
			+-Optional Type Extensions (CGUIObject::ReadExtendedElement) TODO
			|
			+-«object» *recursive*


		\<styles\> (ReadRootStyles)
		 |
		 +-\<style\> (ReadStyle)


		\<sprites\> (ReadRootSprites)
		 |
		 +-\<sprite\> (ReadSprite)
			|
			+-\<image\> (ReadImage)


		\<setup>\ (ReadRootSetup)
		 |
		 +-\<tooltip>\ (ReadToolTip)
		 |
		 +-\<scrollbar>\ (ReadScrollBar)
		 |
		 +-\<icon>\ (ReadIcon)

		</code>
	*/
	//@{

	// Read Roots

	/**
	 * Reads in the root element "objects" (the DOMElement).
	 *
	 * @param pElement	The Xerces C++ Parser object that represents
	 *					the objects-tag.
	 */
	void Xerces_ReadRootObjects(XERCES_CPP_NAMESPACE::DOMElement *pElement);

	/**
	 * Reads in the root element "sprites" (the DOMElement).
	 *
	 * @param pElement	The Xerces C++ Parser object that represents
	 *					the sprites-tag.
	 */
	void Xerces_ReadRootSprites(XERCES_CPP_NAMESPACE::DOMElement *pElement);

	// Read Subs

	/**
	 * Notice! Recursive function!
	 *
	 * Read in an "object"-tag (the DOMElement) and stores it
	 * as a child in the pParent.
	 *
	 * It will also check the object's children and call this function
	 * on them too. Also it will call all other functions that reads
	 * in other stuff that can be found within an object. Such as
	 * "action"-tag will call Xerces_ReadAction (TODO, real funcion?).
	 *
	 * Reads in the root element "sprites" (the DOMElement).
	 *
	 * @param pElement	The Xerces C++ Parser object that represents
	 *					the object-tag.
	 * @param pParent	Parent to add this object as child in.
	 */
	void Xerces_ReadObject(XERCES_CPP_NAMESPACE::DOMElement *, CGUIObject *pParent);

	/**
	 * Reads in the element "sprite" (the DOMElement) and store the
	 * result in a new CGUISprite.
	 *
	 * @param pElement	The Xerces C++ Parser object that represents
	 *					the sprite-tag.
	 */
	void Xerces_ReadSprite(XERCES_CPP_NAMESPACE::DOMElement *);

	/**
	 * Reads in the element "image" (the DOMElement) and store the
	 * result within the CGUISprite.
	 *
	 * @param pElement	The Xerces C++ Parser object that represents
	 *					the image-tag.
	 */
	void Xerces_ReadImage(XERCES_CPP_NAMESPACE::DOMElement *, CGUISprite &parent);

	//@}

private:

	// Variables

	//--------------------------------------------------------
	/** @name Miscellaneous */
	//--------------------------------------------------------
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
	/** @name Objects */
	//--------------------------------------------------------
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
	/** @name Sprites */
	//--------------------------------------------------------
	//@{

	std::map<CStr, CGUISprite>				m_Sprites;

	//@}
};

#endif