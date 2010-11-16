#include "precompiled.h"

/*
 * wxJavaScript - filename.cpp
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
 * $Id: filename.cpp 730 2007-06-05 19:29:52Z fbraem $
 */
// filename.cpp

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../common/main.h"
#include "../common/jsutil.h"

#include "filename.h"
#include "file.h"

using namespace wxjs;
using namespace wxjs::io;

/***
 * <file>filename</file>
 * <module>io</module>
 * <class name="wxFileName" version="0.8.2">
 *  wxFileName encapsulates a file name. This class serves two purposes: first, it provides 
 *  the functions to split the file names into components and to recombine these components 
 *  in the full file name which can then be passed to the OS file functions (and wxWindows 
 *  functions wrapping them). Second, it includes the functions for working with the files 
 *  itself. 
 *  <br /><br />
 *  <b>Remark:</b> To change the file data you should use @wxFile class instead. 
 *  wxFileName provides functions for working with the file attributes. 
 *  <br /><br />
 *  <b>Remark:</b> Some of the properties of wxFileName causes IO processing. For example: 
 *  @wxFileName#accessTime will execute an IO API to retrieve the last access time
 *  of the file. This can result in performance issues when you use this property
 *  frequently. The best solution for this is to put the property in a variable and
 *  use that variable instead of the property:
 *  <pre><code class="whjs">
 *   // Do this
 *   var fileName = new wxFileName("test.txt");
 *   var acc = fileName.accessTime;
 *   // From now on use var acc instead of fileName.accessTime
 *  </code></pre>
 * </class>
 */
WXJS_INIT_CLASS(FileName, "wxFileName", 0)

/***
 * <constants>
 *  <type name="wxPathFormat">
 *   <constant name="NATIVE" />
 *   <constant name="UNIX" />
 *   <constant name="MAC" />
 *   <constant name="DOS" />
 *   <constant name="VMS" />
 *   <constant name="BEOS" />
 *   <constant name="WIN" />
 *   <constant name="OS2" />
 *   <desc>
 *    wxPathFormat is ported as a separate JavaScript object.
 *    Many wxFileName methods accept the path format argument which is 
 *    wxPathFormat.NATIVE by default meaning to use the path format native for
 *    the current platform.
 *    <br /><br />
 *    The path format affects the operation of wxFileName functions in 
 *    several ways: first and foremost, it defines the path separator character 
 *    to use, but it also affects other things such as whether the path has 
 *    the drive part or not.
 *   </desc>
 *  </type>
 *  <type name="wxPathNormalize">
 *   <constant name="ENV_VARS">Replace env vars with their values</constant>
 *   <constant name="DOTS">Squeeze all .. and . and prepend cwd</constant>
 *   <constant name="TILDE">Unix only: replace ~ and ~user</constant>
 *   <constant name="CASE">Case insensitive => tolower</constant>
 *   <constant name="ABSOLUTE">Make the path absolute</constant>
 *   <constant name="LONG">Make the path the long form</constant>
 *   <constant name="SHORTCUT">Resolve if it is a shortcut (Windows only)</constant>
 *   <constant name="ALL" />
 *   <desc>
 *    wxPathNormalize is ported as a separate JavaScript object.
 *    The kind of normalization to do with the file name: these values can be or'd
 *    together to perform several operations at once in @wxFileName#normalize.
 *   </desc>
 *  </type>
 *  <type name="wxPathGet">
 *   <constant name="VOLUME">Return the path with the volume (does nothing for the filename formats without volumes)</constant>
 *   <constant name="SEPARATOR">Return the path with the trailing separator, if this flag is not given 
 *    there will be no separator at the end of the path.</constant>
 *   <desc>
 *    wxPathGet is ported as a separate JavaScript object.
 *   </desc>
 *  </type>
 *  <type name="wxPathMkdir">
 *   <constant name="FULL">Try to create each directory in the path and also don't return an error
 *    if the target directory already exists.</constant>
 *   <desc>
 *    wxPathMkDir is ported as a separate JavaScript object.
 *   </desc>
 *  </type>
 * </constants>
 */
void FileName::InitClass(JSContext *cx, JSObject *obj, JSObject *proto)
{
    JSConstDoubleSpec wxPathFormatMap[] = 
    {
        WXJS_CONSTANT(wxPATH_, NATIVE)
        WXJS_CONSTANT(wxPATH_, UNIX)
        WXJS_CONSTANT(wxPATH_, MAC)
        WXJS_CONSTANT(wxPATH_, DOS)
        WXJS_CONSTANT(wxPATH_, VMS)
        WXJS_CONSTANT(wxPATH_, BEOS)
        WXJS_CONSTANT(wxPATH_, WIN)
        WXJS_CONSTANT(wxPATH_, OS2)
	    { 0 }
    };

    JSObject *constObj = JS_DefineObject(cx, obj, "wxPathFormat", 
									 	 NULL, NULL,
							             JSPROP_READONLY | JSPROP_PERMANENT);
    JS_DefineConstDoubles(cx, constObj, wxPathFormatMap);

    JSConstDoubleSpec wxPathNormalizeMap[] = 
    {
        WXJS_CONSTANT(wxPATH_NORM_, ENV_VARS)
        WXJS_CONSTANT(wxPATH_NORM_, DOTS)
        WXJS_CONSTANT(wxPATH_NORM_, TILDE)
        WXJS_CONSTANT(wxPATH_NORM_, CASE)
        WXJS_CONSTANT(wxPATH_NORM_, ABSOLUTE)
        WXJS_CONSTANT(wxPATH_NORM_, LONG)
        WXJS_CONSTANT(wxPATH_NORM_, SHORTCUT)
        WXJS_CONSTANT(wxPATH_NORM_, ALL)
	    { 0 }
    };
    constObj = JS_DefineObject(cx, obj, "wxPathNormalize", 
		                       NULL, NULL,
							   JSPROP_READONLY | JSPROP_PERMANENT);
    JS_DefineConstDoubles(cx, constObj, wxPathNormalizeMap);

    JSConstDoubleSpec wxPathGetMap[] = 
    {
        WXJS_CONSTANT(wxPATH_GET_, VOLUME)
        WXJS_CONSTANT(wxPATH_GET_, SEPARATOR)
	    { 0 }
    };
    constObj = JS_DefineObject(cx, obj, "wxPathGet", 
	 			               NULL, NULL,
							   JSPROP_READONLY | JSPROP_PERMANENT);
    JS_DefineConstDoubles(cx, constObj, wxPathGetMap);

    JSConstDoubleSpec wxPathMkdirMap[] = 
    {
        WXJS_CONSTANT(wxPATH_MKDIR_, FULL)
	    { 0 }
    };
    constObj = JS_DefineObject(cx, obj, "wxPathMkdir", 
	 			               NULL, NULL,
							   JSPROP_READONLY | JSPROP_PERMANENT);
    JS_DefineConstDoubles(cx, constObj, wxPathMkdirMap);
}

