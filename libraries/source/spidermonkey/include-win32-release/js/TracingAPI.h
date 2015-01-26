/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef js_TracingAPI_h
#define js_TracingAPI_h

#include "mozilla/NullPtr.h"

#include "jspubtd.h"

class JS_PUBLIC_API(JSTracer);

namespace JS {
template <typename T> class Heap;
template <typename T> class TenuredHeap;
}

// Tracer callback, called for each traceable thing directly referenced by a
// particular object or runtime structure. It is the callback responsibility
// to ensure the traversal of the full object graph via calling eventually
// JS_TraceChildren on the passed thing. In this case the callback must be
// prepared to deal with cycles in the traversal graph.
//
// kind argument is one of JSTRACE_OBJECT, JSTRACE_STRING or a tag denoting
// internal implementation-specific traversal kind. In the latter case the only
// operations on thing that the callback can do is to call JS_TraceChildren or
// JS_GetTraceThingInfo.
//
// If eagerlyTraceWeakMaps is true, when we trace a WeakMap visit all
// of its mappings. This should be used in cases where the tracer
// wants to use the existing liveness of entries.
typedef void
(* JSTraceCallback)(JSTracer *trc, void **thingp, JSGCTraceKind kind);

// Callback that JSTraceOp implementation can provide to return a string
// describing the reference traced with JS_CallTracer.
typedef void
(* JSTraceNamePrinter)(JSTracer *trc, char *buf, size_t bufsize);

enum WeakMapTraceKind {
    DoNotTraceWeakMaps = 0,
    TraceWeakMapValues = 1,
    TraceWeakMapKeysValues = 2
};

class JS_PUBLIC_API(JSTracer)
{
  public:
    JSTracer(JSRuntime *rt, JSTraceCallback traceCallback,
             WeakMapTraceKind weakTraceKind = TraceWeakMapValues);

    // Set debugging information about a reference to a traceable thing to prepare
    // for the following call to JS_CallTracer.
    //
    // When printer is null, arg must be const char * or char * C string naming
    // the reference and index must be either (size_t)-1 indicating that the name
    // alone describes the reference or it must be an index into some array vector
    // that stores the reference.
    //
    // When printer callback is not null, the arg and index arguments are
    // available to the callback as debugPrintArg_ and debugPrintIndex_ fields
    // of JSTracer.
    //
    // The storage for name or callback's arguments needs to live only until
    // the following call to JS_CallTracer returns.
    void setTracingDetails(JSTraceNamePrinter printer, const void *arg, size_t index) {
        debugPrinter_ = printer;
        debugPrintArg_ = arg;
        debugPrintIndex_ = index;
    }

    void setTracingIndex(const char *name, size_t index) {
        setTracingDetails(nullptr, (void *)name, index);
    }

    void setTracingName(const char *name) {
        setTracingDetails(nullptr, (void *)name, size_t(-1));
    }

    // Remove the currently set tracing details.
    void clearTracingDetails() {
        debugPrinter_ = nullptr;
        debugPrintArg_ = nullptr;
    }

    // Return true if tracing details are currently set.
    bool hasTracingDetails() const;

    // Get the string set with the most recent call to setTracingName or return
    // fallback if a name printer function has been installed.
    const char *tracingName(const char *fallback) const;

    // Build a description of this edge in the heap graph. This call may invoke
    // the debug printer, which may inspect arbitrary areas of the heap.
    const char *getTracingEdgeName(char *buffer, size_t bufferSize);

    // Access the currently active tracing details.
    JSTraceNamePrinter debugPrinter() const;
    const void *debugPrintArg() const;
    size_t debugPrintIndex() const;

    // Return the runtime set on the tracer.
    JSRuntime *runtime() const { return runtime_; }

    // Return the weak map tracing behavior set on this tracer.
    WeakMapTraceKind eagerlyTraceWeakMaps() const { return eagerlyTraceWeakMaps_; }

