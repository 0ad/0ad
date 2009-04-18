/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

// FIFO queue of load 'functors' with time limit; enables displaying
// load progress without resorting to threads (complicated).

#include "precompiled.h"

#include <deque>
#include <numeric>

#include "lib/timer.h"
#include "CStr.h"
#include "Loader.h"
#include "LoaderThunks.h"


// set by LDR_EndRegistering; may be 0 during development when
// estimated task durations haven't yet been set.
static double total_estimated_duration;

// total time spent loading so far, set by LDR_ProgressiveLoad.
// we need a persistent counter so it can be reset after each load.
// this also accumulates less errors than:
// progress += task_estimated / total_estimated.
static double estimated_duration_tally;

// needed for report of how long each individual task took.
static double task_elapsed_time;

// main purpose is to indicate whether a load is in progress, so that
// LDR_ProgressiveLoad can return 0 iff loading just completed.
// the REGISTERING state allows us to detect 2 simultaneous loads (bogus);
// FIRST_LOAD is used to skip the first timeslice (see LDR_ProgressiveLoad).
static enum
{
	IDLE,
	REGISTERING,
	FIRST_LOAD,
	LOADING
}
state = IDLE;


// holds all state for one load request; stored in queue.
struct LoadRequest
{
	// member documentation is in LDR_Register (avoid duplication).

	LoadFunc func;
	void* param;