/***
 * <class_properties>
 *	<property name="cwd" type="String">Get/Set the current working directory.</property>
 *	<property name="homeDir" type="String">Get/Set the home directory.</property>
 *  <property name="pathSeparator" type="String" readonly="Y">
 *   Returns the usually used path separator for the native platform.
 *   For all platforms but DOS and Windows there is only one path separator anyhow,
 *   but for DOS and Windows there are two of them.
 *  </property>
 *  <property name="pathSeparators" type="String" readonly="Y"> 
 *   Gets the path separator of the native platform
 *  </property>
 *  <property name="volumeSeparator" type="String">
 *   Gets the string separating the volume from the path for the native format.
 *  </property>
 * </class_properties>
 */
WXJS_BEGIN_STATIC_PROPERTY_MAP(FileName)
  WXJS_STATIC_PROPERTY(P_CWD, "cwd")
  WXJS_STATIC_PROPERTY(P_HOME_DIR, "homeDir")
  WXJS_READONLY_STATIC_PROPERTY(P_PATH_SEPARATOR, "pathSeparator")
  WXJS_READONLY_STATIC_PROPERTY(P_PATH_SEPARATORS, "pathSeparators")
  WXJS_READONLY_STATIC_PROPERTY(P_VOLUME_SEPARATOR, "volumeSeparator")
WXJS_END_PROPERTY_MAP()

bool FileName::GetStaticProperty(JSContext *cx, int id, jsval *vp)
{
    switch(id)
    {
    case P_CWD:
        *vp = ToJS(cx, wxFileName::GetCwd());
        break;
    case P_HOME_DIR:
        *vp = ToJS(cx, wxFileName::GetHomeDir());
        break;
    case P_PATH_SEPARATOR:
        *vp = ToJS(cx, wxString(wxFileName::GetPathSeparator()));
        break;
    case P_PATH_SEPARATORS:
        *vp = ToJS(cx, wxFileName::GetPathSeparators());
        break;
    case P_VOLUME_SEPARATOR:
        *vp = ToJS(cx, wxFileName::GetVolumeSeparator());
        break;
    }
    return true;
}

bool FileName::SetStaticProperty(JSContext *cx, int id, jsval *vp)
{
    switch(id)
    {
    case P_CWD:
        {
            wxString cwd;
            FromJS(cx, *vp, cwd);
            wxFileName::SetCwd(cwd);
        }
	}
	return JS_TRUE;
}

