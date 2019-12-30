// This code is in the public domain -- Ignacio Castaño <castano@gmail.com>

#pragma once
#ifndef NV_THREAD_PARALLELFOR_H
#define NV_THREAD_PARALLELFOR_H

#include "nvthread.h"
//#include "Atomic.h" // atomic<uint>

namespace nv
{
    class Thread;
    class ThreadPool;

    typedef void ForTask(void * context, /*int tid,*/ int idx); // @@ It would be nice to have the thread index as an argument here.

    struct ParallelFor {
        ParallelFor(ForTask * task, void * context);
        ~ParallelFor();

        void run(uint count, uint step = 1);

        // Invariant:
        ForTask * task;
        void * context;
        ThreadPool * pool;

        // State:
        uint count;
        uint step;
        /*atomic<uint>*/ uint idx;
    };


#if NV_CC_CPP11

    template <typename F>
    void sequential_for(uint count, F f) {
        for (uint i = 0; i < count; i++) {
            f(i);
        }
    }


    template <typename F>
    void parallel_for(uint count, uint step, F f) {
        // Transform lambda into function pointer.
        auto lambda = [](void* context, /*int tid, */int idx) {
            F & f = *reinterpret_cast<F *>(context);
            f(/*tid, */idx);
        };

        ParallelFor pf(lambda, &f);
        pf.run(count, step);
    }


    template <typename F>
    void parallel_for(uint count, F f) {
        parallel_for(count, /*step=*/1, f);
    }


    template <typename F>
    void parallel_for_if(uint count, uint step, bool condition, F f) {
        if (condition) {
            parallel_for(count, step, f);
        }
        else {
            sequential_for(count, f);
        }
    }


#if 0
    template <typename F, typename T>
    void parallel_for_each(Array<T> & array, uint step, F f) {
        // Transform lambda into function pointer.
        auto lambda = [](void* context, int idx) {
            F & f = *reinterpret_cast<F *>(context);
            f(array[idx]);
        };

        ParallelFor pf(lambda, &f);
        pf.run(count, step);
    }
#endif


#endif // NV_CC_CPP11


/*

#include "nvthread/Mutex.h"
#include "nvcore/Array.inl"

    template <typename T>
    struct ParallelOutputStream {
#if 0
        // In its most basic implementation the parallel stream is simply a single array protected by a mutex.
        Parallel_Output_Stream(uint producer_count) {}

        void reset() { final_array.clear(); }
        void append(uint producer_id, const T & t) { Lock(mutex); final_array.append(t); }
        nv::Array<T> & finalize() { return final_array; }
        
        nv::Mutex mutex;
        nv::Array<T> final_array;

#elif 0
        // Another simple implementation is to have N arrays that are merged at the end.
        ParallelOutputStream(uint producer_count) : producer_count(producer_count) {
            partial_array = new Array<T>[producer_count];
        }

        void reset() {
            for (int i = 0; i < producer_count; i++) {
                partial_array[i].clear();
            }
        }

        void append(uint producer_id, const T & t) { 
            nvCheck(producer_id < producer_count);
            partial_array[producer_id].append(t);
        }

        nv::Array<T> & finalize() {
            for (int i = 1; i < producer_count; i++) {
                partial_array->append(partial_array[i]);
                partial_array[i].clear();
            }
            return *partial_array;
        }

        uint producer_count;
        nv::Array<T> * partial_array;
#else
        ParallelOutputStream(uint producer_count) : producer_count(producer_count) {
            partial_array = new PartialArray[producer_count];
        }

        // But a more sophisticated implementation keeps N short arrays that are merged as they get full. This preserves partial order.
        struct PartialArray {          // Make sure this is aligned to cache lines. We want producers to access their respective arrays without conflicts.
            uint count;
            T data[32];                 // Pick size to minimize wasted space considering cache line alignment?
        };

        const uint producer_count;
        PartialArray * partial_array;

        // @@ Make sure mutex and partial_array are not in the same cache line!

        nv::Mutex mutex;
        nv::Array<T> final_array;

        void append(uint producer_id, const T & t) {
            if (partial_array[producer_id].count == 32) {
                partial_array[producer_id].count = 0;
                Lock(mutex);
                final_array.append(partial_array[producer_id].data, 32);
            }

            partial_array[producer_id].data[partial_array[producer_id].count++] = t;
        }
        nv::Array<T> & finalize() {
            for (int i = 0; i < producer_count; i++) {
                final_array.append(partial_array[producer_id].data, partial_array[producer_id].count);
            }
            return final_array;
        }
#endif
    };

*/


} // nv namespace


#endif // NV_THREAD_PARALLELFOR_H
