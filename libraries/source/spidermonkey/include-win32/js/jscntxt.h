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

#ifndef jscntxt_h___
#define jscntxt_h___
/*
 * JS execution context.
 */
#include <string.h>

/* Gross special case for Gecko, which defines malloc/calloc/free. */
#ifdef mozilla_mozalloc_macro_wrappers_h
#  define JS_CNTXT_UNDEFD_MOZALLOC_WRAPPERS
/* The "anti-header" */
#  include "mozilla/mozalloc_undef_macro_wrappers.h"
#endif

#include "jsprvtd.h"
#include "jsarena.h"
#include "jsclist.h"
#include "jslong.h"
#include "jsatom.h"
#include "jsdhash.h"
#include "jsdtoa.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsgcchunk.h"
#include "jshashtable.h"
#include "jsinterp.h"
#include "jsmath.h"
#include "jsobj.h"
#include "jspropertycache.h"
#include "jspropertytree.h"
#include "jsstaticcheck.h"
#include "jsutil.h"
#include "jsarray.h"
#include "jsvector.h"
#include "prmjtime.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4100) /* Silence unreferenced formal parameter warnings */
#pragma warning(push)
#pragma warning(disable:4355) /* Silence warning about "this" used in base member initializer list */
#endif

/*
 * js_GetSrcNote cache to avoid O(n^2) growth in finding a source note for a
 * given pc in a script. We use the script->code pointer to tag the cache,
 * instead of the script address itself, so that source notes are always found
 * by offset from the bytecode with which they were generated.
 */
typedef struct JSGSNCache {
    jsbytecode      *code;
    JSDHashTable    table;
#ifdef JS_GSNMETER
    uint32          hits;
    uint32          misses;
    uint32          fills;
    uint32          purges;
# define GSN_CACHE_METER(cache,cnt) (++(cache)->cnt)
#else
# define GSN_CACHE_METER(cache,cnt) /* nothing */
#endif
} JSGSNCache;

#define js_FinishGSNCache(cache) js_PurgeGSNCache(cache)

extern void
js_PurgeGSNCache(JSGSNCache *cache);

/* These helper macros take a cx as parameter and operate on its GSN cache. */
#define JS_PURGE_GSN_CACHE(cx)      js_PurgeGSNCache(&JS_GSN_CACHE(cx))
#define JS_METER_GSN_CACHE(cx,cnt)  GSN_CACHE_METER(&JS_GSN_CACHE(cx), cnt)

/* Forward declarations of nanojit types. */
namespace nanojit {

class Assembler;
class CodeAlloc;
class Fragment;
template<typename K> struct DefaultHash;
template<typename K, typename V, typename H> class HashMap;
template<typename T> class Seq;

}  /* namespace nanojit */

namespace js {

/* Tracer constants. */
static const size_t MONITOR_N_GLOBAL_STATES = 4;
static const size_t FRAGMENT_TABLE_SIZE = 512;
static const size_t MAX_NATIVE_STACK_SLOTS = 4096;
static const size_t MAX_CALL_STACK_ENTRIES = 500;
static const size_t MAX_GLOBAL_SLOTS = 4096;
static const size_t GLOBAL_SLOTS_BUFFER_SIZE = MAX_GLOBAL_SLOTS + 1;
static const size_t MAX_SLOW_NATIVE_EXTRA_SLOTS = 16;

/* Forward declarations of tracer types. */
class VMAllocator;
class FrameInfoCache;
struct FrameInfo;
struct VMSideExit;
struct TreeFragment;
struct TracerState;
template<typename T> class Queue;
typedef Queue<uint16> SlotList;
class TypeMap;
class LoopProfile;

#if defined(JS_JIT_SPEW) || defined(DEBUG)
struct FragPI;
typedef nanojit::HashMap<uint32, FragPI, nanojit::DefaultHash<uint32> > FragStatsMap;
#endif

namespace mjit {
class JaegerCompartment;
}

/*
 * Allocation policy that calls JSContext memory functions and reports errors
 * to the context. Since the JSContext given on construction is stored for
 * the lifetime of the container, this policy may only be used for containers
 * whose lifetime is a shorter than the given JSContext.
 */
class ContextAllocPolicy
{
    JSContext *cx;

  public:
    ContextAllocPolicy(JSContext *cx) : cx(cx) {}
    JSContext *context() const { return cx; }

    /* Inline definitions below. */
    void *malloc(size_t bytes);
    void free(void *p);
    void *realloc(void *p, size_t bytes);
    void reportAllocOverflow() const;
};

/*
 * A StackSegment (referred to as just a 'segment') contains a prev-linked set
 * of stack frames and the slots associated with each frame. A segment and its
 * contained frames/slots also have a precise memory layout that is described
 * in the js::StackSpace comment. A key layout invariant for segments is that
 * prev-linked frames are adjacent in memory, separated only by the values that
 * constitute the locals and expression stack of the prev-frame.
 *
 * The set of stack frames in a non-empty segment start at the segment's
 * "current frame", which is the most recently pushed frame, and ends at the
 * segment's "initial frame". Note that, while all stack frames in a segment
 * are prev-linked, not all prev-linked frames are in the same segment. Hence,
 * for a segment |ss|, |ss->getInitialFrame()->prev| may be non-null and in a
 * different segment. This occurs when the VM reenters itself (via Invoke or
 * Execute). In full generality, a single context may contain a forest of trees
 * of stack frames. With respect to this forest, a segment contains a linear
 * path along a single tree, not necessarily to the root.
 *
 * The frames of a non-empty segment must all be in the same context and thus
 * each non-empty segment is referred to as being "in" a context. Segments in a
 * context have an additional state of being either "active" or "suspended". A
 * suspended segment |ss| has a "suspended frame" which is snapshot of |cx->regs|
 * when the segment was suspended and serves as the current frame of |ss|.
 * There is at most one active segment in a given context. Segments in a
 * context execute LIFO and are maintained in a stack.  The top of this stack
 * is the context's "current segment". If a context |cx| has an active segment
 * |ss|, then:
 *   1. |ss| is |cx|'s current segment,
 *   2. |cx->regs != NULL|, and
 *   3. |ss|'s current frame is |cx->regs->fp|.
 * Moreover, |cx->regs != NULL| iff |cx| has an active segment.
 *
 * An empty segment is not associated with any context. Empty segments are
 * created when there is not an active segment for a context at the top of the
 * stack and claim space for the arguments of an Invoke before the Invoke's
 * stack frame is pushed. During the intervals when the arguments have been
 * pushed, but not the stack frame, the segment cannot be pushed onto the
 * context, since that would require some hack to deal with cx->fp not being
 * the current frame of cx->currentSegment.
 *
 * Finally, (to support JS_SaveFrameChain/JS_RestoreFrameChain) a suspended
 * segment may or may not be "saved". Normally, when the active segment is
 * popped, the previous segment (which is necessarily suspended) becomes
 * active. If the previous segment was saved, however, then it stays suspended
 * until it is made active by a call to JS_RestoreFrameChain. This is why a
 * context may have a current segment, but not an active segment.
 */
class StackSegment
{
    /* The context to which this segment belongs. */
    JSContext           *cx;

    /* Link for JSContext segment stack mentioned in big comment above. */
    StackSegment        *previousInContext;

    /* Link for StackSpace segment stack mentioned in StackSpace comment. */
    StackSegment        *previousInMemory;

    /* The first frame executed in this segment. null iff cx is null */
    JSStackFrame        *initialFrame;

    /* If this segment is suspended, |cx->regs| when it was suspended. */
    JSFrameRegs         *suspendedRegs;

    /* The varobj on entry to initialFrame. */
    JSObject            *initialVarObj;

    /* Whether this segment was suspended by JS_SaveFrameChain. */
    bool                saved;

    /* Align at 8 bytes on all platforms. */
#if JS_BITS_PER_WORD == 32
    void                *padding;
#endif

    /*
     * To make isActive a single null-ness check, this non-null constant is
     * assigned to suspendedRegs when !inContext.
     */
#define NON_NULL_SUSPENDED_REGS ((JSFrameRegs *)0x1)

  public:
    StackSegment()
      : cx(NULL), previousInContext(NULL), previousInMemory(NULL),
        initialFrame(NULL), suspendedRegs(NON_NULL_SUSPENDED_REGS),
        initialVarObj(NULL), saved(false)
    {
        JS_ASSERT(!inContext());
    }

    /* Safe casts guaranteed by the contiguous-stack layout. */

    Value *valueRangeBegin() const {
        return (Value *)(this + 1);
    }

    /*
     * As described in the comment at the beginning of the class, a segment
     * is in one of three states:
     *
     *  !inContext:  the segment has been created to root arguments for a
     *               future call to Invoke.
     *  isActive:    the segment describes a set of stack frames in a context,
     *               where the top frame currently executing.
     *  isSuspended: like isActive, but the top frame has been suspended.
     */

    bool inContext() const {
        JS_ASSERT(!!cx == !!initialFrame);
        JS_ASSERT_IF(!cx, suspendedRegs == NON_NULL_SUSPENDED_REGS && !saved);
        return cx;
    }

    bool isActive() const {
        JS_ASSERT_IF(!suspendedRegs, cx && !saved);
        JS_ASSERT_IF(!cx, suspendedRegs == NON_NULL_SUSPENDED_REGS);
        return !suspendedRegs;
    }

    bool isSuspended() const {
        JS_ASSERT_IF(!cx || !suspendedRegs, !saved);
        JS_ASSERT_IF(!cx, suspendedRegs == NON_NULL_SUSPENDED_REGS);
        return cx && suspendedRegs;
    }

    /* Substate of suspended, queryable in any state. */

    bool isSaved() const {
        JS_ASSERT_IF(saved, isSuspended());
        return saved;
    }

    /* Transitioning between inContext <--> isActive */

    void joinContext(JSContext *cx, JSStackFrame *f) {
        JS_ASSERT(!inContext());
        this->cx = cx;
        initialFrame = f;
        suspendedRegs = NULL;
        JS_ASSERT(isActive());
    }

    void leaveContext() {
        JS_ASSERT(isActive());
        this->cx = NULL;
        initialFrame = NULL;
        suspendedRegs = NON_NULL_SUSPENDED_REGS;
        JS_ASSERT(!inContext());
    }

    JSContext *maybeContext() const {
        return cx;
    }

#undef NON_NULL_SUSPENDED_REGS

    /* Transitioning between isActive <--> isSuspended */

    void suspend(JSFrameRegs *regs) {
        JS_ASSERT(isActive());
        JS_ASSERT(regs && regs->fp && contains(regs->fp));
        suspendedRegs = regs;
        JS_ASSERT(isSuspended());
    }

    void resume() {
        JS_ASSERT(isSuspended());
        suspendedRegs = NULL;
        JS_ASSERT(isActive());
    }

    /* When isSuspended, transitioning isSaved <--> !isSaved */

    void save(JSFrameRegs *regs) {
        JS_ASSERT(!isSuspended());
        suspend(regs);
        saved = true;
        JS_ASSERT(isSaved());
    }

    void restore() {
        JS_ASSERT(isSaved());
        saved = false;
        resume();
        JS_ASSERT(!isSuspended());
    }

    /* Data available when inContext */

    JSStackFrame *getInitialFrame() const {
        JS_ASSERT(inContext());
        return initialFrame;
    }

    inline JSFrameRegs *getCurrentRegs() const;
    inline JSStackFrame *getCurrentFrame() const;

    /* Data available when isSuspended. */

    JSFrameRegs *getSuspendedRegs() const {
        JS_ASSERT(isSuspended());
        return suspendedRegs;
    }

    JSStackFrame *getSuspendedFrame() const {
        return suspendedRegs->fp;
    }

    /* JSContext / js::StackSpace bookkeeping. */

    void setPreviousInContext(StackSegment *seg) {
        previousInContext = seg;
    }

    StackSegment *getPreviousInContext() const  {
        return previousInContext;
    }

    void setPreviousInMemory(StackSegment *seg) {
        previousInMemory = seg;
    }

    StackSegment *getPreviousInMemory() const  {
        return previousInMemory;
    }

    void setInitialVarObj(JSObject *obj) {
        JS_ASSERT(inContext());
        initialVarObj = obj;
    }

    bool hasInitialVarObj() {
        JS_ASSERT(inContext());
        return initialVarObj != NULL;
    }

