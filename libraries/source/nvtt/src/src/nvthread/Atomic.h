// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_THREAD_ATOMIC_H
#define NV_THREAD_ATOMIC_H

#include "nvthread.h"

#include "nvcore/Debug.h"


#if NV_CC_MSVC

#include <intrin.h> // Already included by nvthread.h

#pragma intrinsic(_InterlockedIncrement, _InterlockedDecrement)
#pragma intrinsic(_InterlockedCompareExchange, _InterlockedExchange)
//#pragma intrinsic(_InterlockedExchangeAdd64)

/*
extern "C"
{
    #pragma intrinsic(_InterlockedIncrement, _InterlockedDecrement)
    LONG  __cdecl _InterlockedIncrement(long volatile *Addend);
    LONG  __cdecl _InterlockedDecrement(long volatile *Addend);

    #pragma intrinsic(_InterlockedCompareExchange, _InterlockedExchange)
    LONG  __cdecl _InterlockedCompareExchange(long volatile * Destination, long Exchange, long Compared);
    LONG  __cdecl _InterlockedExchange(long volatile * Target, LONG Value);
}
*/

#endif // NV_CC_MSVC

#if NV_CC_CLANG && POSH_CPU_STRONGARM
// LLVM/Clang do not yet have functioning atomics as of 2.1
// #include <atomic>
#endif

//ACS: need this if we want to use Apple's atomics.
/*
#if NV_OS_IOS || NV_OS_DARWIN
// for iOS & OSX we use apple's atomics
#include "libkern/OSAtomic.h"
#endif
*/

namespace nv {

    // Load and stores.
    inline uint32 loadRelaxed(const uint32 * ptr) { return *ptr; }
    inline void storeRelaxed(uint32 * ptr, uint32 value) { *ptr = value; }

    inline uint32 loadAcquire(const volatile uint32 * ptr)
    {
        nvDebugCheck((intptr_t(ptr) & 3) == 0);

#if POSH_CPU_X86 || POSH_CPU_X86_64
        uint32 ret = *ptr;  // on x86, loads are Acquire
        nvCompilerReadBarrier();
        return ret;
#elif POSH_CPU_STRONGARM || POSH_CPU_AARCH64
        // need more specific cpu type for armv7?
        // also utilizes a full barrier
        // currently treating laod like x86 - this could be wrong
        
        // this is the easiest but slowest way to do this
        nvCompilerReadWriteBarrier();
		uint32 ret = *ptr; // replace with ldrex?
        nvCompilerReadWriteBarrier();
        return ret;
#elif POSH_CPU_PPC64
        // need more specific cpu type for ppc64?
        // also utilizes a full barrier
        // currently treating load like x86 - this could be wrong

        // this is the easiest but slowest way to do this
        nvCompilerReadWriteBarrier();
		uint32 ret = *ptr; // replace with ldrex?
        nvCompilerReadWriteBarrier();
        return ret;
#else
#error "Not implemented"
#endif
    }

    inline void storeRelease(volatile uint32 * ptr, uint32 value)
    {
        nvDebugCheck((intptr_t(ptr) & 3) == 0);
        nvDebugCheck((intptr_t(&value) & 3) == 0);

#if POSH_CPU_X86 || POSH_CPU_X86_64
        nvCompilerWriteBarrier();
        *ptr = value;   // on x86, stores are Release
        //nvCompilerWriteBarrier(); // @@ IC: Where does this barrier go? In nvtt it was after, in Witness before. Not sure which one is right.
#elif POSH_CPU_STRONGARM || POSH_CPU_AARCH64
        // this is the easiest but slowest way to do this
        nvCompilerReadWriteBarrier();
		*ptr = value; //strex?
		nvCompilerReadWriteBarrier();
#elif POSH_CPU_PPC64
        // this is the easiest but slowest way to do this
        nvCompilerReadWriteBarrier();
		*ptr = value; //strex?
		nvCompilerReadWriteBarrier();
#else
#error "Atomics not implemented."
#endif
    }


    template <typename T>
    inline void storeReleasePointer(volatile T * pTo, T from)
    {
        NV_COMPILER_CHECK(sizeof(T) == sizeof(intptr_t));
        nvDebugCheck((((intptr_t)pTo) % sizeof(intptr_t)) == 0);
        nvDebugCheck((((intptr_t)&from) % sizeof(intptr_t)) == 0);
        nvCompilerWriteBarrier();
        *pTo = from;    // on x86, stores are Release
    }

    template <typename T>
    inline T loadAcquirePointer(volatile T * ptr)
    {
        NV_COMPILER_CHECK(sizeof(T) == sizeof(intptr_t));
        nvDebugCheck((((intptr_t)ptr) % sizeof(intptr_t)) == 0);
        T ret = *ptr;   // on x86, loads are Acquire
        nvCompilerReadBarrier();
        return ret;
    } 


