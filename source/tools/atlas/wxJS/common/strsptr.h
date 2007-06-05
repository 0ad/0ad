/*
 * wxJavaScript - strsptr.h
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
 * $Id: strsptr.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _STRINGSPTR_H
#define _STRINGSPTR_H

/////////////////////////////////////////////////////////////////////////////
// Name:        strsptr.h
// Purpose:     Proxy for a pointer to an array of wxString elements.
//              Use this class to convert a JavaScript array of strings into
//              a wxString array.
//
//              The pointer returned to the array is only valid in the scoop
//              of this object. When the object goes out of scoop, the array
//              will be destroyed.
//
// Author:      Franky Braem
// Modified by:
// Created:     11.09.02
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

namespace wxjs
{

    class StringsPtr
    {
    public:

	    StringsPtr() : m_strings(NULL), m_count(0)
	    {
	    }

	    virtual ~StringsPtr()
	    {
		    delete[] m_strings;
	    }

	    unsigned int GetCount() const
	    {
		    return m_count;
	    }

	    const wxString* GetStrings() const
	    {
		    return m_strings;
	    }

    private:

	    wxString& operator[](unsigned int i)
	    {
		    return m_strings[i];
	    }

	    void Allocate(unsigned int count)
	    {
		    if ( m_strings != NULL )
			     delete[] m_strings;

		    m_count = count;
		    m_strings = new wxString[m_count];
	    }

        template<typename T> friend bool FromJS(JSContext*cx, jsval v, T &to);

	    // Avoid copying
	    StringsPtr(const StringsPtr&);
    	
	    wxString *m_strings;
	    unsigned int m_count;
    };
};

#endif //_STRINGSPTR_H