    JSObject &getInitialVarObj() const {
        JS_ASSERT(inContext() && initialVarObj);
        return *initialVarObj;
    }

#ifdef DEBUG
    JS_REQUIRES_STACK bool contains(const JSStackFrame *fp) const;
#endif
};

static const size_t VALUES_PER_STACK_SEGMENT = sizeof(StackSegment) / sizeof(Value);
JS_STATIC_ASSERT(sizeof(StackSegment) % sizeof(Value) == 0);

/* See StackSpace::pushInvokeArgs. */
class InvokeArgsGuard : public CallArgs
{
    friend class StackSpace;
    JSContext        *cx;  /* null implies nothing pushed */
    StackSegment     *seg;
    Value            *prevInvokeArgEnd;
#ifdef DEBUG
    StackSegment     *prevInvokeSegment;
    JSStackFrame     *prevInvokeFrame;
#endif
  public:
    InvokeArgsGuard() : cx(NULL), seg(NULL) {}
    ~InvokeArgsGuard();
    bool pushed() const { return cx != NULL; }
};

/*
 * This type can be used to call Invoke when the arguments have already been
 * pushed onto the stack as part of normal execution.
 */
struct InvokeArgsAlreadyOnTheStack : CallArgs
{
    InvokeArgsAlreadyOnTheStack(Value *vp, uintN argc) : CallArgs(vp + 2, argc) {}
};

/* See StackSpace::pushInvokeFrame. */
class InvokeFrameGuard
{
    friend class StackSpace;
    JSContext        *cx_;  /* null implies nothing pushed */
    JSFrameRegs      regs_;
    JSFrameRegs      *prevRegs_;
  public:
    InvokeFrameGuard() : cx_(NULL) {}
    ~InvokeFrameGuard() { if (pushed()) pop(); }
    bool pushed() const { return cx_ != NULL; }
    JSContext *pushedFrameContext() const { JS_ASSERT(pushed()); return cx_; }
    void pop();
    JSStackFrame *fp() const { return regs_.fp; }
};

/* Reusable base; not for direct use. */
class FrameGuard
{
    friend class StackSpace;
    JSContext        *cx_;  /* null implies nothing pushed */
    StackSegment     *seg_;
    Value            *vp_;
    JSStackFrame     *fp_;
  public:
    FrameGuard() : cx_(NULL), vp_(NULL), fp_(NULL) {}
    JS_REQUIRES_STACK ~FrameGuard();
    bool pushed() const { return cx_ != NULL; }
    StackSegment *segment() const { return seg_; }
    Value *vp() const { return vp_; }
    JSStackFrame *fp() const { return fp_; }
};

/* See StackSpace::pushExecuteFrame. */
class ExecuteFrameGuard : public FrameGuard
{
    friend class StackSpace;
    JSFrameRegs      regs_;
};

/* See StackSpace::pushDummyFrame. */
class DummyFrameGuard : public FrameGuard
{
    friend class StackSpace;
    JSFrameRegs      regs_;
};

/* See StackSpace::pushGeneratorFrame. */
class GeneratorFrameGuard : public FrameGuard
{};

/*
 * Stack layout
 *
 * Each JSThreadData has one associated StackSpace object which allocates all
 * segments for the thread. StackSpace performs all such allocations in a
 * single, fixed-size buffer using a specific layout scheme that allows some
 * associations between segments, frames, and slots to be implicit, rather
 * than explicitly stored as pointers. To maintain useful invariants, stack
 * space is not given out arbitrarily, but rather allocated/deallocated for
 * specific purposes. The use cases currently supported are: calling a function
 * with arguments (e.g. Invoke), executing a script (e.g. Execute), inline
 * interpreter calls, and pushing "dummy" frames for bookkeeping purposes. See
 * associated member functions below.
 *
 * First, we consider the layout of individual segments. (See the
 * js::StackSegment comment for terminology.) A non-empty segment (i.e., a
 * segment in a context) has the following layout:
 *
 *           initial frame                 current frame ------.  if regs,
 *          .------------.                           |         |  regs->sp
 *          |            V                           V         V
 *   |segment| slots |frame| slots |frame| slots |frame| slots |
 *                       |  ^          |  ^          |
 *          ? <----------'  `----------'  `----------'
 *                prev          prev          prev
 *
 * Moreover, the bytes in the following ranges form a contiguous array of
 * Values that are marked during GC:
 *   1. between a segment and its first frame
 *   2. between two adjacent frames in a segment
 *   3. between a segment's current frame and (if fp->regs) fp->regs->sp
 * Thus, the VM must ensure that all such Values are safe to be marked.
 *
 * An empty segment is followed by arguments that are rooted by the
 * StackSpace::invokeArgEnd pointer:
 *
 *              invokeArgEnd
 *                   |
 *                   V
 *   |segment| slots |
 *
 * Above the level of segments, a StackSpace is simply a contiguous sequence
 * of segments kept in a linked list:
 *
 *   base                       currentSegment  firstUnused            end
 *    |                               |             |                   |
 *    V                               V             V                   V
 *    |segment| --- |segment| --- |segment| ------- |                   |
 *         | ^           | ^           |
 *   0 <---' `-----------' `-----------'
 *   previous    previous       previous
 *
 * Both js::StackSpace and JSContext maintain a stack of segments, the top of
 * which is the "current segment" for that thread or context, respectively.
 * Since different contexts can arbitrarily interleave execution in a single
 * thread, these stacks are different enough that a segment needs both
 * "previousInMemory" and "previousInContext".
 *
 * For example, in a single thread, a function in segment S1 in a context CX1
 * may call out into C++ code that reenters the VM in a context CX2, which
 * creates a new segment S2 in CX2, and CX1 may or may not equal CX2.
 *
 * Note that there is some structure to this interleaving of segments:
 *   1. the inclusion from segments in a context to segments in a thread
 *      preserves order (in terms of previousInContext and previousInMemory,
 *      respectively).
 *   2. the mapping from stack frames to their containing segment preserves
 *      order (in terms of prev and previousInContext, respectively).
 */
class StackSpace
{
    Value *base;
#ifdef XP_WIN
    mutable Value *commitEnd;
#endif
    Value *end;
    StackSegment *currentSegment;
#ifdef DEBUG
    /*
     * Keep track of which segment/frame bumped invokeArgEnd so that
     * firstUnused() can assert that, when invokeArgEnd is used as the top of
     * the stack, it is being used appropriately.
     */
    StackSegment *invokeSegment;
    JSStackFrame *invokeFrame;
#endif
    Value        *invokeArgEnd;

    friend class InvokeArgsGuard;
    friend class InvokeFrameGuard;
    friend class FrameGuard;

    bool pushSegmentForInvoke(JSContext *cx, uintN argc, InvokeArgsGuard *ag);
    void popSegmentForInvoke(const InvokeArgsGuard &ag);

    bool pushInvokeFrameSlow(JSContext *cx, const InvokeArgsGuard &ag,
                             InvokeFrameGuard *fg);
    void popInvokeFrameSlow(const CallArgs &args);

    bool getSegmentAndFrame(JSContext *cx, uintN vplen, uintN nslots,
                            FrameGuard *fg) const;
    void pushSegmentAndFrame(JSContext *cx, JSFrameRegs *regs, FrameGuard *fg);
    void popSegmentAndFrame(JSContext *cx);

    struct EnsureSpaceCheck {
        inline bool operator()(const StackSpace &, JSContext *, Value *, uintN);
    };

    struct LimitCheck {
        JSStackFrame *base;
        Value **limit;
        LimitCheck(JSStackFrame *base, Value **limit) : base(base), limit(limit) {}
        inline bool operator()(const StackSpace &, JSContext *, Value *, uintN);
    };

    template <class Check>
    inline JSStackFrame *getCallFrame(JSContext *cx, Value *sp, uintN nactual,
                                      JSFunction *fun, JSScript *script,
                                      uint32 *pflags, Check check) const;

    inline void popInvokeArgs(const InvokeArgsGuard &args);
    inline void popInvokeFrame(const InvokeFrameGuard &ag);

    inline Value *firstUnused() const;

    inline bool isCurrentAndActive(JSContext *cx) const;
    friend class AllFramesIter;
    StackSegment *getCurrentSegment() const { return currentSegment; }

#ifdef XP_WIN
    /* Commit more memory from the reserved stack space. */
    JS_FRIEND_API(bool) bumpCommit(Value *from, ptrdiff_t nvals) const;
#endif

  public:
    static const size_t CAPACITY_VALS   = 512 * 1024;
    static const size_t CAPACITY_BYTES  = CAPACITY_VALS * sizeof(Value);
    static const size_t COMMIT_VALS     = 16 * 1024;
    static const size_t COMMIT_BYTES    = COMMIT_VALS * sizeof(Value);

    /*
     * SunSpider and v8bench have roughly an average of 9 slots per script.
     * Our heuristic for a quick over-recursion check uses a generous slot
     * count based on this estimate. We take this frame size and multiply it
     * by the old recursion limit from the interpreter.
     *
     * Worst case, if an average size script (<=9 slots) over recurses, it'll
     * effectively be the same as having increased the old inline call count
     * to <= 5,000.
     */
    static const size_t STACK_QUOTA    = (VALUES_PER_STACK_FRAME + 18) *
                                         JS_MAX_INLINE_CALL_COUNT;

    /* Kept as a member of JSThreadData; cannot use constructor/destructor. */
    bool init();
    void finish();

#ifdef DEBUG
    template <class T>
    bool contains(T *t) const {
        char *v = (char *)t;
        JS_ASSERT(size_t(-1) - uintptr_t(t) >= sizeof(T));
        return v >= (char *)base && v + sizeof(T) <= (char *)end;
    }
#endif

    /*
     * When we LeaveTree, we need to rebuild the stack, which requires stack
     * allocation. There is no good way to handle an OOM for these allocations,
     * so this function checks that they cannot occur using the size of the
     * TraceNativeStorage as a conservative upper bound.
     */
    inline bool ensureEnoughSpaceToEnterTrace();

    /* +1 for slow native's stack frame. */
    static const ptrdiff_t MAX_TRACE_SPACE_VALS =
      MAX_NATIVE_STACK_SLOTS + MAX_CALL_STACK_ENTRIES * VALUES_PER_STACK_FRAME +
      (VALUES_PER_STACK_SEGMENT + VALUES_PER_STACK_FRAME /* synthesized slow native */);

    /* Mark all segments, frames, and slots on the stack. */
    JS_REQUIRES_STACK void mark(JSTracer *trc);

    /*
     * For all five use cases below:
     *  - The boolean-valued functions call js_ReportOutOfScriptQuota on OOM.
     *  - The "get*Frame" functions do not change any global state, they just
     *    check OOM and return pointers to an uninitialized frame with the
     *    requested missing arguments/slots. Only once the "push*Frame"
     *    function has been called is global state updated. Thus, between
     *    "get*Frame" and "push*Frame", the frame and slots are unrooted.
     *  - The "push*Frame" functions will set fp->prev; the caller needn't.
     *  - Functions taking "*Guard" arguments will use the guard's destructor
     *    to pop the allocation. The caller must ensure the guard has the
     *    appropriate lifetime.
     *  - The get*Frame functions put the 'nmissing' slots contiguously after
     *    the arguments.
     */

    /*
     * pushInvokeArgs allocates |argc + 2| rooted values that will be passed as
     * the arguments to Invoke. A single allocation can be used for multiple
     * Invoke calls. The InvokeArgumentsGuard passed to Invoke must come from
     * an immediately-enclosing (stack-wise) call to pushInvokeArgs.
     */
    bool pushInvokeArgs(JSContext *cx, uintN argc, InvokeArgsGuard *ag);

    /* These functions are called inside Invoke, not Invoke clients. */
    bool getInvokeFrame(JSContext *cx, const CallArgs &args, JSFunction *fun,
                        JSScript *script, uint32 *flags, InvokeFrameGuard *fg) const;

    void pushInvokeFrame(JSContext *cx, const CallArgs &args, InvokeFrameGuard *fg);

    /* These functions are called inside Execute, not Execute clients. */
    bool getExecuteFrame(JSContext *cx, JSScript *script, ExecuteFrameGuard *fg) const;
    void pushExecuteFrame(JSContext *cx, JSObject *initialVarObj, ExecuteFrameGuard *fg);

    /*
     * Since RAII cannot be used for inline frames, callers must manually
     * call pushInlineFrame/popInlineFrame.
     */
    inline JSStackFrame *getInlineFrame(JSContext *cx, Value *sp, uintN nactual,
                                        JSFunction *fun, JSScript *script,
                                        uint32 *flags) const;
    inline void pushInlineFrame(JSContext *cx, JSScript *script, JSStackFrame *fp,
                                JSFrameRegs *regs);
    inline void popInlineFrame(JSContext *cx, JSStackFrame *prev, js::Value *newsp);

