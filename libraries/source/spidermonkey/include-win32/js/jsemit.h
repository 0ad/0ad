/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sw=4 et tw=79:
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

#ifndef jsemit_h___
#define jsemit_h___
/*
 * JS bytecode generation.
 */
#include "jstypes.h"
#include "jsatom.h"
#include "jsopcode.h"
#include "jsparse.h"
#include "jsscript.h"
#include "jsprvtd.h"
#include "jspubtd.h"

JS_BEGIN_EXTERN_C

/*
 * NB: If you add enumerators for scope statements, add them between STMT_WITH
 * and STMT_CATCH, or you will break the STMT_TYPE_IS_SCOPE macro. If you add
 * non-looping statement enumerators, add them before STMT_DO_LOOP or you will
 * break the STMT_TYPE_IS_LOOP macro.
 *
 * Also remember to keep the statementName array in jsemit.c in sync.
 */
typedef enum JSStmtType {
    STMT_LABEL,                 /* labeled statement:  L: s */
    STMT_IF,                    /* if (then) statement */
    STMT_ELSE,                  /* else clause of if statement */
    STMT_SEQ,                   /* synthetic sequence of statements */
    STMT_BLOCK,                 /* compound statement: { s1[;... sN] } */
    STMT_SWITCH,                /* switch statement */
    STMT_WITH,                  /* with statement */
    STMT_CATCH,                 /* catch block */
    STMT_TRY,                   /* try block */
    STMT_FINALLY,               /* finally block */
    STMT_SUBROUTINE,            /* gosub-target subroutine body */
    STMT_DO_LOOP,               /* do/while loop statement */
    STMT_FOR_LOOP,              /* for loop statement */
    STMT_FOR_IN_LOOP,           /* for/in loop statement */
    STMT_WHILE_LOOP,            /* while loop statement */
    STMT_LIMIT
} JSStmtType;

#define STMT_TYPE_IN_RANGE(t,b,e) ((uint)((t) - (b)) <= (uintN)((e) - (b)))

/*
 * A comment on the encoding of the JSStmtType enum and type-testing macros:
 *
 * STMT_TYPE_MAYBE_SCOPE tells whether a statement type is always, or may
 * become, a lexical scope.  It therefore includes block and switch (the two
 * low-numbered "maybe" scope types) and excludes with (with has dynamic scope
 * pending the "reformed with" in ES4/JS2).  It includes all try-catch-finally
 * types, which are high-numbered maybe-scope types.
 *
 * STMT_TYPE_LINKS_SCOPE tells whether a JSStmtInfo of the given type eagerly
 * links to other scoping statement info records.  It excludes the two early
 * "maybe" types, block and switch, as well as the try and both finally types,
 * since try and the other trailing maybe-scope types don't need block scope
 * unless they contain let declarations.
 *
 * We treat WITH as a static scope because it prevents lexical binding from
 * continuing further up the static scope chain. With the lost "reformed with"
 * proposal for ES4, we would be able to model it statically, too.
 */
#define STMT_TYPE_MAYBE_SCOPE(type)                                           \
    (type != STMT_WITH &&                                                     \
     STMT_TYPE_IN_RANGE(type, STMT_BLOCK, STMT_SUBROUTINE))

#define STMT_TYPE_LINKS_SCOPE(type)                                           \
    STMT_TYPE_IN_RANGE(type, STMT_WITH, STMT_CATCH)

#define STMT_TYPE_IS_TRYING(type)                                             \
    STMT_TYPE_IN_RANGE(type, STMT_TRY, STMT_SUBROUTINE)

#define STMT_TYPE_IS_LOOP(type) ((type) >= STMT_DO_LOOP)

#define STMT_MAYBE_SCOPE(stmt)  STMT_TYPE_MAYBE_SCOPE((stmt)->type)
#define STMT_LINKS_SCOPE(stmt)  (STMT_TYPE_LINKS_SCOPE((stmt)->type) ||       \
                                 ((stmt)->flags & SIF_SCOPE))
#define STMT_IS_TRYING(stmt)    STMT_TYPE_IS_TRYING((stmt)->type)
#define STMT_IS_LOOP(stmt)      STMT_TYPE_IS_LOOP((stmt)->type)

typedef struct JSStmtInfo JSStmtInfo;

struct JSStmtInfo {
    uint16          type;           /* statement type */
    uint16          flags;          /* flags, see below */
    uint32          blockid;        /* for simplified dominance computation */
    ptrdiff_t       update;         /* loop update offset (top if none) */
    ptrdiff_t       breaks;         /* offset of last break in loop */
    ptrdiff_t       continues;      /* offset of last continue in loop */
    union {
        JSAtom      *label;         /* name of LABEL */
        JSObjectBox *blockBox;      /* block scope object */
    };
    JSStmtInfo      *down;          /* info for enclosing statement */
    JSStmtInfo      *downScope;     /* next enclosing lexical scope */
};

#define SIF_SCOPE        0x0001     /* statement has its own lexical scope */
#define SIF_BODY_BLOCK   0x0002     /* STMT_BLOCK type is a function body */
#define SIF_FOR_BLOCK    0x0004     /* for (let ...) induced block scope */

/*
 * To reuse space in JSStmtInfo, rename breaks and continues for use during
 * try/catch/finally code generation and backpatching. To match most common
 * use cases, the macro argument is a struct, not a struct pointer. Only a
 * loop, switch, or label statement info record can have breaks and continues,
 * and only a for loop has an update backpatch chain, so it's safe to overlay
 * these for the "trying" JSStmtTypes.
 */
#define CATCHNOTE(stmt)  ((stmt).update)
#define GOSUBS(stmt)     ((stmt).breaks)
#define GUARDJUMP(stmt)  ((stmt).continues)

#define SET_STATEMENT_TOP(stmt, top)                                          \
    ((stmt)->update = (top), (stmt)->breaks = (stmt)->continues = (-1))

