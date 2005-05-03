// FIFO queue of load 'functors' with time limit; enables displaying
// load progress without resorting to threads (complicated).
//
// Jan Wassenberg, initial implementation finished 2005-03-21
// jan@wildfiregames.com


// intended use:
// replace the InitEverything function with the following:
//   LDR_BeginRegistering()
//   LDR_Register() for each sub-function (*)
//   LDR_EndRegistering()
// then in the main loop, call LDR_ProgressiveLoad().
//
// *: splitting up InitEverything is required so that control returns to
// the main loop occasionally; that allows displaying the progress.
// note that we can't interrupt loading without threads (complex).
//
// this module is not thread-safe!


#include <wchar.h>


// call before starting to register load requests.
// this routine is provided so we can prevent 2 simultaneous load operations,
// which is bogus. that can happen by clicking the load button quickly,
// or issuing via console while already loading.
extern int LDR_BeginRegistering();


// callback function of a load request; performs the actual work.
// it receives a param (see below) and the exact time remaining [s].
//
// return semantics:
// - if the entire task was successfully completed, return 0:
//   the load request will then be de-queued.
// - if the work can be split into smaller subtasks, process those until
//   <time_left> is reached or exceeded and then return an estimate
//   of progress in percent (> 0 or it's treated as "finished").
// - on failure, return a negative error code; LDR_ProgressiveLoad
//   will abort immediately and return that.
typedef int (*LoadFunc)(void* param, double time_left);

// register a load request (later processed in FIFO order).
// <func>: function that will perform the actual work; see LoadFunc.
// <param>: (optional) parameter/persistent state; must be freed by func.
// <description>: user-visible description of the current task, e.g.
//   "Loading map".
// <estimated_duration_ms>: used to calculate progress, and when checking
//   whether there is enough of the time budget left to process this task
//   (reduces timeslice overruns, making the main loop more responsive).
extern int LDR_Register(LoadFunc func, void* param, const wchar_t* description,
	int estimated_duration_ms);


// call when finished registering load requests; subsequent calls to
// LDR_ProgressiveLoad will then work off the queued entries.
extern int LDR_EndRegistering();


// immediately cancel the load. note: no special notification will be
// returned by LDR_ProgressiveLoad.
extern int LDR_Cancel();


// process as many of the queued load requests as possible within
// <time_budget> [s]. if a request is lengthy, the budget may be exceeded.
// call from the main loop.
//
// passes back a description of the next task that will be undertaken
// ("" if finished) and the progress value established by the
// last request to complete.
//
// return semantics:
// - if loading just completed, return 0.
// - if loading is in progress but didn't finish, return ERR_TIMED_OUT.
// - if not currently loading (no-op), return 1.
// - any other value indicates a failure; the request has been de-queued.
//
// string interface rationale: for better interoperability, we avoid C++
// std::wstring and PS CStr. since the registered description may not be
// persistent, we can't just store a pointer. returning a pointer to
// our copy of the description doesn't work either, since it's freed when
// the request is de-queued. that leaves writing into caller's buffer.
extern int LDR_ProgressiveLoad(double time_budget, wchar_t* next_description,
	size_t max_chars, int* progress_percent);

// immediately process all queued load requests.
// returns 0 on success, something else on failure.
extern int LDR_NonprogressiveLoad();