/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sw=4 et tw=78:
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef jsobj_h___
#define jsobj_h___

/* Gross special case for Gecko, which defines malloc/calloc/free. */
#ifdef mozilla_mozalloc_macro_wrappers_h
#  define JS_OBJ_UNDEFD_MOZALLOC_WRAPPERS
/* The "anti-header" */
#  include "mozilla/mozalloc_undef_macro_wrappers.h"
#endif

/*
 * JS object definitions.
 *
 * A JS object consists of a possibly-shared object descriptor containing
 * ordered property names, called the map; and a dense vector of property
 * values, called slots.  The map/slot pointer pair is GC'ed, while the map
 * is reference counted and the slot vector is malloc'ed.
 */
#include "jsapi.h"
#include "jshash.h"
#include "jspubtd.h"
#include "jsprvtd.h"
#include "jslock.h"
#include "jsvalue.h"
#include "jsvector.h"
#include "jscell.h"

namespace js {

class JSProxyHandler;
class AutoPropDescArrayRooter;

namespace mjit {
class Compiler;
}

static inline PropertyOp
CastAsPropertyOp(JSObject *object)
{
    return JS_DATA_TO_FUNC_PTR(PropertyOp, object);
}

static inline StrictPropertyOp
CastAsStrictPropertyOp(JSObject *object)
{
    return JS_DATA_TO_FUNC_PTR(StrictPropertyOp, object);
}

static inline JSPropertyOp
CastAsJSPropertyOp(JSObject *object)
{
    return JS_DATA_TO_FUNC_PTR(JSPropertyOp, object);
}

static inline JSStrictPropertyOp
CastAsJSStrictPropertyOp(JSObject *object)
{
    return JS_DATA_TO_FUNC_PTR(JSStrictPropertyOp, object);
}

inline JSObject *
CastAsObject(PropertyOp op)
{
    return JS_FUNC_TO_DATA_PTR(JSObject *, op);
}

inline JSObject *
CastAsObject(StrictPropertyOp op)
{
    return JS_FUNC_TO_DATA_PTR(JSObject *, op);
}

inline Value
CastAsObjectJsval(PropertyOp op)
{
    return ObjectOrNullValue(CastAsObject(op));
}

inline Value
CastAsObjectJsval(StrictPropertyOp op)
{
    return ObjectOrNullValue(CastAsObject(op));
}

} /* namespace js */

/*
 * A representation of ECMA-262 ed. 5's internal property descriptor data
 * structure.
 */
struct PropDesc {
    friend class js::AutoPropDescArrayRooter;

    PropDesc();

  public:
    /* 8.10.5 ToPropertyDescriptor(Obj) */
    bool initialize(JSContext* cx, jsid id, const js::Value &v);

    /* 8.10.1 IsAccessorDescriptor(desc) */
    bool isAccessorDescriptor() const {
        return hasGet || hasSet;
    }

    /* 8.10.2 IsDataDescriptor(desc) */
    bool isDataDescriptor() const {
        return hasValue || hasWritable;
    }

    /* 8.10.3 IsGenericDescriptor(desc) */
    bool isGenericDescriptor() const {
        return !isAccessorDescriptor() && !isDataDescriptor();
    }

    bool configurable() const {
        return (attrs & JSPROP_PERMANENT) == 0;
    }

    bool enumerable() const {
        return (attrs & JSPROP_ENUMERATE) != 0;
    }

    bool writable() const {
        return (attrs & JSPROP_READONLY) == 0;
    }

    JSObject* getterObject() const {
        return get.isUndefined() ? NULL : &get.toObject();
    }
    JSObject* setterObject() const {
        return set.isUndefined() ? NULL : &set.toObject();
    }

    const js::Value &getterValue() const {
        return get;
    }
    const js::Value &setterValue() const {
        return set;
    }

    js::PropertyOp getter() const {
        return js::CastAsPropertyOp(getterObject());
    }
    js::StrictPropertyOp setter() const {
        return js::CastAsStrictPropertyOp(setterObject());
    }

    js::Value pd;
    jsid id;
    js::Value value, get, set;

    /* Property descriptor boolean fields. */
    uint8 attrs;

    /* Bits indicating which values are set. */
    bool hasGet : 1;
    bool hasSet : 1;
    bool hasValue : 1;
    bool hasWritable : 1;
    bool hasEnumerable : 1;
    bool hasConfigurable : 1;
};

namespace js {

typedef Vector<PropDesc, 1> PropDescArray;

} /* namespace js */

struct JSObjectMap {
    uint32 shape;       /* shape identifier */
    uint32 slotSpan;    /* one more than maximum live slot number */

    static JS_FRIEND_DATA(const JSObjectMap) sharedNonNative;

    explicit JSObjectMap(uint32 shape) : shape(shape), slotSpan(0) {}
    JSObjectMap(uint32 shape, uint32 slotSpan) : shape(shape), slotSpan(slotSpan) {}

    enum { INVALID_SHAPE = 0x8fffffff, SHAPELESS = 0xffffffff };

    bool isNative() const { return this != &sharedNonNative; }

  private:
    /* No copy or assignment semantics. */
    JSObjectMap(JSObjectMap &);
    void operator=(JSObjectMap &);
};

/*
 * Unlike js_DefineNativeProperty, propp must be non-null. On success, and if
 * id was found, return true with *objp non-null and with a property of *objp
 * stored in *propp. If successful but id was not found, return true with both
 * *objp and *propp null.
 */
extern JS_FRIEND_API(JSBool)
js_LookupProperty(JSContext *cx, JSObject *obj, jsid id, JSObject **objp,
                  JSProperty **propp);

extern JSBool
js_DefineProperty(JSContext *cx, JSObject *obj, jsid id, const js::Value *value,
                  js::PropertyOp getter, js::StrictPropertyOp setter, uintN attrs);

extern JSBool
js_GetProperty(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id, js::Value *vp);

inline JSBool
js_GetProperty(JSContext *cx, JSObject *obj, jsid id, js::Value *vp)
{
    return js_GetProperty(cx, obj, obj, id, vp);
}

namespace js {

extern JSBool
GetPropertyDefault(JSContext *cx, JSObject *obj, jsid id, const Value &def, Value *vp);

} /* namespace js */

extern JSBool
js_SetProperty(JSContext *cx, JSObject *obj, jsid id, js::Value *vp, JSBool strict);

extern JSBool
js_GetAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp);

extern JSBool
js_SetAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp);

extern JSBool
js_DeleteProperty(JSContext *cx, JSObject *obj, jsid id, js::Value *rval, JSBool strict);

extern JS_FRIEND_API(JSBool)
js_Enumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
             js::Value *statep, jsid *idp);

extern JSType
js_TypeOf(JSContext *cx, JSObject *obj);

namespace js {

struct NativeIterator;

}

struct JSFunction;

namespace nanojit {
class ValidateWriter;
}

/*
 * JSObject struct, with members sized to fit in 32 bytes on 32-bit targets,
 * 64 bytes on 64-bit systems. The JSFunction struct is an extension of this
 * struct allocated from a larger GC size-class.
 *
 * The clasp member stores the js::Class pointer for this object. We do *not*
 * synchronize updates of clasp or flags -- API clients must take care.
 *
 * An object is a delegate if it is on another object's prototype (the proto
 * field) or scope chain (the parent field), and therefore the delegate might
 * be asked implicitly to get or set a property on behalf of another object.
 * Delegates may be accessed directly too, as may any object, but only those
 * objects linked after the head of any prototype or scope chain are flagged
 * as delegates. This definition helps to optimize shape-based property cache
 * invalidation (see Purge{Scope,Proto}Chain in jsobj.cpp).
 *
 * The meaning of the system object bit is defined by the API client. It is
 * set in JS_NewSystemObject and is queried by JS_IsSystemObject (jsdbgapi.h),
 * but it has no intrinsic meaning to SpiderMonkey. Further, JSFILENAME_SYSTEM
 * and JS_FlagScriptFilenamePrefix (also exported via jsdbgapi.h) are intended
 * to be complementary to this bit, but it is up to the API client to implement
 * any such association.
 *
 * Both these flag bits are initially zero; they may be set or queried using
 * the (is|set)(Delegate|System) inline methods.
 *
 * The slots member is a pointer to the slot vector for the object.
 * This can be either a fixed array allocated immediately after the object,
 * or a dynamically allocated array.  A dynamic array can be tested for with
 * hasSlotsArray().  In all cases, capacity gives the number of usable slots.
 * Two objects with the same shape have the same number of fixed slots,
 * and either both have or neither have dynamically allocated slot arrays.
 *
 * If you change this struct, you'll probably need to change the AccSet values
 * in jsbuiltins.h.
 */
