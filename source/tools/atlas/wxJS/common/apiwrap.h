/*
 * wxJavaScript - apiwrap.h
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
 * $Id: apiwrap.h 810 2007-07-13 20:07:05Z fbraem $
 */

// ApiWrapper uses the Barton and Nackman trick (also known as
// "Curiously Recursive Template Pattern")

#ifndef _WXJS_APIWRAPPER_H
#define _WXJS_APIWRAPPER_H

#include <wx/string.h>
#include "../common/type.h"

namespace wxjs
{

  template<class T_Port, class T_Priv>
  class ApiWrapper
  {
    public:
      typedef ApiWrapper<T_Port, T_Priv> TOBJECT;

      /**
       * Creates an object for the given WX class. From now
       * on wxJS owns the pointer to the WX class. So don't delete it.
       */
      static jsval CreateObject(JSContext *cx, 
                                T_Priv *p, 
                                JSObject *parent = NULL)
      {
        JSObject *obj = JS_NewObject(cx, &wxjs_class, m_classProto, parent);
        if ( obj == NULL )
          return JSVAL_NULL;

        JS_SetPrivate(cx, obj, p);
        return OBJECT_TO_JSVAL(obj);
      }

      // A type-safe SetPrivate method
      static void SetPrivate(JSContext *cx, JSObject *obj, T_Priv *p)
      {
        JS_SetPrivate(cx, obj, p);
      }

      /**
       * Creates an object for the given WX class and defines it as a 
       * property of the given object. From now on wxJS owns the pointer
       * to the WX class. So don't delete it.
       */
      static JSObject* DefineObject(JSContext *cx, 
                                    JSObject *obj, 
                                    const char *name, 
                                    T_Priv *p)
      {
        JSObject *propObj = JS_DefineObject(cx, obj, name,
                                            &wxjs_class, m_classProto, 
                                            JSPROP_READONLY | JSPROP_PERMANENT);
        if ( propObj )
        {
          JS_SetPrivate(cx, propObj, p);
        }
        return propObj;
      }

      /**
       * Returns the ported class from the private data of an object.
       * When check is true, it will check the type of the class and
       * returns NULL, when the class is not of the correct type.
       */
      static T_Priv* GetPrivate(JSContext *cx, 
                                JSObject *obj, 
                                bool check = true)
      {
        T_Priv *p = NULL;
        if ( check )
        {
          if (  
               ! (    JS_InstanceOf(cx, obj, &wxjs_class, NULL)
                   || HasPrototype(cx, obj)) 
                 )
          {
            JS_ReportError(cx, 
                           "The object should be an instance of %s", 
                           m_jsClassName);
            return NULL;
          }
        }

         JSClass *clazz = JS_GET_CLASS(cx, obj);
         if ( clazz == NULL )
           return NULL;

         while((clazz->flags & JSCLASS_HAS_PRIVATE) != JSCLASS_HAS_PRIVATE)
         {
           obj = JS_GetPrototype(cx, obj);
           if ( obj == NULL )
             return NULL;
           clazz = JS_GET_CLASS(cx, obj);
           if ( clazz == NULL )
             return NULL;
         }

         p = (T_Priv*) JS_GetPrivate(cx, obj);

         return p;
        }

      /**
       * Returns the ported class from the private data of an object.
       * Does the same as above, but for an object which is stored in a jsval.
       */
      static T_Priv* GetPrivate(JSContext *cx, jsval v, bool check = true)
      {
        if (    JSVAL_IS_VOID(v) 
             || JSVAL_IS_NULL(v) )
        {
          return NULL;
        }

        return JSVAL_IS_OBJECT(v) ? GetPrivate(cx, JSVAL_TO_OBJECT(v), check) 
                                  : NULL;
      }

      /**
       *  Returns true when the prototype of the object is this class.
       */
      static bool HasPrototype(JSContext *cx, JSObject *obj)
      {
        JSObject *prototype = JS_GetPrototype(cx, obj);
        while(   prototype != NULL
              && JS_InstanceOf(cx, prototype, &wxjs_class, NULL)== JS_FALSE )
        {
          prototype = JS_GetPrototype(cx, prototype);
        }
        return prototype != NULL;
      }