    // Atomics. @@ Assuming sequential memory order?

#if NV_CC_MSVC
    NV_COMPILER_CHECK(sizeof(uint32) == sizeof(long));

    // Returns incremented value.
    inline uint32 atomicIncrement(uint32 * value)
    {
        nvDebugCheck((intptr_t(value) & 3) == 0);
        return uint32(_InterlockedIncrement((long *)value));
    }

    // Returns decremented value.
    inline uint32 atomicDecrement(uint32 * value)
    {
        nvDebugCheck((intptr_t(value) & 3) == 0);
        return uint32(_InterlockedDecrement((long *)value));
    }

    // Returns added value.
    inline uint32 atomicAdd(uint32 * value, uint32 value_to_add) {
        nvDebugCheck((intptr_t(value) & 3) == 0);
        return uint32(_InterlockedExchangeAdd((long*)value, (long)value_to_add)) + value_to_add;
    }

    // Returns original value before addition.
    inline uint32 atomicFetchAndAdd(uint32 * value, uint32 value_to_add) {
        nvDebugCheck((intptr_t(value) & 3) == 0);
        return uint32(_InterlockedExchangeAdd((long*)value, (long)value_to_add));
    }




    // Compare '*value' against 'expected', if equal, then stores 'desired' in '*value'.
    // @@ C++0x style CAS? Unlike the C++0x version, 'expected' is not passed by reference and not mutated.
    // @@ Is this strong or weak? Does InterlockedCompareExchange have spurious failures?
    inline bool atomicCompareAndSwap(uint32 * value, uint32 expected, uint32 desired)
    {
        nvDebugCheck((intptr_t(value) & 3) == 0);
        long result = _InterlockedCompareExchange((long *)value, (long)desired, (long)expected);
        return result == (long)expected;
    }


    inline uint32 atomicSwap(uint32 * value, uint32 desired)
    {
        nvDebugCheck((intptr_t(value) & 3) == 0);
        return (uint32)_InterlockedExchange((long *)value, (long)desired);
    }



#elif NV_CC_CLANG && (NV_OS_IOS || NV_OS_DARWIN)

    //ACS: Use Apple's atomics instead? I don't know if these are better in any way; there are non-barrier versions too. There's no OSAtomicSwap32 tho'
    /*
    inline uint32 atomicIncrement(uint32 * value)
    {
        nvDebugCheck((intptr_t(value) & 3) == 0);
        return (uint32)OSAtomicIncrement32Barrier((int32_t *)value);
    }
    
    inline uint32 atomicDecrement(uint32 * value)
    {
        nvDebugCheck((intptr_t(value) & 3) == 0);
        return (uint32)OSAtomicDecrement32Barrier((int32_t *)value);
    }

    // Compare '*value' against 'expected', if equal, then stores 'desired' in '*value'.
    // @@ C++0x style CAS? Unlike the C++0x version, 'expected' is not passed by reference and not mutated.
    // @@ Is this strong or weak?
    inline bool atomicCompareAndSwap(uint32 * value, uint32 expected, uint32 desired)
    {
        nvDebugCheck((intptr_t(value) & 3) == 0);
        return OSAtomicCompareAndSwap32Barrier((int32_t)expected, (int32_t)desired, (int32_t *)value);
    }
    */

    // Returns incremented value.
    inline uint32 atomicIncrement(uint32 * value) {
        nvDebugCheck((intptr_t(value) & 3) == 0);
        return __sync_add_and_fetch(value, 1);
    }
    
    // Returns decremented value.
    inline uint32 atomicDecrement(uint32 * value) {
        nvDebugCheck((intptr_t(value) & 3) == 0);
        return __sync_sub_and_fetch(value, 1);
    }

    // Returns added value.
    inline uint32 atomicAdd(uint32 * value, uint32 value_to_add) {
        nvDebugCheck((intptr_t(value) & 3) == 0);
        return __sync_add_and_fetch(value, value_to_add);
    }

    // Returns original value before addition.
    inline uint32 atomicFetchAndAdd(uint32 * value, uint32 value_to_add) {
        nvDebugCheck((intptr_t(value) & 3) == 0);
        return __sync_fetch_and_add(value, value_to_add);
    }


    // Compare '*value' against 'expected', if equal, then stores 'desired' in '*value'.
    // @@ C++0x style CAS? Unlike the C++0x version, 'expected' is not passed by reference and not mutated.
    // @@ Is this strong or weak?
    inline bool atomicCompareAndSwap(uint32 * value, uint32 expected, uint32 desired)
    {
        nvDebugCheck((intptr_t(value) & 3) == 0);
        return __sync_bool_compare_and_swap(value, expected, desired);
    }
    
    inline uint32 atomicSwap(uint32 * value, uint32 desired)
    {
        nvDebugCheck((intptr_t(value) & 3) == 0);
        // this is confusingly named, it doesn't actually do a test but always sets
        return __sync_lock_test_and_set(value, desired);
    }




#elif NV_CC_CLANG && POSH_CPU_STRONGARM
    
