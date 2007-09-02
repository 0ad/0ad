#include "precompiled.h"

/*
 * wxJavaScript - listctrl.cpp
 *
 * Copyright (c) 2002-2007 Franky Braem and the wxJavaScript project
 *
 * Project Info: http://www.wxjavascript.net or http://wxjs.sourceforge.net
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * $Id: listctrl.cpp 746 2007-06-11 20:58:21Z fbraem $
 */
// ListCtrl.cpp
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#ifdef __WXMSW__
	#include "commctrl.h"
#endif

#include "../../common/main.h"


#include "../event/jsevent.h"
#include "../event/listevt.h"

#include "../misc/app.h"
#include "../misc/validate.h"
#include "../misc/point.h"
#include "../misc/size.h"
#include "../misc/rect.h"
#include "../misc/colour.h"
#include "../misc/imagelst.h"

#include "listctrl.h"
#include "listitem.h"
#include "listitattr.h"
#include "listhit.h"
#include "window.h"
#include "textctrl.h"
#include "../errors.h"

using namespace wxjs;
using namespace wxjs::gui;

ListCtrl::ListCtrl() : wxListCtrl()
{
}

ListCtrl::~ListCtrl()
{
  // Destroy all item data
    int n = GetItemCount();
    long i = 0;

    for (i = 0; i < n; i++)
    {
        ListObjectData *wxjs_data = (ListObjectData*) GetItemData(i);
        if ( wxjs_data != NULL )
        {
            delete wxjs_data;
            wxjs_data = NULL;
        }
    }
}

int wxCALLBACK ListCtrl::SortFn(long item1, long item2, long data)
{
    ListSort *sortData = reinterpret_cast<ListSort*>(data);

    ListObjectData *data1 = (ListObjectData*) item1;
    ListObjectData *data2 = (ListObjectData*) item2;

    jsval rval;
    jsval argv[] = { data1 == NULL ? JSVAL_VOID : data1->GetJSVal(),
                     data2 == NULL ? JSVAL_VOID : data2->GetJSVal(),
                     ToJS(sortData->GetContext(), sortData->GetData())
                   };
    if ( JS_CallFunction(sortData->GetContext(), JS_GetGlobalObject(sortData->GetContext()), sortData->GetFn(), 3, argv, &rval) == JS_TRUE )
    {
        int rc;
        if ( FromJS(sortData->GetContext(), rval, rc) )
            return rc;
        else
            return 0;
    }
    else
    {
        JS_ReportError(sortData->GetContext(), "Error occurred while running sort function of wxListCtrl");
    }
    return 0;
}

/***
 * <file>control/listctrl</file>
 * <module>gui</module>
 * <class name="wxListCtrl" prototype="@wxControl">
 *  A list control presents lists in a number of formats: list view, report view,
 *  icon view and small icon view. In any case, elements are numbered from zero.
 *  For all these modes, the items are stored in the control and must be added to 
 *  it using @wxListCtrl#insertItem method.
 *  <br /><br />
 *  A special case of report view quite different from the other modes of the 
 *  list control is a virtual control in which the items data (including text,
 *  images and attributes) is managed by the main program and is requested by 
 *  the control itself only when needed which allows to have controls with millions 
 *  of items without consuming much memory. To use virtual list control you must set
 *  @wxListCtrl#itemCount first and set at least @wxListCtrl#onGetItemText
 *  (and optionally @wxListCtrl#onGetItemImage and @wxListCtrl#onGetItemAttr)
 *  to return the information about the items when the control requests it.
 *  Virtual list control can be used as a normal one except that no operations 
 *  which can take time proportional to the number of items in the control happen
 *  -- this is required to allow having a practically infinite number of items. 
 *  For example, in a multiple selection virtual list control, the selections won't be 
 *  sent when many items are selected at once because this could mean iterating 
 *  over all the items.
 *  <br /><br /><b>How to sort items?</b><br />
 *  This example shows a dialog containing a wxListCtrl and two buttons.
 *  The list control contains 4 elements 'A', 'B', 'C' and 'D'. Clicking
 *  on one of the buttons, will sort the elements in
 *  descending or ascending order. First the dialog box is created. 
 *  A @wxBoxSizer is used to layout
 *  the dialog. The list control will be shown above the buttons and centered
 *  horizontally. Above and below the listcontrol a space of 10 is placed.
 *  <pre><code class="whjs">
 *   dlg = new wxDialog(null, -1, "wxListCtrl sort example", 
 *                      wxDefaultPosition, new wxSize(200, 200));
 *   // The main sizer
 *   dlg.sizer = new wxBoxSizer(wxOrientation.VERTICAL);
 *
 *   // Create the listcontrol and add it to the sizer.
 *   list = new wxListCtrl(dlg, -1, wxDefaultPosition, new wxSize(100, 100));
 *   dlg.sizer.add(list, 0, wxAlignment.CENTER | wxDirection.BOTTOM | wxDirection.TOP, 10);
 *
 *   // Create buttons and add them to a boxsizer.
 *   var btn1 = new wxButton(dlg, -1, "Sort Descending");
 *   btn1.onClicked = function()
 *   {
 *     list.sortItems(sort, 1);
 *   };
 *  
 *   var btn2 = new wxButton(dlg, -2, "Sort Ascending");
 *   btn2.onClicked = function()
 *   {
 *     list.sortItems(sort, 0);
 *   };
 *
 *   boxsizer = new wxBoxSizer(wxOrientation.HORIZONTAL);
 *   boxsizer.add(btn1, 0, wxDirection.RIGHT, 10);
 *   boxsizer.add(btn2);
 *
 *   dlg.sizer.add(boxsizer, 0, wxAlignment.CENTER);
 *   </code></pre>
 *  An array which hold the characters 'A', 'B', 'C' and 'D' is created. Each element is
 *  inserted. The itemdata is set to the letter. This is necessary
 *  because the sort function uses the itemdata.
 *  <pre><code class="whjs"> 
 *   var letter = new Array();
 *   letter[0] = "A";
 *   letter[1] = "B";
 *   letter[2] = "C";
 *   letter[3] = "D";
 *   for(e in letter)
 *   {
 *     var idx = list.insertItem(e, letter[e]);
 *     list.setItemData(idx, letter[e]);
 *   }
 *  </code></pre>
 *  Show the dialog.
 *  <pre><code class="whjs">
 *   dlg.layout();
 *   dlg.showModal();
 *  </code></pre>
 *  
 *  The function gets 3 arguments. The first two are the itemdata (which is the index
 *  of the corresponding array element) of the first item and the second item. Data
 *  is the data passed to @wxListCtrl#sortItems in the @wxButton#onClicked event
 *  of @wxButton. This way the sort function knows how to sort the items.
 *  <pre><code class="whjs">
 *   function sort(item1, item2, data)
 *   {
 *     if ( data == 1 )
 *     {
 *       if ( item1 &gt; item2 )
 *         return -1;
 *       else if ( item1 &lt; item2 )
 *         return 1;
 *       else return 0;
 *     }
 *     else
 *     {
 *       if ( item1 &gt; item2 )
 *         return 1;
 *       else if ( item1 &lt; item2 )
 *         return -1;
 *       else return 0;
 *     }
 *   }
 *   </code></pre>
 *  <br /><b>How to use virtual list controls ?</b><br />
 *  This example shows how a virtual list control is built.
 *  First an array is create which will contain objects of type Car.
 *  <pre><code class="whjs">
 *   var cars = new Array();
 *
 *   function Car(brand, type, colour)
 *   {
 *     this.brand = brand;
 *     this.type = type;
 *     this.colour = colour;
 *   }
 *
 *   cars[0] = new Car("Opel", "Astra", "Red");
 *   cars[1] = new Car("Ford", "Focus", "Black");
 *   cars[2] = new Car("Skoda", "Octavia", "Green");
 *   cars[3] = new Car("Peugeot", "305", "White");
 *   cars[4] = new Car("Citroen", "Picasso", "Green");
 *   </code></pre>
 *  The <i>init</i> function will create a frame and a list control. The list control
 *  is the child of the frame and its style is REPORT and VIRTUAL. For each property
 *  of Car, a column is created.
 *  Because the list control is virtual, @wxListCtrl#onGetItemText and 
 *  @wxListCtrl#itemCount must be set.
 *  <pre><code class="whjs">
 *   function init()
 *   {
 *     var frame = new wxFrame(null, -1, "wxListCtrl example");
 *   
 *     var listCtrl = new wxListCtrl(frame, -1, wxDefaultPosition,
 *                                   wxDefaultSize, wxListCtrl.REPORT | wxListCtrl.VIRTUAL);
 *     listCtrl.insertColumn(0, "Brand");
 *     listCtrl.insertColumn(1, "Type");
 *     listCtrl.insertColumn(2, "Colour");
 *     listCtrl.onGetItemText = getItemText;
 *     listCtrl.itemCount = cars.length;
 *     
 *     frame.visible = true;
 *     topWindow = frame;
 *     
 *     return true;
 *     }
 *   </code></pre>
 *  The function <i>getItemText</i> will be called by wxWidgets whenever it needs
 *  to know the text of an item. This function gets two arguments: the item and the column
 *  index. The text that must be shown is returned.
 *  <pre><code class="whjs">
 *   function getItemText(item, col)
 *   {
 *     var car = cars[item];
 *   
 *     switch(col)
 *     {
 *       case 0: return car.brand;
 *       case 1: return car.type;
 *       case 2: return car.colour;    
 *     }
 *   }
 *   
 *   wxTheApp.onInit = init;
 *   wxTheApp.mainLoop();
 *  </code></pre>
 * </class>
 */
WXJS_INIT_CLASS(ListCtrl, "wxListCtrl", 2)

