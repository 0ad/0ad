#include "precompiled.h"

/*
 * wxJavaScript - autoobj.cpp
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
 * $Id: autoobj.cpp 598 2007-03-07 20:13:28Z fbraem $
 */
// wxJSAutomationObject.cpp
#ifdef __WXMSW__

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/msw/ole/automtn.h>

#include "../../common/main.h"

#include "autoobj.h"

using namespace wxjs;
using namespace wxjs::gui;

/***
 * <file>misc/autoobj</file>
 * <module>gui</module>
 * <class name="wxAutomationObject">
 *  The wxAutomationObject class represents an OLE automation object containing 
 *  a single data member, an IDispatch pointer. It contains a number of functions
 *  that make it easy to perform automation operations, and set and get properties. 
 *  The wxVariant class will not be ported, because wxJS translates every type to
 *  the corresponding JavaScript type.
 *  This class is only available on a Windows platform.
 * </class>
 */
WXJS_INIT_CLASS(AutomationObject, "wxAutomationObject", 0)

/***
 * <ctor>
 *  <function />
 *  <desc>
 *   Constructs a new wxAutomationObject object. Unlike
 *   wxWidgets, this constructor can't take an 
 *   IDispatch pointer because this type can't be used in
 *   JavaScript.
 *  </desc>
 * </ctor>
 */
wxAutomationObject* AutomationObject::Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, bool constructing)
{
    return new wxAutomationObject();
}

WXJS_BEGIN_METHOD_MAP(AutomationObject)
    WXJS_METHOD("callMethod", callMethod, 1)
    WXJS_METHOD("createInstance", createInstance, 1)
    WXJS_METHOD("getInstance", getInstance, 1)
    WXJS_METHOD("getObject", getObject, 2)
    WXJS_METHOD("getProperty", getProperty, 1)
    WXJS_METHOD("putProperty", putProperty, 2)
WXJS_END_METHOD_MAP()

/***
 * <method name="callMethod">
 *  <function returns="Any">
 *   <arg name="Method" type="String">
 *    The name of the method to call.
 *   </arg>
 *   <arg name="Arg1" type="...">
 *    A variable list of arguments passed to the method.
 *   </arg>
 *  </function>
 *  <desc>
 *  Calls an automation method for this object. When the method
 *  returns a value it is not returned as a wxVariant but it is converted
 *  to its corresponding JavaScript type.
 *  <br /><br />
 *  The following example opens a workbook into Microsoft Excel:
 *  <pre><code class="whjs">
 *   var objExcel = new wxAutomationObject();
 *
 *   if ( objExcel.createInstance("Excel.Application") )
 *   {
 *      objExcel.putProperty("visible", true);
 *      var objBooks = new wxAutomationObject();
 *      objExcel.getObject(objBooks, "workbooks");
 *      objBooks.callMethod("open", "c:\\temp\\wxjs.xls");
 *   }
 *  </code></pre>
 *  The name of the method can contain dot-separated property names,
 *  to save the application needing to call @wxAutomationObject#getProperty
 *  several times using several temporary objects.
 *  For example:
 *  <code class="whjs">objExcel.callMethod("workbooks.open", "c:\\temp\\wxjs.xls");</code>
 *  </desc>
 * </method>
 */
JSBool AutomationObject::callMethod(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxAutomationObject *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxString method;
    FromJS(cx, argv[0], method);

    wxVariant res;
    if ( argc == 0 )
    {
        res = p->CallMethod(method);
    }
    else
    {
        const wxVariant **vars = new const wxVariant*[argc - 1];
        uint i;
        for(i = 1; i < argc; i++)
        {
            vars[i-1] = CreateVariant(cx, argv[i]);
        }

        *rval = CreateJSVal(cx, p->CallMethodArray(method, argc - 1, vars));

        for(i = 0; i < argc - 1; i++)
            delete vars[i];
        delete[] vars;
    }
    
    return JS_FALSE;
}

/***
 * <method name="createInstance">
 *  <function returns="Boolean">
 *   <arg name="classId" type="String" />
 *  </function>
 *  <desc>
 *   Creates a new object based on the class id, returning true if the
 *   object was successfully created, or false if not.
 *   <br /><br />
 *  The following example starts Microsoft Excel:
 *  <pre><code class="whjs">
 *  var obj = new wxAutomationObject();
 *  if ( obj.createInstance("Excel.Application") )
 *  {
 *    // Excel object is created
 *  }
 *  </code></pre>
 *  </desc>   
 * </method>
 */
