/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/* 
ScriptableComplex.h

The version of CJSObject<> that retains the ability to use inheritance
in its objects. Shouldn't be used any more for anything but entity code.

This file contains only declarations of class CJSComplex and its methods.
Their implementations are in ScriptableComplex.inl. Because CJSComplex is 
a templated class, any source file that uses these methods directly must 
#include ScritpableComplex.inl to link to them. However, files that
only need to know that something is a CJSComplex need not do this. This
was done to speed up compile times after modifying CJSComplex's internals:
before, 30+ files had to be recompiled because they #included Entity.h
which #includes ScriptableComplex.h.
*/

#ifndef INCLUDED_SCRIPTABLECOMPLEX
#define INCLUDED_SCRIPTABLECOMPLEX

#include "scripting/ScriptingHost.h"
#include "simulation/ScriptObject.h"
#include "JSConversions.h"

#include <set>

class IJSComplex;

class IJSComplexProperty
{
public:

	bool m_AllowsInheritance;
	bool m_Inherited;
	bool m_Intrinsic;
	
	// This is to make sure that all the fields are initialized at construction
	inline IJSComplexProperty():
		m_AllowsInheritance(true),
		m_Inherited(true),
		m_Intrinsic(true)
	{}

	virtual jsval Get( JSContext* cx, IJSComplex* owner ) = 0;
	virtual void Set( JSContext* cx, IJSComplex* owner, jsval Value ) = 0;

	// Copies the data directly out of a parent property
	// Warning: Don't use if you're not certain the properties are not of the same type.
	virtual void ImmediateCopy( IJSComplex* CopyTo, IJSComplex* CopyFrom, IJSComplexProperty* CopyProperty ) = 0;

	jsval Get( IJSComplex* owner ) { return( Get( g_ScriptingHost.GetContext(), owner ) ); }
	void Set( IJSComplex* owner, jsval Value ) { return( Set( g_ScriptingHost.GetContext(), owner, Value ) ); }

	virtual ~IJSComplexProperty() {}
};

class IJSComplex
{
	// Make copy constructor and assignment operator private - since copying of
	// these objects is unsafe unless done specially.
	// These will never be implemented (they are, after all, here to *prevent*
	// copying)
	IJSComplex(const IJSComplex &other);
	IJSComplex& operator=(const IJSComplex &other);

public:
	typedef STL_HASH_MAP<CStrW, IJSComplexProperty*, CStrW_hash_compare> PropertyTable;
	typedef std::vector<IJSComplex*> InheritorsList;
	typedef std::set<CStrW> StringTable;
	typedef std::pair<StringTable, StringTable::iterator> IteratorState;
		
	// Used for freshen/update
	typedef void (IJSComplex::*NotifyFn)();

	// Property getters and setters
	typedef jsval (IJSComplex::*GetFn)();
	typedef void (IJSComplex::*SetFn)( jsval );

	// Properties of this object
	PropertyTable m_Properties;

	// Parent object
	IJSComplex* m_Parent;

	// Objects that inherit from this
	InheritorsList m_Inheritors;

	// Destructor
	virtual ~IJSComplex() { }
	
	// Set the base, and rebuild
	void SetBase( IJSComplex* m_Parent );

	// Rebuild any intrinsic (mapped-to-C++-variable) properties
	virtual void Rebuild() = 0;

	// HACK: Doesn't belong here.
	virtual void RebuildClassSet() = 0;
	
	// Check for a property
	virtual IJSComplexProperty* HasProperty( const CStrW& PropertyName ) = 0;

	// Get all properties of an object
	virtual void FillEnumerateSet( IteratorState* it, CStrW* PropertyRoot = NULL ) = 0;

	// Retrieve the value of a property (returning false if that property is not defined)
	virtual bool GetProperty( JSContext* cx, const CStrW& PropertyName, jsval* vp ) = 0;
	
	// Add a property (with immediate value)
	virtual void AddProperty( const CStrW& PropertyName, jsval Value ) = 0;
	virtual void AddProperty( const CStrW& PropertyName, const CStrW& Value ) = 0;

