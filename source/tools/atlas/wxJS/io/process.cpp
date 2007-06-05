#include "precompiled.h"

/*
 * wxJavaScript - process.cpp
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
 * $Id: process.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "../common/main.h"
#include "process.h"
#include "stream.h"
#include "istream.h"
#include "ostream.h"

using namespace wxjs;
using namespace wxjs::io;

Process::Process(  JSContext *cx
				 , JSObject *obj
				 , int flags) : wxProcess(flags)
{
}

Process::~Process()
{
}

/*** 
 * <file>process</file>
 * <module>io</module>
 * <class name="wxProcess" version="0.9.5">
 *  The objects of this class are used in conjunction with the @wxExecute function. 
 *  When a wxProcess object is passed to @wxExecute(), its @wxProcess#onTerminate event
 *  is called when the process terminates. This allows the program to be (asynchronously)
 *  notified about the process termination and also retrieve its exit status which is 
 *  unavailable from @wxExecute in the case of asynchronous execution.
 *  <br /><br />
 *  wxProcess also supports IO redirection of the child process. For this, you have to call 
 *  its @wxProcess#redirect method before passing it to @wxExecute. If the child process was launched successfully, 
 *  @wxProcess#inputStream, @wxProcess#outputStream and @wxProcess#errorStream can then be used to retrieve the
 *  streams corresponding to the child process standard output, input and error output respectively.
 * </class>
 */
WXJS_INIT_CLASS(Process, "wxProcess", 1)

/***
 * <constants>
 *  <type>
 *   <constant name="DEFAULT" />
 *   <constant name="REDIRECT">Turn on redirection</constant>
 *  </type>
 *  <type name="wxSignal">
 *   <constant name="SIGNONE" />
 *   <constant name="SIGHUP" />
 *   <constant name="SIGINT" />
 *   <constant name="SIGQUIT" />
 *   <constant name="SIGILL" />
 *   <constant name="SIGTRAP" />
 *   <constant name="SIGABRT" />
 *   <constant name="SIGEMT" />
 *   <constant name="SIGFPE" />
 *   <constant name="SIGKILL" />
 *   <constant name="SIGBUS" />
 *   <constant name="SIGSEGV" />
 *   <constant name="SIGSYS" />
 *   <constant name="SIGPIPE" />
 *   <constant name="SIGALRM" />
 *   <constant name="SIGTERM" />
 *  </type>
 *  <type name="wxKillError">
 *   <constant name="OK">no error</constant>
 *   <constant name="BAD_SIGNAL">no such signal</constant>
 *   <constant name="ACCESS_DENIED">permission denied</constant>
 *   <constant name="NO_PROCESS">no such process</constant>
 *   <constant name="ERROR">another, unspecified error</constant>
 *  </type>
 * </constants>
 */
WXJS_BEGIN_CONSTANT_MAP(Process)
    WXJS_CONSTANT(wxPROCESS_, DEFAULT)
    WXJS_CONSTANT(wxPROCESS_, REDIRECT)
WXJS_END_CONSTANT_MAP()