JSBool AutomationObject::createInstance(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxAutomationObject *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxString classId;
    if ( FromJS(cx, argv[0], classId) )
    {
        *rval = ToJS(cx, p->CreateInstance(classId));
        return JS_TRUE;
    }
    return JS_FALSE;
}

/***
 * <method name="getInstance">
 *  <function returns="Boolean">
 *   <arg name="classId" type="String" />
 *  </function>
 *  <desc>
 *  Retrieves the current object associated with a class id, and attaches 
 *  the IDispatch pointer to this object. Returns true if a pointer was 
 *  successfully retrieved, false otherwise.
 *  </desc>
 * </method>
 */
JSBool AutomationObject::getInstance(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxAutomationObject *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxString classId;
    if ( FromJS(cx, argv[0], classId) )
    {
        *rval = ToJS(cx, p->GetInstance(classId));
        return JS_TRUE;
    }
    return JS_FALSE;
}

/***
 * <method name="putProperty">
 *  <function returns="Boolean">
 *   <arg name="Property" type="String">
 *    The name of the property.
 *   </arg>
 *   <arg name="Arg1" type="..." />
 *  </function>
 *  <desc>
 *   Puts a property value into this object.
 *   The following example starts Microsoft Excel and makes it visible.
 *   <pre><code class="whjs">
 *  var obj = new wxAutomationObject();
 *  if ( obj.createInstance("Excel.Application") )
 *  {
 *     obj.putProperty("visible", true);
 *  }
 *  </code></pre>
 *  The name of the property can contain dot-separated property names,
 *  to save the application needing to call @wxAutomationObject#getProperty
 *  several times using several temporary objects.
 *  </desc>
 * </method>
 */
JSBool AutomationObject::putProperty(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxAutomationObject *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxString prop;
    FromJS(cx, argv[0], prop);

    const wxVariant **vars = new const wxVariant*[argc - 1];
    uint i;
    for(i = 1; i < argc; i++)
    {
        vars[i-1] = CreateVariant(cx, argv[i]);
    }

    wxVariant res = p->PutPropertyArray(prop, argc - 1, vars);

    for(i = 0; i < argc - 1; i++)
        delete vars[i];
    delete[] vars;
    
    return JS_TRUE;
}

/***
 * <method name="getProperty">
 *  <function returns="Any">
 *   <arg name="Property" type="String">
 *    The name of the property.
 *   </arg>
 *   <arg name="Arg1" type="..." />
 *  </function>
 *  <desc>
 *   Gets a property from this object. The return type depends on the type of the property.
 *   Use @wxAutomationObject#getObject when the property is an object.
 *
 *   The name of the method can contain dot-separated property names,
 *   to save the application needing to call @wxAutomationObject#getProperty
 *   several times using several temporary objects.
 *  </desc>
 * </method>
 */
JSBool AutomationObject::getProperty(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxAutomationObject *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxString prop;
    FromJS(cx, argv[0], prop);
    
    const wxVariant **vars = new const wxVariant*[argc - 1];
    uint i;
    for(i = 1; i < argc; i++)
    {
        vars[i-1] = CreateVariant(cx, argv[i]);
    }

    wxVariant res = p->GetPropertyArray(prop, argc - 1, vars);
    *rval = CreateJSVal(cx, res);

    for(i = 0; i < argc - 1; i++)
        delete vars[i];
    delete[] vars;
    
    return JS_TRUE;
}

/***
 * <method name="getObject">
 *  <function returns="Boolean">
 *   <arg name="Object" type="@wxAutomationObject">
 *    The automation object that receives the property.
 *   </arg>
 *   <arg name="Property" type="String">
 *    The name of the property.
 *   </arg>
 *   <arg name="Arg1" type="..." />
 *  </function>
 *  <desc>
 *   Gets a property from this object. The type of the property is also an object.
 *   The following example retrieves the object Workbooks from an Excel application:
 *   <pre><code class="whjs">
 *  var objExcel = new wxAutomationObject();
 *  if ( objExcel.createInstance("Excel.Application") )
 *  {
 *    objExcel.putProperty("visible", true);
 *    var objBooks = new wxAutomationObject();
 *    objExcel.getObject(objBooks, "workbooks");
 *  }
 *  </code></pre>
 *   The name of the method can contain dot-separated property names,
 *   to save the application needing to call @wxAutomationObject#getProperty
 *   several times using several temporary objects.
 *  </desc>
 * </method>
 */
