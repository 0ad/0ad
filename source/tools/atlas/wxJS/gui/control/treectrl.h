/*
 * wxJavaScript - treectrl.h
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
 * $Id: treectrl.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _WXJSTreeCtrl_H
#define _WXJSTreeCtrl_H

/////////////////////////////////////////////////////////////////////////////
// Name:        treectrl.h
// Purpose:     TreeCtrl ports wxTreeCtrl to JavaScript
// Author:      Franky Braem
// Modified by:
// Created:     03.01.2003
// Copyright:   (c) 2001-2003 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

#include <wx/treectrl.h>
namespace wxjs
{
    namespace gui
    {
        class TreeCtrl : public wxTreeCtrl
                           , public ApiWrapper<TreeCtrl, wxTreeCtrl>
                           , public Object
        {
        public:
            TreeCtrl(JSContext *cx, JSObject *obj);

            virtual ~TreeCtrl();
            
            virtual int OnCompareItems(const wxTreeItemId& item1,
                                       const wxTreeItemId& item2);
            /**
             * Callback for retrieving properties of wxTreeCtrl
             */
            static bool GetProperty(wxTreeCtrl *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

            /**
             * Callback for setting properties
             */
            static bool SetProperty(wxTreeCtrl *p, JSContext *cx, JSObject *obj, int id, jsval *vp);

            /**
             * Callback for when a wxTreeCtrl object is created
             */
            static wxTreeCtrl* Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing);

            /**
             * Callback for when a wxTreeCtrl object is destroyed
             */
            static void Destruct(JSContext *cx, wxTreeCtrl *p);

            WXJS_DECLARE_PROPERTY_MAP()

            /**
             * Property Ids.
             */
            enum
            {
                P_COUNT
                , P_INDENT
                , P_IMAGE_LIST
                , P_STATE_IMAGE_LIST
                , P_ROOT_ITEM
                , P_SELECTION
                , P_SELECTIONS
                , P_FIRST_VISIBLE
                , P_EDIT_CONTROL
            };

            WXJS_DECLARE_METHOD_MAP()
            static JSBool getItem(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getItemText(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setItemText(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getItemImage(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setItemImage(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getItemData(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setItemData(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getItemTextColour(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setItemTextColour(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getItemBackgroundColour(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setItemBackgroundColour(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getItemFont(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setItemFont(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setItemHasChildren(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool isBold(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setItemBold(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool setItemDropHighlight(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool isVisible(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool isExpanded(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool isSelected(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getChildrenCount(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getItemParent(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getFirstChild(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getNextChild(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getPrevSibling(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getNextSibling(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getPrevVisible(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool getNextVisible(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool addRoot(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool appendItem(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool prependItem(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool insertItem(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool deleteItem(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool deleteChildren(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool deleteAllItems(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool expand(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool collapse(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool collapseAndReset(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool toggle(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool unselect(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool unselectAll(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool selectItem(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool ensureVisible(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool scrollTo(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool editLabel(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool endEditLabel(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool sortChildren(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
            static JSBool hitTest(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

            WXJS_DECLARE_CONSTANT_MAP()

            DECLARE_EVENT_TABLE()

        protected:
            void onBeginDrag(wxTreeEvent &event);
            void onBeginRDrag(wxTreeEvent &event);
            void onBeginLabelEdit(wxTreeEvent &event);
            void onEndLabelEdit(wxTreeEvent &event);
            void onDeleteItem(wxTreeEvent &event);
            void onGetInfo(wxTreeEvent &event);
            void onSetInfo(wxTreeEvent &event);
            void onItemExpanded(wxTreeEvent &event);
            void onItemExpanding(wxTreeEvent &event);
            void onItemCollapsed(wxTreeEvent &event);
            void onItemCollapsing(wxTreeEvent &event);
            void onSelChanged(wxTreeEvent &event);
            void onSelChanging(wxTreeEvent &event);
            void onKeyDown(wxTreeEvent &event);
            void onItemActivated(wxTreeEvent &event);
            void onItemRightClick(wxTreeEvent &event);
            void onItemMiddleClick(wxTreeEvent &event);
            void onEndDrag(wxTreeEvent &event);
        };

        /**
         * Wrapper class for a treeitem. This is necessary 
         * to protect the JavaScript objects from garbage collection
         */
        class ObjectTreeData : public wxTreeItemData
        {
        public:
	        ObjectTreeData(JSContext *cx, jsval v) : wxTreeItemData(), m_cx(cx), m_val(v)
	        {
                if ( JSVAL_IS_GCTHING(m_val) )
                    JS_AddRoot(m_cx, &m_val);
	        }

	        virtual ~ObjectTreeData()
	        {
                if ( JSVAL_IS_GCTHING(m_val) )
                    JS_RemoveRoot(m_cx, &m_val);
	        }

	        inline jsval GetJSVal()
	        {
		        return m_val;
	        }
        protected:
        private:
	        JSContext *m_cx;
	        jsval m_val;
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_WXJSTreeCtrl_H
