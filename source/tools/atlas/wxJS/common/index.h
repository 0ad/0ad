/*
 * wxJavaScript - index.h
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
 * $Id: index.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _wxJS_Index_h
#define _wxJS_Index_h

/////////////////////////////////////////////////////////////////////////////
// Name:        index.h
// Purpose:		wxJS_Index is used to keep information about indexed objects
// Author:      Franky Braem
// Modified by:
// Created:     23.09.02
// Copyright:   (c) 2001-2002 Franky Braem
// Licence:     LGPL
/////////////////////////////////////////////////////////////////////////////

namespace wxjs
{
    class Index
    {
    public:

        Index(int idx) : m_index(idx)
        {
        }

        inline int GetIndex() const
        {
            return m_index;
        }

        inline void SetIndex(int idx)
        {
            m_index = idx;
        }

    private:
        int m_index;
    };
};
#endif // _wxJS_Index_h
