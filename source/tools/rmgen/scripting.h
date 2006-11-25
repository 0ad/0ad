#ifndef SCRIPTING_H
#define SCRIPTING_H

#define XP_WIN
#include <js/jsapi.h>
#include <string>
#include <vector>
#include <iostream>
#include <cstdlib>

extern JSContext* cx;
extern JSObject* global;

// Initialize scripting, declaring all our JS classes and functions
void ScriptingInit();

// Run a script 
void RunScript(const std::string& filename);

// Shortcut for reporting a nice error to the user if an assertion fails
#define jsASSERT(expr) if(!(expr)) { JS_ReportError(cx, \
	"Assertion failed in %s (%s:%d): " #expr, __FUNCTION__, __FILE__, __LINE__); return JS_FALSE; }

// A namespace containing utility functions used internally by the scripting API
namespace js {
	// An interface to a property object (we'll have a subclass below once we have 
	// the conversion functions, but this is needed for Class which is needed for
	// some of the conversion functions)
	
	class AbstractProperty {
	public:
		virtual ~AbstractProperty() {};
		virtual JSBool set(JSObject *obj, jsval *val) = 0;
		virtual JSBool get(JSObject *obj, jsval *rval) = 0;
	};
	
	// An easy way to wrap an entire class into JavaScript; just call add on
	// the methods and properties, then call Class<T>::init.
	
	template<typename T> class Class
	{
	public:
		static JSClass jsClass;
	private:
		static JSNative constructor;
		static std::vector<JSFunctionSpec> methodSpecs;
		static std::vector<JSPropertySpec> propertySpecs;
		static std::vector<AbstractProperty*> properties;
	public:
		static void addMethod(JSNative func, int numArgs, const char* name) {
			JSFunctionSpec spec = { name, func, numArgs, 0, 0 };
			methodSpecs.push_back(spec);
		}
	
		static void addProperty(AbstractProperty* (*propGen)(), const char* name) {
			AbstractProperty* prop = propGen();
			int index = properties.size();
			properties.push_back(prop);
			JSPropertySpec spec = { name, index, JSPROP_ENUMERATE };
			propertySpecs.push_back(spec);
		}
	private:
		static JSBool getProperty(JSContext*, JSObject *obj, jsval id, jsval *vp) {
			if(!JSVAL_IS_INT(id)) {
				if(strcmp(JS_GetStringBytes(JSVAL_TO_STRING(id)), "constructor")==0) {
					// For some reason this is called for the constructor
					*vp = JSVAL_NULL; return JS_TRUE;
				}
				else {
					return JS_TRUE; 
				}
			};
			int index = JSVAL_TO_INT(id);
			properties[index]->get(obj, vp);
			return JS_TRUE;
		}
		
		static JSBool setProperty(JSContext*, JSObject *obj, jsval id, jsval *vp) {
			if(!JSVAL_IS_INT(id)) return JS_TRUE;
			int index = JSVAL_TO_INT(id);
			properties[index]->set(obj, vp);
			return JS_TRUE;
		}
	public:
		static void init(const char* name, JSNative cons) {
			constructor = cons;
			
			JSFunctionSpec* methods = new JSFunctionSpec[methodSpecs.size() + 1];
			for(size_t i=0; i<methodSpecs.size(); i++) {
				methods[i] = methodSpecs[i];
			}
			JSFunctionSpec endMethod = { 0 };
			methods[methodSpecs.size()] = endMethod;
			
			JSPropertySpec* props = new JSPropertySpec[propertySpecs.size() + 1];
			for(size_t i=0; i<propertySpecs.size(); i++) {
				props[i] = propertySpecs[i];
			}
			JSPropertySpec endProp = { 0 };
			props[propertySpecs.size()] = endProp;
			
			jsClass.name = name;
			jsClass.construct = cons;
			
			JS_InitClass(cx, global, 0, &jsClass, constructor, 0, 
				props, methods, 0, 0);
		}
	};
	
