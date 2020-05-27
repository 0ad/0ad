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

#include "scriptinterface/ScriptRuntime.h"
#include "scriptinterface/ScriptTypes.h"
#include "scriptinterface/ScriptExtraHeaders.h"
#include <stdint.h>

// Map JSObjects -> ids
template <typename T>
class ObjectIdCache
{
	using Hasher = js::MovableCellHasher<JS::Heap<JSObject*>>;
	using Table = JS::GCHashMap<JS::Heap<JSObject*>, T, Hasher, js::SystemAllocPolicy>;

	NONCOPYABLE(ObjectIdCache);

public:
    ObjectIdCache() : table_(js::SystemAllocPolicy(), 32) {}
	virtual ~ObjectIdCache(){}

	void trace(JSTracer* trc)
	{
        table_.trace();
	}

// TODO sweep?

	bool find(JSObject* obj, T& ret)
	{
		typename Table::Ptr p = table_.lookup(obj);
		if (!p) {
			return false;
        }
		ret = p->value();
		return true;
	}

	bool add(JSContext* cx, JSObject* obj, T id)
	{
        (void) cx;
		return table_.put(obj, id);
	}

	void remove(JSObject* obj)
	{
		table_.remove(obj);
	}

    void clear()
    {
        return table_.clear();
    }

	bool empty()
	{
		return table_.empty();
	}

	bool has(JSObject* obj)
	{
		return table_.has(obj);
	}

private:
	Table table_;
};

#endif // INCLUDED_OBJECTTOIDMAP