struct JSObject : js::gc::Cell {
    /*
     * TraceRecorder must be a friend because it generates code that
     * manipulates JSObjects, which requires peeking under any encapsulation.
     * ValidateWriter must be a friend because it works in tandem with
     * TraceRecorder.
     */
    friend class js::TraceRecorder;
    friend class nanojit::ValidateWriter;
    friend class GetPropCompiler;

    /*
     * Private pointer to the last added property and methods to manipulate the
     * list it links among properties in this scope. The {remove,insert} pair
     * for DictionaryProperties assert that the scope is in dictionary mode and
     * any reachable properties are flagged as dictionary properties.
     *
     * For native objects, this field is always a Shape. For non-native objects,
     * it points to the singleton sharedNonNative JSObjectMap, whose shape field
     * is SHAPELESS.
     *
     * NB: these private methods do *not* update this scope's shape to track
     * lastProp->shape after they finish updating the linked list in the case
     * where lastProp is updated. It is up to calling code in jsscope.cpp to
     * call updateShape(cx) after updating lastProp.
     */
    union {
        js::Shape       *lastProp;
        JSObjectMap     *map;
    };

    js::Class           *clasp;

  private:
    inline void setLastProperty(const js::Shape *shape);
    inline void removeLastProperty();

#ifdef DEBUG
    void checkShapeConsistency();
#endif

  public:
    inline const js::Shape *lastProperty() const;

    inline js::Shape **nativeSearch(jsid id, bool adding = false);
    inline const js::Shape *nativeLookup(jsid id);

    inline bool nativeContains(jsid id);
    inline bool nativeContains(const js::Shape &shape);

    enum {
        DELEGATE                  =  0x01,
        SYSTEM                    =  0x02,
        NOT_EXTENSIBLE            =  0x04,
        BRANDED                   =  0x08,
        GENERIC                   =  0x10,
        METHOD_BARRIER            =  0x20,
        INDEXED                   =  0x40,
        OWN_SHAPE                 =  0x80,
        BOUND_FUNCTION            = 0x100,
        HAS_EQUALITY              = 0x200,
        METHOD_THRASH_COUNT_MASK  = 0xc00,
        METHOD_THRASH_COUNT_SHIFT =    10,
        METHOD_THRASH_COUNT_MAX   = METHOD_THRASH_COUNT_MASK >> METHOD_THRASH_COUNT_SHIFT
    };

    /*
     * Impose a sane upper bound, originally checked only for dense arrays, on
     * number of slots in an object.
     */
    enum {
        NSLOTS_BITS     = 29,
        NSLOTS_LIMIT    = JS_BIT(NSLOTS_BITS)
    };

    uint32      flags;                      /* flags */
    uint32      objShape;                   /* copy of lastProp->shape, or override if different */

    /* If prototype, lazily filled array of empty shapes for each object size. */
    js::EmptyShape **emptyShapes;

    JSObject    *proto;                     /* object's prototype */
    JSObject    *parent;                    /* object's parent */
    void        *privateData;               /* private data */
    jsuword     capacity;                   /* capacity of slots */
    js::Value   *slots;                     /* dynamically allocated slots,
                                               or pointer to fixedSlots() */

    /*
     * Return an immutable, shareable, empty shape with the same clasp as this
     * and the same slotSpan as this had when empty.
     *
     * If |this| is the scope of an object |proto|, the resulting scope can be
     * used as the scope of a new object whose prototype is |proto|.
     */
    inline bool canProvideEmptyShape(js::Class *clasp);
    inline js::EmptyShape *getEmptyShape(JSContext *cx, js::Class *aclasp,
                                         /* gc::FinalizeKind */ unsigned kind);

    bool isNative() const       { return map->isNative(); }

    js::Class *getClass() const { return clasp; }
    JSClass *getJSClass() const { return Jsvalify(clasp); }

    bool hasClass(const js::Class *c) const {
        return c == clasp;
    }

    const js::ObjectOps *getOps() const {
        return &getClass()->ops;
    }

    inline void trace(JSTracer *trc);

    uint32 shape() const {
        JS_ASSERT(objShape != JSObjectMap::INVALID_SHAPE);
        return objShape;
    }

    bool isDelegate() const     { return !!(flags & DELEGATE); }
    void setDelegate()          { flags |= DELEGATE; }
    void clearDelegate()        { flags &= ~DELEGATE; }

    bool isBoundFunction() const { return !!(flags & BOUND_FUNCTION); }

    static void setDelegateNullSafe(JSObject *obj) {
        if (obj)
            obj->setDelegate();
    }

    bool isSystem() const       { return !!(flags & SYSTEM); }
    void setSystem()            { flags |= SYSTEM; }

    /*
     * A branded object contains plain old methods (function-valued properties
     * without magic getters and setters), and its shape evolves whenever a
     * function value changes.
     */
    bool branded()              { return !!(flags & BRANDED); }

    /*
     * NB: these return false on shape overflow but do not report any error.
     * Callers who depend on shape guarantees should therefore bail off trace,
     * e.g., on false returns.
     */
    bool brand(JSContext *cx);
    bool unbrand(JSContext *cx);

    bool generic()              { return !!(flags & GENERIC); }
    void setGeneric()           { flags |= GENERIC; }

    uintN getMethodThrashCount() const {
        return (flags & METHOD_THRASH_COUNT_MASK) >> METHOD_THRASH_COUNT_SHIFT;
    }

    void setMethodThrashCount(uintN count) {
        JS_ASSERT(count <= METHOD_THRASH_COUNT_MAX);
        flags = (flags & ~METHOD_THRASH_COUNT_MASK) | (count << METHOD_THRASH_COUNT_SHIFT);
    }

    bool hasSpecialEquality() const { return !!(flags & HAS_EQUALITY); }
    void assertSpecialEqualitySynced() const {
        JS_ASSERT(!!clasp->ext.equality == hasSpecialEquality());
    }

    /* Sets an object's HAS_EQUALITY flag based on its clasp. */
    inline void syncSpecialEquality();

  private:
    void generateOwnShape(JSContext *cx);

    void setOwnShape(uint32 s)  { flags |= OWN_SHAPE; objShape = s; }
    void clearOwnShape()        { flags &= ~OWN_SHAPE; objShape = map->shape; }

  public:
    inline bool nativeEmpty() const;

    bool hasOwnShape() const    { return !!(flags & OWN_SHAPE); }

    void setMap(const JSObjectMap *amap) {
        JS_ASSERT(!hasOwnShape());
        map = const_cast<JSObjectMap *>(amap);
        objShape = map->shape;
    }

    void setSharedNonNativeMap() {
        setMap(&JSObjectMap::sharedNonNative);
    }

    void deletingShapeChange(JSContext *cx, const js::Shape &shape);
    const js::Shape *methodShapeChange(JSContext *cx, const js::Shape &shape);
    bool methodShapeChange(JSContext *cx, uint32 slot);
    void protoShapeChange(JSContext *cx);
    void shadowingShapeChange(JSContext *cx, const js::Shape &shape);
    bool globalObjectOwnShapeChange(JSContext *cx);
    void watchpointOwnShapeChange(JSContext *cx) { generateOwnShape(cx); }

    void extensibleShapeChange(JSContext *cx) {
        /* This will do for now. */
        generateOwnShape(cx);
    }

    /*
     * A scope has a method barrier when some compiler-created "null closure"
     * function objects (functions that do not use lexical bindings above their
     * scope, only free variable names) that have a correct JSSLOT_PARENT value
     * thanks to the COMPILE_N_GO optimization are stored as newly added direct
     * property values of the scope's object.
     *
     * The de-facto standard JS language requires each evaluation of such a
     * closure to result in a unique (according to === and observable effects)
     * function object. ES3 tried to allow implementations to "join" such
     * objects to a single compiler-created object, but this makes an overt
     * mutation hazard, also an "identity hazard" against interoperation among
     * implementations that join and do not join.
     *
     * To stay compatible with the de-facto standard, we store the compiler-
     * created function object as the method value and set the METHOD_BARRIER
     * flag.
     *
     * The method value is part of the method property tree node's identity, so
     * it effectively  brands the scope with a predictable shape corresponding
     * to the method value, but without the overhead of setting the BRANDED
     * flag, which requires assigning a new shape peculiar to each branded
     * scope. Instead the shape is shared via the property tree among all the
     * scopes referencing the method property tree node.
     *
     * Then when reading from a scope for which scope->hasMethodBarrier() is
     * true, we count on the scope's qualified/guarded shape being unique and
     * add a read barrier that clones the compiler-created function object on
     * demand, reshaping the scope.
     *
     * This read barrier is bypassed when evaluating the callee sub-expression
     * of a call expression (see the JOF_CALLOP opcodes in jsopcode.tbl), since
     * such ops do not present an identity or mutation hazard. The compiler
     * performs this optimization only for null closures that do not use their
     * own name or equivalent built-in references (arguments.callee).
     *
     * The BRANDED write barrier, JSObject::methodWriteBarrer, must check for
     * METHOD_BARRIER too, and regenerate this scope's shape if the method's
     * value is in fact changing.
     */
    bool hasMethodBarrier()     { return !!(flags & METHOD_BARRIER); }
    void setMethodBarrier()     { flags |= METHOD_BARRIER; }

