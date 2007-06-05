#include "precompiled.h"

/*
 * wxJavaScript - dir.cpp
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
 * $Id: dir.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// wxJSDir.cpp

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../common/main.h"

#include "dir.h"
#include "dirtrav.h"

using namespace wxjs;
using namespace wxjs::io;

/***
 * <file>dir</file>
 * <module>io</module>
 * <class name="wxDir">
 *  wxDir is a portable equivalent of Unix open/read/closedir functions which allow 
 *  enumerating of the files in a directory. wxDir can enumerate files as well as directories.
 *  <br /><br />
 *  The following example checks if the temp-directory exists,
 *  and when it does, it retrieves all files with ".tmp" as extension.
 *  <pre><code class="whjs">
 *   if ( wxDir.exists("c:\\temp") )
 *   {
 *     files = wxDir.getAllFiles("c:\\temp", "*.tmp");
 *     for(e in files)
 *       wxMessageBox(files[e]);
 *   }
 *  </code></pre>
 *  See also @wxDirTraverser
 * </class>
 */
WXJS_INIT_CLASS(Dir, "wxDir", 0)

/***
 * <constants>
 *  <type name="wxDirFlags">
 *   <constant name="FILES" />
 *   <constant name="DIRS" />
 *   <constant name="HIDDEN" />
 *   <constant name="DOTDOT" />
 *   <constant name="DEFAULT" />
 *   <desc>Constants used for filtering files. The default: FILES | DIRS | HIDDEN.</desc>
 *  </type>
 * </constants>
 */
WXJS_BEGIN_CONSTANT_MAP(Dir)
    WXJS_CONSTANT(wxDIR_, FILES)
    WXJS_CONSTANT(wxDIR_, DIRS)
    WXJS_CONSTANT(wxDIR_, HIDDEN)
    WXJS_CONSTANT(wxDIR_, DOTDOT)
    WXJS_CONSTANT(wxDIR_, DEFAULT)
WXJS_END_CONSTANT_MAP()

WXJS_BEGIN_STATIC_METHOD_MAP(Dir)
    WXJS_METHOD("getAllFiles", getAllFiles, 1)
    WXJS_METHOD("exists", exists, 1)
WXJS_END_METHOD_MAP()

/***
 * <class_method name="exists">
 *  <function returns="Boolean">
 *   <arg name="DirName" type="String" />
 *  </function>
 *  <desc>
 *   Returns true when the directory exists.
 *  </desc>
 * </class_method>
 */
JSBool Dir::exists(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxString dir;
    FromJS(cx, argv[0], dir);

    *rval = ToJS(cx, wxDir::Exists(dir));
    return JS_TRUE;
}

/***
 * <class_method name="getAllFiles">
 *  <function returns="String Array">
 *   <arg name="DirName" type="String" />
 *   <arg name="FileSpec" type="String" default="" />
 *   <arg name="Flags" type="Integer" default="wxDir.FILES | wxDir.DIRS | wxDir.HIDDEN" />
 *  </function>
 *  <desc>
 *   This function returns an array of all filenames under directory <strong>DirName</strong>
 *   Only files matching <strong>FileSpec</strong> are taken. When an empty spec is given,
 *   all files are given.
 *   <br /><br />
 *   Remark: when wxDir.DIRS is specified in <strong>Flags</strong> then
 *   getAllFiles recurses into subdirectories. So be carefull when using this method
 *   on a root directory.
 *  </desc>
 * </class_method>
 */
JSBool Dir::getAllFiles(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    if ( argc > 3 )
        argc = 3;

    wxString filespec = wxEmptyString;
    int flags = wxDIR_DEFAULT;
    wxArrayString files;

    switch(argc)
    {
    case 3:
        if ( ! FromJS(cx, argv[2], flags) )
            return JS_FALSE;
        // Fall through
    case 2:
        FromJS(cx, argv[1], filespec);
        // Fall through
    default:
        wxString dir;
        FromJS(cx, argv[0], dir);

        wxArrayString files;
        wxDir::GetAllFiles(dir, &files, filespec, flags);

        *rval = ToJS(cx, files);
    }

    return JS_TRUE;
}

/***
 * <ctor>
 *  <function>
 *   <arg name="Directory" type="String" default="null">The name of the directory.</arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxDir object. When no argument is specified, use @wxDir#open
 *   afterwards. When a directory is passed, use @wxDir#isOpened to test for errors.
 *  </desc>
 * </ctor>
 */
wxDir* Dir::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    if ( argc == 0 )
        return new wxDir();

    if ( argc == 1 )
    {
        wxString dir;
        FromJS(cx, argv[0], dir);
        return new wxDir(dir);
    }

    return NULL;
}