/***
 * <constants>
 *  <type name="wxListMask">
 *	 <constant name="STATE" />
 *	 <constant name="TEXT" />
 *	 <constant name="IMAGE" />
 *	 <constant name="DATA" />
 *	 <constant name="ITEM" />
 *	 <constant name="WIDTH" />
 *	 <constant name="FORMAT" />
 *   <desc>
 *    wxListMask is ported as a separate JavaScript class
 *   </desc>
 *  </type>
 *  <type name="wxListState">
  *	 <constant name="DONTCARE" />
 *	 <constant name="DROPHILITED">(Windows only)</constant>
 *	 <constant name="FOCUSED" />
 *	 <constant name="SELECTED" />
 *	 <constant name="CUT">(Windows only)</constant>
 *   <desc>
 *    wxListState is ported as a separate JavaScript class
 *   </desc>
 *  </type>
 *  <type name="wxListNext">
 *	 <constant name="ABOVE">Searches for an item above the specified item</constant>
 *	 <constant name="ALL">Searches for subsequent item by index</constant>
 *	 <constant name="BELOW">Searches for an item below the specified item</constant>
 *	 <constant name="LEFT">Searches for an item to the left of the specified item</constant>
 *	 <constant name="RIGHT">Searches for an item to the right of the specified item</constant>
 *   <desc>
 *    wxListNext is ported as a separate JavaScript class. See @wxListCtrl#getNextItem.
 *   </desc>
 *  </type>
 *  <type name="wxListAlign">
 *	 <constant name="DEFAULT" />
 *	 <constant name="ALIGN_LEFT" />
 *	 <constant name="ALIGN_TOP" />
 *	 <constant name="ALIGN_SNAP_TO_GRID" />
 *   <desc>
 *    wxListAlign is ported as a separate JavaScript class.
 *   </desc>
 *  </type>
 *  <type name="wxListColumnFormat">
 *	 <constant name="LEFT" />
 *	 <constant name="RIGHT" />
 *	 <constant name="CENTRE" />
 *	 <constant name="CENTER" />
 *   <desc>
 *    wxListColumnFormat is ported as a separate JavaScript class.
 *   </desc>
 *  </type>
 *  <type name="wxListRect">
 *	 <constant name="BOUNDS" />
 *	 <constant name="ICON" />
 *	 <constant name="LABEL" />
 *   <desc>
 *    wxListRect is ported as a separate JavaScript class.
 *   </desc>
 *  </type>
 *  <type name="wxListFind">
 *	 <constant name="UP" />
 *	 <constant name="DOWN" />
 *	 <constant name="LEFT" />
 *	 <constant name="RIGHT" />
 *   <desc>
 *    wxListFind is ported as a separate JavaScript class.
 *   </desc>
 *  </type>
 *  <type name="Styles">
 *   <constant name="LIST">  
 *    Multicolumn list view, with optional small icons. Columns are computed automatically, 
 *    i.e. you don't set columns as in REPORT. In other words, the list wraps, unlike a 
 *    wxListBox.  </constant>
 *   <constant name="REPORT">
 *    Single or multicolumn report view, with optional header.    </constant>
 *   <constant name="VIRTUAL">
 *    Virtual control, may only be used with REPORT    </constant>
 *   <constant name="ICON">
 *    Large icon view, with optional labels.    </constant>
 *   <constant name="SMALL_ICON">
 *    Small icon view, with optional labels.    </constant>
 *   <constant name="ALIGN_TOP"> 
 *    Icons align to the top. Win32 default, Win32 only.    </constant>
 *   <constant name="ALIGN_LEFT"> 
 *   Icons align to the left.    </constant>
 *   <constant name="AUTOARRANGE">
 *   Icons arrange themselves. Win32 only.    </constant>
 *   <constant name="USER_TEXT">
 *   The application provides label text on demand, except for column headers. Win32 only.    </constant>
 *   <constant name="EDIT_LABELS"> 
 *   Labels are editable: the application will be notified when editing starts.    </constant>
 *   <constant name="NO_HEADER"> 
 *   No header in report mode. Win32 only.    </constant>
 *   <constant name="SINGLE_SEL">
 *   Single selection.    </constant>
 *   <constant name="SORT_ASCENDING">
 *   Sort in ascending order (must still supply a comparison callback in SortItems).    </constant>
 *   <constant name="SORT_DESCENDING">
 *   Sort in descending order (must still supply a comparison callback in SortItems).    </constant>
 *   <constant name="HRULES"> 
 *   Draws light horizontal rules between rows in report mode.    </constant>
 *   <constant name="VRULES">
 *   Draws light vertical rules between columns in report mode.    </constant>
 *  </type>
 *  <type name="Autosize">
 *   <constant name="AUTOSIZE">
 *  Resize the column to the length of its longest item  </constant>
 *   <constant name="AUTOSIZE_USEHEADER">
 *  Resize the column to the length of the header (Win32) or 80 pixels (other platforms).  </constant>
 *   <desc>These constants can be used with @wxListCtrl#setColumnWidth.</desc>
 *  </type>
 *  <type name="ImageList Constants">
 *   <constant name="NORMAL "> 
 *   The normal (large icon) image list.    </constant>
 *   <constant name="SMALL  ">
 *   The small icon image list.    </constant>
 *   <constant name="STATE  ">
 *   The user-defined state image list (unimplemented).    </constant>
 *  </type>
 * </constants>
 */
WXJS_BEGIN_CONSTANT_MAP(ListCtrl)
    WXJS_CONSTANT(wxLC_, LIST)
    WXJS_CONSTANT(wxLC_, REPORT)
    WXJS_CONSTANT(wxLC_, VIRTUAL)
    WXJS_CONSTANT(wxLC_, ICON)
    WXJS_CONSTANT(wxLC_, SMALL_ICON)
    WXJS_CONSTANT(wxLC_, ALIGN_TOP)
    WXJS_CONSTANT(wxLC_, ALIGN_LEFT)
    WXJS_CONSTANT(wxLC_, AUTOARRANGE)
    WXJS_CONSTANT(wxLC_, USER_TEXT)
    WXJS_CONSTANT(wxLC_, EDIT_LABELS)
    WXJS_CONSTANT(wxLC_, NO_HEADER)
    WXJS_CONSTANT(wxLC_, SINGLE_SEL)
    WXJS_CONSTANT(wxLC_, SORT_ASCENDING)
    WXJS_CONSTANT(wxLC_, SORT_DESCENDING)
    WXJS_CONSTANT(wxLC_, HRULES)
    WXJS_CONSTANT(wxLIST_, AUTOSIZE)
    WXJS_CONSTANT(wxLIST_, AUTOSIZE_USEHEADER)
    WXJS_CONSTANT(wxIMAGE_LIST_, NORMAL)
    WXJS_CONSTANT(wxIMAGE_LIST_, SMALL)
    WXJS_CONSTANT(wxIMAGE_LIST_, STATE)
WXJS_END_CONSTANT_MAP()

void ListCtrl::InitClass(JSContext* cx,
                         JSObject* obj,
                         JSObject* WXUNUSED(proto))
{
  ListCtrlEventHandler::InitConnectEventMap();

  JSConstDoubleSpec wxListMaskMap[] = 
  {
    WXJS_CONSTANT(wxLIST_MASK_, STATE)
    WXJS_CONSTANT(wxLIST_MASK_, TEXT)
    WXJS_CONSTANT(wxLIST_MASK_, IMAGE)
    WXJS_CONSTANT(wxLIST_MASK_, DATA)
    WXJS_CONSTANT(wxLIST_SET_, ITEM)
    WXJS_CONSTANT(wxLIST_MASK_, WIDTH)
    WXJS_CONSTANT(wxLIST_MASK_, FORMAT)
    { 0 }
  };
  JSObject *constObj = JS_DefineObject(cx, obj, "wxListMask", 
    					               NULL, NULL,
						               JSPROP_READONLY | JSPROP_PERMANENT);
  JS_DefineConstDoubles(cx, constObj, wxListMaskMap);

  JSConstDoubleSpec wxListStateMap[] = 
  {
    WXJS_CONSTANT(wxLIST_STATE_, DONTCARE)
    WXJS_CONSTANT(wxLIST_STATE_, DROPHILITED)
    WXJS_CONSTANT(wxLIST_STATE_, FOCUSED)
    WXJS_CONSTANT(wxLIST_STATE_, SELECTED)
    WXJS_CONSTANT(wxLIST_STATE_, CUT)
    { 0 }
  };
  constObj = JS_DefineObject(cx, obj, "wxListState", 
						     NULL, NULL,
						     JSPROP_READONLY | JSPROP_PERMANENT);
  JS_DefineConstDoubles(cx, constObj, wxListStateMap);

  JSConstDoubleSpec wxListNextMap[] = 
  {
    WXJS_CONSTANT(wxLIST_NEXT_, ABOVE)
    WXJS_CONSTANT(wxLIST_NEXT_, ALL)
    WXJS_CONSTANT(wxLIST_NEXT_, BELOW)
    WXJS_CONSTANT(wxLIST_NEXT_, LEFT)
    WXJS_CONSTANT(wxLIST_NEXT_, RIGHT)
    { 0 }
  };
  constObj = JS_DefineObject(cx, obj, "wxListNext", 
						     NULL, NULL,
						     JSPROP_READONLY | JSPROP_PERMANENT);
  JS_DefineConstDoubles(cx, constObj, wxListNextMap);
  
  JSConstDoubleSpec wxListAlignMap[] = 
  {
      WXJS_CONSTANT(wxLIST_ALIGN_, DEFAULT)
      WXJS_CONSTANT(wxLIST_ALIGN_, LEFT)
      WXJS_CONSTANT(wxLIST_ALIGN_, TOP)
      WXJS_CONSTANT(wxLIST_ALIGN_, SNAP_TO_GRID)
    { 0 }
  };
  constObj = JS_DefineObject(cx, obj, "wxListAlign", 
						     NULL, NULL,
						     JSPROP_READONLY | JSPROP_PERMANENT);
  JS_DefineConstDoubles(cx, constObj, wxListAlignMap);

  JSConstDoubleSpec wxListColumnFormatMap[] = 
  {
    WXJS_CONSTANT(wxLIST_FORMAT_, LEFT)
    WXJS_CONSTANT(wxLIST_FORMAT_, RIGHT)
    WXJS_CONSTANT(wxLIST_FORMAT_, CENTRE)
    WXJS_CONSTANT(wxLIST_FORMAT_, CENTER)
    { 0 }
  };
  constObj = JS_DefineObject(cx, obj, "wxListColumnFormat", 
						     NULL, NULL,
						     JSPROP_READONLY | JSPROP_PERMANENT);
  JS_DefineConstDoubles(cx, constObj, wxListColumnFormatMap);

  JSConstDoubleSpec wxListRectMap[] = 
  {
    WXJS_CONSTANT(wxLIST_RECT_, BOUNDS)
    WXJS_CONSTANT(wxLIST_RECT_, ICON)
    WXJS_CONSTANT(wxLIST_RECT_, LABEL)
    { 0 }
  };
  constObj = JS_DefineObject(cx, obj, "wxListRect", 
 						     NULL, NULL,
						     JSPROP_READONLY | JSPROP_PERMANENT);
  JS_DefineConstDoubles(cx, constObj, wxListRectMap);

  JSConstDoubleSpec wxListFindMap[] = 
  {
    WXJS_CONSTANT(wxLIST_FIND_, UP)
    WXJS_CONSTANT(wxLIST_FIND_, DOWN)
    WXJS_CONSTANT(wxLIST_FIND_, LEFT)
    WXJS_CONSTANT(wxLIST_FIND_, RIGHT)
    { 0 }
  };
  constObj = JS_DefineObject(cx, obj, "wxListFind", 
						     NULL, NULL,
						     JSPROP_READONLY | JSPROP_PERMANENT);
  JS_DefineConstDoubles(cx, constObj, wxListFindMap);
}


