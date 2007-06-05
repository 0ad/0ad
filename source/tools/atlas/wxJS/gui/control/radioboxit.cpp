#include "precompiled.h"

// radioboxit.cpp

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../../common/main.h"
#include "../../common/index.h"

#include "radiobox.h"
#include "radioboxit.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>control/radioboxit</file>
 * <module>gui</module>
 * <class name="wxRadioBoxItem">
 *	wxRadioBoxItem is a helper class for working with items of @wxRadioBox.
 *	There's no corresponding class in wxWidgets. 
 *	See wxRadioBox @wxRadioBox#item property.
 * </class>
 */
WXJS_INIT_CLASS(RadioBoxItem, "wxRadioBoxItem", 0)

/***
 * <properties>
 *	<property name="enable" type="Boolean">
 *   Enables/Disables the button.
 *  </property>
 *	<property name="selected" type="Boolean">
 *   Returns true when the item is selected or sets the selection
 *  </property>
 *	<property name="show" type="Boolean">
 *   Hides or shows the item.
 *  </property>
 *	<property name="string" type="String">
 *	 Get/Set the label of the button.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(RadioBoxItem)
	WXJS_PROPERTY(P_ENABLE, "enable")
	WXJS_PROPERTY(P_STRING, "string")
	WXJS_PROPERTY(P_SELECTED, "selected")
	WXJS_PROPERTY(P_SHOW, "show")
WXJS_END_PROPERTY_MAP()

bool RadioBoxItem::GetProperty(Index *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	JSObject *parent = JS_GetParent(cx, obj);
	wxASSERT_MSG(parent != NULL, wxT("No parent found for RadioBoxItem"));

    wxRadioBox *radiobox = RadioBox::GetPrivate(cx, parent);
	if ( radiobox == NULL )
		return false;

	// When id is greater then 0, then we have an array index.
	if ( id >= 0 )
	{
		if ( id < radiobox->GetCount() )
		{
			// Set the item index and don't forget to return ourselves.
			p->SetIndex(id);
			*vp = OBJECT_TO_JSVAL(obj);
		}
	}
	else
	{
		// A negative index means a defined property.
        int idx = p->GetIndex();
		if ( idx < radiobox->GetCount() ) // To be sure
		{
			switch (id) 
			{
			case P_STRING:
                *vp = ToJS(cx, radiobox->wxControl::GetLabel());
				break;
			case P_SELECTED:
				*vp = ToJS(cx, radiobox->GetSelection() == idx);
				break;
			}
		}
	}
    return true;
}

bool RadioBoxItem::SetProperty(Index *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	JSObject *parent = JS_GetParent(cx, obj);
	wxASSERT_MSG(parent != NULL, wxT("No parent found for RadioBoxItem"));

    wxRadioBox *radiobox = RadioBox::GetPrivate(cx, parent);
	if ( radiobox == NULL )
		return false;

    int idx = p->GetIndex();
	if ( idx < radiobox->GetCount() ) // To be sure
	{
		switch (id) 
		{
		case P_STRING:
			{
				wxString str;
				FromJS(cx, *vp, str);
				radiobox->SetString(idx, str);
				break;
			}
		case P_SELECTED:
			{
				bool value;
				if (    FromJS(cx, *vp, value) 
					 && value )
					radiobox->SetSelection(idx);
				break;
			}
		case P_ENABLE:
			{
				bool value;
				if ( FromJS(cx, *vp, value) )
					radiobox->Enable(idx, value);
				break;
			}
		case P_SHOW:
			{
				bool value;
				if ( FromJS(cx, *vp, value) )
					radiobox->Show(idx, value);
				break;
			}
		}
	}
	return true;
}