	template<typename T> struct PointerClass {
		typedef int type;
	};
	
	template<typename T> struct PointerClass<T*> {
		typedef Class<T> type;
	};
	
	// Conversion functions from  types to Javascript
	
	template<typename T> jsval ToJsval(T v) {	// T is really a pointer here (since we want to do ToJsval(pointer))
		if(v == 0) {
			return JSVAL_NULL;
		}
		else {
			JSObject* obj = JS_NewObject(cx, &(PointerClass<T>::type::jsClass), 0, 0);
			JS_SetPrivate(cx, obj, v);
			return OBJECT_TO_JSVAL(obj);
		}
	}
	
	template<> inline jsval ToJsval<bool>(bool v) {
		return v ? JSVAL_TRUE : JSVAL_FALSE;
	}
	
	template<> inline jsval ToJsval<int>(int v) {
		return INT_TO_JSVAL(v);
	}
	
	template<> inline jsval ToJsval<double>(double v) {
		jsval ret;
		JS_NewDoubleValue(cx, v, &ret);
		return ret;
	}
	
	template<> inline jsval ToJsval<float>(float v) {
		jsval ret;
		JS_NewDoubleValue(cx, v, &ret);
		return ret;
	}
	
	template<> inline jsval ToJsval<const char*>(const char* v) {
		int length = strlen(v);
		char* buf = (char*) JS_malloc(cx, length+1);
		memcpy(buf, v, length+1);
		return STRING_TO_JSVAL(JS_NewString(cx, buf, length+1));
	}
	
	template<> inline jsval ToJsval<const std::string&>(const std::string& v) {
		return ToJsval(v.c_str());
	}
	
	template<> inline jsval ToJsval<JSFunction*>(JSFunction* v) {
		if(v == 0) return JSVAL_NULL;
		else return OBJECT_TO_JSVAL(JS_GetFunctionObject(v));
	}
	
	// Conversion functions from Javascript to  types
	
	template<typename T> T To(jsval v) {	// T is really a pointer here (since we want to do To<pointer>(jsval))
		if(v == JSVAL_NULL) {
			return 0;
		}
		else {
			JSObject* obj = JSVAL_TO_OBJECT(v);
			return (T) JS_GetPrivate(cx, obj);
		}
	}
	
	template<> inline bool To<bool>(jsval v) {
		return (v==JSVAL_TRUE);
	}
	
	template<> inline int To<int>(jsval v) {
		return JSVAL_TO_INT(v);
	}
	
	template<> inline double To<double>(jsval v) {
		jsdouble d;
		JS_ValueToNumber(cx, v, &d);
		return d;
	}
	
	template<> inline float To<float>(jsval v) {
		jsdouble d;
		JS_ValueToNumber(cx, v, &d);
		return (float) d;
	}
	
	template<> inline std::string To<std::string>(jsval v) {
		return std::string(JS_GetStringBytes(JS_ValueToString(cx, v)));
	}
	
	template<> inline JSFunction* To<JSFunction*>(jsval v) {
		if(v == JSVAL_NULL) return 0;
		return JS_ValueToFunction(cx, v);
	}
	
	// A nice way of automatically creating a JS function for any  
	// function or method; could be made shorter by using some kind of
	// preprocessor loop (boost), but that might get confusing.
	// Combining member function pointers and templates... fun.
	
	// Functions with return type R and up to three arguments A, B, C
	
	template<typename R, R (*func)()> 
	JSBool Function
		(JSContext *cx, JSObject*, uintN argc, jsval *argv, jsval *rval)
	{
		jsASSERT(argc==0);
		*rval = ToJsval(func());
		return JS_TRUE;
	}
	
	template<typename R, typename A, R (*func)(A)> 
	JSBool Function
		(JSContext *cx, JSObject*, uintN argc, jsval *argv, jsval *rval)
	{
		jsASSERT(argc==1);
		*rval = ToJsval(func(To<A>(argv[0])));
		return JS_TRUE;
	}
	
