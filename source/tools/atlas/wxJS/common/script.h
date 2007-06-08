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
 * $Id: script.h 598 2007-03-07 20:13:28Z fbraem $
 */
#ifndef _wxjs_script_h
#define _wxjs_script_h

#include <wx/string.h>

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
      
      wxString GetName() const
      {
          return m_name; 
      }
  
      void SetName(const wxString &name)
      {
          m_name = name;
      }
      
      virtual wxString GetSource() const;
  
      wxString GetFile() const 
      {
          return m_file; 
      }
      
    // Sets the filename and reads the source from it
    virtual void SetFile(const wxString &file, wxMBConv &conv = wxConvUTF8);
  
    // Sets the filename
    virtual void SetFilename(const wxString &name) { m_file = name; }
      
  private:
      wxString m_file;
      wxString m_source;
      wxString m_name;
  };
}; // namespace wxjs
#endif // _wxjs_script_h
