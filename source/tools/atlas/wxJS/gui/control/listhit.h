/*
 * wxJavaScript - listhit.h
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
 * $Id: listhit.h 688 2007-04-27 20:45:09Z fbraem $
 */
#ifndef _wxjs_gui_listhit_h
#define _wxjs_gui_listhit_h

#include <wx/listctrl.h>

namespace wxjs
{
    namespace gui
    {
        /**
         * Helper class for returning information for hittest
         */
        class wxListHitTest
        {
        public:
            long GetItem() const
            {
                return m_item;
            }
            long GetFlags() const
            {
                return m_flags;
            }
            friend class ListCtrl;
        private:

            wxListHitTest(long item, int flags) : m_item(item), m_flags(flags)
            {
            }

            long m_item;
            int m_flags;
        };

        class ListHitTest : public ApiWrapper<ListHitTest, wxListHitTest>
        {
        public:
            static bool GetProperty(wxListHitTest *p,
                                    JSContext *cx,
                                    JSObject *obj,
                                    int id,
                                    jsval *vp);

            WXJS_DECLARE_PROPERTY_MAP()
            /**
             * Property Ids.
             */
            enum
            {
                P_ITEM
                , P_FLAGS
            };

            WXJS_DECLARE_CONSTANT_MAP()
        };
    }; // namespace gui
}; // namespace wxjs

#endif //_wxjs_gui_listhit_h