#ifdef JS_SCOPE_DEPTH_METER
# define JS_SCOPE_DEPTH_METERING(code) ((void) (code))
# define JS_SCOPE_DEPTH_METERING_IF(cond, code) ((cond) ? (void) (code) : (void) 0)
#else
# define JS_SCOPE_DEPTH_METERING(code) ((void) 0)
# define JS_SCOPE_DEPTH_METERING_IF(code, x) ((void) 0)
#endif

#define TCF_COMPILING           0x01 /* JSTreeContext is JSCodeGenerator */
#define TCF_IN_FUNCTION         0x02 /* parsing inside function body */
#define TCF_RETURN_EXPR         0x04 /* function has 'return expr;' */
#define TCF_RETURN_VOID         0x08 /* function has 'return;' */
#define TCF_IN_FOR_INIT         0x10 /* parsing init expr of for; exclude 'in' */
#define TCF_FUN_SETS_OUTER_NAME 0x20 /* function set outer name (lexical or free) */
#define TCF_FUN_PARAM_ARGUMENTS 0x40 /* function has parameter named arguments */
#define TCF_FUN_USES_ARGUMENTS  0x80 /* function uses arguments except as a
                                        parameter name */
#define TCF_FUN_HEAVYWEIGHT    0x100 /* function needs Call object per call */
#define TCF_FUN_IS_GENERATOR   0x200 /* parsed yield statement in function */
#define TCF_FUN_USES_OWN_NAME  0x400 /* named function expression that uses its
                                        own name */
#define TCF_HAS_FUNCTION_STMT  0x800 /* block contains a function statement */
#define TCF_GENEXP_LAMBDA     0x1000 /* flag lambda from generator expression */
#define TCF_COMPILE_N_GO      0x2000 /* compile-and-go mode of script, can
                                        optimize name references based on scope
                                        chain */
#define TCF_NO_SCRIPT_RVAL    0x4000 /* API caller does not want result value
                                        from global script */
#define TCF_HAS_SHARPS        0x8000 /* source contains sharp defs or uses */

/*
 * Set when parsing a declaration-like destructuring pattern.  This
 * flag causes PrimaryExpr to create PN_NAME parse nodes for variable
 * references which are not hooked into any definition's use chain,
 * added to any tree context's AtomList, etc. etc.  CheckDestructuring
 * will do that work later.
 *
 * The comments atop CheckDestructuring explain the distinction
 * between assignment-like and declaration-like destructuring
 * patterns, and why they need to be treated differently.
 */
#define TCF_DECL_DESTRUCTURING  0x10000

/*
 * A request flag passed to Compiler::compileScript and then down via
 * JSCodeGenerator to js_NewScriptFromCG, from script_compile_sub and any
 * kindred functions that need to make mutable scripts (even empty ones;
 * i.e., they can't share the const JSScript::emptyScript() singleton).
 */
#define TCF_NEED_MUTABLE_SCRIPT 0x20000

/*
 * This function/global/eval code body contained a Use Strict Directive. Treat
 * certain strict warnings as errors, and forbid the use of 'with'. See also
 * TSF_STRICT_MODE_CODE, JSScript::strictModeCode, and JSREPORT_STRICT_ERROR.
 */
#define TCF_STRICT_MODE_CODE    0x40000

/* bit 0x80000 is unused */

/*
 * Flag signifying that the current function seems to be a constructor that
 * sets this.foo to define "methods", at least one of which can't be a null
 * closure, so we should avoid over-specializing property cache entries and
 * trace inlining guards to method function object identity, which will vary
 * per instance.
 */
#define TCF_FUN_UNBRAND_THIS   0x100000

/*
 * "Module pattern", i.e., a lambda that is immediately applied and the whole
 * of an expression statement.
 */
#define TCF_FUN_MODULE_PATTERN 0x200000

/*
 * Flag to prevent a non-escaping function from being optimized into a null
 * closure (i.e., a closure that needs only its global object for free variable
 * resolution), because this function contains a closure that needs one or more
 * scope objects surrounding it (i.e., a Call object for an outer heavyweight
 * function). See bug 560234.
 */
#define TCF_FUN_ENTRAINS_SCOPES 0x400000

/* The function calls 'eval'. */
#define TCF_FUN_CALLS_EVAL       0x800000

/* The function mutates a positional (non-destructuring) parameter. */
#define TCF_FUN_MUTATES_PARAMETER 0x1000000

/*
 * Compiling an eval() script.
 */
#define TCF_COMPILE_FOR_EVAL     0x2000000

/*
 * The function or a function that encloses it may define new local names
 * at runtime through means other than calling eval.
 */
#define TCF_FUN_MIGHT_ALIAS_LOCALS  0x4000000

/*
 * The script contains singleton initialiser JSOP_OBJECT.
 */
#define TCF_HAS_SINGLETONS       0x8000000

/*
 * Some enclosing scope is a with-statement or E4X filter-expression.
 */
#define TCF_IN_WITH             0x10000000

/*
 * Flags to check for return; vs. return expr; in a function.
 */
#define TCF_RETURN_FLAGS        (TCF_RETURN_EXPR | TCF_RETURN_VOID)

/*
 * Sticky deoptimization flags to propagate from FunctionBody.
 */
#define TCF_FUN_FLAGS           (TCF_FUN_SETS_OUTER_NAME |                    \
                                 TCF_FUN_USES_ARGUMENTS  |                    \
                                 TCF_FUN_PARAM_ARGUMENTS |                    \
                                 TCF_FUN_HEAVYWEIGHT     |                    \
                                 TCF_FUN_IS_GENERATOR    |                    \
                                 TCF_FUN_USES_OWN_NAME   |                    \
                                 TCF_HAS_SHARPS          |                    \
                                 TCF_FUN_CALLS_EVAL      |                    \
                                 TCF_FUN_MIGHT_ALIAS_LOCALS |                 \
                                 TCF_FUN_MUTATES_PARAMETER |                  \
                                 TCF_STRICT_MODE_CODE)

