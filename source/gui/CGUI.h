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

/*
 * This is the top class of the whole GUI, all objects
 * and settings are stored within this class.
 */

#ifndef INCLUDED_CGUI
#define INCLUDED_CGUI

#include "GUITooltip.h"
#include "GUIbase.h"

#include "lib/input.h"
#include "ps/Shapes.h"
#include "ps/XML/Xeromyces.h"
#include "scriptinterface/ScriptInterface.h"

#include <boost/unordered_set.hpp>

ERROR_TYPE(GUI, JSOpenFailed);

extern const double SELECT_DBLCLICK_RATE;

/**
 * Contains a list of values for new defaults to objects.
 */
struct SGUIStyle
{
	std::map<CStr, CStrW> m_SettingsDefaults;
};

class JSObject; // The GUI stores a JSObject*, so needs to know that JSObject exists
class IGUIObject;
class CGUISpriteInstance;
struct SGUIText;
struct CColor;
struct SGUIText;
struct SGUIIcon;
class CGUIString;
class CGUISprite;
struct SGUIImageEffects;
struct SGUIScrollBarStyle;
class GUITooltip;

/**
 * The main object that represents a whole GUI page.
 *
 * No interfacial functions throws.
 */
class CGUI
{
	NONCOPYABLE(CGUI);

	friend class IGUIObject;
	friend class CInternalCGUIAccessorBase;

private:
	// Private typedefs
	typedef IGUIObject *(*ConstructObjectFunction)();

public:
	CGUI(const shared_ptr<ScriptRuntime>& runtime);
	~CGUI();

	/**
	 * Initializes the GUI, needs to be called before the GUI is used
	 */
	void Initialize();

	/**
	 * Performs processing that should happen every frame
	 * (including sending the "Tick" event to scripts)
	 */
	void TickObjects();

	/**
	 * Sends a specified script event to every object
	 *
	 * @param EventName String representation of event name
	 */
	void SendEventToAll(const CStr& EventName);

	/**
	 * Displays the whole GUI
	 */
	void Draw();

	/**
	 * Draw GUI Sprite
	 *
	 * @param Sprite Object referring to the sprite (which also caches
	 *        calculations for faster rendering)
	 * @param CellID Number of the icon cell to use. (Ignored if this sprite doesn't
	 *        have any images with "cell-size")
	 * @param Z Drawing order, depth value
	 * @param Rect Position and Size
	 * @param Clipping The sprite shouldn't be drawn outside this rectangle
	 */
	void DrawSprite(const CGUISpriteInstance& Sprite, int CellID, const float& Z, const CRect& Rect, const CRect& Clipping = CRect());

	/**
	 * Draw a SGUIText object
	 *
	 * @param Text Text object.
	 * @param DefaultColor Color used if no tag applied.
	 * @param pos position
	 * @param z z value.
	 * @param clipping
	 */
	void DrawText(SGUIText& Text, const CColor& DefaultColor, const CPos& pos, const float& z, const CRect& clipping);

	/**
	 * Clean up, call this to clean up all memory allocated
	 * within the GUI.
	 */
	void Destroy();

	/**
	 * The replacement of Process(), handles an SDL_Event_
	 *
	 * @param ev SDL Event, like mouse/keyboard input
	 */
	InReaction HandleEvent(const SDL_Event_* ev);

	/**
	 * Load a GUI XML file into the GUI.
	 *
	 * <b>VERY IMPORTANT!</b> All \<styles\>-files must be read before
	 * everything else!
	 *
	 * @param Filename Name of file
	 * @param Paths Set of paths; all XML and JS files loaded will be added to this
	 */
	void LoadXmlFile(const VfsPath& Filename, boost::unordered_set<VfsPath>& Paths);

	/**
	 * Checks if object exists and return true or false accordingly
	 *
	 * @param Name String name of object
	 * @return true if object exists
	 */
	bool ObjectExists(const CStr& Name) const;