      /**
       * Same as above, but for an object that is stored in a jsval
       */
      static bool HasPrototype(JSContext *cx, jsval v)
      {
        return JSVAL_IS_OBJECT(v) ? HasPrototype(cx, JSVAL_TO_OBJECT(v))
                                  : false;
      }

      /**
       * Initializes the class.
       */
      static JSObject* JSInit(JSContext *cx, 
                              JSObject *obj,
                              JSObject *proto = NULL)
      {
        m_classProto = JS_InitClass(cx, obj, proto, &wxjs_class, 
                                    T_Port::JSConstructor, m_ctorArguments, 
                                    NULL, NULL, NULL, NULL);
        if ( m_classProto != NULL )
        {
          T_Port::DefineProperties(cx, m_classProto);
          T_Port::DefineMethods(cx, m_classProto);

          JSObject *ctor = JS_GetConstructor(cx, m_classProto);
          if ( ctor != NULL )
          {
            T_Port::DefineConstants(cx, ctor);
            T_Port::DefineStaticProperties(cx, ctor);
            T_Port::DefineStaticMethods(cx, ctor);
          }
          T_Port::InitClass(cx, obj, m_classProto);
        }
        return m_classProto;
      }

      /**
       * Default implementation for adding a property
       * Returning false, will end the execution of the script. 
       * The default implementation returns true.
       */
      static bool AddProperty(T_Priv* WXUNUSED(p), 
                              JSContext* WXUNUSED(cx), 
                              JSObject* WXUNUSED(obj), 
                              const wxString& WXUNUSED(prop),
                              jsval* WXUNUSED(vp))
      {
        return true;
      }

      /**
       * Default implementation for deleting a property
       * Returning false, will end the execution of the script. 
       * The default implementation returns true.
       */
      static bool DeleteProperty(T_Priv* WXUNUSED(p), 
                                 JSContext* WXUNUSED(cx), 
                                 JSObject* WXUNUSED(obj), 
                                 const wxString& WXUNUSED(prop))
      {
        return true;
      }

      /**
       *  The default implementation of the Get method for a ported object.
       *  Overwrite this method when your object has properties.
       *  Returning false, will end the execution of the script. 
       *  The default implementation returns true.
       */
      static bool GetProperty(T_Priv* WXUNUSED(p),
                              JSContext* WXUNUSED(cx),
                              JSObject* WXUNUSED(obj),
                              int WXUNUSED(id),
                              jsval* WXUNUSED(vp))
      {
        return true;
      }

      /**
       *  The default implementation of the Get method for a ported object.
       *  Overwrite this method when your object has properties.
       *  Returning false, will end the execution of the script. 
       *  The default implementation returns true.
       */
      static bool GetStringProperty(T_Priv* WXUNUSED(p),
                                    JSContext* WXUNUSED(cx),
                                    JSObject* WXUNUSED(obj),
                                    const wxString& WXUNUSED(propertyName), 
                                    jsval* WXUNUSED(vp))
      {
        return true;
      }

      /**
       *  The default implementation of the Set method for a ported object.
       *  Overwrite this method when your object has properties.
       *  @remark Returning false, will end the execution of the script. 
       *  The default implementation returns true.
       */
      static bool SetProperty(T_Priv* WXUNUSED(p),
                              JSContext* WXUNUSED(cx),
                              JSObject* WXUNUSED(obj),
                              int WXUNUSED(id),
                              jsval* WXUNUSED(vp))
      {
        return true;
      }

      static bool SetStringProperty(T_Priv* WXUNUSED(p),
                                    JSContext* WXUNUSED(cx), 
                                    JSObject* WXUNUSED(obj),
                                    const wxString& WXUNUSED(propertyName),
                                    jsval* WXUNUSED(vp))
      {
        return true;
      }

      static bool Resolve(JSContext* WXUNUSED(cx),
                          JSObject* WXUNUSED(obj),
                          jsval WXUNUSED(id))
      {
        return true;
      }

      /**
       *  The default implementation of the Destruct method. Overwrite this
       *  when you need to do some cleanup before the object is destroyed.
       *  The default implementation calls the destructor of the private 
       *  object.
       */
      static void Destruct(JSContext* WXUNUSED(cx),
                           T_Priv* p)
      {
        delete p;
        p = NULL;
      }

