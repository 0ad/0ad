// FIFO queue of load 'functors' with time limit; enables displaying
// load progress without resorting to threads (complicated).
//
// Jan Wassenberg, initial implementation finished 2005-03-21
// jan@wildfiregames.com

#include "precompiled.h"

#include <deque>
#include <functional>

#include "lib.h"	// error codes
#include "timer.h"
#include "CStr.h"
#include "loader.h"


// need a persistent counter so we can reset after each load,
// and incrementally add "estimated/total".
static int progress_percent = 0;

// set by LDR_EndRegistering; used for progress % calculation. may be 0.
static double total_estimated_duration;

// used by LDR_ProgressiveLoad to add up the duration of requests that
// time out by themselves (and therefore may be split across multiple calls)
static double request_duration;

// main purpose is to indicate whether a load is in progress, so that
// LDR_ProgressiveLoad can return 0 iff loading just completed.
// the REGISTERING state allows us to detect 2 simultaneous loads (bogus);
// FIRST_LOAD is used to skip the first timeslice (see LDR_ProgressiveLoad).
static enum
{
	IDLE,
	REGISTERING,
	FIRST_LOAD,
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
		// rationale for storing as CStrW here:
		// - needs to be wide because it's user-visible and will be translated.
		// - don't just store a pointer - the caller's string may be volatile.
		// - the module interface must work in C, so we get/set as wchar_t*.

	int estimated_duration_ms;

	// LDR_Register gets these as parameters; pack everything together.
	LoadRequest(LoadFunc func_, void* param_, const wchar_t* desc_, int ms_)
		: func(func_), param(param_), description(desc_),
		  estimated_duration_ms(ms_)
	{
	}
};

typedef std::deque<const LoadRequest> LoadRequests;
static LoadRequests load_requests;

// std::accumulate binary op; used by LDR_EndRegistering to sum up all
// estimated durations (for % progress calculation)
struct DurationAdder: public std::binary_function<double, const LoadRequest&, double>
{
	double operator()(double partial_result, const LoadRequest& lr) const
	{
		return partial_result + lr.estimated_duration_ms*1e-3;
	}
};


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
// <estimated_duration_ms>: used to calculate progress, and when checking
//   whether there is enough of the time budget left to process this task
//   (reduces timeslice overruns, making the main loop more responsive).
int LDR_Register(LoadFunc func, void* param, const wchar_t* description,
	int estimated_duration_ms)
{
	if(state != REGISTERING)
	{
		debug_warn("LDR_Register: not called between LDR_(Begin|End)Register - why?!");
			// warn here instead of relying on the caller to CHECK_ERR because
			// there will be lots of call sites spread around.
		return -1;
	}

	const LoadRequest lr(func, param, description, estimated_duration_ms);
	load_requests.push_back(lr);
	return 0;
}


// call when finished registering load requests; subsequent calls to
// LDR_ProgressiveLoad will then work off the queued entries.
int LDR_EndRegistering()
{
	if(state != REGISTERING)
		return -1;

	if(load_requests.empty())
		debug_warn("LDR_EndRegistering: no LoadRequests queued");

	state = FIRST_LOAD;
	progress_percent = 0;
	total_estimated_duration = std::accumulate(load_requests.begin(), load_requests.end(), 0.0, DurationAdder());
	request_duration = 0.0;
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
	// have already exceeded our time budget
	if(time_left <= 0.0)
		return false;

	// check next task length. we want a lengthy task to happen in its own
	// timeslice so that its description is displayed beforehand.
	const double estimated_duration = estimated_duration_ms*1e-3;
	if(time_left+estimated_duration > time_budget*1.20)
		return false;

	return true;
}


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
int LDR_ProgressiveLoad(double time_budget, wchar_t* description_,
	size_t max_chars, int* progress_percent_)
{
	int ret;	// single exit; this is returned
	double time_left = time_budget;

	// don't do any work the first call so that a graphics update
	// happens before the first (probably lengthy) timeslice.
	if(state == FIRST_LOAD)
	{
		state = LOADING;
		ret = ERR_TIMED_OUT;	// make caller think we did something
		goto done;
	}

	// we're called unconditionally from the main loop, so this isn't
	// an error; there is just nothing to do.
	if(state != LOADING)
		return 1;

	while(!load_requests.empty())
	{
		// do actual work of loading
		const LoadRequest& lr = load_requests.front();
		const double t0 = get_time();
		ret = lr.func(lr.param, time_left);
		const double elapsed_time = get_time() - t0;

		// time accounting
		time_left -= elapsed_time;
		request_duration += elapsed_time;
		wcscpy_s(description_, max_chars, lr.description);	// HACK, used below

		// .. either finished entirely, or failed => remove from queue
		if(ret != ERR_TIMED_OUT)
			load_requests.pop_front();
		// .. failed or timed out => abort immediately; loading will
		// continue when we're called in the next iteration of the main loop.
		// rationale: bail immediately instead of remembering the first error
		// that came up, so that we report can all errors that happen.
		if(ret != 0)
			goto done;
		// .. completed normally => update progress
		//    note: during development, estimates won't yet be set,
		//    so allow this to be 0 and don't fail in LDR_EndRegistering
		if(total_estimated_duration != 0.0)	// prevent division by zero
		{
			const double fraction = lr.estimated_duration_ms*1e-3 / total_estimated_duration;
			progress_percent += (int)(fraction * 100.0);
			assert(0 <= progress_percent && progress_percent <= 100);
		}
		debug_out("LOADER: completed %ls in %f ms\n", description_, elapsed_time*1e3);
		request_duration = 0.0;

		// check if we're out of time; take into account next task length.
		// note: do this at the end of the loop to make sure there's
		// progress even if the timer is low-resolution (=> time_left = 0).
		if(!HaveTimeForNextTask(time_left, time_budget, lr.estimated_duration_ms))
		{
			ret = ERR_TIMED_OUT;
			goto done;
		}
	}

	// queue is empty, we just finished.
	state = IDLE;
	ret = 0;


	// set output params (there are several return points above)
done:
	*progress_percent_ = progress_percent;
	// we want the next task, instead of what just completed:
	// it will be displayed during the next load phase.
	const wchar_t* description = L"";	// assume finished
	if(!load_requests.empty())
		description = load_requests.front().description.c_str();
	wcscpy_s(description_, max_chars, description);

	debug_out("LDR_ProgressiveLoad RETURNING; desc=%ls progress=%d\n", description_, progress_percent);

	return ret;
}


// immediately process all queued load requests.
// returns 0 on success, something else on failure.
int LDR_NonprogressiveLoad()
{
	int progress_percent;
	wchar_t description[100];
	int ret;
	
	while(1)
	{
		ret = LDR_ProgressiveLoad(100.f, description, ARRAY_SIZE(description), &progress_percent);

		switch(ret)
		{
		case 1: debug_warn("NonprogressiveLoad: No load in progress"); return 0;
		case 0: return 0; // success
		case ERR_TIMED_OUT: break; // try again
		default: CHECK_ERR(ret);
		}
	}
}