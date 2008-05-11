// FIFO queue of load 'functors' with time limit; enables displaying
// load progress without resorting to threads (complicated).

#ifndef INCLUDED_LOADER
#define INCLUDED_LOADER

#include <wchar.h>

/*

[KEEP IN SYNC WITH WIKI!]

Overview
--------

"Loading" is the act of preparing a game session, including reading all
required data from disk. Ideally, this would be very quick, but for complex
maps and/or low-end machines, a duration of several seconds can be expected.
Having the game freeze that long is unacceptable; instead, we want to display
the current progress and task, which greatly increases user patience.


Allowing for Display
--------------------

To display progress, we need to periodically 'interrupt' loading.
Threads come to mind, but there is a problem: since OpenGL graphics calls
must come from the main thread, loading would have to happen in a
background thread. Everything would thus need to be made thread-safe,
which is a considerable complication.

Therefore, we load from a single thread, and split the operation up into
"tasks" (as short as possible). These are typically function calls from the
old InitEverything(); instead of being called directly, they are registered
with our queue. We are called from the main loop and process as many tasks
as possible within one "timeslice".

After that, progress is updated: an estimated duration for each task
(derived from timings on one machine) is used to calculate headway.
As long as task lengths only differ by a constant factor between machines,
this timing is exact; even if not, only smoothness of update suffers.


Interrupting Lengthy Tasks
--------------------------

The above is sufficient for basic needs, but breaks down if tasks are long
(> 500ms). To fix this, we will need to modify the tasks themselves:
either make them coroutines, i.e. have them return to the main loop and then
resume where they left off, or re-enter a limited version of the main loop.
The former requires persistent state and careful implementation,
but yields significant advantages:
- progress calculation is easy and smooth,
- all services of the main loop (especially input*) are available, and
- complexity due to reentering the main loop is avoided.

* input is important, since we want to be able to abort long loads or
even exit the game immediately.

We therefore go with the 'coroutine' (more correctly 'generator') approach.
Examples of tasks that take so long and typical implementations may
be seen in MapReader.cpp.


Intended Use
------------

Replace the InitEverything() function with the following:
  LDR_BeginRegistering();
  LDR_Register(..) for each sub-function (*)
  LDR_EndRegistering();
Then in the main loop, call LDR_ProgressiveLoad().

* RegMemFun from LoaderThunks.h may be used instead; it takes care of
registering member functions, which would otherwise be messy.

*/


// NOTE: this module is not thread-safe!


// call before starting to register tasks.
// this routine is provided so we can prevent 2 simultaneous load operations,
// which is bogus. that can happen by clicking the load button quickly,
// or issuing via console while already loading.
extern LibError LDR_BeginRegistering();


// callback function of a task; performs the actual work.
// it receives a param (see below) and the exact time remaining [s].
//
// return semantics:
// - if the entire task was successfully completed, return 0;
//   it will then be de-queued.
// - if the work can be split into smaller subtasks, process those until
//   <time_left> is reached or exceeded and then return an estimate
//   of progress in percent (<= 100, otherwise it's a warning;
//   != 0, or it's treated as "finished")
// - on failure, return a negative error code or 'warning' (see above);
//   LDR_ProgressiveLoad will abort immediately and return that.
typedef int (*LoadFunc)(void* param, double time_left);

// register a task (later processed in FIFO order).
// <func>: function that will perform the actual work; see LoadFunc.
// <param>: (optional) parameter/persistent state; must be freed by func.
// <description>: user-visible description of the current task, e.g.
//   "Loading Textures".
// <estimated_duration_ms>: used to calculate progress, and when checking
//   whether there is enough of the time budget left to process this task
//   (reduces timeslice overruns, making the main loop more responsive).
extern LibError LDR_Register(LoadFunc func, void* param, const wchar_t* description,
	int estimated_duration_ms);


// call when finished registering tasks; subsequent calls to
// LDR_ProgressiveLoad will then work off the queued entries.
extern LibError LDR_EndRegistering();


// immediately cancel this load; no further tasks will be processed.
// used to abort loading upon user request or failure.
// note: no special notification will be returned by LDR_ProgressiveLoad.
extern LibError LDR_Cancel();


// process as many of the queued tasks as possible within <time_budget> [s].
// if a task is lengthy, the budget may be exceeded. call from the main loop.
//
// passes back a description of the next task that will be undertaken
// ("" if finished) and the current progress value.
//
// return semantics:
// - if the final load task just completed, return INFO::ALL_COMPLETE.
// - if loading is in progress but didn't finish, return ERR::TIMED_OUT.
// - if not currently loading (no-op), return 0.
// - any other value indicates a failure; the request has been de-queued.
//
// string interface rationale: for better interoperability, we avoid C++
// std::wstring and PS CStr. since the registered description may not be
// persistent, we can't just store a pointer. returning a pointer to
// our copy of the description doesn't work either, since it's freed when
// the request is de-queued. that leaves writing into caller's buffer.
extern LibError LDR_ProgressiveLoad(double time_budget, wchar_t* next_description, size_t max_chars, int* progress_percent);

// immediately process all queued load requests.
// returns 0 on success or a negative error code.
extern LibError LDR_NonprogressiveLoad();


// boilerplate check-if-timed-out and return-progress-percent code.
// completed_jobs and total_jobs are ints and must be updated by caller.
// assumes presence of a local variable (double)<end_time>
// (as returned by timer_Time()) that indicates the time at which to abort.
#define LDR_CHECK_TIMEOUT(completed_jobs, total_jobs)\
	if(timer_Time() > end_time)\
	{\
		size_t progress_percent = ((completed_jobs)*100 / (total_jobs));\
		/* 0 means "finished", so don't return that! */\
		if(progress_percent == 0)\
			progress_percent = 1;\
		debug_assert(0 < progress_percent && progress_percent <= 100);\
		return (int)progress_percent;\
	}

#endif	// #ifndef INCLUDED_LOADER
