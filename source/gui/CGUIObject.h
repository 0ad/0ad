/*
The base class of an object
by Gustav Larsson
gee@pyro.nu

--Overview--

	All objects are derived from this class, it's an ADT
	so it can't be used per se

	Also contains a Dummy object which is used for
	 completely blank objects.

--Usage--

	Write about how to use it here

--Examples--

	Provide examples of how to use this code, if necessary

--More info--

	Check GUI.h

*/

#ifndef CGUIObject_H
#define CGUIObject_H

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------
#include "GUI.h"
#include <string>
#include <vector>

struct SGUISetting;
class CGUI;

//--------------------------------------------------------
//  Macros
//--------------------------------------------------------

//--------------------------------------------------------
//  Types
//--------------------------------------------------------

// Map with pointers
typedef std::map<CStr, SGUISetting> map_Settings;
typedef std::vector<CGUIObject*> vector_pObjects;

//--------------------------------------------------------
//  Error declarations
//--------------------------------------------------------

//--------------------------------------------------------
//  Declarations
//--------------------------------------------------------

// TEMP
struct CRect
{
	CRect() {}
	CRect(int _l, int _b, int _r, int _t) :
		top(_t),
		bottom(_b),
		right(_r),
		left(_l) {}
	int bottom, top, left, right;

	bool operator ==(const CRect &rect) const
	{
		return	(bottom==rect.bottom) &&
				(top==rect.top) &&
				(left==rect.left) &&
				(right==rect.right);
	}

	bool operator !=(const CRect &rect) const
	{
		return	!(*this==rect);
	}
};

// TEMP
struct CColor
{
	float r, g, b, a;
};

// Text alignments
enum EAlign { EAlign_Left, EAlign_Right, EAlign_Center };
enum EValign { EValign_Top, EValign_Bottom, EValign_Center };

/**
 * Stores the information where to find a variable
 * in a GUI-Object object, also what type it is.
 */
struct SGUISetting
{
	size_t			m_Offset;		// The offset from CGUIObject to the variable (not from SGUIBaseSettings or similar)
	CStr			m_Type;			// "string" or maybe "int"
};

/**
 * Base settings, all objects possess these settings
 * in their <code>m_BaseSettings</code>
 * Instructions can be found in the documentations.
 */
struct SGUIBaseSettings
{
	bool			m_Hidden;
	bool			m_Enabled;
	bool			m_Absolute;
	CRect			m_Size;
	CStr			m_Style;
	float			m_Z;
	CStr			m_Caption;	// Is usually set within an XML element and not in the attributes
};

//////////////////////////////////////////////////////////

/**
 * @author Gustav Larsson
 *
 * GUI object such as a button or an input-box.
 * Abstract data type !
 */
class CGUIObject
{
	friend class CGUI;
	friend class GUI;

public:
	CGUIObject();
	virtual ~CGUIObject();
	
	/**
	 * Get offsets
	 *
	 * @return Retrieves settings info
	 */
	virtual map_Settings GetSettingsInfo() const { return m_SettingsInfo; }

	/**
	 * Is mouse over
	 * because it's virtual you can change the
	 * mouse over demands, such as making a round
	 * button, or just modifying it when changed,
	 * like for a combo box.
	 *
	 * The default one uses <code>m_Size</code>.
	 *
	 * @return true if mouse is over
	 */
	virtual bool MouseOver();

	//--------------------------------------------------------
	//	Leaf Functions
	//--------------------------------------------------------
	/** @name Leaf Functions */
	//@{

	/// Get object name, name is unique
	CStr GetName() const { return m_Name; }

	/// Get object name
	void SetName(const CStr &Name) { m_Name = Name; }

	/**
	 * Fill a map_pObjects with this object (does not include recursion)
	 *
	 * @param ObjectMap Adds <code>this</code> to the <code>map_pObjects</code>.
	 */
	void AddToPointersMap(map_pObjects &ObjectMap);

	/**
	 * Add child
	 *
	 * @param pChild Add child by pointer
	 */
	void AddChild(CGUIObject *pChild);

	//@}
	//--------------------------------------------------------
	//	Iterate
	//--------------------------------------------------------
	/** @name Iterate */
	//@{

	vector_pObjects::iterator ChildrenItBegin()	{ return m_Children.begin(); }
	vector_pObjects::iterator ChildrenItEnd()	{ return m_Children.end(); }

	//@}
	//--------------------------------------------------------
	//	Settings Management
	//--------------------------------------------------------
	/** @name Settings Management */
	//@{

