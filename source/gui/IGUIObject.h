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

#ifndef IGUIObject_H
#define IGUIObject_H

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

//--------------------------------------------------------
//  Error declarations
//--------------------------------------------------------

//--------------------------------------------------------
//  Declarations
//--------------------------------------------------------


// Text alignments
enum EAlign { EAlign_Left, EAlign_Right, EAlign_Center };
enum EValign { EValign_Top, EValign_Bottom, EValign_Center };

/**
 * @author Gustav Larsson
 *
 * Stores the information where to find a variable
 * in a GUI-Object object, also what type it is.
 */
struct SGUISetting
{
	size_t			m_Offset;		// The offset from IGUIObject to the variable (not from SGUIBaseSettings or similar)
	CStr			m_Type;			// "string" or maybe "int"
};

/**
 * @author Gustav Larsson
 *
 * Base settings, all objects possess these settings
 * in their m_BaseSettings
 * Instructions can be found in the documentations.
 */
struct SGUIBaseSettings
{
	bool			m_Hidden;
	bool			m_Enabled;
	bool			m_Absolute;
	CClientArea		m_Size;
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
class IGUIObject
{
	friend class CGUI;

	friend class CInternalCGUIAccessorBase;
#ifndef _MSC_VER
	template <class T>
#endif
	friend class GUI;

public:
	IGUIObject();
	virtual ~IGUIObject();
	
	/**
	 * Get offsets
	 *
	 * @return Retrieves settings info
	 */
	virtual map_Settings GetSettingsInfo() const { return m_SettingsInfo; }

	/**
	 * Checks if mouse is hovering this object.
     * The mouse position is cached in CGUI.
	 *
	 * This function checks if the mouse is hovering the
	 * rectangle that the base setting "size" makes.
	 * Although it is virtual, so one could derive
	 * an object from CButton, which changes only this
	 * to checking the circle that "size" makes.
	 *
	 * @return true if mouse is hovering
	 */
	virtual bool MouseOver();

	//--------------------------------------------------------
	/** @name Leaf Functions */
	//--------------------------------------------------------
	//@{

	/// Get object name, name is unique
	CStr GetName() const { return m_Name; }

	/// Get object name
	void SetName(const CStr &Name) { m_Name = Name; }

	/**
	 * Adds object and its children to the map, it's name being the
	 * first part, and the second being itself.
	 *
	 * @param ObjectMap Adds this to the map_pObjects.
	 *
	 * @throws PS_NEEDS_NAME Name is missing
	 * @throws PS_NAME_AMBIGUITY Name is already taken
	 */
	void AddToPointersMap(map_pObjects &ObjectMap);

	/**
	 * Notice nothing will be returned or thrown if the child hasn't 
	 * been inputted into the GUI yet. This is because that's were 
	 * all is checked. Now we're just linking two objects, but
	 * it's when we're inputting them into the GUI we'll check
	 * validity! Notice also when adding it to the GUI this function
	 * will inevitably have been called by CGUI::AddObject which
	 * will catch the throw and return the error code.
	 * i.e. The user will never put in the situation wherein a throw
	 * must be caught, the GUI's internal error handling will be
	 * completely transparent to the interfacially sequential model.
	 *
	 * @param pChild Child to add
	 *
	 * @throws PS_RESULT from CGUI::UpdateObjects().
	 */
	void AddChild(IGUIObject *pChild);

	//@}
	//--------------------------------------------------------
	/** @name Iterate */
	//--------------------------------------------------------
	//@{

	vector_pObjects::iterator ChildrenItBegin()	{ return m_Children.begin(); }
	vector_pObjects::iterator ChildrenItEnd()	{ return m_Children.end(); }

	//@}
	//--------------------------------------------------------
	/** @name Settings Management */
	//--------------------------------------------------------
	//@{

	SGUIBaseSettings GetBaseSettings() const { return m_BaseSettings; }
	
	/**
	 * Checks if settings exists, only available for derived
	 * classes that has this set up, that's why the base
	 * class just returns false
	 *
	 * @param Setting setting name
	 * @return True if settings exist.
	 */
	bool SettingExists(const CStr &Setting) const;
	
	/**
	 * All sizes are relative to resolution, and the calculation
	 * is not wanted in real time, therefore it is cached, update
	 * the cached size with this function.
	 */
	void UpdateCachedSize();

	/**
	 * Should be called every time the settings has been updated
	 * will also send a message GUIM_SETTINGS_UPDATED, so that
	 * if a derived object wants to add things to be updated,
	 * they add it in that message part, this is a better solution
	 * than making this virtual, since the updates that the base
	 * class does, are the most essential.
	 * This is not private since there should be no harm in
	 * checking validity.
	 *
	 * @throws GeeTODO not quite settled yet.
	 */
	void CheckSettingsValidity();