struct JSTreeContext {              /* tree context for semantic checks */
    uint32          flags;          /* statement state flags, see above */
    uint32          bodyid;         /* block number of program/function body */
    uint32          blockidGen;     /* preincremented block number generator */
    JSStmtInfo      *topStmt;       /* top of statement info stack */
    JSStmtInfo      *topScopeStmt;  /* top lexical scope statement */
    JSObjectBox     *blockChainBox; /* compile time block scope chain (NB: one
                                       deeper than the topScopeStmt/downScope
                                       chain when in head of let block/expr) */
    JSParseNode     *blockNode;     /* parse node for a block with let declarations
                                       (block with its own lexical scope)  */
    JSAtomList      decls;          /* function, const, and var declarations */
    js::Parser      *parser;        /* ptr to common parsing and lexing data */

  private:
    union {
        JSFunction  *fun_;          /* function to store argument and variable
                                       names when flags & TCF_IN_FUNCTION */
        JSObject    *scopeChain_;   /* scope chain object for the script */
    };

  public:
    JSFunction *fun() const {
        JS_ASSERT(inFunction());
        return fun_;
    }
    void setFunction(JSFunction *fun) {
        JS_ASSERT(inFunction());
        fun_ = fun;
    }
    JSObject *scopeChain() const {
        JS_ASSERT(!inFunction());
        return scopeChain_;
    }
    void setScopeChain(JSObject *scopeChain) {
        JS_ASSERT(!inFunction());
        scopeChain_ = scopeChain;
    }

    JSAtomList      lexdeps;        /* unresolved lexical name dependencies */
    JSTreeContext   *parent;        /* enclosing function or global context */
    uintN           staticLevel;    /* static compilation unit nesting level */

    JSFunctionBox   *funbox;        /* null or box for function we're compiling
                                       if (flags & TCF_IN_FUNCTION) and not in
                                       Compiler::compileFunctionBody */
    JSFunctionBox   *functionList;

    JSParseNode     *innermostWith; /* innermost WITH parse node */

    js::Bindings    bindings;       /* bindings in this code, including
                                       arguments if we're compiling a function */

#ifdef JS_SCOPE_DEPTH_METER
    uint16          scopeDepth;     /* current lexical scope chain depth */
    uint16          maxScopeDepth;  /* maximum lexical scope chain depth */
#endif

    void trace(JSTracer *trc);

    JSTreeContext(js::Parser *prs)
      : flags(0), bodyid(0), blockidGen(0), topStmt(NULL), topScopeStmt(NULL),
        blockChainBox(NULL), blockNode(NULL), parser(prs), scopeChain_(NULL), parent(prs->tc),
        staticLevel(0), funbox(NULL), functionList(NULL), innermostWith(NULL), bindings(prs->context),
        sharpSlotBase(-1)
    {
        prs->tc = this;
        JS_SCOPE_DEPTH_METERING(scopeDepth = maxScopeDepth = 0);
    }

    /*
     * For functions the tree context is constructed and destructed a second
     * time during code generation. To avoid a redundant stats update in such
     * cases, we store uint16(-1) in maxScopeDepth.
     */
    ~JSTreeContext() {
        parser->tc = this->parent;
        JS_SCOPE_DEPTH_METERING_IF((maxScopeDepth != uint16(-1)),
                                   JS_BASIC_STATS_ACCUM(&parser
                                                          ->context
                                                          ->runtime
                                                          ->lexicalScopeDepthStats,
                                                        maxScopeDepth));
    }

    uintN blockid() { return topStmt ? topStmt->blockid : bodyid; }

    JSObject *blockChain() {
        return blockChainBox ? blockChainBox->object : NULL;
    }

    /*
     * True if we are at the topmost level of a entire script or function body.
     * For example, while parsing this code we would encounter f1 and f2 at
     * body level, but we would not encounter f3 or f4 at body level:
     *
     *   function f1() { function f2() { } }
     *   if (cond) { function f3() { if (cond) { function f4() { } } } }
     */
    bool atBodyLevel() { return !topStmt || (topStmt->flags & SIF_BODY_BLOCK); }

    /* Test whether we're in a statement of given type. */
    bool inStatement(JSStmtType type);

    bool inStrictMode() const {
        return flags & TCF_STRICT_MODE_CODE;
    }

    inline bool needStrictChecks();

    /* 
     * sharpSlotBase is -1 or first slot of pair for [sharpArray, sharpDepth].
     * The parser calls ensureSharpSlots to allocate these two stack locals.
     */
    int sharpSlotBase;
    bool ensureSharpSlots();

    js::Compiler *compiler() { return (js::Compiler *)parser; }

    // Return true there is a generator function within |skip| lexical scopes
    // (going upward) from this context's lexical scope. Always return true if
    // this context is itself a generator.
    bool skipSpansGenerator(unsigned skip);

    bool compileAndGo() const { return flags & TCF_COMPILE_N_GO; }
    bool inFunction() const { return flags & TCF_IN_FUNCTION; }

    bool compiling() const { return flags & TCF_COMPILING; }
    inline JSCodeGenerator *asCodeGenerator();

    bool usesArguments() const {
        return flags & TCF_FUN_USES_ARGUMENTS;
    }

    void noteCallsEval() {
        flags |= TCF_FUN_CALLS_EVAL;
    }

    bool callsEval() const {
        return flags & TCF_FUN_CALLS_EVAL;
    }

    void noteMightAliasLocals() {
        flags |= TCF_FUN_MIGHT_ALIAS_LOCALS;
    }

    bool mightAliasLocals() const {
        return flags & TCF_FUN_MIGHT_ALIAS_LOCALS;
    }

    void noteParameterMutation() {
        JS_ASSERT(inFunction());
        flags |= TCF_FUN_MUTATES_PARAMETER;
    }

    bool mutatesParameter() const {
        JS_ASSERT(inFunction());
        return flags & TCF_FUN_MUTATES_PARAMETER;
    }

    void noteArgumentsUse() {
        JS_ASSERT(inFunction());
        flags |= TCF_FUN_USES_ARGUMENTS;
        if (funbox)
            funbox->node->pn_dflags |= PND_FUNARG;
    }

    bool needsEagerArguments() const {
        return inStrictMode() && ((usesArguments() && mutatesParameter()) || callsEval());
    }
};