/***
 * <properties>
 *  <property name="columnCount" type="Integer" readonly="Y"> 
 *   Gets the number of columns.
 *  </property>
 *  <property name="countPerPage" type="Integer" readonly="Y">
 *   Gets the number of items that can fit vertically in the visible area of the list control 
 *   (list or report view) or the total number of items in the list control 
 *   (icon or small icon view).
 *  </property>
 *  <property name="editControl" type="@wxTextCtrl" readonly="Y">
 *   Gets the textcontrol used for editing labels.
 *  </property>
 *  <property name="virtual" type="Boolean" readonly="Y">
 *   Returns true when the list control is virtual.
 *  </property>
 *  <property name="itemCount" type="Integer"> 
 *   Gets/Sets the number of items. Setting the number of items is only allowed
 *   with virtual list controls.
 *  </property>
 *  <property name="onGetItemAttr" type="Function">
 *   Assign a function that returns a @wxListItemAttr object.
 *   The function gets one argument: the index of the item.
 *   This can only be used in virtual list controls.
 *  </property>
 *  <property name="onGetItemImage" type="Function">
 *   Assign a function that returns the index of the image.
 *   The function gets one argument: the index of the item.
 *   This can only be used in virtual list controls.
 *  </property>
 *  <property name="onGetItemText" type="Function">
 *   Assign a function that returns a String. This String is used as text of the item.
 *   The function gets two arguments: the index of the item and the index of the column.
 *   You <b>must</b> set this property for virtual list controls.
 *  </property>
 *  <property name="selectedItemCount" type="Integer" readonly="Y"> 
 *   Gets the number of selected items.
 *  </property>
 *  <property name="textColour" type="@wxColour">
 *   Gets/Sets the text colour of the list control.
 *  </property>
 *  <property name="topItem" type="Integer" readonly="Y">
 *   Gets the index of the topmost visible item in report view.
 *  </property>
 *  <property name="windowStyleFlag" type="Integer"> 
 *   Gets/Sets the window style flag.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(ListCtrl)
    WXJS_READONLY_PROPERTY(P_COLUMN_COUNT, "columnCount")
    WXJS_READONLY_PROPERTY(P_COUNT_PER_PAGE, "countPerPage")
    WXJS_READONLY_PROPERTY(P_EDIT_CONTROL, "editControl")
    WXJS_PROPERTY(P_ITEM_COUNT, "itemCount")
    WXJS_READONLY_PROPERTY(P_SELECTED_ITEM_COUNT, "selectedItemCount")
    WXJS_PROPERTY(P_TEXT_COLOUR, "textColour")
    WXJS_READONLY_PROPERTY(P_TOP_ITEM, "topItem")
    WXJS_PROPERTY(P_WINDOW_STYLE, "windowStyleFlag")
    WXJS_READONLY_PROPERTY(P_IS_VIRTUAL, "virtual")
WXJS_END_PROPERTY_MAP()

wxString ListCtrl::OnGetItemText(long item, long column) const
{
  JavaScriptClientData *data 
       = dynamic_cast<JavaScriptClientData*>(GetClientObject());
  JSContext *cx = data->GetContext();
  JSObject *obj = data->GetObject();

  jsval fval;
  if ( GetFunctionProperty(cx, obj, "onGetItemText", &fval) == JS_FALSE )
	return wxEmptyString;

  jsval rval;
  jsval argv[] = { ToJS(cx, item), ToJS(cx, column) };
  if ( JS_CallFunctionValue(cx, obj, fval, 2, argv, &rval) == JS_TRUE )
  {
      wxString text;
      FromJS(cx, rval, text);
      return text;
  }
  else
  {
      if ( JS_IsExceptionPending(cx) )
      {
          JS_ReportPendingException(cx);
      }
  }
  return wxEmptyString;
}

int ListCtrl::OnGetItemImage(long item) const
{
  JavaScriptClientData *data 
       = dynamic_cast<JavaScriptClientData*>(GetClientObject());
  JSContext *cx = data->GetContext();
  JSObject *obj = data->GetObject();

  jsval fval;
  if ( GetFunctionProperty(cx, obj, "onGetItemImage", &fval) == JS_FALSE )
      return -1;

    jsval rval;
    jsval argv[] = { ToJS(cx, item) };
    if ( JS_CallFunctionValue(cx, obj, fval, 1, argv, &rval) == JS_TRUE )
    {
        int img; 
        if ( FromJS(cx, rval, img) )
            return img;
        else
            return -1;
    }
    else
    {
        if ( JS_IsExceptionPending(cx) )
        {
            JS_ReportPendingException(cx);
        }
    }
    return -1;
}

wxListItemAttr *ListCtrl::OnGetItemAttr(long item) const
{
  JavaScriptClientData *data 
       = dynamic_cast<JavaScriptClientData*>(GetClientObject());
  JSContext *cx = data->GetContext();
  JSObject *obj = data->GetObject();

  jsval fval;
  if ( GetFunctionProperty(cx, obj, "onGetItemText", &fval) == JS_FALSE )
      return NULL;

  jsval rval;
  jsval argv[] = { ToJS(cx, item) };
  if ( JS_CallFunctionValue(cx, obj, fval, 1, argv, &rval) == JS_TRUE )
  {
      wxListItemAttr *attr = ListItemAttr::GetPrivate(cx, rval);
      return attr;
  }
  else
  {
      if ( JS_IsExceptionPending(cx) )
      {
          JS_ReportPendingException(cx);
      }
  }
  return NULL;
}

bool ListCtrl::GetProperty(wxListCtrl *p,
                           JSContext *cx,
                           JSObject *obj,
                           int id,
                           jsval *vp)
{
    switch (id)
    {
    case P_COLUMN_COUNT:
        *vp = ToJS(cx, p->GetColumnCount());
        break;
    case P_COUNT_PER_PAGE:
        *vp = ToJS(cx, p->GetCountPerPage());
        break;
    case P_EDIT_CONTROL:
        #ifdef __WXMSW__
        	*vp = TextCtrl::CreateObject(cx, p->GetEditControl(), obj);
        #endif
        break;
    case P_ITEM_COUNT:
        *vp = ToJS(cx, p->GetColumnCount());
        break;
    case P_SELECTED_ITEM_COUNT:
        *vp = ToJS(cx, p->GetSelectedItemCount());
        break;
    case P_TEXT_COLOUR:
        *vp = Colour::CreateObject(cx, new wxColour(p->GetTextColour()));
        break;
    case P_TOP_ITEM:
        *vp = ToJS(cx, p->GetTopItem());
        break;
    case P_WINDOW_STYLE:
        *vp = ToJS(cx, p->GetWindowStyleFlag());
        break;
    case P_IS_VIRTUAL:
        *vp = ToJS(cx, p->IsVirtual());
        break;
    }
    return true;
}

bool ListCtrl::SetProperty(wxListCtrl *p,
                           JSContext *cx,
                           JSObject* WXUNUSED(obj),
                           int id,
                           jsval *vp)
{
    switch (id)
    {
    case P_ITEM_COUNT:
        {
            long count;
            if (    p->IsVirtual()
                 && FromJS(cx, *vp, count) )
                p->SetItemCount(count);
            break;
        }
    case P_TEXT_COLOUR:
        {
            wxColour *colour = Colour::GetPrivate(cx, *vp);
            if ( colour != NULL )
                p->SetTextColour(*colour);
            break;
        }
    case P_WINDOW_STYLE:
        {
            long style;
            if  ( FromJS(cx, *vp, style) )
                p->SetWindowStyleFlag(style);
            break;
        }
    }
    return true;
}

bool ListCtrl::AddProperty(wxListCtrl *p, 
                           JSContext* WXUNUSED(cx), 
                           JSObject* WXUNUSED(obj), 
                           const wxString &prop, 
                           jsval* WXUNUSED(vp))
{
    if ( WindowEventHandler::ConnectEvent(p, prop, true) )
        return true;
    
    ListCtrlEventHandler::ConnectEvent(p, prop, true);

    return true;
}


bool ListCtrl::DeleteProperty(wxListCtrl *p, 
                              JSContext* WXUNUSED(cx), 
                              JSObject* WXUNUSED(obj), 
                              const wxString &prop)
{
  if ( WindowEventHandler::ConnectEvent(p, prop, false) )
    return true;
  
  ListCtrlEventHandler::ConnectEvent(p, prop, false);
  return true;
}


/***
 * <ctor>
 *  <function />
 *  <function>
 *   <arg name="Parent" type="@wxWindow">
 *    The parent of wxListCtrl. Can't be null.
 *   </arg>
 *   <arg name="Id" type="Integer">
 *    An window identifier. Use -1 when you don't need it.
 *   </arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *    The position of the ListCtrl control on the given parent.
 *   </arg>
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize">
 *    The size of the ListCtrl control.
 *   </arg>
 *   <arg name="Style" type="Integer" default="wxListCtrl.ICON">
 *    The wxListCtrl style.
 *   </arg>
 *   <arg name="Validator" type="@wxValidator" default="null">
 *    Validator.
 *   </arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxListCtrl object.
 *  </desc>
 * </ctor>
 */
wxListCtrl* ListCtrl::Construct(JSContext *cx,
                                JSObject *obj,
                                uintN argc,
                                jsval *argv,
                                bool WXUNUSED(constructing))
{
  wxListCtrl *p = new ListCtrl();
  SetPrivate(cx, obj, p);
  if ( argc > 0 )
  {
    jsval rval;
    if ( ! create(cx, obj, argc, argv, &rval) )
      return NULL;
  }
  return p;
}

WXJS_BEGIN_METHOD_MAP(ListCtrl)
    WXJS_METHOD("create", create, 2)
    WXJS_METHOD("getColumn", getColumn, 2)
    WXJS_METHOD("setColumn", setColumn, 2)
    WXJS_METHOD("getColumnWidth", getColumnWidth, 1)
    WXJS_METHOD("setColumnWidth", setColumnWidth, 2)
    WXJS_METHOD("getItem", getItem, 1)
    WXJS_METHOD("setItem", setItem, 1)
    WXJS_METHOD("getItemState", getItemState, 2)
    WXJS_METHOD("setItemState", setItemState, 3)
    WXJS_METHOD("setItemImage", setItemImage, 3)
    WXJS_METHOD("getItemText", getItemText, 1)
    WXJS_METHOD("setItemText", setItemText, 2)
    WXJS_METHOD("getItemData", getItemData, 1)
    WXJS_METHOD("setItemData", setItemData, 2)
    WXJS_METHOD("getItemRect", getItemRect, 2)
    WXJS_METHOD("getItemPosition", getItemPosition, 1)
    WXJS_METHOD("setItemPosition", setItemPosition, 2)
    WXJS_METHOD("getItemSpacing", getItemSpacing, 1)
    WXJS_METHOD("setSingleStyle", setSingleStyle, 1)
    WXJS_METHOD("getNextItem", getNextItem, 1)
    WXJS_METHOD("getImageList", getImageList, 1)
    WXJS_METHOD("setImageList", setImageList, 2)
    WXJS_METHOD("refreshItem", refreshItem, 1)
    WXJS_METHOD("refreshItems", refreshItems, 2)
    WXJS_METHOD("arrange", arrange, 0)
    WXJS_METHOD("deleteItem", deleteItem, 1)
    WXJS_METHOD("deleteAllItems", deleteAllItems, 0)
    WXJS_METHOD("deleteColumn", deleteColumn, 1)
    WXJS_METHOD("deleteAllColumns", deleteAllColumns, 0)
    WXJS_METHOD("clearAll", clearAll, 0)
    WXJS_METHOD("insertItem", insertItem, 1)
    WXJS_METHOD("insertColumn", insertColumn, 2)
    WXJS_METHOD("editLabel", editLabel, 1)
    WXJS_METHOD("endEditLabel", endEditLabel, 1)
    WXJS_METHOD("ensureVisible", ensureVisible, 1)
    WXJS_METHOD("findItem", findItem, 2)
    WXJS_METHOD("hitTest", hitTest, 1)
    WXJS_METHOD("scrollList", scrollList, 2)
    WXJS_METHOD("sortItems", sortItems, 1)
WXJS_END_METHOD_MAP()