/***
 * <properties>
 *  <property name="accessTime" type="Date">Get/Set the last access time</property>
 *  <property name="createTime" type="Date">Get/Set the creation time</property>
 *  <property name="dirCount" type="Integer" readonly="Y">Returns the number of directories in the file name.</property>
 *  <property name="dirExists" type="Boolean" readonly="Y">Returns true when the directory exists.</property>
 *  <property name="ext" type="String">Get/Set the extension</property>
 *  <property name="fileExists" type="Boolean" readonly="Y">Returns true when the file exists.</property>
 *  <property name="fullName" type="String">Get/Set the full name (including extension but without directories).</property>
 *  <property name="fullPath" type="String">Returns the full path with name and extension for the native platform.</property>
 *  <property name="hasExt" type="boolean" readonly="Y">Returns true when an extension is present.</property>
 *  <property name="isDir" type="boolean" readonly="Y">Returns true if this object represents a directory, false otherwise (i.e. if it is a file).
 *  This property doesn't test whether the directory or file really exists, 
 *  you should use @wxFileName#dirExists or @wxFileName#fileExists for this.</property>
 *  <property name="longPath" type="String" readonly="Y">Return the long form of the path (returns identity on non-Windows platforms).</property>
 *  <property name="modificationTime" type="Date">Get/Set the time the file was last modified</property>
 *  <property name="ok" type="Boolean" readonly="Y">
 *    Returns true if the filename is valid, false if it is not initialized yet. 
 *    The assignment functions and @wxFileName#clear may reset the object to
 *    the uninitialized, invalid state.</property>
 *  <property name="shortPath" type="String" readonly="Y">
 *   Return the short form of the path (returns identity on non-Windows platforms).
 *  </property>
 *  <property name="volume" type="String">Get/Set the volume name for this file name.</property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(FileName)
    WXJS_READONLY_PROPERTY(P_DIR_COUNT, "dirCount")
    WXJS_READONLY_PROPERTY(P_DIRS, "dirs")
    WXJS_READONLY_PROPERTY(P_DIR_EXISTS, "dirExists")
    WXJS_PROPERTY(P_EXT, "ext")
    WXJS_READONLY_PROPERTY(P_FILE_EXISTS, "fileExists")
    WXJS_READONLY_PROPERTY(P_FULL_NAME, "fullName")
    WXJS_READONLY_PROPERTY(P_FULL_PATH, "fullPath")
    WXJS_READONLY_PROPERTY(P_HAS_EXT, "hasExt")
    WXJS_READONLY_PROPERTY(P_LONG_PATH, "longPath")
    WXJS_PROPERTY(P_MODIFICATION_TIME, "modificationTime")
    WXJS_PROPERTY(P_NAME, "name")
    WXJS_READONLY_PROPERTY(P_SHORT_PATH, "shortPath")
    WXJS_PROPERTY(P_ACCESS_TIME, "accessTime")
    WXJS_PROPERTY(P_CREATE_TIME, "createTime")
    WXJS_PROPERTY(P_VOLUME, "volume")
    WXJS_READONLY_PROPERTY(P_OK, "ok")
    WXJS_READONLY_PROPERTY(P_IS_DIR, "isDir")
WXJS_END_PROPERTY_MAP()

bool FileName::GetProperty(wxFileName *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch (id)
    {
    case P_DIR_COUNT:
        *vp = ToJS(cx, p->GetDirCount());
        break;
    case P_DIR_EXISTS:
        *vp = ToJS(cx, p->DirExists());
        break;
    case P_DIRS:
        *vp = ToJS(cx, p->GetDirs());
        break;
    case P_EXT:
        *vp = ToJS(cx, p->GetExt());
        break;
    case P_FILE_EXISTS:
        *vp = ToJS(cx, p->FileExists());
        break;
    case P_FULL_NAME:
        *vp = ToJS(cx, p->GetFullName());
        break;
    case P_FULL_PATH:
        *vp = ToJS(cx, p->GetFullPath());
        break;
    case P_HAS_EXT:
        *vp = ToJS(cx, p->HasExt());
        break;
    case P_LONG_PATH:
        *vp = ToJS(cx, p->GetLongPath());
        break;
    case P_MODIFICATION_TIME:
        *vp = ToJS(cx, p->GetModificationTime());
        break;
    case P_NAME:
        *vp = ToJS(cx, p->GetName());
        break;
    case P_SHORT_PATH:
        *vp = ToJS(cx, p->GetShortPath());
        break;
    case P_ACCESS_TIME:
        {
            wxDateTime date;
            p->GetTimes(&date, NULL, NULL);
            *vp = ToJS(cx, date);
            break;
        }
    case P_CREATE_TIME:
        {
            wxDateTime date;
            p->GetTimes(NULL, NULL, &date);
            *vp = ToJS(cx, date);
            break;
        }
    case P_VOLUME:
        *vp = ToJS(cx, p->GetVolume());
        break;
    case P_OK:
        *vp = ToJS(cx, p->IsOk());
        break;
    case P_IS_DIR:
        *vp = ToJS(cx, p->IsDir());
        break;
    }
    return true;
}

bool FileName::SetProperty(wxFileName *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch (id)
    {
    case P_EXT:
        {
            wxString ext;
            FromJS(cx, *vp, ext);
            p->SetExt(ext);
            break;
        }
    case P_FULL_NAME:
        {
            wxString fullName;
            FromJS(cx, *vp, fullName);
            p->SetFullName(fullName);
            break;
        }
    case P_NAME:
        {
            wxString name;
            FromJS(cx, *vp, name);
            p->SetName(name);
            break;
        }
    case P_ACCESS_TIME:
        {
            wxDateTime date;
            if ( FromJS(cx, *vp, date) )
                p->SetTimes(&date, NULL, NULL);
            break;
        }
    case P_CREATE_TIME:
        {
            wxDateTime date;
            if ( FromJS(cx, *vp, date) )
                p->SetTimes(NULL, NULL, &date);
            break;
        }
    case P_MODIFICATION_TIME:
        {
            wxDateTime date;
            if ( FromJS(cx, *vp, date) )
                p->SetTimes(NULL, &date, NULL);
            break;
        }
    case P_VOLUME:
        {
            wxString vol;
            FromJS(cx, *vp, vol);
            p->SetVolume(vol);
            break;
        }
    }
    return true;
}

/***
 * <ctor>
 *  <function />
 *  <function>
 *   <arg name="FileName" type="wxFileName" />
 *  </function>
 *  <function>
 *   <arg name="FullPath" type="String">
 *    A full path. When it terminates with a pathseparator, a directory path is constructed,
 *    otherwise a filename and extension is extracted from it.
 *   </arg>
 *   <arg name="Format" type="@wxFileName#wxPathFormat" default="wxPathFormat.NATIVE" />
 *  </function>
 *  <function>
 *   <arg name="Path" type="String">A directory name</arg>
 *   <arg name="Name" type="String">A filename</arg>
 *   <arg name="Format" type="@wxFileName#wxPathFormat" default="wxPathFormat.NATIVE" />
 *  </function>
 *  <function>
 *   <arg name="Path" type="String">A directory name</arg>
 *   <arg name="Name" type="String">A filename</arg>
 *   <arg name="Ext" type="String">An extension</arg>
 *   <arg name="Format" type="@wxFileName#wxPathFormat" default="wxPathFormat.NATIVE" />
 *  </function>
 *  <function>
 *   <arg name="Volume" type="String">A volume name</arg>
 *   <arg name="Path" type="String">A directory name</arg>
 *   <arg name="Name" type="String">A filename</arg>
 *   <arg name="Ext" type="String">An extension</arg>
 *   <arg name="Format" type="@wxFileName#wxPathFormat" default="wxPathFormat.NATIVE" />
 *  </function>
 *  <desc>
 *   Constructs a new wxFileName object.
 *  </desc>
 * </ctor>
 */
wxFileName* FileName::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    if ( argc == 0 )
        return new wxFileName();

    if ( argc == 1 )
    {
        if ( FileName::HasPrototype(cx, argv[0]) )
        {
            return new wxFileName(*GetPrivate(cx, argv[0], false));
        }

        wxString filename;
        FromJS(cx, argv[0], filename);
        return new wxFileName(filename);
    }

    wxFileName *p = NULL;

    // Argc > 1
    int format = wxPATH_NATIVE;
    uintN stringCount = argc;
    if ( JSVAL_IS_NUMBER(argv[argc-1]) )
    {
        FromJS(cx, argv[argc-1], format);
        stringCount--;
    }

    wxString *str = new wxString[stringCount];
    for(uintN i = 0; i < stringCount; i++)
        FromJS(cx, argv[i], str[i]);

    switch(stringCount)
    {
    case 1:
        p = new wxFileName(str[0], (wxPathFormat) format);
        break;
    case 2:
        p = new wxFileName(str[0], str[1], (wxPathFormat) format);
        break;
    case 3:
        p = new wxFileName(str[0], str[1], str[2], (wxPathFormat) format);
        break;
    case 4:
        p = new wxFileName(str[0], str[1], str[2], str[3], (wxPathFormat) format);
        break;
    }

    delete[] str;

    return p;
}

WXJS_BEGIN_METHOD_MAP(FileName)
    WXJS_METHOD("appendDir", appendDir, 1)
    WXJS_METHOD("assign", assign, 1)
    WXJS_METHOD("assignDir", assignDir, 1)
    WXJS_METHOD("assignHomeDir", assignHomeDir, 0)
    WXJS_METHOD("assignTempFileName", assignTempFileName, 1)
    WXJS_METHOD("clear", clear, 0)
    WXJS_METHOD("getFullPath", getFullPath, 0)
    WXJS_METHOD("getPath", getPath, 0)
    WXJS_METHOD("getPathSeparator", getPathSeparator, 0)
    WXJS_METHOD("getPathSeparators", getPathSeparators, 0)
    WXJS_METHOD("getTimes", getTimes, 3)
    WXJS_METHOD("setTimes", setTimes, 3)
    WXJS_METHOD("getVolumeSeparator", getVolumeSeparator, 0)
    WXJS_METHOD("insertDir", insertDir, 2)
    WXJS_METHOD("isAbsolute", isAbsolute, 0)
    WXJS_METHOD("isPathSeparator", isPathSeparator, 1)
    WXJS_METHOD("isRelative", isRelative, 0)
    WXJS_METHOD("makeRelativeTo", makeRelativeTo, 0)
    WXJS_METHOD("mkdir", mkdir, 0)
    WXJS_METHOD("normalize", normalize, 0)
    WXJS_METHOD("prependDir", prependDir, 1)
    WXJS_METHOD("removeDir", removeDir, 1)
    WXJS_METHOD("removeLastDir", removeLastDir, 0)
    WXJS_METHOD("rmdir", rmdir, 0)
    WXJS_METHOD("sameAs", sameAs, 1)
    WXJS_METHOD("setCwd", setCwd, 0)
    WXJS_METHOD("touch", touch, 0)
