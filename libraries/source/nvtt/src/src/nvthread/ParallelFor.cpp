// This code is in the public domain -- Ignacio Castaño <castano@gmail.com>

#include "ParallelFor.h"
#include "Thread.h"
#include "Atomic.h"
#include "ThreadPool.h"

#include "nvcore/Utils.h" // toI32

using namespace nv;

#define ENABLE_PARALLEL_FOR 1

static void worker(void * arg, int tid) {
    ParallelFor * owner = (ParallelFor *)arg;

    while(true) {
        uint new_idx = atomicFetchAndAdd(&owner->idx, owner->step);
        if (new_idx >= owner->count) {
            break;
        }

        const uint count = min(owner->count, new_idx + owner->step);
        for (uint i = new_idx; i < count; i++) {
            owner->task(owner->context, /*tid, */i);
        }
    }
}


ParallelFor::ParallelFor(ForTask * task, void * context) : task(task), context(context) {
#if ENABLE_PARALLEL_FOR
    pool = ThreadPool::acquire();
#endif
}

ParallelFor::~ParallelFor() {
#if ENABLE_PARALLEL_FOR
    ThreadPool::release(pool);
#endif
}

void ParallelFor::run(uint count, uint step/*= 1*/) {
#if ENABLE_PARALLEL_FOR
    storeRelease(&this->count, count);
    storeRelease(&this->step, step);

    // Init atomic counter to zero.
    storeRelease(&idx, 0);

    // Start threads.
    pool->run(worker, this);

    nvDebugCheck(idx >= count);
#else
    for (int i = 0; i < toI32(count); i++) {
        task(context, i);
    }
#endif
}

