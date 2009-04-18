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