WXJS_END_METHOD_MAP()

/***
 * <method name="appendDir">
 *  <function>
 *   <arg name="Dir" type="String" />
 *  </function>
 *  <desc>
 *   Appends a directory component to the path. This component 
 *   should contain a single directory name level, i.e. not contain 
 *   any path or volume separators nor should it be empty, otherwise the function does nothing
 *  </desc>
 * </method>
 */
JSBool FileName::appendDir(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxFileName *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxString dir;
    FromJS(cx, argv[0], dir);
    p->AppendDir(dir);
    return JS_TRUE;
}

/***
 * <method name="assign">
 *  <function>
 *   <arg name="FileName" type="wxFileName" />
 *  </function>
 *  <function>
 *   <arg name="FullPath" type="String">
 *    A full path. When it terminates with a pathseparator, a directory path is constructed,
 *    otherwise a filename and extension is extracted from it.
 *   </arg>
 *   <arg name="Format" type="@wxFileName#wxPathFormat" default="wxPathFormat.NATIVE" />
 *  </function>
 *  <function>
 *   <arg name="Path" type="String">A directory name</arg>
 *   <arg name="Name" type="String">A filename</arg>
 *   <arg name="Format" type="@wxFileName#wxPathFormat" default="wxPathFormat.NATIVE" />
 *  </function>
 *  <function>
 *   <arg name="Path" type="String">A directory name</arg>
 *   <arg name="Name" type="String">A filename</arg>
 *   <arg name="Ext" type="String">An extension</arg>
 *   <arg name="Format" type="@wxFileName#wxPathFormat" default="wxPathFormat.NATIVE" />
 *  </function>
 *  <function>
 *   <arg name="Volume" type="String">A volume name</arg>
 *   <arg name="Path" type="String">A directory name</arg>
 *   <arg name="Name" type="String">A filename</arg>
 *   <arg name="Ext" type="String">An extension</arg>
 *   <arg name="Format" type="@wxFileName#wxPathFormat" default="wxPathFormat.NATIVE" />
 *  </function>
 *  <desc>
 *   Creates the file name from various combinations of data.
 *  </desc>
 * </method>
 */
JSBool FileName::assign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxFileName *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    if ( argc == 1 )
    {
        if ( HasPrototype(cx, argv[0]) )
        {
            p->Assign(*GetPrivate(cx, argv[0], false));
            return JS_TRUE;
        }

        wxString filename;
        FromJS(cx, argv[0], filename);
        p->Assign(filename);
        return JS_TRUE;
    }

    // Argc > 1
    int format = wxPATH_NATIVE;
    uintN stringCount = argc;
    if ( JSVAL_IS_NUMBER(argv[argc-1]) )
    {
        FromJS(cx, argv[argc-1], format);
        stringCount--;
    }

    wxString *str = new wxString[stringCount];
    for(uintN i = 0; i < stringCount; i++)
        FromJS(cx, argv[i], str[i]);

    switch(stringCount)
    {
    case 1:
        p->Assign(str[0], (wxPathFormat) format);
        break;
    case 2:
        p->Assign(str[0], str[1], (wxPathFormat) format);
        break;
    case 3:
        p->Assign(str[0], str[1], str[2], (wxPathFormat) format);
        break;
    case 4:
        p->Assign(str[0], str[1], str[2], str[3], (wxPathFormat) format);
        break;
    }

    delete[] str;

    return JS_TRUE;
}

/***
 * <method name="assignDir">
 *  <function>
 *   <arg name="Dir" type="String" />
 *   <arg name="Format" type="@wxFileName#wxPathFormat" default="wxPathFormat.NATIVE" />
 *  </function>
 *  <desc>
 *   Sets this file name object to the given directory name. 
 *   The name and extension will be empty.
 *  </desc>
 * </method>
 */
JSBool FileName::assignDir(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxFileName *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxString dir;
    FromJS(cx, argv[0], dir);

    int format = wxPATH_NATIVE;
    if (     argc > 1 
        && ! FromJS(cx, argv[1], format) )
        return JS_FALSE;

    p->AssignDir(dir, (wxPathFormat) format);
    return JS_TRUE;
}

/***
 * <method name="assignHomeDir">
 *  <function />
 *  <desc>
 *   Sets this file name object to the home directory.
 *  </desc>
 * </method>
 */
JSBool FileName::assignHomeDir(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxFileName *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    p->AssignHomeDir();
    return JS_TRUE;
}

/***
 * <method name="assignTempFileName">
 *  <function>
 *   <arg name="Prefix" type="String" />
 *   <arg name="File" type="@wxFile" default="undefined">
 *    Object with wil get the file pointer to the temp file.
 *   </arg>
 *  </function>
 *  <desc>
 *   The function calls @wxFileName#createTempFileName to create
 *   a temporary file and sets this object to the name of the file.
 *   If a temporary file couldn't be created, the object is put into the invalid state.
 *   See also @wxFile.
 *   <br /><br />
 *   The following sample shows how to create a file with a temporary filename:
 *   <pre><code class="whjs">
 *    var name = new wxFileName();
 *    var file = new wxFile();
 * 
 *    name.assignTempFileName('wxjs', file);
 *    file.write('This is a test');
 *    file.close();
 *
 *    wxMessageBox('Filename: ' + name.fullPath);
 *   </code></pre>
 *  </desc>
 * </method>
 */
JSBool FileName::assignTempFileName(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxFileName *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxString prefix;
    FromJS(cx, argv[0], prefix);

    wxFile *file = NULL;
    if (     argc > 1 
        && (file = File::GetPrivate(cx, argv[1])) == NULL )
        return JS_FALSE;

    p->AssignTempFileName(prefix, file);
    return JS_TRUE;
}

/***
 * <method name="clear">
 *  <function />
 *  <desc>Reset all components to default, uninitialized state.</desc>
 * </method>
 */
JSBool FileName::clear(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxFileName *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    p->Clear();
    return JS_TRUE;
}

