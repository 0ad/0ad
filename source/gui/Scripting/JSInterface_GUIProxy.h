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

#ifndef INCLUDED_JSI_GUIPROXY
#define INCLUDED_JSI_GUIPROXY

#include "scriptinterface/ScriptExtraHeaders.h"

#include <utility>

class ScriptInterface;

// See JSI_GuiProxy below
#if GCC_VERSION
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#elif MSC_VERSION
# pragma warning(push, 1)
# pragma warning(disable: 4265)
#endif

/**
 * Handles the js interface with C++ GUI objects.
 * Proxy handlers must live for at least as long as the JS runtime
 * where a proxy object with that handler was created. The reason is that
 * proxy handlers are called during GC, such as on runtime destruction.
 * In practical terms, this means "just keep them static and store no data".
 *
 * GUI Objects only exist in C++ and have no JS-only properties.
 * As such, there is no "target" JS object that this proxy could point to,
 * and thus we should inherit from BaseProxyHandler and not js::Wrapper.
 */
template<typename GUIObjectType>
class JSI_GUIProxy : public js::BaseProxyHandler
{
public:
	// Access the js::Class of the Proxy.
	static JSClass& ClassDefinition();

	// For convenience, this is the single instantiated JSI_GUIProxy.
	static JSI_GUIProxy& Singleton();

	static std::pair<const js::BaseProxyHandler*, void*> CreateData(ScriptInterface& scriptInterface);
protected:
	// @param family can't be nullptr because that's used for some DOM object and it crashes.
	JSI_GUIProxy() : BaseProxyHandler(this, false, false) {};
	// Note: SM provides no virtual destructor for baseProxyHandler.
	// This also enforces making proxy handlers dataless static variables.
	~JSI_GUIProxy() {};

	// This handles returning function properties.
	// Specialize this.
	bool FuncGetter(JS::HandleObject proxy, const std::string& propName, JS::MutableHandleValue vp) const;
protected:
	// BaseProxyHandler interface below

	// Handler for `object.x`
	virtual bool get(JSContext* cx, JS::HandleObject proxy, JS::HandleValue receiver, JS::HandleId id, JS::MutableHandleValue vp) const override final;
	// Handler for `object.x = y;`
	virtual bool set(JSContext* cx, JS::HandleObject proxy, JS::HandleId id, JS::HandleValue vp,
					 JS::HandleValue receiver, JS::ObjectOpResult& result) const final;
	// Handler for `delete object.x;`
	virtual bool delete_(JSContext* cx, JS::HandleObject proxy, JS::HandleId id, JS::ObjectOpResult& result) const override final;

	// The following methods are not provided by BaseProxyHandler.
	// We provide defaults that do nothing (some raise JS exceptions).

	// The JS code will see undefined when querying a property descriptor.
	virtual bool getOwnPropertyDescriptor(JSContext* UNUSED(cx), JS::HandleObject UNUSED(proxy), JS::HandleId UNUSED(id),
										  JS::MutableHandle<JS::PropertyDescriptor> UNUSED(desc)) const override
	{
		return true;
	}
	// Throw an exception is JS code attempts defining a property.
	virtual bool defineProperty(JSContext* UNUSED(cx), JS::HandleObject UNUSED(proxy), JS::HandleId UNUSED(id),
								JS::Handle<JS::PropertyDescriptor> UNUSED(desc), JS::ObjectOpResult& UNUSED(result)) const override
	{
		return false;
	}
	// No accessible properties.
	virtual bool ownPropertyKeys(JSContext* UNUSED(cx), JS::HandleObject UNUSED(proxy), JS::MutableHandleIdVector UNUSED(props)) const override
	{
		return true;
	}
	// Nothing to enumerate.
	virtual bool enumerate(JSContext* UNUSED(cx), JS::HandleObject UNUSED(proxy), JS::MutableHandleIdVector UNUSED(props)) const override
	{
		return true;
	}
	// Throw an exception is JS attempts to query the prototype.
	virtual bool getPrototypeIfOrdinary(JSContext* UNUSED(cx), JS::HandleObject UNUSED(proxy), bool* UNUSED(isOrdinary), JS::MutableHandleObject UNUSED(protop)) const override
	{
		return false;
	}
	// Throw an exception - no prototype to set.
	virtual bool setImmutablePrototype(JSContext* UNUSED(cx), JS::HandleObject UNUSED(proxy), bool* UNUSED(succeeded)) const override
	{
		return false;
	}
	// We are not extensible.
	virtual bool preventExtensions(JSContext* UNUSED(cx), JS::HandleObject UNUSED(proxy), JS::ObjectOpResult& UNUSED(result)) const override
	{
		return true;
	}
	virtual bool isExtensible(JSContext* UNUSED(cx), JS::HandleObject UNUSED(proxy), bool* extensible) const override
	{
		*extensible = false;
		return true;
	}
};

#if GCC_VERSION
# pragma GCC diagnostic pop
#elif MSC_VERSION
# pragma warning(pop)
#endif


#endif // INCLUDED_JSI_GUIPROXY