    /* These functions are called inside SendToGenerator. */
    bool getGeneratorFrame(JSContext *cx, uintN vplen, uintN nslots,
                           GeneratorFrameGuard *fg);
    void pushGeneratorFrame(JSContext *cx, JSFrameRegs *regs, GeneratorFrameGuard *fg);

    /* Pushes a JSStackFrame::isDummyFrame. */
    bool pushDummyFrame(JSContext *cx, JSObject &scopeChain, DummyFrameGuard *fg);

    /* Check and bump the given stack limit. */
    inline JSStackFrame *getInlineFrameWithinLimit(JSContext *cx, Value *sp, uintN nactual,
                                                   JSFunction *fun, JSScript *script, uint32 *flags,
                                                   JSStackFrame *base, Value **limit) const;

    /*
     * Compute a stack limit for entering method jit code which allows the
     * method jit to check for end-of-stack and over-recursion with a single
     * comparison. See STACK_QUOTA above.
     */
    inline Value *getStackLimit(JSContext *cx);

    /*
     * Try to bump the given 'limit' by bumping the commit limit. Return false
     * if fully committed or if 'limit' exceeds 'base' + STACK_QUOTA.
     */
    bool bumpCommitAndLimit(JSStackFrame *base, Value *from, uintN nvals, Value **limit) const;

    /*
     * Allocate nvals on the top of the stack, report error on failure.
     * N.B. the caller must ensure |from >= firstUnused()|.
     */
    inline bool ensureSpace(JSContext *maybecx, Value *from, ptrdiff_t nvals) const;
};

JS_STATIC_ASSERT(StackSpace::CAPACITY_VALS % StackSpace::COMMIT_VALS == 0);

/*
 * While |cx->fp|'s pc/sp are available in |cx->regs|, to compute the saved
 * value of pc/sp for any other frame, it is necessary to know about that
 * frame's next-frame. This iterator maintains this information when walking
 * a chain of stack frames starting at |cx->fp|.
 *
 * Usage:
 *   for (FrameRegsIter i(cx); !i.done(); ++i)
 *     ... i.fp() ... i.sp() ... i.pc()
 */
class FrameRegsIter
{
    JSContext         *cx;
    StackSegment      *curseg;
    JSStackFrame      *curfp;
    Value             *cursp;
    jsbytecode        *curpc;

    void initSlow();
    void incSlow(JSStackFrame *fp, JSStackFrame *prev);

  public:
    JS_REQUIRES_STACK inline FrameRegsIter(JSContext *cx);

    bool done() const { return curfp == NULL; }
    inline FrameRegsIter &operator++();

    JSStackFrame *fp() const { return curfp; }
    Value *sp() const { return cursp; }
    jsbytecode *pc() const { return curpc; }
};

/*
 * Utility class for iteration over all active stack frames.
 */
class AllFramesIter
{
public:
    AllFramesIter(JSContext *cx);

    bool done() const { return curfp == NULL; }
    AllFramesIter& operator++();

    JSStackFrame *fp() const { return curfp; }

private:
    StackSegment *curcs;
    JSStackFrame *curfp;
};

} /* namespace js */

#ifdef DEBUG
# define FUNCTION_KIND_METER_LIST(_)                                          \
                        _(allfun), _(heavy), _(nofreeupvar), _(onlyfreevar),  \
                        _(flat), _(badfunarg),                                \
                        _(joinedsetmethod), _(joinedinitmethod),              \
                        _(joinedreplace), _(joinedsort), _(joinedmodulepat),  \
                        _(mreadbarrier), _(mwritebarrier), _(mwslotbarrier),  \
                        _(unjoined), _(indynamicscope)
# define identity(x)    x

struct JSFunctionMeter {
    int32 FUNCTION_KIND_METER_LIST(identity);
};

# undef identity

# define JS_FUNCTION_METER(cx,x) JS_RUNTIME_METER((cx)->runtime, functionMeter.x)
#else
# define JS_FUNCTION_METER(cx,x) ((void)0)
#endif


struct JSPendingProxyOperation {
    JSPendingProxyOperation *next;
    JSObject *object;
};

struct JSThreadData {
#ifdef JS_THREADSAFE
    /* The request depth for this thread. */
    unsigned            requestDepth;
#endif

#ifdef JS_TRACER
    /*
     * During trace execution (or during trace recording or
     * profiling), these fields point to the compartment doing the
     * execution on this thread. At other times, they are NULL.  If a
     * thread tries to execute/record/profile one trace while another
     * is still running, the initial one will abort. Therefore, we
     * only need to track one at a time.
     */
    JSCompartment       *onTraceCompartment;
    JSCompartment       *recordingCompartment;
    JSCompartment       *profilingCompartment;
 #endif

    /*
     * If non-zero, we were been asked to call the operation callback as soon
     * as possible.  If the thread has an active request, this contributes
     * towards rt->interruptCounter.
     */
    volatile int32      interruptFlags;

    /* Keeper of the contiguous stack used by all contexts in this thread. */
    js::StackSpace      stackSpace;

    /*
     * Flag indicating that we are waiving any soft limits on the GC heap
     * because we want allocations to be infallible (except when we hit OOM).
     */
    bool                waiveGCQuota;

    /*
     * The GSN cache is per thread since even multi-cx-per-thread embeddings
     * do not interleave js_GetSrcNote calls.
     */
    JSGSNCache          gsnCache;

    /* Property cache for faster call/get/set invocation. */
    js::PropertyCache   propertyCache;

#ifdef JS_TRACER
    /* Maximum size of the tracer's code cache before we start flushing. */
    uint32              maxCodeCacheBytes;
#endif

    /* State used by dtoa.c. */
    DtoaState           *dtoaState;

    /* Base address of the native stack for the current thread. */
    jsuword             *nativeStackBase;

    /* List of currently pending operations on proxies. */
    JSPendingProxyOperation *pendingProxyOperation;

    js::ConservativeGCThreadData conservativeGC;

    bool init();
    void finish();
    void mark(JSTracer *trc);
    void purge(JSContext *cx);

    /* This must be called with the GC lock held. */
    inline void triggerOperationCallback(JSRuntime *rt);
};

#ifdef JS_THREADSAFE

/*
 * Structure uniquely representing a thread.  It holds thread-private data
 * that can be accessed without a global lock.
 */
struct JSThread {
    typedef js::HashMap<void *,
                        JSThread *,
                        js::DefaultHasher<void *>,
                        js::SystemAllocPolicy> Map;

    /* Linked list of all contexts in use on this thread. */
    JSCList             contextList;

    /* Opaque thread-id, from NSPR's PR_GetCurrentThread(). */
    void                *id;

    /* Number of JS_SuspendRequest calls withot JS_ResumeRequest. */
    unsigned            suspendCount;

# ifdef DEBUG
    unsigned            checkRequestDepth;
# endif

    /* Factored out of JSThread for !JS_THREADSAFE embedding in JSRuntime. */
    JSThreadData        data;
};

#define JS_THREAD_DATA(cx)      (&(cx)->thread->data)

extern JSThread *
js_CurrentThread(JSRuntime *rt);

/*
 * The function takes the GC lock and does not release in successful return.
 * On error (out of memory) the function releases the lock but delegates
 * the error reporting to the caller.
 */
extern JSBool
js_InitContextThread(JSContext *cx);

/*
 * On entrance the GC lock must be held and it will be held on exit.
 */
extern void
js_ClearContextThread(JSContext *cx);

#endif /* JS_THREADSAFE */

typedef enum JSDestroyContextMode {
    JSDCM_NO_GC,
    JSDCM_MAYBE_GC,
    JSDCM_FORCE_GC,
    JSDCM_NEW_FAILED
} JSDestroyContextMode;

typedef enum JSRuntimeState {
    JSRTS_DOWN,
    JSRTS_LAUNCHING,
    JSRTS_UP,
    JSRTS_LANDING
} JSRuntimeState;

typedef struct JSPropertyTreeEntry {
    JSDHashEntryHdr     hdr;
    js::Shape           *child;
} JSPropertyTreeEntry;

typedef void
(* JSActivityCallback)(void *arg, JSBool active);

namespace js {

typedef js::Vector<JSCompartment *, 0, js::SystemAllocPolicy> WrapperVector;

}

struct JSRuntime {
    /* Default compartment. */
    JSCompartment       *atomsCompartment;
#ifdef JS_THREADSAFE
    bool                atomsCompartmentIsLocked;
#endif

    /* List of compartments (protected by the GC lock). */
    js::WrapperVector compartments;

    /* Runtime state, synchronized by the stateChange/gcLock condvar/lock. */
    JSRuntimeState      state;

    /* Context create/destroy callback. */
    JSContextCallback   cxCallback;

    /* Compartment create/destroy callback. */
    JSCompartmentCallback compartmentCallback;

    /*
     * Sets a callback that is run whenever the runtime goes idle - the
     * last active request ceases - and begins activity - when it was
     * idle and a request begins. Note: The callback is called under the
     * GC lock.
     */
    void setActivityCallback(JSActivityCallback cb, void *arg) {
        activityCallback = cb;
        activityCallbackArg = arg;
    }

    JSActivityCallback    activityCallback;
    void                 *activityCallbackArg;

    /*
     * Shape regenerated whenever a prototype implicated by an "add property"
     * property cache fill and induced trace guard has a readonly property or a
     * setter defined on it. This number proxies for the shapes of all objects
     * along the prototype chain of all objects in the runtime on which such an
     * add-property result has been cached/traced.
     *
     * See bug 492355 for more details.
     *
     * This comes early in JSRuntime to minimize the immediate format used by
     * trace-JITted code that reads it.
     */
    uint32              protoHazardShape;

    /* Garbage collector state, used by jsgc.c. */
    js::GCChunkSet      gcChunkSet;

    js::RootedValueMap  gcRootsHash;
    js::GCLocks         gcLocksHash;
    jsrefcount          gcKeepAtoms;
    size_t              gcBytes;
    size_t              gcTriggerBytes;
    size_t              gcLastBytes;
    size_t              gcMaxBytes;
    size_t              gcMaxMallocBytes;
    size_t              gcChunksWaitingToExpire;
    uint32              gcEmptyArenaPoolLifespan;
    uint32              gcNumber;
    js::GCMarker        *gcMarkingTracer;
    uint32              gcTriggerFactor;
    int64               gcJitReleaseTime;
    JSGCMode            gcMode;
    volatile bool       gcIsNeeded;

    /*
     * Compartment that triggered GC. If more than one Compatment need GC,
     * gcTriggerCompartment is reset to NULL and a global GC is performed.
     */
    JSCompartment       *gcTriggerCompartment;

    /* Compartment that is currently involved in per-compartment GC */
    JSCompartment       *gcCurrentCompartment;

    /*
     * We can pack these flags as only the GC thread writes to them. Atomic
     * updates to packed bytes are not guaranteed, so stores issued by one
     * thread may be lost due to unsynchronized read-modify-write cycles on
     * other threads.
     */
    bool                gcPoke;
    bool                gcMarkAndSweep;
    bool                gcRunning;
    bool                gcRegenShapes;

#ifdef JS_GC_ZEAL
    jsrefcount          gcZeal;
#endif

    JSGCCallback        gcCallback;

  private:
    /*
     * Malloc counter to measure memory pressure for GC scheduling. It runs
     * from gcMaxMallocBytes down to zero.
     */
    volatile ptrdiff_t  gcMallocBytes;

  public:
    js::GCChunkAllocator    *gcChunkAllocator;

    void setCustomGCChunkAllocator(js::GCChunkAllocator *allocator) {
        JS_ASSERT(allocator);
        JS_ASSERT(state == JSRTS_DOWN);
        gcChunkAllocator = allocator;
    }

    /*
     * The trace operation and its data argument to trace embedding-specific
     * GC roots.
     */
    JSTraceDataOp       gcExtraRootsTraceOp;
    void                *gcExtraRootsData;

    /* Well-known numbers held for use by this runtime's contexts. */
    js::Value           NaNValue;
    js::Value           negativeInfinityValue;
    js::Value           positiveInfinityValue;

    JSFlatString        *emptyString;

    /* List of active contexts sharing this runtime; protected by gcLock. */
    JSCList             contextList;

    /* Per runtime debug hooks -- see jsprvtd.h and jsdbgapi.h. */
    JSDebugHooks        globalDebugHooks;

    /*
     * Right now, we only support runtime-wide debugging.
     */
    JSBool              debugMode;

#ifdef JS_TRACER
    /* True if any debug hooks not supported by the JIT are enabled. */
    bool debuggerInhibitsJIT() const {
        return (globalDebugHooks.interruptHook ||
                globalDebugHooks.callHook);
    }
#endif