/***
 * <method name="create">
 *  <function returns="Boolean">
 *   <arg name="Parent" type="@wxWindow">
 *    The parent of wxListCtrl. Can't be null.
 *   </arg>
 *   <arg name="Id" type="Integer">
 *    An window identifier. Use -1 when you don't need it.
 *   </arg>
 *   <arg name="Position" type="@wxPoint" default="wxDefaultPosition">
 *    The position of the ListCtrl control on the given parent.
 *   </arg>
 *   <arg name="Size" type="@wxSize" default="wxDefaultSize">
 *    The size of the ListCtrl control.
 *   </arg>
 *   <arg name="Style" type="Integer" default="wxListCtrl.ICON">
 *    The wxListCtrl style.
 *   </arg>
 *   <arg name="Validator" type="@wxValidator" default="null">
 *    Validator.
 *   </arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxListCtrl object.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::create(JSContext *cx,
                        JSObject *obj,
                        uintN argc,
                        jsval *argv,
                        jsval *rval)
{
  wxListCtrl *p = GetPrivate(cx, obj);
  *rval = JSVAL_FALSE;

  if ( argc > 6 )
      argc = 6;

  int style = wxLC_ICON;
  const wxPoint *pt = &wxDefaultPosition;
  const wxSize *size = &wxDefaultSize;
  const wxValidator *val = &wxDefaultValidator;

  switch(argc)
  {
  case 6:
      val = Validator::GetPrivate(cx, argv[5]);
      if ( val == NULL )
      {
        JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 6, "wxValidator");
        return JS_FALSE;
      }
  case 5:
      if ( ! FromJS(cx, argv[4], style) )
      {
        JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 5, "Integer");
        return JS_FALSE;
      }
  case 4:
	size = Size::GetPrivate(cx, argv[3]);
	if ( size == NULL )
    {
      JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 4, "wxSize");
      return JS_FALSE;
    }
	// Fall through
case 3:
	pt = Point::GetPrivate(cx, argv[2]);
	if ( pt == NULL )
    {
      JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 3, "wxPoint");
      return JS_FALSE;
    }
	// Fall through
  default:
      
      int id;
      if ( ! FromJS(cx, argv[1], id) )
      {
        JS_ReportError(cx, WXJS_INVALID_ARG_TYPE, 2, "Integer");
        return JS_FALSE;
      }

      wxWindow *parent = Window::GetPrivate(cx, argv[0]);
      if ( parent == NULL )
      {
          JS_ReportError(cx, WXJS_NO_PARENT_ERROR, GetClass()->name);
          return JS_FALSE;
      }
      JavaScriptClientData *clntParent =
            dynamic_cast<JavaScriptClientData *>(parent->GetClientObject());
      if ( clntParent == NULL )
      {
          JS_ReportError(cx, WXJS_NO_PARENT_ERROR, GetClass()->name);
          return JS_FALSE;
      }
      JS_SetParent(cx, obj, clntParent->GetObject());

      if ( p->Create(parent, id, *pt, *size, style, *val) )
      {
        *rval = JSVAL_TRUE;
        p->SetClientObject(new JavaScriptClientData(cx, obj, true, false));
      }
  }
  return JS_TRUE;
}

/***
 * <method name="getColumn">
 *  <function returns="@wxListItem">
 *   <arg name="Col" type="Integer">
 *    The column number
 *   </arg>
 *   <arg name="Item" type="Integer">
 *    The item index
 *   </arg>
 *  </function>
 *  <function returns="Boolean">
 *   <arg name="Col" type="Integer">
 *    The column number
 *   </arg>
 *   <arg name="ListItem" type="@wxListItem">
 *    The item info. Make sure that the id property is set.
 *   </arg>
 *  </function>
 *  <desc>
 *   Gets the column information. The first form returns a @wxListItem object or
 *   undefined when the column is not found. The second form returns true when the
 *   column is found and puts the column information in the second argument.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::getColumn(JSContext* cx,
                           JSObject* obj,
                           uintN WXUNUSED(argc),
                           jsval* argv, 
                           jsval* rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int col;
    if ( FromJS(cx, argv[0], col) )
    {
        if ( ListItem::HasPrototype(cx, argv[1]) )
        {
            wxListItem *item = ListItem::GetPrivate(cx, argv[1], false);
            *rval = ToJS(cx, p->GetColumn(col, *item));
        }
        else
        {
            int idx;
            if ( FromJS(cx, argv[1], idx) )
            {
                wxListItem *item = new wxListItem();
                item->SetId(idx);
                if ( p->GetColumn(col, *item) )
                {
                    *rval = ListItem::CreateObject(cx, item, obj);
                }
                else
                {
                    delete item;
                    *rval = JSVAL_VOID;
                }
            }
            else
            {
                return JS_FALSE;
            }
        }
    }
    else
    {
        return JS_FALSE;
    }

    return JS_TRUE;
}

/***
 * <method name="setColumn">
 *  <function returns="@wxListItem">
 *   <arg name="Col" type="Integer">
 *    The column number
 *   </arg>
 *   <arg name="Item" type="Integer">
 *    The item index
 *   </arg>
 *  </function>
 *  <function returns="Boolean">
 *   <arg name="Col" type="Integer">
 *    The column number
 *   </arg>
 *   <arg name="ListItem" type="@wxListItem">
 *    The item info. Make sure that the id property is set.
 *   </arg>
 *  </function>
 *  <desc>
 *   Sets the column information. False is returned when the column doesn't exist.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::setColumn(JSContext *cx,
                           JSObject *obj,
                           uintN WXUNUSED(argc),
                           jsval *argv,
                           jsval* WXUNUSED(rval))
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int col;

    if ( ! FromJS(cx, argv[0], col) )
        return JS_FALSE;

    wxListItem *item = ListItem::GetPrivate(cx, argv[1]);
    if ( item == NULL )
        return JS_FALSE;

    p->SetColumn(col, *item);

    return JS_TRUE;
}

/***
 * <method name="getColumnWidth">
 *  <function returns="Integer">
 *   <arg name="Col" type="Integer">
 *    The column number
 *   </arg>
 *  </function>
 *  <desc>
 *   Returns the width of the column.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::getColumnWidth(JSContext *cx,
                                JSObject *obj,
                                uintN WXUNUSED(argc),
                                jsval *argv,
                                jsval *rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int col;
    if ( FromJS(cx, argv[0], col) )
    {
        *rval = ToJS(cx, p->GetColumnWidth(col));
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="setColumnWidth">
 *  <function returns="Boolean">
 *   <arg name="Col" type="Integer">
 *    The column number. In small or normal icon view, col must be -1, and the column 
 *    width is set for all columns.
 *   </arg>
 *   <arg name="Width" type="Integer">
 *    Width can be the width in pixels, AUTOSIZE or AUTOSIZE_USEHEADER.
 *    AUTOSIZE will resize the column to the length of the longest item. 
 *    AUTOSIZE_USEHEADER will resize the column to the length of the header 
 *    (on windows) or on 80 pixels (other platforms).
 *   </arg>
 *  </function>
 *  <desc>
 *   Sets the columnwidth. Returns true on success, false on failure.
 *   See @wxListCtrl#autosize.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::setColumnWidth(JSContext *cx,
                                JSObject *obj,
                                uintN WXUNUSED(argc),
                                jsval *argv,
                                jsval *rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int col;
    if ( ! FromJS(cx, argv[0], col) )
        return JS_FALSE;

    int width;
    if ( ! FromJS(cx, argv[1], width) )
        return JS_FALSE;

    *rval = ToJS(cx, p->SetColumnWidth(col, width));
    return JS_TRUE;
}

/***
 * <method>
 *  <function returns="@wxListItem">
 *   <arg name="Item" type="Integer">
 *    The item index
 *   </arg>
 *  </function>
 *  <function returns="Boolean">
 *   <arg name="ListItem" type="@wxListItem">
 *    The item info. Make sure that the id property is set.
 *   </arg>
 *  </function>
 *  <desc>
 *   Gets the item information. The first form returns a @wxListItem object or
 *   undefined when the item is not found. The second form returns true when the
 *   item is found and puts the item information in the argument.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::getItem(JSContext *cx,
                         JSObject *obj,
                         uintN WXUNUSED(argc),
                         jsval *argv,
                         jsval *rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    if ( ListItem::HasPrototype(cx, argv[0]) )
    {
        wxListItem *item = ListItem::GetPrivate(cx, argv[0], false);
        *rval = ToJS(cx, p->GetItem(*item));
    }
    else
    {
        int idx;
        if ( FromJS(cx, argv[0], idx) )
        {
            wxListItem *item = new wxListItem();
            item->SetId(idx);
            if ( p->GetItem(*item) )
            {
                *rval = ListItem::CreateObject(cx, item, obj);
            }
            else
            {
                delete item;
                *rval = JSVAL_VOID;
            }
        }
        else
            return JS_FALSE;
    }
    
    return JS_TRUE;
}

/***
 * <method name="setItem">
 *  <function returns="Boolean">
 *   <arg name="ListItem" type="@wxListItem">
 *    The item info
 *   </arg>
 *  </function>
 *  <function>
 *   <arg name="Item" type="Integer">
 *    The item index
 *   </arg>
 *   <arg name="Col" type="Integer">
 *    The column index
 *   </arg>
 *   <arg name="Label" type="String">
 *    The text of the item
 *   </arg>
 *   <arg name="ImageId" type="Integer" default="-1">
 *    The zero-base index of an image list.
 *   </arg>
 *  </function>
 *  <desc>
 *   Sets the item information. On success, true is returned.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::setItem(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    if ( ListItem::HasPrototype(cx, argv[0]) )
    {
        wxListItem *item = ListItem::GetPrivate(cx, argv[0], false);
        *rval = ToJS(cx, p->SetItem(*item));
    }
    else
    {
        if ( argc < 3 )
            return JS_FALSE;

        int imageId = -1;
        if (    argc > 3 
             && ! FromJS(cx, argv[3], imageId) )
             return JS_FALSE;

        wxString label;
        FromJS(cx, argv[2], label);

        int idx;
        int col;

        if (   FromJS(cx, argv[1], col)
            && FromJS(cx, argv[0], idx) )
        {
            *rval = ToJS(cx, p->SetItem(idx, col, label, imageId));
        }
        else
            return JS_FALSE;
    }
    return JS_TRUE;
}

/***
 * <method name="getItemState">
 *  <function returns="Integer">
 *   <arg name="Item" type="Integer">
 *    The item index
 *   </arg>
 *   <arg name="StateMask" type="Integer">
 *    Indicates which state flags are valid.
 *   </arg>
 *  </function>
 *  <desc>
 *   Gets the item state. See @wxListCtrl#wxListState.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::getItemState(JSContext *cx,
                              JSObject *obj,
                              uintN WXUNUSED(argc),
                              jsval *argv,
                              jsval *rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    long item;
    int stateMask;

    if (    FromJS(cx, argv[0], item) 
         && FromJS(cx, argv[1], stateMask) )
    {
        *rval = ToJS(cx, p->GetItemState(item, stateMask));
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="setItemState">
 *  <function returns="Boolean">
 *   <arg name="Item" type="Integer">
 *    The item index.
 *   </arg>
 *   <arg name="State" type="Integer">
 *    The new state.
 *   </arg>
 *   <arg name="StateMask" type="Integer">
 *    Indicates which state flags are valid.
 *   </arg>
 *  </function>
 *  <desc>
 *   Sets the new state of an item. See @wxListCtrl#wxListState.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::setItemState(JSContext *cx,
                              JSObject *obj,
                              uintN WXUNUSED(argc),
                              jsval *argv,
                              jsval *rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    long item;
    long state;
    long stateMask;

    if (    FromJS(cx, argv[0], item)
         && FromJS(cx, argv[1], state)
         && FromJS(cx, argv[2], stateMask) )
    {
        *rval = ToJS(cx, p->SetItemState(item, state, stateMask));
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="setItemImage">
 *  <function returns="Boolean">
 *   <arg name="Item" type="Integer">
 *    The item index.
 *   </arg>
 *   <arg name="Image" type="Integer">
 *    Index of the image to show for unselected items.
 *   </arg>
 *   <arg name="SelImage" type="Integer">
 *    Index of the image to show for selected items.
 *   </arg>
 *  </function>
 *  <desc>
 *   Sets the unselected and selected images associated with the item. 
 *   The images are indices into the image list associated with the list control.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::setItemImage(JSContext *cx,
                              JSObject *obj,
                              uintN WXUNUSED(argc),
                              jsval *argv,
                              jsval *rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    long item;
    int image;
    int selImage;

    if (    FromJS(cx, argv[0], item)
         && FromJS(cx, argv[1], image)
         && FromJS(cx, argv[2], selImage) )
    {
        *rval = ToJS(cx, p->SetItemImage(item, image, selImage));
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="getItemText">
 *  <function returns="String">
 *   <arg name="Item" type="Integer">
 *    The item index.
 *   </arg>
 *  </function>
 *  <desc>
 *   Returns the text of the item.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::getItemText(JSContext *cx,
                             JSObject *obj,
                             uintN WXUNUSED(argc),
                             jsval *argv,
                             jsval *rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    long item;

    if (    FromJS(cx, argv[0], item) )
    {
        *rval = ToJS(cx, p->GetItemText(item));
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="setItemText">
 *  <function>
 *   <arg name="Item" type="Integer">
 *    The item index.
 *   </arg>
 *   <arg name="Text" type="String">
 *    The new text.
 *   </arg>
 *  </function>
 *  <desc>
 *   Sets the text of an item.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::setItemText(JSContext *cx,
                             JSObject *obj,
                             uintN WXUNUSED(argc),
                             jsval *argv,
                             jsval* WXUNUSED(rval))
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    long item;
    wxString text;

    if (    FromJS(cx, argv[0], item)
         && FromJS(cx, argv[1], text) )
    {
        p->SetItemText(item, text);
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="getItemData">
 *  <function returns="Any">
 *   <arg name="Item" type="Integer">
 *    The item index.
 *   </arg>
 *  </function>
 *  <desc>
 *   Returns the associated data of the item. The type of the data can be any type:
 *   integer, long, object, ...
 *  </desc>
 * </method>
 */
