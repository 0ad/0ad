/*
 * wxJavaScript - listctrl.h
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
 * $Id: listctrl.h 734 2007-06-06 20:09:13Z fbraem $
 */
#ifndef _WXJSListCtrl_H
#define _WXJSListCtrl_H

#include <wx/listctrl.h>

#include "../../common/evtconn.h"

namespace wxjs
{
    namespace gui
    {
        class ListCtrl : public wxListCtrl
                       , public ApiWrapper<ListCtrl, wxListCtrl>
        {
        public:
            ListCtrl();
            ~ListCtrl();

            // Callback used for sorting.
            static int wxCALLBACK SortFn(long item1, long item2, long data);
            // Fn's for virtual list controls.
            wxString OnGetItemText(long item, long column) const;
            int OnGetItemImage(long item) const;
            wxListItemAttr *OnGetItemAttr(long item) const;

            static bool GetProperty(wxListCtrl *p,
                                    JSContext *cx,
                                    JSObject *obj,
                                    int id,
                                    jsval *vp);
            static bool SetProperty(wxListCtrl *p,
                                    JSContext *cx,
                                    JSObject *obj,
                                    int id,
                                    jsval *vp);
          static bool AddProperty(wxListCtrl *p, 
                                  JSContext *cx, 
                                  JSObject *obj, 
                                  const wxString &prop, 
                                  jsval *vp);
          static bool DeleteProperty(wxListCtrl *p, 
                                     JSContext* cx, 
                                     JSObject* obj, 
                                     const wxString &prop);

            /**
             * Callback for when a wxListCtrl object is created
             */
            static wxListCtrl* Construct(JSContext *cx,
                                         JSObject *obj,
                                         uintN argc,
                                         jsval *argv,
                                         bool constructing);
            static void InitClass(JSContext *cx,
                                  JSObject *obj,
                                  JSObject *proto);

            WXJS_DECLARE_PROPERTY_MAP()
            enum
            {
                P_COUNT_PER_PAGE
                , P_EDIT_CONTROL
                , P_COLUMN_COUNT
                , P_ITEM_COUNT
                , P_SELECTED_ITEM_COUNT
                , P_TEXT_COLOUR
                , P_TOP_ITEM
                , P_WINDOW_STYLE
                , P_IS_VIRTUAL
            };

            WXJS_DECLARE_METHOD_MAP()
            WXJS_DECLARE_METHOD(create)
            WXJS_DECLARE_METHOD(getColumn)
            WXJS_DECLARE_METHOD(setColumn)
            WXJS_DECLARE_METHOD(getColumnWidth)
            WXJS_DECLARE_METHOD(setColumnWidth)
            WXJS_DECLARE_METHOD(getItem)
            WXJS_DECLARE_METHOD(setItem)
            WXJS_DECLARE_METHOD(getItemState)
            WXJS_DECLARE_METHOD(setItemState)
            WXJS_DECLARE_METHOD(setItemImage)
            WXJS_DECLARE_METHOD(getItemText)
            WXJS_DECLARE_METHOD(setItemText)
            WXJS_DECLARE_METHOD(getItemData)
            WXJS_DECLARE_METHOD(setItemData)
            WXJS_DECLARE_METHOD(getItemRect)
            WXJS_DECLARE_METHOD(getItemPosition)
            WXJS_DECLARE_METHOD(setItemPosition)
            WXJS_DECLARE_METHOD(getItemSpacing)
            WXJS_DECLARE_METHOD(setSingleStyle)
            WXJS_DECLARE_METHOD(getNextItem)
            WXJS_DECLARE_METHOD(getImageList)
            WXJS_DECLARE_METHOD(setImageList)
            WXJS_DECLARE_METHOD(refreshItem)
            WXJS_DECLARE_METHOD(refreshItems)
            WXJS_DECLARE_METHOD(arrange)
            WXJS_DECLARE_METHOD(deleteItem)
            WXJS_DECLARE_METHOD(deleteAllItems)
            WXJS_DECLARE_METHOD(deleteColumn)
            WXJS_DECLARE_METHOD(deleteAllColumns)
            WXJS_DECLARE_METHOD(clearAll)
            WXJS_DECLARE_METHOD(insertColumn)
            WXJS_DECLARE_METHOD(insertItem)
            WXJS_DECLARE_METHOD(editLabel)
            WXJS_DECLARE_METHOD(endEditLabel)
            WXJS_DECLARE_METHOD(ensureVisible)
            WXJS_DECLARE_METHOD(findItem)
            WXJS_DECLARE_METHOD(hitTest)
            WXJS_DECLARE_METHOD(scrollList)
            WXJS_DECLARE_METHOD(sortItems)

            WXJS_DECLARE_CONSTANT_MAP()
        };

