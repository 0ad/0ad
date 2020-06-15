/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


/**
 * Providing a map-like structure with JSObject pointers (actually their hash) as keys
 * with correct garbage collection handling (JSObjects can move in memory).
 *
 * The code in this class was copied from here and modified to work in our environment.
 *  * https://mxr.mozilla.org/mozilla-esr38/source/js/ipc/JavaScriptShared.h
 *  * https://mxr.mozilla.org/mozilla-esr38/source/js/ipc/JavaScriptShared.cpp
 *
 * When updating SpiderMonkey, you most likely have to reintegrate an updated version
 * of the class(es) in this file. The best way is probably to get a diff between the
 * original files and integrate that because this file is heavily modified from the
 * original version.
 */

#ifndef INCLUDED_OBJECTTOIDMAP
#define INCLUDED_OBJECTTOIDMAP

#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/ScriptTypes.h"
#include "scriptinterface/ScriptExtraHeaders.h"
#include <cstdint>

// Map JSObjects -> ids
template <typename T>
class ObjectIdCache
{
	using Hasher = js::MovableCellHasher<JS::Heap<JSObject*>>;
	using Table = JS::GCHashMap<JS::Heap<JSObject*>, 
                                JS::Heap<JS::Value>, 
                                Hasher, 
                                js::SystemAllocPolicy>;

	NONCOPYABLE(ObjectIdCache);

public:
    ObjectIdCache(const ScriptInterface* iface) : table_(js::SystemAllocPolicy(), 32), m_iface(iface) {}
	
    virtual ~ObjectIdCache() {}

	void trace(JSTracer* trc)
	{
        table_.trace(trc);
	}

	bool find(JSObject* obj, T& id)
	{
        CX_IN_REALM(cx,m_iface);
		typename Table::Ptr p = table_.lookup(obj);
		if (!p) {
			return false;
        }
	    
        JS::RootedValue rooted(cx, p->value().get());
        ScriptInterface::FromJSVal(cx, rooted, id);
		
		return true;
	}

	bool add(JSObject* obj, T& id)
	{
        CX_IN_REALM(cx,m_iface);
		JS::RootedValue idval(cx);
        ScriptInterface::ToJSVal(cx, &idval, id);
		return table_.put(obj, idval);
	}

    bool add(JSObject* obj, T&& id)
	{
        CX_IN_REALM(cx,m_iface);
		JS::RootedValue idval(cx);
        ScriptInterface::ToJSVal(cx, &idval, id);
		return table_.put(obj, idval);
	}

	void remove(JSObject* obj)
	{
        CX_IN_REALM(cx,m_iface);
		table_.remove(obj);
	}

    void clear()
    {
        CX_IN_REALM(cx,m_iface);
        return table_.clear();
    }

	bool empty()
	{
        CX_IN_REALM(cx,m_iface);
		return table_.empty();
	}

	bool has(JSObject* obj)
	{
        CX_IN_REALM(cx,m_iface);
		return table_.has(obj);
	}

private:
    const ScriptInterface* m_iface;
	Table table_;
};

#endif // INCLUDED_OBJECTTOIDMAP