    /*
     * Test whether this object may be branded due to method calls, which means
     * any assignment to a function-valued property must regenerate shape; else
     * test whether this object has method properties, which require a method
     * write barrier.
     */
    bool brandedOrHasMethodBarrier() { return !!(flags & (BRANDED | METHOD_BARRIER)); }

    /*
     * Read barrier to clone a joined function object stored as a method.
     * Defined in jsobjinlines.h, but not declared inline per standard style in
     * order to avoid gcc warnings.
     */
    const js::Shape *methodReadBarrier(JSContext *cx, const js::Shape &shape, js::Value *vp);

    /*
     * Write barrier to check for a change of method value. Defined inline in
     * jsobjinlines.h after methodReadBarrier. The slot flavor is required by
     * JSOP_*GVAR, which deals in slots not shapes, while not deoptimizing to
     * map slot to shape unless JSObject::flags show that this is necessary.
     * The methodShapeChange overload (above) parallels this.
     */
    const js::Shape *methodWriteBarrier(JSContext *cx, const js::Shape &shape, const js::Value &v);
    bool methodWriteBarrier(JSContext *cx, uint32 slot, const js::Value &v);

    bool isIndexed() const          { return !!(flags & INDEXED); }
    void setIndexed()               { flags |= INDEXED; }

    /*
     * Return true if this object is a native one that has been converted from
     * shared-immutable prototype-rooted shape storage to dictionary-shapes in
     * a doubly-linked list.
     */
    inline bool inDictionaryMode() const;

    inline uint32 propertyCount() const;

    inline bool hasPropertyTable() const;

    /* gc::FinalizeKind */ unsigned finalizeKind() const;

    uint32 numSlots() const { return capacity; }

    size_t slotsAndStructSize(uint32 nslots) const;
    size_t slotsAndStructSize() const { return slotsAndStructSize(numSlots()); }

    inline js::Value* fixedSlots() const;
    inline size_t numFixedSlots() const;

    static inline size_t getFixedSlotOffset(size_t slot);

  public:
    /* Minimum size for dynamically allocated slots. */
    static const uint32 SLOT_CAPACITY_MIN = 8;

    bool allocSlots(JSContext *cx, size_t nslots);
    bool growSlots(JSContext *cx, size_t nslots);
    void shrinkSlots(JSContext *cx, size_t nslots);

    bool ensureSlots(JSContext *cx, size_t nslots) {
        if (numSlots() < nslots)
            return growSlots(cx, nslots);
        return true;
    }

    /*
     * Ensure that the object has at least JSCLASS_RESERVED_SLOTS(clasp) +
     * nreserved slots.
     *
     * This method may be called only for native objects freshly created using
     * NewObject or one of its variant where the new object will both (a) never
     * escape to script and (b) never be extended with ad-hoc properties that
     * would try to allocate higher slots without the fresh object first having
     * its map set to a shape path that maps those slots.
     *
     * Block objects satisfy (a) and (b), as there is no evil eval-based way to
     * add ad-hoc properties to a Block instance. Call objects satisfy (a) and
     * (b) as well, because the compiler-created Shape path that covers args,
     * vars, and upvars, stored in their callee function in u.i.names, becomes
     * their initial map.
     */
    bool ensureInstanceReservedSlots(JSContext *cx, size_t nreserved);

    /*
     * Get a direct pointer to the object's slots.
     * This can be reallocated if the object is modified, watch out!
     */
    js::Value *getSlots() const {
        return slots;
    }

    /*
     * NB: ensureClassReservedSlotsForEmptyObject asserts that nativeEmpty()
     * Use ensureClassReservedSlots for any object, either empty or already
     * extended with properties.
     */
    bool ensureClassReservedSlotsForEmptyObject(JSContext *cx);

    inline bool ensureClassReservedSlots(JSContext *cx);

    uint32 slotSpan() const { return map->slotSpan; }

    bool containsSlot(uint32 slot) const { return slot < slotSpan(); }

    js::Value& getSlotRef(uintN slot) {
        JS_ASSERT(slot < capacity);
        return slots[slot];
    }

    js::Value &nativeGetSlotRef(uintN slot) {
        JS_ASSERT(isNative());
        JS_ASSERT(containsSlot(slot));
        return getSlotRef(slot);
    }

    const js::Value &getSlot(uintN slot) const {
        JS_ASSERT(slot < capacity);
        return slots[slot];
    }

    const js::Value &nativeGetSlot(uintN slot) const {
        JS_ASSERT(isNative());
        JS_ASSERT(containsSlot(slot));
        return getSlot(slot);
    }

    void setSlot(uintN slot, const js::Value &value) {
        JS_ASSERT(slot < capacity);
        slots[slot] = value;
    }

    void nativeSetSlot(uintN slot, const js::Value &value) {
        JS_ASSERT(isNative());
        JS_ASSERT(containsSlot(slot));
        return setSlot(slot, value);
    }

    inline js::Value getReservedSlot(uintN index) const;

    /* Defined in jsscopeinlines.h to avoid including implementation dependencies here. */
    inline void updateShape(JSContext *cx);
    inline void updateFlags(const js::Shape *shape, bool isDefinitelyAtom = false);

    /* Extend this object to have shape as its last-added property. */
    inline void extend(JSContext *cx, const js::Shape *shape, bool isDefinitelyAtom = false);

    JSObject *getProto() const  { return proto; }
    void clearProto()           { proto = NULL; }

    void setProto(JSObject *newProto) {
#ifdef DEBUG
        for (JSObject *obj = newProto; obj; obj = obj->getProto())
            JS_ASSERT(obj != this);
#endif
        setDelegateNullSafe(newProto);
        proto = newProto;
    }

    JSObject *getParent() const {
        return parent;
    }

    void clearParent() {
        parent = NULL;
    }

    void setParent(JSObject *newParent) {
#ifdef DEBUG
        for (JSObject *obj = newParent; obj; obj = obj->getParent())
            JS_ASSERT(obj != this);
#endif
        setDelegateNullSafe(newParent);
        parent = newParent;
    }

    JS_FRIEND_API(JSObject *) getGlobal() const;

    bool isGlobal() const {
        return !!(getClass()->flags & JSCLASS_IS_GLOBAL);
    }

    void *getPrivate() const {
        JS_ASSERT(getClass()->flags & JSCLASS_HAS_PRIVATE);
        return privateData;
    }

    void setPrivate(void *data) {
        JS_ASSERT(getClass()->flags & JSCLASS_HAS_PRIVATE);
        privateData = data;
    }


    /*
     * ES5 meta-object properties and operations.
     */

  private:
    enum ImmutabilityType { SEAL, FREEZE };

    /*
     * The guts of Object.seal (ES5 15.2.3.8) and Object.freeze (ES5 15.2.3.9): mark the
     * object as non-extensible, and adjust each property's attributes appropriately: each
     * property becomes non-configurable, and if |freeze|, data properties become
     * read-only as well.
     */
    bool sealOrFreeze(JSContext *cx, ImmutabilityType it);

  public:
    bool isExtensible() const { return !(flags & NOT_EXTENSIBLE); }
    bool preventExtensions(JSContext *cx, js::AutoIdVector *props);

    /* ES5 15.2.3.8: non-extensible, all props non-configurable */
    inline bool seal(JSContext *cx) { return sealOrFreeze(cx, SEAL); }
    /* ES5 15.2.3.9: non-extensible, all properties non-configurable, all data props read-only */
    bool freeze(JSContext *cx) { return sealOrFreeze(cx, FREEZE); }
        
    /*
     * Primitive-specific getters and setters.
     */

  private:
    static const uint32 JSSLOT_PRIMITIVE_THIS = 0;

  public:
    inline const js::Value &getPrimitiveThis() const;
    inline void setPrimitiveThis(const js::Value &pthis);

    /*
     * Array-specific getters and setters (for both dense and slow arrays).
     */

    inline uint32 getArrayLength() const;
    inline void setArrayLength(uint32 length);

    inline uint32 getDenseArrayCapacity();
    inline js::Value* getDenseArrayElements();
    inline const js::Value &getDenseArrayElement(uintN idx);
    inline js::Value* addressOfDenseArrayElement(uintN idx);
    inline void setDenseArrayElement(uintN idx, const js::Value &val);
    inline void shrinkDenseArrayElements(JSContext *cx, uintN cap);