	SGUIBaseSettings GetBaseSettings() const { return m_BaseSettings; }
	void SetBaseSettings(const SGUIBaseSettings &Set);

	/**
	 * Checks if settings exists, only available for derived
	 * classes that has this set up, that's why the base
	 * class just returns false
	 *
	 * @param Setting setting name
	 * @return true if settings exist
	 */
	bool SettingExists(const CStr &Setting) const;

	/// Setup base pointers
	void SetupBaseSettingsInfo(map_Settings &SettingsInfo);

	/// Set Setting by string
	void SetSetting(const CStr &Setting, const CStr &Value);

	/**
	 * Should be called every time the settings has been updated
	 * will also send a message <code>GUIM_SETTINGS_UPDATED</code>, so that
	 * if a derived object wants to add things to be updated,
	 * they add it in that message part, this is a better solution
	 * than making this virtual, since the updates that the base
	 * class does, are the most essential.
	 * This is not private since there should be no harm in
	 * checking validity.
	 */
	void CheckSettingsValidity();

	//@}
protected:
	//--------------------------------------------------------
	//	Methods that the CGUI will call using
	//   its friendship, these should not
	//	 be called by user.
	//
	//	These functions' security are a lot
	//	 what constitutes the GUI's
	//--------------------------------------------------------
	/** @name Internal methods that uses friendship */
	//@{

	/**
	 * Calls Destroy on all children, and deallocates all memory
	 */
	virtual void Destroy();
	
	/**
     * This function is called with different messages
	 * for instance when the mouse enters the object.
	 *
	 * @param Message <code>EGUIMessage</code>.
	 */
	virtual void HandleMessage(const EGUIMessage &Message)=0;

	/**
	 * Draws the object
	 */
	virtual void Draw()=0;

	// This is done internally
	CGUI *GetGUI() { return m_pGUI; }
	const CGUI *GetGUI() const { return m_pGUI; }
	void SetGUI(CGUI * const &pGUI) { m_pGUI = pGUI; }

	// Set parent
	void SetParent(CGUIObject *pParent) { m_pParent = pParent; }
	
	/**
	 * <b>NOTE!</b> This will not just return <code>m_pParent</code>, when that is need
	 * use it! There is one exception to it, when the parent is
	 * the top-node (the object that isn't a real object), this
	 * will return <code>NULL</code>, so that the top-node's children are
	 * seemingly parentless.
	 *
	 * @return Pointer to parent
	 */
	CGUIObject *GetParent();

	/**
	 * Clear children, removes all children
	 * Update objects, will basically use the this base class
	 * to access a private member in <code>CGUI</code>, this base
	 * class will be only one with permission
	 */
//	void UpdateObjects();

	// Get cached mouse x/y from CGUI
	u16 GetMouseX() const;
	u16 GetMouseY() const;

private:
	// Functions used fully private and by friends (mainly the CGUI)
	
	/**
	 * You input pointer, and if the Z value of this object
	 * is greater than the one inputted, the pointer is changed to this.
	 *
	 * @param pObject Object pointer
	 */
	void ChooseMouseOverAndClosest(CGUIObject* &pObject);

	/**
	 * Update Mouse Over (for this object only)
	 *
	 * @param pMouseOver
	 */
	void UpdateMouseOver(CGUIObject * const &pMouseOver);


	// Variables

protected:
	/// Name of object
	CStr									m_Name;

	/// Constructed on the heap, will be destroyed along with the the object
	vector_pObjects							m_Children;

	/// Pointer to parent
	CGUIObject								*m_pParent;

	/// Base settings
	SGUIBaseSettings						m_BaseSettings;

	// More variables

	/// Is mouse hovering the object? used with the function <code>MouseOver()</code>
	bool									m_MouseHovering;

	/**	
	 * Offset database
	 * tells us where a variable by a string name is
	 * located hardcoded, in order to acquire a pointer
	 * for that variable... Say "frozen" gives
	 * the offset from CGUIObject to m_Frozen.
	 * <b>note!</b> <u>NOT</u> from SGUIBaseSettings to 
	 * m_Frozen!
	 */
	static map_Settings						m_SettingsInfo;

private:
	/// An object can't function stand alone
	CGUI									*m_pGUI;
};


/**
 * @author Gustav Larsson
 *
 * Dummy object used primarily for the root object
 * which isn't a *real* object in the GUI.
 */
class CGUIDummyObject : public CGUIObject
{
	virtual void HandleMessage(const EGUIMessage &Message) {}
	virtual void Draw() {}
};

#endif