      /**
       *  The default implementation of the Construct method. Overwrite this 
       *  when a script is allowed to create an object with the new statement.
       *  The default implementation returns NULL, which means that is not 
       *  allowed to create an object.
       */
      static T_Priv* Construct(JSContext* WXUNUSED(cx), 
                               JSObject* WXUNUSED(obj),
                               uintN WXUNUSED(argc),
                               jsval* WXUNUSED(argv), 
                               bool WXUNUSED(constructing))
      {
        return NULL;
      }

      /**
       *  Default implementation for defining properties.
       *  Use the WXJS_DECLARE_PROPERTY_MAP, WXJS_BEGIN_PROPERTY_MAP and
       *  WXJS_END_PROPERTY_MAP macro's for hiding the complexity of 
       *  defining properties. The default implementation does nothing.
       */
      static void DefineProperties(JSContext* WXUNUSED(cx),
                                   JSObject* WXUNUSED(obj))
      {
      }

      /**
       * InitClass is called when the prototype object is created
       * It can be used for example to initialize constants related to 
       * this class.
       * The argument obj is normally the global object.
       * The default implementation does nothing.
       */
      static void InitClass(JSContext* WXUNUSED(cx),
                            JSObject* WXUNUSED(obj),
                            JSObject* WXUNUSED(proto))
      {
      }

      /**
       *  Default implementation for defining methods.
       *  Use the WXJS_DECLARE_METHOD_MAP, WXJS_BEGIN_METHOD_MAP and
       *  WXJS_END_METHOD_MAP macro's for hiding the complexity of 
       *  defining methods.
       *  The default implementation does nothing.
       */
      static void DefineMethods(JSContext* WXUNUSED(cx),
                                JSObject* WXUNUSED(obj))
      {
      }

      /**
       *  Default implementation for defining constants.
       *  Use the WXJS_DECLARE_CONSTANT_MAP, WXJS_BEGIN_CONSTANT_MAP and
       *  WXJS_END_CONSTANT_MAP macro's for hiding the complexity of
       *  defining constants.
       *  The default implementation does nothing.
       *  Only numeric constants are allowed.
       */
      static void DefineConstants(JSContext* WXUNUSED(cx),
                                  JSObject* WXUNUSED(obj))
      {
      }

      /**
       *  Default implementation for defining static(class) properties.
       *  Use the WXJS_DECLARE_STATIC_PROPERTY_MAP, 
       *  WXJS_BEGIN_STATIC_PROPERTY_MAP and WXJS_END_PROPERTY_MAP macro's 
       *  for hiding the complexity of defining properties.
       *  The default implementation does nothing.
       */
      static void DefineStaticProperties(JSContext* WXUNUSED(cx),
                                         JSObject* WXUNUSED(obj))
      {
      }

      /**
       *  Default implementation for defining static(class) methods.
       *  Use the WXJS_DECLARE_STATIC_METHOD_MAP, WXJS_BEGIN_STATIC_METHOD_MAP
       *  and WXJS_END_METHOD_MAP macro's for hiding the complexity of 
       *  defining methods.
       *  The default implementation does nothing.
       */
      static void DefineStaticMethods(JSContext* WXUNUSED(cx),
                                      JSObject* WXUNUSED(obj))
      {
      }

      /**
       * Returns the JSClass of the object
       */
      static JSClass* GetClass()
      {
        return &wxjs_class;
      }

      /**
       *  The default implementation of the static Get method for a ported 
       *  object. Overwrite this method when your object has static 
       *  properties.
       *  Returning false, will end the execution of the script. 
       *  The default implementation returns true.
       */
      static bool GetStaticProperty(JSContext* WXUNUSED(cx),
                                    int WXUNUSED(id),
                                    jsval* WXUNUSED(vp))
      {
        return true;
      }

      /**
       *  The default implementation of the static Set method for a ported 
       *  object.
       *  Overwrite this method when your object has static properties.
       *  Returning false, will end the execution of the script. 
       *  The default implementation returns true.
       */
      static bool SetStaticProperty(JSContext* WXUNUSED(cx), 
                                    int WXUNUSED(id),
                                    jsval* WXUNUSED(vp))
      {
        return true;
      }

      static bool Enumerate(T_Priv* WXUNUSED(p),
                            JSContext* WXUNUSED(cx),
                            JSObject* WXUNUSED(obj),
                            JSIterateOp WXUNUSED(enum_op),
                            jsval* WXUNUSED(statep),
                            jsid* WXUNUSED(idp))
      {
        return true;
      }