/*
 * Return true if we need to check for conditions that elicit
 * JSOPTION_STRICT warnings or strict mode errors.
 */
inline bool JSTreeContext::needStrictChecks() {
    return parser->context->hasStrictOption() || inStrictMode();
}

/*
 * Span-dependent instructions are jumps whose span (from the jump bytecode to
 * the jump target) may require 2 or 4 bytes of immediate operand.
 */
typedef struct JSSpanDep    JSSpanDep;
typedef struct JSJumpTarget JSJumpTarget;

struct JSSpanDep {
    ptrdiff_t       top;        /* offset of first bytecode in an opcode */
    ptrdiff_t       offset;     /* offset - 1 within opcode of jump operand */
    ptrdiff_t       before;     /* original offset - 1 of jump operand */
    JSJumpTarget    *target;    /* tagged target pointer or backpatch delta */
};

/*
 * Jump targets are stored in an AVL tree, for O(log(n)) lookup with targets
 * sorted by offset from left to right, so that targets after a span-dependent
 * instruction whose jump offset operand must be extended can be found quickly
 * and adjusted upward (toward higher offsets).
 */
struct JSJumpTarget {
    ptrdiff_t       offset;     /* offset of span-dependent jump target */
    int             balance;    /* AVL tree balance number */
    JSJumpTarget    *kids[2];   /* left and right AVL tree child pointers */
};

#define JT_LEFT                 0
#define JT_RIGHT                1
#define JT_OTHER_DIR(dir)       (1 - (dir))
#define JT_IMBALANCE(dir)       (((dir) << 1) - 1)
#define JT_DIR(imbalance)       (((imbalance) + 1) >> 1)

/*
 * Backpatch deltas are encoded in JSSpanDep.target if JT_TAG_BIT is clear,
 * so we can maintain backpatch chains when using span dependency records to
 * hold jump offsets that overflow 16 bits.
 */
#define JT_TAG_BIT              ((jsword) 1)
#define JT_UNTAG_SHIFT          1
#define JT_SET_TAG(jt)          ((JSJumpTarget *)((jsword)(jt) | JT_TAG_BIT))
#define JT_CLR_TAG(jt)          ((JSJumpTarget *)((jsword)(jt) & ~JT_TAG_BIT))
#define JT_HAS_TAG(jt)          ((jsword)(jt) & JT_TAG_BIT)

#define BITS_PER_PTRDIFF        (sizeof(ptrdiff_t) * JS_BITS_PER_BYTE)
#define BITS_PER_BPDELTA        (BITS_PER_PTRDIFF - 1 - JT_UNTAG_SHIFT)
#define BPDELTA_MAX             (((ptrdiff_t)1 << BITS_PER_BPDELTA) - 1)
#define BPDELTA_TO_JT(bp)       ((JSJumpTarget *)((bp) << JT_UNTAG_SHIFT))
#define JT_TO_BPDELTA(jt)       ((ptrdiff_t)((jsword)(jt) >> JT_UNTAG_SHIFT))

#define SD_SET_TARGET(sd,jt)    ((sd)->target = JT_SET_TAG(jt))
#define SD_GET_TARGET(sd)       (JS_ASSERT(JT_HAS_TAG((sd)->target)),         \
                                 JT_CLR_TAG((sd)->target))
#define SD_SET_BPDELTA(sd,bp)   ((sd)->target = BPDELTA_TO_JT(bp))
#define SD_GET_BPDELTA(sd)      (JS_ASSERT(!JT_HAS_TAG((sd)->target)),        \
                                 JT_TO_BPDELTA((sd)->target))

/* Avoid asserting twice by expanding SD_GET_TARGET in the "then" clause. */
#define SD_SPAN(sd,pivot)       (SD_GET_TARGET(sd)                            \
                                 ? JT_CLR_TAG((sd)->target)->offset - (pivot) \
                                 : 0)

typedef struct JSTryNode JSTryNode;

struct JSTryNode {
    JSTryNote       note;
    JSTryNode       *prev;
};

struct JSCGObjectList {
    uint32              length;     /* number of emitted so far objects */
    JSObjectBox         *lastbox;   /* last emitted object */

    JSCGObjectList() : length(0), lastbox(NULL) {}

    uintN index(JSObjectBox *objbox);
    void finish(JSObjectArray *array);
};

class JSGCConstList {
    js::Vector<js::Value> list;
  public:
    JSGCConstList(JSContext *cx) : list(cx) {}
    bool append(js::Value v) { return list.append(v); }
    size_t length() const { return list.length(); }
    void finish(JSConstArray *array);

};

struct JSCodeGenerator : public JSTreeContext
{
    JSArenaPool     *codePool;      /* pointer to thread code arena pool */
    JSArenaPool     *notePool;      /* pointer to thread srcnote arena pool */
    void            *codeMark;      /* low watermark in cg->codePool */
    void            *noteMark;      /* low watermark in cg->notePool */

    struct {
        jsbytecode  *base;          /* base of JS bytecode vector */
        jsbytecode  *limit;         /* one byte beyond end of bytecode */
        jsbytecode  *next;          /* pointer to next free bytecode */
        jssrcnote   *notes;         /* source notes, see below */
        uintN       noteCount;      /* number of source notes so far */
        uintN       noteMask;       /* growth increment for notes */
        ptrdiff_t   lastNoteOffset; /* code offset for last source note */
        uintN       currentLine;    /* line number for tree-based srcnote gen */
    } prolog, main, *current;

    JSAtomList      atomList;       /* literals indexed for mapping */
    uintN           firstLine;      /* first line, for js_NewScriptFromCG */

    intN            stackDepth;     /* current stack depth in script frame */
    uintN           maxStackDepth;  /* maximum stack depth so far */

    uintN           ntrynotes;      /* number of allocated so far try notes */
    JSTryNode       *lastTryNode;   /* the last allocated try node */

    JSSpanDep       *spanDeps;      /* span dependent instruction records */
    JSJumpTarget    *jumpTargets;   /* AVL tree of jump target offsets */
    JSJumpTarget    *jtFreeList;    /* JT_LEFT-linked list of free structs */
    uintN           numSpanDeps;    /* number of span dependencies */
    uintN           numJumpTargets; /* number of jump targets */
    ptrdiff_t       spanDepTodo;    /* offset from main.base of potentially
                                       unoptimized spandeps */

