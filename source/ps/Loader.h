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
// - if the work can be split into smaller subtasks, process those until
//   <time_left> is reached or exceeded and then return ERR_TIMED_OUT.
// - if the entire task was successfully completed, return 0:
//   the load request will then be de-queued.
// - any other return value indicates failure and causes
//   LDR_ProgressiveLoad to immediately abort and return that.
typedef int (*LoadFunc)(void* param, double time_left);

// register a load request (later processed in FIFO order).
// <func>: function that will perform the actual work; see LoadFunc above.
// <param>: (optional) parameter/persistent state; must be freed by func.
// <description>: user-visible description of the current task, e.g.
//   "Loading map".
// <progress_percent_after_completion>: optional; if non-zero, progress is
//   set to this value after the current task completes.
//   must increase monotonically.
// <estimated_duration_ms>: optional; if non-zero, this task will be
//   postponed until the next LDR_ProgressiveLoad timeslice if there's not
//   much time left. this reduces overruns of the timeslice => main loop is
//   more responsive.
extern int LDR_Register(LoadFunc func, void* param, const wchar_t* description,
	int progress_percent_after_completion = 0, int estimated_duration_ms = 0);

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
// passes back a description of the last task undertaken and the progress
// value established by the last request to complete.
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
extern int LDR_ProgressiveLoad(double time_budget, wchar_t* current_description,
	size_t max_chars, int* progress_percent);
