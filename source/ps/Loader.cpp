#include "precompiled.h"

#include <deque>

#include "lib.h"	// error codes
#include "timer.h"
#include "loader.h"
#include "CStr.h"

// note: not thread-safe!


// need to maintain this counter ourselves so we can reset after each load.
static int progress_percent = 0;

// main purpose is to indicate whether a load is in progress, so that
// LDR_ProgressiveLoad can return 0 iff loading just completed.
// the REGISTERING state allows us to detect 2 simultaneous loads (bogus).
static enum
{
	IDLE,
	REGISTERING,
	LOADING,
}
state = IDLE;


// holds all state for one load request; stored in queue.
struct LoadRequest
{
	// member documentation is in LDR_Register (avoid duplication).

	LoadFunc func;
	void* param;

	const CStrW description;
		// rationale:
		// - don't just store a pointer - the caller's string may be volatile.
		// - the module interface must work in C, so we get/set as wchar_t*.

	int progress_percent_after_completion;

	int estimated_duration_ms;

	// LDR_Register gets these as parameters; pack everything together.
	LoadRequest(LoadFunc func_, void* param_, const wchar_t* desc_, int pc_, int ms_)
		: func(func_), param(param_), description(desc_),
		  progress_percent_after_completion(pc_), estimated_duration_ms(ms_)
	{
	}
};

typedef std::deque<const LoadRequest> LoadRequests;
static LoadRequests load_requests;


// call before starting to register load requests.
// this routine is provided so we can prevent 2 simultaneous load operations,
// which is bogus. that can happen by clicking the load button quickly,
// or issuing via console while already loading.
int LDR_BeginRegistering()
{
	if(state != IDLE)
		return -1;

	state = REGISTERING;
	load_requests.clear();
	return 0;
}


// register a load request (later processed in FIFO order).
// <func>: function that will perform the actual work; see LoadFunc.
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
int LDR_Register(LoadFunc func, void* param, const wchar_t* description,
	int progress_percent_after_completion, int estimated_duration_ms)
{
	if(state != REGISTERING)
		return -1;

	const LoadRequest lr(func, param, description,
		progress_percent_after_completion, estimated_duration_ms);
	load_requests.push_back(lr);
	return 0;
}


// call when finished registering load requests; subsequent calls to
// LDR_ProgressiveLoad will then work off the queued entries.
int LDR_EndRegistering()
{
	if(state != REGISTERING)
		return -1;

	state = LOADING;
	progress_percent = 0;
	return 0;
}


// immediately cancel the load. note: no special notification will be
// returned by LDR_ProgressiveLoad.
int LDR_Cancel()
{
	// note: calling during registering doesn't make sense - that
	// should be an atomic sequence of begin, register [..], end.
	if(state != LOADING)
		return -1;

	state = IDLE;
	// the queue doesn't need to be emptied now; that'll happen during the
	// next LDR_StartRegistering. for now, it is sufficient to set the
	// state, so that LDR_ProgressiveLoad is a no-op.
	return 0;
}


// helper routine for LDR_ProgressiveLoad.
// tries to prevent starting a long task when at the end of a timeslice.
static bool HaveTimeForNextTask(double time_left, double time_budget, int estimated_duration_ms)
{
	// have already exceeded our time budget => stop for now.
	if(time_left <= 0.0)
		return false;

	// we've already used up more than 60%
	// (if it's less than that, we won't check the next task length)
	if(time_left < 0.40*time_budget)
	{
		const double estimated_duration = estimated_duration_ms * 1e-3;
		// .. next task is expected to be long - do it next call
		if(estimated_duration > time_left + time_budget*0.20)
			return false;
	}

	return true;
}


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
int LDR_ProgressiveLoad(double time_budget, wchar_t* current_description,
	size_t max_chars, int* progress_percent_)
{
	// we're called unconditionally from the main loop, so this isn't
	// an error; there is just nothing to do.
	if(state != LOADING)
		return 1;

	const double end_time = get_time() + time_budget;

	// in case it's never set below (because all LoadRequests have
	// progress_percent_after_completion = 0)
	*progress_percent_ = progress_percent;

	// (function will return immediately on failure or timeout)
	while(!load_requests.empty())
	{
		const double time_left = end_time - get_time();
		const LoadRequest& lr = load_requests.front();

		// latch description of the current task now (it may be removed below)
		wcscpy_s(current_description, max_chars, lr.description);

		// do actual work of loading
		int ret = lr.func(lr.param, time_left);
		// .. either finished entirely, or failed => remove from queue
		if(ret != ERR_TIMED_OUT)
			load_requests.pop_front();
		// .. failed or timed out => abort immediately; loading will
		// continue when we're called in the next iteration of the main loop.
		// rationale: bail immediately instead of remembering the first error
		// that came up, so that we report can all errors that happen.
		if(ret != 0)
			return ret;
		// .. completed normally:

		// update progress
		const int new_pc = lr.progress_percent_after_completion;
		if(new_pc)
		{
			assert(new_pc > progress_percent);
			*progress_percent_ = progress_percent = new_pc;
		}

		// check if we're out of time; take into account next task length.
		// note: do this at the end of the loop to make sure there's
		// progress even if the timer is low-resolution (=> time_left = 0).
		if(!HaveTimeForNextTask(time_left, time_budget, lr.estimated_duration_ms))
			return ERR_TIMED_OUT;
	}

	// queue is empty, we just finished.
	state = IDLE;
	assert(progress_percent == 100);
	return 0;
}
