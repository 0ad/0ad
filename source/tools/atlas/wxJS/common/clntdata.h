/*
 * wxJavaScript - clntdata.h
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
 * $Id$
 */
#ifndef _wxjs_clientdata_h
#define _wxjs_clientdata_h

#include <wx/clntdata.h>

namespace wxjs
{
    class JavaScriptClientData : public wxClientData
    {
    public:
        JavaScriptClientData(JSContext *cx, 
                             JSObject *obj, 
                             bool protect, 
                             bool owner = true) : m_cx(cx)
                                                , m_obj(obj)
                                                , m_protected(false)
                                                , m_owner(owner)
        {
          Protect(protect);
        }

        JavaScriptClientData(const JavaScriptClientData &copy) : wxClientData(copy)
        {
          m_cx = copy.m_cx;
          m_obj = copy.m_obj;
          Protect(copy.m_protected);
          SetOwner(copy.m_owner);
        }

        inline bool IsProtected() const
        {
          return m_protected;
        }

        inline bool IsOwner() const
        {
          return m_owner;
        }

        void Protect(bool protect)
        {
          if ( protect == m_protected )
            return; // Don't protect/unprotect twice

          if (    m_protected
               && ! protect )
          {
            JS_RemoveObjectRoot(m_cx, &m_obj);
          }
          else if (    protect
                    && ! m_protected )
          {
            JS_AddObjectRoot(m_cx, &m_obj);
          }
          m_protected = protect;
        }

        virtual ~JavaScriptClientData() 
        {
            Protect(false);

            // When wxJavaScript is not the owner of the object, the
            // private data will be set to NULL, so that the js-destructor
            // doesn't destroy the data twice.
            if ( ! m_owner )
            {
                JS_SetPrivate(m_cx, m_obj, NULL);
            }
        }

        void SetOwner(bool owner) { m_owner = owner; }

        inline JSObject* GetObject() { return m_obj; }
        inline JSContext* GetContext() { return m_cx; }

    private:

        JSContext *m_cx;
        JSObject* m_obj;
        bool m_protected;
        bool m_owner;
    };
};

#endif //  _wxjs_clientdata_h