	inline IJSComplex() {}
};


class CJSReflector;

template<typename T, bool ReadOnly, typename ReturnType, ReturnType (T::*NativeFunction)( JSContext* cx, uintN argc, jsval* argv )> 
void AddMethodImpl( const char* Name, uintN MinArgs );

template<typename T, bool ReadOnly, typename PropType>
void AddClassPropertyImpl( const CStrW& PropertyName, PropType T::*Native, bool PropAllowInheritance = true, IJSComplex::NotifyFn Update = NULL, IJSComplex::NotifyFn Refresh = NULL );

template<typename T, bool ReadOnly, typename PropType>
void AddReadOnlyClassPropertyImpl( const CStrW& PropertyName, PropType T::*Native, bool PropAllowInheritance = true, IJSComplex::NotifyFn Update = NULL, IJSComplex::NotifyFn Refresh = NULL );

template<typename T, bool ReadOnly, typename PropType>
void MemberAddPropertyImpl( IJSComplex* obj, const CStrW& PropertyName, PropType* Native, bool PropAllowInheritance = true, IJSComplex::NotifyFn Update = NULL, IJSComplex::NotifyFn Refresh = NULL );

template<typename T, bool ReadOnly, typename PropType>
void MemberAddReadOnlyPropertyImpl( IJSComplex* obj, const CStrW& PropertyName, PropType* Native, bool PropAllowInheritance = true, IJSComplex::NotifyFn Update = NULL, IJSComplex::NotifyFn Refresh = NULL );

