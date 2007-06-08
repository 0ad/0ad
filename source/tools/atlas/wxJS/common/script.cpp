#include "precompiled.h"

/*
 * wxJavaScript - script.cpp
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
 * $Id: script.cpp 631 2007-03-23 20:14:28Z fbraem $
 */
#include <wx/txtstrm.h>
#include <wx/wfstream.h>
#include <wx/textfile.h>
#include "script.h"

using namespace wxjs;

wxString ScriptSource::GetSource() const
{
    return m_source;
}

void ScriptSource::SetFile(const wxString &file, wxMBConv &conv)
{
	m_file = file;
    wxFileInputStream fis(file);
    if ( fis.IsOk() )
    {
	    wxTextInputStream tis(fis, wxT("\t"), conv);
        bool first = true;
        while(! fis.Eof())
        {
            wxString line = tis.ReadLine();
            if (    first 
                 && line.StartsWith(wxT("#!")) ) // The first line can hold a shebang
            {
                first = false;
                continue;
            }
            first = false;
            m_source.Append(line);
            m_source.Append(wxTextFile::GetEOL());
        }
    }
}

ScriptSource::ScriptSource(const ScriptSource &copy) : m_file(copy.m_file)
                                                     , m_source(copy.m_source)
                                                     , m_name(copy.m_name)
{
}

void ScriptSource::SetSource(const wxString &source)
{
    m_source.append(source);
}
