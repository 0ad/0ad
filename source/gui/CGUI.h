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

#include "Singleton.h"
#include "input.h"	// JW: grr, classes suck in this case :P

class XERCES_CPP_NAMESPACE::DOMElement;

extern bool gui_handler(const SDL_Event& ev);

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
 * Contains a list of values for new defaults to objects.
 */
struct SGUIStyle
{
	// A list of defualts for 
	std::map<CStr, CStr> m_SettingsDefaults;
};

/**
 * @author Gustav Larsson
 *
 * The main object that includes the whole GUI. Is singleton
 * and accessed by g_GUI.
 *
 * No interfacial functions throws.
 */
class CGUI : public Singleton<CGUI>
{
	friend class IGUIObject;
	friend class IGUIScrollBarOwner;
	friend class CInternalCGUIAccessorBase;

private:
	// Private typedefs
	typedef IGUIObject *(*ConstructObjectFunction)();

public:
	CGUI();
	~CGUI();

	// TODO Gee: (MEGA) Extremely temporary.
	std::string TEMPmessage;

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
	 * Draw GUI Sprite, cooperates with CRenderer.
	 *
	 * @param SpriteName By name! The GUI will fetch the real object itself.
	 * @param Z Drawing order, depth value
	 * @param Rect Position and Size
	 * @param Clipping The sprite shouldn't be drawn outside this rectangle
	 */
	void DrawSprite(const CStr &SpriteName, const float &Z, 
					const CRect &Rect, const CRect &Clipping=CRect());

	/**
	 * Draw a SGUIText object
	 *
	 * @param Text Text object.
	 * @param DefaultColor Color used if no tag applied.
	 * @param pos position
	 * @param z z value.
	 */
	void DrawText(const SGUIText &Text, const CColor &DefaultColor, 
				  const CPos &pos, const float &z);

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
	 * <b>VERY IMPORTANT!</b> All \<styles\>-files must be read before
	 * everything else!
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
	 * To add a type:
	 * @code
	 * AddObjectType("button", &CButton::ConstructObject);
	 * @endcode
	 *
	 * @param str Reference name of object type
	 * @param pFunc Pointer of function ConstuctObject() in the object
	 *
	 * @see CGUI#ConstructObject()
	 */
	void AddObjectType(const CStr &str, ConstructObjectFunction pFunc) { m_ObjectTypes[str] = pFunc; }

	/**
	 * Update Resolution, should be called every time the resolution
	 * of the opengl screen has been changed, this is becuase it needs
	 * to re-cache all its actual sizes
	 *
	 * Needs no input since screen resolution is global.
	 *
	 * @see IGUIObject#UpdateCachedSize()
	 */
	void UpdateResolution();

	/**
	 * Generate a SGUIText object from the inputted string.
	 * The function will break down the string and its
	 * tags to calculate exactly which rendering queries
	 * will be sent to the Renderer.
	 *
	 * Done through the CGUI since it can communicate with 
	 *
	 * @param Text Text to generate SGUIText object from
	 * @param Color Default color
	 * @param Font Default font, notice both Default color and defult font
	 *		  can be changed by tags.
	 * @param Width Width, 0 if no word-wrapping.
	 * @param BufferZone space between text and edge, and space between text and images.
	 */
	SGUIText GenerateText(const CGUIString &Text, /*const CColor &Color, */
						  const CStr &Font, const int &Width, const int &BufferZone);

private:
	/**
	 * Updates the object pointers, needs to be called each
	 * time an object has been added or removed.
	 *
	 * This function is atomic, meaning if it throws anything, it will
	 * have seen it through that nothing was ultimately changed.
	 *
	 * @throws PS_RESULT that is thrown from IGUIObject::AddToPointersMap().
	 */
	void UpdateObjects();

	/**
	 * Adds an object to the GUI's object database
	 * Private, since you can only add objects through 
	 * XML files. Why? Becasue it enables the GUI to
	 * be much more encapsulated and safe.
	 *
	 * @throws	Rethrows PS_RESULT from IGUIObject::SetGUI() and
	 *			IGUIObject::AddChild().
	 */
	void AddObject(IGUIObject* pObject);

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
	 * @return Newly constructed IGUIObject (but constructed as a subclass)
	 */
	IGUIObject *ConstructObject(const CStr &str);

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
			+-Optional Type Extensions (IGUIObject::ReadExtendedElement) TODO
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
	 * Reads in the root element \<objects\> (the DOMElement).
	 *
	 * @param pElement	The Xerces C++ Parser object that represents
	 *					the objects-tag.
	 *
	 * @see LoadXMLFile()
	 */
	void Xerces_ReadRootObjects(XERCES_CPP_NAMESPACE::DOMElement *pElement);

