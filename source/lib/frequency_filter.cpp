/* Copyright (C) 2013 Wildfire Games.
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

#include "precompiled.h"
#include "lib/frequency_filter.h"

static const double errorTolerance = 0.25;
static const double sensitivity = 0.10;
static const double sampleTime = 2.0; // seconds

/**
 * variable-width window for frequency determination
 **/
class FrequencyEstimator
{
	NONCOPYABLE(FrequencyEstimator);
public:
	FrequencyEstimator(double resolution)
		: m_minDeltaTime(4.0 * resolution)	// chosen to reduce error but still yield rapid updates.
		, m_lastTime(0)	// will be set on first call
		, m_numEvents(0)
	{
		ENSURE(resolution > 0.0);
	}

	bool operator()(double time, double& frequency)
	{
		m_numEvents++;

		if(m_lastTime == 0.0)
			m_lastTime = time;

		// count # events until deltaTime is large enough
		// (reduces quantization errors if resolution is low)
		const double deltaTime = time - m_lastTime;
		if(deltaTime <= m_minDeltaTime)
			return false;

		frequency = m_numEvents / deltaTime;
		m_numEvents = 0;
		m_lastTime = time;
		return true;	// success
	}

private:
	const double m_minDeltaTime;
	double m_lastTime;
	int m_numEvents;
};


/**
 * variable-gain IIR filter
 **/
class IirFilter
{
public:
	IirFilter(double sensitivity, double initialValue)
		: m_sensitivity(sensitivity), m_prev(initialValue)
	{
	}

	// bias = 0: no change. > 0: increase (n-th root). < 0: decrease (^n)
	double operator()(double x, int bias)
	{
		// sensitivity to changes ([0,1]).
		const double gain = pow(m_sensitivity, ComputeExponent(bias));
		return m_prev = x*gain + m_prev*(1.0-gain);
	}

private:
	static double ComputeExponent(int bias)
	{
		if(bias > 0)
			return 1.0 / bias;	// n-th root
		else if(bias == 0)
			return 1.0;		// no change
		else
			return -bias;	// power-of-n
	}

	double m_sensitivity;
	double m_prev;
};


/**
 * regulate IIR gain for rapid but smooth tracking of a function.
 * this is similar in principle to a PID controller but is tuned for
 * the special case of FPS values to simplify stabilizing the filter.
 **/
class Controller
{
public:
	Controller(double initialValue)
		: m_timesOnSameSide(0)
	{
		std::fill(m_history, m_history+m_historySize, initialValue);
	}

	// bias := exponential change to gain, (-inf, inf)
	int ComputeBias(double smoothedValue, double value)
	{
		if(WasOnSameSide(value))	// (must be checked before updating history)
			m_timesOnSameSide++;
		else
			m_timesOnSameSide = 0;

		// update history
		std::copy(m_history, m_history+m_historySize, m_history+1);
		m_history[m_historySize-1] = value;

		// dampen jitter
		if(Change(smoothedValue, value) < 0.04)
			return -1;

		// dampen spikes/bounces.
		if(WasSpike())
			return -2;

		// if the past few samples have been consistently above/below
		// average, the function is changing and we need to catch up.
		// (similar to I in a PID)
		if(m_timesOnSameSide >= 3)
			return std::min(m_timesOnSameSide, 4);

		// suppress large jumps.
		if(Change(m_history[m_historySize-1], value) > 0.30)
			return -4;	// gain -> 0

		return 0;
	}

private:
	bool WasOnSameSide(double value) const
	{
		int sum = 0;
		for(size_t i = 0; i < m_historySize; i++)
		{
			const int vote = (value >= m_history[i])? 1 : -1;
			sum += vote;
		}
		return abs(sum) == (int)m_historySize;
	}

	static double Change(double from, double to)
	{
		return fabs(from - to) / from;
	}

	// /\ or \/ in last three history entries
	bool WasSpike() const
	{
		cassert(m_historySize >= 3);
		const double h2 = m_history[m_historySize-3], h1 = m_history[m_historySize-2], h0 = m_history[m_historySize-1];
		if(((h2-h1) * (h1-h0)) > 0)	// no sign change
			return false;
		if(Change(h2, h0) > 0.05)	// overall change from oldest to newest value
			return false;
		if(Change(h1, h0) < 0.10)	// no intervening spike
			return false;
		return true;
	}

	static const size_t m_historySize = 3;
	double m_history[m_historySize];
	int m_timesOnSameSide;
};


class FrequencyFilter : public IFrequencyFilter
{
	NONCOPYABLE(FrequencyFilter);
public:
	FrequencyFilter(double resolution, double expectedFrequency)
		: m_frequencyEstimator(resolution), m_controller(expectedFrequency), m_iirFilter(sensitivity, expectedFrequency)
		, m_stableFrequency((int)expectedFrequency), m_smoothedFrequency(expectedFrequency), m_averagedFrequency(expectedFrequency)
		, m_numberOfSamples((int)(sampleTime * expectedFrequency) + 1)
	{
	}

	virtual void Update(double time)
	{
		double frequency;
		if(!m_frequencyEstimator(time, frequency))
			return;

		const int bias = m_controller.ComputeBias(m_smoothedFrequency, frequency);
		m_smoothedFrequency = m_iirFilter(frequency, bias);

		// Keep a moving average of the frequency over the last two seconds
		// If there is a spike of more than 25% (e.g. loading screen => game)
		// then reset the moving average and reset the number of samples needed
		const double difference = fabs(m_smoothedFrequency - m_averagedFrequency);
		if (difference > errorTolerance * m_averagedFrequency)
		{
			m_averagedFrequency = m_smoothedFrequency;
			m_numberOfSamples = (int)(m_averagedFrequency * sampleTime) + 1;
		}
		else
			m_averagedFrequency = ((double)(m_numberOfSamples - 1) * m_averagedFrequency + m_smoothedFrequency) / (double)m_numberOfSamples;

		// allow the smoothed FPS to free-run until it is no longer near the
		// previous stable FPS value. round up because values are more often
		// too low than too high.
		m_stableFrequency = (int)(m_averagedFrequency + 0.99);
	}

	virtual double SmoothedFrequency() const
	{
		return m_smoothedFrequency;
	}

	virtual int StableFrequency() const
	{
		return m_stableFrequency;
	}

private:
	FrequencyEstimator m_frequencyEstimator;
	Controller m_controller;
	IirFilter m_iirFilter;

	int m_stableFrequency;
	double m_smoothedFrequency;
	double m_averagedFrequency;
	int m_numberOfSamples;
};


PIFrequencyFilter CreateFrequencyFilter(double resolution, double expectedFrequency)
{
	return PIFrequencyFilter(new FrequencyFilter(resolution, expectedFrequency));
}