/***
 * <properties>
 *  <property name="opened" type="Boolean" readonly="Y">
 *   Returns true when the directory was opened by calling @wxDir#open.
 *  </property>
 *  <property name="name" type="String" readonly="Y">
 *   Returns the name of the directory itself. The returned string 
 *   does not have the trailing path separator (slash or backslash).
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(Dir)
    WXJS_READONLY_PROPERTY(P_OPENED, "opened")
    WXJS_READONLY_PROPERTY(P_NAME, "name")
WXJS_END_PROPERTY_MAP()

bool Dir::GetProperty(wxDir *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    if ( id == P_OPENED )
        *vp = ToJS(cx, p->IsOpened());
	else if ( id == P_NAME )
		*vp = ToJS(cx, p->GetName());
    return true;
}

WXJS_BEGIN_METHOD_MAP(Dir)
    WXJS_METHOD("getFirst", getFirst, 0)
    WXJS_METHOD("getNext", getNext, 0)
    WXJS_METHOD("hasFiles", hasFiles, 0)
    WXJS_METHOD("hasSubDirs", hasSubDirs, 0)
    WXJS_METHOD("open", open, 1)
    WXJS_METHOD("traverse", traverse, 1)
WXJS_END_METHOD_MAP()

/***
 * <method name="getFirst">
 *  <function returns="String">
 *   <arg name="FileSpec" type="String" default="" />
 *   <arg name="Flags" type="Integer" default="wxDir.FILES | wxDir.DIRS | wxDir.HIDDEN" />
 *  </function>
 *  <desc>
 *   Starts the enumerating. All files matching <strong>FileSpec</strong> 
 *   (or all files when not specified or empty) and <strong>Flags</strong> will be
 *   enumerated. Use @wxDir#getNext for the next file.
 *   <br /><br />
 *   An empty String is returned when there's no file found.
 *  </desc>
 * </method>
 */
JSBool Dir::getFirst(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxDir *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    if ( argc > 2 )
    {
        return JS_FALSE;
    }

    wxString filespec = wxEmptyString;
    int flags = wxDIR_DEFAULT;

    switch(argc)
    {
    case 2:
        if ( ! FromJS(cx, argv[1], flags) )
            return JS_FALSE;
        // Fall through
    case 1:
        FromJS(cx, argv[0], filespec);
        // Fall through
    default:
        wxString fileName = wxEmptyString;
        p->GetFirst(&fileName, filespec, flags);
        *rval = ToJS(cx, fileName);
    }

    return JS_TRUE;
}

/***
 * <method name="getNext">
 *  <function returns="String" />
 *  <desc>
 *   Continues to enumerate files and/or directories which satisfy
 *   the criteria specified in @wxDir#getFirst. An empty String
 *   is returned when there are no more files.
 *  </desc>
 * </method>
 */
JSBool Dir::getNext(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxDir *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxString next = wxEmptyString;
    p->GetNext(&next);
    *rval = ToJS(cx, next);

    return JS_TRUE;
}

/***
 * <method name="hasFiles">
 *  <function returns="Boolean">
 *   <arg name="FileSpec" type="String" default="" />
 *  </function>
 *  <desc>
 *   Returns true if the directory contains any files matching the given <strong>FileSpec</strong>.
 *   When no argument is given, it will look if there are files in the directory.
 *  </desc>
 * </method>
 */
JSBool Dir::hasFiles(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxDir *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxString filespec = wxEmptyString;
    if ( argc == 1 )
        FromJS(cx, argv[0], filespec);

    *rval = ToJS(cx, p->HasFiles(filespec));

    return JS_TRUE;
}

/***
 * <method name="hasSubDirs">
 *  <function returns="Boolean">
 *   <arg name="DirSpec" type="String" default="" />
 *  </function>
 *  <desc>
 *   Returns true if the directory contains any subdirectories matching the given <strong>DirSpec</strong>.
 *   When no argument is given, it will look if there are any subdirectories in the directory.
 *  </desc>
 * </method>
 */
JSBool Dir::hasSubDirs(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxDir *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxString dirspec = wxEmptyString;
    if ( argc == 1 )
        FromJS(cx, argv[0], dirspec);

    *rval = ToJS(cx, p->HasSubDirs(dirspec));

    return JS_TRUE;
}

/***
 * <method name="open">
 *  <function returns="Boolean">
 *   <arg name="Directory" type="String" />
 *  </function>
 *  <desc>
 *   Open directory for enumerating files and subdirectories. Returns true
 *   on success, or false on failure.
 *  </desc>
 * </method>
 */
JSBool Dir::open(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxDir *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxString dir;
    FromJS(cx, argv[0], dir);

    *rval = ToJS(cx, p->Open(dir));
    return JS_TRUE;
}

/***
 * <method name="traverse">
 *  <function returns="Integer">
 *   <arg name="Traverser" type="@wxDirTraverser" />
 *   <arg name="FileSpec" type="String" default="" />
 *   <arg name="Flags" type="Integer" default="wxDir.FILES | wxDir.DIRS | wxDir.HIDDEN" />
 *  </function>
 *  <desc>
 *   Open directory for enumerating files and subdirectories. Returns true
 *   on success, or false on failure. For an example see @wxDirTraverser.
 *  </desc>
 * </method>
 */
JSBool Dir::traverse(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxDir *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    if ( argc > 3 )
        argc = 3;

    int flags = wxDIR_DEFAULT;
    wxString filespec = wxEmptyString;

    switch(argc)
    {
    case 3:
        if ( ! FromJS(cx, argv[2], flags) )
            return JS_FALSE;
        // Fall through
    case 2:
        FromJS(cx, argv[1], filespec);
    default:
        DirTraverser *traverser = DirTraverser::GetPrivate(cx, argv[0]);
        if ( traverser == NULL )
        {
            return JS_FALSE;
        }

        *rval = ToJS(cx, p->Traverse(*traverser, filespec, flags));
    }

    return JS_TRUE;
}