    inline uint32 atomicIncrement(uint32 * value)
    {
        nvDebugCheck((intptr_t(value) & 3) == 0);
        
        // this should work in LLVM eventually, but not as of 2.1
        // return (uint32)AtomicIncrement((long *)value);
        
        // in the mean time,
        register uint32 result;
        asm volatile (
                      "1:   ldrexb  %0,  [%1]	\n\t"
                      "add     %0,   %0, #1     \n\t"
                      "strexb  r1,   %0, [%1]	\n\t"
                      "cmp     r1,   #0			\n\t"
                      "bne     1b"
                      : "=&r" (result)
                      : "r"(value)
                      : "r1"
                      );
        return result;

    }
    
    inline uint32 atomicDecrement(uint32 * value)
    {
        nvDebugCheck((intptr_t(value) & 3) == 0);
        
        // this should work in LLVM eventually, but not as of 2.1:
        // return (uint32)sys::AtomicDecrement((long *)value);

        // in the mean time,
        
        register uint32 result;
        asm volatile (
                      "1:   ldrexb  %0,  [%1]	\n\t"
                      "sub     %0,   %0, #1     \n\t"
                      "strexb  r1,   %0, [%1]	\n\t"
                      "cmp     r1,   #0			\n\t"
                      "bne     1b"
                      : "=&r" (result)
                      : "r"(value)
                      : "r1"
                      );
        return result;
         
    }

#elif NV_CC_GNUC
    // Many alternative implementations at:
    // http://www.memoryhole.net/kyle/2007/05/atomic_incrementing.html

    // Returns incremented value.
    inline uint32 atomicIncrement(uint32 * value) {
        nvDebugCheck((intptr_t(value) & 3) == 0);
        return __sync_add_and_fetch(value, 1);
    }

    // Returns decremented value.
    inline uint32 atomicDecrement(uint32 * value) {
        nvDebugCheck((intptr_t(value) & 3) == 0);
        return __sync_sub_and_fetch(value, 1);
    }

    // Returns added value.
    inline uint32 atomicAdd(uint32 * value, uint32 value_to_add) {
        nvDebugCheck((intptr_t(value) & 3) == 0);
        return __sync_add_and_fetch(value, value_to_add);
    }

    // Returns original value before addition.
    inline uint32 atomicFetchAndAdd(uint32 * value, uint32 value_to_add) {
        nvDebugCheck((intptr_t(value) & 3) == 0);
        return __sync_fetch_and_add(value, value_to_add);
    }

    // Compare '*value' against 'expected', if equal, then stores 'desired' in '*value'.
    // @@ C++0x style CAS? Unlike the C++0x version, 'expected' is not passed by reference and not mutated.
    // @@ Is this strong or weak?
    inline bool atomicCompareAndSwap(uint32 * value, uint32 expected, uint32 desired)
    {
        nvDebugCheck((intptr_t(value) & 3) == 0);
        return __sync_bool_compare_and_swap(value, expected, desired);
    }
    
    inline uint32 atomicSwap(uint32 * value, uint32 desired)
    {
        nvDebugCheck((intptr_t(value) & 3) == 0);
        // this is confusingly named, it doesn't actually do a test but always sets
        return __sync_lock_test_and_set(value, desired);
    }
    
#else
#error "Atomics not implemented."

#endif




    // It would be nice to have C++0x-style atomic types, but I'm not in the mood right now. Only uint32 supported so far.
#if 0
    template <typename T>
    void increment(T * value);

    template <typename T>
    void decrement(T * value);

    template <>
    void increment(uint32 * value) {
    }

    template <>
    void increment(uint64 * value) {
    }



    template <typename T>
    class Atomic
    {
    public:
        explicit Atomic()  : m_value() { }
        explicit Atomic( T val ) : m_value(val) { }
        ~Atomic() { }

        T loadRelaxed()  const { return m_value; }
        void storeRelaxed(T val) { m_value = val; }

        //T loadAcquire() const volatile { return nv::loadAcquire(&m_value); }
        //void storeRelease(T val) volatile { nv::storeRelease(&m_value, val); }

        void increment() /*volatile*/ { nv::atomicIncrement(m_value); }
        void decrement() /*volatile*/ { nv::atomicDecrement(m_value); }

        void compareAndStore(T oldVal, T newVal) { nv::atomicCompareAndStore(&m_value, oldVal, newVal); }
        T compareAndExchange(T oldVal, T newVal) { nv::atomicCompareAndStore(&m_value, oldVal, newVal); }
        T exchange(T newVal) { nv::atomicExchange(&m_value, newVal); }

    private:
        // don't provide operator = or == ; make the client write Store( Load() )
        NV_FORBID_COPY(Atomic);

        NV_COMPILER_CHECK(sizeof(T) == sizeof(uint32) || sizeof(T) == sizeof(uint64));

        T m_value;
    };
#endif

} // nv namespace 


#endif // NV_THREADS_ATOMICS_H