    uintN           arrayCompDepth; /* stack depth of array in comprehension */

    uintN           emitLevel;      /* js_EmitTree recursion level */

    typedef js::HashMap<JSAtom *, js::Value> ConstMap;
    ConstMap        constMap;       /* compile time constants */

    JSGCConstList   constList;      /* constants to be included with the script */

    JSCGObjectList  objectList;     /* list of emitted objects */
    JSCGObjectList  regexpList;     /* list of emitted regexp that will be
                                       cloned during execution */

    JSAtomList      upvarList;      /* map of atoms to upvar indexes */
    JSUpvarArray    upvarMap;       /* indexed upvar pairs (JS_realloc'ed) */

    typedef js::Vector<js::GlobalSlotArray::Entry, 16, js::ContextAllocPolicy> GlobalUseVector;

    GlobalUseVector globalUses;     /* per-script global uses */
    JSAtomList      globalMap;      /* per-script map of global name to globalUses vector */

    /* Vectors of pn_cookie slot values. */
    typedef js::Vector<uint32, 8, js::ContextAllocPolicy> SlotVector;
    SlotVector      closedArgs;
    SlotVector      closedVars;

    uint16          traceIndex;     /* index for the next JSOP_TRACE instruction */

    /*
     * Initialize cg to allocate bytecode space from codePool, source note
     * space from notePool, and all other arena-allocated temporaries from
     * parser->context->tempPool.
     */
    JSCodeGenerator(js::Parser *parser,
                    JSArenaPool *codePool, JSArenaPool *notePool,
                    uintN lineno);
    bool init();

    /*
     * Release cg->codePool, cg->notePool, and parser->context->tempPool to
     * marks set by JSCodeGenerator's ctor. Note that cgs are magic: they own
     * the arena pool "tops-of-stack" space above their codeMark, noteMark, and
     * tempMark points.  This means you cannot alloc from tempPool and save the
     * pointer beyond the next JSCodeGenerator destructor call.
     */
    ~JSCodeGenerator();

    /*
     * Adds a use of a variable that is statically known to exist on the
     * global object. 
     *
     * The actual slot of the variable on the global object is not known
     * until after compilation. Properties must be resolved before being
     * added, to avoid aliasing properties that should be resolved. This makes
     * slot prediction based on the global object's free slot impossible. So,
     * we use the slot to index into cg->globalScope->defs, and perform a
     * fixup of the script at the very end of compilation.
     *
     * If the global use can be cached, |cookie| will be set to |slot|.
     * Otherwise, |cookie| is set to the free cookie value.
     */
    bool addGlobalUse(JSAtom *atom, uint32 slot, js::UpvarCookie *cookie);

    bool hasSharps() const {
        bool rv = !!(flags & TCF_HAS_SHARPS);
        JS_ASSERT((sharpSlotBase >= 0) == rv);
        return rv;
    }

    uintN sharpSlots() const {
        return hasSharps() ? SHARP_NSLOTS : 0;
    }

    bool compilingForEval() const { return !!(flags & TCF_COMPILE_FOR_EVAL); }
    JSVersion version() const { return parser->versionWithFlags(); }

    bool shouldNoteClosedName(JSParseNode *pn);

    bool checkSingletonContext() {
        if (!compileAndGo() || inFunction())
            return false;
        for (JSStmtInfo *stmt = topStmt; stmt; stmt = stmt->down) {
            if (STMT_IS_LOOP(stmt))
                return false;
        }
        flags |= TCF_HAS_SINGLETONS;
        return true;
    }
};

#define CG_TS(cg)               TS((cg)->parser)

#define CG_BASE(cg)             ((cg)->current->base)
#define CG_LIMIT(cg)            ((cg)->current->limit)
#define CG_NEXT(cg)             ((cg)->current->next)
#define CG_CODE(cg,offset)      (CG_BASE(cg) + (offset))
#define CG_OFFSET(cg)           (CG_NEXT(cg) - CG_BASE(cg))

#define CG_NOTES(cg)            ((cg)->current->notes)
#define CG_NOTE_COUNT(cg)       ((cg)->current->noteCount)
#define CG_NOTE_MASK(cg)        ((cg)->current->noteMask)
#define CG_LAST_NOTE_OFFSET(cg) ((cg)->current->lastNoteOffset)
#define CG_CURRENT_LINE(cg)     ((cg)->current->currentLine)

#define CG_PROLOG_BASE(cg)      ((cg)->prolog.base)
#define CG_PROLOG_LIMIT(cg)     ((cg)->prolog.limit)
#define CG_PROLOG_NEXT(cg)      ((cg)->prolog.next)
#define CG_PROLOG_CODE(cg,poff) (CG_PROLOG_BASE(cg) + (poff))
#define CG_PROLOG_OFFSET(cg)    (CG_PROLOG_NEXT(cg) - CG_PROLOG_BASE(cg))

#define CG_SWITCH_TO_MAIN(cg)   ((cg)->current = &(cg)->main)
#define CG_SWITCH_TO_PROLOG(cg) ((cg)->current = &(cg)->prolog)

inline JSCodeGenerator *
JSTreeContext::asCodeGenerator()
{
    JS_ASSERT(compiling());
    return static_cast<JSCodeGenerator *>(this);
}

/*
 * Emit one bytecode.
 */
extern ptrdiff_t
js_Emit1(JSContext *cx, JSCodeGenerator *cg, JSOp op);

/*
 * Emit two bytecodes, an opcode (op) with a byte of immediate operand (op1).
 */
extern ptrdiff_t
js_Emit2(JSContext *cx, JSCodeGenerator *cg, JSOp op, jsbytecode op1);

/*
 * Emit three bytecodes, an opcode with two bytes of immediate operands.
 */
extern ptrdiff_t
js_Emit3(JSContext *cx, JSCodeGenerator *cg, JSOp op, jsbytecode op1,
         jsbytecode op2);