	template<typename R, typename A, typename B, R (*func)(A,B)> 
		JSBool Function(JSContext *cx, JSObject*, uintN argc, jsval *argv, jsval *rval)
	{
		jsASSERT(argc==2);
		*rval = ToJsval(func(To<A>(argv[0]), To<B>(argv[1])));
		return JS_TRUE;
	}
	
	template<typename R, typename A, typename B, typename C, R (*func)(A,B,C)> 
		JSBool Function(JSContext *cx, JSObject*, uintN argc, jsval *argv, jsval *rval)
	{
		jsASSERT(argc==3);
		*rval = ToJsval(func(To<A>(argv[0]), To<B>(argv[1]), To<C>(argv[2])));
		return JS_TRUE;
	}
	
	// Void functions with up to three arguments A, B, C
	
	template<void (*func)()> 
	JSBool VoidFunction
		(JSContext *cx, JSObject*, uintN argc, jsval *argv, jsval *rval)
	{
		jsASSERT(argc==1);
		func();
		*rval = JSVAL_VOID;
		return JS_TRUE;
	}
	
	template<typename A, void (*func)(A)> 
	JSBool VoidFunction
		(JSContext *cx, JSObject*, uintN argc, jsval *argv, jsval *rval)
	{
		jsASSERT(argc==1);
		func(To<A>(argv[0]));
		*rval = JSVAL_VOID;
		return JS_TRUE;
	}
	
	template<typename A, typename B, void (*func)(A,B)> 
	JSBool VoidFunction
		(JSContext *cx, JSObject*, uintN argc, jsval *argv, jsval *rval)
	{
		jsASSERT(argc==2);
		func(To<A>(argv[0]), To<B>(argv[1]));
		*rval = JSVAL_VOID;
		return JS_TRUE;
	}
	
	template<typename A, typename B, typename C, void (*func)(A,B,C)> 
	JSBool VoidFunction
		(JSContext *cx, JSObject*, uintN argc, jsval *argv, jsval *rval)
	{
		jsASSERT(argc==3);
		func(To<A>(argv[0]), To<B>(argv[1]), To<C>(argv[2]));
		*rval = JSVAL_VOID;
		return JS_TRUE;
	}
	
	// Methods of O with return type R and up to three arguments A, B, C
	