    /*
     * ensureDenseArrayElements ensures that the dense array can hold at least
     * index + extra elements. It returns ED_OK on success, ED_FAILED on
     * failure to grow the array, ED_SPARSE when the array is too sparse to
     * grow (this includes the case of index + extra overflow). In the last
     * two cases the array is kept intact.
     */
    enum EnsureDenseResult { ED_OK, ED_FAILED, ED_SPARSE };
    inline EnsureDenseResult ensureDenseArrayElements(JSContext *cx, uintN index, uintN extra);

    /*
     * Check if after growing the dense array will be too sparse.
     * newElementsHint is an estimated number of elements to be added.
     */
    bool willBeSparseDenseArray(uintN requiredCapacity, uintN newElementsHint);

    JSBool makeDenseArraySlow(JSContext *cx);

    /*
     * Arguments-specific getters and setters.
     */

  private:
    /*
     * We represent arguments objects using js_ArgumentsClass and
     * js::StrictArgumentsClass. The two are structured similarly, and methods
     * valid on arguments objects of one class are also generally valid on
     * arguments objects of the other.
     *
     * Arguments objects of either class store arguments length in a slot:
     *
     * JSSLOT_ARGS_LENGTH   - the number of actual arguments and a flag
     *                        indicating whether arguments.length was
     *                        overwritten. This slot is not used to represent
     *                        arguments.length after that property has been
     *                        assigned, even if the new value is integral: it's
     *                        always the original length.
     *
     * Both arguments classes use a slot for storing arguments data:
     *
     * JSSLOT_ARGS_DATA     - pointer to an ArgumentsData structure
     *
     * ArgumentsData for normal arguments stores the value of arguments.callee,
     * as long as that property has not been overwritten. If arguments.callee
     * is overwritten, the corresponding value in ArgumentsData is set to
     * MagicValue(JS_ARGS_HOLE). Strict arguments do not store this value
     * because arguments.callee is a poison pill for strict mode arguments.
     *
     * The ArgumentsData structure also stores argument values. For normal
     * arguments this occurs after the corresponding function has returned, and
     * for strict arguments this occurs when the arguments object is created,
     * or sometimes shortly after (but not observably so). arguments[i] is
     * stored in ArgumentsData.slots[i], accessible via getArgsElement() and
     * setArgsElement(). Deletion of arguments[i] overwrites that slot with
     * MagicValue(JS_ARGS_HOLE); subsequent redefinition of arguments[i] will
     * use a normal property to store the value, ignoring the slot.
     *
     * Non-strict arguments have a private:
     *
     * private              - the function's stack frame until the function
     *                        returns, when it is replaced with null; also,
     *                        JS_ARGUMENTS_OBJECT_ON_TRACE while on trace, if
     *                        arguments was created on trace
     *
     * Technically strict arguments have a private, but it's always null.
     * Conceptually it would be better to remove this oddity, but preserving it
     * allows us to work with arguments objects of either kind more abstractly,
     * so we keep it for now.
     */
    static const uint32 JSSLOT_ARGS_DATA = 1;

  public:
    /* Number of extra fixed arguments object slots besides JSSLOT_PRIVATE. */
    static const uint32 JSSLOT_ARGS_LENGTH = 0;
    static const uint32 ARGS_CLASS_RESERVED_SLOTS = 2;
    static const uint32 ARGS_FIRST_FREE_SLOT = ARGS_CLASS_RESERVED_SLOTS + 1;

    /* Lower-order bit stolen from the length slot. */
    static const uint32 ARGS_LENGTH_OVERRIDDEN_BIT = 0x1;
    static const uint32 ARGS_PACKED_BITS_COUNT = 1;

    /*
     * Set the initial length of the arguments, and mark it as not overridden.
     */
    inline void setArgsLength(uint32 argc);

    /*
     * Return the initial length of the arguments.  This may differ from the
     * current value of arguments.length!
     */
    inline uint32 getArgsInitialLength() const;

    inline void setArgsLengthOverridden();
    inline bool isArgsLengthOverridden() const;

    inline js::ArgumentsData *getArgsData() const;
    inline void setArgsData(js::ArgumentsData *data);

    inline const js::Value &getArgsCallee() const;
    inline void setArgsCallee(const js::Value &callee);

    inline const js::Value &getArgsElement(uint32 i) const;
    inline js::Value *getArgsElements() const;
    inline js::Value *addressOfArgsElement(uint32 i);
    inline void setArgsElement(uint32 i, const js::Value &v);

  private:
    /*
     * Reserved slot structure for Call objects:
     *
     * private               - the stack frame corresponding to the Call object
     *                         until js_PutCallObject or its on-trace analog
     *                         is called, null thereafter
     * JSSLOT_CALL_CALLEE    - callee function for the stack frame, or null if
     *                         the stack frame is for strict mode eval code
     * JSSLOT_CALL_ARGUMENTS - arguments object for non-strict mode eval stack
     *                         frames (not valid for strict mode eval frames)
     */
    static const uint32 JSSLOT_CALL_CALLEE = 0;
    static const uint32 JSSLOT_CALL_ARGUMENTS = 1;

  public:
    /* Number of reserved slots. */
    static const uint32 CALL_RESERVED_SLOTS = 2;

    /* True if this is for a strict mode eval frame or for a function call. */
    inline bool callIsForEval() const;

    /* The stack frame for this Call object, if the frame is still active. */
    inline JSStackFrame *maybeCallObjStackFrame() const;

    /*
     * The callee function if this Call object was created for a function
     * invocation, or null if it was created for a strict mode eval frame.
     */
    inline JSObject *getCallObjCallee() const;
    inline JSFunction *getCallObjCalleeFunction() const; 
    inline void setCallObjCallee(JSObject *callee);

    inline const js::Value &getCallObjArguments() const;
    inline void setCallObjArguments(const js::Value &v);

    /* Returns the formal argument at the given index. */
    inline const js::Value &callObjArg(uintN i) const;
    inline js::Value &callObjArg(uintN i);

    /* Returns the variable at the given index. */
    inline const js::Value &callObjVar(uintN i) const;
    inline js::Value &callObjVar(uintN i);

    /*
     * Date-specific getters and setters.
     */

    static const uint32 JSSLOT_DATE_UTC_TIME = 0;

    /*
     * Cached slots holding local properties of the date.
     * These are undefined until the first actual lookup occurs
     * and are reset to undefined whenever the date's time is modified.
     */
    static const uint32 JSSLOT_DATE_COMPONENTS_START = 1;

    static const uint32 JSSLOT_DATE_LOCAL_TIME = 1;
    static const uint32 JSSLOT_DATE_LOCAL_YEAR = 2;
    static const uint32 JSSLOT_DATE_LOCAL_MONTH = 3;
    static const uint32 JSSLOT_DATE_LOCAL_DATE = 4;
    static const uint32 JSSLOT_DATE_LOCAL_DAY = 5;
    static const uint32 JSSLOT_DATE_LOCAL_HOURS = 6;
    static const uint32 JSSLOT_DATE_LOCAL_MINUTES = 7;
    static const uint32 JSSLOT_DATE_LOCAL_SECONDS = 8;

    static const uint32 DATE_CLASS_RESERVED_SLOTS = 9;

    inline const js::Value &getDateUTCTime() const;
    inline void setDateUTCTime(const js::Value &pthis);

    /*
     * Function-specific getters and setters.
     */

  private:
    friend struct JSFunction;
    friend class js::mjit::Compiler;

    /*
     * Flat closures with one or more upvars snapshot the upvars' values into a
     * vector of js::Values referenced from this slot.
     */
    static const uint32 JSSLOT_FLAT_CLOSURE_UPVARS = 0;

    /*
     * Null closures set or initialized as methods have these slots. See the
     * "method barrier" comments and methods.
     */

    static const uint32 JSSLOT_FUN_METHOD_ATOM = 0;
    static const uint32 JSSLOT_FUN_METHOD_OBJ  = 1;

    static const uint32 JSSLOT_BOUND_FUNCTION_THIS       = 0;
    static const uint32 JSSLOT_BOUND_FUNCTION_ARGS_COUNT = 1;

  public:
    static const uint32 FUN_CLASS_RESERVED_SLOTS = 2;

    inline JSFunction *getFunctionPrivate() const;

    inline js::Value *getFlatClosureUpvars() const;
    inline js::Value getFlatClosureUpvar(uint32 i) const;
    inline js::Value &getFlatClosureUpvar(uint32 i);
    inline void setFlatClosureUpvars(js::Value *upvars);

    inline bool hasMethodObj(const JSObject& obj) const;
    inline void setMethodObj(JSObject& obj);

    inline bool initBoundFunction(JSContext *cx, const js::Value &thisArg,
                                  const js::Value *args, uintN argslen);