	/**
	 * Sets up a map_size_t to include the variables in m_BaseSettings
	 *
	 * @param SettingsInfo Pointers that should be filled with base variables
	 */
	void SetupBaseSettingsInfo(map_Settings &SettingsInfo);

	/**
	 * Set a setting by string, regardless of what type it is.
	 *
	 * example a CRect(10,10,20,20) would be "10 10 20 20"
	 *
	 * @param Setting Setting by name
	 * @param Value Value to set to
	 */
	void SetSetting(const CStr &Setting, const CStr &Value);

	//@}
protected:
	//--------------------------------------------------------
	/** @name Called by CGUI and friends
	 *
	 * Methods that the CGUI will call using
	 * its friendship, these should not
	 * be called by user.
	 * These functions' security are a lot
	 * what constitutes the GUI's
	 */
	//--------------------------------------------------------
	//@{

	/**
	 * Calls Destroy on all children, and deallocates all memory.
	 * BIG TODO Should it destroy it's children?
	 */
	virtual void Destroy();
	
	/**
     * This function is called with different messages
	 * for instance when the mouse enters the object.
	 *
	 * @param Message EGUIMessage
	 */
	virtual void HandleMessage(const EGUIMessage &Message)=0;

	/**
	 * Draws the object.
	 *
	 * @throws	PS_RESULT if any. But this will mostlikely be
	 *			very rare since if an object is drawn unsuccessfully
	 *			it'll probably only output in the Error log, and not
	 *			disrupt the whole GUI drawing.
	 */
	virtual void Draw()=0;

	// This is done internally
	CGUI *GetGUI() { return m_pGUI; }
	const CGUI *GetGUI() const { return m_pGUI; }
	void SetGUI(CGUI * const &pGUI) { m_pGUI = pGUI; }

	// Set parent
	void SetParent(IGUIObject *pParent) { m_pParent = pParent; }
	
	virtual void ResetStates()
	{
		m_MouseHovering = false;
	}

	/**
	 * <b>NOTE!</b> This will not just return m_pParent, when that is
	 * need use it! There is one exception to it, when the parent is
	 * the top-node (the object that isn't a real object), this
	 * will return NULL, so that the top-node's children are
	 * seemingly parentless.
	 *
	 * @return Pointer to parent
	 */
	IGUIObject *GetParent();

	// Get cached mouse x/y from CGUI
	u16 GetMouseX() const;
	u16 GetMouseY() const;

	/**
	 * Cached size, real size m_Size is actually dependent on resolution
	 * and can have different *real* outcomes, this is the real outcome
	 * cached to avoid slow calculations in real time.
	 */
	CRect m_CachedActualSize;

	//@}
private:
	//--------------------------------------------------------
	/** @name Internal functions */
	//--------------------------------------------------------
	//@{
	
	/**
	 * Inputs a reference pointer, checks if the new inputted object
	 * if hovered, if so, then check if this's Z value is greater
	 * than the inputted object... If so then the object is closer
	 * and we'll replace the pointer with this.
	 * Also Notice input can be NULL, which means the Z value demand
	 *  is out. NOTICE you can't input NULL as const so you'll have
	 * to set an object to NULL.
	 *
	 * @param pObject	Object pointer, can be either the old one, or
	 *					the new one.
	 */
	void ChooseMouseOverAndClosest(IGUIObject* &pObject);

	/**
	 * Inputes the object that is currently hovered, this function
	 * updates this object accordingly (i.e. if it's the object
	 * being inputted one thing happens, and not, another).
	 *
	 * @param pMouseOver	Object that is currently hovered,
	 *						can OF COURSE be NULL too!
	 */
	void UpdateMouseOver(IGUIObject * const &pMouseOver);

	//@}

	// Variables

protected:
	/// Name of object
	CStr									m_Name;

	/// Constructed on the heap, will be destroyed along with the the object TODO Really?
	vector_pObjects							m_Children;

	/// Pointer to parent
	IGUIObject								*m_pParent;

	/// Base settings
	SGUIBaseSettings						m_BaseSettings;

	// More variables

	/// Is mouse hovering the object? used with the function MouseOver()
	bool									m_MouseHovering;

	/**	
	 * Tells us where a variable by a string name is
	 * located hardcoded, in order to acquire a pointer
	 * for that variable... Say "frozen" gives
	 * the offset from IGUIObject to m_Frozen.
	 * <b>note!</b> @uNOT from SGUIBaseSettings to 
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
class CGUIDummyObject : public IGUIObject
{
	virtual void HandleMessage(const EGUIMessage &Message) {}
	virtual void Draw() {}
};

#endif
