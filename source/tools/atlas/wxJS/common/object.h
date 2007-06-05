/*
 * wxJavaScript - object.h
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
 * $Id: object.h 598 2007-03-07 20:13:28Z fbraem $
 */
/**
 * (c) 2001-2002 Franky Braem (S.A.W.)
 *
 * This file is part of wxJS. wxJS ports wxWindows to JavaScript
 * 
 * File      : object.h
 * Desc.     : Keeps a pointer to the associated JSObject.
 * Created   : 06-01-2002
 */

#ifndef _Object_H
#define _Object_H

namespace wxjs
{
    class Object
    {
    public:
	    /**
	     * Constructor
	     */

	    Object() : m_obj(NULL), m_cx(NULL)
	    {
	    }

	    Object(JSObject *obj, JSContext *cx);

        Object(const Object &copy);

	    /**
	     * Destructor
	     */	
	    virtual ~Object();

	    /**
	     * Returns the JavaScript object
	     */
	    inline JSObject* GetObject() const
	    {
		    return m_obj;
	    }

	    inline JSContext *GetContext() const
	    {
		    return m_cx;
	    }
    	
	    inline void SetObject(JSObject *obj)
	    {
		    m_obj = obj;
	    }

    private:

	    // The actual JSObject. We don't have to root this
	    // because it is stored in the private data of the JSObject.
	    // The private data will only be destroyed together with the object
	    // when it is garbage collected.

	    JSObject *m_obj;
	    JSContext *m_cx;
    };
}; // namespace wxjs
#endif