    inline JSObject *getBoundFunctionTarget() const;
    inline const js::Value &getBoundFunctionThis() const;
    inline const js::Value *getBoundFunctionArguments(uintN &argslen) const;

    /*
     * RegExp-specific getters and setters.
     */

  private:
    static const uint32 JSSLOT_REGEXP_LAST_INDEX = 0;

  public:
    static const uint32 REGEXP_CLASS_RESERVED_SLOTS = 1;

    inline const js::Value &getRegExpLastIndex() const;
    inline void setRegExpLastIndex(const js::Value &v);
    inline void setRegExpLastIndex(jsdouble d);
    inline void zeroRegExpLastIndex();

    /*
     * Iterator-specific getters and setters.
     */

    inline js::NativeIterator *getNativeIterator() const;
    inline void setNativeIterator(js::NativeIterator *);

    /*
     * Script-related getters.
     */

    inline JSScript *getScript() const;

    /*
     * XML-related getters and setters.
     */

    /*
     * Slots for XML-related classes are as follows:
     * - js_NamespaceClass.base reserves the *_NAME_* and *_NAMESPACE_* slots.
     * - js_QNameClass.base, js_AttributeNameClass, js_AnyNameClass reserve
     *   the *_NAME_* and *_QNAME_* slots.
     * - Others (js_XMLClass, js_XMLFilterClass) don't reserve any slots.
     */
  private:
    static const uint32 JSSLOT_NAME_PREFIX          = 0;   // shared
    static const uint32 JSSLOT_NAME_URI             = 1;   // shared

    static const uint32 JSSLOT_NAMESPACE_DECLARED   = 2;

    static const uint32 JSSLOT_QNAME_LOCAL_NAME     = 2;

  public:
    static const uint32 NAMESPACE_CLASS_RESERVED_SLOTS = 3;
    static const uint32 QNAME_CLASS_RESERVED_SLOTS     = 3;

    inline JSLinearString *getNamePrefix() const;
    inline jsval getNamePrefixVal() const;
    inline void setNamePrefix(JSLinearString *prefix);
    inline void clearNamePrefix();

    inline JSLinearString *getNameURI() const;
    inline jsval getNameURIVal() const;
    inline void setNameURI(JSLinearString *uri);

    inline jsval getNamespaceDeclared() const;
    inline void setNamespaceDeclared(jsval decl);

    inline JSLinearString *getQNameLocalName() const;
    inline jsval getQNameLocalNameVal() const;
    inline void setQNameLocalName(JSLinearString *name);

    /*
     * Proxy-specific getters and setters.
     */

    inline js::JSProxyHandler *getProxyHandler() const;
    inline const js::Value &getProxyPrivate() const;
    inline void setProxyPrivate(const js::Value &priv);
    inline const js::Value &getProxyExtra() const;
    inline void setProxyExtra(const js::Value &extra);

    /*
     * With object-specific getters and setters.
     */
    inline JSObject *getWithThis() const;
    inline void setWithThis(JSObject *thisp);

    /*
     * Back to generic stuff.
     */
    inline bool isCallable();

    /* The map field is not initialized here and should be set separately. */
    void init(JSContext *cx, js::Class *aclasp, JSObject *proto, JSObject *parent,
              void *priv, bool useHoles);

    inline void finish(JSContext *cx);
    JS_ALWAYS_INLINE void finalize(JSContext *cx);

    /*
     * Like init, but also initializes map. The catch: proto must be the result
     * of a call to js_InitClass(...clasp, ...).
     */
    inline bool initSharingEmptyShape(JSContext *cx,
                                      js::Class *clasp,
                                      JSObject *proto,
                                      JSObject *parent,
                                      void *priv,
                                      /* gc::FinalizeKind */ unsigned kind);

    inline bool hasSlotsArray() const;

    /* This method can only be called when hasSlotsArray() returns true. */
    inline void freeSlotsArray(JSContext *cx);

    /* Free the slots array and copy slots that fit into the fixed array. */
    inline void revertToFixedSlots(JSContext *cx);

    inline bool hasProperty(JSContext *cx, jsid id, bool *foundp, uintN flags = 0);

    /*
     * Allocate and free an object slot. Note that freeSlot is infallible: it
     * returns true iff this is a dictionary-mode object and the freed slot was
     * added to the freelist.
     *
     * FIXME: bug 593129 -- slot allocation should be done by object methods
     * after calling object-parameter-free shape methods, avoiding coupling
     * logic across the object vs. shape module wall.
     */
    bool allocSlot(JSContext *cx, uint32 *slotp);
    bool freeSlot(JSContext *cx, uint32 slot);

  public:
    bool reportReadOnly(JSContext* cx, jsid id, uintN report = JSREPORT_ERROR);
    bool reportNotConfigurable(JSContext* cx, jsid id, uintN report = JSREPORT_ERROR);
    bool reportNotExtensible(JSContext *cx, uintN report = JSREPORT_ERROR);

  private:
    js::Shape *getChildProperty(JSContext *cx, js::Shape *parent, js::Shape &child);

    /*
     * Internal helper that adds a shape not yet mapped by this object.
     *
     * Notes:
     * 1. getter and setter must be normalized based on flags (see jsscope.cpp).
     * 2. !isExtensible() checking must be done by callers.
     */
    const js::Shape *addPropertyInternal(JSContext *cx, jsid id,
                                         js::PropertyOp getter, js::StrictPropertyOp setter,
                                         uint32 slot, uintN attrs,
                                         uintN flags, intN shortid,
                                         js::Shape **spp);

    bool toDictionaryMode(JSContext *cx);

  public:
    /* Add a property whose id is not yet in this scope. */
    const js::Shape *addProperty(JSContext *cx, jsid id,
                                 js::PropertyOp getter, js::StrictPropertyOp setter,
                                 uint32 slot, uintN attrs,
                                 uintN flags, intN shortid);

    /* Add a data property whose id is not yet in this scope. */
    const js::Shape *addDataProperty(JSContext *cx, jsid id, uint32 slot, uintN attrs) {
        JS_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
        return addProperty(cx, id, NULL, NULL, slot, attrs, 0, 0);
    }

    /* Add or overwrite a property for id in this scope. */
    const js::Shape *putProperty(JSContext *cx, jsid id,
                                 js::PropertyOp getter, js::StrictPropertyOp setter,
                                 uint32 slot, uintN attrs,
                                 uintN flags, intN shortid);

    /* Change the given property into a sibling with the same id in this scope. */
    const js::Shape *changeProperty(JSContext *cx, const js::Shape *shape, uintN attrs, uintN mask,
                                    js::PropertyOp getter, js::StrictPropertyOp setter);

    /* Remove the property named by id from this object. */
    bool removeProperty(JSContext *cx, jsid id);

    /* Clear the scope, making it empty. */
    void clear(JSContext *cx);

    JSBool lookupProperty(JSContext *cx, jsid id, JSObject **objp, JSProperty **propp) {
        js::LookupPropOp op = getOps()->lookupProperty;
        return (op ? op : js_LookupProperty)(cx, this, id, objp, propp);
    }

    JSBool defineProperty(JSContext *cx, jsid id, const js::Value &value,
                          js::PropertyOp getter = js::PropertyStub,
                          js::StrictPropertyOp setter = js::StrictPropertyStub,
                          uintN attrs = JSPROP_ENUMERATE) {
        js::DefinePropOp op = getOps()->defineProperty;
        return (op ? op : js_DefineProperty)(cx, this, id, &value, getter, setter, attrs);
    }

    JSBool getProperty(JSContext *cx, JSObject *receiver, jsid id, js::Value *vp) {
        js::PropertyIdOp op = getOps()->getProperty;
        return (op ? op : (js::PropertyIdOp)js_GetProperty)(cx, this, receiver, id, vp);
    }

    JSBool getProperty(JSContext *cx, jsid id, js::Value *vp) {
        return getProperty(cx, this, id, vp);
    }

    JSBool setProperty(JSContext *cx, jsid id, js::Value *vp, JSBool strict) {
        js::StrictPropertyIdOp op = getOps()->setProperty;
        return (op ? op : js_SetProperty)(cx, this, id, vp, strict);
    }

    JSBool getAttributes(JSContext *cx, jsid id, uintN *attrsp) {
        js::AttributesOp op = getOps()->getAttributes;
        return (op ? op : js_GetAttributes)(cx, this, id, attrsp);
    }

    JSBool setAttributes(JSContext *cx, jsid id, uintN *attrsp) {
        js::AttributesOp op = getOps()->setAttributes;
        return (op ? op : js_SetAttributes)(cx, this, id, attrsp);
    }

    JSBool deleteProperty(JSContext *cx, jsid id, js::Value *rval, JSBool strict) {
        js::DeleteIdOp op = getOps()->deleteProperty;
        return (op ? op : js_DeleteProperty)(cx, this, id, rval, strict);
    }

