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
#include <stdint.h>

// Map JSObjects -> ids
template <typename T>
class ObjectIdCache
{
	typedef js::PointerHasher<JSObject*, 3> Hasher;
	typedef js::HashMap<JSObject*, T, Hasher, js::SystemAllocPolicy> Table;

	NONCOPYABLE(ObjectIdCache);

public:
	ObjectIdCache(shared_ptr<ScriptRuntime> rt)
		: table_(nullptr), m_rt(rt)
	{
		JS_AddExtraGCRootsTracer(m_rt->m_rt, ObjectIdCache::Trace, this);
	}

	~ObjectIdCache()
	{
		if (table_)
		{
			m_rt->AddDeferredFinalizationObject(std::shared_ptr<void>((void*)table_, DeleteTable));
			table_ = nullptr;
		}

		JS_RemoveExtraGCRootsTracer(m_rt->m_rt, ObjectIdCache::Trace, this);
	}

	bool init()
	{
		if (table_)
			return true;

		table_ = new Table(js::SystemAllocPolicy());
		return table_ && table_->init(32);
	}

	void trace(JSTracer* trc)
	{
		for (typename Table::Enum e(*table_); !e.empty(); e.popFront())
		{
			JSObject* obj = e.front().key();
			JS_CallUnbarrieredObjectTracer(trc, &obj, "ipc-object");
			if (obj != e.front().key())
				e.rekeyFront(obj);
		}
	}

// TODO sweep?

	bool find(JSObject* obj, T& ret)
	{
		typename Table::Ptr p = table_->lookup(obj);
		if (!p)
			return false;
		ret = p->value();
		return true;
	}

	bool add(JSContext* cx, JSObject* obj, T id)
	{
		if (!table_->put(obj, id))
			return false;
		JS_StoreObjectPostBarrierCallback(cx, keyMarkCallback, obj, table_);
		return true;
	}

	void remove(JSObject* obj)
	{
		table_->remove(obj);
	}

// TODO clear?

	bool empty()
	{
		return table_->empty();
	}

	bool has(JSObject* obj)
	{
		return table_->has(obj);
	}

private:
	static void keyMarkCallback(JSTracer* trc, JSObject* key, void* data)
	{
		Table* table = static_cast<Table*>(data);
		JSObject* prior = key;
		JS_CallUnbarrieredObjectTracer(trc, &key, "ObjectIdCache::table_ key");
		table->rekeyIfMoved(prior, key);
	}

	static void Trace(JSTracer* trc, void* data)
	{
		reinterpret_cast<ObjectIdCache*>(data)->trace(trc);
	}

	static void DeleteTable(void* table)
	{
		delete (Table*)table;
	}

	shared_ptr<ScriptRuntime> m_rt;
	Table* table_;
};

#endif // INCLUDED_OBJECTTOIDMAP
