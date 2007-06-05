#include "precompiled.h"

/*
 * wxJavaScript - textfile.cpp
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
 * $Id: textfile.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../common/main.h"
#include "../common/index.h"
#include "../ext/wxjs_ext.h"
#include "textfile.h"
#include "textline.h"

using namespace wxjs;
using namespace wxjs::io;

/***
 * <file>textfile</file>
 * <module>io</module>
 * <class name="wxTextFile" version="0.8.3">
 *  The wxTextFile is a simple class which allows to work with text files on line by line basis. 
 *  It also understands the differences in line termination characters under different platforms 
 *  and will not do anything bad to files with "non native" line termination sequences - in fact, 
 *  it can be also used to modify the text files and change the line termination characters from 
 *  one type (say DOS) to another (say Unix).
 *  <br /><br />
 *  One word of warning: the class is not at all optimized for big files and thus it will load the 
 *  file entirely into memory when opened. Of course, you should not work in this way with large files
 *  (as an estimation, anything over 1 Megabyte is surely too big for this class). On the other hand,
 *  it is not a serious limitation for small files like configuration files or program sources which
 *  are well handled by wxTextFile.
 *  <br /><br />
 *  The typical things you may do with wxTextFile in order are:
 *  <ul>
 *  <li>
 *   Create and open it: this is done with either @wxTextFile#create or @wxTextFile#open function 
 *   which opens the file (name may be specified either as the argument to these functions or in 
 *   the constructor), reads its contents in memory (in the case of open) and closes it. 
 *  </li>
 *  <li>
 *   Work with the lines in the file: this may be done either with "direct access" properties 
 *   like @wxTextFile#lines or with "sequential access" properties which include
 *   @wxTextFile#firstLine/@wxTextFile#nextLine and also @wxTextFile#lastLine/@wxTextFile#prevLine. 
 *   For the sequential access properties the current line number is maintained: @wxTextFile#currentLine 
 *   and may be changed with @wxTextFile#goToLine. 
 *  </li>
 *  <li>
 *   Add/remove lines to the file: @wxTextFile#addLine and @wxTextFile#insertLine add new lines while
 *   @wxTextFile#removeLine deletes the existing ones. @wxTextFile#clear resets the file to empty.
 *  </li>
 *  <li>
 *   Save your changes: notice that the changes you make to the file will not be saved automatically;
 *   calling @wxTextFile#close or doing nothing discards them! To save the changes you must explicitly 
 *   call @wxTextFile#write - here, you may also change the line termination type if you wish 
 *  </li>
 *  </ul>
 *  <b>Remark :</b>wxJS adds the @wxTextLine class. In wxWidgets, lines can be changed because
 *  most methods return a reference. This is not possible in JavaScript, but this can be solved by
 *  returning an object: @wxTextLine. This class can be used to access the lines like they were
 *  an array and you can change them directly:
 *  <pre><code class="whjs">
 *   var file = new wxTextFile("text.txt");
 *   file.line[0] = "This is the first line";
 *  </code></pre>
 * </class>
 */
WXJS_INIT_CLASS(TextFile, "wxTextFile", 0)

/***
 * <constants>
 *  <type name="wxTextFileType">
 *   <constant name="None" />
 *   <constant name="Unix" />
 *   <constant name="Dos" />
 *   <constant name="Mac" />
 *  </type>
 * </constants>
 */

void TextFile::InitClass(JSContext *cx, JSObject *obj, JSObject *proto)
{
    JSConstDoubleSpec wxTextFileType[] = 
    {
		WXJS_CONSTANT(wxTextFileType_, None)
		WXJS_CONSTANT(wxTextFileType_, Unix)
		WXJS_CONSTANT(wxTextFileType_, Dos)
		WXJS_CONSTANT(wxTextFileType_, Mac)
	    { 0 }
    };

    JSObject *constObj = JS_DefineObject(cx, obj, "wxTextFileType", 
									 	 NULL, NULL,
							             JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineConstDoubles(cx, constObj, wxTextFileType);
}

/***
 * <ctor>
 *  <function />
 *  <function>
 *   <arg name="FileName" type="String">The filename</arg>
 *  </function>
 *  <desc>
 *   Constructs a new wxTextFile object. The file is not loaded
 *   into memory. Use @wxTextFile#open to do it. 
 *  </desc>
 * </ctor>
 */
wxTextFile *TextFile::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
	wxString fileName;

	switch(argc)
	{
	case 1:
		{
			FromJS(cx, argv[0], fileName);
			wxTextFile *file = new wxTextFile(fileName);
			return file;
		}
	case 0:
		return new wxTextFile();
	}
	return NULL;
}

