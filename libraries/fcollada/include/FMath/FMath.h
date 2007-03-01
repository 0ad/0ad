/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FMath.h
	The file containing functions and constants for math.

	@defgroup FMath Mathematics Classes.
*/

#ifndef _F_MATH_H_
#define _F_MATH_H_

#include <math.h>
#include <algorithm>

// Includes the common integer and floating-point definitions and functions.
#ifndef _FM_FLOAT_H_
#include "FMath/FMFloat.h"
#endif // _FM_FLOAT_H_


/** Retrieves whether two values are equivalent.
	This template simply calls the operator== on the two values.
	@param v1 A first value.
	@param v2 A second value.
	@return Whether the two values are equivalent. */
template <class T>
bool IsEquivalent(const T& v1, const T& v2) {return v1 == v2;}

// Includes the improved vector class.
#ifndef _FM_ARRAY_H_
#include "FMath/FMArray.h"
#endif // _FM_ARRAY_H_
#ifndef _FM_ARRAY_POINTER_H_
#include "FMath/FMArrayPointer.h"
#endif // _FM_ARRAY_POINTER_H_

/** A dynamically-sized array of double-sized floating-point values. */
typedef fm::vector<double> DoubleList;

/** A dynamically-sized array of floating-point values. */
typedef fm::vector<float> FloatList;


#ifndef _FM_INTEGER_H_
#include "FMath/FMInteger.h"
#endif // _FM_INTEGER_H_



/**
	A namespace for common math functions.

	@ingroup FMath
*/
namespace FMath
{
	const double Pi = 3.14159265358979323846264338327950288419716939937510;	/**< Mathematical value of pi to 50 decimals. */

	/**
	 * Convert radians to degrees.
	 * 
	 * @param val The value in radians.
	 * @return The value in degrees.
	 */
	inline double RadToDeg(double val) { return (val * 180.0/Pi); }

	/**
	 * Convert radians to degrees.
	 * 
	 * @param val The value in radians.
	 * @return The value in degrees.
	 */
	inline float RadToDeg(float val) { return (val * 180.0f/(float)Pi); }

	/**
	 * Convert degrees to radians.
	 * 
	 * @param val The value in degrees.
	 * @return The value in radians.
	 */
	inline double DegToRad(double val) { return (val * Pi/180.0); }

	/**
	 * Convert degrees to radians.
	 * 
	 * @param val The value in degrees.
	 * @return The value in radians.
	 */
	inline float DegToRad(float val) { return (val * (float)Pi/180.0f); }

	/** Determines if given float is encoding for not a number (NAN).
		@param f The float to check.
		@return 0 if it is a number, something else if is NAN. */
#ifdef WIN32
	inline int IsNotANumber(float f) { return _isnan(f); }
#elif __PPU__
	inline int IsNotANumber(float f) { return !isfinite(f); }
#else // Linux and Mac
	inline int IsNotANumber(float f) { return !finite(f); }
#endif

	/**	Retrieves the sign of a number.
		@param val The number to check.
		@return 1 if positive, -1 if negative. */
	template <class T>
	inline T Sign(const T& val) { return (val >= T(0)) ? T(1) : T(-1); }

	/** Clamps the specified object within a range specified by two other objects
		of the same class.
		Clamping refers to setting a value within a given range. If the value is
		lower than the minimum of the range, it is set to the minimum; same for
		the maximum.
		@param val The object to clamp.
		@param mx The highest object of the range.
		@param mn The lowest object of the range.
		@return The clamped value. */
	template <class T>
	inline T Clamp(T val, T mn, T mx) { return (val > mx) ? mx : (val < mn) ? mn : val; }
};

// Include commonly used mathematical classes
#include "FMath/FMVector2.h"
#include "FMath/FMVector3.h"
#include "FMath/FMVector4.h"
#include "FMath/FMColor.h"
#include "FMath/FMMatrix33.h"
#include "FMath/FMMatrix44.h"

#endif // _F_MATH_H_