JSBool ListCtrl::getItemData(JSContext *cx,
                             JSObject *obj,
                             uintN WXUNUSED(argc),
                             jsval *argv,
                             jsval *rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    long item;
    if ( FromJS(cx, argv[0], item) )
    {
        ListObjectData *data = (ListObjectData*) p->GetItemData(item);
        *rval = ( data == NULL ) ? JSVAL_VOID : data->GetJSVal();
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="setItemData">
 *  <function returns="Boolean">
 *   <arg name="Item" type="Integer">
 *    The item index.
 *   </arg>
 *   <arg name="Data" type="Any">
 *    The associated data.
 *   </arg>
 *  </function>
 *  <desc>
 *   Sets the associated data of an item. The type of the data can 
 *   be any type: integer, long, object, ...
 *   When the listcontrol is sortable, the item data must be set.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::setItemData(JSContext *cx,
                             JSObject *obj,
                             uintN WXUNUSED(argc),
                             jsval *argv,
                             jsval *rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    long item;

    if ( FromJS(cx, argv[0], item) )
    {
        ListObjectData *data = (ListObjectData*) p->GetItemData(item);
        if ( data != NULL )
        {
            delete data;
            data = NULL;
        }

        data = new ListObjectData(cx, argv[1]);
        if ( p->SetItemData(item, (long) data) )
        {
            *rval = JSVAL_TRUE;
        }
        else
        {
            *rval = JSVAL_FALSE;
            delete data;
            data = NULL;
        }
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="getItemRect">
 *  <function returns="@wxRect">
 *   <arg name="Item" type="Integer">
 *    The item index.
 *   </arg>
 *   <arg name="Code" type="Integer">
 *    Indicates which rectangle to return.
 *   </arg>
 *  </function>
 *  <desc>
 *   Returns the item rectangle. See @wxListCtrl#wxListRect.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::getItemRect(JSContext *cx,
                             JSObject *obj,
                             uintN WXUNUSED(argc),
                             jsval *argv,
                             jsval *rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    long item;
    int code;

    if (    FromJS(cx, argv[0], item) 
         && FromJS(cx, argv[1], code) )
    {
        wxRect *rect = new wxRect();
        if ( p->GetItemRect(item, *rect, code) )
        {
            *rval = Rect::CreateObject(cx, rect);
        }
        else
        {
            delete rect;
            *rval = JSVAL_VOID;
        }
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="getItemPosition">
 *  <function returns="@wxPoint">
 *   <arg name="Item" type="Integer">
 *    The item index.
 *   </arg>
 *  </function>
 *  <desc>
 *   Returns the position of the item in icon or small icon view.
 *   Returns undefined on failure.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::getItemPosition(JSContext *cx,
                                 JSObject *obj,
                                 uintN WXUNUSED(argc),
                                 jsval *argv,
                                 jsval *rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    long item;

    if ( FromJS(cx, argv[0], item) )
    {
        wxPoint *pt = new wxPoint();
        if ( p->GetItemPosition(item, *pt) )
        {
            *rval = Point::CreateObject(cx, pt);
        }
        else
        {
            delete pt;
            *rval = JSVAL_VOID;
        }
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="setItemPosition">
 *  <function returns="Boolean">
 *   <arg name="Item" type="Integer">
 *    The item index.
 *   </arg>
 *   <arg name="Pos" type="@wxPoint">
 *    The new position
 *   </arg>
 *  </function>
 *  <desc>
 *   Sets the item position in icon or small icon view. Returns true on success.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::setItemPosition(JSContext *cx,
                                 JSObject *obj, 
                                 uintN WXUNUSED(argc), 
                                 jsval *argv, 
                                 jsval *rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    long item;
    if ( FromJS(cx, argv[0], item) )
    {
        wxPoint *pt = Point::GetPrivate(cx, argv[1]);
        if ( pt != NULL )
        {
            *rval = ToJS(cx, p->SetItemPosition(item, *pt));
            return JS_TRUE;
        }
    }

    return JS_FALSE;
}

/***
 * <method name="getItemSpacing">
 *  <function returns="Integer">
 *   <arg name="IsSmall" type="Boolean">
 *    When true, the item spacing for small icon view is returned. Otherwise
 *    the item spacing for large icon view is returned.
 *   </arg>
 *  </function>
 *  <desc>
 *   Returns the item spacing used in icon view.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::getItemSpacing(JSContext *cx, 
                                JSObject *obj, 
                                uintN WXUNUSED(argc),
                                jsval* WXUNUSED(argv), 
                                jsval* rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

#if wxCHECK_VERSION(2,7,0)
    *rval = Size::CreateObject(cx, new wxSize(p->GetItemSpacing()));
#else
    bool isSmall;

    if ( FromJS(cx, argv[0], isSmall) )
    {
        *rval = ToJS(cx, p->GetItemSpacing(isSmall));
    }
#endif
    return JS_TRUE;
}

/***
 * <method name="setSingleStyle">
 *  <function>
 *   <arg name="Style" type="Integer">
 *    A window style.
 *   </arg>
 *   <arg name="Add" type="Boolean" default="true">
 *    When true (= default), the style is added. Otherwise the style is removed.
 *   </arg>
 *  </function>
 *  <desc>
 *   Adds or removes a single window style.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::setSingleStyle(JSContext *cx,
                                JSObject *obj,
                                uintN argc,
                                jsval *argv,
                                jsval* WXUNUSED(rval))
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    long style;

    if ( ! FromJS(cx, argv[0], style) )
        return JS_FALSE;

    bool add = true;
    if (    argc > 1
         && ! FromJS(cx, argv[1], add) )
         return JS_FALSE;

    p->SetSingleStyle(style, add);

    return JS_TRUE;
}

/***
 * <method name="getNextItem">
 *  <function returns="Integer">
 *   <arg name="Item" type="Integer">
 *    The start item. Use -1 to start from the beginning.
 *   </arg>
 *   <arg name="Geometry" type="Integer" default="wxListNext.ALL">
 *    Search direction. Use one of the constants defined in @wxListCtrl#wxListNext.
 *   </arg>
 *   <arg name="State" type="Integer" default="wxListState.DONTCARE">
 *    A state. Use one of the constants defined in @wxListCtrl#wxListState.
 *   </arg>
 *  </function>
 *  <desc>
 *   Searches for an item with the given goemetry or state, starting from item but 
 *   excluding the item itself. If item is -1, the first item that matches the specified
 *   flags will be returned.<br />
 *   The following example iterates all selected items:
 *   <pre><code class="whjs">
 *    var item = -1;
 *    do
 *    {
 *      item = listctrl.getNextItem(item, wxListNext.ALL, wxListState.SELECTED);
 *    } 
 *    while(item != -1);
 *   </code></pre>
 *  </desc>
 * </method>
 */
JSBool ListCtrl::getNextItem(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    if ( argc > 3 )
        argc = 3;

    int state = wxLIST_STATE_DONTCARE;
    int next = wxLIST_NEXT_ALL;

    switch(argc)
    {
    case 3:
        if ( ! FromJS(cx, argv[2], state) )
            return JS_FALSE;
        // Fall through
    case 2:
        if ( ! FromJS(cx, argv[1], next) )
            return JS_FALSE;
        // Fall through
    default:
        long item;
        if ( ! FromJS(cx, argv[0], item) )
            return JS_FALSE;

        *rval = ToJS(cx, p->GetNextItem(item, next, state));
    }

    return JS_TRUE;
}

/***
 * <method name="getImageList">
 *  <function returns="@wxImageList">
 *   <arg name="Which" type="Integer">
 *    Type of the imagelist.
 *   </arg>
 *  </function>
 *  <desc>
 *   Returns the associated imagelist.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::getImageList(JSContext *cx,
                              JSObject *obj,
                              uintN WXUNUSED(argc),
                              jsval *argv,
                              jsval *rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int which;
    if ( FromJS(cx, argv[0], which) )
    {
        ImageList *imgList = dynamic_cast<ImageList *>(p->GetImageList(which));
        JavaScriptClientData *data
          = dynamic_cast<JavaScriptClientData*>(imgList->GetClientObject());
        if ( data != NULL )
        {
          *rval = OBJECT_TO_JSVAL(data->GetObject());
        }
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="setImageList">
 *  <function>
 *   <arg name="ImageList" type="@wxImageList" />
 *   <arg name="Which" type="Integer">
 *    Type of the imagelist.
 *   </arg>
 *  </function>
 *  <desc>
 *   Sets the imagelist associated with the control.  
 *  </desc>
 * </method>
 */
JSBool ListCtrl::setImageList(JSContext *cx,
                              JSObject *obj,
                              uintN WXUNUSED(argc),
                              jsval *argv,
                              jsval* WXUNUSED(rval))
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    ImageList *imgList = ImageList::GetPrivate(cx, argv[0]);
    int which;
    if ( FromJS(cx, argv[1], which) )
    {
        p->AssignImageList(imgList, which);
        JavaScriptClientData *data
          = dynamic_cast<JavaScriptClientData*>(imgList->GetClientObject());
        if ( data != NULL )
        {
          data->Protect(true);
          data->SetOwner(false);
        }
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="refreshItem">
 *  <function>
 *   <arg name="Item" type="Integer">
 *    The item index.
 *   </arg>
 *  </function>
 *  <desc>
 *   Refreshes the item. Only useful for virtual list controls.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::refreshItem(JSContext *cx,
                             JSObject *obj,
                             uintN WXUNUSED(argc),
                             jsval *argv,
                             jsval* WXUNUSED(rval))
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    long item;
    if ( FromJS(cx, argv[0], item) )
    {
        p->RefreshItem(item);
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="refreshItems">
 *  <function>
 *   <arg name="From" type="Integer">
 *    The item index to start from.
 *   </arg>
 *   <arg name="To" type="Integer">
 *    The item index of the last item to refresh.
 *   </arg>
 *  </function>
 *  <desc>
 *   Refreshes a range of items. Only useful for virtual list controls.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::refreshItems(JSContext *cx,
                              JSObject *obj,
                              uintN WXUNUSED(argc),
                              jsval* argv,
                              jsval* WXUNUSED(rval))
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    long start;
    long end;

    if (    FromJS(cx, argv[0], start) 
         && FromJS(cx, argv[0], end) )
    {
        p->RefreshItems(start, end);
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="arrange">
 *  <function returns="Boolean">
 *   <arg name="Align" type="Integer" default="wxListAlign.DEFAULT" />
 *  </function>
 *  <desc>
 *   Arranges the items in icon or small icon view. (Windows only)
 *   See @wxListCtrl#wxListAlign
 *  </desc>
 * </method>
 */
JSBool ListCtrl::arrange(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int flag = wxLIST_ALIGN_DEFAULT;
    if (    argc > 0 
         && !FromJS(cx, argv[0], flag) )
        return JS_FALSE;

    *rval = ToJS(cx, p->Arrange(flag));
    return JS_TRUE;
}

/***
 * <method name="deleteItem">
 *  <function returns="Boolean">
 *   <arg name="Item" type="Integer">
 *    The item index.
 *   </arg>
 *  </function>
 *  <desc>
 *   Deletes the item. Returns true on success.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::deleteItem(JSContext *cx,
                            JSObject *obj,
                            uintN WXUNUSED(argc),
                            jsval *argv,
                            jsval *rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    long item;

    if ( FromJS(cx, argv[0], item) )
    {
        *rval = ToJS(cx, p->DeleteItem(item));
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="deleteAllItems">
 *  <function returns="Boolean" />
 *  <desc>
 *   Deletes all items. Returns true on success.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::deleteAllItems(JSContext *cx,
                                JSObject *obj,
                                uintN WXUNUSED(argc),
                                jsval* WXUNUSED(argv),
                                jsval* rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    *rval = ToJS(cx, p->DeleteAllItems());
    return JS_TRUE;
}

/***
 * <method name="deleteColumn">
 *  <function returns="Boolean">
 *   <arg name="Col" type="Integer">
 *    The column index.
 *   </arg>
 *  </function>
 *  <desc>
 *   Deletes the column. Returns true on success.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::deleteColumn(JSContext *cx,
                              JSObject *obj,
                              uintN WXUNUSED(argc),
                              jsval* argv,
                              jsval *rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    long col;

    if ( FromJS(cx, argv[0], col) )
    {
        *rval = ToJS(cx, p->DeleteColumn(col));
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="deleteAllColumns">
 *  <function returns="Boolean" />
 *  <desc>
 *   Deletes all columns. Returns true on success.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::deleteAllColumns(JSContext *cx,
                                  JSObject *obj,
                                  uintN WXUNUSED(argc), 
                                  jsval* WXUNUSED(argv),
                                  jsval *rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    *rval = ToJS(cx, p->DeleteAllColumns());
    return JS_TRUE;
}

/***
 * <method name="clearAll">
 *  <function />
 *  <desc>
 *   Deletes all items and columns.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::clearAll(JSContext *cx,
                          JSObject *obj,
                          uintN WXUNUSED(argc),
                          jsval* WXUNUSED(argv),
                          jsval* WXUNUSED(rval))
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    p->ClearAll();
    return JS_TRUE;
}

/***
 * <method name="insertColumn">
 *  <function returns="Integer">
 *   <arg name="Col" type="Integer">
 *    A column index
 *   </arg>
 *   <arg name="ListItem" type="@wxListItem">
 *    The item
 *   </arg>
 *  </function>
 *  <function returns="Integer">
 *   <arg name="Col" type="Integer">
 *    A column index
 *   </arg>
 *   <arg name="Heading" type="String">
 *    The name for the header.
 *   </arg>
 *   <arg name="Format" type="Integer" default="wxListFormat.LEFT">
 *    The format of the header. Use one of the constants of @wxListCtrl#wxListColumnFormat.
 *   </arg>
 *   <arg name="Width" type="Integer" default="-1">
 *    The width of the column.
 *   </arg>
 *  </function>
 *  <desc>
 *   Inserts a column. On success, the index of the new column is returned. On failure, -1 is returned.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::insertColumn(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    long col;
    if ( ! FromJS(cx, argv[0], col) )
        return JS_FALSE;

    if ( ListItem::HasPrototype(cx, argv[1]) )
    {
        wxListItem *item = ListItem::GetPrivate(cx, argv[1], false);
        *rval = ToJS(cx, p->InsertColumn(col, *item));
    }
    else
    {
        wxString header;
        FromJS(cx, argv[1], header);

        int format = wxLIST_FORMAT_LEFT;
        int width = -1;

        if (      argc == 3 
             && ! FromJS(cx, argv[2], format) )
            return JS_FALSE;

        if (      argc == 4 
             && ! FromJS(cx, argv[3], width) )
            return JS_FALSE;

        *rval = ToJS(cx, p->InsertColumn(col, header, format, width));
    }

    return JS_TRUE;
}

/***
 * <method name="insertItem">
 *  <function returns="Integer">
 *   <arg name="ListItem" type="@wxListItem">
 *    Item information
 *   </arg>
 *  </function>
 *  <function returns="Integer">
 *   <arg name="Item" type="Integer">
 *    The item index.
 *   </arg>
 *   <arg name="Label" type="String">
 *    Label of the item
 *   </arg>
 *   <arg name="ImageIdx" type="Integer" default="-1">
 *    The index of the image in the associated imagelist
 *   </arg>
 *  </function>
 *  <function returns="Integer">
 *   <arg name="Item" type="Integer">
 *    The item index.
 *   </arg>
 *   <arg name="ImageIdx" type="Integer">
 *    The index of the image in the associated imagelist.
 *   </arg>
 *  </function>
 *  <desc>
 *   Inserts an item. On success, the index of the new item is returned. On failure, -1 is returned.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::insertItem(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    if ( ListItem::HasPrototype(cx, argv[0]) )
    {
        wxListItem *item = ListItem::GetPrivate(cx, argv[0], false);
        *rval = ToJS(cx, p->InsertItem(*item));
        return JS_TRUE;
    }
    else if ( argc > 1 )
    {
        long item;
        if ( FromJS(cx, argv[0], item) )
        {
            int imageIdx;
            if ( JSVAL_IS_INT(argv[1]) )
            {
                imageIdx = JSVAL_TO_INT(argv[1]);
                *rval = ToJS(cx, p->InsertItem(item, imageIdx));
                return JS_TRUE;
            }
            else
            {
                wxString label;
                FromJS(cx, argv[1], label);
                if ( argc > 2 )
                {
                    if ( FromJS(cx, argv[2], imageIdx) )
                    {
                        *rval = ToJS(cx, p->InsertItem(item, label, imageIdx));
                        return JS_TRUE;
                    }
                }
                else
                {
                    *rval = ToJS(cx, p->InsertItem(item, label));
                    return JS_TRUE;
                }
            }
        }
    }
    return JS_FALSE;
}

/***
 * <method name="editLabel">
 *  <function returns="@wxTextCtrl">
 *   <arg name="Item" type="Integer">
 *    The item index.
 *   </arg>
 *  </function>
 *  <desc>
 *   Edit the label. The text control is returned (on Windows).
 *  </desc>
 * </method>
 */
JSBool ListCtrl::editLabel(JSContext *cx,
                           JSObject *obj,
                           uintN WXUNUSED(argc), 
                           jsval *argv,
                           jsval *rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    long item;

    if ( FromJS(cx, argv[0], item) )
    {
   		p->EditLabel(item);
    	#ifdef __WXMSW__
	        *rval = TextCtrl::CreateObject(cx, p->GetEditControl());
	    #endif
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="endEditLabel">
 *  <function returns="Boolean">
 *   <arg name="Cancel" type="Boolean">
 *    Cancel the edit, or commit the changes.
 *   </arg>
 *  </function>
 *  <desc>
 *   Ends editing the label. When <i>Cancel</i> is true, the label is left unchanged.
 *   Windows only
 *  </desc>
 * </method>
 */
JSBool ListCtrl::endEditLabel(JSContext *cx,
                              JSObject *obj,
                              uintN WXUNUSED(argc),
                              jsval *argv,
                              jsval *rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

	#ifdef __WXMSW__
	    bool cancel;
	
	    if ( FromJS(cx, argv[0], cancel) )
	    {
	        *rval = ToJS(cx, p->EndEditLabel(cancel));
	        return JS_TRUE;
	    }
	#endif
	
    return JS_FALSE;
}

/***
 * <method name="ensureVisible">
 *  <function returns="Boolean">
 *   <arg name="Item" type="Integer">
 *    The item index.
 *   </arg>
 *  </function>
 *  <desc>
 *   Makes sure the item is visible. Returns true on success.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::ensureVisible(JSContext *cx,
                               JSObject *obj,
                               uintN WXUNUSED(argc),
                               jsval *argv,
                               jsval *rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    long item;

    if ( FromJS(cx, argv[0], item) )
    {
        *rval = ToJS(cx, p->EnsureVisible(item));
        return JS_TRUE;
    }

    return JS_FALSE;
}


/***
 * <method name="findItem">
 *  <function returns="Integer">
 *   <arg name="Start" type="Integer">
 *    The item to start the search or -1 to start from the beginning.
 *   </arg>
 *   <arg name="Str" type="String">
 *    The label to search
 *   </arg>
 *   <arg name="Partial" type="Boolean" default="false">
 *    The label must be exactly the same or may start with <I>Str</I>. Default is false.
 *   </arg>
 *  </function>
 *  <function returns="Integer">
 *   <arg name="Start" type="Integer">
 *    The item to start the search or -1 to start from the beginning.
 *   </arg>
 *   <arg name="Data" type="Integer">
 *    The associated data of an item.
 *   </arg>
 *  </function>
 *  <function returns="Integer">
 *   <arg name="Start" type="Integer">
 *    The item to start the search or -1 to start from the beginning.
 *   </arg>
 *   <arg name="Pos" type="@wxPoint">
 *    Find an item near this position.
 *   </arg>
 *   <arg name="Direction" type="Integer">
 *    The direction of the search. See @wxListCtrl#wxListFind.
 *   </arg>
 *  </function>
 *  <desc>
 *   <ol>
 *    <li>Find an item whose label matches the string exact or partial.</li>
 *    <li>Find an item with the given associated data.</li>
 *    <li>Find an item nearest the position in the specified direction.</li>
 *   </ol>
 *   The search starts from the given item, or at the beginning when start is -1.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::findItem(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    long item;

    if ( FromJS(cx, argv[0], item) )
    {
        if ( Point::HasPrototype(cx, argv[1]) )
        {
            if ( argc < 3 )
                return JS_FALSE;

            wxPoint *pt = Point::GetPrivate(cx, argv[1], false);
            int direction;
            if ( FromJS(cx, argv[2], direction) )
            {
                *rval = ToJS(cx, p->FindItem(item, *pt, direction));
                return JS_TRUE;
            }
        }
        else if ( JSVAL_IS_INT(argv[1]) )
        {
            long data = JSVAL_TO_INT(argv[1]);
            *rval = ToJS(cx, p->FindItem(item, data));
            return JS_TRUE;
        }
        else
        {
            wxString str;
            FromJS(cx, argv[1], str);
            
            bool partial = false;
            if (    argc > 2 
                 && ! FromJS(cx, argv[2], partial) )
                return JS_FALSE;

            *rval = ToJS(cx, p->FindItem(item, str, partial));
            return JS_TRUE;
        }
    }

    return JS_FALSE;
}

/***
 * <method name="hitTest">
 *  <function returns="@wxListHitTest">
 *   <arg name="Pos" type="@wxPoint">
 *    The position to test.
 *   </arg>
 *  </function>
 *  <desc>
 *   Determines which item (if any) is at the specified point.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::hitTest(JSContext *cx,
                         JSObject *obj,
                         uintN WXUNUSED(argc),
                         jsval *argv,
                         jsval *rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxPoint *pt = Point::GetPrivate(cx, argv[0]);
    if ( pt != NULL )
    {
        int flags;
        long item = p->HitTest(*pt, flags);

        *rval = ListHitTest::CreateObject(cx, new wxListHitTest(item, flags));
        return JS_TRUE;
    }
    
    return JS_FALSE;
}

/***
 * <method name="scrollList">
 *  <function returns="Boolean">
 *   <arg name="X" type="Integer" />
 *   <arg name="Y" type="Integer" />
 *  </function>
 *  <desc>
 *   Scrolls the list control. If in icon, small icon or report view mode,
 *   <I>X</I> specifies the number of pixels to scroll. If in list view mode, <I>X</I>
 *   specifies the number of columns to scroll.
 *   If in icon, small icon or list view mode, <I>Y</I> specifies the number of pixels
 *   to scroll. If in report view mode, <I>Y</I> specifies the number of lines to scroll.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::scrollList(JSContext *cx,
                            JSObject *obj,
                            uintN WXUNUSED(argc),
                            jsval *argv,
                            jsval *rval)
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int x;
    int y;

    if (    FromJS(cx, argv[0], x)
         && FromJS(cx, argv[1], y) )
    {
        *rval = ToJS(cx, p->ScrollList(x, y));
        return JS_TRUE;
    }
    
    return JS_FALSE;
}

/***
 * <method name="sortItems">
 *  <function returns="Boolean">
 *   <arg name="SortFn" type="Function">
 *    A function which receives 3 arguments. The first argument is the associated data
 *    of the first item. The second argument is the associated data of the second item.
 *    The third arugment is the <I>Data</I> argument you specify with this method. The
 *    third argument may be omitted when you don't use it. The return value is an integer
 *    value. Return 0 when the items are equal, a negative value if the first item 
 *    is less then the second item and a positive value when the first item is greater
 *    then the second item.
 *   </arg>
 *   <arg name="Data" type="Integer" default="0">
 *    A value passed as the third argument of the sort function. When not passed, 0 is used.
 *   </arg>
 *  </function>
 *  <desc>
 *   Call this method to sort the items in the list control. Sorting is done using the 
 *   specified <I>SortFn</I>.<br />
 *   <b>Remark: </b>
 *   Notice that the control may only be sorted on client data associated with the items, 
 *   so you must use @wxListCtrl#setItemData if you want to be able to sort the items 
 *   in the control.
 *  </desc>
 * </method>
 */
JSBool ListCtrl::sortItems(JSContext *cx,
                           JSObject *obj,
                           uintN argc,
                           jsval *argv,
                           jsval* WXUNUSED(rval))
{
    wxListCtrl *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    JSFunction *fun = JS_ValueToFunction(cx, argv[0]);
    if ( fun == NULL )
        return JS_FALSE;

    long data = 0;

    if (      argc > 1 
         && ! FromJS(cx, argv[1], data) )
    {
        return JS_FALSE;
    }

    ListSort *sortData = new ListSort(cx, fun, data);
    p->SortItems(ListCtrl::SortFn, (long)((void *) sortData));
    delete sortData;
    
    return JS_TRUE;
}

/***
 * <events>
 *  <event name="onBeginDrag">
 *   This event is triggered when the user starts dragging with the left mouse button. 
 *   The function receives a @wxListEvent.
 *  </event>
 *  <event name="onBeginRDrag">
 *   This event is triggered when the user starts dragging with the right mouse button.
 *   The function receives a @wxListEvent.
 *  </event>
 *  <event name="onBeginLabelEdit">
 *   This event is triggered when the user starts editing the label. This event
 *   can be prevented by setting @wxNotifyEvent#veto to true.
 *   The function receives a @wxListEvent.
 *  </event>
 *  <event name="onEndLabelEdit">
 *   This event is triggered when the user ends editing the label.
 *   This event can be prevented by setting @wxNotifyEvent#veto to true.
 *   The function receives a @wxListEvent.
 *  </event>
 *  <event name="onDeleteItem">
 *   This event is triggered when an item is deleted.
 *   The function receives a @wxListEvent.
 *  </event>
 *  <event name="onDeleteAllItems">
 *   This event is triggered when all items are deleted.
 *   The function receives a @wxListEvent.
 *  </event>
 *  <event name="onItemSelected">
 *   This event is triggered when the user selects an item.
 *   The function receives a @wxListEvent.
 *  </event>
 *  <event name="onItemDeselected">
 *   This event is triggered when the user deselects an item.
 *   The function receives a @wxListEvent.
 *  </event>
 *  <event name="onItemActivated">
 *   This event is triggered when the user activates an item by pressing Enter or double clicking.
 *   The function receives a @wxListEvent.
 *  </event>
 *  <event name="onItemFocused">
 *   This event is triggered when the currently focused item changes.
 *   The function receives a @wxListEvent.
 *  </event>
 *  <event name="onItemRightClick">
 *   This event is triggered when the user starts right clicks an item.
 *   The function receives a @wxListEvent.
 *  </event>
 *  <event name="onListKeyDown">
 *   This event is triggered when a key is pressed.
 *   The function receives a @wxListEvent.
 *  </event>
 *  <event name="onInsertItem">
 *   This event is triggered when an item is inserted.
 *   The function receives a @wxListEvent.
 *  </event>
 *  <event name="onColClick">
 *   This event is triggered when the user left-clicks a column.
 *   The function receives a @wxListEvent.
 *  </event>
 *  <event name="onColRightClick">
 *   This event is triggered when the user right-clicks a column.
 *   The function receives a @wxListEvent.
 *  </event>
 *  <event name="onColBeginDrag">
 *   This event is triggered when the user starts resizing a column.
 *   The function receives a @wxListEvent.
 *  </event>
 *  <event name="onColDragging">
 *   This event is triggered when the divider between two columns is dragged.
 *   The function receives a @wxListEvent.
 *  <event name="onColEndDrag">
 *  </event>
 *   This event is triggered when a column is resized.
 *   The function receives a @wxListEvent.
 *  </event>
 *  <event name="onCacheHint">
 *   This event is triggered so that the application can prepare a cache for a virtual list control.
 *   The function receives a @wxListEvent.
 *  </event>
 * </events>
 */
WXJS_INIT_EVENT_MAP(wxListCtrl)
const wxString WXJS_LISTCTRL_BEGIN_DRAG = wxT("onBeginDrag");
const wxString WXJS_LISTCTRL_BEGIN_RDRAG = wxT("onBeginRDrag");
const wxString WXJS_LISTCTRL_BEGIN_LABELEDIT = wxT("onBeginLabelEdit");
const wxString WXJS_LISTCTRL_END_LABELEDIT = wxT("onEndLabelEdit");
const wxString WXJS_LISTCTRL_DELETE_ITEM = wxT("onDeleteItem");
const wxString WXJS_LISTCTRL_DELETE_ALL_ITEMS = wxT("onDeleteAllItems");
const wxString WXJS_LISTCTRL_ITEM_SELECTED = wxT("onItemSelected");
const wxString WXJS_LISTCTRL_ITEM_DESELECTED = wxT("onItemDeselected");
const wxString WXJS_LISTCTRL_ITEM_ACTIVATED = wxT("onItemActivated");
const wxString WXJS_LISTCTRL_ITEM_FOCUSED = wxT("onItemFocused");
const wxString WXJS_LISTCTRL_ITEM_RIGHT_CLICK = wxT("onItemRightClick");
const wxString WXJS_LISTCTRL_KEY_DOWN = wxT("onListKeyDown");
const wxString WXJS_LISTCTRL_INSERT_ITEM = wxT("onInsertItem");
const wxString WXJS_LISTCTRL_COL_CLICK = wxT("onColClick");
const wxString WXJS_LISTCTRL_COL_RCLICK = wxT("onColRightClick");
const wxString WXJS_LISTCTRL_COL_BEGIN_DRAG = wxT("onColBeginDrag");
const wxString WXJS_LISTCTRL_COL_DRAGGING = wxT("onColDragging");
const wxString WXJS_LISTCTRL_COL_END_DRAG = wxT("onColEndDrag");
const wxString WXJS_LISTCTRL_CACHE_HINT = wxT("onCacheHint");

void ListCtrlEventHandler::OnBeginDrag(wxListEvent &event)
{
  PrivListEvent::Fire<ListEvent>(event, WXJS_LISTCTRL_BEGIN_DRAG);
}

void ListCtrlEventHandler::OnBeginRDrag(wxListEvent &event)
{
  PrivListEvent::Fire<ListEvent>(event, WXJS_LISTCTRL_BEGIN_RDRAG);
}

void ListCtrlEventHandler::OnBeginLabelEdit(wxListEvent &event)
{
  PrivListEvent::Fire<ListEvent>(event, WXJS_LISTCTRL_BEGIN_LABELEDIT);
}

void ListCtrlEventHandler::OnEndLabelEdit(wxListEvent &event)
{
  PrivListEvent::Fire<ListEvent>(event, WXJS_LISTCTRL_END_LABELEDIT);
}

void ListCtrlEventHandler::OnDeleteItem(wxListEvent &event)
{
  PrivListEvent::Fire<ListEvent>(event, WXJS_LISTCTRL_DELETE_ITEM);
}

void ListCtrlEventHandler::OnDeleteAllItems(wxListEvent &event)
{
  PrivListEvent::Fire<ListEvent>(event, WXJS_LISTCTRL_DELETE_ALL_ITEMS);
}

void ListCtrlEventHandler::OnItemSelected(wxListEvent &event)
{
  PrivListEvent::Fire<ListEvent>(event, WXJS_LISTCTRL_ITEM_SELECTED);
}

void ListCtrlEventHandler::OnItemDeselected(wxListEvent &event)
{
  PrivListEvent::Fire<ListEvent>(event, WXJS_LISTCTRL_ITEM_DESELECTED);
}

void ListCtrlEventHandler::OnItemActivated(wxListEvent &event)
{
  PrivListEvent::Fire<ListEvent>(event, WXJS_LISTCTRL_ITEM_ACTIVATED);
}

void ListCtrlEventHandler::OnItemFocused(wxListEvent &event)
{
  PrivListEvent::Fire<ListEvent>(event, WXJS_LISTCTRL_ITEM_FOCUSED);
}

void ListCtrlEventHandler::OnItemRightClick(wxListEvent &event)
{
  PrivListEvent::Fire<ListEvent>(event, WXJS_LISTCTRL_ITEM_RIGHT_CLICK);
}

void ListCtrlEventHandler::OnListKeyDown(wxListEvent &event)
{
  PrivListEvent::Fire<ListEvent>(event, WXJS_LISTCTRL_KEY_DOWN);
}

void ListCtrlEventHandler::OnInsertItem(wxListEvent &event)
{
  PrivListEvent::Fire<ListEvent>(event, WXJS_LISTCTRL_INSERT_ITEM);
}

void ListCtrlEventHandler::OnColClick(wxListEvent &event)
{
  PrivListEvent::Fire<ListEvent>(event, WXJS_LISTCTRL_COL_CLICK);
}

void ListCtrlEventHandler::OnColRightClick(wxListEvent &event)
{
  PrivListEvent::Fire<ListEvent>(event, WXJS_LISTCTRL_COL_RCLICK);
}

void ListCtrlEventHandler::OnColBeginDrag(wxListEvent &event)
{
  PrivListEvent::Fire<ListEvent>(event, WXJS_LISTCTRL_COL_BEGIN_DRAG);
}

void ListCtrlEventHandler::OnColDragging(wxListEvent &event)
{
  PrivListEvent::Fire<ListEvent>(event, WXJS_LISTCTRL_COL_DRAGGING);
}

void ListCtrlEventHandler::OnColEndDrag(wxListEvent &event)
{
  PrivListEvent::Fire<ListEvent>(event, WXJS_LISTCTRL_COL_END_DRAG);
}

void ListCtrlEventHandler::OnCacheHint(wxListEvent &event)
{
  PrivListEvent::Fire<ListEvent>(event, WXJS_LISTCTRL_CACHE_HINT);
}

void ListCtrlEventHandler::ConnectBeginDrag(wxListCtrl *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_COMMAND_LIST_BEGIN_DRAG,
               wxListEventHandler(ListCtrlEventHandler::OnBeginDrag));
  }
  else
  {
    p->Disconnect(wxEVT_COMMAND_LIST_BEGIN_DRAG, 
                  wxListEventHandler(ListCtrlEventHandler::OnBeginDrag));
  }
}

void ListCtrlEventHandler::ConnectBeginRDrag(wxListCtrl *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_COMMAND_LIST_BEGIN_RDRAG,
               wxListEventHandler(ListCtrlEventHandler::OnBeginRDrag));
  }
  else
  {
    p->Disconnect(wxEVT_COMMAND_LIST_BEGIN_RDRAG, 
                  wxListEventHandler(ListCtrlEventHandler::OnBeginRDrag));
  }
}

void ListCtrlEventHandler::ConnectBeginLabelEdit(wxListCtrl *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_COMMAND_LIST_BEGIN_LABEL_EDIT,
               wxListEventHandler(ListCtrlEventHandler::OnBeginLabelEdit));
  }
  else
  {
    p->Disconnect(wxEVT_COMMAND_LIST_BEGIN_LABEL_EDIT, 
                  wxListEventHandler(ListCtrlEventHandler::OnBeginLabelEdit));
  }
}

void ListCtrlEventHandler::ConnectEndLabelEdit(wxListCtrl *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_COMMAND_LIST_END_LABEL_EDIT,
               wxListEventHandler(ListCtrlEventHandler::OnEndLabelEdit));
  }
  else
  {
    p->Disconnect(wxEVT_COMMAND_LIST_BEGIN_DRAG, 
                  wxListEventHandler(ListCtrlEventHandler::OnEndLabelEdit));
  }
}

void ListCtrlEventHandler::ConnectDeleteItem(wxListCtrl *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_COMMAND_LIST_DELETE_ITEM,
               wxListEventHandler(ListCtrlEventHandler::OnDeleteItem));
  }
  else
  {
    p->Disconnect(wxEVT_COMMAND_LIST_DELETE_ITEM, 
                  wxListEventHandler(ListCtrlEventHandler::OnDeleteItem));
  }
}

void ListCtrlEventHandler::ConnectDeleteAllItems(wxListCtrl *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_COMMAND_LIST_DELETE_ALL_ITEMS,
               wxListEventHandler(ListCtrlEventHandler::OnDeleteAllItems));
  }
  else
  {
    p->Disconnect(wxEVT_COMMAND_LIST_DELETE_ALL_ITEMS, 
                  wxListEventHandler(ListCtrlEventHandler::OnDeleteAllItems));
  }
}