	CStrW description;
		// rationale for storing as CStrW here:
		// - needs to be WCS because it's user-visible and will be translated.
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

typedef std::deque<LoadRequest> LoadRequests;
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
void LDR_BeginRegistering()
{
	debug_assert(state == IDLE);

	state = REGISTERING;
	load_requests.clear();
}


// register a task (later processed in FIFO order).
// <func>: function that will perform the actual work; see LoadFunc.
// <param>: (optional) parameter/persistent state; must be freed by func.
// <description>: user-visible description of the current task, e.g.
//   "Loading Textures".
// <estimated_duration_ms>: used to calculate progress, and when checking
//   whether there is enough of the time budget left to process this task
//   (reduces timeslice overruns, making the main loop more responsive).
void LDR_Register(LoadFunc func, void* param, const wchar_t* description,
	int estimated_duration_ms)
{
	debug_assert(state == REGISTERING);	// must be called between LDR_(Begin|End)Register

	const LoadRequest lr(func, param, description, estimated_duration_ms);
	load_requests.push_back(lr);
}


// call when finished registering tasks; subsequent calls to
// LDR_ProgressiveLoad will then work off the queued entries.
void LDR_EndRegistering()
{
	debug_assert(state == REGISTERING);
	debug_assert(!load_requests.empty());

	state = FIRST_LOAD;
	estimated_duration_tally = 0.0;
	task_elapsed_time = 0.0;
	total_estimated_duration = std::accumulate(load_requests.begin(), load_requests.end(), 0.0, DurationAdder());
}


// immediately cancel this load; no further tasks will be processed.
// used to abort loading upon user request or failure.
// note: no special notification will be returned by LDR_ProgressiveLoad.
void LDR_Cancel()
{
	// note: calling during registering doesn't make sense - that
	// should be an atomic sequence of begin, register [..], end.
	debug_assert(state == LOADING);

	// the queue doesn't need to be emptied now; that'll happen during the
	// next LDR_StartRegistering. for now, it is sufficient to set the
	// state, so that LDR_ProgressiveLoad is a no-op.
	state = IDLE;
}


// helper routine for LDR_ProgressiveLoad.
// tries to prevent starting a long task when at the end of a timeslice.
static bool HaveTimeForNextTask(double time_left, double time_budget, int estimated_duration_ms)
{
	// have already exceeded our time budget
	if(time_left <= 0.0)
		return false;

	// we haven't started a request yet this timeslice. start it even if
	// it's longer than time_budget to make sure there is progress.
	if(time_left == time_budget)
		return true;

	// check next task length. we want a lengthy task to happen in its own
	// timeslice so that its description is displayed beforehand.
	const double estimated_duration = estimated_duration_ms*1e-3;
	if(time_left+estimated_duration > time_budget*1.20)
		return false;

	return true;
}


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
LibError LDR_ProgressiveLoad(double time_budget, wchar_t* description, size_t max_chars, int* progress_percent)
{
	LibError ret;	// single exit; this is returned
	double progress = 0.0;	// used to set progress_percent
	double time_left = time_budget;

	// don't do any work the first time around so that a graphics update
	// happens before the first (probably lengthy) timeslice.
	if(state == FIRST_LOAD)
	{
		state = LOADING;

		ret  = ERR::TIMED_OUT;	// make caller think we did something
		// progress already set to 0.0; that'll be passed back.
		goto done;
	}

	// we're called unconditionally from the main loop, so this isn't
	// an error; there is just nothing to do.
	if(state != LOADING)
		return INFO::OK;

	while(!load_requests.empty())
	{
		// get next task; abort if there's not enough time left for it.
		const LoadRequest& lr = load_requests.front();
		const double estimated_duration = lr.estimated_duration_ms*1e-3;
		if(!HaveTimeForNextTask(time_left, time_budget, lr.estimated_duration_ms))
		{
			ret  = ERR::TIMED_OUT;
			goto done;
		}

		// call this task's function and bill elapsed time.
		const double t0 = timer_Time();
		int status = lr.func(lr.param, time_left);
		const bool timed_out = ldr_was_interrupted(status);
		const double elapsed_time = timer_Time() - t0;
		time_left -= elapsed_time;
		task_elapsed_time += elapsed_time;

		// either finished entirely, or failed => remove from queue.
		if(!timed_out)
		{
			debug_printf("LOADER| completed %ls in %g ms; estimate was %g ms\n", lr.description.c_str(), task_elapsed_time*1e3, estimated_duration*1e3);
			task_elapsed_time = 0.0;
			estimated_duration_tally += estimated_duration;
			load_requests.pop_front();
		}

		// calculate progress (only possible if estimates have been given)
		if(total_estimated_duration != 0.0)
		{
			double current_estimate = estimated_duration_tally;

			// function interrupted itself; add its estimated progress.
			// note: monotonicity is guaranteed since we never add more than
			//   its estimated_duration_ms.
			if(timed_out)
				current_estimate += estimated_duration * status/100.0;

			progress = current_estimate / total_estimated_duration;
		}

		// do we need to continue?
		// .. function interrupted itself, i.e. timed out; abort.
		if(timed_out)
		{
			ret  = ERR::TIMED_OUT;
			goto done;
		}
		// .. failed; abort. loading will continue when we're called in
		//    the next iteration of the main loop.
		//    rationale: bail immediately instead of remembering the first
		//    error that came up so we report can all errors that happen.
		else if(status < 0)
		{
			ret = (LibError)status;
			goto done;
		}
		// .. succeeded; continue and process next queued task.
	}

	// queue is empty, we just finished.
	state = IDLE;
	ret = INFO::ALL_COMPLETE;


	// set output params (there are several return points above)
done:
	*progress_percent = (int)(progress * 100.0);
	debug_assert(0 <= *progress_percent && *progress_percent <= 100);

	// we want the next task, instead of what just completed:
	// it will be displayed during the next load phase.
	const wchar_t* new_description = L"";	// assume finished
	if(!load_requests.empty())
		new_description = load_requests.front().description.c_str();
	wcscpy_s(description, max_chars, new_description);

	debug_printf("LOADER| returning; desc=%ls progress=%d\n", description, *progress_percent);

	return ret;
}


// immediately process all queued load requests.
// returns 0 on success or a negative error code.
LibError LDR_NonprogressiveLoad()
{
	const double time_budget = 100.0;
		// large enough so that individual functions won't time out
		// (that'd waste time).
	wchar_t description[100];
	int progress_percent;

	for(;;)
	{
		LibError ret = LDR_ProgressiveLoad(time_budget, description, ARRAY_SIZE(description), &progress_percent);
		switch(ret)
		{
		case INFO::OK:
			debug_warn("No load in progress");
			return INFO::OK;
		case INFO::ALL_COMPLETE:
			return INFO::OK;
		case ERR::TIMED_OUT:
			break;			// continue loading
		default:
			CHECK_ERR(ret);	// failed; complain
		}
	}
}
