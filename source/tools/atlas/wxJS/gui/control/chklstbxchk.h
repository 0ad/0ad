/*
 * wxJavaScript - chklstbxchk.h
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
 * $Id: chklstbxchk.h 711 2007-05-14 20:59:29Z fbraem $
 */
#ifndef _wxjs_gui_chklstbxchk_h
#define _wxjs_gui_chklstbxchk_h

namespace wxjs
{
    namespace gui
    {
      class CheckListBoxItem : public ApiWrapper<CheckListBoxItem, Index>
      {
      public:
        static bool GetProperty(Index *p,
                                JSContext *cx,
                                JSObject *obj,
                                int id,
                                jsval *vp);
        static bool SetProperty(Index *p,
                                JSContext *cx,
                                JSObject *obj,
                                int id, 
                                jsval *vp);
      };
    }; // namespace gui
}; // namespace wxjs

#endif // _wxjs_gui_chklstbxchk_h