void ListCtrlEventHandler::ConnectItemSelected(wxListCtrl *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_COMMAND_LIST_ITEM_SELECTED,
               wxListEventHandler(ListCtrlEventHandler::OnItemSelected));
  }
  else
  {
    p->Disconnect(wxEVT_COMMAND_LIST_ITEM_SELECTED, 
                  wxListEventHandler(ListCtrlEventHandler::OnItemSelected));
  }
}

void ListCtrlEventHandler::ConnectItemDeselected(wxListCtrl *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_COMMAND_LIST_ITEM_DESELECTED,
               wxListEventHandler(ListCtrlEventHandler::OnItemDeselected));
  }
  else
  {
    p->Disconnect(wxEVT_COMMAND_LIST_ITEM_DESELECTED, 
                  wxListEventHandler(ListCtrlEventHandler::OnItemDeselected));
  }
}

void ListCtrlEventHandler::ConnectItemActivated(wxListCtrl *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_COMMAND_LIST_ITEM_ACTIVATED,
               wxListEventHandler(ListCtrlEventHandler::OnItemActivated));
  }
  else
  {
    p->Disconnect(wxEVT_COMMAND_LIST_ITEM_ACTIVATED, 
                  wxListEventHandler(ListCtrlEventHandler::OnItemActivated));
  }
}