      // The JS API callbacks

      static JSBool JSGetStaticProperty(JSContext* cx, 
                                        JSObject* WXUNUSED(obj), 
                                        jsval id, 
                                        jsval* vp)
      {
        if ( JSVAL_IS_INT(id) )
        {
          return T_Port::GetStaticProperty(cx, JSVAL_TO_INT(id), vp) ? JS_TRUE 
                                                                     : JS_FALSE;
        }
        return JS_TRUE;
      }

      static JSBool JSSetStaticProperty(JSContext* cx, 
                                        JSObject* WXUNUSED(obj), 
                                        jsval id, 
                                        jsval *vp)
      {
        if ( JSVAL_IS_INT(id) )
        {
          return T_Port::SetStaticProperty(cx, JSVAL_TO_INT(id), vp) ? JS_TRUE 
                                                                     : JS_FALSE;
        }
        return JS_TRUE;
      }

      static JSObject *GetClassPrototype()
      {
          return m_classProto;
      }

    private:
      /**
       *  Contains the number of arguments that a constructor can receive. 
       *  This doesn't mean that the constructor always receives these number
       *  of arguments. SpiderMonkey makes sure that the constructor receives
       *  a correct number of arguments. When not all arguments are given, 
       *  SpiderMonkey will create arguments of the 'undefined' type. It's 
       *  also possible that the constructor receives more arguments.
       *  It's up to you to decide what happens with these arguments. 
       *  A rule of thumb: Set this value to the number of required arguments. 
       *  This way you never have to check the number of arguments when you 
       *  check the type of these arguments.  When argc is greater 
       *  then this value, you know there are optional values passed.
       *  You can use the WXJS_INIT_CLASS macro, to initialize this.
       */
      static int m_ctorArguments;

      /**
       * The prototype object of the class
       */
      static JSObject *m_classProto;

      /**
       *  The name of the class.
       *  You can use the WXJS_INIT_CLASS macro, to initialize this.
       */
      static const char* m_jsClassName;

      /**
       * The JSClass structure
       */
      static JSClass wxjs_class;

      /**
       * Enumeration callback
       */
      static JSBool JSEnumerate(JSContext *cx, JSObject *obj,
                                JSIterateOp enum_op,
                                jsval *statep, jsid *idp)
      {
        JSBool res = JS_TRUE;
        T_Priv *p = (T_Priv *) GetPrivate(cx, obj, false);
        if ( p != NULL )
        {
          res = T_Port::Enumerate(p, cx, obj, enum_op, statep, idp) 
                ? JS_TRUE : JS_FALSE;
        }
        return res;
      }

      /**
       *  AddProperty callback. This will call the AddProperty method of 
       *  the ported object.
       */
      static JSBool JSAddProperty(JSContext *cx, 
                                  JSObject *obj, 
                                  jsval id, 
                                  jsval *vp)
      {
        if (JSVAL_IS_STRING(id)) 
        {
          T_Priv *p = (T_Priv *) GetPrivate(cx, obj, false);
          if ( p != NULL )
          {
            wxString str;
            FromJS(cx, id, str);
            JSBool res = T_Port::AddProperty(p, cx, obj, str, vp) ? JS_TRUE 
                                                                  : JS_FALSE;
            return res;
          }
        }
        return JS_TRUE;
      }

      /**
       *  AddProperty callback. This will call the AddProperty method of 
       *  the ported object.
       */
      static JSBool JSDeleteProperty(JSContext *cx, 
                                     JSObject *obj, 
                                     jsval id, 
                                     jsval* WXUNUSED(vp))
      {
        if (JSVAL_IS_STRING(id)) 
        {
          T_Priv *p = (T_Priv *) GetPrivate(cx, obj, false);
          if ( p != NULL )
          {
            wxString str;
            FromJS(cx, id, str);
            JSBool res = T_Port::DeleteProperty(p, cx, obj, str) ? JS_TRUE 
                                                                  : JS_FALSE;
            return res;
          }
        }
        return JS_TRUE;
      }