    JSBool enumerate(JSContext *cx, JSIterateOp iterop, js::Value *statep, jsid *idp) {
        js::NewEnumerateOp op = getOps()->enumerate;
        return (op ? op : js_Enumerate)(cx, this, iterop, statep, idp);
    }

    JSType typeOf(JSContext *cx) {
        js::TypeOfOp op = getOps()->typeOf;
        return (op ? op : js_TypeOf)(cx, this);
    }

    /* These four are time-optimized to avoid stub calls. */
    JSObject *thisObject(JSContext *cx) {
        JSObjectOp op = getOps()->thisObject;
        return op ? op(cx, this) : this;
    }

    static bool thisObject(JSContext *cx, const js::Value &v, js::Value *vp);

    inline JSCompartment *getCompartment() const;

    inline JSObject *getThrowTypeError() const;

    JS_FRIEND_API(JSObject *) clone(JSContext *cx, JSObject *proto, JSObject *parent);
    JS_FRIEND_API(bool) copyPropertiesFrom(JSContext *cx, JSObject *obj);
    bool swap(JSContext *cx, JSObject *other);

    const js::Shape *defineBlockVariable(JSContext *cx, jsid id, intN index);

    inline bool canHaveMethodBarrier() const;

    inline bool isArguments() const;
    inline bool isNormalArguments() const;
    inline bool isStrictArguments() const;
    inline bool isArray() const;
    inline bool isDenseArray() const;
    inline bool isSlowArray() const;
    inline bool isNumber() const;
    inline bool isBoolean() const;
    inline bool isString() const;
    inline bool isPrimitive() const;
    inline bool isDate() const;
    inline bool isFunction() const;
    inline bool isObject() const;
    inline bool isWith() const;
    inline bool isBlock() const;
    inline bool isStaticBlock() const;
    inline bool isClonedBlock() const;
    inline bool isCall() const;
    inline bool isRegExp() const;
    inline bool isScript() const;
    inline bool isXML() const;
    inline bool isXMLId() const;
    inline bool isNamespace() const;
    inline bool isQName() const;

    inline bool isProxy() const;
    inline bool isObjectProxy() const;
    inline bool isFunctionProxy() const;

    JS_FRIEND_API(bool) isWrapper() const;
    JS_FRIEND_API(JSObject *) unwrap(uintN *flagsp = NULL);

    inline void initArrayClass();
};

/* Check alignment for any fixed slots allocated after the object. */
JS_STATIC_ASSERT(sizeof(JSObject) % sizeof(js::Value) == 0);

inline js::Value*
JSObject::fixedSlots() const {
    return (js::Value*) (jsuword(this) + sizeof(JSObject));
}

inline bool
JSObject::hasSlotsArray() const { return this->slots != fixedSlots(); }

/* static */ inline size_t
JSObject::getFixedSlotOffset(size_t slot) {
    return sizeof(JSObject) + (slot * sizeof(js::Value));
}

struct JSObject_Slots2 : JSObject { js::Value fslots[2]; };
struct JSObject_Slots4 : JSObject { js::Value fslots[4]; };
struct JSObject_Slots8 : JSObject { js::Value fslots[8]; };
struct JSObject_Slots12 : JSObject { js::Value fslots[12]; };
struct JSObject_Slots16 : JSObject { js::Value fslots[16]; };

#define JSSLOT_FREE(clasp)  JSCLASS_RESERVED_SLOTS(clasp)

#ifdef JS_THREADSAFE

/*
 * The GC runs only when all threads except the one on which the GC is active
 * are suspended at GC-safe points, so calling obj->getSlot() from the GC's
 * thread is safe when rt->gcRunning is set. See jsgc.cpp for details.
 */
#define THREAD_IS_RUNNING_GC(rt, thread)                                      \
    ((rt)->gcRunning && (rt)->gcThread == (thread))

#define CX_THREAD_IS_RUNNING_GC(cx)                                           \
    THREAD_IS_RUNNING_GC((cx)->runtime, (cx)->thread)

#endif /* JS_THREADSAFE */

inline void
OBJ_TO_INNER_OBJECT(JSContext *cx, JSObject *&obj)
{
    if (JSObjectOp op = obj->getClass()->ext.innerObject)
        obj = op(cx, obj);
}

inline void
OBJ_TO_OUTER_OBJECT(JSContext *cx, JSObject *&obj)
{
    if (JSObjectOp op = obj->getClass()->ext.outerObject)
        obj = op(cx, obj);
}

class JSValueArray {
  public:
    jsval *array;
    size_t length;

    JSValueArray(jsval *v, size_t c) : array(v), length(c) {}
};

class ValueArray {
  public:
    js::Value *array;
    size_t length;

    ValueArray(js::Value *v, size_t c) : array(v), length(c) {}
};

extern js::Class js_ObjectClass;
extern js::Class js_WithClass;
extern js::Class js_BlockClass;

inline bool JSObject::isObject() const { return getClass() == &js_ObjectClass; }
inline bool JSObject::isWith() const   { return getClass() == &js_WithClass; }
inline bool JSObject::isBlock() const  { return getClass() == &js_BlockClass; }

/*
 * Block scope object macros.  The slots reserved by js_BlockClass are:
 *
 *   private              JSStackFrame *    active frame pointer or null
 *   JSSLOT_BLOCK_DEPTH   int               depth of block slots in frame
 *
 * After JSSLOT_BLOCK_DEPTH come one or more slots for the block locals.
 *
 * A With object is like a Block object, in that both have one reserved slot
 * telling the stack depth of the relevant slots (the slot whose value is the
 * object named in the with statement, the slots containing the block's local
 * variables); and both have a private slot referring to the JSStackFrame in
 * whose activation they were created (or null if the with or block object
 * outlives the frame).
 */
static const uint32 JSSLOT_BLOCK_DEPTH = 0;
static const uint32 JSSLOT_BLOCK_FIRST_FREE_SLOT = JSSLOT_BLOCK_DEPTH + 1;

inline bool
JSObject::isStaticBlock() const
{
    return isBlock() && !getProto();
}

inline bool
JSObject::isClonedBlock() const
{
    return isBlock() && !!getProto();
}

static const uint32 JSSLOT_WITH_THIS = 1;

#define OBJ_BLOCK_COUNT(cx,obj)                                               \
    (obj)->propertyCount()
#define OBJ_BLOCK_DEPTH(cx,obj)                                               \
    (obj)->getSlot(JSSLOT_BLOCK_DEPTH).toInt32()
#define OBJ_SET_BLOCK_DEPTH(cx,obj,depth)                                     \
    (obj)->setSlot(JSSLOT_BLOCK_DEPTH, Value(Int32Value(depth)))

/*
 * To make sure this slot is well-defined, always call js_NewWithObject to
 * create a With object, don't call js_NewObject directly.  When creating a
 * With object that does not correspond to a stack slot, pass -1 for depth.
 *
 * When popping the stack across this object's "with" statement, client code
 * must call withobj->setPrivate(NULL).
 */
extern JS_REQUIRES_STACK JSObject *
js_NewWithObject(JSContext *cx, JSObject *proto, JSObject *parent, jsint depth);

inline JSObject *
js_UnwrapWithObject(JSContext *cx, JSObject *withobj)
{
    JS_ASSERT(withobj->getClass() == &js_WithClass);
    return withobj->getProto();
}

/*
 * Create a new block scope object not linked to any proto or parent object.
 * Blocks are created by the compiler to reify let blocks and comprehensions.
 * Only when dynamic scope is captured do they need to be cloned and spliced
 * into an active scope chain.
 */
extern JSObject *
js_NewBlockObject(JSContext *cx);

extern JSObject *
js_CloneBlockObject(JSContext *cx, JSObject *proto, JSStackFrame *fp);

extern JS_REQUIRES_STACK JSBool
js_PutBlockObject(JSContext *cx, JSBool normalUnwind);

JSBool
js_XDRBlockObject(JSXDRState *xdr, JSObject **objp);

struct JSSharpObjectMap {
    jsrefcount  depth;
    jsatomid    sharpgen;
    JSHashTable *table;
};

#define SHARP_BIT       ((jsatomid) 1)
#define BUSY_BIT        ((jsatomid) 2)
#define SHARP_ID_SHIFT  2
#define IS_SHARP(he)    (uintptr_t((he)->value) & SHARP_BIT)
#define MAKE_SHARP(he)  ((he)->value = (void *) (uintptr_t((he)->value)|SHARP_BIT))
#define IS_BUSY(he)     (uintptr_t((he)->value) & BUSY_BIT)
#define MAKE_BUSY(he)   ((he)->value = (void *) (uintptr_t((he)->value)|BUSY_BIT))
#define CLEAR_BUSY(he)  ((he)->value = (void *) (uintptr_t((he)->value)&~BUSY_BIT))

