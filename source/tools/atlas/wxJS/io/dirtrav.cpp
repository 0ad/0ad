#include "precompiled.h"

/*
 * wxJavaScript - dirtrav.cpp
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
 * $Id: dirtrav.cpp 716 2007-05-20 17:57:22Z fbraem $
 */
// wxJSDirTraverser.cpp

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../common/main.h"
#include "../common/jsutil.h"

#include "dirtrav.h"

using namespace wxjs;
using namespace wxjs::io;

DirTraverser::DirTraverser()
        : wxDirTraverser()
{
}

wxDirTraverseResult DirTraverser::OnFile(const wxString& filename)
{
  JavaScriptClientData *data
    = dynamic_cast<JavaScriptClientData*>(GetClientObject());
	JSContext *cx = data->GetContext();
	jsval fval;
	if ( GetFunctionProperty(cx, data->GetObject(), "onFile", &fval) == JS_TRUE )
	{
		jsval rval;
		jsval argv[] = { ToJS(cx, filename) };
		if ( JS_CallFunctionValue(cx, data->GetObject(), fval, 1, argv, &rval) == JS_TRUE )
		{
			int result;
			if ( FromJS(cx, rval, result ) )
			{
                return (wxDirTraverseResult) result;
			}
		}
        else
        {
            if ( JS_IsExceptionPending(cx) )
            {
                JS_ReportPendingException(cx);
            }
        }
		return wxDIR_STOP;
	}

    // No function set, we return continue
	return wxDIR_CONTINUE;
}

wxDirTraverseResult DirTraverser::OnDir(const wxString& filename)
{
  JavaScriptClientData *data
    = dynamic_cast<JavaScriptClientData*>(GetClientObject());

	JSContext *cx = data->GetContext();
	jsval fval;
	if ( GetFunctionProperty(cx, data->GetObject(), "onDir", &fval) == JS_TRUE )
	{
		jsval rval;
		jsval argv[] = { ToJS(cx, filename) };
		if ( JS_CallFunctionValue(cx, data->GetObject(), fval, 1, argv, &rval) == JS_TRUE )
		{
			int result;
			if ( FromJS(cx, rval, result) )
			{
                return (wxDirTraverseResult) result;
			}
		}
        else
        {
            if ( JS_IsExceptionPending(cx) )
            {
                JS_ReportPendingException(cx);
            }
        }
		return wxDIR_STOP;
	}

    // No function set, we return continue
	return wxDIR_CONTINUE;
}

/***
 * <file>dirtrav</file>
 * <module>io</module>
 * <class name="wxDirTraverser">
 *  wxDirTraverser can be used to traverse into directories to retrieve filenames and subdirectories.
 *  <br /><br />
 *  See also @wxDir
 *  <br /><br />
 *  The following example counts all subdirectories of the temporary directory.
 *  Counting the direct subdirectories of temp is possible by returning
 *  IGNORE in @wxDirTraverser#onDir.
 *  <pre><code class="whjs">
 *  var dir = new wxDir("c:\\temp");
 *  var trav = new wxDirTraverser();
 *  subdir = 0; // Don't use var, it doesn't seem to work with closures
 *  
 *  trav.onDir = function(filename)
 *  {
 *    subdir = subdir + 1;
 *    return wxDirTraverser.CONTINUE;
 *  }
 * 
 *  dir.traverse(trav);
 *
 *  wxMessageBox("Number of subdirectories: " + subdir);
 *  </code></pre>
 * </class>
 */
WXJS_INIT_CLASS(DirTraverser, "wxDirTraverser", 0)

/***
 * <constants>
 *  <type name="wxDirTraverseResult">
 *   <constant name="IGNORE" />
 *   <constant name="STOP" />
 *   <constant name="CONTINUE" />
 *  </type>
 * </constants>
 */
WXJS_BEGIN_CONSTANT_MAP(DirTraverser)
    WXJS_CONSTANT(wxDIR_, IGNORE)
    WXJS_CONSTANT(wxDIR_, STOP)
    WXJS_CONSTANT(wxDIR_, CONTINUE)
WXJS_END_CONSTANT_MAP()

/***
 * <properties>
 *  <property name="onFile" type="Function">
 *   Function which will be called for each file. The function
 *   gets a filename and should return a constant @wxDirTraverser#wxDirTraverseResult except 
 *   for IGNORE. When no function is set, it will return 
 *   CONTINUE. This makes it possible to enumerate the subdirectories.
 *  </property>
 *  <property name="onDir" type="Function">
 *   Function which will be called for each directory. The function
 *   gets a directoryname and should return a constant @wxDirTraverser#wxDirTraverseResult.
 *   It may return STOP to abort traversing completely, IGNORE to skip this directory but continue with 
 *   others or CONTINUE to enumerate all files and subdirectories
 *   in this directory. When no function is set, it will return CONTINUE.
 *   This makes it possible to enumerate all files.
 *  </property>
 * </properties>
 */

/***
 * <ctor>
 *  <function />
 *  <desc>
 *   Constructs a new wxDirTraverser object.
 *  </desc>
 * </ctor>
 */
DirTraverser* DirTraverser::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    DirTraverser *p = new DirTraverser();
    p->SetClientObject(new JavaScriptClientData(cx, obj, false, true));
    return p;
}