/***
 * <method name="getFullPath">
 *  <function returns="String">
 *   <arg name="Format" type="@wxFileName#wxPathFormat" default="wxPathFormat.NATIVE" />
 *  </function>
 *  <desc>
 *   Returns the full path with name and extension.
 *  </desc>
 * </method>
 */
JSBool FileName::getFullPath(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxFileName *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int format = wxPATH_NATIVE;
    if (     argc > 0
        && ! FromJS(cx, argv[0], format) )
        return JS_FALSE;

    *rval = ToJS(cx, p->GetFullPath((wxPathFormat) format));
    return JS_TRUE;
}

/***
 * <method name="getPath">
 *  <function returns="String">
 *   <arg name="Flag" type="@wxFileName#wxPathGet" default="wxPathGet.VOLUME" />
 *   <arg name="Format" type="@wxFileName#wxPathFormat" default="wxPathFormat.NATIVE" />
 *  </function>
 *  <desc>
 *   Returns the path part of the filename (without the name or extension).
 *  </desc>
 * </method>
 */
JSBool FileName::getPath(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxFileName *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int flag = wxPATH_GET_VOLUME;
    int format = wxPATH_NATIVE;
    if (     argc > 0
        && ! FromJS(cx, argv[0], flag) )
        return JS_FALSE;

    if (      argc > 1
         && ! FromJS(cx, argv[0], format) )
        return JS_FALSE;

    *rval = ToJS(cx, p->GetPath(flag, (wxPathFormat) format));
    return JS_TRUE;
}

/***
 * <method name="getTimes">
 *  <function returns="Boolean">
 *   <arg name="AccessTime" type="Date" />
 *   <arg name="ModTime" type="Date" default="null" />
 *   <arg name="CreateTime" type="Date" default="null" />
 *  </function>
 *  <desc>
 *  Gets the last access, last modification and creation times. 
 *  The last access time is updated whenever the file is read 
 *  or written (or executed in the case of Windows), last modification 
 *  time is only changed when the file is written to. Finally, 
 *  the creation time is indeed the time when the file was created under 
 *  Windows and the inode change time under Unix (as it is impossible to 
 *  retrieve the real file creation time there anyhow) which can also be 
 *  changed by many operations after the file creation.
 *  <br /><br />
 *  Any of the arguments may be null if the corresponding time is not needed.
 *  </desc>
 * </method>
 */
JSBool FileName::getTimes(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxFileName *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxDateTime access;
    wxDateTime mod;
    wxDateTime create;

    if ( p->GetTimes(&access, &mod, &create) )
    {
        if ( argv[0] != JSVAL_VOID )
        {
            if ( ! SetDate(cx, argv[0], access) )
                return JS_FALSE;
        }
        
        if ( argv[1] != JSVAL_VOID )
        {
            if ( ! SetDate(cx, argv[1], mod) )
                return JS_FALSE;
        }

        if ( argv[2] != JSVAL_VOID )
        {
            if ( ! SetDate(cx, argv[2], create) )
                return JS_FALSE;
        }
        *rval = JSVAL_TRUE;
    }
    else
        *rval = JSVAL_FALSE;

    return JS_TRUE;
}

/***
 * <method name="setTimes">
 *  <function returns="Boolean">
 *   <arg name="AccessTime" type="Date" />
 *   <arg name="ModTime" type="Date" default="null" />
 *   <arg name="CreateTime" type="Date" default="null" />
 *  </function>
 *  <desc>
 *   Sets the file creation and last access/modification times.
 *   Any of the arguments may be null if the corresponding time is not needed.
 *  </desc>
 * </method>
 */
JSBool FileName::setTimes(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxFileName *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxDateTime acc;
    wxDateTime mod;
    wxDateTime create;
        
    if ( argv[0] == JSVAL_VOID )
    {
        if ( ! FromJS(cx, argv[0], acc) )
            return JS_FALSE;
    }

    if ( argv[1] != JSVAL_VOID )
    {
        if ( ! FromJS(cx, argv[1], mod) )
            return JS_FALSE;
    }

    if ( argv[2] != JSVAL_VOID )
    {
        if ( ! FromJS(cx, argv[2], create) )
            return JS_FALSE;
    }

    *rval = ToJS(cx, p->SetTimes(acc.IsValid() ? &acc : NULL,
                                 mod.IsValid() ? &mod : NULL,
                                 create.IsValid() ? &create : NULL));
    return JS_TRUE;
}

/***
 * <method name="insertDir">
 *  <function>
 *   <arg name="Pos" type="Integer" />
 *   <arg name="Dir" type="String" />
 *  </function>
 *  <desc>
 *   Inserts a directory before the zero-based position in the directory list.
 *  </desc>
 * </method>
 */
JSBool FileName::insertDir(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxFileName *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int pos;
    wxString dir;

    if (    FromJS(cx, argv[0], pos) 
         && FromJS(cx, argv[1], dir) )
        p->InsertDir(pos, dir);
    else
        return JS_FALSE;

    return JS_TRUE;
}

/***
 * <method name="isAbsolute">
 *  <function returns="Boolean">
 *   <arg name="Format" type="@wxFileName#wxPathFormat" default="wxPathFormat.NATIVE" />
 *  </function>
 *  <desc>
 *   Returns true if this filename is absolute.
 *  </desc>
 * </method>
 */
JSBool FileName::isAbsolute(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxFileName *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int format = wxPATH_NATIVE;
    if (     argc > 0
        && ! FromJS(cx, argv[0], format) )
        return JS_FALSE;

    *rval = ToJS(cx, p->IsAbsolute((wxPathFormat) format));
    return JS_TRUE;
}

/***
 * <method name="isRelative">
 *  <function returns="Boolean">
 *   <arg name="Format" type="@wxFileName#wxPathFormat" default="wxPathFormat.NATIVE" />
 *  </function>
 *  <desc>
 *   Returns true if this filename is not absolute.
 *  </desc> 
 * </method>
 */
JSBool FileName::isRelative(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxFileName *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int format = wxPATH_NATIVE;
    if (     argc > 0
        && ! FromJS(cx, argv[0], format) )
        return JS_FALSE;

    *rval = ToJS(cx, p->IsRelative((wxPathFormat) format));
    return JS_TRUE;
}

/***
 * <method name="makeRelativeTo">
 *  <function returns="Boolean">
 *   <arg name="Base" type="String" default="">
 *    The directory to use as root. When not given, the current directory is used.
 *   </arg>
 *   <arg name="Format" type="@wxFileName#wxPathFormat" default="wxPathFormat.NATIVE" />
 *  </function>
 *  <desc>
 *   This function tries to put this file name in a form relative to Base.
 *   In other words, it returns the file name which should be used to access
 *   this file if the current directory were Base.
 *   <br /><br />
 *   Returns true if the file name has been changed, false if we failed to do anything
 *   with it (currently this only happens if the file name is on a volume different from 
 *   the volume specified by Base).
 *  </desc>
 * </method>
 */