extern JSHashEntry *
js_EnterSharpObject(JSContext *cx, JSObject *obj, JSIdArray **idap,
                    jschar **sp);

extern void
js_LeaveSharpObject(JSContext *cx, JSIdArray **idap);

/*
 * Mark objects stored in map if GC happens between js_EnterSharpObject
 * and js_LeaveSharpObject. GC calls this when map->depth > 0.
 */
extern void
js_TraceSharpMap(JSTracer *trc, JSSharpObjectMap *map);

extern JSBool
js_HasOwnPropertyHelper(JSContext *cx, js::LookupPropOp lookup, uintN argc,
                        js::Value *vp);

extern JSBool
js_HasOwnProperty(JSContext *cx, js::LookupPropOp lookup, JSObject *obj, jsid id,
                  JSObject **objp, JSProperty **propp);

extern JSBool
js_NewPropertyDescriptorObject(JSContext *cx, jsid id, uintN attrs,
                               const js::Value &getter, const js::Value &setter,
                               const js::Value &value, js::Value *vp);

extern JSBool
js_PropertyIsEnumerable(JSContext *cx, JSObject *obj, jsid id, js::Value *vp);

#ifdef OLD_GETTER_SETTER_METHODS
JS_FRIEND_API(JSBool) js_obj_defineGetter(JSContext *cx, uintN argc, js::Value *vp);
JS_FRIEND_API(JSBool) js_obj_defineSetter(JSContext *cx, uintN argc, js::Value *vp);
#endif

extern JSObject *
js_InitObjectClass(JSContext *cx, JSObject *obj);

namespace js {
JSObject *
DefineConstructorAndPrototype(JSContext *cx, JSObject *obj, JSProtoKey key, JSAtom *atom,
                              JSObject *protoProto, Class *clasp,
                              Native constructor, uintN nargs,
                              JSPropertySpec *ps, JSFunctionSpec *fs,
                              JSPropertySpec *static_ps, JSFunctionSpec *static_fs);
}

extern JSObject *
js_InitClass(JSContext *cx, JSObject *obj, JSObject *parent_proto,
             js::Class *clasp, js::Native constructor, uintN nargs,
             JSPropertySpec *ps, JSFunctionSpec *fs,
             JSPropertySpec *static_ps, JSFunctionSpec *static_fs);

/*
 * Select Object.prototype method names shared between jsapi.cpp and jsobj.cpp.
 */
extern const char js_watch_str[];
extern const char js_unwatch_str[];
extern const char js_hasOwnProperty_str[];
extern const char js_isPrototypeOf_str[];
extern const char js_propertyIsEnumerable_str[];

#ifdef OLD_GETTER_SETTER_METHODS
extern const char js_defineGetter_str[];
extern const char js_defineSetter_str[];
extern const char js_lookupGetter_str[];
extern const char js_lookupSetter_str[];
#endif

extern JSBool
js_PopulateObject(JSContext *cx, JSObject *newborn, JSObject *props);

/*
 * Fast access to immutable standard objects (constructors and prototypes).
 */
extern JSBool
js_GetClassObject(JSContext *cx, JSObject *obj, JSProtoKey key,
                  JSObject **objp);

extern JSBool
js_SetClassObject(JSContext *cx, JSObject *obj, JSProtoKey key,
                  JSObject *cobj, JSObject *prototype);

/*
 * If protoKey is not JSProto_Null, then clasp is ignored. If protoKey is
 * JSProto_Null, clasp must non-null.
 */
extern JSBool
js_FindClassObject(JSContext *cx, JSObject *start, JSProtoKey key,
                   js::Value *vp, js::Class *clasp = NULL);

extern JSObject *
js_ConstructObject(JSContext *cx, js::Class *clasp, JSObject *proto,
                   JSObject *parent, uintN argc, js::Value *argv);

// Specialized call for constructing |this| with a known function callee,
// and a known prototype.
extern JSObject *
js_CreateThisForFunctionWithProto(JSContext *cx, JSObject *callee, JSObject *proto);

// Specialized call for constructing |this| with a known function callee.
extern JSObject *
js_CreateThisForFunction(JSContext *cx, JSObject *callee);

// Generic call for constructing |this|.
extern JSObject *
js_CreateThis(JSContext *cx, JSObject *callee);

extern jsid
js_CheckForStringIndex(jsid id);

/*
 * js_PurgeScopeChain does nothing if obj is not itself a prototype or parent
 * scope, else it reshapes the scope and prototype chains it links. It calls
 * js_PurgeScopeChainHelper, which asserts that obj is flagged as a delegate
 * (i.e., obj has ever been on a prototype or parent chain).
 */
extern void
js_PurgeScopeChainHelper(JSContext *cx, JSObject *obj, jsid id);

inline void
js_PurgeScopeChain(JSContext *cx, JSObject *obj, jsid id)
{
    if (obj->isDelegate())
        js_PurgeScopeChainHelper(cx, obj, id);
}

/*
 * Find or create a property named by id in obj's scope, with the given getter
 * and setter, slot, attributes, and other members.
 */
extern const js::Shape *
js_AddNativeProperty(JSContext *cx, JSObject *obj, jsid id,
                     js::PropertyOp getter, js::StrictPropertyOp setter, uint32 slot,
                     uintN attrs, uintN flags, intN shortid);

/*
 * Change shape to have the given attrs, getter, and setter in scope, morphing
 * it into a potentially new js::Shape.  Return a pointer to the changed
 * or identical property.
 */
extern const js::Shape *
js_ChangeNativePropertyAttrs(JSContext *cx, JSObject *obj,
                             const js::Shape *shape, uintN attrs, uintN mask,
                             js::PropertyOp getter, js::StrictPropertyOp setter);

extern JSBool
js_DefineOwnProperty(JSContext *cx, JSObject *obj, jsid id,
                     const js::Value &descriptor, JSBool *bp);

/*
 * Flags for the defineHow parameter of js_DefineNativeProperty.
 */
const uintN JSDNP_CACHE_RESULT = 1; /* an interpreter call from JSOP_INITPROP */
const uintN JSDNP_DONT_PURGE   = 2; /* suppress js_PurgeScopeChain */
const uintN JSDNP_SET_METHOD   = 4; /* js_{DefineNativeProperty,SetPropertyHelper}
                                       must pass the js::Shape::METHOD
                                       flag on to JSObject::{add,put}Property */
const uintN JSDNP_UNQUALIFIED  = 8; /* Unqualified property set.  Only used in
                                       the defineHow argument of
                                       js_SetPropertyHelper. */

/*
 * On error, return false.  On success, if propp is non-null, return true with
 * obj locked and with a held property in *propp; if propp is null, return true
 * but release obj's lock first.
 */
extern JSBool
js_DefineNativeProperty(JSContext *cx, JSObject *obj, jsid id, const js::Value &value,
                        js::PropertyOp getter, js::StrictPropertyOp setter, uintN attrs,
                        uintN flags, intN shortid, JSProperty **propp,
                        uintN defineHow = 0);

/*
 * Specialized subroutine that allows caller to preset JSRESOLVE_* flags and
 * returns the index along the prototype chain in which *propp was found, or
 * the last index if not found, or -1 on error.
 */
extern int
js_LookupPropertyWithFlags(JSContext *cx, JSObject *obj, jsid id, uintN flags,
                           JSObject **objp, JSProperty **propp);


extern JS_FRIEND_DATA(js::Class) js_CallClass;
extern JS_FRIEND_DATA(js::Class) js_DeclEnvClass;

namespace js {

/*
 * We cache name lookup results only for the global object or for native
 * non-global objects without prototype or with prototype that never mutates,
 * see bug 462734 and bug 487039.
 */
static inline bool
IsCacheableNonGlobalScope(JSObject *obj)
{
    JS_ASSERT(obj->getParent());

    js::Class *clasp = obj->getClass();
    bool cacheable = (clasp == &js_CallClass ||
                      clasp == &js_BlockClass ||
                      clasp == &js_DeclEnvClass);

    JS_ASSERT_IF(cacheable, !obj->getOps()->lookupProperty);
    return cacheable;
}

}

/*
 * If cacheResult is false, return JS_NO_PROP_CACHE_FILL on success.
 */
extern js::PropertyCacheEntry *
js_FindPropertyHelper(JSContext *cx, jsid id, JSBool cacheResult,
                      JSObject **objp, JSObject **pobjp, JSProperty **propp);

/*
 * Return the index along the scope chain in which id was found, or the last
 * index if not found, or -1 on error.
 */
extern JS_FRIEND_API(JSBool)
js_FindProperty(JSContext *cx, jsid id, JSObject **objp, JSObject **pobjp,
                JSProperty **propp);