    /* More debugging state, see jsdbgapi.c. */
    JSCList             trapList;
    JSCList             watchPointList;

    /* Client opaque pointers */
    void                *data;

#ifdef JS_THREADSAFE
    /* These combine to interlock the GC and new requests. */
    PRLock              *gcLock;
    PRCondVar           *gcDone;
    PRCondVar           *requestDone;
    uint32              requestCount;
    JSThread            *gcThread;

    js::GCHelperThread  gcHelperThread;

    /* Lock and owning thread pointer for JS_LOCK_RUNTIME. */
    PRLock              *rtLock;
#ifdef DEBUG
    void *              rtLockOwner;
#endif

    /* Used to synchronize down/up state change; protected by gcLock. */
    PRCondVar           *stateChange;

    /*
     * Lock serializing trapList and watchPointList accesses, and count of all
     * mutations to trapList and watchPointList made by debugger threads.  To
     * keep the code simple, we define debuggerMutations for the thread-unsafe
     * case too.
     */
    PRLock              *debuggerLock;

    JSThread::Map       threads;
#endif /* JS_THREADSAFE */
    uint32              debuggerMutations;

    /*
     * Security callbacks set on the runtime are used by each context unless
     * an override is set on the context.
     */
    JSSecurityCallbacks *securityCallbacks;

    /* Structured data callbacks are runtime-wide. */
    const JSStructuredCloneCallbacks *structuredCloneCallbacks;

    /*
     * The propertyRemovals counter is incremented for every JSObject::clear,
     * and for each JSObject::remove method call that frees a slot in the given
     * object. See js_NativeGet and js_NativeSet in jsobj.cpp.
     */
    int32               propertyRemovals;

    /* Script filename table. */
    struct JSHashTable  *scriptFilenameTable;
    JSCList             scriptFilenamePrefixes;
#ifdef JS_THREADSAFE
    PRLock              *scriptFilenameTableLock;
#endif

    /* Number localization, used by jsnum.c */
    const char          *thousandsSeparator;
    const char          *decimalSeparator;
    const char          *numGrouping;

    /*
     * Weak references to lazily-created, well-known XML singletons.
     *
     * NB: Singleton objects must be carefully disconnected from the rest of
     * the object graph usually associated with a JSContext's global object,
     * including the set of standard class objects.  See jsxml.c for details.
     */
    JSObject            *anynameObject;
    JSObject            *functionNamespaceObject;

#ifdef JS_THREADSAFE
    /* Number of threads with active requests and unhandled interrupts. */
    volatile int32      interruptCounter;
#else
    JSThreadData        threadData;

#define JS_THREAD_DATA(cx)      (&(cx)->runtime->threadData)
#endif

    /*
     * Object shape (property cache structural type) identifier generator.
     *
     * Type 0 stands for the empty scope, and must not be regenerated due to
     * uint32 wrap-around. Since js_GenerateShape (in jsinterp.cpp) uses
     * atomic pre-increment, the initial value for the first typed non-empty
     * scope will be 1.
     *
     * If this counter overflows into SHAPE_OVERFLOW_BIT (in jsinterp.h), the
     * cache is disabled, to avoid aliasing two different types. It stays
     * disabled until a triggered GC at some later moment compresses live
     * types, minimizing rt->shapeGen in the process.
     */
    volatile uint32     shapeGen;

    /* Literal table maintained by jsatom.c functions. */
    JSAtomState         atomState;

    /*
     * Various metering fields are defined at the end of JSRuntime. In this
     * way there is no need to recompile all the code that refers to other
     * fields of JSRuntime after enabling the corresponding metering macro.
     */
#ifdef JS_DUMP_ENUM_CACHE_STATS
    int32               nativeEnumProbes;
    int32               nativeEnumMisses;
# define ENUM_CACHE_METER(name)     JS_ATOMIC_INCREMENT(&cx->runtime->name)
#else
# define ENUM_CACHE_METER(name)     ((void) 0)
#endif

#ifdef JS_DUMP_LOOP_STATS
    /* Loop statistics, to trigger trace recording and compiling. */
    JSBasicStats        loopStats;
#endif

#ifdef DEBUG
    /* Function invocation metering. */
    jsrefcount          inlineCalls;
    jsrefcount          nativeCalls;
    jsrefcount          nonInlineCalls;
    jsrefcount          constructs;

    jsrefcount          liveObjectProps;
    jsrefcount          liveObjectPropsPreSweep;

    /*
     * NB: emptyShapes (in JSCompartment) is init'ed iff at least one
     * of these envars is set:
     *
     *  JS_PROPTREE_STATFILE  statistics on the property tree forest
     *  JS_PROPTREE_DUMPFILE  all paths in the property tree forest
     */
    const char          *propTreeStatFilename;
    const char          *propTreeDumpFilename;

    bool meterEmptyShapes() const { return propTreeStatFilename || propTreeDumpFilename; }

    /* String instrumentation. */
    jsrefcount          liveStrings;
    jsrefcount          totalStrings;
    jsrefcount          liveDependentStrings;
    jsrefcount          totalDependentStrings;
    jsrefcount          badUndependStrings;
    double              lengthSum;
    double              lengthSquaredSum;
    double              strdepLengthSum;
    double              strdepLengthSquaredSum;

    /* Script instrumentation. */
    jsrefcount          liveScripts;
    jsrefcount          totalScripts;
    jsrefcount          liveEmptyScripts;
    jsrefcount          totalEmptyScripts;
    jsrefcount          highWaterLiveScripts;
#endif /* DEBUG */

#ifdef JS_SCOPE_DEPTH_METER
    /*
     * Stats on runtime prototype chain lookups and scope chain depths, i.e.,
     * counts of objects traversed on a chain until the wanted id is found.
     */
    JSBasicStats        protoLookupDepthStats;
    JSBasicStats        scopeSearchDepthStats;

    /*
     * Stats on compile-time host environment and lexical scope chain lengths
     * (maximum depths).
     */
    JSBasicStats        hostenvScopeDepthStats;
    JSBasicStats        lexicalScopeDepthStats;
#endif

#ifdef JS_GCMETER
    js::gc::JSGCStats           gcStats;
    js::gc::JSGCArenaStats      globalArenaStats[js::gc::FINALIZE_LIMIT];
#endif

#ifdef DEBUG
    /*
     * If functionMeterFilename, set from an envariable in JSRuntime's ctor, is
     * null, the remaining members in this ifdef'ed group are not initialized.
     */
    const char          *functionMeterFilename;
    JSFunctionMeter     functionMeter;
    char                lastScriptFilename[1024];

    typedef js::HashMap<JSFunction *,
                        int32,
                        js::DefaultHasher<JSFunction *>,
                        js::SystemAllocPolicy> FunctionCountMap;

    FunctionCountMap    methodReadBarrierCountMap;
    FunctionCountMap    unjoinedFunctionCountMap;
#endif

    JSWrapObjectCallback wrapObjectCallback;
    JSPreWrapCallback    preWrapObjectCallback;

#ifdef JS_METHODJIT
    uint32               mjitMemoryUsed;
#endif
    uint32               stringMemoryUsed;

    JSRuntime();
    ~JSRuntime();

    bool init(uint32 maxbytes);

    void setGCTriggerFactor(uint32 factor);
    void setGCLastBytes(size_t lastBytes);

    /*
     * Call the system malloc while checking for GC memory pressure and
     * reporting OOM error when cx is not null.
     */
    void* malloc(size_t bytes, JSContext *cx = NULL) {
        updateMallocCounter(bytes);
        void *p = ::js_malloc(bytes);
        return JS_LIKELY(!!p) ? p : onOutOfMemory(NULL, bytes, cx);
    }

    /*
     * Call the system calloc while checking for GC memory pressure and
     * reporting OOM error when cx is not null.
     */
    void* calloc(size_t bytes, JSContext *cx = NULL) {
        updateMallocCounter(bytes);
        void *p = ::js_calloc(bytes);
        return JS_LIKELY(!!p) ? p : onOutOfMemory(reinterpret_cast<void *>(1), bytes, cx);
    }

    void* realloc(void* p, size_t oldBytes, size_t newBytes, JSContext *cx = NULL) {
        JS_ASSERT(oldBytes < newBytes);
        updateMallocCounter(newBytes - oldBytes);
        void *p2 = ::js_realloc(p, newBytes);
        return JS_LIKELY(!!p2) ? p2 : onOutOfMemory(p, newBytes, cx);
    }

    void* realloc(void* p, size_t bytes, JSContext *cx = NULL) {
        /*
         * For compatibility we do not account for realloc that increases
         * previously allocated memory.
         */
        if (!p)
            updateMallocCounter(bytes);
        void *p2 = ::js_realloc(p, bytes);
        return JS_LIKELY(!!p2) ? p2 : onOutOfMemory(p, bytes, cx);
    }

    void free(void* p) { ::js_free(p); }

    bool isGCMallocLimitReached() const { return gcMallocBytes <= 0; }

    void resetGCMallocBytes() { gcMallocBytes = ptrdiff_t(gcMaxMallocBytes); }

    void setGCMaxMallocBytes(size_t value) {
        /*
         * For compatibility treat any value that exceeds PTRDIFF_T_MAX to
         * mean that value.
         */
        gcMaxMallocBytes = (ptrdiff_t(value) >= 0) ? value : size_t(-1) >> 1;
        resetGCMallocBytes();
    }

    /*
     * Call this after allocating memory held by GC things, to update memory
     * pressure counters or report the OOM error if necessary. If oomError and
     * cx is not null the function also reports OOM error.
     *
     * The function must be called outside the GC lock and in case of OOM error
     * the caller must ensure that no deadlock possible during OOM reporting.
     */
    void updateMallocCounter(size_t nbytes) {
        /* We tolerate any thread races when updating gcMallocBytes. */
        ptrdiff_t newCount = gcMallocBytes - ptrdiff_t(nbytes);
        gcMallocBytes = newCount;
        if (JS_UNLIKELY(newCount <= 0))
            onTooMuchMalloc();
    }

  private:
    /*
     * The function must be called outside the GC lock.
     */
    JS_FRIEND_API(void) onTooMuchMalloc();

    /*
     * This should be called after system malloc/realloc returns NULL to try
     * to recove some memory or to report an error. Failures in malloc and
     * calloc are signaled by p == null and p == reinterpret_cast<void *>(1).
     * Other values of p mean a realloc failure.
     *
     * The function must be called outside the GC lock.
     */
    JS_FRIEND_API(void *) onOutOfMemory(void *p, size_t nbytes, JSContext *cx);
};

/* Common macros to access thread-local caches in JSThread or JSRuntime. */
#define JS_GSN_CACHE(cx)        (JS_THREAD_DATA(cx)->gsnCache)
#define JS_PROPERTY_CACHE(cx)   (JS_THREAD_DATA(cx)->propertyCache)

#ifdef DEBUG
# define JS_RUNTIME_METER(rt, which)    JS_ATOMIC_INCREMENT(&(rt)->which)
# define JS_RUNTIME_UNMETER(rt, which)  JS_ATOMIC_DECREMENT(&(rt)->which)
#else
# define JS_RUNTIME_METER(rt, which)    /* nothing */
# define JS_RUNTIME_UNMETER(rt, which)  /* nothing */
#endif

#define JS_KEEP_ATOMS(rt)   JS_ATOMIC_INCREMENT(&(rt)->gcKeepAtoms);
#define JS_UNKEEP_ATOMS(rt) JS_ATOMIC_DECREMENT(&(rt)->gcKeepAtoms);

#ifdef JS_ARGUMENT_FORMATTER_DEFINED
/*
 * Linked list mapping format strings for JS_{Convert,Push}Arguments{,VA} to
 * formatter functions.  Elements are sorted in non-increasing format string
 * length order.
 */
struct JSArgumentFormatMap {
    const char          *format;
    size_t              length;
    JSArgumentFormatter formatter;
    JSArgumentFormatMap *next;
};
#endif

/*
 * Key and entry types for the JSContext.resolvingTable hash table, typedef'd
 * here because all consumers need to see these declarations (and not just the
 * typedef names, as would be the case for an opaque pointer-to-typedef'd-type
 * declaration), along with cx->resolvingTable.
 */
typedef struct JSResolvingKey {
    JSObject            *obj;
    jsid                id;
} JSResolvingKey;

typedef struct JSResolvingEntry {
    JSDHashEntryHdr     hdr;
    JSResolvingKey      key;
    uint32              flags;
} JSResolvingEntry;