/***
 * <properties>
 *  <property name="currentLine" type="Integer" readonly="Y">
 *   Returns the current line: it has meaning only when you're using 
 *   @wxTextFile#firstLine/@wxTextFile#nextLine properties, it doesn't get updated when you're 
 *   using "direct access" property @wxTextFile#lines. @wxTextFile#firstLine and @wxTextFile#lastLine 
 *   also change the value of the current line, as well as @wxTextFile#goToLine.
 *  </property>
 *	<property name="eof" type="Boolean" readonly="Y">
 *	 Returns true when end of file is reached. 
 *   When the file is not open, undefined is returned.
 *  </property>
 *  <property name="firstLine" type="@wxTextLine">
 *   Get the first line.
 *  </property>
 *  <property name="guessType" type="@wxTextFileType" readonly="Y">
 *   Guess the type of file (which is supposed to be opened). If sufficiently many lines of the file 
 *   are in DOS/Unix/Mac format, the corresponding value will be returned. If the detection mechanism fails wxTextFileType_None is returned
 *  </property>
 *  <property name="opened" type="Boolean" readonly="Y">
 *   Is the file open?
 *  </property>
 *  <property name="lastLine" type="@wxTextLine">
 *   Get the last line
 *  </property>
 *  <property name="lineCount" type="Integer" readonly="Y">
 *   Number of lines in the file.
 *  </property>
 *  <property name="lines" type="Array of @wxTextLine">
 *   Get an array of lines
 *  </property>
 *  <property name="nextLine" type="@wxTextLine">
 *   Get the nextline
 *  </property>
 *  <property name="prevLine" type="@wxTextLine">
 *   Get the previous line
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(TextFile)
	WXJS_READONLY_PROPERTY(P_CURRENT_LINE, "currentLine")
	WXJS_READONLY_PROPERTY(P_EOF, "eof")
	WXJS_READONLY_PROPERTY(P_FIRST_LINE, "firstLine")
	WXJS_READONLY_PROPERTY(P_LAST_LINE, "lastLine")
	WXJS_READONLY_PROPERTY(P_NEXT_LINE, "nextLine")
	WXJS_READONLY_PROPERTY(P_PREV_LINE, "prevLine")
	WXJS_READONLY_PROPERTY(P_OPENED, "opened")
	WXJS_READONLY_PROPERTY(P_LINE_COUNT, "lineCount")
	WXJS_READONLY_PROPERTY(P_NAME, "name")
	WXJS_READONLY_PROPERTY(P_GUESS_TYPE, "guessType")
	WXJS_READONLY_PROPERTY(P_LINES, "lines")
WXJS_END_PROPERTY_MAP()

bool TextFile::GetProperty(wxTextFile *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
	switch(id)
	{
	case P_EOF:
		*vp = p->IsOpened() ? ToJS(cx, p->Eof()) : JSVAL_VOID;
		break;
	case P_OPENED:
		*vp = ToJS(cx, p->IsOpened());
		break;
	case P_LINE_COUNT:
		*vp = ToJS(cx, p->GetLineCount());
		break;
	case P_CURRENT_LINE:
		*vp = ToJS(cx, p->GetCurrentLine());
		break;
	case P_FIRST_LINE:
		p->GetFirstLine();
		*vp = TextLine::CreateObject(cx, new Index(0), obj);
		break;
	case P_NEXT_LINE:
		if ( ! p->Eof() )
		{
			p->GetNextLine();
			*vp = TextLine::CreateObject(cx, new Index(p->GetCurrentLine()), obj);
		}
		else
		{
			*vp = JSVAL_VOID;
		}
		break;
	case P_PREV_LINE:
		if ( p->GetCurrentLine() == 0 )
		{
			*vp = JSVAL_VOID;
		}
		else
		{
			p->GetPrevLine();
			*vp = TextLine::CreateObject(cx, new Index(p->GetCurrentLine()), obj);
		}
		break;
	case P_LAST_LINE:
		p->GetLastLine();
		*vp = TextLine::CreateObject(cx, new Index(p->GetCurrentLine()), obj);
		break;
	case P_GUESS_TYPE:
		*vp = ToJS(cx, (int) p->GuessType());
		break;
	case P_LINES:
		*vp = TextLine::CreateObject(cx, new Index(0), obj);
		break;
	case P_NAME:
		*vp = ToJS(cx, wxString(p->GetName()));
		break;
	}
	return true;
}

WXJS_BEGIN_METHOD_MAP(TextFile)
	WXJS_METHOD("addLine", addLine, 1)
	WXJS_METHOD("close", close, 0)
	WXJS_METHOD("clear", clear, 0)
	WXJS_METHOD("create", create, 1)
	WXJS_METHOD("exists", exists, 0)
	WXJS_METHOD("gotoLine", gotoLine, 1)
	WXJS_METHOD("open", open, 2)
	WXJS_METHOD("insertLine", insertLine, 2)
	WXJS_METHOD("removeLine", removeLine, 1)
	WXJS_METHOD("write", write, 2)
WXJS_END_METHOD_MAP()

/***
 * <method name="addLine">
 *  <function>
 *   <arg name="Line" type="String">
 *    The line to add
 *   </arg>
 *   <arg name="Type" type="@wxTextFile#wxTextFileType" />
 *  </function>
 *  <desc>
 *   Adds a line to the end of file. When <i>Type</i> is omitted 
 *   the native default is used.
 *  </desc>
 * </method>
 */