	/**
	 * Reads in the root element \<sprites\> (the DOMElement).
	 *
	 * @param pElement	The Xerces C++ Parser object that represents
	 *					the sprites-tag.
	 *
	 * @see LoadXMLFile()
	 */
	void Xerces_ReadRootSprites(XERCES_CPP_NAMESPACE::DOMElement *pElement);

	/**
	 * Reads in the root element \<styles\> (the DOMElement).
	 *
	 * @param pElement	The Xerces C++ Parser object that represents
	 *					the sprites-tag.
	 *
	 * @see LoadXMLFile()
	 */
	void Xerces_ReadRootStyles(XERCES_CPP_NAMESPACE::DOMElement *pElement);

	/**
	 * Reads in the root element \<setup\> (the DOMElement).
	 *
	 * @param pElement	The Xerces C++ Parser object that represents
	 *					the setup-tag.
	 *
	 * @see LoadXMLFile()
	 */
	void Xerces_ReadRootSetup(XERCES_CPP_NAMESPACE::DOMElement *pElement);

	// Read Subs

	/**
	 * Notice! Recursive function!
	 *
	 * Read in an \<object\> (the DOMElement) and stores it
	 * as a child in the pParent.
	 *
	 * It will also check the object's children and call this function
	 * on them too. Also it will call all other functions that reads
	 * in other stuff that can be found within an object. Such as a
	 * \<action\> will call Xerces_ReadAction (TODO, real funcion?).
	 *
	 * Reads in the root element \<sprites\> (the DOMElement).
	 *
	 * @param pElement	The Xerces C++ Parser object that represents
	 *					the object-tag.
	 * @param pParent	Parent to add this object as child in.
	 *
	 * @see LoadXMLFile()
	 */
	void Xerces_ReadObject(XERCES_CPP_NAMESPACE::DOMElement *, IGUIObject *pParent);

	/**
	 * Reads in the element \<sprite\> (the DOMElement) and stores the
	 * result in a new CGUISprite.
	 *
	 * @param pElement	The Xerces C++ Parser object that represents
	 *					the sprite-tag.
	 *
	 * @see LoadXMLFile()
	 */
	void Xerces_ReadSprite(XERCES_CPP_NAMESPACE::DOMElement *pElement);

	/**
	 * Reads in the element \<image\> (the DOMElement) and stores the
	 * result within the CGUISprite.
	 *
	 * @param pElement	The Xerces C++ Parser object that represents
	 *					the image-tag.
	 * @param parent	Parent sprite.
	 *
	 * @see LoadXMLFile()
	 */
	void Xerces_ReadImage(XERCES_CPP_NAMESPACE::DOMElement *pElement, CGUISprite &parent);

	/**
	 * Reads in the element \<style\> (the DOMElement) and stores the
	 * result in m_Styles.
	 *
	 * @param pElement	The Xerces C++ Parser object that represents
	 *					the sprite-tag.
	 *
	 * @see LoadXMLFile()
	 */
	void Xerces_ReadStyle(XERCES_CPP_NAMESPACE::DOMElement *pElement);

	/**
	 * Reads in the element \<scrollbar\> (the DOMElement) and stores the
	 * result in m_ScrollBarStyles.
	 *
	 * @param pElement	The Xerces C++ Parser object that represents
	 *					the scrollbar-tag.
	 *
	 * @see LoadXMLFile()
	 */
	void Xerces_ReadScrollBarStyle(XERCES_CPP_NAMESPACE::DOMElement *pElement);

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
	CPos m_MousePos;

	/// Used when reading in XML files
	// TODO Gee: Used?
	int16_t m_Errors;

	//@}
	//--------------------------------------------------------
	/** @name Objects */
	//--------------------------------------------------------
	//@{

	/**
	 * Base Object, all its children are considered parentless
	 * because this is not a real object per se.
	 */
	IGUIObject* m_BaseObject;

	/** 
	 * Just pointers for fast name access, each object
	 * is really constructed within its parent for easy
	 * recursive management.
	 * Notice m_BaseObject won't belong here since it's
	 * not considered a real object.
	 */
	map_pObjects m_pAllObjects;

	/**
	 * Number of object that has been given name automatically.
	 * the name given will be '__internal(#)', the number (#)
	 * being this variable. When an object's name has been set
	 * as followed, the value will increment.
	 */
	int m_InternalNameNumber;

	/**
	 * Function pointers to functions that constructs
	 * IGUIObjects by name... For instance m_ObjectTypes["button"]
	 * is filled with a function that will "return new CButton();"
	 */
	std::map<CStr, ConstructObjectFunction>	m_ObjectTypes;

	//@}
	//--------------------------------------------------------
	/** @name Databases */
	//--------------------------------------------------------
	//@{

	/// Sprites
	std::map<CStr, CGUISprite> m_Sprites;

	/// Styles
	std::map<CStr, SGUIStyle> m_Styles;

	/// Scroll-bar styles
	std::map<CStr, SGUIScrollBarStyle> m_ScrollBarStyles;

	//@}
};

#endif