void ListCtrlEventHandler::ConnectItemFocused(wxListCtrl *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_COMMAND_LIST_ITEM_FOCUSED,
               wxListEventHandler(ListCtrlEventHandler::OnItemFocused));
  }
  else
  {
    p->Disconnect(wxEVT_COMMAND_LIST_ITEM_FOCUSED, 
                  wxListEventHandler(ListCtrlEventHandler::OnItemFocused));
  }
}

void ListCtrlEventHandler::ConnectItemRightClick(wxListCtrl *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK,
               wxListEventHandler(ListCtrlEventHandler::OnItemRightClick));
  }
  else
  {
    p->Disconnect(wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, 
                  wxListEventHandler(ListCtrlEventHandler::OnItemRightClick));
  }
}

void ListCtrlEventHandler::ConnectListKeyDown(wxListCtrl *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_COMMAND_LIST_KEY_DOWN,
               wxListEventHandler(ListCtrlEventHandler::OnListKeyDown));
  }
  else
  {
    p->Disconnect(wxEVT_COMMAND_LIST_KEY_DOWN, 
                  wxListEventHandler(ListCtrlEventHandler::OnListKeyDown));
  }
}

void ListCtrlEventHandler::ConnectInsertItem(wxListCtrl *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_COMMAND_LIST_INSERT_ITEM,
               wxListEventHandler(ListCtrlEventHandler::OnInsertItem));
  }
  else
  {
    p->Disconnect(wxEVT_COMMAND_LIST_INSERT_ITEM, 
                  wxListEventHandler(ListCtrlEventHandler::OnInsertItem));
  }
}