JSBool TextFile::addLine(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxTextFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxTextFile"));

	wxString line;
	FromJS(cx, argv[0], line);

	if ( argc == 1 )
	{
		p->AddLine(line);
		return JS_TRUE;
	}
	else
	{
		int type;
		if ( FromJS(cx, argv[1], type) )
		{
			p->AddLine(line, (wxTextFileType) type);
			return JS_TRUE;
		}
	}

	return JS_FALSE;
}

/***
 * <method name="clear">
 *  <function />
 *  <desc>
 *   Removes all lines
 *  </desc>
 * </method>
 */
JSBool TextFile::clear(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxTextFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxTextFile"));

    p->Clear();

	return JS_TRUE;
}

/***
 * <method name="close">
 *  <function returns="Boolean" />
 *  <desc> 
 *   Closes the file and frees memory, losing all changes. Use @wxTextFile#write if you want to save them.
 *  </desc>
 * </method>
 */
JSBool TextFile::close(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxTextFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxFile"));

	*rval = ToJS(cx, p->Close());

	return JS_TRUE;
}

/***
 * <method name="create">
 *  <function returns="Boolean" />
 *  <function returns="Boolean">
 *   <arg name="FileName" type="String">The name of the file to create.</arg>
 *  </function>
 *  <desc>
 *   Creates the file with the given name or the name which was given in the constructor. 
 *   The array of file lines is initially empty.
 *   It will fail if the file already exists, @wxTextFile#open should be used in this case. 
 *  </desc>
 * </method>
 */
JSBool TextFile::create(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxTextFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxTextFile"));

	if ( argc != 0 )
	{
		wxString fileName;
		FromJS(cx, argv[0], fileName);
		*rval = ToJS(cx, p->Create(fileName));
		return JS_TRUE;
	}
	
	*rval = ToJS(cx, p->Create());
	return JS_TRUE;
}

/***
 * <method name="exists">
 *  <function returns="Boolean" />
 *  <desc> 
 *   Return true if file exists - the name of the file should have been specified in the 
 *   constructor before calling exists().
 *  </desc>
 * </method>
 */
JSBool TextFile::exists(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxTextFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxTextFile"));

	*rval = ToJS(cx, p->Exists());

	return JS_TRUE;
}

/***
 * <method name="gotoLine">
 *  <function>
 *   <arg name="Line" type="Integer" />
 *  </function>
 *  <desc> 
 *   Changes the value returned by @wxTextFile#currentLine.
 *  </desc>
 * </method>
 */
JSBool TextFile::gotoLine(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxTextFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxTextFile"));

	int line;
	if ( FromJS(cx, argv[0], line) )
	{
		p->GoToLine(line);
		return JS_TRUE;
	}

	return JS_FALSE;
}

/***
 * <method name="insertLine">
 *  <function>
 *   <arg name="Line" type="String">The line to insert (without the end-of-line character(s)).</arg>
 *   <arg name="Pos" type="Integer">The line position</arg>
 *   <arg name="Type" type="@wxTextFile#wxTextFileType" default="typeDefault" />
 *  </function>
 *  <desc>
 *   Insert a line before the line number <i>Pos</i>.
 *  </desc>
 * </method>
 */
JSBool TextFile::insertLine(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxTextFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxTextFile"));

	wxString line;
	int pos;

	FromJS(cx, argv[0], line);
	if ( ! FromJS(cx, argv[1], pos) )
		return JS_FALSE;

	if ( argc > 2 )
	{
		int type;
		if ( ! FromJS(cx, argv[2], type) )
			return JS_FALSE;
		p->InsertLine(line, pos, (wxTextFileType) type);
	}
	else
	{
		p->InsertLine(line, pos);
	}
	return JS_TRUE;
}