	/**
	 * Returns the GUI object with the desired name, or NULL
	 * if no match is found,
	 *
	 * @param Name String name of object
	 * @return Matching object, or NULL
	 */
	IGUIObject* FindObjectByName(const CStr& Name) const;

	/**
	 * Returns the GUI object under the mouse, or NULL if none.
	 */
	IGUIObject* FindObjectUnderMouse() const;

	const SGUIScrollBarStyle* GetScrollBarStyle(const CStr& style) const;

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
	void AddObjectType(const CStr& str, ConstructObjectFunction pFunc) { m_ObjectTypes[str] = pFunc; }

	/**
	 * Update Resolution, should be called every time the resolution
	 * of the OpenGL screen has been changed, this is because it needs
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
	 * will be sent to the Renderer. Also, horizontal alignment
	 * is taken into acount in this method but NOT vertical alignment.
	 *
	 * Done through the CGUI since it can communicate with
	 *
	 * @param Text Text to generate SGUIText object from
	 * @param Font Default font, notice both Default color and default font
	 *		  can be changed by tags.
	 * @param Width Width, 0 if no word-wrapping.
	 * @param BufferZone space between text and edge, and space between text and images.
	 * @param pObject Optional parameter for error output. Used *only* if error parsing fails,
	 *		  and we need to be able to output which object the error occurred in to aid the user.
	 */
	SGUIText GenerateText(const CGUIString& Text, const CStrW& Font, const float& Width, const float& BufferZone, const IGUIObject* pObject = NULL);


	/**
	 * Check if an icon exists
	 */
	bool IconExists(const CStr& str) const { return (m_Icons.count(str) != 0); }

	/**
	 * Get Icon (a copy, can never be changed)
	 */
	SGUIIcon GetIcon(const CStr& str) const { return m_Icons.find(str)->second; }

	/**
	 * Get pre-defined color (if it exists)
	 * Returns false if it fails.
	 */
	bool GetPreDefinedColor(const CStr& name, CColor& Output) const;

	shared_ptr<ScriptInterface> GetScriptInterface() { return m_ScriptInterface; };
	JS::Value GetGlobalObject() { return m_ScriptInterface->GetGlobalObject(); };

private:

	/**
	 * Updates the object pointers, needs to be called each
	 * time an object has been added or removed.
	 *
	 * This function is atomic, meaning if it throws anything, it will
	 * have seen it through that nothing was ultimately changed.
	 *
	 * @throws PSERROR_GUI that is thrown from IGUIObject::AddToPointersMap().
	 */
	void UpdateObjects();

	/**
	 * Adds an object to the GUI's object database
	 * Private, since you can only add objects through
	 * XML files. Why? Because it enables the GUI to
	 * be much more encapsulated and safe.
	 *
	 * @throws	Rethrows PSERROR_GUI from IGUIObject::AddChild().
	 */
	void AddObject(IGUIObject* pObject);

	/**
	 * You input the name of the object type, and let's
	 * say you input "button", then it will construct a
	 * CGUIObjet* as a CButton.
	 *
	 * @param str Name of object type
	 * @return Newly constructed IGUIObject (but constructed as a subclass)
	 */
	IGUIObject* ConstructObject(const CStr& str);

	/**
	 * Get Focused Object.
	 */
	IGUIObject* GetFocusedObject() { return m_FocusedObject; }

public:
	/**
	 * Change focus to new object.
	 * Will send LOST_FOCUS/GOT_FOCUS messages as appropriate.
	 * pObject can be NULL to remove all focus.
	 */
	void SetFocusedObject(IGUIObject* pObject);

private:
	//--------------------------------------------------------
	/** @name XML Reading Xeromyces specific subroutines
	 *
	 * These does not throw!
	 * Because when reading in XML files, it won't be fatal
	 * if an error occurs, perhaps one particular object
	 * fails, but it'll still continue reading in the next.
	 * All Error are reported with ReportParseError
	 */
	//--------------------------------------------------------