/*
 * Emit five bytecodes, an opcode with two 16-bit immediates.
 */
extern ptrdiff_t
js_Emit5(JSContext *cx, JSCodeGenerator *cg, JSOp op, uint16 op1,
         uint16 op2);

/*
 * Emit (1 + extra) bytecodes, for N bytes of op and its immediate operand.
 */
extern ptrdiff_t
js_EmitN(JSContext *cx, JSCodeGenerator *cg, JSOp op, size_t extra);

/*
 * Unsafe macro to call js_SetJumpOffset and return false if it does.
 */
#define CHECK_AND_SET_JUMP_OFFSET_CUSTOM(cx,cg,pc,off,BAD_EXIT)               \
    JS_BEGIN_MACRO                                                            \
        if (!js_SetJumpOffset(cx, cg, pc, off)) {                             \
            BAD_EXIT;                                                         \
        }                                                                     \
    JS_END_MACRO

#define CHECK_AND_SET_JUMP_OFFSET(cx,cg,pc,off)                               \
    CHECK_AND_SET_JUMP_OFFSET_CUSTOM(cx,cg,pc,off,return JS_FALSE)

#define CHECK_AND_SET_JUMP_OFFSET_AT_CUSTOM(cx,cg,off,BAD_EXIT)               \
    CHECK_AND_SET_JUMP_OFFSET_CUSTOM(cx, cg, CG_CODE(cg,off),                 \
                                     CG_OFFSET(cg) - (off), BAD_EXIT)

#define CHECK_AND_SET_JUMP_OFFSET_AT(cx,cg,off)                               \
    CHECK_AND_SET_JUMP_OFFSET_AT_CUSTOM(cx, cg, off, return JS_FALSE)

extern JSBool
js_SetJumpOffset(JSContext *cx, JSCodeGenerator *cg, jsbytecode *pc,
                 ptrdiff_t off);

/*
 * Push the C-stack-allocated struct at stmt onto the stmtInfo stack.
 */
extern void
js_PushStatement(JSTreeContext *tc, JSStmtInfo *stmt, JSStmtType type,
                 ptrdiff_t top);

/*
 * Push a block scope statement and link blockObj into tc->blockChain. To pop
 * this statement info record, use js_PopStatement as usual, or if appropriate
 * (if generating code), js_PopStatementCG.
 */
extern void
js_PushBlockScope(JSTreeContext *tc, JSStmtInfo *stmt, JSObjectBox *blockBox,
                  ptrdiff_t top);

/*
 * Pop tc->topStmt. If the top JSStmtInfo struct is not stack-allocated, it
 * is up to the caller to free it.
 */
extern void
js_PopStatement(JSTreeContext *tc);

/*
 * Like js_PopStatement(cg), also patch breaks and continues unless the top
 * statement info record represents a try-catch-finally suite. May fail if a
 * jump offset overflows.
 */
extern JSBool
js_PopStatementCG(JSContext *cx, JSCodeGenerator *cg);

/*
 * Define and lookup a primitive jsval associated with the const named by atom.
 * js_DefineCompileTimeConstant analyzes the constant-folded initializer at pn
 * and saves the const's value in cg->constList, if it can be used at compile
 * time. It returns true unless an error occurred.
 *
 * If the initializer's value could not be saved, js_DefineCompileTimeConstant
 * calls will return the undefined value. js_DefineCompileTimeConstant tries
 * to find a const value memorized for atom, returning true with *vp set to a
 * value other than undefined if the constant was found, true with *vp set to
 * JSVAL_VOID if not found, and false on error.
 */
extern JSBool
js_DefineCompileTimeConstant(JSContext *cx, JSCodeGenerator *cg, JSAtom *atom,
                             JSParseNode *pn);

/*
 * Find a lexically scoped variable (one declared by let, catch, or an array
 * comprehension) named by atom, looking in tc's compile-time scopes.
 *
 * If a WITH statement is reached along the scope stack, return its statement
 * info record, so callers can tell that atom is ambiguous. If slotp is not
 * null, then if atom is found, set *slotp to its stack slot, otherwise to -1.
 * This means that if slotp is not null, all the block objects on the lexical
 * scope chain must have had their depth slots computed by the code generator,
 * so the caller must be under js_EmitTree.
 *
 * In any event, directly return the statement info record in which atom was
 * found. Otherwise return null.
 */
extern JSStmtInfo *
js_LexicalLookup(JSTreeContext *tc, JSAtom *atom, jsint *slotp,
                 JSStmtInfo *stmt = NULL);

/*
 * Emit code into cg for the tree rooted at pn.
 */
extern JSBool
js_EmitTree(JSContext *cx, JSCodeGenerator *cg, JSParseNode *pn);

/*
 * Emit function code using cg for the tree rooted at body.
 */
extern JSBool
js_EmitFunctionScript(JSContext *cx, JSCodeGenerator *cg, JSParseNode *body);

/*
 * Source notes generated along with bytecode for decompiling and debugging.
 * A source note is a uint8 with 5 bits of type and 3 of offset from the pc of
 * the previous note. If 3 bits of offset aren't enough, extended delta notes
 * (SRC_XDELTA) consisting of 2 set high order bits followed by 6 offset bits
 * are emitted before the next note. Some notes have operand offsets encoded
 * immediately after them, in note bytes or byte-triples.
 *
 *                 Source Note               Extended Delta
 *              +7-6-5-4-3+2-1-0+           +7-6-5+4-3-2-1-0+
 *              |note-type|delta|           |1 1| ext-delta |
 *              +---------+-----+           +---+-----------+
 *
 * At most one "gettable" note (i.e., a note of type other than SRC_NEWLINE,
 * SRC_SETLINE, and SRC_XDELTA) applies to a given bytecode.
 *
 * NB: the js_SrcNoteSpec array in jsemit.c is indexed by this enum, so its
 * initializers need to match the order here.
 *
 * Note on adding new source notes: every pair of bytecodes (A, B) where A and
 * B have disjoint sets of source notes that could apply to each bytecode may
 * reuse the same note type value for two notes (snA, snB) that have the same
 * arity, offsetBias, and isSpanDep initializers in js_SrcNoteSpec. This is
 * why SRC_IF and SRC_INITPROP have the same value below. For bad historical
 * reasons, some bytecodes below that could be overlayed have not been, but
 * before using SRC_EXTENDED, consider compressing the existing note types.
 *
 * Don't forget to update JSXDR_BYTECODE_VERSION in jsxdrapi.h for all such
 * incompatible source note or other bytecode changes.
 */
