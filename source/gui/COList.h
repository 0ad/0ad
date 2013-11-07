#ifndef INCLUDED_COLIST
#define INCLUDED_COLIST

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------
#include "GUI.h"
#include "CList.h"

//--------------------------------------------------------
//  Macros
//--------------------------------------------------------

//--------------------------------------------------------
//  Types
//--------------------------------------------------------

//--------------------------------------------------------
//  Declarations
//--------------------------------------------------------

struct ObjectDef
{
  CColor m_TextColor;
  CStr m_Id;
  float m_Width;
  CStrW m_Heading;

};

/**
 *  Todo : add description
 *
 */
class COList : public CList
{
	GUI_OBJECT(COList)

public:
	COList();

protected:
	void SetupText();
	void HandleMessage(SGUIMessage &Message);

	/**
	 * Handle the \<item\> tag.
	 */
	virtual bool HandleAdditionalChildren(const XMBElement& child, CXeromyces* pFile);

	void DrawList(const int &selected, const CStr& _sprite,
					const CStr& _sprite_selected, const CStr& _textcolor);

	virtual CRect GetListRect() const;

	std::vector<ObjectDef> m_ObjectsDefs;

private:
	float m_HeadingHeight;
	// Width of space avalible for columns
	float m_TotalAvalibleColumnWidth;
};

#endif // INCLUDED_COLIST