template<typename T, bool ReadOnly = false> class CJSComplex : public IJSComplex
{
public:
	typedef STL_HASH_MAP<CStrW, CJSReflector*, CStrW_hash_compare> ReflectorTable;
	template<typename Q> friend class CJSComplexPropertyAccessor;
	JSObject* m_JS;


	std::vector<CScriptObject> m_Watches;

	ReflectorTable m_Reflectors;

	static JSPropertySpec JSI_props[];
	static std::vector<JSFunctionSpec> m_Methods;
	static PropertyTable m_IntrinsicProperties;


public:
	static JSClass JSI_class;

	// Whether native code is responsible for managing this object.
	// Script constructors should clear this *BEFORE* creating a JS
	// mirror (otherwise it'll be rooted).

	bool m_EngineOwned;

	// JS Property access
	bool GetProperty( JSContext* cx, const CStrW& PropertyName, jsval* vp );
	void SetProperty( JSContext* cx, const CStrW& PropertyName, jsval* vp );
	void WatchNotify( JSContext* cx, const CStrW& PropertyName, jsval* newval );

	// 
	// Functions that must be provided to JavaScript
	// 
	static JSBool JSGetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
	static JSBool JSSetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp );
	static JSBool JSEnumerate( JSContext* cx, JSObject* obj, JSIterateOp enum_op, jsval* statep, jsid *idp );
	static JSBool SetWatchAll( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
	static JSBool UnWatchAll( JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval );
	static void ScriptingInit( const char* ClassName, JSNative Constructor = NULL, uintN ConstructorMinArgs = 0 );
	static void ScriptingShutdown();
	static void DefaultFinalize( JSContext *cx, JSObject *obj );

public:

	// Creating and releasing script objects is done automatically most of the time, but you
	// can do it explicitly. 
	void CreateScriptObject();
	void ReleaseScriptObject();

	JSObject* GetScript()
	{
		if( !m_JS )
                	CreateScriptObject();
        	return( m_JS );
	}

	CJSComplex();
	virtual ~CJSComplex();
	void Shutdown();
	void SetBase( IJSComplex* Parent );
	void Rebuild();


	IJSComplexProperty* HasProperty( const CStrW& PropertyName );

	void FillEnumerateSet( IteratorState* it, CStrW* PropertyRoot = NULL );
	
	
	void AddProperty( const CStrW& PropertyName, jsval Value );
	void AddProperty( const CStrW& PropertyName, const CStrW& Value );

	static void AddClassProperty( const CStrW& PropertyName, GetFn Getter, SetFn Setter = NULL );

	// these functions are themselves templatized. we don't want to implement
	// them in the header because that would drag in many dependencies.
	//
	// therefore, the publicly visible functions actually only call out to
	// external friend functions implemented in the .inl file.
	// these receive the template parameters from the class as well as the
	// ones added for the member function.
	// 
	// for non-static members, the friends additionally take a "this" pointer.
	template<typename ReturnType, ReturnType (T::*NativeFunction)( JSContext* cx, uintN argc, jsval* argv )> 
	static void AddMethod( const char* Name, uintN MinArgs )
	{
		AddMethodImpl<T, ReadOnly, ReturnType, NativeFunction>(Name, MinArgs);
	}
	
	template<typename PropType>
	static void AddClassProperty( const CStrW& PropertyName, PropType T::*Native, bool PropAllowInheritance = true, NotifyFn Update = NULL, NotifyFn Refresh = NULL )
	{
		AddClassPropertyImpl<T, ReadOnly, PropType>(PropertyName, Native, PropAllowInheritance, Update, Refresh);
	}

	template<typename PropType>
	static void AddReadOnlyClassProperty( const CStrW& PropertyName, PropType T::*Native, bool PropAllowInheritance = true, NotifyFn Update = NULL, NotifyFn Refresh = NULL )
	{
		AddReadOnlyClassPropertyImpl<T, ReadOnly, PropType>(PropertyName, Native, PropAllowInheritance, Update, Refresh);		
	}

	// PropertyName must not already exist! (verified in debug build)
	template<typename PropType>
	void AddProperty( const CStrW& PropertyName, PropType* Native, bool PropAllowInheritance = true, NotifyFn Update = NULL, NotifyFn Refresh = NULL )
	{
		MemberAddPropertyImpl<T, ReadOnly, PropType>(this, PropertyName, Native, PropAllowInheritance, Update, Refresh);
	}

	// PropertyName must not already exist! (verified in debug build)
	template<typename PropType>
	void AddReadOnlyProperty( const CStrW& PropertyName, PropType* Native, bool PropAllowInheritance = true, NotifyFn Update = NULL, NotifyFn Refresh = NULL )
	{
		MemberAddReadOnlyPropertyImpl<T, ReadOnly, PropType>(this, PropertyName, Native, PropAllowInheritance, Update, Refresh);
	}

	// helper routine for Add*Property. Their interface requires the
	// property not already exist; we check for this (in debug builds)
	// and if so, warn and free the previously new-ed memory in
	// m_Properties[PropertyName] (avoids mem leak).
	void DeletePreviouslyAssignedProperty( const CStrW& PropertyName );
};


//
// static members
//

template<typename T, bool ReadOnly> JSClass CJSComplex<T, ReadOnly>::JSI_class = {
	NULL, JSCLASS_HAS_PRIVATE | JSCLASS_NEW_ENUMERATE,
	JS_PropertyStub, JS_PropertyStub,
	JSGetProperty, JSSetProperty,
	(JSEnumerateOp)JSEnumerate, JS_ResolveStub,
	JS_ConvertStub, DefaultFinalize,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL
};

template<typename T, bool ReadOnly> JSPropertySpec CJSComplex<T, ReadOnly>::JSI_props[] = {
	{ NULL, 0, 0, NULL, NULL },
};

template<typename T, bool ReadOnly> std::vector<JSFunctionSpec> CJSComplex<T, ReadOnly>::m_Methods;
template<typename T, bool ReadOnly> typename CJSComplex<T, ReadOnly>::PropertyTable CJSComplex<T, ReadOnly>::m_IntrinsicProperties;



template<typename T>
void ScriptableComplex_InitComplexPropertyAccessor();



//
// suballocator for CJSComplex.m_Properties elements
// (referenced from implementation in .inl)
//
extern void jscomplexproperty_suballoc_attach();
extern void jscomplexproperty_suballoc_detach();
extern void* jscomplexproperty_suballoc();
extern void jscomplexproperty_suballoc_free(IJSComplexProperty* p);

#endif