/***
 * <method name="open">
 *  <function returns="Boolean" />
 *  <function returns="Boolean">
 *   <arg name="Encoding" type="String" default="UTF-8" />  
 *   <arg name="FileName" type="String" default="">The name of the file to open</arg>
 *  </function>
 *  <desc>
 *   Opens the file with the given name or the name which was given in the constructor 
 *   and also loads file in memory on success. It will fail if the file does not exist, 
 *   @wxTextFile#create should be used in this case. When no encoding is specified
 *   the file is opened in UTF-8.
 *  </desc>
 * </method>
 */
JSBool TextFile::open(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxTextFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxTextFile"));

	if ( p->IsOpened() )
		return JS_TRUE;

	if ( argc == 0 )
	{
		*rval = ToJS(cx, p->Open());
		return JS_TRUE;
	}

    wxString encoding(wxJS_EXTERNAL_ENCODING);
    FromJS(cx, argv[0], encoding);
    wxCSConv conv(encoding);

    bool ok = false;
    if ( argc > 1 )
    {
	    wxString fileName;
	    FromJS(cx, argv[1], fileName);
	    ok = p->Open(fileName, conv);
    }
    else
    {
        ok = p->Open(conv);
    }

    if ( ok )
    {
        // Convert all lines, because internally we want another encoding
        if ( ! encoding.IsSameAs(wxJS_INTERNAL_ENCODING, false) )
        {
            wxCSConv internalConv(wxJS_INTERNAL_ENCODING);
            for (wxString str = p->GetFirstLine(); !p->Eof(); str = p->GetNextLine() )
            {
                str = wxString(str.mb_str(conv), internalConv);
            }
            p->GoToLine(0);
        }
    }

    *rval = ToJS(cx, ok);

	return JS_TRUE;
}

/***
 * <method name="removeLine">
 *  <function>
 *   <arg name="Line" type="Integer">The line to remove.</arg>
 *  </function>
 *  <desc>
 *   Removes the line from the file
 *  </desc>
 * </method>
 */
JSBool TextFile::removeLine(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxTextFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxFile"));

    int line;
	if ( ! FromJS(cx, argv[0], line) )
	{
		return JS_FALSE;
	}

	p->RemoveLine(line);
	return JS_TRUE;
}

/***
 * <method name="write">
 *  <function returns="Boolean">
 *   <arg name="Type" type="@wxTextFile#wxTextFileType" default="wxTextFileType.None" />
 *   <arg name="Encoding" type="String" default="UTF-8" />
 *  </function>
 *  <desc>
 *   Change the file on disk. The <i>Type</i> parameter allows you to change the file format 
 *   (default argument means "don't change type") and may be used to convert, for example,
 *   DOS files to Unix.
 *  </desc>
 * </method>
 */
JSBool TextFile::write(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	wxTextFile *p = GetPrivate(cx, obj);
	wxASSERT_MSG(p != NULL, wxT("No private data associated with wxTextFile"));

	int type = wxTextFileType_None;
	if ( argc > 0 )
	{
		if ( ! FromJS(cx, argv[0], type) )
			return JS_FALSE;
	}
    wxString encoding(wxJS_EXTERNAL_ENCODING);
    if ( argc > 1 )
    {
        FromJS(cx, argv[1], encoding);
    }
    wxCSConv conv(encoding);
	*rval = ToJS(cx, p->Write((wxTextFileType) type, conv));

	return JS_TRUE;
}

WXJS_BEGIN_STATIC_METHOD_MAP(TextFile)
	WXJS_METHOD("getEOL", getEOL, 1)
WXJS_END_METHOD_MAP()

/***
 * <class_method name="getEOL">
 *  <function returns="String">
 *   <arg name="Type" type="@wxTextFileType" default="typeDefault" />
 *  </function> 
 *  <desc>
 *   Get the line termination string corresponding to given constant. 
 *   typeDefault is the value defined during the compilation and corresponds to the 
 *   native format of the platform, i.e. it will be wxTextFileType.Dos under Windows, 
 *   wxTextFileType.Unix under Unix (including Mac OS X when compiling with the Apple
 *   Developer Tools) and wxTextFileType.Mac under Mac OS 
 *   (including Mac OS X when compiling with CodeWarrior).
 *  </desc>
 * </class_method>
 */
JSBool TextFile::getEOL(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int type;
	if ( ! FromJS(cx, argv[0], type) )
		return JS_FALSE;

	*rval = ToJS(cx, wxString(wxTextFile::GetEOL((wxTextFileType) type)));
	return JS_TRUE;
}
