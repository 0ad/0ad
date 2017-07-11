/* Copyright (C) 2010 Wildfire Games.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "precompiled.h"

#if 0

// we need a means of measuring performance, since it is hard to predict and
// depends on many factors. to cover a wider range of configurations, this
// must also be possible on end-user systems lacking specialized developer
// tools. therefore, we must ship our own implementation; this complements
// Intel VTune et al.
//
// there are 3 approaches to the problem:
// - single-step analysis logs every executed instruction. very thorough, but
//   intolerably slow (~1000x) and not suitable for performance measurement.
// - intrusive measuring tracks execution time of explicitly marked
//   functions or 'zones'. more complex, requires adding code, and
//   inaccurate when thread switches are frequent.
// - IP sampling records the current instruction pointer at regular
//   intervals; slow sections of code will over time appear more often.
//   not exact, but simple and low-overhead.
//
// we implement IP sampling due to its simplicity. an intrusive approach
// might also be added later to account for performance per-module
// (helps spot the culprit in case hotspots are called from multiple sites).


// on Windows, we retrieve the current IP with GetThreadContext. dox require
// this to happen from another thread, and for the target to be suspended
// (now enforced by XP SP2). this leads to all sorts of problems:
// - if the suspended thread was dispatching an exception in the kernel,
//   register state may be a mix between the correct values and
//   those captured from the exception.
// - if running on Win9x with real-mode drivers, interrupts may interfere
//   with GetThreadContext. however, it's not supported anyway due to other
//   deficiencies (e.g. lack of proper mmap support).
// - the suspended thread may be holding locks; we need to be extremely
//   careful to avoid deadlock! many win api functions acquire locks in
//   non-obvious ways.

static HANDLE prof_target_thread;

static pthread_t prof_thread;

// delay [ms] between samples. OS sleep timers usually provide only
// ms resolution. increasing interval reduces overhead and accuracy.
static const int PROFILE_INTERVAL_MS = 1;


static uintptr_t get_target_pc()
{
	DWORD ret;
	HANDLE hThread = prof_target_thread;	// convenience

	ret = SuspendThread(hThread);
	if(ret == (DWORD)-1)
	{
		DEBUG_WARN_ERR(ERR::LOGIC);	// get_target_pc: SuspendThread failed
		return 0;
	}
	// note: we don't need to call more than once: this increments a DWORD
	// 'suspend count'; target is guaranteed to be suspended unless
	// the function failed.

	/////////////////////////////////////////////

	// be VERY CAREFUL to avoid anything that may acquire a lock until
	// after ResumeThread! this includes locks taken by the OS,
	// e.g. malloc -> heap or GetProcAddress -> loader.
	// reason is, if the target thread was holding a lock we try to
	// acquire here, a classic deadlock results.

	uintptr_t pc = 0;	// => will return 0 if GetThreadContext fails

	CONTEXT context;
	context.ContextFlags = CONTEXT_CONTROL;
	if(GetThreadContext(hThread, &context))
		pc = context.PC_;

	/////////////////////////////////////////////

	ret = ResumeThread(hThread);
	ENSURE(ret != 0);
	// don't fail (we have a valid PC), but warn

	return pc;
}


static pthread_t thread;
static sem_t exit_flag;

static void* prof_thread_func(void* UNUSED(data))
{
	debug_SetThreadName("eip_sampler");

	const long _1e6 = 1000000;
	const long _1e9 = 1000000000;

	for(;;)
	{
		// calculate absolute timeout for sem_timedwait
		struct timespec abs_timeout;
		clock_gettime(CLOCK_REALTIME, &abs_timeout);
		abs_timeout.tv_nsec += PROFILE_INTERVAL_MS * _1e6;
		// .. handle nanosecond wraparound (must not be > 1000m)
		if(abs_timeout.tv_nsec >= _1e9)
		{
			abs_timeout.tv_nsec -= _1e9;
			abs_timeout.tv_sec++;
		}

		errno = 0;
		// if we acquire the semaphore, exit was requested.
		if(sem_timedwait(&exit_flag, &abs_timeout) == 0)
			break;
		// actual error: warn
		if(errno != ETIMEDOUT)
			DEBUG_WARN_ERR(ERR::LOGIC);	// wpcu prof_thread_func: sem_timedwait failed

		uintptr_t pc = get_target_pc();
		UNUSED2(pc);

		// ADD TO LIST
	}

	return 0;
}



// call from thread that is to be profiled
Status prof_start()
{
	// we need a real HANDLE to the target thread for use with
	// Suspend|ResumeThread and GetThreadContext.
	// alternative: DuplicateHandle on the current thread pseudo-HANDLE.
	// this way is a bit more obvious/simple.
	const DWORD access = THREAD_GET_CONTEXT|THREAD_SUSPEND_RESUME;
	HANDLE hThread = OpenThread(access, FALSE, GetCurrentThreadId());
	if(hThread == INVALID_HANDLE_VALUE)
		WARN_RETURN(ERR::FAIL);

	prof_target_thread = hThread;

	sem_init(&exit_flag, 0, 0);
	pthread_create(&thread, 0, prof_thread_func, 0);
	return INFO::OK;
}

Status prof_shutdown()
{
	WARN_IF_FALSE(CloseHandle(prof_target_thread));
	return INFO::OK;
}



/*
open question: how to store the EIP values returned? some background:
the mechanism above churns out an EIP value (may be in our process, but might
also be bogus); we need to store it somehow pending analysis.

when done with the current run, we'd want to resolve EIP -> function name,
source file etc. (rather slow, so don't do it at runtime).

so, how to store it in the meantime? 2 possibilities:
- simple array/vector of addresses (of course optimized to reduce allocs)
- fixed size array of 'bins' (range of addresses; may be as fine as 1 byte);
each bin has a counter which is incremented when the bin's corresponding
address has been hit.

it's a size tradeoff here; for simple runs of < 1 min (60,000 ms), #1
would use 240kb of mem. #2 requires sizeof_whole_program * bytes_per_counter
up front, and has problems measuring DLLs (we'd have to explicitly map
the DLL address range into a bin - ugh). however, if we ever want to
test for say an hour (improves accuracy of profiling due to larger sample size),
#1 would guzzle 15mb of memory.

hm, another idea would be to write out #1's list of addresses periodically.
to make sure the disk I/O doesn't come at a bad time, we could have the main
thread call into the profiler and request it write out at that time.
this would require extreme caution to avoid the deadlock problem, but looks
doable.

-------- [2] ----------

realistic profiler runs will take up to an hour.

writing out to disk would work: could have main thread call back.
that and adding EIP to list would be atomic (locked).
BUT: large amount of data, that's bad (loading at 30mb/s => 500ms load time alone)

problem with enumerating all symbols at startup: how do we enum all DLLs?

hybrid idea: std::map of EIPs. we don't build the map at startup,
but add when first seen and subsequently increment counter stored there.
problem: uses more memory/slower access than list.
would have to make sure EIPs are reused.
to help that, could quantize down to 4 byte (or so) bins.
accessing debug information at runtime to determine function length is too slow.

maybe some weird data structure: one bucket controls say 256 bytes of code
bucket is found by stripping off lower 8 bits. then, store only
the hit count for that byte. where's the savings over normal count?

TODO: what if the thread is sleeping at the time we query EIP?
can't detect that - suspend count is only set by SuspendThread
do we want to report that point (it's good to know), or try to access other threads?

TODO split off target thread / get PC into sysdep; profiler thread is portable!


at exit: resolve list to hotspots
probably hard; a start would be just the function in which the address is, then hit count


==========================================


*/
#endif
