
#include "nvtt.h"

// OpenMP
// http://en.wikipedia.org/wiki/OpenMP
#if defined(HAVE_OPENMP)
#include <omp.h>
#endif

// Gran Central Dispatch (GCD/libdispatch)
// http://developer.apple.com/mac/library/documentation/Performance/Reference/GCD_libdispatch_Ref/Reference/reference.html
#if NV_OS_DARWIN && defined(HAVE_DISPATCH_H)
#define HAVE_GCD 1
#include <dispatch/dispatch.h>
#endif

// Parallel Patterns Library (PPL) is part of Microsoft's concurrency runtime: 
// http://msdn.microsoft.com/en-us/library/dd504870.aspx
#if NV_OS_WIN32 && _MSC_VER >= 1600
#define HAVE_PPL 1
#include <ppl.h>
#endif

// Intel Thread Building Blocks (TBB).
// http://www.threadingbuildingblocks.org/
#if defined(HAVE_TBB)
#include <tbb/parallel_for.h>
#endif

#include "nvthread/ParallelFor.h"


namespace nvtt {

    struct SequentialTaskDispatcher : public TaskDispatcher
    {
        virtual void dispatch(Task * task, void * context, int count) {
            for (int i = 0; i < count; i++) {
                task(context, i);
            }
        }
    };

    struct ParallelTaskDispatcher : public TaskDispatcher
    {
        virtual void dispatch(Task * task, void * context, int count) {
            nv::ParallelFor parallelFor(task, context);
            parallelFor.run(count); // @@ Add support for custom grain.
        }
    };


#if defined(HAVE_OPENMP)

    struct OpenMPTaskDispatcher : public TaskDispatcher
    {
        virtual void dispatch(Task * task, void * context, int count) {
            #pragma omp parallel for
            for (int i = 0; i < count; i++) {
                task(context, i);
            }
        }
    };

#endif

#if NV_OS_DARWIN && defined(HAVE_DISPATCH_H)

    // Task dispatcher using Apple's Grand Central Dispatch.
    struct AppleTaskDispatcher : public TaskDispatcher
    {
        // @@ This is really lame, but I refuse to use size_t in the public API.
        struct BlockContext {
            Task * task;
            void * context;
        };

        static void block(void * context, size_t id) {
            BlockContext * ctx = (BlockContext *)context;
            ctx->task(ctx->context, int(id));
        }

        virtual void dispatch(Task * task, void * context, int count) {
            dispatch_queue_t q = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
            BlockContext blockCtx = { task, context };
            dispatch_apply_f(count, q, &blockCtx, block);
        }
    };

#endif

#if defined(HAVE_PPL)

    struct TaskFunctor {
        TaskFunctor(Task * task, void * context) : task(task), context(context) {}
        void operator()(int n) const {
            task(context, n);
        }
        Task * task;
        void * context;
    };

    // Task dispatcher using Microsoft's concurrency runtime.
    struct MicrosoftTaskDispatcher : public TaskDispatcher
    {
        virtual void dispatch(Task * task, void * context, int count)
        {
            TaskFunctor func(task, context);
            Concurrency::parallel_for(0, count, func);
        }
    };

#endif

#if defined(HAVE_TBB)

    struct TaskFunctor {
        TaskFunctor(Task * task, void * context) : task(task), context(context) {}
        void operator()(int & n) const {
            task(context, n);
        }
        Task * task;
        void * context;
    };

    // Task dispatcher using Inte's Thread Building Blocks.
    struct IntelTaskDispatcher : public TaskDispatcher
    {
        virtual void dispatch(Task * task, void * context, int count) {
            parallel_for(blocked_range<int>(0, count, 1), TaskFunctor(task, context));
        }
    };

#endif

#if defined(HAVE_OPENMP)
    typedef OpenMPTaskDispatcher        ConcurrentTaskDispatcher;
#elif defined(HAVE_TBB)
    typedef IntelTaskDispatcher         ConcurrentTaskDispatcher;
#elif defined(HAVE_PPL)
    typedef MicrosoftTaskDispatcher     ConcurrentTaskDispatcher;
#elif defined(HAVE_GCD)
    typedef AppleTaskDispatcher         ConcurrentTaskDispatcher;
#else
    //typedef SequentialTaskDispatcher    ConcurrentTaskDispatcher;
    typedef ParallelTaskDispatcher        ConcurrentTaskDispatcher;
#endif

} // namespace nvtt
