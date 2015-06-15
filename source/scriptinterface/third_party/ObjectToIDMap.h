/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sw=4 et tw=80:
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


/**
 * Providing a map-like structure with JSObject pointers (actually their hash) as keys
 * with correct garbage collection handling (JSObjects can move in memory).
 *
 * The code in this class was copied from here and modified to work in our environment.
 *  * https://mxr.mozilla.org/mozilla-esr31/source/js/ipc/JavaScriptShared.h
 *  * https://mxr.mozilla.org/mozilla-esr31/source/js/ipc/JavaScriptShared.cpp
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
	typedef js::PointerHasher<JSObject *, 3> Hasher;
	typedef js::HashMap<JSObject *, T, Hasher, js::SystemAllocPolicy> ObjectIdTable;

	NONCOPYABLE(ObjectIdCache);

public:
	ObjectIdCache(shared_ptr<ScriptRuntime> rt)
		: table_(nullptr), m_rt(rt)
	{
		JS_AddExtraGCRootsTracer(m_rt->m_rt, ObjectIdCache::Trace, this);
	}

	~ObjectIdCache()
	{
		if (table_) {
			m_rt->AddDeferredFinalizationObject(std::shared_ptr<void>((void*)table_, DeleteObjectIDTable));
			table_ = nullptr;
		}

		JS_RemoveExtraGCRootsTracer(m_rt->m_rt, ObjectIdCache::Trace, this);
	}

	bool init()
	{
		MOZ_ASSERT(!table_);
		table_ = new ObjectIdTable(js::SystemAllocPolicy());
		return table_ && table_->init(32);
	}

	void trace(JSTracer *trc)
	{
		for (typename ObjectIdTable::Range r(table_->all()); !r.empty(); r.popFront()) {
			JSObject *obj = r.front().key();
			JS_CallObjectTracer(trc, &obj, "ipc-id");
			MOZ_ASSERT(obj == r.front().key());
		}
	}

	bool add(JSContext *cx, JSObject *obj, T id)
	{
		if (!table_->put(obj, id))
			return false;
		JS_StoreObjectPostBarrierCallback(cx, keyMarkCallback, obj, table_);
		return true;
	}

	bool find(JSObject *obj, T& ret)
	{
		typename ObjectIdTable::Ptr p = table_->lookup(obj);
		if (!p)
			return false;
		ret = p->value();
		return true;
	}

	void remove(JSObject *obj)
	{
		table_->remove(obj);
	}

	bool empty()
	{
		return table_->empty();
	}

	bool has(JSObject* obj)
	{
		return table_->has(obj);
	}

private:
	static void keyMarkCallback(JSTracer *trc, JSObject *key, void *data)
	{
		ObjectIdTable* table = static_cast<ObjectIdTable*>(data);
		JSObject *prior = key;
		JS_CallObjectTracer(trc, &key, "ObjectIdCache::table_ key");
		table->rekeyIfMoved(prior, key);
	}

	static void Trace(JSTracer *trc, void *data)
	{
		reinterpret_cast<ObjectIdCache*>(data)->trace(trc);
	}

	static void DeleteObjectIDTable(void* table)
	{
		delete (ObjectIdTable*)table;
	}

	shared_ptr<ScriptRuntime> m_rt;
	ObjectIdTable *table_;
};

#endif // INCLUDED_OBJECTTOIDMAP