        class ListCtrlEventHandler : public EventConnector<wxListCtrl>
                                   , public wxEvtHandler
        {
        public:
            // Events
            void OnBeginDrag(wxListEvent &event);
            void OnBeginRDrag(wxListEvent &event);
            void OnBeginLabelEdit(wxListEvent &event);
            void OnEndLabelEdit(wxListEvent &event);
            void OnDeleteItem(wxListEvent &event);
            void OnDeleteAllItems(wxListEvent &event);
            void OnItemSelected(wxListEvent &event);
            void OnItemDeselected(wxListEvent &event);
            void OnItemActivated(wxListEvent &event);
            void OnItemFocused(wxListEvent &event);
            void OnItemRightClick(wxListEvent &event);
            void OnListKeyDown(wxListEvent &event);
            void OnInsertItem(wxListEvent &event);
            void OnColClick(wxListEvent &event);
            void OnColRightClick(wxListEvent &event);
            void OnColBeginDrag(wxListEvent &event);
            void OnColDragging(wxListEvent &event);
            void OnColEndDrag(wxListEvent &event);
            void OnCacheHint(wxListEvent &event);
            static void InitConnectEventMap();
        private:
            static void ConnectBeginDrag(wxListCtrl *p, bool connect);
            static void ConnectBeginRDrag(wxListCtrl *p, bool connect);
            static void ConnectBeginLabelEdit(wxListCtrl *p, bool connect);
            static void ConnectEndLabelEdit(wxListCtrl *p, bool connect);
            static void ConnectDeleteItem(wxListCtrl *p, bool connect);
            static void ConnectDeleteAllItems(wxListCtrl *p, bool connect);
            static void ConnectItemSelected(wxListCtrl *p, bool connect);
            static void ConnectItemDeselected(wxListCtrl *p, bool connect);
            static void ConnectItemActivated(wxListCtrl *p, bool connect);
            static void ConnectItemFocused(wxListCtrl *p, bool connect);
            static void ConnectItemRightClick(wxListCtrl *p, bool connect);
            static void ConnectListKeyDown(wxListCtrl *p, bool connect);
            static void ConnectInsertItem(wxListCtrl *p, bool connect);
            static void ConnectColClick(wxListCtrl *p, bool connect);
            static void ConnectColRightClick(wxListCtrl *p, bool connect);
            static void ConnectColBeginDrag(wxListCtrl *p, bool connect);
            static void ConnectColDragging(wxListCtrl *p, bool connect);
            static void ConnectColEndDrag(wxListCtrl *p, bool connect);
            static void ConnectCacheHint(wxListCtrl *p, bool connect);
        };

        /**
         * Helper class used for sorting items in wxListCtrl
         */
        class ListSort
        {
        public:
            ListSort(JSContext *cx, JSFunction *fun, long data) : m_cx(cx), m_fun(fun), m_data(data)
            {
            }

            JSFunction *GetFn() const
            {
                return m_fun;
            }

            long GetData() const
            {
                return m_data;
            }

	        JSContext *GetContext() const
	        {
		        return m_cx;
	        }

        private:
	        JSContext *m_cx;
            JSFunction *m_fun;
            long m_data;
        };

        class ListObjectData
        {
        public:
            ListObjectData(JSContext *cx, jsval v) : m_cx(cx), m_val(v)
            {
                if ( JSVAL_IS_GCTHING(m_val) ) {}
                    JS_AddValueRoot(m_cx, &m_val);
            }

            virtual ~ListObjectData()
            {
                if ( JSVAL_IS_GCTHING(m_val) ) {}
                    JS_RemoveValueRoot(m_cx, &m_val);
            }

            jsval GetJSVal()
            {
                return m_val;
            }
        private:
	        JSContext *m_cx;
            jsval m_val;
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSListCtrl_H