void Process::InitClass(JSContext *cx, JSObject *obj, JSObject *proto)
{
    JSConstDoubleSpec wxSignalType[] = 
    {
        WXJS_CONSTANT(wx, SIGNONE)
        WXJS_CONSTANT(wx, SIGHUP)
        WXJS_CONSTANT(wx, SIGINT)
        WXJS_CONSTANT(wx, SIGQUIT)
        WXJS_CONSTANT(wx, SIGILL)
        WXJS_CONSTANT(wx, SIGTRAP)
        WXJS_CONSTANT(wx, SIGABRT)
        WXJS_CONSTANT(wx, SIGEMT)
        WXJS_CONSTANT(wx, SIGFPE)
        WXJS_CONSTANT(wx, SIGKILL)
        WXJS_CONSTANT(wx, SIGBUS)
        WXJS_CONSTANT(wx, SIGSEGV)
        WXJS_CONSTANT(wx, SIGSYS)
        WXJS_CONSTANT(wx, SIGPIPE)
        WXJS_CONSTANT(wx, SIGALRM)
        WXJS_CONSTANT(wx, SIGTERM)
        { 0 }
    };

    JSObject *constObj = JS_DefineObject(cx, obj, "wxSignal", 
									 	 NULL, NULL,
							             JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineConstDoubles(cx, constObj, wxSignalType);

    JSConstDoubleSpec wxKillErrorType[] = 
    {
        WXJS_CONSTANT(wxKILL_, OK)
        WXJS_CONSTANT(wxKILL_, BAD_SIGNAL)
        WXJS_CONSTANT(wxKILL_, ACCESS_DENIED)
        WXJS_CONSTANT(wxKILL_, NO_PROCESS)
        WXJS_CONSTANT(wxKILL_, ERROR)
		{ 0 }
    };
    constObj = JS_DefineObject(cx, obj, "wxKillError", 
						 	   NULL, NULL,
				               JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineConstDoubles(cx, constObj, wxKillErrorType);
}

/***
 * <properties>
 *  <property name="errorStream" type="@wxInputStream" readonly="Y">
 *   Returns an input stream which corresponds to the standard error output (stderr) of the child process.
 *  </property>
 *  <property name="inputStream" type="@wxInputStream" readonly="Y">
 *   It returns an input stream corresponding to the standard output stream of the subprocess. 
 *   If it is null, you have not turned on the redirection 
 *  </property>
 *  <property name="outputStream" type="@wxOutputStream" readonly="Y">
 *   It returns an output stream corresponding to the input stream of the subprocess. 
 *   If it is null, you have not turned on the redirection.
 *  </property>
 *  <property name="errorAvailable" type="Boolean" readonly="Y">
 *   Returns true if there is data to be read on the child process standard error stream.
 *  </property>
 *  <property name="inputAvailable" type="Boolean" readonly="Y">
 *   Returns true if there is data to be read on the child process standard output stream.
 *  </property>
 *  <property name="inputOpened" type="Boolean" readonly="Y">
 *   Returns true if the child process standard output stream is opened.
 *  </property>
 *  <property name="pid" type="Integer" readonly="Y">
 *   Returns the process ID of the process launched by @wxProcess#open.
 *  </property>
 * </properties>
 */
WXJS_BEGIN_PROPERTY_MAP(Process)
    WXJS_READONLY_PROPERTY(P_ERROR_STREAM, "errorStream")
    WXJS_READONLY_PROPERTY(P_INPUT_STREAM, "inputStream")
    WXJS_READONLY_PROPERTY(P_OUTPUT_STREAM, "outputStream")
    WXJS_READONLY_PROPERTY(P_ERROR_AVAILABLE, "errorAvailable")
    WXJS_READONLY_PROPERTY(P_INPUT_AVAILABLE, "inputAvailable")
    WXJS_READONLY_PROPERTY(P_INPUT_OPENED, "inputOpened")
    WXJS_READONLY_PROPERTY(P_PID, "pid")
WXJS_END_PROPERTY_MAP()

bool Process::GetProperty(Process *p, JSContext *cx, JSObject *obj, int id, jsval *vp)
{
    switch(id)
    {
    case P_ERROR_STREAM:
        {
            wxInputStream *in = p->GetErrorStream();
            if ( in != NULL )
            {
                *vp = InputStream::CreateObject(cx, new Stream(in));
            }
        }
        break;
    case P_INPUT_STREAM:
        {
            wxInputStream *in = p->GetInputStream();
            if ( in != NULL )
            {
                *vp = InputStream::CreateObject(cx, new Stream(in));
            }
        }
        break;
    case P_OUTPUT_STREAM:
        {
            wxOutputStream *out = p->GetOutputStream();
            if ( out != NULL )
            {
                *vp = OutputStream::CreateObject(cx, new Stream(out));
            }
        }
        break;
    case P_ERROR_AVAILABLE:
        *vp = ToJS(cx, p->IsErrorAvailable());
        break;
    case P_INPUT_AVAILABLE:
        *vp = ToJS(cx, p->IsInputAvailable());
        break;
    case P_INPUT_OPENED:
        *vp = ToJS(cx, p->IsInputOpened());
        break;
    case P_PID:
        *vp = ToJS(cx, p->GetPid());
        break;
    }
    return true;
}

/***
 * <ctor>
 *  <function>
 *   <arg name="Flag" type="Integer" default="wxProcess.DEFAULT" />
 *  </function>
 *  <desc>
 *   Constructs a new wxProcess object.
 *  </desc>
 * </ctor>
 */
Process *Process::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    int flags = wxPROCESS_DEFAULT;
	if ( argc == 1 )
	{
        if ( ! FromJS(cx, argv[0], flags) )
            return NULL;
	}
	return new Process(cx, obj, flags);
}

void Process::Destruct(JSContext *cx, Process *p)
{
}

WXJS_BEGIN_METHOD_MAP(Process)
	WXJS_METHOD("closeOutput", closeOutput, 0) 
	WXJS_METHOD("detach", detach, 0) 
    WXJS_METHOD("redirect", redirect, 0)
WXJS_END_METHOD_MAP()

/***
 * <method name="closeOutput">
 *  <function />
 *  <desc>
 *   Closes the output stream (the one connected to the stdin of the child process). 
 *   This function can be used to indicate to the child process that there is no more 
 *   data to be read - usually, a filter program will only terminate when the input stream is closed.
 *  </desc>
 * </method>
 */
JSBool Process::closeOutput(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	Process *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

    p->CloseOutput();

	return JS_TRUE;
}

/***
 * <method name="detach">
 *  <function />
 *  <desc>
 *   Normally, a wxProcess object is destroyed when it receives the notification about the process termination.
 *   However, it might happen that the parent application ends before the external process is terminated 
 *   (e.g. a window from which this external process was launched is closed by the user) and in this case 
 *   it should detach.
 *  </desc>
 * </method>
 */
JSBool Process::detach(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	Process *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

    p->Detach();

	return JS_TRUE;
}

/***
 * <method name="redirect">
 *  <function />
 *  <desc>
 *   Turns on redirection. @wxExecute will try to open a couple of pipes to catch the subprocess stdio. 
 *   The caught input stream is returned by @wxProcess#outputStream as a non-seekable stream. The caught
 *   output stream is returned by @wxProcess#inputStream as a non-seekable stream.
 *  </desc>
 * </method>
 */
JSBool Process::redirect(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	Process *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

    p->Redirect();

	return JS_TRUE;
}

WXJS_BEGIN_STATIC_METHOD_MAP(Process)
	WXJS_METHOD("kill", kill, 1)
    WXJS_METHOD("exists", exists, 1)
WXJS_END_METHOD_MAP()

/***
 * <class_method name="kill">
 *  <function returns="@wxKillError">
 *   <arg name="pid" type="Integer">The process ID</arg>
 *   <arg name="signal" type="Integer" default="wxSignal.SIGNONE">The signal to send to the process</arg>
 *   <arg name="killChildren" type="Boolean" default="false">Kill child processes?</arg>
 *  </function>
 *  <desc>
 *   Send the specified signal to the given process.
 *  </desc>
 * </class_method>
 */
JSBool Process::kill(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	Process *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

    if ( argc > 3 )
        argc = 3;

    int signal = wxSIGNONE;
    bool killChildren = false;
    switch(argc)
    {
    case 3:
        if ( ! FromJS(cx, argv[2], killChildren) )
            break;
        // Fall through
    case 2:
       if ( ! FromJS(cx, argv[1], signal ) )
            break;
       // Fall through
    default:
        {
            long pid;
            if ( FromJS(cx, argv[0], pid) )
            {
                *rval = ToJS(cx, (int) wxProcess::Kill(pid, (wxSignal) signal, killChildren ? wxKILL_CHILDREN : wxKILL_NOCHILDREN));
                return JS_TRUE;
            }
        }
    }
	return JS_FALSE;
}

/***
 * <class_method name="exists">
 *  <function returns="Boolean">
 *   <arg name="Pid" type="Integer">Process ID</arg>
 *  </function>
 *  <desc>
 *   Returns true if the given process exists in the system
 *  </desc>
 * </class_method>
 */
JSBool Process::exists(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	Process *p = GetPrivate(cx, obj);
	if ( p == NULL )
		return JS_FALSE;

    int pid;
    if ( FromJS(cx, argv[0], pid) )
    {
        *rval = ToJS(cx, wxProcess::Exists(pid));
        return JS_TRUE;
    }
    return JS_FALSE;
}