JSBool FileName::makeRelativeTo(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxFileName *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxString base = wxEmptyString;
    int format = wxPATH_NATIVE;
    switch(argc)
    {
    case 2:
        if ( ! FromJS(cx, argv[1], format) )
            return JS_FALSE;
        // Fall through
    case 1:
        FromJS(cx, argv[0], base);
    }

    *rval = ToJS(cx, p->MakeRelativeTo(base, (wxPathFormat) format));
    return JS_TRUE;
}

/***
 * <method name="mkdir">
 *  <function returns="Boolean">
 *   <arg name="Perm" type="Integer" default="777">Permission for the directory</arg>
 *   <arg name="Flags" type="@wxFileName#wxPathMkdir" default="0">
 *    Default is 0. If the flags contain wxPathMkdir.FULL flag,
 *    try to create each directory in the path and also don't return 
 *    an error if the target directory already exists.
 *  </arg>
 *  </function>
 *  <desc>
 *   Creates a directory. Returns true on success.
 *  </desc>
 * </method>
 */
JSBool FileName::mkdir(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxFileName *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int perm = 777;
    int flag = 0;
    switch(argc)
    {
    case 2:
        if ( ! FromJS(cx, argv[1], flag) )
            return JS_FALSE;
        // Fall through
    case 1:
        if ( ! FromJS(cx, argv[1], perm) )
            return JS_FALSE;
    }

    *rval = ToJS(cx, p->Mkdir(perm, flag));
    return JS_TRUE;
}

/***
 * <method name="normalize">
 *  <function returns="Boolean">
 *   <arg name="Flags" type="@wxFileName#wxPathNormalize" default="wxPathNormalize.ALL" />
 *   <arg name="Cwd" type="String" default="" />
 *   <arg name="Format" type="@wxFileName#wxPathFormat" default="wxPathFormat.NATIVE" />
 *  </function>
 *  <desc>
 *   Normalize the path: with the default flags value, the path will be made absolute,
 *   without any ".." and "." and all environment variables will be expanded in it this 
 *   may be done using another (than current) value of cwd.
 *  </desc>
 * </method>
 */
JSBool FileName::normalize(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxFileName *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxString cwd = wxEmptyString;
    int format = wxPATH_NATIVE;
    int flag = wxPATH_NORM_ALL;

    switch(argc)
    {
    case 3:
        if ( ! FromJS(cx, argv[2], format) )
            return JS_FALSE;
        // Fall through
    case 2:
        FromJS(cx, argv[1], cwd);
        // Fall through
    case 1:
        // Fall through
        if ( ! FromJS(cx, argv[0], flag) )
            return JS_FALSE;
    case 0:
        *rval = ToJS(cx, p->Normalize(flag, cwd, (wxPathFormat) format));
        return JS_TRUE;
    }

    return JS_FALSE;
}

/***
 * <method name="prependDir">
 *  <function>
 *   <arg name="Dir" type="String" />
 *  </function>
 *  <desc>
 *   Prepends a directory name.
 *  </desc>
 * </method>
 */
JSBool FileName::prependDir(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxFileName *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxString dir;

    FromJS(cx, argv[1], dir);
    p->PrependDir(dir);

    return JS_TRUE;
}

/***
 * <method name="removeDir">
 *  <function>
 *   <arg name="Pos" type="Integer" />
 *  </function>
 *  <desc>
 *   Removes a directory name.
 *  </desc>
 * </method>
 */
JSBool FileName::removeDir(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxFileName *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    int pos;

    if ( FromJS(cx, argv[0], pos) )
        p->RemoveDir(pos);
    else
        return JS_FALSE;

    return JS_TRUE;
}

/***
 * <method name="removeLastDir">
 *  <function />
 *  <desc>
 *   Removes the last directory component from the path.
 *  </desc>
 * </method>
 */
JSBool FileName::removeLastDir(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxFileName *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    p->RemoveLastDir();

    return JS_TRUE;
}

/***
 * <method name="rmdir">
 *  <function />
 *  <desc>
 *   Deletes the directory from the file system.
 *  </desc>
 * </method>
 */
JSBool FileName::rmdir(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxFileName *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    p->Rmdir();

    return JS_TRUE;
}

/***
 * <method name="sameAs">
 *  <function returns="Boolean">
 *   <arg name="FileName" type="String" />
 *   <arg name="Format" type="@wxFileName#wxPathFormat" default="wxPathFormat.NATIVE" />
 *  </function>
 *  <desc>
 *   Compares the filename using the rules of the format.
 *  </desc>
 * </method>
 */
JSBool FileName::sameAs(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxFileName *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxFileName *compareTo = GetPrivate(cx, argv[0]);
    if ( compareTo == NULL )
        return JS_FALSE;

    int format = wxPATH_NATIVE;
    if (     argc > 1
        && ! FromJS(cx, argv[1], format) )
        return JS_FALSE;

    *rval = ToJS(cx, p->SameAs(*compareTo, (wxPathFormat) format));
    return JS_TRUE;
}

/***
 * <method name="setCwd">
 *  <function />
 *  <desc>
 *   Set this to the current directory.
 *  </desc>
 * </method>
 */
JSBool FileName::setCwd(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxFileName *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    p->SetCwd();

    return JS_TRUE;
}

/***
 * <method name="touch">
 *  <function />
 *  <desc>
 *   Sets the access and modification times to the current moment.
 *  </desc>
 * </method>
 */
JSBool FileName::touch(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxFileName *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    p->Touch();

    return JS_TRUE;
}

WXJS_BEGIN_STATIC_METHOD_MAP(FileName)
    WXJS_METHOD("assignCwd", assignCwd, 0)
    WXJS_METHOD("createTempFileName", createTempFileName, 2)
    WXJS_METHOD("dirExists", dirExists, 1)
    WXJS_METHOD("dirName", dirName, 1)
    WXJS_METHOD("fileExists", fileExists, 1)
    WXJS_METHOD("fileName", fileName, 1)
    WXJS_METHOD("getCwd", getCwd, 0)
    WXJS_METHOD("getFormat", getFormat, 0)
    WXJS_METHOD("mkdir", smkdir, 1)
    WXJS_METHOD("rmdir", srmdir, 1)
    WXJS_METHOD("splitPath", splitPath, 1)
    WXJS_METHOD("getPathSeparator", getPathSeparator, 0)
    WXJS_METHOD("getPathSeparators", getPathSeparators, 0)
    WXJS_METHOD("getVolumeSeparator", getVolumeSeparator, 0)
    WXJS_METHOD("isCaseSensitive", isAbsolute, 0)
    WXJS_METHOD("isPathSeparator", isPathSeparator, 1)
