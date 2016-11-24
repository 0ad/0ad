/* Copyright (C) 2012 Wildfire Games.
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

#ifndef INCLUDED_EASE
#define INCLUDED_EASE

/*
 * Straightforward C++ port of Robert Penner's easing equations
 * http://www.robertpenner.com/easing/
 *
 * Copyright (c) 2001 Robert Penner
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *     - Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 *     - Redistributions in binary form must reproduce the above copyright notice, this list of
 *       conditions and the following disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     - Neither the name of the author nor the names of contributors may be used to endorse or
 *       promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <math.h>

/**
 * Generic easing functions. In each function, the parameters are:
 *
 * @param t Current time in seconds, as a float between 0 and d (inclusive).
 * @param d Total duration of the ease, in seconds. Must be strictly positive.
 * @param b Baseline value (at t = 0).
 * @param c Delta from baseline value to reach the target value (at t = d). I.e., target = b + c.
 *
 * Each function outputs the eased value between 'b' and 'b+c' at time 't'.
 */
class Ease
{
public:
	static float QuadIn(float t, const float b, const float c, const float d)
	{
		t /= d;
		return c*t*t + b;
	}

	static float QuadOut(float t, const float b, const float c, const float d)
	{
		t /= d;
		return -c * t*(t-2) + b;
	}

	static float QuadInOut(float t, const float b, const float c, const float d)
	{
		t /= d/2;
		if (t < 1)
			return c/2*t*t + b;
		--t;
		return -c/2 * (t*(t-2) - 1) + b;
	}

	static float CubicIn(float t, const float b, const float c, const float d)
	{
		t /= d;
		return c*t*t*t + b;
	}

	static float CubicOut(float t, const float b, const float c, const float d)
	{
		t = t/d - 1;
		return c*(t*t*t + 1) + b;
	}

	static float CubicInOut(float t, const float b, const float c, const float d)
	{
		t /= d/2;
		if (t < 1)
			return c/2*t*t*t + b;
		t -= 2;
		return c/2*(t*t*t + 2) + b;
	}

	static float QuartIn(float t, const float b, const float c, const float d)
	{
		t /= d;
		return c*t*t*t*t + b;
	}

	static float QuartOut(float t, const float b, const float c, const float d)
	{
		t = t/d - 1;
		return -c*(t*t*t*t - 1) + b;
	}

	static float QuartInOut(float t, const float b, const float c, const float d)
	{
		t /= d/2;
		if (t < 1)
			return c/2*t*t*t*t + b;
		t -= 2;
		return -c/2 * (t*t*t*t - 2) + b;
	}

	static float QuintIn(float t, const float b, const float c, const float d)
	{
		t /= d;
		return c*t*t*t*t*t + b;
	}

	static float QuintOut(float t, const float b, const float c, const float d)
	{
		t = t/d - 1;
		return c*(t*t*t*t*t + 1) + b;
	}

	static float QuintInOut(float t, const float b, const float c, const float d)
	{
		t /= d/2;
		if (t < 1)
			return c/2*t*t*t*t*t + b;
		t -= 2;
		return c/2*(t*t*t*t*t + 2) + b;
	}
};

#endif // INCLUDED_EASE