typedef enum JSSrcNoteType {
    SRC_NULL        = 0,        /* terminates a note vector */
    SRC_IF          = 1,        /* JSOP_IFEQ bytecode is from an if-then */
    SRC_BREAK       = 1,        /* JSOP_GOTO is a break */
    SRC_INITPROP    = 1,        /* disjoint meaning applied to JSOP_INITELEM or
                                   to an index label in a regular (structuring)
                                   or a destructuring object initialiser */
    SRC_GENEXP      = 1,        /* JSOP_LAMBDA from generator expression */
    SRC_IF_ELSE     = 2,        /* JSOP_IFEQ bytecode is from an if-then-else */
    SRC_FOR_IN      = 2,        /* JSOP_GOTO to for-in loop condition from
                                   before loop (same arity as SRC_IF_ELSE) */
    SRC_FOR         = 3,        /* JSOP_NOP or JSOP_POP in for(;;) loop head */
    SRC_WHILE       = 4,        /* JSOP_GOTO to for or while loop condition
                                   from before loop, else JSOP_NOP at top of
                                   do-while loop */
    SRC_TRACE       = 4,        /* For JSOP_TRACE; includes distance to loop end */
    SRC_CONTINUE    = 5,        /* JSOP_GOTO is a continue, not a break;
                                   also used on JSOP_ENDINIT if extra comma
                                   at end of array literal: [1,2,,];
                                   JSOP_DUP continuing destructuring pattern */
    SRC_DECL        = 6,        /* type of a declaration (var, const, let*) */
    SRC_DESTRUCT    = 6,        /* JSOP_DUP starting a destructuring assignment
                                   operation, with SRC_DECL_* offset operand */
    SRC_PCDELTA     = 7,        /* distance forward from comma-operator to
                                   next POP, or from CONDSWITCH to first CASE
                                   opcode, etc. -- always a forward delta */
    SRC_GROUPASSIGN = 7,        /* SRC_DESTRUCT variant for [a, b] = [c, d] */
    SRC_ASSIGNOP    = 8,        /* += or another assign-op follows */
    SRC_COND        = 9,        /* JSOP_IFEQ is from conditional ?: operator */
    SRC_BRACE       = 10,       /* mandatory brace, for scope or to avoid
                                   dangling else */
    SRC_HIDDEN      = 11,       /* opcode shouldn't be decompiled */
    SRC_PCBASE      = 12,       /* distance back from annotated getprop or
                                   setprop op to left-most obj.prop.subprop
                                   bytecode -- always a backward delta */
    SRC_LABEL       = 13,       /* JSOP_NOP for label: with atomid immediate */
    SRC_LABELBRACE  = 14,       /* JSOP_NOP for label: {...} begin brace */
    SRC_ENDBRACE    = 15,       /* JSOP_NOP for label: {...} end brace */
    SRC_BREAK2LABEL = 16,       /* JSOP_GOTO for 'break label' with atomid */
    SRC_CONT2LABEL  = 17,       /* JSOP_GOTO for 'continue label' with atomid */
    SRC_SWITCH      = 18,       /* JSOP_*SWITCH with offset to end of switch,
                                   2nd off to first JSOP_CASE if condswitch */
    SRC_FUNCDEF     = 19,       /* JSOP_NOP for function f() with atomid */
    SRC_CATCH       = 20,       /* catch block has guard */
    SRC_EXTENDED    = 21,       /* extended source note, 32-159, in next byte */
    SRC_NEWLINE     = 22,       /* bytecode follows a source newline */
    SRC_SETLINE     = 23,       /* a file-absolute source line number note */
    SRC_XDELTA      = 24        /* 24-31 are for extended delta notes */
} JSSrcNoteType;

/*
 * Constants for the SRC_DECL source note. Note that span-dependent bytecode
 * selection means that any SRC_DECL offset greater than SRC_DECL_LET may need
 * to be adjusted, but these "offsets" are too small to span a span-dependent
 * instruction, so can be used to denote distinct declaration syntaxes to the
 * decompiler.
 *
 * NB: the var_prefix array in jsopcode.c depends on these dense indexes from
 * SRC_DECL_VAR through SRC_DECL_LET.
 */
#define SRC_DECL_VAR            0
#define SRC_DECL_CONST          1
#define SRC_DECL_LET            2
#define SRC_DECL_NONE           3

#define SN_TYPE_BITS            5
#define SN_DELTA_BITS           3
#define SN_XDELTA_BITS          6
#define SN_TYPE_MASK            (JS_BITMASK(SN_TYPE_BITS) << SN_DELTA_BITS)
#define SN_DELTA_MASK           ((ptrdiff_t)JS_BITMASK(SN_DELTA_BITS))
#define SN_XDELTA_MASK          ((ptrdiff_t)JS_BITMASK(SN_XDELTA_BITS))

#define SN_MAKE_NOTE(sn,t,d)    (*(sn) = (jssrcnote)                          \
                                          (((t) << SN_DELTA_BITS)             \
                                           | ((d) & SN_DELTA_MASK)))
#define SN_MAKE_XDELTA(sn,d)    (*(sn) = (jssrcnote)                          \
                                          ((SRC_XDELTA << SN_DELTA_BITS)      \
                                           | ((d) & SN_XDELTA_MASK)))

#define SN_IS_XDELTA(sn)        ((*(sn) >> SN_DELTA_BITS) >= SRC_XDELTA)
#define SN_TYPE(sn)             ((JSSrcNoteType)(SN_IS_XDELTA(sn)             \
                                                 ? SRC_XDELTA                 \
                                                 : *(sn) >> SN_DELTA_BITS))
