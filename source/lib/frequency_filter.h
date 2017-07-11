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

#ifndef INCLUDED_FREQUENCY_FILTER
#define INCLUDED_FREQUENCY_FILTER

// calculate frequency of events (tuned for 100 Hz)
struct IFrequencyFilter
{
	virtual ~IFrequencyFilter() {}

	virtual void Update(double value) = 0;

	// smoothed but rapidly tracked frequency
	virtual double SmoothedFrequency() const = 0;

	// stable, non-fluctuating value for user display
	virtual int StableFrequency() const = 0;
};

typedef shared_ptr<IFrequencyFilter> PIFrequencyFilter;

// expectedFrequency is a guess that hopefully speeds up convergence
LIB_API PIFrequencyFilter CreateFrequencyFilter(double resolution, double expectedFrequency);

#endif	// #ifndef INCLUDED_FREQUENCY_FILTER