#define JSRESFLAG_LOOKUP        0x1     /* resolving id from lookup */
#define JSRESFLAG_WATCH         0x2     /* resolving id from watch */
#define JSRESOLVE_INFER         0xffff  /* infer bits from current bytecode */

extern const JSDebugHooks js_NullDebugHooks;  /* defined in jsdbgapi.cpp */

namespace js {

class AutoGCRooter;

static inline bool
OptionsHasXML(uint32 options)
{
    return !!(options & JSOPTION_XML);
}

static inline bool
OptionsHasAnonFunFix(uint32 options)
{
    return !!(options & JSOPTION_ANONFUNFIX);
}

static inline bool
OptionsSameVersionFlags(uint32 self, uint32 other)
{
    static const uint32 mask = JSOPTION_XML | JSOPTION_ANONFUNFIX;
    return !((self & mask) ^ (other & mask));
}

/*
 * Flags accompany script version data so that a) dynamically created scripts
 * can inherit their caller's compile-time properties and b) scripts can be
 * appropriately compared in the eval cache across global option changes. An
 * example of the latter is enabling the top-level-anonymous-function-is-error
 * option: subsequent evals of the same, previously-valid script text may have
 * become invalid.
 */
namespace VersionFlags {
static const uintN MASK         = 0x0FFF; /* see JSVersion in jspubtd.h */
static const uintN HAS_XML      = 0x1000; /* flag induced by XML option */
static const uintN ANONFUNFIX   = 0x2000; /* see jsapi.h comment on JSOPTION_ANONFUNFIX */
static const uintN FULL_MASK    = 0x3FFF;
}

static inline JSVersion
VersionNumber(JSVersion version)
{
    return JSVersion(uint32(version) & VersionFlags::MASK);
}

static inline bool
VersionHasXML(JSVersion version)
{
    return !!(version & VersionFlags::HAS_XML);
}

/* @warning This is a distinct condition from having the XML flag set. */
static inline bool
VersionShouldParseXML(JSVersion version)
{
    return VersionHasXML(version) || VersionNumber(version) >= JSVERSION_1_6;
}

static inline bool
VersionHasAnonFunFix(JSVersion version)
{
    return !!(version & VersionFlags::ANONFUNFIX);
}

static inline void
VersionSetXML(JSVersion *version, bool enable)
{
    if (enable)
        *version = JSVersion(uint32(*version) | VersionFlags::HAS_XML);
    else
        *version = JSVersion(uint32(*version) & ~VersionFlags::HAS_XML);
}

static inline void
VersionSetAnonFunFix(JSVersion *version, bool enable)
{
    if (enable)
        *version = JSVersion(uint32(*version) | VersionFlags::ANONFUNFIX);
    else
        *version = JSVersion(uint32(*version) & ~VersionFlags::ANONFUNFIX);
}

static inline JSVersion
VersionExtractFlags(JSVersion version)
{
    return JSVersion(uint32(version) & ~VersionFlags::MASK);
}

static inline void
VersionCopyFlags(JSVersion *version, JSVersion from)
{
    *version = JSVersion(VersionNumber(*version) | VersionExtractFlags(from));
}

static inline bool
VersionHasFlags(JSVersion version)
{
    return !!VersionExtractFlags(version);
}

static inline uintN
VersionFlagsToOptions(JSVersion version)
{
    uintN copts = (VersionHasXML(version) ? JSOPTION_XML : 0) |
                  (VersionHasAnonFunFix(version) ? JSOPTION_ANONFUNFIX : 0);
    JS_ASSERT((copts & JSCOMPILEOPTION_MASK) == copts);
    return copts;
}

static inline JSVersion
OptionFlagsToVersion(uintN options, JSVersion version)
{
    VersionSetXML(&version, OptionsHasXML(options));
    VersionSetAnonFunFix(&version, OptionsHasAnonFunFix(options));
    return version;
}

static inline bool
VersionIsKnown(JSVersion version)
{
    return VersionNumber(version) != JSVERSION_UNKNOWN;
}

typedef js::HashSet<JSObject *,
                    js::DefaultHasher<JSObject *>,
                    js::SystemAllocPolicy> BusyArraysMap;

} /* namespace js */

struct JSContext
{
    explicit JSContext(JSRuntime *rt);

    /* JSRuntime contextList linkage. */
    JSCList             link;

  private:
    /* See JSContext::findVersion. */
    JSVersion           defaultVersion;      /* script compilation version */
    JSVersion           versionOverride;     /* supercedes defaultVersion when valid */
    bool                hasVersionOverride;

    /* Exception state -- the exception member is a GC root by definition. */
    JSBool              throwing;           /* is there a pending exception? */
    js::Value           exception;          /* most-recently-thrown exception */

    /* Per-context run options. */
    uintN               runOptions;            /* see jsapi.h for JSOPTION_* */

  public:
    /* Locale specific callbacks for string conversion. */
    JSLocaleCallbacks   *localeCallbacks;

    /*
     * cx->resolvingTable is non-null and non-empty if we are initializing
     * standard classes lazily, or if we are otherwise recursing indirectly
     * from js_LookupProperty through a Class.resolve hook.  It is used to
     * limit runaway recursion (see jsapi.c and jsobj.c).
     */
    JSDHashTable        *resolvingTable;

    /*
     * True if generating an error, to prevent runaway recursion.
     * NB: generatingError packs with throwing below.
     */
    JSPackedBool        generatingError;

    /* Limit pointer for checking native stack consumption during recursion. */
    jsuword             stackLimit;

    /* Quota on the size of arenas used to compile and execute scripts. */
    size_t              scriptStackQuota;

    /* Data shared by threads in an address space. */
    JSRuntime *const    runtime;

    /* GC heap compartment. */
    JSCompartment       *compartment;

    /* Currently executing frame and regs, set by stack operations. */
    JS_REQUIRES_STACK
    JSFrameRegs         *regs;

    /* Current frame accessors. */

    JSStackFrame* fp() {
        JS_ASSERT(regs && regs->fp);
        return regs->fp;
    }

    JSStackFrame* maybefp() {
        JS_ASSERT_IF(regs, regs->fp);
        return regs ? regs->fp : NULL;
    }

    bool hasfp() {
        JS_ASSERT_IF(regs, regs->fp);
        return !!regs;
    }

  public:
    friend class js::StackSpace;
    friend bool js::Interpret(JSContext *, JSStackFrame *, uintN, JSInterpMode);

    void resetCompartment();
    void wrapPendingException();

    /* For grep-ability, changes to 'regs' should call this function. */
    void setCurrentRegs(JSFrameRegs *regs) {
        JS_ASSERT_IF(regs, regs->fp);
        this->regs = regs;
    }

    /* Temporary arena pool used while compiling and decompiling. */
    JSArenaPool         tempPool;

    /* Temporary arena pool used while evaluate regular expressions. */
    JSArenaPool         regExpPool;

    /* Top-level object and pointer to top stack frame's scope chain. */
    JSObject            *globalObject;

    /* State for object and array toSource conversion. */
    JSSharpObjectMap    sharpObjectMap;
    js::BusyArraysMap   busyArrays;

    /* Argument formatter support for JS_{Convert,Push}Arguments{,VA}. */
    JSArgumentFormatMap *argumentFormatMap;

    /* Last message string and log file for debugging. */
    char                *lastMessage;
#ifdef DEBUG
    void                *logfp;
    jsbytecode          *logPrevPc;
#endif

    /* Per-context optional error reporter. */
    JSErrorReporter     errorReporter;

    /* Branch callback. */
    JSOperationCallback operationCallback;

    /* Interpreter activation count. */
    uintN               interpLevel;

    /* Client opaque pointers. */
    void                *data;
    void                *data2;

  private:
    /* Linked list of segments. See StackSegment. */
    js::StackSegment *currentSegment;

  public:
    void assertSegmentsInSync() const {
#ifdef DEBUG
        if (regs) {
            JS_ASSERT(currentSegment->isActive());
            if (js::StackSegment *prev = currentSegment->getPreviousInContext())
                JS_ASSERT(!prev->isActive());
        } else {
            JS_ASSERT_IF(currentSegment, !currentSegment->isActive());
        }
#endif
    }

    /* Return whether this context has an active segment. */
    bool hasActiveSegment() const {
        assertSegmentsInSync();
        return !!regs;
    }

    /* Assuming there is an active segment, return it. */
    js::StackSegment *activeSegment() const {
        JS_ASSERT(hasActiveSegment());
        return currentSegment;
    }

    /* Return the current segment, which may or may not be active. */
    js::StackSegment *getCurrentSegment() const {
        assertSegmentsInSync();
        return currentSegment;
    }

    inline js::RegExpStatics *regExpStatics();

    /* Add the given segment to the list as the new active segment. */
    void pushSegmentAndFrame(js::StackSegment *newseg, JSFrameRegs &regs);

    /* Remove the active segment and make the next segment active. */
    void popSegmentAndFrame();

    /* Mark the top segment as suspended, without pushing a new one. */
    void saveActiveSegment();

    /* Undoes calls to suspendActiveSegment. */
    void restoreSegment();

    /* Get the frame whose prev() is fp, which may be in any segment. */
    inline JSStackFrame *computeNextFrame(JSStackFrame *fp);

    /*
     * Perform a linear search of all frames in all segments in the given context
     * for the given frame, returning the segment, if found, and null otherwise.
     */
    js::StackSegment *containingSegment(const JSStackFrame *target);

    /* Search the call stack for the nearest frame with static level targetLevel. */
    JSStackFrame *findFrameAtLevel(uintN targetLevel) const {
        JSStackFrame *fp = regs->fp;
        while (true) {
            JS_ASSERT(fp && fp->isScriptFrame());
            if (fp->script()->staticLevel == targetLevel)
                break;
            fp = fp->prev();
        }
        return fp;
    }

  public:
    /*
     * The default script compilation version can be set iff there is no code running.
     * This typically occurs via the JSAPI right after a context is constructed.
     */
    bool canSetDefaultVersion() const {
        return !regs && !hasVersionOverride;
    }

    /* Force a version for future script compilation. */
    void overrideVersion(JSVersion newVersion) {
        JS_ASSERT(!canSetDefaultVersion());
        versionOverride = newVersion;
        hasVersionOverride = true;
    }

    /* Set the default script compilation version. */
    void setDefaultVersion(JSVersion version) {
        defaultVersion = version;
    }

    void clearVersionOverride() { hasVersionOverride = false; }
    JSVersion getDefaultVersion() const { return defaultVersion; }
    bool isVersionOverridden() const { return hasVersionOverride; }

    JSVersion getVersionOverride() const {
        JS_ASSERT(isVersionOverridden());
        return versionOverride;
    }

    /*
     * Set the default version if possible; otherwise, force the version.
     * Return whether an override occurred.
     */
    bool maybeOverrideVersion(JSVersion newVersion) {
        if (canSetDefaultVersion()) {
            setDefaultVersion(newVersion);
            return false;
        }
        overrideVersion(newVersion);
        return true;
    }

  private:
    /*
     * If there is no code currently executing, turn the override version into
     * the default version.
     *
     * NB: the only time the version is potentially capable of migrating is
     * on return from the Execute or ExternalInvoke paths as they call through
     * JSContext::popSegmentAndFrame.
     */
    void maybeMigrateVersionOverride() {
        if (JS_LIKELY(!isVersionOverridden() || currentSegment))
            return;
        defaultVersion = versionOverride;
        clearVersionOverride();
    }

  public:
    /*
     * Return:
     * - The override version, if there is an override version.
     * - The newest scripted frame's version, if there is such a frame.
     * - The default verion.
     *
     * Note: if this ever shows up in a profile, just add caching!
     */
    JSVersion findVersion() const {
        if (hasVersionOverride)
            return versionOverride;

        if (regs) {
            /* There may be a scripted function somewhere on the stack! */
            JSStackFrame *fp = regs->fp;
            while (fp && !fp->isScriptFrame())
                fp = fp->prev();
            if (fp)
                return fp->script()->getVersion();
        }

        return defaultVersion;
    }

    void setRunOptions(uintN ropts) {
        JS_ASSERT((ropts & JSRUNOPTION_MASK) == ropts);
        runOptions = ropts;
    }

    /* Note: may override the version. */
    void setCompileOptions(uintN newcopts) {
        JS_ASSERT((newcopts & JSCOMPILEOPTION_MASK) == newcopts);
        if (JS_LIKELY(getCompileOptions() == newcopts))
            return;
        JSVersion version = findVersion();
        JSVersion newVersion = js::OptionFlagsToVersion(newcopts, version);
        maybeOverrideVersion(newVersion);
    }