	/*
		Xeromyces_* functions tree
		<objects> (ReadRootObjects)
		 |
		 +-<script> (ReadScript)
		 |
		 +-<object> (ReadObject)
			|
			+-<action>
			|
			+-Optional Type Extensions (IGUIObject::ReadExtendedElement) TODO
			|
			+-<<object>> *recursive*


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
		 |
		 +-<color> (ReadColor)
	*/
	//@{

	// Read Roots

	/**
	 * Reads in the root element \<objects\> (the DOMElement).
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the objects-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 * @param Paths		Collects the set of all XML/JS files that are loaded
	 *
	 * @see LoadXmlFile()
	 */
	void Xeromyces_ReadRootObjects(XMBElement Element, CXeromyces* pFile, boost::unordered_set<VfsPath>& Paths);

	/**
	 * Reads in the root element \<sprites\> (the DOMElement).
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the sprites-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 *
	 * @see LoadXmlFile()
	 */
	void Xeromyces_ReadRootSprites(XMBElement Element, CXeromyces* pFile);

	/**
	 * Reads in the root element \<styles\> (the DOMElement).
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the styles-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 *
	 * @see LoadXmlFile()
	 */
	void Xeromyces_ReadRootStyles(XMBElement Element, CXeromyces* pFile);

	/**
	 * Reads in the root element \<setup\> (the DOMElement).
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the setup-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 *
	 * @see LoadXmlFile()
	 */
	void Xeromyces_ReadRootSetup(XMBElement Element, CXeromyces* pFile);

	// Read Subs

	/**
	 * Notice! Recursive function!
	 *
	 * Read in an \<object\> (the XMBElement) and stores it
	 * as a child in the pParent.
	 *
	 * It will also check the object's children and call this function
	 * on them too. Also it will call all other functions that reads
	 * in other stuff that can be found within an object. Check the
	 * tree in the beginning of this class' Xeromyces_* section.
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the object-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 * @param pParent	Parent to add this object as child in.
	 * @param NameSubst	A set of substitution strings that will be
	 * 					applied to all object names within this object.
	 * @param Paths		Output set of file paths that this GUI object
	 * 					relies on.
	 *
	 * @see LoadXmlFile()
	 */
	void Xeromyces_ReadObject(XMBElement Element, CXeromyces* pFile, IGUIObject* pParent, std::vector<std::pair<CStr, CStr> >& NameSubst, boost::unordered_set<VfsPath>& Paths, u32 nesting_depth);

	/**
	 * Reads in the element \<repeat\>, which repeats its child \<object\>s
	 * 'count' times, replacing the string "[n]" (or the value of the attribute
	 * 'var' enclosed in square brackets) in its descendants' names with "[0]",
	 * "[1]", etc.
	 */
	void Xeromyces_ReadRepeat(XMBElement Element, CXeromyces* pFile, IGUIObject* pParent, std::vector<std::pair<CStr, CStr> >& NameSubst, boost::unordered_set<VfsPath>& Paths, u32 nesting_depth);

	/**
	 * Reads in the element \<script\> (the XMBElement) and executes
	 * the script's code.
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the script-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 * @param Paths		Output set of file paths that this script is loaded from.
	 *
	 * @see LoadXmlFile()
	 */
	void Xeromyces_ReadScript(XMBElement Element, CXeromyces* pFile, boost::unordered_set<VfsPath>& Paths);

	/**
	 * Reads in the element \<sprite\> (the XMBElement) and stores the
	 * result in a new CGUISprite.
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the sprite-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 *
	 * @see LoadXmlFile()
	 */
	void Xeromyces_ReadSprite(XMBElement Element, CXeromyces* pFile);

	/**
	 * Reads in the element \<image\> (the XMBElement) and stores the
	 * result within the CGUISprite.
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the image-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 * @param parent	Parent sprite.
	 *
	 * @see LoadXmlFile()
	 */
	void Xeromyces_ReadImage(XMBElement Element, CXeromyces* pFile, CGUISprite& parent);