WXJS_END_METHOD_MAP()

/***
 * <class_method name="assignCwd">
 *  <function>
 *   <arg name="Volume" type="String" default="" />
 *  </function>
 *  <desc>
 *   Makes this object refer to the current working directory on the 
 *   specified volume (or current volume if volume is not specified).
 *  </desc>
 * </class_method>
 */
JSBool FileName::assignCwd(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxFileName *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxString volume = wxEmptyString;
    if ( argc > 0 )
        FromJS(cx, argv[0], volume);

    p->AssignCwd(volume);
    return JS_TRUE;
}

/***
 * <class_method name="createTempFileName">
 *  <function returns="String">
 *   <arg name="Prefix" type="String" />
 *   <arg name="File" type="@wxFile" default="null">
 *    Object which wil get the file pointer to the temp file.
 *   </arg>
 *  </function>
 *  <desc>
 *  Returns a temporary file name starting with the given prefix. 
 *  If the prefix is an absolute path, the temporary file is created 
 *  in this directory, otherwise it is created in the default system 
 *  directory for the temporary files or in the current directory.
 *  <br /><br />
 *  If the function succeeds, the temporary file is actually created. 
 *  When <i>File</i> is not omitted, this file will be opened using the name
 *  of the temporary file. When possible, this is done in an atomic way ensuring 
 *  that no race condition occurs between the temporary file name generation and 
 *  opening it which could often lead to security compromise on the multiuser systems.
 *  If file is omitted, the file is only created, but not opened.
 *  <br /><br />
 *  Under Unix, the temporary file will have read and write permissions for the owner 
 *  only to minimize the security problems.
 *  </desc>
 * </class_method>
 */
JSBool FileName::createTempFileName(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxString prefix;
    FromJS(cx, argv[0], prefix);

    wxFile *file = NULL;
    if (     argc > 1 
        && (file = File::GetPrivate(cx, argv[1])) == NULL )
        return JS_FALSE;

    wxFileName::CreateTempFileName(prefix, file);
    return JS_TRUE;
}

/***
 * <class_method name="dirExists">
 *  <function returns="Boolean">
 *   <arg name="Dir" type="String" />
 *  </function>
 *  <desc>
 *   Returns true when the directory exists.
 *  </desc>
 * </class_method>
 */
JSBool FileName::dirExists(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxString dir;
    FromJS(cx, argv[0], dir);

    *rval = ToJS(cx, wxFileName::DirExists(dir));
    return JS_TRUE;
}

/***
 * <class_method name="dirName">
 *  <function returns="wxFileName">
 *   <arg name="Dir" type="String" />
 *  </function>
 *  <desc>
 *   Creates a new wxFileName object based on the given directory.
 *  </desc>
 * </class_method>
 */
JSBool FileName::dirName(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxString dir;
    FromJS(cx, argv[0], dir);

    *rval = CreateObject(cx, new wxFileName(wxFileName::DirName(dir)));
    return JS_TRUE;
}

/***
 * <class_method name="fileExists">
 *  <function returns="Boolean">
 *   <arg name="File" type="String" />
 *  </function>
 *  <desc>
 *   Returns true when the file exists.
 *  </desc>
 * </class_method>
 */
JSBool FileName::fileExists(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxString file;
    FromJS(cx, argv[0], file);

    *rval = ToJS(cx, wxFileName::FileExists(file));
    return JS_TRUE;
}

/***
 * <class_method name="fileName">
 *  <function returns="@wxFileName">
 *   <arg name="File" type="String" />
 *  </function>
 *  <desc>
 *   Creates a new wxFileName object based on the given file.
 *  </desc>
 * </class_method>
 */
JSBool FileName::fileName(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxString file;
    FromJS(cx, argv[0], file);

    *rval = CreateObject(cx, new wxFileName(wxFileName::FileName(file)));
    return JS_TRUE;
}

/***
 * <class_method name="getCwd">
 *  <function returns="String">
 *   <arg name="Volume" type="String" default="" />
 *  </function>
 *  <desc>
 *   Retrieves the value of the current working directory on the specified volume. 
 *   When the volume is omitted, the programs current working directory is returned 
 *   for the current volume.
 *  </desc>
 * </class_method>
 */
JSBool FileName::getCwd(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxString vol = wxEmptyString;
    if ( argc > 0 )
        FromJS(cx, argv[0], vol);

    *rval = ToJS(cx, wxFileName::GetCwd(vol));
    return JS_TRUE;
}

/***
 * <class_method name="getFormat">
 *  <function returns="Integer">
 *   <arg name="Format" type="@wxFileName#wxPathFormat" default="wxPathFormat.NATIVE" />
 *  </function>
 *  <desc>
 *   Returns the canonical path format for this platform.
 *  </desc>
 * </class_method>
 */
JSBool FileName::getFormat(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    int format = wxPATH_NATIVE;
    if ( argc > 0 )
        FromJS(cx, argv[0], format);

    *rval = ToJS<int>(cx, wxFileName::GetFormat((wxPathFormat) format));
    return JS_TRUE;
}

/***
 * <class_method name="getPathSeparator">
 *  <function returns="String">
 *   <arg name="Format" type="@wxFileName#wxPathFormat" default="wxPathFormat.NATIVE" />
 *  </function>
 *  <desc>
 *   Returns the usually used path separator for this format. 
 *   For all formats but wxPathFormat.DOS there is only one path 
 *   separator anyhow, but for DOS there are two of them and the 
 *   native one, i.e. the backslash is returned by this method.
 *  </desc>
 * </class_method>
 */
JSBool FileName::getPathSeparator(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    int format = wxPATH_NATIVE;
    if (     argc > 0
        && ! FromJS(cx, argv[0], format) )
        return JS_FALSE;

    *rval = ToJS(cx, wxString(wxFileName::GetPathSeparator((wxPathFormat) format)));
    return JS_TRUE;
}

/***
 * <class_method name="getPathSeparators">
 *  <function returns="String">
 *   <arg name="Format" type="@wxFileName#wxPathFormat" default="wxPathFormat.NATIVE" />
 *  </function>
 *  <desc>
 *   Returns the string containing all the path separators for this format.
 *   For all formats but wxPathFormat.DOS this string contains only one character 
 *   but for DOS and Windows both '/' and '\' may be used as separators.
 *  </desc>
 * </class_method>
 */
