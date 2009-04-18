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

#ifndef INCLUDED_FREQUENCY_FILTER
#define INCLUDED_FREQUENCY_FILTER

// calculate frequency of events (tuned for 100 Hz)
struct IFrequencyFilter
{
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