	/**
	 * Reads in the element \<effect\> (the XMBElement) and stores the
	 * result within the SGUIImageEffects.
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the image-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 * @param effects	Effects object to add this effect to.
	 *
	 * @see LoadXmlFile()
	 */
	void Xeromyces_ReadEffects(XMBElement Element, CXeromyces* pFile, SGUIImageEffects& effects);

	/**
	 * Reads in the element \<style\> (the XMBElement) and stores the
	 * result in m_Styles.
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the style-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 *
	 * @see LoadXmlFile()
	 */
	void Xeromyces_ReadStyle(XMBElement Element, CXeromyces* pFile);

	/**
	 * Reads in the element \<scrollbar\> (the XMBElement) and stores the
	 * result in m_ScrollBarStyles.
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the scrollbar-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 *
	 * @see LoadXmlFile()
	 */
	void Xeromyces_ReadScrollBarStyle(XMBElement Element, CXeromyces* pFile);

	/**
	 * Reads in the element \<icon\> (the XMBElement) and stores the
	 * result in m_Icons.
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the scrollbar-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 *
	 * @see LoadXmlFile()
	 */
	void Xeromyces_ReadIcon(XMBElement Element, CXeromyces* pFile);

	/**
	 * Reads in the element \<tooltip\> (the XMBElement) and stores the
	 * result as an object with the name __tooltip_#.
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the scrollbar-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 *
	 * @see LoadXmlFile()
	 */
	void Xeromyces_ReadTooltip(XMBElement Element, CXeromyces* pFile);

	/**
	 * Reads in the element \<color\> (the XMBElement) and stores the
	 * result in m_PreDefinedColors
	 *
	 * @param Element	The Xeromyces object that represents
	 *					the scrollbar-tag.
	 * @param pFile		The Xeromyces object for the file being read
	 *
	 * @see LoadXmlFile()
	 */
	void Xeromyces_ReadColor(XMBElement Element, CXeromyces* pFile);

	//@}

private:

	// Variables

	//--------------------------------------------------------
	/** @name Miscellaneous */
	//--------------------------------------------------------
	//@{

	shared_ptr<ScriptInterface> m_ScriptInterface;

	/**
	 * don't want to pass this around with the
	 * ChooseMouseOverAndClosest broadcast -
	 * we'd need to pack this and pNearest in a struct
	 */
	CPos m_MousePos;

	/**
	 * Indicates which buttons are pressed (bit 0 = LMB,
	 * bit 1 = RMB, bit 2 = MMB)
	 */
	unsigned int m_MouseButtons;

	// Tooltip
	GUITooltip m_Tooltip;

	/**
	 * This is a bank of custom colors, it is simply a look up table that
	 * will return a color object when someone inputs the name of that
	 * color. Of course the colors have to be declared in XML, there are
	 * no hard-coded values.
	 */
	std::map<CStr, CColor>	m_PreDefinedColors;

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
	 * Focused object!
	 * Say an input box that is selected. That one is focused.
	 * There can only be one focused object.
	 */
	IGUIObject* m_FocusedObject;

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

	/**
	 * Map from hotkey names to objects that listen to the hotkey.
	 * (This is an optimisation to avoid recursing over the whole GUI
	 * tree every time a hotkey is pressed).
	 * Currently this is only set at load time - dynamic changes to an
	 * object's hotkey property will be ignored.
	 */
	std::map<CStr, std::vector<IGUIObject*> > m_HotkeyObjects;

	//--------------------------------------------------------
	//	Databases
	//--------------------------------------------------------

	// Sprites
	std::map<CStr, CGUISprite*> m_Sprites;

	// Styles
	std::map<CStr, SGUIStyle> m_Styles;

	// Scroll-bar styles
	std::map<CStr, SGUIScrollBarStyle> m_ScrollBarStyles;

	// Icons
	std::map<CStr, SGUIIcon> m_Icons;
};

#endif // INCLUDED_CGUI
