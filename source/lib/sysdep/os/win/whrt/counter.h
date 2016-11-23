/* Copyright (c) 2010 Wildfire Games
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
	virtual Status Activate() = 0;
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
 * @return a newly created ICounter of type \<id\> or 0 iff the ID is invalid.
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