    uintN getRunOptions() const { return runOptions; }
    uintN getCompileOptions() const { return js::VersionFlagsToOptions(findVersion()); }
    uintN allOptions() const { return getRunOptions() | getCompileOptions(); }

    bool hasRunOption(uintN ropt) const {
        JS_ASSERT((ropt & JSRUNOPTION_MASK) == ropt);
        return !!(runOptions & ropt);
    }

    bool hasStrictOption() const { return hasRunOption(JSOPTION_STRICT); }
    bool hasWErrorOption() const { return hasRunOption(JSOPTION_WERROR); }
    bool hasAtLineOption() const { return hasRunOption(JSOPTION_ATLINE); }

#ifdef JS_THREADSAFE
    JSThread            *thread;
    unsigned            outstandingRequests;/* number of JS_BeginRequest calls
                                               without the corresponding
                                               JS_EndRequest. */
    JSCList             threadLinks;        /* JSThread contextList linkage */

#define CX_FROM_THREAD_LINKS(tl) \
    ((JSContext *)((char *)(tl) - offsetof(JSContext, threadLinks)))
#endif

    /* Stack of thread-stack-allocated GC roots. */
    js::AutoGCRooter   *autoGCRooters;

    /* Debug hooks associated with the current context. */
    const JSDebugHooks  *debugHooks;

    /* Security callbacks that override any defined on the runtime. */
    JSSecurityCallbacks *securityCallbacks;

    /* Stored here to avoid passing it around as a parameter. */
    uintN               resolveFlags;

    /* Random number generator state, used by jsmath.cpp. */
    int64               rngSeed;

    /* Location to stash the iteration value between JSOP_MOREITER and JSOP_FOR*. */
    js::Value           iterValue;

#ifdef JS_TRACER
    /*
     * True if traces may be executed. Invariant: The value of traceJitenabled
     * is always equal to the expression in updateJITEnabled below.
     *
     * This flag and the fields accessed by updateJITEnabled are written only
     * in runtime->gcLock, to avoid race conditions that would leave the wrong
     * value in traceJitEnabled. (But the interpreter reads this without
     * locking. That can race against another thread setting debug hooks, but
     * we always read cx->debugHooks without locking anyway.)
     */
    bool                 traceJitEnabled;
#endif

#ifdef JS_METHODJIT
    bool                 methodJitEnabled;
    bool                 profilingEnabled;
#endif

    /* Caller must be holding runtime->gcLock. */
    void updateJITEnabled();

#ifdef MOZ_TRACE_JSCALLS
    /* Function entry/exit debugging callback. */
    JSFunctionCallback    functionCallback;

    void doFunctionCallback(const JSFunction *fun,
                            const JSScript *scr,
                            int entering) const
    {
        if (functionCallback)
            functionCallback(fun, scr, this, entering);
    }
#endif

    DSTOffsetCache dstOffsetCache;

    /* List of currently active non-escaping enumerators (for-in). */
    JSObject *enumerators;

  private:
    /*
     * To go from a live generator frame (on the stack) to its generator object
     * (see comment js_FloatingFrameIfGenerator), we maintain a stack of active
     * generators, pushing and popping when entering and leaving generator
     * frames, respectively.
     */
    js::Vector<JSGenerator *, 2, js::SystemAllocPolicy> genStack;

  public:
#ifdef JS_METHODJIT
    inline js::mjit::JaegerCompartment *jaegerCompartment();
#endif

    /* Return the generator object for the given generator frame. */
    JSGenerator *generatorFor(JSStackFrame *fp) const;

    /* Early OOM-check. */
    inline bool ensureGeneratorStackSpace();

    bool enterGenerator(JSGenerator *gen) {
        return genStack.append(gen);
    }

    void leaveGenerator(JSGenerator *gen) {
        JS_ASSERT(genStack.back() == gen);
        genStack.popBack();
    }

#ifdef JS_THREADSAFE
    /*
     * When non-null JSContext::free delegates the job to the background
     * thread.
     */
    js::GCHelperThread *gcBackgroundFree;
#endif

    inline void* malloc(size_t bytes) {
        return runtime->malloc(bytes, this);
    }

    inline void* mallocNoReport(size_t bytes) {
        JS_ASSERT(bytes != 0);
        return runtime->malloc(bytes, NULL);
    }

    inline void* calloc(size_t bytes) {
        JS_ASSERT(bytes != 0);
        return runtime->calloc(bytes, this);
    }

    inline void* realloc(void* p, size_t bytes) {
        return runtime->realloc(p, bytes, this);
    }

    inline void* realloc(void* p, size_t oldBytes, size_t newBytes) {
        return runtime->realloc(p, oldBytes, newBytes, this);
    }

    inline void free(void* p) {
#ifdef JS_THREADSAFE
        if (gcBackgroundFree) {
            gcBackgroundFree->freeLater(p);
            return;
        }
#endif
        runtime->free(p);
    }

    /*
     * In the common case that we'd like to allocate the memory for an object
     * with cx->malloc/free, we cannot use overloaded C++ operators (no
     * placement delete).  Factor the common workaround into one place.
     */
#define CREATE_BODY(parms)                                                    \
    void *memory = this->malloc(sizeof(T));                                   \
    if (!memory)                                                              \
        return NULL;                                                          \
    return new(memory) T parms;

    template <class T>
    JS_ALWAYS_INLINE T *create() {
        CREATE_BODY(())
    }

    template <class T, class P1>
    JS_ALWAYS_INLINE T *create(const P1 &p1) {
        CREATE_BODY((p1))
    }

    template <class T, class P1, class P2>
    JS_ALWAYS_INLINE T *create(const P1 &p1, const P2 &p2) {
        CREATE_BODY((p1, p2))
    }

    template <class T, class P1, class P2, class P3>
    JS_ALWAYS_INLINE T *create(const P1 &p1, const P2 &p2, const P3 &p3) {
        CREATE_BODY((p1, p2, p3))
    }
#undef CREATE_BODY

    template <class T>
    JS_ALWAYS_INLINE void destroy(T *p) {
        p->~T();
        this->free(p);
    }

    void purge();

    js::StackSpace &stack() const {
        return JS_THREAD_DATA(this)->stackSpace;
    }

#ifdef DEBUG
    void assertValidStackDepth(uintN depth) {
        JS_ASSERT(0 <= regs->sp - regs->fp->base());
        JS_ASSERT(depth <= uintptr_t(regs->sp - regs->fp->base()));
    }
#else
    void assertValidStackDepth(uintN /*depth*/) {}
#endif

    bool isExceptionPending() {
        return throwing;
    }

    js::Value getPendingException() {
        JS_ASSERT(throwing);
        return exception;
    }

    void setPendingException(js::Value v);

    void clearPendingException() {
        this->throwing = false;
        this->exception.setUndefined();
    }

  private:
    /*
     * The allocation code calls the function to indicate either OOM failure
     * when p is null or that a memory pressure counter has reached some
     * threshold when p is not null. The function takes the pointer and not
     * a boolean flag to minimize the amount of code in its inlined callers.
     */
    JS_FRIEND_API(void) checkMallocGCPressure(void *p);
}; /* struct JSContext */

#ifdef JS_THREADSAFE
# define JS_THREAD_ID(cx)       ((cx)->thread ? (cx)->thread->id : 0)
#endif

#if defined JS_THREADSAFE && defined DEBUG

namespace js {

class AutoCheckRequestDepth {
    JSContext *cx;
  public:
    AutoCheckRequestDepth(JSContext *cx) : cx(cx) { cx->thread->checkRequestDepth++; }

    ~AutoCheckRequestDepth() {
        JS_ASSERT(cx->thread->checkRequestDepth != 0);
        cx->thread->checkRequestDepth--;
    }
};

}

# define CHECK_REQUEST(cx)                                                    \
    JS_ASSERT((cx)->thread);                                                  \
    JS_ASSERT((cx)->thread->data.requestDepth || (cx)->thread == (cx)->runtime->gcThread); \
    AutoCheckRequestDepth _autoCheckRequestDepth(cx);

#else
# define CHECK_REQUEST(cx)          ((void) 0)
# define CHECK_REQUEST_THREAD(cx)   ((void) 0)
#endif

static inline uintN
FramePCOffset(JSContext *cx, JSStackFrame* fp)
{
    jsbytecode *pc = fp->hasImacropc() ? fp->imacropc() : fp->pc(cx);
    return uintN(pc - fp->script()->code);
}

static inline JSAtom **
FrameAtomBase(JSContext *cx, JSStackFrame *fp)
{
    return fp->hasImacropc()
           ? COMMON_ATOMS_START(&cx->runtime->atomState)
           : fp->script()->atomMap.vector;
}

namespace js {

class AutoGCRooter {
  public:
    AutoGCRooter(JSContext *cx, ptrdiff_t tag)
      : down(cx->autoGCRooters), tag(tag), context(cx)
    {
        JS_ASSERT(this != cx->autoGCRooters);
        CHECK_REQUEST(cx);
        cx->autoGCRooters = this;
    }

    ~AutoGCRooter() {
        JS_ASSERT(this == context->autoGCRooters);
        CHECK_REQUEST(context);
        context->autoGCRooters = down;
    }

    /* Implemented in jsgc.cpp. */
    inline void trace(JSTracer *trc);

#ifdef __GNUC__
# pragma GCC visibility push(default)
#endif
    friend JS_FRIEND_API(void) MarkContext(JSTracer *trc, JSContext *acx);
    friend void MarkRuntime(JSTracer *trc);
#ifdef __GNUC__
# pragma GCC visibility pop
#endif

  protected:
    AutoGCRooter * const down;

    /*
     * Discriminates actual subclass of this being used.  If non-negative, the
     * subclass roots an array of values of the length stored in this field.
     * If negative, meaning is indicated by the corresponding value in the enum
     * below.  Any other negative value indicates some deeper problem such as
     * memory corruption.
     */
    ptrdiff_t tag;

    JSContext * const context;

    enum {
        JSVAL =        -1, /* js::AutoValueRooter */
        SHAPE =        -2, /* js::AutoShapeRooter */
        PARSER =       -3, /* js::Parser */
        SCRIPT =       -4, /* js::AutoScriptRooter */
        ENUMERATOR =   -5, /* js::AutoEnumStateRooter */
        IDARRAY =      -6, /* js::AutoIdArray */
        DESCRIPTORS =  -7, /* js::AutoPropDescArrayRooter */
        NAMESPACES =   -8, /* js::AutoNamespaceArray */
        XML =          -9, /* js::AutoXMLRooter */
        OBJECT =      -10, /* js::AutoObjectRooter */
        ID =          -11, /* js::AutoIdRooter */
        VALVECTOR =   -12, /* js::AutoValueVector */
        DESCRIPTOR =  -13, /* js::AutoPropertyDescriptorRooter */
        STRING =      -14, /* js::AutoStringRooter */
        IDVECTOR =    -15, /* js::AutoIdVector */
        BINDINGS =    -16, /* js::Bindings */
        SHAPEVECTOR = -17  /* js::AutoShapeVector */
    };

    private:
    /* No copy or assignment semantics. */
    AutoGCRooter(AutoGCRooter &ida);
    void operator=(AutoGCRooter &ida);
};

/* FIXME(bug 332648): Move this into a public header. */
class AutoValueRooter : private AutoGCRooter
{
  public:
    explicit AutoValueRooter(JSContext *cx
                             JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, JSVAL), val(js::NullValue())
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    AutoValueRooter(JSContext *cx, const Value &v
                    JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, JSVAL), val(v)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    AutoValueRooter(JSContext *cx, jsval v
                    JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, JSVAL), val(js::Valueify(v))
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    /*
     * If you are looking for Object* overloads, use AutoObjectRooter instead;
     * rooting Object*s as a js::Value requires discerning whether or not it is
     * a function object. Also, AutoObjectRooter is smaller.
     */

    void set(Value v) {
        JS_ASSERT(tag == JSVAL);
        val = v;
    }

    void set(jsval v) {
        JS_ASSERT(tag == JSVAL);
        val = js::Valueify(v);
    }

    const Value &value() const {
        JS_ASSERT(tag == JSVAL);
        return val;
    }

    Value *addr() {
        JS_ASSERT(tag == JSVAL);
        return &val;
    }

    const jsval &jsval_value() const {
        JS_ASSERT(tag == JSVAL);
        return Jsvalify(val);
    }

    jsval *jsval_addr() {
        JS_ASSERT(tag == JSVAL);
        return Jsvalify(&val);
    }

    friend void AutoGCRooter::trace(JSTracer *trc);
    friend void MarkRuntime(JSTracer *trc);