extern JS_REQUIRES_STACK JSObject *
js_FindIdentifierBase(JSContext *cx, JSObject *scopeChain, jsid id);

extern JSObject *
js_FindVariableScope(JSContext *cx, JSFunction **funp);

/*
 * JSGET_CACHE_RESULT is the analogue of JSDNP_CACHE_RESULT for js_GetMethod.
 *
 * JSGET_METHOD_BARRIER (the default, hence 0 but provided for documentation)
 * enables a read barrier that preserves standard function object semantics (by
 * default we assume our caller won't leak a joined callee to script, where it
 * would create hazardous mutable object sharing as well as observable identity
 * according to == and ===.
 *
 * JSGET_NO_METHOD_BARRIER avoids the performance overhead of the method read
 * barrier, which is not needed when invoking a lambda that otherwise does not
 * leak its callee reference (via arguments.callee or its name).
 */
const uintN JSGET_CACHE_RESULT      = 1; // from a caching interpreter opcode
const uintN JSGET_METHOD_BARRIER    = 0; // get can leak joined function object
const uintN JSGET_NO_METHOD_BARRIER = 2; // call to joined function can't leak

/*
 * NB: js_NativeGet and js_NativeSet are called with the scope containing shape
 * (pobj's scope for Get, obj's for Set) locked, and on successful return, that
 * scope is again locked.  But on failure, both functions return false with the
 * scope containing shape unlocked.
 */
extern JSBool
js_NativeGet(JSContext *cx, JSObject *obj, JSObject *pobj, const js::Shape *shape, uintN getHow,
             js::Value *vp);

extern JSBool
js_NativeSet(JSContext *cx, JSObject *obj, const js::Shape *shape, bool added,
             bool strict, js::Value *vp);

extern JSBool
js_GetPropertyHelper(JSContext *cx, JSObject *obj, jsid id, uint32 getHow, js::Value *vp);

extern bool
js_GetPropertyHelperWithShape(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id,
                              uint32 getHow, js::Value *vp,
                              const js::Shape **shapeOut, JSObject **holderOut);

extern JSBool
js_GetOwnPropertyDescriptor(JSContext *cx, JSObject *obj, jsid id, js::Value *vp);

extern JSBool
js_GetMethod(JSContext *cx, JSObject *obj, jsid id, uintN getHow, js::Value *vp);

/*
 * Check whether it is OK to assign an undeclared property with name
 * propname of the global object in the current script on cx.  Reports
 * an error if one needs to be reported (in particular in all cases
 * when it returns false).
 */
extern JS_FRIEND_API(bool)
js_CheckUndeclaredVarAssignment(JSContext *cx, JSString *propname);

extern JSBool
js_SetPropertyHelper(JSContext *cx, JSObject *obj, jsid id, uintN defineHow,
                     js::Value *vp, JSBool strict);

/*
 * Change attributes for the given native property. The caller must ensure
 * that obj is locked and this function always unlocks obj on return.
 */
extern JSBool
js_SetNativeAttributes(JSContext *cx, JSObject *obj, js::Shape *shape,
                       uintN attrs);

namespace js {

/*
 * If obj has a data property methodid which is a function object for the given
 * native, return that function object. Otherwise, return NULL.
 */
extern JSObject *
HasNativeMethod(JSObject *obj, jsid methodid, Native native);

extern bool
DefaultValue(JSContext *cx, JSObject *obj, JSType hint, Value *vp);

extern JSBool
CheckAccess(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
            js::Value *vp, uintN *attrsp);

} /* namespace js */

extern bool
js_IsDelegate(JSContext *cx, JSObject *obj, const js::Value &v);

/*
 * If protoKey is not JSProto_Null, then clasp is ignored. If protoKey is
 * JSProto_Null, clasp must non-null.
 */
extern JS_FRIEND_API(JSBool)
js_GetClassPrototype(JSContext *cx, JSObject *scope, JSProtoKey protoKey,
                     JSObject **protop, js::Class *clasp = NULL);

extern JSBool
js_SetClassPrototype(JSContext *cx, JSObject *ctor, JSObject *proto,
                     uintN attrs);

/*
 * Wrap boolean, number or string as Boolean, Number or String object.
 * *vp must not be an object, null or undefined.
 */
extern JSBool
js_PrimitiveToObject(JSContext *cx, js::Value *vp);

/*
 * v and vp may alias. On successful return, vp->isObjectOrNull(). If vp is not
 * rooted, the caller must root vp before the next possible GC.
 */
extern JSBool
js_ValueToObjectOrNull(JSContext *cx, const js::Value &v, JSObject **objp);

namespace js {

/*
 * Invokes the ES5 ToObject algorithm on *vp, writing back the object to vp.
 * If *vp might already be an object, use ToObject.
 */
extern JSObject *
ToObjectSlow(JSContext *cx, js::Value *vp);

JS_ALWAYS_INLINE JSObject *
ToObject(JSContext *cx, js::Value *vp)
{
    if (vp->isObject())
        return &vp->toObject();
    return ToObjectSlow(cx, vp);
}

}

/*
 * v and vp may alias. On successful return, vp->isObject(). If vp is not
 * rooted, the caller must root vp before the next possible GC.
 */
extern JSObject *
js_ValueToNonNullObject(JSContext *cx, const js::Value &v);

extern JSBool
js_TryValueOf(JSContext *cx, JSObject *obj, JSType type, js::Value *rval);

extern JSBool
js_TryMethod(JSContext *cx, JSObject *obj, JSAtom *atom,
             uintN argc, js::Value *argv, js::Value *rval);

extern JSBool
js_XDRObject(JSXDRState *xdr, JSObject **objp);

extern void
js_TraceObject(JSTracer *trc, JSObject *obj);

extern void
js_PrintObjectSlotName(JSTracer *trc, char *buf, size_t bufsize);

extern void
js_ClearNative(JSContext *cx, JSObject *obj);

extern bool
js_GetReservedSlot(JSContext *cx, JSObject *obj, uint32 index, js::Value *vp);

extern bool
js_SetReservedSlot(JSContext *cx, JSObject *obj, uint32 index, const js::Value &v);

extern JSBool
js_CheckPrincipalsAccess(JSContext *cx, JSObject *scopeobj,
                         JSPrincipals *principals, JSAtom *caller);

/* For CSP -- checks if eval() and friends are allowed to run. */
extern JSBool
js_CheckContentSecurityPolicy(JSContext *cx, JSObject *scopeObj);

/* NB: Infallible. */
extern const char *
js_ComputeFilename(JSContext *cx, JSStackFrame *caller,
                   JSPrincipals *principals, uintN *linenop);

extern JSBool
js_ReportGetterOnlyAssignment(JSContext *cx);

extern JS_FRIEND_API(JSBool)
js_GetterOnlyPropertyStub(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp);

#ifdef DEBUG
JS_FRIEND_API(void) js_DumpChars(const jschar *s, size_t n);
JS_FRIEND_API(void) js_DumpString(JSString *str);
JS_FRIEND_API(void) js_DumpAtom(JSAtom *atom);
JS_FRIEND_API(void) js_DumpObject(JSObject *obj);
JS_FRIEND_API(void) js_DumpValue(const js::Value &val);
JS_FRIEND_API(void) js_DumpId(jsid id);
JS_FRIEND_API(void) js_DumpStackFrame(JSContext *cx, JSStackFrame *start = NULL);
#endif

extern uintN
js_InferFlags(JSContext *cx, uintN defaultFlags);

/* Object constructor native. Exposed only so the JIT can know its address. */
JSBool
js_Object(JSContext *cx, uintN argc, js::Value *vp);


namespace js {

extern bool
SetProto(JSContext *cx, JSObject *obj, JSObject *proto, bool checkForCycles);

extern JSString *
obj_toStringHelper(JSContext *cx, JSObject *obj);

enum EvalType { INDIRECT_EVAL, DIRECT_EVAL };

/*
 * Common code implementing direct and indirect eval.
 *
 * Evaluate vp[2], if it is a string, in the context of the given calling
 * frame, with the provided scope chain, with the semantics of either a direct
 * or indirect eval (see ES5 10.4.2).  If this is an indirect eval, scopeobj
 * must be a global object.
 *
 * On success, store the completion value in *vp and return true.
 */
extern bool
EvalKernel(JSContext *cx, uintN argc, js::Value *vp, EvalType evalType, JSStackFrame *caller,
           JSObject *scopeobj);

extern JS_FRIEND_API(bool)
IsBuiltinEvalFunction(JSFunction *fun);

}

#ifdef JS_OBJ_UNDEFD_MOZALLOC_WRAPPERS
#  include "mozilla/mozalloc_macro_wrappers.h"
#endif

#endif /* jsobj_h___ */