    // Update the trace callback.
    void setTraceCallback(JSTraceCallback traceCallback);

#ifdef JS_GC_ZEAL
    // Sets the "real" location for a marked reference, when passing the address
    // directly is not feasable. This address is used for matching against the
    // store buffer when verifying the correctness of the entrees there.
    //
    // This is currently complicated by our need to nest calls for Values
    // stored as keys in hash tables.
    void setTracingLocation(void *location);
    void unsetTracingLocation();
    void **tracingLocation(void **thingp);
#else
    void setTracingLocation(void *location) {}
    void unsetTracingLocation() {}
    void **tracingLocation(void **thingp) { return nullptr; }
#endif

    // We expose |callback| directly so that IS_GC_MARKING_TRACER can compare
    // it to GCMarker::GrayCallback.
    JSTraceCallback     callback;

  private:
    JSRuntime           *runtime_;
    JSTraceNamePrinter  debugPrinter_;
    const void          *debugPrintArg_;
    size_t              debugPrintIndex_;
    WeakMapTraceKind    eagerlyTraceWeakMaps_;
#ifdef JS_GC_ZEAL
    void                *realLocation_;
#endif
};

// The JS_Call*Tracer family of functions traces the given GC thing reference.
// This performs the tracing action configured on the given JSTracer:
// typically calling the JSTracer::callback or marking the thing as live.
//
// The argument to JS_Call*Tracer is an in-out param: when the function
// returns, the garbage collector might have moved the GC thing. In this case,
// the reference passed to JS_Call*Tracer will be updated to the object's new
// location. Callers of this method are responsible for updating any state
// that is dependent on the object's address. For example, if the object's
// address is used as a key in a hashtable, then the object must be removed
// and re-inserted with the correct hash.
//
extern JS_PUBLIC_API(void)
JS_CallValueTracer(JSTracer *trc, JS::Value *valuep, const char *name);

extern JS_PUBLIC_API(void)
JS_CallIdTracer(JSTracer *trc, jsid *idp, const char *name);

extern JS_PUBLIC_API(void)
JS_CallObjectTracer(JSTracer *trc, JSObject **objp, const char *name);

extern JS_PUBLIC_API(void)
JS_CallStringTracer(JSTracer *trc, JSString **strp, const char *name);

extern JS_PUBLIC_API(void)
JS_CallScriptTracer(JSTracer *trc, JSScript **scriptp, const char *name);

extern JS_PUBLIC_API(void)
JS_CallHeapValueTracer(JSTracer *trc, JS::Heap<JS::Value> *valuep, const char *name);

extern JS_PUBLIC_API(void)
JS_CallHeapIdTracer(JSTracer *trc, JS::Heap<jsid> *idp, const char *name);

extern JS_PUBLIC_API(void)
JS_CallHeapObjectTracer(JSTracer *trc, JS::Heap<JSObject *> *objp, const char *name);

extern JS_PUBLIC_API(void)
JS_CallHeapStringTracer(JSTracer *trc, JS::Heap<JSString *> *strp, const char *name);

extern JS_PUBLIC_API(void)
JS_CallHeapScriptTracer(JSTracer *trc, JS::Heap<JSScript *> *scriptp, const char *name);

extern JS_PUBLIC_API(void)
JS_CallHeapFunctionTracer(JSTracer *trc, JS::Heap<JSFunction *> *funp, const char *name);

template <typename HashSetEnum>
inline void
JS_CallHashSetObjectTracer(JSTracer *trc, HashSetEnum &e, JSObject *const &key, const char *name)
{
    JSObject *updated = key;
    trc->setTracingLocation(reinterpret_cast<void *>(&const_cast<JSObject *&>(key)));
    JS_CallObjectTracer(trc, &updated, name);
    if (updated != key)
        e.rekeyFront(key, updated);
}

// Trace an object that is known to always be tenured.  No post barriers are
// required in this case.
extern JS_PUBLIC_API(void)
JS_CallTenuredObjectTracer(JSTracer *trc, JS::TenuredHeap<JSObject *> *objp, const char *name);

extern JS_PUBLIC_API(void)
JS_TraceChildren(JSTracer *trc, void *thing, JSGCTraceKind kind);

extern JS_PUBLIC_API(void)
JS_TraceRuntime(JSTracer *trc);

extern JS_PUBLIC_API(void)
JS_GetTraceThingInfo(char *buf, size_t bufsize, JSTracer *trc,
                     void *thing, JSGCTraceKind kind, bool includeDetails);

#endif /* js_TracingAPI_h */