  private:
    Value val;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoObjectRooter : private AutoGCRooter {
  public:
    AutoObjectRooter(JSContext *cx, JSObject *obj = NULL
                     JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, OBJECT), obj(obj)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    void setObject(JSObject *obj) {
        this->obj = obj;
    }

    JSObject * object() const {
        return obj;
    }

    JSObject ** addr() {
        return &obj;
    }

    friend void AutoGCRooter::trace(JSTracer *trc);
    friend void MarkRuntime(JSTracer *trc);

  private:
    JSObject *obj;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoStringRooter : private AutoGCRooter {
  public:
    AutoStringRooter(JSContext *cx, JSString *str = NULL
                     JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, STRING), str(str)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    void setString(JSString *str) {
        this->str = str;
    }

    JSString * string() const {
        return str;
    }

    JSString ** addr() {
        return &str;
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    JSString *str;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoArrayRooter : private AutoGCRooter {
  public:
    AutoArrayRooter(JSContext *cx, size_t len, Value *vec
                    JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, len), array(vec)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_ASSERT(tag >= 0);
    }

    AutoArrayRooter(JSContext *cx, size_t len, jsval *vec
                    JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, len), array(Valueify(vec))
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_ASSERT(tag >= 0);
    }

    void changeLength(size_t newLength) {
        tag = ptrdiff_t(newLength);
        JS_ASSERT(tag >= 0);
    }

    void changeArray(Value *newArray, size_t newLength) {
        changeLength(newLength);
        array = newArray;
    }

    Value *array;

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoShapeRooter : private AutoGCRooter {
  public:
    AutoShapeRooter(JSContext *cx, const js::Shape *shape
                    JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, SHAPE), shape(shape)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    friend void AutoGCRooter::trace(JSTracer *trc);
    friend void MarkRuntime(JSTracer *trc);

  private:
    const js::Shape * const shape;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoScriptRooter : private AutoGCRooter {
  public:
    AutoScriptRooter(JSContext *cx, JSScript *script
                     JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, SCRIPT), script(script)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    void setScript(JSScript *script) {
        this->script = script;
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    JSScript *script;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoIdRooter : private AutoGCRooter
{
  public:
    explicit AutoIdRooter(JSContext *cx, jsid id = INT_TO_JSID(0)
                          JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, ID), id_(id)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    jsid id() {
        return id_;
    }

    jsid * addr() {
        return &id_;
    }

    friend void AutoGCRooter::trace(JSTracer *trc);
    friend void MarkRuntime(JSTracer *trc);

  private:
    jsid id_;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoIdArray : private AutoGCRooter {
  public:
    AutoIdArray(JSContext *cx, JSIdArray *ida JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, IDARRAY), idArray(ida)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }
    ~AutoIdArray() {
        if (idArray)
            JS_DestroyIdArray(context, idArray);
    }
    bool operator!() {
        return idArray == NULL;
    }
    jsid operator[](size_t i) const {
        JS_ASSERT(idArray);
        JS_ASSERT(i < size_t(idArray->length));
        return idArray->vector[i];
    }
    size_t length() const {
         return idArray->length;
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

    JSIdArray *steal() {
        JSIdArray *copy = idArray;
        idArray = NULL;
        return copy;
    }

  protected:
    inline void trace(JSTracer *trc);

  private:
    JSIdArray * idArray;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

    /* No copy or assignment semantics. */
    AutoIdArray(AutoIdArray &ida);
    void operator=(AutoIdArray &ida);
};

/* The auto-root for enumeration object and its state. */
class AutoEnumStateRooter : private AutoGCRooter
{
  public:
    AutoEnumStateRooter(JSContext *cx, JSObject *obj
                        JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, ENUMERATOR), obj(obj), stateValue()
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_ASSERT(obj);
    }

    ~AutoEnumStateRooter() {
        if (!stateValue.isNull()) {
#ifdef DEBUG
            JSBool ok =
#endif
            obj->enumerate(context, JSENUMERATE_DESTROY, &stateValue, 0);
            JS_ASSERT(ok);
        }
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

    const Value &state() const { return stateValue; }
    Value *addr() { return &stateValue; }

  protected:
    void trace(JSTracer *trc);

    JSObject * const obj;

  private:
    Value stateValue;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

#ifdef JS_HAS_XML_SUPPORT
class AutoXMLRooter : private AutoGCRooter {
  public:
    AutoXMLRooter(JSContext *cx, JSXML *xml
                  JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, XML), xml(xml)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_ASSERT(xml);
    }

    friend void AutoGCRooter::trace(JSTracer *trc);
    friend void MarkRuntime(JSTracer *trc);

  private:
    JSXML * const xml;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};
#endif /* JS_HAS_XML_SUPPORT */

class AutoBindingsRooter : private AutoGCRooter {
  public:
    AutoBindingsRooter(JSContext *cx, Bindings &bindings
                       JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : AutoGCRooter(cx, BINDINGS), bindings(bindings)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    Bindings &bindings;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoLockGC {
  public:
    explicit AutoLockGC(JSRuntime *rt
                        JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : rt(rt)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_LOCK_GC(rt);
    }
    ~AutoLockGC() { JS_UNLOCK_GC(rt); }

  private:
    JSRuntime *rt;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoUnlockGC {
  private:
    JSRuntime *rt;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    explicit AutoUnlockGC(JSRuntime *rt
                          JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : rt(rt)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_UNLOCK_GC(rt);
    }
    ~AutoUnlockGC() { JS_LOCK_GC(rt); }
};

class AutoLockAtomsCompartment {
  private:
    JSContext *cx;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    AutoLockAtomsCompartment(JSContext *cx
                               JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : cx(cx)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_LOCK(cx, &cx->runtime->atomState.lock);
#ifdef JS_THREADSAFE
        cx->runtime->atomsCompartmentIsLocked = true;
#endif
    }
    ~AutoLockAtomsCompartment() {
#ifdef JS_THREADSAFE
        cx->runtime->atomsCompartmentIsLocked = false;
#endif
        JS_UNLOCK(cx, &cx->runtime->atomState.lock);
    }
};

class AutoUnlockAtomsCompartment {
    JSContext *cx;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    AutoUnlockAtomsCompartment(JSContext *cx
                                 JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : cx(cx)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
#ifdef JS_THREADSAFE
        cx->runtime->atomsCompartmentIsLocked = false;
#endif
        JS_UNLOCK(cx, &cx->runtime->atomState.lock);
    }
    ~AutoUnlockAtomsCompartment() {
        JS_LOCK(cx, &cx->runtime->atomState.lock);
#ifdef JS_THREADSAFE
        cx->runtime->atomsCompartmentIsLocked = true;
#endif
    }
};

class AutoKeepAtoms {
    JSRuntime *rt;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    explicit AutoKeepAtoms(JSRuntime *rt
                           JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : rt(rt)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        JS_KEEP_ATOMS(rt);
    }
    ~AutoKeepAtoms() { JS_UNKEEP_ATOMS(rt); }
};

class AutoArenaAllocator {
    JSArenaPool *pool;
    void        *mark;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    explicit AutoArenaAllocator(JSArenaPool *pool
                                JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : pool(pool), mark(JS_ARENA_MARK(pool))
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }
    ~AutoArenaAllocator() { JS_ARENA_RELEASE(pool, mark); }

    template <typename T>
    T *alloc(size_t elems) {
        void *ptr;
        JS_ARENA_ALLOCATE(ptr, pool, elems * sizeof(T));
        return static_cast<T *>(ptr);
    }
};

class AutoReleasePtr {
    JSContext   *cx;
    void        *ptr;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

    AutoReleasePtr operator=(const AutoReleasePtr &other);

  public:
    explicit AutoReleasePtr(JSContext *cx, void *ptr
                            JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : cx(cx), ptr(ptr)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }
    ~AutoReleasePtr() { cx->free(ptr); }
};

/*
 * FIXME: bug 602774: cleaner API for AutoReleaseNullablePtr
 */
class AutoReleaseNullablePtr {
    JSContext   *cx;
    void        *ptr;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

    AutoReleaseNullablePtr operator=(const AutoReleaseNullablePtr &other);

  public:
    explicit AutoReleaseNullablePtr(JSContext *cx, void *ptr
                                    JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : cx(cx), ptr(ptr)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }
    void reset(void *ptr2) {
        if (ptr)
            cx->free(ptr);
        ptr = ptr2;
    }
    ~AutoReleaseNullablePtr() { if (ptr) cx->free(ptr); }
};

class AutoLocalNameArray {
  public:
    explicit AutoLocalNameArray(JSContext *cx, JSFunction *fun
                                JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : context(cx),
        mark(JS_ARENA_MARK(&cx->tempPool)),
        names(fun->script()->bindings.getLocalNameArray(cx, &cx->tempPool)),
        count(fun->script()->bindings.countLocalNames())
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    ~AutoLocalNameArray() {
        JS_ARENA_RELEASE(&context->tempPool, mark);
    }

    operator bool() const { return !!names; }

    uint32 length() const { return count; }

    const jsuword &operator [](unsigned i) const { return names[i]; }

  private:
    JSContext   *context;
    void        *mark;
    jsuword     *names;
    uint32      count;

    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

template <class RefCountable>
class AlreadyIncRefed
{
    typedef RefCountable *****ConvertibleToBool;

    RefCountable *obj;

  public:
    explicit AlreadyIncRefed(RefCountable *obj) : obj(obj) {}

    bool null() const { return obj == NULL; }
    operator ConvertibleToBool() const { return (ConvertibleToBool)obj; }

    RefCountable *operator->() const { JS_ASSERT(!null()); return obj; }
    RefCountable &operator*() const { JS_ASSERT(!null()); return *obj; }
    RefCountable *get() const { return obj; }
};

template <class RefCountable>
class NeedsIncRef
{
    typedef RefCountable *****ConvertibleToBool;

    RefCountable *obj;

  public:
    explicit NeedsIncRef(RefCountable *obj) : obj(obj) {}

    bool null() const { return obj == NULL; }
    operator ConvertibleToBool() const { return (ConvertibleToBool)obj; }

    RefCountable *operator->() const { JS_ASSERT(!null()); return obj; }
    RefCountable &operator*() const { JS_ASSERT(!null()); return *obj; }
    RefCountable *get() const { return obj; }
};

template <class RefCountable>
class AutoRefCount
{
    typedef RefCountable *****ConvertibleToBool;

    JSContext *const cx;
    RefCountable *obj;

    AutoRefCount(const AutoRefCount &);
    void operator=(const AutoRefCount &);

  public:
    explicit AutoRefCount(JSContext *cx)
      : cx(cx), obj(NULL)
    {}

    AutoRefCount(JSContext *cx, NeedsIncRef<RefCountable> aobj)
      : cx(cx), obj(aobj.get())
    {
        if (obj)
            obj->incref(cx);
    }

    AutoRefCount(JSContext *cx, AlreadyIncRefed<RefCountable> aobj)
      : cx(cx), obj(aobj.get())
    {}

    ~AutoRefCount() {
        if (obj)
            obj->decref(cx);
    }

    void reset(NeedsIncRef<RefCountable> aobj) {
        if (obj)
            obj->decref(cx);
        obj = aobj.get();
        if (obj)
            obj->incref(cx);
    }

    void reset(AlreadyIncRefed<RefCountable> aobj) {
        if (obj)
            obj->decref(cx);
        obj = aobj.get();
    }

    bool null() const { return obj == NULL; }
    operator ConvertibleToBool() const { return (ConvertibleToBool)obj; }

    RefCountable *operator->() const { JS_ASSERT(!null()); return obj; }
    RefCountable &operator*() const { JS_ASSERT(!null()); return *obj; }
    RefCountable *get() const { return obj; }
};

} /* namespace js */

class JSAutoResolveFlags
{
  public:
    JSAutoResolveFlags(JSContext *cx, uintN flags
                       JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : mContext(cx), mSaved(cx->resolveFlags)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
        cx->resolveFlags = flags;
    }

    ~JSAutoResolveFlags() { mContext->resolveFlags = mSaved; }

  private:
    JSContext *mContext;
    uintN mSaved;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

extern JSThreadData *
js_CurrentThreadData(JSRuntime *rt);

extern JSBool
js_InitThreads(JSRuntime *rt);

extern void
js_FinishThreads(JSRuntime *rt);

extern void
js_PurgeThreads(JSContext *cx);

namespace js {

#ifdef JS_THREADSAFE

/* Iterator over JSThreadData from all JSThread instances. */
class ThreadDataIter : public JSThread::Map::Range
{
  public:
    ThreadDataIter(JSRuntime *rt) : JSThread::Map::Range(rt->threads.all()) {}

    JSThreadData *threadData() const {
        return &front().value->data;
    }
};

#else /* !JS_THREADSAFE */

class ThreadDataIter
{
    JSRuntime *runtime;
    bool done;
  public:
    ThreadDataIter(JSRuntime *rt) : runtime(rt), done(false) {}

    bool empty() const {
        return done;
    }

    void popFront() {
        JS_ASSERT(!done);
        done = true;
    }

    JSThreadData *threadData() const {
        JS_ASSERT(!done);
        return &runtime->threadData;
    }
};

#endif  /* !JS_THREADSAFE */

} /* namespace js */

/*
 * Create and destroy functions for JSContext, which is manually allocated
 * and exclusively owned.
 */
extern JSContext *
js_NewContext(JSRuntime *rt, size_t stackChunkSize);

extern void
js_DestroyContext(JSContext *cx, JSDestroyContextMode mode);

static JS_INLINE JSContext *
js_ContextFromLinkField(JSCList *link)
{
    JS_ASSERT(link);
    return (JSContext *) ((uint8 *) link - offsetof(JSContext, link));
}

/*
 * If unlocked, acquire and release rt->gcLock around *iterp update; otherwise
 * the caller must be holding rt->gcLock.
 */
extern JSContext *
js_ContextIterator(JSRuntime *rt, JSBool unlocked, JSContext **iterp);

/*
 * Iterate through contexts with active requests. The caller must be holding
 * rt->gcLock in case of a thread-safe build, or otherwise guarantee that the
 * context list is not alternated asynchroniously.
 */
extern JS_FRIEND_API(JSContext *)
js_NextActiveContext(JSRuntime *, JSContext *);

/*
 * Class.resolve and watchpoint recursion damping machinery.
 */
extern JSBool
js_StartResolving(JSContext *cx, JSResolvingKey *key, uint32 flag,
                  JSResolvingEntry **entryp);

extern void
js_StopResolving(JSContext *cx, JSResolvingKey *key, uint32 flag,
                 JSResolvingEntry *entry, uint32 generation);

/*
 * Report an exception, which is currently realized as a printf-style format
 * string and its arguments.
 */
typedef enum JSErrNum {
#define MSG_DEF(name, number, count, exception, format) \
    name = number,
#include "js.msg"
#undef MSG_DEF
    JSErr_Limit
} JSErrNum;

extern JS_FRIEND_API(const JSErrorFormatString *)
js_GetErrorMessage(void *userRef, const char *locale, const uintN errorNumber);

#ifdef va_start
extern JSBool
js_ReportErrorVA(JSContext *cx, uintN flags, const char *format, va_list ap);

extern JSBool
js_ReportErrorNumberVA(JSContext *cx, uintN flags, JSErrorCallback callback,
                       void *userRef, const uintN errorNumber,
                       JSBool charArgs, va_list ap);

extern JSBool
js_ExpandErrorArguments(JSContext *cx, JSErrorCallback callback,
                        void *userRef, const uintN errorNumber,
                        char **message, JSErrorReport *reportp,
                        bool charArgs, va_list ap);
#endif

extern void
js_ReportOutOfMemory(JSContext *cx);

/*
 * Report that cx->scriptStackQuota is exhausted.
 */
void
js_ReportOutOfScriptQuota(JSContext *cx);

extern JS_FRIEND_API(void)
js_ReportOverRecursed(JSContext *cx);

extern JS_FRIEND_API(void)
js_ReportAllocationOverflow(JSContext *cx);

#define JS_CHECK_RECURSION(cx, onerror)                                       \
    JS_BEGIN_MACRO                                                            \
        int stackDummy_;                                                      \
                                                                              \
        if (!JS_CHECK_STACK_SIZE(cx->stackLimit, &stackDummy_)) {             \
            js_ReportOverRecursed(cx);                                        \
            onerror;                                                          \
        }                                                                     \
    JS_END_MACRO

/*
 * Report an exception using a previously composed JSErrorReport.
 * XXXbe remove from "friend" API
 */
extern JS_FRIEND_API(void)
js_ReportErrorAgain(JSContext *cx, const char *message, JSErrorReport *report);

extern void
js_ReportIsNotDefined(JSContext *cx, const char *name);

/*
 * Report an attempt to access the property of a null or undefined value (v).
 */
extern JSBool
js_ReportIsNullOrUndefined(JSContext *cx, intN spindex, const js::Value &v,
                           JSString *fallback);

extern void
js_ReportMissingArg(JSContext *cx, const js::Value &v, uintN arg);

/*
 * Report error using js_DecompileValueGenerator(cx, spindex, v, fallback) as
 * the first argument for the error message. If the error message has less
 * then 3 arguments, use null for arg1 or arg2.
 */
extern JSBool
js_ReportValueErrorFlags(JSContext *cx, uintN flags, const uintN errorNumber,
                         intN spindex, const js::Value &v, JSString *fallback,
                         const char *arg1, const char *arg2);

#define js_ReportValueError(cx,errorNumber,spindex,v,fallback)                \
    ((void)js_ReportValueErrorFlags(cx, JSREPORT_ERROR, errorNumber,          \
                                    spindex, v, fallback, NULL, NULL))

#define js_ReportValueError2(cx,errorNumber,spindex,v,fallback,arg1)          \
    ((void)js_ReportValueErrorFlags(cx, JSREPORT_ERROR, errorNumber,          \
                                    spindex, v, fallback, arg1, NULL))

#define js_ReportValueError3(cx,errorNumber,spindex,v,fallback,arg1,arg2)     \
    ((void)js_ReportValueErrorFlags(cx, JSREPORT_ERROR, errorNumber,          \
                                    spindex, v, fallback, arg1, arg2))

extern JSErrorFormatString js_ErrorFormatString[JSErr_Limit];

#ifdef JS_THREADSAFE
# define JS_ASSERT_REQUEST_DEPTH(cx)  (JS_ASSERT((cx)->thread),               \
                                       JS_ASSERT((cx)->thread->data.requestDepth >= 1))
#else
# define JS_ASSERT_REQUEST_DEPTH(cx)  ((void) 0)
#endif

/*
 * If the operation callback flag was set, call the operation callback.
 * This macro can run the full GC. Return true if it is OK to continue and
 * false otherwise.
 */
#define JS_CHECK_OPERATION_LIMIT(cx)                                          \
    (JS_ASSERT_REQUEST_DEPTH(cx),                                             \
     (!JS_THREAD_DATA(cx)->interruptFlags || js_InvokeOperationCallback(cx)))

JS_ALWAYS_INLINE void
JSThreadData::triggerOperationCallback(JSRuntime *rt)
{
    /*
     * Use JS_ATOMIC_SET and JS_ATOMIC_INCREMENT in the hope that it ensures
     * the write will become immediately visible to other processors polling
     * the flag.  Note that we only care about visibility here, not read/write
     * ordering: this field can only be written with the GC lock held.
     */
    if (interruptFlags)
        return;
    JS_ATOMIC_SET(&interruptFlags, 1);

#ifdef JS_THREADSAFE
    /* rt->interruptCounter does not reflect suspended threads. */
    if (requestDepth != 0)
        JS_ATOMIC_INCREMENT(&rt->interruptCounter);
#endif
}

/*
 * Invoke the operation callback and return false if the current execution
 * is to be terminated.
 */
extern JSBool
js_InvokeOperationCallback(JSContext *cx);

extern JSBool
js_HandleExecutionInterrupt(JSContext *cx);

namespace js {

/* These must be called with GC lock taken. */

JS_FRIEND_API(void)
TriggerOperationCallback(JSContext *cx);

void
TriggerAllOperationCallbacks(JSRuntime *rt);

} /* namespace js */

extern JSStackFrame *
js_GetScriptedCaller(JSContext *cx, JSStackFrame *fp);

extern jsbytecode*
js_GetCurrentBytecodePC(JSContext* cx);

extern bool
js_CurrentPCIsInImacro(JSContext *cx);

namespace js {

class RegExpStatics;

extern JS_FORCES_STACK JS_FRIEND_API(void)
LeaveTrace(JSContext *cx);

} /* namespace js */

/*
 * Get the current frame, first lazily instantiating stack frames if needed.
 * (Do not access cx->fp() directly except in JS_REQUIRES_STACK code.)
 *
 * Defined in jstracer.cpp if JS_TRACER is defined.
 */
static JS_FORCES_STACK JS_INLINE JSStackFrame *
js_GetTopStackFrame(JSContext *cx)
{
    js::LeaveTrace(cx);
    return cx->maybefp();
}

static JS_INLINE JSBool
js_IsPropertyCacheDisabled(JSContext *cx)
{
    return cx->runtime->shapeGen >= js::SHAPE_OVERFLOW_BIT;
}

static JS_INLINE uint32
js_RegenerateShapeForGC(JSRuntime *rt)
{
    JS_ASSERT(rt->gcRunning);
    JS_ASSERT(rt->gcRegenShapes);

    /*
     * Under the GC, compared with js_GenerateShape, we don't need to use
     * atomic increments but we still must make sure that after an overflow
     * the shape stays such.
     */
    uint32 shape = rt->shapeGen;
    shape = (shape + 1) | (shape & js::SHAPE_OVERFLOW_BIT);
    rt->shapeGen = shape;
    return shape;
}

namespace js {

inline void *
ContextAllocPolicy::malloc(size_t bytes)
{
    return cx->malloc(bytes);
}

inline void
ContextAllocPolicy::free(void *p)
{
    cx->free(p);
}

inline void *
ContextAllocPolicy::realloc(void *p, size_t bytes)
{
    return cx->realloc(p, bytes);
}

inline void
ContextAllocPolicy::reportAllocOverflow() const
{
    js_ReportAllocationOverflow(cx);
}

template<class T>
class AutoVectorRooter : protected AutoGCRooter
{
  public:
    explicit AutoVectorRooter(JSContext *cx, ptrdiff_t tag
                              JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoGCRooter(cx, tag), vector(cx)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    size_t length() const { return vector.length(); }

    bool append(const T &v) { return vector.append(v); }

    void popBack() { vector.popBack(); }

    bool growBy(size_t inc) {
        size_t oldLength = vector.length();
        if (!vector.growByUninitialized(inc))
            return false;
        MakeRangeGCSafe(vector.begin() + oldLength, vector.end());
        return true;
    }

    bool resize(size_t newLength) {
        size_t oldLength = vector.length();
        if (newLength <= oldLength) {
            vector.shrinkBy(oldLength - newLength);
            return true;
        }
        if (!vector.growByUninitialized(newLength - oldLength))
            return false;
        MakeRangeGCSafe(vector.begin() + oldLength, vector.end());
        return true;
    }

    bool reserve(size_t newLength) {
        return vector.reserve(newLength);
    }

    T &operator[](size_t i) { return vector[i]; }
    const T &operator[](size_t i) const { return vector[i]; }

    const T *begin() const { return vector.begin(); }
    T *begin() { return vector.begin(); }

    const T *end() const { return vector.end(); }
    T *end() { return vector.end(); }

    const T &back() const { return vector.back(); }

    friend void AutoGCRooter::trace(JSTracer *trc);

  private:
    Vector<T, 8> vector;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoValueVector : public AutoVectorRooter<Value>
{
  public:
    explicit AutoValueVector(JSContext *cx
                             JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<Value>(cx, VALVECTOR)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    const jsval *jsval_begin() const { return Jsvalify(begin()); }
    jsval *jsval_begin() { return Jsvalify(begin()); }

    const jsval *jsval_end() const { return Jsvalify(end()); }
    jsval *jsval_end() { return Jsvalify(end()); }

    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoIdVector : public AutoVectorRooter<jsid>
{
  public:
    explicit AutoIdVector(JSContext *cx
                          JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<jsid>(cx, IDVECTOR)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

class AutoShapeVector : public AutoVectorRooter<const Shape *>
{
  public:
    explicit AutoShapeVector(JSContext *cx
                             JS_GUARD_OBJECT_NOTIFIER_PARAM)
        : AutoVectorRooter<const Shape *>(cx, SHAPEVECTOR)
    {
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    JS_DECL_USE_GUARD_OBJECT_NOTIFIER
};

JSIdArray *
NewIdArray(JSContext *cx, jsint length);

} /* namespace js */

#ifdef _MSC_VER
#pragma warning(pop)
#pragma warning(pop)
#endif

#ifdef JS_CNTXT_UNDEFD_MOZALLOC_WRAPPERS
#  include "mozilla/mozalloc_macro_wrappers.h"
#endif

#endif /* jscntxt_h___ */
