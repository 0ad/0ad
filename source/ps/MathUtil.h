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


#ifndef MATH_UTIL_H
#define MATH_UTIL_H


//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------

#include "Prometheus.h"		// Standard Engine Include
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
	const _double PI = 3.14159265358932384;
	const _double FL_FP_TOLERANCE = .000000001;


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
	template <class T>
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
	template <class T>
	T Clamp(T &num, const _int &lowerBound,const _int &upperBound)
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
	template <class T>
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
	template <class T>
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
	template <class T>
	_int Sign(const T &num)
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
	template <class T>
	inline _double Square(const T &num)
	{
		return num*num;
	}


	//////////////////////////////////////////////////////////
	//    NAME: Swap
	// PURPOSE: Swaps two numbers
	//
	template <class T>
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
	template <class T>
	PS_RESULT Wrap(T *num,const T &lowerBound, const T &upperBound)
	{
		if(lowerBound >= upperBound)
			return ERRONEOUS_BOUND_ERROR;
		else
		{
			// translate to range 0 to n-1, find the modulus, then
			// translate back to range lowerBound to upperBound.
			num -= lowerBound;
			num = SignedModulus( num, Abs(upperBound - lowerBound) );
			num += lowerBound;
		}
		return PS_OK;
	}



	//--------------------------------------------------------
	//  Non-template functions
	//--------------------------------------------------------

	_int Ceiling(const float &num);
	_int Ceiling(const double &num);

	_bool CompareFloat(const _double &, const _double &);

	_int Floor(const float &num);
	_int Floor(const double &num);

	inline _double RadiansToDegrees(const _double &num);
	inline _double DegreesToRadians(const _double &num);

	_float Random(const _float &, const _float &);
	_int Random(const _int &,const _int &);	

	_int Round(const float &num);
	_int Round(const double &num);

	_int SignedModulus(const _int &num, const _int &n);
	_long SignedModulus(const _long &num, const _long &n);
	_float SignedModulus(const _float &num, const _float &n);
	_double SignedModulus(const _double &num, const _double &n);
}
#endif