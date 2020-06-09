/* Copyright (C) 2019 Wildfire Games.
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

#ifndef INCLUDED_JSI_IGUIOBJECT
#define INCLUDED_JSI_IGUIOBJECT

#include "scriptinterface/ScriptExtraHeaders.h"
#include "scriptinterface/ScriptInterface.h"

class CText;

namespace JSI_GUI
{
	class GUIObjectFactory;

	/**
	 * Handles the js interface with C++ GUI objects.
	 * Proxy handlers must live for at least as long as the JS runtime
	 * where a proxy object with that handler was created. The reason is that
	 * proxy handlers are called during GC, such as on runtime destruction.
	 * In practical terms, this means "just keep them static and store no data".
	 *
	 * Function properties are defined once per IGUIObject type and not on each object,
	 * so we fetch them from the GUIObjectFactory in the CGUI (via the object).
	 * To avoid running these fetches on any property access, and since we know the names of such properties
	 * at compile time, we pass them as template arguments and check that the prop name matches one.
	 *
	 * GUI Objects only exist in C++ and have no JS-only properties.
	 * As such, there is no "target" JS object that this proxy could point to,
	 * and thus we should inherit from BaseProxyHandler and not js::Wrapper.
	 */
	class GUIProxy : protected js::BaseProxyHandler
	{
		friend class GUIObjectFactory;
	public:
		GUIProxy();
		virtual ~GUIProxy() {};

	private:
		// See GUIObjectFactory on why we want a long-lived proxy.

		// Handler got object.x
		virtual bool get(JSContext* cx, JS::HandleObject proxy, JS::HandleValue receiver, JS::HandleId id, JS::MutableHandleValue vp) const final;
		// Handler for object.x = y;
		virtual bool set(JSContext* cx, JS::HandleObject proxy, JS::HandleId id, JS::HandleValue vp,
						 JS::HandleValue receiver, JS::ObjectOpResult& result) const final;
		// Handler for delete object.x;
		virtual bool delete_(JSContext* cx, JS::HandleObject proxy, JS::HandleId id, JS::ObjectOpResult& result) const final;

		// The following methods are not provided by BaseProxyHandler.
		// We provide a default that does nothing.

		// The JS code will see undefined when querying a property descriptor.
		virtual bool getOwnPropertyDescriptor(JSContext* UNUSED(cx), 
                                              JS::HandleObject UNUSED(proxy), 
                                              JS::HandleId UNUSED(id),
											  JS::MutableHandle<JS::PropertyDescriptor> UNUSED(desc)) const
		{
			return true;
		}
		// Throw an exception is JS code attempts defining a property.
		virtual bool defineProperty(JSContext* UNUSED(cx), JS::HandleObject UNUSED(proxy), JS::HandleId UNUSED(id),
									JS::Handle<JS::PropertyDescriptor> UNUSED(desc), JS::ObjectOpResult& UNUSED(result)) const
		{
			return false;
		}
		// Return nothing.
		virtual bool ownPropertyKeys(JSContext* UNUSED(cx), JS::HandleObject UNUSED(proxy), JS::MutableHandleIdVector UNUSED(props)) const
		{
			return true;
		}
		// Return nothing.
		virtual bool enumerate(JSContext* UNUSED(cx), JS::HandleObject UNUSED(proxy), JS::MutableHandleObject UNUSED(objp)) const
		{
			return true;
		}
		// We are not extensible.
		virtual bool preventExtensions(JSContext* UNUSED(cx), JS::HandleObject UNUSED(proxy), JS::ObjectOpResult& UNUSED(result)) const
		{
			return true;
		}
		virtual bool isExtensible(JSContext* UNUSED(cx), JS::HandleObject UNUSED(proxy), bool* extensible) const
		{
			*extensible = false;
			return true;
		}
        
        virtual bool getPrototypeIfOrdinary(JSContext* UNUSED(cx), JS::HandleObject UNUSED(proxy), bool* isOrdinary, JS::MutableHandleObject UNUSED(protop)) const
        {
            *isOrdinary = false;
            return true;
        }
        
		static GUIProxy singleton;
	};

	/**
	 * A GUIObjectFactory is created by CGUI for each object definition it has.
	 * This class is responsible for creating the JS counterparts to C++ objects,
	 * which are Proxy objects implement GUIProxy.
	 *
	 * A word on function properties.
	 *
	 * Some properties of JS-GUIObjects are functions, which must exist in the JS compartment
	 * (get returns the JSNative function, which then gets called from JS code).
	 * We could create them as properties of the newly created JS-GUIObject, but that would
	 * mean we have n-handlers * GUI objects definitions of the same function. We could create them
	 * everytime they are asked for, but that seems rather inefficient.
	 * It seems easier to create them once and always return the same JS functions.
	 * However, the lifetime of those JS functions must be the same as the lifetime of the compartment,
	 * (unlike the proxy handler which lives forever), so we must store them in an object whose lifetime
	 * is tied to the CGUI lifetime. We could, for now, store them in the IGUIObject directly, since we don't
	 * really create/destroy objects at runtime, but that's preventing future evolution.
	 * The simplest solution is to store them in the GUIObjectFactory.
	 */
	class GUIObjectFactory
	{
		friend class ::JSI_GUI::GUIProxy;
	public:
		/**
		 * This is the "least derived" object type that this factory can work with.
		 * A pointer to the c++ object is stored in the private data of the proxy, which is void*.
		 * We need to know what type to use when writing and reading the pointer.
		 * This is that type.
		 */
		using cppType = IGUIObject;

		GUIObjectFactory(ScriptInterface& scriptInterface);
		JSObject* CreateObject(JSContext* cx);
	protected:
		/**
		 * Call the "callable" member function, correctly converting input and output from and to JS.
		 * This is a generic method that does no particular transformation on JS inputs, provided for convenience.
		 * If some specific pre-processing of arguments is wanted, it should be specialised.
		 *
		 * Templated on on objType because `using cppType` isn't virtual-like.
		 *
		 * TODO: this method will generally not work for member functions with multiple overloads.
		 * It sounds fixable, but rather difficult.
		 */
		template <typename objType, typename funcPtr, funcPtr callable>
		static bool scriptMethod(JSContext* cx, unsigned argc, JS::Value* vp);

		std::map<std::string, JS::PersistentRootedFunction> m_FunctionHandlers;
		static js::Class m_ProxyObjectClass;
	};
	class TextObjectFactory : public GUIObjectFactory
	{
	public:
		using cppType = CText;
		TextObjectFactory(ScriptInterface& scriptInterface);
	};
}

#endif // INCLUDED_JSI_IGUIOBJECT