JSBool AutomationObject::getObject(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    wxAutomationObject *p = GetPrivate(cx, obj);
    if ( p == NULL )
        return JS_FALSE;

    wxAutomationObject *autoObj = GetPrivate(cx, argv[0]);
    if ( autoObj == NULL )
        return JS_FALSE;

    wxString prop;
    FromJS(cx, argv[1], prop);

    if ( argc > 2 )
    {
        const wxVariant **vars = new const wxVariant*[argc - 2];
        uint i;
        for(i = 2; i < argc; i++)
        {
            vars[i-2] = CreateVariant(cx, argv[i]);
        }

        *rval = ToJS(cx, p->GetObject(*autoObj, prop, argc - 2, vars));

        for(i = 0; i < argc - 2; i++)
        {
            delete vars[i];
        }
        delete[] vars;
    }
    else
        *rval = ToJS(cx, p->GetObject(*autoObj, prop));

 
    return JS_TRUE;
}

wxVariant *AutomationObject::CreateVariant(JSContext *cx, jsval v)
{
    switch(JS_TypeOfValue(cx, v))
    {
    case JSTYPE_VOID:
        break;
    case JSTYPE_OBJECT:
        {
	        JSObject *obj = JSVAL_TO_OBJECT(v);
	        if ( js_DateIsValid(cx, obj) )
            {
                wxDateTime date;
                FromJS(cx, v, date);
                return new wxVariant(date);
            }
           
            wxAutomationObject *o = GetPrivate(cx, obj);
            if ( o != NULL )
            {
                return new wxVariant((void *) o);
            }

            if ( JS_IsArrayObject(cx, obj) == JS_TRUE )
            {
            }
            break;
        }
    case JSTYPE_FUNCTION:
        break;
    case JSTYPE_STRING:
        {
            wxString str;
            FromJS(cx, v, str);
            return new wxVariant(str);
            break;
        }
    case JSTYPE_NUMBER:
        if ( JSVAL_IS_INT(v) )
        {
            long value;
            FromJS(cx, v, value);
            return new wxVariant(value);
        }
        else if ( JSVAL_IS_DOUBLE(v) )
        {
            double value;
            FromJS(cx, v, value);
            return new wxVariant(value);
        }
        break;
    case JSTYPE_BOOLEAN:
        {
            bool b;
            FromJS(cx, v, b);
            return new wxVariant(b);
        }
    }

    return new wxVariant();
}

jsval AutomationObject::CreateJSVal(JSContext *cx, const wxVariant &var)
{
    if ( var.IsNull() )
        return JSVAL_VOID;

    wxString type(var.GetType());

    if ( type.CompareTo(wxT("string")) == 0 )
    {
        return ToJS(cx, var.GetString());
    }
    else if ( type.CompareTo(wxT("bool")) == 0 )
    {
        return ToJS(cx, var.GetBool());
    }
    else if ( type.CompareTo(wxT("list")) == 0 )
    {
        wxLogDebug(wxT("List"));
    }
    else if ( type.CompareTo(wxT("long")) == 0 )
    {
        return ToJS(cx, var.GetLong());
    }
    else if ( type.CompareTo(wxT("double")) == 0 )
    {
        return ToJS(cx, var.GetDouble());
    }
    else if ( type.CompareTo(wxT("char")) == 0 )
    {
		return ToJS(cx, wxString::FromAscii(var.GetChar()));
    }
    else if ( type.CompareTo(wxT("time")) == 0 )
    {
        return ToJS(cx, var.GetDateTime());
    }
    else if ( type.CompareTo(wxT("date")) == 0 )
    {
        return ToJS(cx, var.GetDateTime());
    }
    else if ( type.CompareTo(wxT("void*")) == 0 )
    {
        // void* means an object
        void* p = var.GetVoidPtr();
        if ( p != NULL )
        {
            // We need to create a new wxAutomationObject because
            // otherwise the object is released to early.
            wxAutomationObject *o = static_cast<wxAutomationObject*>(p);
            wxAutomationObject *newObj = new wxAutomationObject();
            newObj->SetDispatchPtr(o->GetDispatchPtr());
            return CreateObject(cx, newObj);
        }
    }
    else if ( type.CompareTo(wxT("datetime")) == 0 )
    {
        return ToJS(cx, var.GetDateTime());
    }
    else if ( type.CompareTo(wxT("arrstring")) == 0 )
    {
		return ToJS(cx, var.GetArrayString());
    }
 
    return JSVAL_VOID;
}

#endif // WXJS_FOR_UNIX