JSBool FileName::getPathSeparators(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    int format = wxPATH_NATIVE;
    if (     argc > 0
        && ! FromJS(cx, argv[0], format) )
        return JS_FALSE;

    *rval = ToJS(cx, wxFileName::GetPathSeparators((wxPathFormat) format));
    return JS_TRUE;
}

/***
 * <class_method name="getVolumeSeparator">
 *  <function returns="String">
 *   <arg name="Format" type="@wxFileName#wxPathFormat" default="wxPathFormat.NATIVE" />
 *  </function>
 *  <desc>
 *   Returns the string separating the volume from the path for this format.
 *  </desc>
 * </class_method>
 */
JSBool FileName::getVolumeSeparator(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    int format = wxPATH_NATIVE;
    if (     argc > 0
        && ! FromJS(cx, argv[0], format) )
        return JS_FALSE;

    *rval = ToJS(cx, wxString(wxFileName::GetVolumeSeparator((wxPathFormat) format)));
    return JS_TRUE;
}

/***
 * <class_method name="isCaseSensitive">
 *  <function returns="String">
 *   <arg name="Format" type="@wxFileName#wxPathFormat" default="wxPathFormat.NATIVE" />
 *  </function>
 *  <desc>
 *   Returns true if filenames for the given format are case-sensitive.
 *  </desc>
 * </class_method>
 */
JSBool FileName::isCaseSensitive(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    int format = wxPATH_NATIVE;
    if (     argc > 0
        && ! FromJS(cx, argv[0], format) )
        return JS_FALSE;

    *rval = ToJS(cx, wxFileName::IsCaseSensitive((wxPathFormat) format));
    return JS_TRUE;
}

/***
 * <class_method name="isPathSeparator">
 *  <function returns="Boolean">
 *   <arg name="Sep" type="String">A path separator</arg>
 *   <arg name="Format" type="@wxFileName#wxPathFormat" default="wxPathFormat.NATIVE" />
 *  </function>
 *  <desc>
 *   Returns true if the given string (only the first character is checked!)
 *   is a separator on the format.
 *  </desc>
 * </class_method>
 */
JSBool FileName::isPathSeparator(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxString sep;
    FromJS(cx, argv[0], sep);
    if ( sep.Length() == 0 )
        return JS_TRUE;

    int format = wxPATH_NATIVE;
    if (     argc > 1
        && ! FromJS(cx, argv[1], format) )
        return JS_FALSE;

    *rval = ToJS(cx, wxFileName::IsPathSeparator(sep[0], (wxPathFormat) format));
    return JS_TRUE;
}

/***
 * <class_method name="mkdir">
 *  <function returns="Boolean">
 *   <arg name="Dir" type="String">Name of the directory</arg>
 *   <arg name="Perm" type="Integer" default="777">Permissions</arg>
 *   <arg name="Flags" type="@wxFileName#wxPathMkdir" default="0">
 *    Default is 0. If the flags contain wxPathMkdir.FULL flag,
 *    try to create each directory in the path and also don't return 
 *    an error if the target directory already exists.
 *   </arg>
 *  </function>
 *  <desc>
 *   Creates a directory. Returns true on success.
 *  </desc>
 * </class_method>
 */
JSBool FileName::smkdir(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    if ( argc > 3 )
        argc = 3;

    wxString dir;
    int perm = 777;
    int flag = 0;
    
    switch(argc)
    {
    case 3:
        if ( ! FromJS(cx, argv[2], flag) )
            return JS_FALSE;
        // Fall through
    case 2:
        if ( ! FromJS(cx, argv[1], perm) )
            return JS_FALSE;
    default:
        FromJS(cx, argv[0], dir);
        *rval = ToJS(cx, wxFileName::Mkdir(dir, perm, flag));
    }
    return JS_TRUE;
}

/***
 * <class_method name="rmdir">
 *  <function>
 *   <arg name="Dir" type="String" />
 *  </function>
 *  <desc>
 *   Deletes the directory from the file system.
 *  </desc>
 * </class_method>
 */
JSBool FileName::srmdir(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxString dir;
    FromJS(cx, argv[0], dir);
    wxFileName::Rmdir(dir);

    return JS_TRUE;
}

/***
 * <class_method name="splitPath">
 *  <function returns="String Array">
 *   <arg name="FullPath" type="String" />
 *   <arg name="Format" type="@wxFileName#wxPathFormat" default="wxPathFormat.NATIVE" />
 *  </function>
 *  <desc>
 *   This function splits a full file name into components: the volume, path,
 *   the base name and the extension. 
 *   <br /><br /><b>Remark:</b> In wxWindows the arguments are passed as
 *   pointers. Because this is not possible in JavaScript, wxJS returns an array containing
 *   the parts of the path.
 *   <br /><br />
 *   The following code illustrates how splitPath works in JavaScript. parts[0] contains "C", 
 *   parts[1] contains "\\", parts[2] contains "Temp" and parts[3] is empty.
 *   <pre><code class="whjs">
 *    var parts = wxFileName.splitPath("C:\\Temp");
 *    for(element in parts)
 *    {
 *      ...
 *    }
 *    </code></pre>
 *  </desc>
 * </class_method>
 */
JSBool FileName::splitPath(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxString path;
    FromJS(cx, argv[0], path);

    int format = wxPATH_NATIVE;
    if (      argc > 1 
         && ! FromJS(cx, argv[1], format) )
        return JS_FALSE;

    wxString parts[4];
    wxFileName::SplitPath(path, &parts[0], &parts[1], &parts[2], &parts[3], (wxPathFormat) format);
    
    JSObject *objArray = JS_NewArrayObject(cx, 4, NULL);
    *rval = OBJECT_TO_JSVAL(objArray);
    for(int i = 0; i < 4; i++)
    {
        jsval element = ToJS(cx, parts[i]);
        JS_SetElement(cx, objArray, i, &element);
    }

    return JS_TRUE;
}

bool FileName::SetDate(JSContext *cx, jsval v, const wxDateTime &date)
{
    if ( JSVAL_IS_OBJECT(v) )
    {
        JSObject *obj = JSVAL_TO_OBJECT(v);
        if ( js_DateIsValid(cx, obj) )
        {
            js_DateSetYear(cx, obj, date.GetYear());
            js_DateSetMonth(cx, obj, date.GetMonth());
            js_DateSetDate(cx, obj, date.GetDay());
            js_DateSetHours(cx, obj, date.GetHour());
            js_DateSetMinutes(cx, obj, date.GetMinute());
            js_DateSetSeconds(cx, obj, date.GetSecond());
            return true;
        }
    }
    return false;
}