#define SN_SET_TYPE(sn,type)    SN_MAKE_NOTE(sn, type, SN_DELTA(sn))
#define SN_IS_GETTABLE(sn)      (SN_TYPE(sn) < SRC_NEWLINE)

#define SN_DELTA(sn)            ((ptrdiff_t)(SN_IS_XDELTA(sn)                 \
                                             ? *(sn) & SN_XDELTA_MASK         \
                                             : *(sn) & SN_DELTA_MASK))
#define SN_SET_DELTA(sn,delta)  (SN_IS_XDELTA(sn)                             \
                                 ? SN_MAKE_XDELTA(sn, delta)                  \
                                 : SN_MAKE_NOTE(sn, SN_TYPE(sn), delta))

#define SN_DELTA_LIMIT          ((ptrdiff_t)JS_BIT(SN_DELTA_BITS))
#define SN_XDELTA_LIMIT         ((ptrdiff_t)JS_BIT(SN_XDELTA_BITS))

/*
 * Offset fields follow certain notes and are frequency-encoded: an offset in
 * [0,0x7f] consumes one byte, an offset in [0x80,0x7fffff] takes three, and
 * the high bit of the first byte is set.
 */
#define SN_3BYTE_OFFSET_FLAG    0x80
#define SN_3BYTE_OFFSET_MASK    0x7f

typedef struct JSSrcNoteSpec {
    const char      *name;      /* name for disassembly/debugging output */
    int8            arity;      /* number of offset operands */
    uint8           offsetBias; /* bias of offset(s) from annotated pc */
    int8            isSpanDep;  /* 1 or -1 if offsets could span extended ops,
                                   0 otherwise; sign tells span direction */
} JSSrcNoteSpec;

extern JS_FRIEND_DATA(JSSrcNoteSpec) js_SrcNoteSpec[];
extern JS_FRIEND_API(uintN)          js_SrcNoteLength(jssrcnote *sn);

#define SN_LENGTH(sn)           ((js_SrcNoteSpec[SN_TYPE(sn)].arity == 0) ? 1 \
                                 : js_SrcNoteLength(sn))
#define SN_NEXT(sn)             ((sn) + SN_LENGTH(sn))

/* A source note array is terminated by an all-zero element. */
#define SN_MAKE_TERMINATOR(sn)  (*(sn) = SRC_NULL)
#define SN_IS_TERMINATOR(sn)    (*(sn) == SRC_NULL)

/*
 * Append a new source note of the given type (and therefore size) to cg's
 * notes dynamic array, updating cg->noteCount. Return the new note's index
 * within the array pointed at by cg->current->notes. Return -1 if out of
 * memory.
 */
extern intN
js_NewSrcNote(JSContext *cx, JSCodeGenerator *cg, JSSrcNoteType type);

extern intN
js_NewSrcNote2(JSContext *cx, JSCodeGenerator *cg, JSSrcNoteType type,
               ptrdiff_t offset);

extern intN
js_NewSrcNote3(JSContext *cx, JSCodeGenerator *cg, JSSrcNoteType type,
               ptrdiff_t offset1, ptrdiff_t offset2);

/*
 * NB: this function can add at most one extra extended delta note.
 */
extern jssrcnote *
js_AddToSrcNoteDelta(JSContext *cx, JSCodeGenerator *cg, jssrcnote *sn,
                     ptrdiff_t delta);

/*
 * Get and set the offset operand identified by which (0 for the first, etc.).
 */
extern JS_FRIEND_API(ptrdiff_t)
js_GetSrcNoteOffset(jssrcnote *sn, uintN which);

extern JSBool
js_SetSrcNoteOffset(JSContext *cx, JSCodeGenerator *cg, uintN index,
                    uintN which, ptrdiff_t offset);

/*
 * Finish taking source notes in cx's notePool, copying final notes to the new
 * stable store allocated by the caller and passed in via notes. Return false
 * on malloc failure, which means this function reported an error.
 *
 * To compute the number of jssrcnotes to allocate and pass in via notes, use
 * the CG_COUNT_FINAL_SRCNOTES macro. This macro knows a lot about details of
 * js_FinishTakingSrcNotes, SO DON'T CHANGE jsemit.c's js_FinishTakingSrcNotes
 * FUNCTION WITHOUT CHECKING WHETHER THIS MACRO NEEDS CORRESPONDING CHANGES!
 */
#define CG_COUNT_FINAL_SRCNOTES(cg, cnt)                                      \
    JS_BEGIN_MACRO                                                            \
        ptrdiff_t diff_ = CG_PROLOG_OFFSET(cg) - (cg)->prolog.lastNoteOffset; \
        cnt = (cg)->prolog.noteCount + (cg)->main.noteCount + 1;              \
        if ((cg)->prolog.noteCount &&                                         \
            (cg)->prolog.currentLine != (cg)->firstLine) {                    \
            if (diff_ > SN_DELTA_MASK)                                        \
                cnt += JS_HOWMANY(diff_ - SN_DELTA_MASK, SN_XDELTA_MASK);     \
            cnt += 2 + (((cg)->firstLine > SN_3BYTE_OFFSET_MASK) << 1);       \
        } else if (diff_ > 0) {                                               \
            if (cg->main.noteCount) {                                         \
                jssrcnote *sn_ = (cg)->main.notes;                            \
                diff_ -= SN_IS_XDELTA(sn_)                                    \
                         ? SN_XDELTA_MASK - (*sn_ & SN_XDELTA_MASK)           \
                         : SN_DELTA_MASK - (*sn_ & SN_DELTA_MASK);            \
            }                                                                 \
            if (diff_ > 0)                                                    \
                cnt += JS_HOWMANY(diff_, SN_XDELTA_MASK);                     \
        }                                                                     \
    JS_END_MACRO

extern JSBool
js_FinishTakingSrcNotes(JSContext *cx, JSCodeGenerator *cg, jssrcnote *notes);

extern void
js_FinishTakingTryNotes(JSCodeGenerator *cg, JSTryNoteArray *array);

JS_END_EXTERN_C

#endif /* jsemit_h___ */