      /**
       *  GetProperty callback. This will call the Get method of the 
       *  ported object.
       */
      static JSBool JSGetProperty(JSContext *cx, 
                                  JSObject *obj, 
                                  jsval id, 
                                  jsval *vp)
      {
        T_Priv *p = (T_Priv *) GetPrivate(cx, obj, false);
        if (JSVAL_IS_INT(id)) 
        {
          return T_Port::GetProperty(p, cx, obj, JSVAL_TO_INT(id), vp) 
                 ? JS_TRUE : JS_FALSE; 
        }
        else
        {
          if (JSVAL_IS_STRING(id)) 
          {
            wxString s;
            FromJS(cx, id, s);
            JSBool res = T_Port::GetStringProperty(p, cx, obj, s, vp) ? JS_TRUE 
                                                                     : JS_FALSE;
            return res;
          }
        } 
        return JS_TRUE;
      }

      /**
       *  SetProperty callback. This will call the Set method of the ported
       *  object.
       */
      static JSBool JSSetProperty(JSContext *cx, 
                                  JSObject *obj, 
                                  jsval id, 
                                  jsval *vp)
      {
        T_Priv *p = (T_Priv *) GetPrivate(cx, obj, false);
        if (JSVAL_IS_INT(id)) 
        {
          return T_Port::SetProperty(p, cx, obj, JSVAL_TO_INT(id), vp) 
                 ? JS_TRUE : JS_FALSE; 
        }
        else
        {
          if (JSVAL_IS_STRING(id)) 
          {
              wxString s;
              FromJS(cx, id, s);
              JSBool res = T_Port::SetStringProperty(p, cx, obj, s, vp) 
                           ? JS_TRUE : JS_FALSE;
              return res;
          }
        } 
        return JS_TRUE;
      }

      static JSBool JSResolve(JSContext *cx, JSObject *obj, jsval id)
      {
        return T_Port::Resolve(cx, obj, id) ? JS_TRUE : JS_FALSE;
      }

      /**
       *  Constructor callback. This will call the static Construct 
       *  method of the ported object.
       *  When this is not available, the ported object can't be created 
       *  with a new statement in JavaScript.
       */
      static JSBool JSConstructor(JSContext *cx, 
                                  JSObject *obj, 
                                  uintN argc, 
                                  jsval *argv, 
                                  jsval* WXUNUSED(rval))
      {
        T_Priv *p = T_Port::Construct(cx, obj, argc, argv, 
                                      JS_IsConstructing(cx) == JS_TRUE);
        if ( p == NULL )
        {
          JS_ReportError(cx, "Class %s can't be constructed", m_jsClassName);
          return JS_FALSE;
        }
        JS_SetPrivate(cx, obj, p);
        return JS_TRUE;
      }

      /**
       * Destructor callback. This will call the Destruct method of the
       * ported object.
       */
      static void JSDestructor(JSContext *cx, JSObject *obj)
      {
        T_Priv *p = (T_Priv *) JS_GetPrivate(cx, obj);
        if ( p != NULL )
        {
          T_Port::Destruct(cx, p);
        }
      }
    };
}; // namespace wxjs

