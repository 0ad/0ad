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
 * $Id: treectrl.h 696 2007-05-07 21:16:23Z fbraem $
 */
#ifndef _WXJSTreeCtrl_H
#define _WXJSTreeCtrl_H

#include "../../common/evtconn.h"

#include <wx/treectrl.h>
namespace wxjs
{
    namespace gui
    {
        class TreeCtrl : public wxTreeCtrl
                       , public ApiWrapper<TreeCtrl, wxTreeCtrl>
        {
        public:
            TreeCtrl();
            virtual ~TreeCtrl();
            
            virtual int OnCompareItems(const wxTreeItemId& item1,
                                       const wxTreeItemId& item2);
            static void InitClass(JSContext* cx, 
                                  JSObject* obj, 
                                  JSObject* proto);
            static bool AddProperty(wxTreeCtrl *p, 
                                    JSContext *cx, 
                                    JSObject *obj, 
                                    const wxString &prop, 
                                    jsval *vp);
            static bool DeleteProperty(wxTreeCtrl *p, 
                                       JSContext* cx, 
                                       JSObject* obj, 
                                       const wxString &prop);
            static bool GetProperty(wxTreeCtrl *p,
                                    JSContext *cx,
                                    JSObject *obj,
                                    int id,
                                    jsval *vp);
            static bool SetProperty(wxTreeCtrl *p,
                                    JSContext *cx,
                                    JSObject *obj,
                                    int id,
                                    jsval *vp);

            static wxTreeCtrl* Construct(JSContext *cx,
                                         JSObject *obj,
                                         uintN argc,
                                         jsval *argv,
                                         bool constructing);

            WXJS_DECLARE_PROPERTY_MAP()
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
            WXJS_DECLARE_METHOD(create)
            WXJS_DECLARE_METHOD(getItem)
            WXJS_DECLARE_METHOD(getItemText)
            WXJS_DECLARE_METHOD(setItemText)
            WXJS_DECLARE_METHOD(getItemImage)
            WXJS_DECLARE_METHOD(setItemImage)
            WXJS_DECLARE_METHOD(getItemData)
            WXJS_DECLARE_METHOD(setItemData)
            WXJS_DECLARE_METHOD(getItemTextColour)
            WXJS_DECLARE_METHOD(setItemTextColour)
            WXJS_DECLARE_METHOD(getItemBackgroundColour)
            WXJS_DECLARE_METHOD(setItemBackgroundColour)
            WXJS_DECLARE_METHOD(getItemFont)
            WXJS_DECLARE_METHOD(setItemFont)
            WXJS_DECLARE_METHOD(setItemHasChildren)
            WXJS_DECLARE_METHOD(isBold)
            WXJS_DECLARE_METHOD(setItemBold)
            WXJS_DECLARE_METHOD(setItemDropHighlight)
            WXJS_DECLARE_METHOD(isVisible)
            WXJS_DECLARE_METHOD(isExpanded)
            WXJS_DECLARE_METHOD(isSelected)
            WXJS_DECLARE_METHOD(getChildrenCount)
            WXJS_DECLARE_METHOD(getItemParent)
            WXJS_DECLARE_METHOD(getFirstChild)
            WXJS_DECLARE_METHOD(getNextChild)
            WXJS_DECLARE_METHOD(getPrevSibling)
            WXJS_DECLARE_METHOD(getNextSibling)
            WXJS_DECLARE_METHOD(getPrevVisible)
            WXJS_DECLARE_METHOD(getNextVisible)
            WXJS_DECLARE_METHOD(addRoot)
            WXJS_DECLARE_METHOD(appendItem)
            WXJS_DECLARE_METHOD(prependItem)
            WXJS_DECLARE_METHOD(insertItem)
            WXJS_DECLARE_METHOD(deleteItem)
            WXJS_DECLARE_METHOD(deleteChildren)
            WXJS_DECLARE_METHOD(deleteAllItems)
            WXJS_DECLARE_METHOD(expand)
            WXJS_DECLARE_METHOD(collapse)
            WXJS_DECLARE_METHOD(collapseAndReset)
            WXJS_DECLARE_METHOD(toggle)
            WXJS_DECLARE_METHOD(unselect)
            WXJS_DECLARE_METHOD(unselectAll)
            WXJS_DECLARE_METHOD(selectItem)
            WXJS_DECLARE_METHOD(ensureVisible)
            WXJS_DECLARE_METHOD(scrollTo)
            WXJS_DECLARE_METHOD(editLabel)
            WXJS_DECLARE_METHOD(endEditLabel)
            WXJS_DECLARE_METHOD(sortChildren)
            WXJS_DECLARE_METHOD(hitTest)

            WXJS_DECLARE_CONSTANT_MAP()
        };

        class TreeCtrlEventHandler : public EventConnector<wxTreeCtrl>
                                 , public wxEvtHandler
        {
        public:
          // Events
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
            static void InitConnectEventMap();
        private:
            static void ConnectBeginDrag(wxTreeCtrl *p, bool connect);
            static void ConnectBeginRDrag(wxTreeCtrl *p, bool connect);
            static void ConnectBeginLabelEdit(wxTreeCtrl *p, bool connect);
            static void ConnectEndLabelEdit(wxTreeCtrl *p, bool connect);
            static void ConnectDeleteItem(wxTreeCtrl *p, bool connect);
            static void ConnectGetInfo(wxTreeCtrl *p, bool connect);
            static void ConnectSetInfo(wxTreeCtrl *p, bool connect);
            static void ConnectItemExpanded(wxTreeCtrl *p, bool connect);
            static void ConnectItemExpanding(wxTreeCtrl *p, bool connect);
            static void ConnectItemCollapsed(wxTreeCtrl *p, bool connect);
            static void ConnectItemCollapsing(wxTreeCtrl *p, bool connect);
            static void ConnectSelChanged(wxTreeCtrl *p, bool connect);
            static void ConnectSelChanging(wxTreeCtrl *p, bool connect);
            static void ConnectKeyDown(wxTreeCtrl *p, bool connect);
            static void ConnectItemActivated(wxTreeCtrl *p, bool connect);
            static void ConnectItemRightClick(wxTreeCtrl *p, bool connect);
            static void ConnectItemMiddleClick(wxTreeCtrl *p, bool connect);
            static void ConnectEndDrag(wxTreeCtrl *p, bool connect);
        };

        /**
         * Wrapper class for a treeitem. This is necessary 
         * to protect the JavaScript objects from garbage collection
         */
        class ObjectTreeData : public wxTreeItemData
        {
        public:
	        ObjectTreeData(JSContext *cx, jsval v) : wxTreeItemData()
                                                   , m_cx(cx)
                                                   , m_val(v)
	        {
                if ( JSVAL_IS_GCTHING(m_val) )
                    JS_AddValueRoot(m_cx, &m_val);
	        }

	        virtual ~ObjectTreeData()
	        {
                if ( JSVAL_IS_GCTHING(m_val) )
                    JS_RemoveValueRoot(m_cx, &m_val);
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
