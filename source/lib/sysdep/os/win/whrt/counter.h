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

/*
 * Interface for counter implementations
 */

#ifndef INCLUDED_COUNTER
#define INCLUDED_COUNTER

// derived implementations must be called CounterIMPL,
// where IMPL matches the WHRT_IMPL identifier. (see CREATE)
class ICounter
{
public:
	// (compiled-generated) ctor only sets up the vptr
	virtual ~ICounter() {}

	virtual const char* Name() const = 0;

	// Activate with an error return value is much cleaner+safer than
	// throwing exceptions in the ctor.
	virtual LibError Activate() = 0;
	virtual void Shutdown() = 0;

	virtual bool IsSafe() const = 0;

	/**
	 * @return the current value of the counter (all but the lower
	 * CounterBits() bits must be zero)
	 **/
	virtual u64 Counter() const = 0;

	// note: implementations need not cache the following; that's taken
	// care of by WHRT.

	/**
	 * @return the bit width of the counter (<= 64)
	 * WHRT uses this to ensure the counter (running at nominal frequency)
	 * doesn't overflow more than once during CALIBRATION_INTERVAL_MS.
	 **/
	virtual size_t CounterBits() const = 0;

	/**
	 * initial measurement of the tick rate. not necessarily correct
	 * (e.g. when using TSC: os_cpu_ClockFrequency isn't exact).
	 **/
	virtual double NominalFrequency() const = 0;

	/**
	 * actual resolution [s]. differs from 1/NominalFrequency if the
	 * timer adjustment is greater than 1 tick.
	 **/
	virtual double Resolution() const = 0;
};


/**
 * @return a newly created ICounter of type <id> or 0 iff the ID is invalid.
 * @param id integer ID (0..N-1)
 *
 * there can only be one active counter at a time; the previous one must
 * have been destroyed before creating another!
 **/
extern ICounter* CreateCounter(size_t id);

/**
 * shut down the counter, free its resources and zero its pointer.
 **/
extern void DestroyCounter(ICounter*& counter);

#endif	// #ifndef INCLUDED_COUNTER
