#ifndef MATH_UTIL_H
#define MATH_UTIL_H

#ifndef PI
#define PI							3.14159265358979323846f
#endif

#define DEGTORAD(a)					((a) * (PI/180.0f))
#define RADTODEG(a)					((a) * (180.0f/PI))
#define SQR(x)						((x) * (x))
#define MAX3(a,b,c)					( MAX (MAX(a,b), c) )
#define ABS(a)						((a > 0) ? (a) : (-a))

template <typename T>
T Interpolate( T& a, T& b, float l )
{
	return( a + ( b - a ) * l );
}

template <typename T>
inline T clamp(T value, T min, T max)
{
	if (value<=min) return min;
	else if (value>=max) return max;
	else return value;
}

#if 0

/*
Math utility functions
by Michael Reiland
recondite_phreak@yahool.com

--Overview--

	Contains common math functions like Abs, Sign, Max, Min, etc.


--More info--

	TODO: actually write corresponding documentation
    http://wildfiregames.com/0ad/codepit/TDD/math_utils.html

*/



//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------

#include "Pyrogenesis.h"		// Standard Engine Include
#include <math.h>			// Needed for fmod()

//--------------------------------------------------------
//  Error declarations
//--------------------------------------------------------


// MathUtil Errors
DECLARE_ERROR(ERRONEOUS_BOUND_ERROR);


//--------------------------------------------------------
//  Declarations
//--------------------------------------------------------

namespace MathUtil
{
	const double PI = 3.14159265358932384;
	const double FL_FP_TOLERANCE = .000000001;


	//--------------------------------------------------------
	//  Template functions
	//--------------------------------------------------------


	//--------------------------------------------------------
	//  Declarations
	//--------------------------------------------------------

	//////////////////////////////////////////////////////////
	//    NAME: Abs
	// PURPOSE: Calculates the Absolute value
	//
	template <typename T>
	T Abs(const T &num)
	{
		if( num < 0)
			return -1*num;
		return num;
	}


	//////////////////////////////////////////////////////////
	//    NAME: Clamp
	// PURPOSE: Forces num to be between lowerBound and upperBound
	//
	template <typename T>
	T Clamp(T &num, const int &lowerBound,const int &upperBound)
	{
		if(num <= lowerBound)
			num = static_cast<T>(lowerBound);
		else if( num >= upperBound)
			num = static_cast<T>(upperBound);
	}


	//////////////////////////////////////////////////////////
	//    NAME: Max
	// PURPOSE: Returns the largest number.
	//
	template <typename T>
	T Max(const T &num1, const T &num2)
	{
		if( num1 > num2)
			return num1;
		else
			return num2;
	}


	//////////////////////////////////////////////////////////
	//    NAME: Min
	// PURPOSE: Returns the smallest number.
	//
	template <typename T>
	T Min(const T &num1, const T &num2)
	{
		if( num1 < num2)
			return num1;
		else
			return num2;
	}


	//////////////////////////////////////////////////////////
	//    NAME: Sign
	// PURPOSE: Returns 1 if the number is > 0, -1 if it's < 0,
	//            otherwise returns 0.
	//
	template <typename T>
	int Sign(const T &num)
	{
		if( num > 0 )
			return 1;
		else if( num < 0 )
			return -1;
		else
			return 0;
	}


	//////////////////////////////////////////////////////////
	//    NAME: Square
	// PURPOSE: Returns the square of a number
	//   NOTES: Num should be less than the square root of the
	//            maximum representable number for the data type.
	//
	template <typename T>
	inline double Square(const T &num)
	{
		return num*num;
	}


	//////////////////////////////////////////////////////////
	//    NAME: Swap
	// PURPOSE: Swaps two numbers
	//
	template <typename T>
	void Swap(T *num1, T *num2)
	{
		T temp = num1;
		num1 = num2;
		num2 = temp;
	}


	//////////////////////////////////////////////////////////
	//    NAME: Wrap
	// PURPOSE: Wraps num between lowerBound and upperBound.
	//
	template <typename T>
	int Wrap(T *num,const T &lowerBound, const T &upperBound)
	{
		if(lowerBound >= upperBound)
			return -1;
		else
		{
			// translate to range 0 to n-1, find the modulus, then
			// translate back to range lowerBound to upperBound.
			num -= lowerBound;
			num = SignedModulus( num, Abs(upperBound - lowerBound) );
			num += lowerBound;
		}
		return 0;
	}

	//--------------------------------------------------------
	//  Non-template functions
	//--------------------------------------------------------

	int Ceiling(const float &num);
	int Ceiling(const double &num);

	bool CompareFloat(const double &, const double &);

	int Floor(const float &num);
	int Floor(const double &num);

	inline double RadiansToDegrees(const double &num);
	inline double DegreesToRadians(const double &num);

	float Random(const float &, const float &);
	int Random(const int &,const int &);	

	int Round(const float &num);
	int Round(const double &num);

	int SignedModulus(const int &num, const int &n);
	long SignedModulus(const long &num, const long &n);
	float SignedModulus(const float &num, const float &n);
	double SignedModulus(const double &num, const double &n);
}

#endif


#endif
