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
 * $Id: listctrl.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSListCtrl_H
#define _WXJSListCtrl_H

/////////////////////////////////////////////////////////////////////////////
// Name:        listctrl.h
// Purpose:     ListCtrl ports wxListCtrl to JavaScript
// Author:      Franky Braem
// Modified by:
// Created:     28.09.2002
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

#include <wx/listctrl.h>

namespace wxjs
{
    namespace gui
    {
        class ListCtrl : public wxListCtrl
                           , public ApiWrapper<ListCtrl, wxListCtrl>
                           , public Object
        {
        public:
            ListCtrl(JSContext *cx, JSObject *obj);

            virtual ~ListCtrl();

            // Callback used for sorting.
            static int wxCALLBACK SortFn(long item1, long item2, long data);

            // Fn's for virtual list controls.
            wxString OnGetItemText(long item, long column) const;
            int OnGetItemImage(long item) const;
            wxListItemAttr *OnGetItemAttr(long item) const;

            /**
             * Callback for retrieving properties of wxListCtrl
             */
            static bool GetProperty(wxListCtrl *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

            /**
             * Callback for setting properties
             */
            static bool SetProperty(wxListCtrl *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

            /**
             * Callback for when a wxListCtrl object is created
             */
            static wxListCtrl* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);
            static void InitClass(JSContext *cx, JSObject *obj, JSObject *proto);

            /**
             * Callback for when a wxListCtrl object is destroyed
             */
            static void Destruct(JSContext *cx, wxListCtrl *p);

            WXJS_DECLARE_PROPERTY_MAP()
            /**
             * Property Ids.
             */
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
            static JSBool getColumn(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setColumn(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getColumnWidth(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setColumnWidth(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getItem(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setItem(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getItemState(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setItemState(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setItemImage(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getItemText(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setItemText(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getItemData(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setItemData(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getItemRect(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getItemPosition(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setItemPosition(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getItemSpacing(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setSingleStyle(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getNextItem(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getImageList(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setImageList(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool refreshItem(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool refreshItems(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool arrange(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool deleteItem(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool deleteAllItems(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool deleteColumn(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool deleteAllColumns(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool clearAll(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool insertColumn(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool insertItem(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool editLabel(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool endEditLabel(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool ensureVisible(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool findItem(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool hitTest(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool scrollList(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool sortItems(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

            WXJS_DECLARE_CONSTANT_MAP()

            DECLARE_EVENT_TABLE()

            void OnBeginDrag(wxListEvent &event);
            void OnBeginRDrag(wxListEvent &event);
            void OnBeginLabelEdit(wxListEvent &event);
            void OnEndLabelEdit(wxListEvent &event);
            void onDeleteItem(wxListEvent &event);
            void onDeleteAllItems(wxListEvent &event);
            void onItemSelected(wxListEvent &event);
            void onItemDeselected(wxListEvent &event);
            void onItemActivated(wxListEvent &event);
            void onItemFocused(wxListEvent &event);
            void onItemRightClick(wxListEvent &event);
            void onListKeyDown(wxListEvent &event);
            void onInsertItem(wxListEvent &event);
            void onColClick(wxListEvent &event);
            void onColRightClick(wxListEvent &event);
            void onColBeginDrag(wxListEvent &event);
            void onColDragging(wxListEvent &event);
            void onColEndDrag(wxListEvent &event);
            void onCacheHint(wxListEvent &event);
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
                    JS_AddRoot(m_cx, &m_val);
            }

            virtual ~ListObjectData()
            {
                if ( JSVAL_IS_GCTHING(m_val) ) {}
                    JS_RemoveRoot(m_cx, &m_val);
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
