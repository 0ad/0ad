/*
 * wxJavaScript - script.h
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
 * $Id: script.h 806 2007-07-05 20:17:47Z fbraem $
 */
#ifndef _wxjs_script_h
#define _wxjs_script_h

#include <wx/string.h>

class wxFileName;

namespace wxjs
{
  class ScriptSource
  {
  public:
    ScriptSource()
    {
    }

    ScriptSource(const ScriptSource &copy);

    virtual ~ScriptSource()
    {
    }

    virtual void SetSource(const wxString &source);
    
    bool HasSource() const 
    {
      return m_source.length() > 0;
    }

    virtual wxString GetSource() const;

    wxString GetFileName() const 
    {
        return m_file; 
    }

    wxString GetPath() const
    {
        return m_path;
    }

    void SetPath(const wxString &path)
    {
      m_path = path;
    }

    // Sets the filename and reads the source from it
    void SetFile(const wxFileName &file, wxMBConv &conv = wxConvUTF8);
  
  private:
      wxString m_file;
      wxString m_path;
      wxString m_source;
  };
}; // namespace wxjs
#endif // _wxjs_script_h