	template<typename O, typename R, R (O::*func)()> 
	JSBool Method
		(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
	{
		jsASSERT(argc==0);
		O* o = (O*) JS_GetPrivate(cx, obj);
		*rval = ToJsval((o->*func)());
		return JS_TRUE;
	}
	
	template<typename O, typename R, typename A, R (O::*func)(A)> 
	JSBool Method
		(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
	{
		jsASSERT(argc==1);
		O* o = (O*) JS_GetPrivate(cx, obj);
		*rval = ToJsval((o->*func)(To<A>(argv[0])));
		return JS_TRUE;
	}
	
	template<typename O, typename R, typename A, typename B, R (O::*func)(A,B)> 
	JSBool Method
		(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
	{
		jsASSERT(argc==2);
		O* o = (O*) JS_GetPrivate(cx, obj);
		*rval = ToJsval((o->*func)(To<A>(argv[0]), To<B>(argv[1])));
		return JS_TRUE;
	}
	
	template<typename O, typename R, typename A, typename B, typename C, R (O::*func)(A,B,C)> 
	JSBool Method
		(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
	{
		jsASSERT(argc==3);
		O* o = (O*) JS_GetPrivate(cx, obj);
		*rval = ToJsval((o->*func)(To<A>(argv[0]), To<B>(argv[1]), To<C>(argv[2])));
		return JS_TRUE;
	}
	
	// Void methods of O with up to three arguments A, B, C
	
	template<typename O, void (O::*func)()> 
	JSBool VoidMethod
		(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
	{
		jsASSERT(argc==0);
		O* o = (O*) JS_GetPrivate(cx, obj);
		(o->*func)();
		*rval = JSVAL_VOID;
		return JS_TRUE;
	}
	
	template<typename O, typename A, void (O::*func)(A)> 
	JSBool VoidMethod
		(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
	{
		jsASSERT(argc==1);
		O* o = (O*) JS_GetPrivate(cx, obj);
		(o->*func)(To<A>(argv[0]));
		*rval = JSVAL_VOID;
		return JS_TRUE;
	}
	
	template<typename O, typename A, typename B, void (O::*func)(A,B)> 
	JSBool VoidMethod
		(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
	{
		jsASSERT(argc==2);
		O* o = (O*) JS_GetPrivate(cx, obj);
		(o->*func)(To<A>(argv[0]), To<B>(argv[1]));
		*rval = JSVAL_VOID;
		return JS_TRUE;
	}
	
	template<typename O, typename A, typename B, typename C, void (O::*func)(A,B,C)> 
	JSBool VoidMethod
		(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
	{
		jsASSERT(argc==3);
		O* o = (O*) JS_GetPrivate(cx, obj);
		(o->*func)(To<A>(argv[0]), To<B>(argv[1]), To<C>(argv[2]));
		*rval = JSVAL_VOID;
		return JS_TRUE;
	}
	
	
	// Constructors of O with up to three arguments A, B, C
	
	template<typename O> 
	JSBool Constructor
		(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
	{
		jsASSERT(argc==0);
		JS_SetPrivate(cx, obj, new O());
		return JS_TRUE;
	}
	
	template<typename O, typename A> 
	JSBool Constructor
		(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval*)
	{
		jsASSERT(argc==1);
		JS_SetPrivate(cx, obj, new O(To<A>(argv[0])));
		return JS_TRUE;
	}
	
	template<typename O, typename A, typename B> 
	JSBool Constructor
		(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval*)
	{
		jsASSERT(argc==2);
		JS_SetPrivate(cx, obj, new O(To<A>(argv[0]), To<B>(argv[1])));
		return JS_TRUE;
	}
	
	template<typename O, typename A, typename B, typename C> 
	JSBool Constructor
		(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval*)
	{
		jsASSERT(argc==3);
		JS_SetPrivate(cx, obj, new O(To<A>(argv[0]), To<B>(argv[1]), To<C>(argv[2])));
		return JS_TRUE;
	}
	
	template<typename O, typename A, typename B, typename C, typename D> 
	JSBool Constructor
		(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval*)
	{
		jsASSERT(argc==4);
		JS_SetPrivate(cx, obj, new O(
			To<A>(argv[0]), To<B>(argv[1]), To<C>(argv[2]), To<D>(argv[3])));
		return JS_TRUE;
	}
	
	// Implementation of AbstractProperty for a given field of an object
	
	template<typename O, typename T, T O::*field> class PropertyImpl: public AbstractProperty {
	public:
		virtual ~PropertyImpl() {}
		
		virtual JSBool set(JSObject *obj, jsval *val) {
			O* o = (O*) JS_GetPrivate(cx, obj);
			(o->*field) = To<T>(*val);
			return JS_TRUE;
		}
		
		virtual JSBool get(JSObject *obj, jsval *rval) {
			O* o = (O*) JS_GetPrivate(cx, obj);
			*rval = ToJsval(o->*field);
			return JS_TRUE;
		}
	};
	
	// A nice function to pass to AddProperty so we don't have to type new PropertyImpl<blah>()
	
	template<typename O, typename T, T O::*field> AbstractProperty* Property() {
		return new PropertyImpl<O, T, field>();
	}

}

// Call a void function with 1 argument
template<typename T> void CallJSFunction(JSFunction* f, T arg) {
	if(f == 0) {
		return;
	}
	
	jsval argv[1] = { js::ToJsval(arg) };
	jsval rval;
	JS_CallFunction(cx, global, f, 1, argv, &rval);
}

#endif