// The initialisation of wxjs_class
template<class T_Port, class T_Priv>
JSClass wxjs::ApiWrapper<T_Port, T_Priv>::wxjs_class =
{ 
    wxjs::ApiWrapper<T_Port, T_Priv>::m_jsClassName,
    JSCLASS_HAS_PRIVATE | JSCLASS_NEW_ENUMERATE,
    wxjs::ApiWrapper<T_Port, T_Priv>::JSAddProperty,
    wxjs::ApiWrapper<T_Port, T_Priv>::JSDeleteProperty,
    wxjs::ApiWrapper<T_Port, T_Priv>::JSGetProperty,
    wxjs::ApiWrapper<T_Port, T_Priv>::JSSetProperty, 
    (JSEnumerateOp) wxjs::ApiWrapper<T_Port, T_Priv>::JSEnumerate,
    wxjs::ApiWrapper<T_Port, T_Priv>::JSResolve,
    JS_ConvertStub, 
    wxjs::ApiWrapper<T_Port, T_Priv>::JSDestructor,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

// Some usefull macro's that makes the use of ApiWrapper easy
// PROPERTY MACROS
#define WXJS_NORMAL   JSPROP_ENUMERATE | JSPROP_PERMANENT 
#define WXJS_READONLY JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_READONLY

// Declare a property map (use it in headers)
#define WXJS_DECLARE_PROPERTY_MAP() \
    static void DefineProperties(JSContext *cx, JSObject *obj);

// Declare a static property map (use it in headers)
#define WXJS_DECLARE_STATIC_PROPERTY_MAP() \
    static void DefineStaticProperties(JSContext *cx, JSObject *obj);

// Begins a property map (use it in source files)
#define WXJS_BEGIN_PROPERTY_MAP(className) \
    void className::DefineProperties(JSContext *cx, JSObject *obj) \
    { \
        JSPropertySpec props[] = \
        {
// Ends a property map (use it in source files)
#define WXJS_END_PROPERTY_MAP() \
            { 0, 0, 0, 0, 0 } \
        }; \
        JS_DefineProperties(cx, obj, props); \
    }
// Begins a static property map
#define WXJS_BEGIN_STATIC_PROPERTY_MAP(className) \
    void className::DefineStaticProperties(JSContext *cx, JSObject *obj) \
    { \
        JSPropertySpec props[] = \
        {

// Defines a property
#define WXJS_PROPERTY(id, name) \
    { name, id, WXJS_NORMAL, 0, 0 },

// Defines a static property
#define WXJS_STATIC_PROPERTY(id, name) \
    { name, id, WXJS_NORMAL, JSGetStaticProperty, JSSetStaticProperty },

// Defines a readonly property
#define WXJS_READONLY_PROPERTY(id, name) \
    { name, id, WXJS_READONLY, 0, 0 },

// Defines a readonly static property
#define WXJS_READONLY_STATIC_PROPERTY(id, name) \
    { name, id, WXJS_READONLY, JSGetStaticProperty, 0 },

// Declares a constant map
#define WXJS_DECLARE_CONSTANT_MAP() \
    static void DefineConstants(JSContext *cx, JSObject *obj);

// Begins a constant map
#define WXJS_BEGIN_CONSTANT_MAP(className)                         \
    void className::DefineConstants(JSContext *cx, JSObject *obj)  \
    {                                                              \
      JSConstDoubleSpec consts[] =                                 \
      {

// Ends a constant map
#define WXJS_END_CONSTANT_MAP()                 \
            { 0, 0, 0, { 0 } }                      \
        };                                      \
        JS_DefineConstDoubles(cx, obj, consts); \
    }

// Defines a constant with a prefix
#define WXJS_CONSTANT(prefix, name)	{ (int)prefix##name, #name, WXJS_READONLY, { 0 } },
// Defines a constant
#define WXJS_SIMPLE_CONSTANT(name)  { name, #name, WXJS_READONLY, { 0 } },

// METHOD MACROS
#define WXJS_DECLARE_METHOD_MAP() \
        static void DefineMethods(JSContext *cx, JSObject *obj);

#define WXJS_BEGIN_METHOD_MAP(className)                         \
    void className::DefineMethods(JSContext *cx, JSObject *obj)  \
    {                                                            \
        JSFunctionSpec methods[] =                               \
        {

#define WXJS_END_METHOD_MAP()                  \
            { 0, 0, 0, 0, 0 }                  \
        };                                     \
        JS_DefineFunctions(cx, obj, methods);  \
    }
 
#define WXJS_METHOD(name, fun, args) \
    { name, fun, args, 0, 0 },

// A macro to reduce the size of the ported classes header.
#define WXJS_DECLARE_METHOD(name) static JSBool name(JSContext *cx, \
                                                     JSObject *obj, \
                                                     uintN argc,    \
                                                     jsval *argv,   \
                                                     jsval *rval);

#define WXJS_DECLARE_STATIC_METHOD_MAP() \
    static void DefineStaticMethods(JSContext *cx, JSObject *obj);

#define WXJS_BEGIN_STATIC_METHOD_MAP(className)                       \
    void className::DefineStaticMethods(JSContext *cx, JSObject *obj) \
    {                                                                 \
        JSFunctionSpec methods[] =                                    \
        {

// CLASS MACROS
#define WXJS_INIT_CLASS(type, name, ctor)                             \
    namespace wxjs {                                                  \
        template<> JSObject *type::TOBJECT::m_classProto = NULL;      \
        template<> int type::TOBJECT::m_ctorArguments = ctor;         \
        template<> const char* type::TOBJECT::m_jsClassName = name;   \
    }
#endif //  _JSOBJECT_H