void ListCtrlEventHandler::ConnectColClick(wxListCtrl *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_COMMAND_LIST_COL_CLICK,
               wxListEventHandler(ListCtrlEventHandler::OnColClick));
  }
  else
  {
    p->Disconnect(wxEVT_COMMAND_LIST_COL_CLICK, 
                  wxListEventHandler(ListCtrlEventHandler::OnColClick));
  }
}

void ListCtrlEventHandler::ConnectColRightClick(wxListCtrl *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_COMMAND_LIST_COL_RIGHT_CLICK,
               wxListEventHandler(ListCtrlEventHandler::OnColRightClick));
  }
  else
  {
    p->Disconnect(wxEVT_COMMAND_LIST_COL_RIGHT_CLICK, 
                  wxListEventHandler(ListCtrlEventHandler::OnColRightClick));
  }
}

void ListCtrlEventHandler::ConnectColBeginDrag(wxListCtrl *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_COMMAND_LIST_COL_BEGIN_DRAG,
               wxListEventHandler(ListCtrlEventHandler::OnColBeginDrag));
  }
  else
  {
    p->Disconnect(wxEVT_COMMAND_LIST_COL_BEGIN_DRAG, 
                  wxListEventHandler(ListCtrlEventHandler::OnColBeginDrag));
  }
}

void ListCtrlEventHandler::ConnectColDragging(wxListCtrl *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_COMMAND_LIST_COL_DRAGGING,
               wxListEventHandler(ListCtrlEventHandler::OnColDragging));
  }
  else
  {
    p->Disconnect(wxEVT_COMMAND_LIST_COL_DRAGGING, 
                  wxListEventHandler(ListCtrlEventHandler::OnColDragging));
  }
}

void ListCtrlEventHandler::ConnectColEndDrag(wxListCtrl *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_COMMAND_LIST_COL_END_DRAG,
               wxListEventHandler(ListCtrlEventHandler::OnColEndDrag));
  }
  else
  {
    p->Disconnect(wxEVT_COMMAND_LIST_COL_END_DRAG, 
                  wxListEventHandler(ListCtrlEventHandler::OnColEndDrag));
  }
}

void ListCtrlEventHandler::ConnectCacheHint(wxListCtrl *p, bool connect)
{
  if ( connect )
  {
    p->Connect(wxEVT_COMMAND_LIST_CACHE_HINT,
               wxListEventHandler(ListCtrlEventHandler::OnCacheHint));
  }
  else
  {
    p->Disconnect(wxEVT_COMMAND_LIST_CACHE_HINT, 
                  wxListEventHandler(ListCtrlEventHandler::OnCacheHint));
  }
}

void ListCtrlEventHandler::InitConnectEventMap()
{
  AddConnector(WXJS_LISTCTRL_BEGIN_DRAG, ConnectBeginDrag);
  AddConnector(WXJS_LISTCTRL_BEGIN_RDRAG, ConnectBeginRDrag);
  AddConnector(WXJS_LISTCTRL_BEGIN_LABELEDIT, ConnectBeginLabelEdit);
  AddConnector(WXJS_LISTCTRL_END_LABELEDIT, ConnectEndLabelEdit);
  AddConnector(WXJS_LISTCTRL_DELETE_ITEM, ConnectDeleteItem);
  AddConnector(WXJS_LISTCTRL_DELETE_ALL_ITEMS, ConnectDeleteAllItems);
  AddConnector(WXJS_LISTCTRL_ITEM_SELECTED, ConnectItemSelected);
  AddConnector(WXJS_LISTCTRL_ITEM_DESELECTED, ConnectItemDeselected);
  AddConnector(WXJS_LISTCTRL_ITEM_ACTIVATED, ConnectItemActivated);
  AddConnector(WXJS_LISTCTRL_ITEM_FOCUSED, ConnectItemFocused);
  AddConnector(WXJS_LISTCTRL_ITEM_RIGHT_CLICK, ConnectItemRightClick);
  AddConnector(WXJS_LISTCTRL_KEY_DOWN, ConnectListKeyDown);
  AddConnector(WXJS_LISTCTRL_INSERT_ITEM, ConnectInsertItem);
  AddConnector(WXJS_LISTCTRL_COL_CLICK, ConnectColClick);
  AddConnector(WXJS_LISTCTRL_COL_RCLICK, ConnectColRightClick);
  AddConnector(WXJS_LISTCTRL_COL_BEGIN_DRAG, ConnectBeginDrag);
  AddConnector(WXJS_LISTCTRL_COL_DRAGGING, ConnectColDragging);
  AddConnector(WXJS_LISTCTRL_COL_END_DRAG, ConnectColEndDrag);
  AddConnector(WXJS_LISTCTRL_CACHE_HINT, ConnectCacheHint);
}
