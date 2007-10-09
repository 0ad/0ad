//Experiments.h

//DJD: definitions used in running experiments {

#ifndef EXPERIMENTS_H
#define EXPERIMENTS_H

//windows-dependent timing mechanism
#include <windows.h>

//for accurate timing of paths
class Timer
{
protected:
	//time at which the timer was started
	LARGE_INTEGER StartTime;
//	LARGE_INTEGER StopTime;
	//frequency of the counter
	LONGLONG Frequency;
	//calibration component - time between lines
	LONGLONG Correction;
public:
	//constructor
	Timer()
	{
		//retrieves the frequency of the counter
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		//gets the relevant portion of that data
		Frequency = freq.QuadPart;
		LARGE_INTEGER StopTime;
		//starts the timer
		Start();
//		Stop();
		//and measures the time between these lines
		QueryPerformanceCounter(&StopTime);
		//stores the relevant part of that figure
		Correction = StopTime.QuadPart - StartTime.QuadPart;
	}

	//start timing
	void Start()
	{
//		Sleep(0);
		//record the start time
		QueryPerformanceCounter(&StartTime);
	}

//	void Stop()
//	{
//		QueryPerformanceCounter(&StopTime);
//	}

	//return the time since the start time in milliseconds
	float GetDuration()
	{
		//get the current time from the timer
		LARGE_INTEGER StopTime;
		QueryPerformanceCounter(&StopTime);
		//return the calculated duration since the timer was started
		return (float)(StopTime.QuadPart - StartTime.QuadPart - Correction) * 1000.0f / Frequency;
	}
};

//structure for holding data from the experiments
struct Data
{
	//the total time the algorithm ran
	float TotalTime;
	//the time taken to construct channels, run funnel algorithms, etc.
	float ConstructionTime;
	//the length of the best path found to this point
	float Length;
	//the number of nodes searched at this point
	int SearchNodes;
	//the number of triangles used in funnel algorithms, etc.
	int ConstructionNodes;
	//the number of paths found so far
	int Paths;
	//constructor
	Data()
	{
		//initializes all variables
		Reset();
	}
	//initializes all variables to default values
	void Reset()
	{
		TotalTime = 0.0f;
		ConstructionTime = 0.0f;
		Length = 0.0f;
		SearchNodes = 0;
		ConstructionNodes = 0;
		Paths = 1;
	}
};

#endif

//definitions used in running experiments }
