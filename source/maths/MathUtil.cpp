#if 0
// last modified Thursday, May 08, 2003

#include <time.h>
#include <cstdlib>

#include "MathUtil.h"


// MathUtil Errors
DEFINE_ERROR(ERRONEOUS_BOUND_ERROR, "Lower Bound is >= Upper Bound");



//////////////////////////////////////////////////////////////////////
//    NAME: CompareFloat
// PURPOSE: Returns true if two floating point numbers are within
//            FL_FP_TOLERANCE of each other.
//
_bool MathUtil::CompareFloat(const _double &num1, const _double &num2)
{
	if( Abs(num1 - num2) < FL_FP_TOLERANCE )
		return true;
	else
		return false;
}


//////////////////////////////////////////////////////////////////////
//    NAME: RadiansToDegrees
// PURPOSE: Converts from Radians to Degrees
//
inline _double MathUtil::RadiansToDegrees(const _double &num)
{
	return num*(PI/180);
}


//////////////////////////////////////////////////////////////////////
//    NAME: RadiansToDegrees
// PURPOSE: Converts from Degrees to Radians
//
inline _double MathUtil::DegreesToRadians(const _double &num)
{
	return (num*180)/PI;
}

/*
//////////////////////////////////////////////////////////////////////
//    NAME: Random
// PURPOSE: returns a random floating point number between lowerBound
//            and upperBound
//   NOTES: returns -1 if lowerBound >= upperBound
//
_float MathUtil::Random(const _float &lowerBound, const _float &upperBound)
{

	if( lowerBound >= upperBound)
		return -1;
	else
	{
		// seed generator with current time
		srand( static_cast<unsigned>( time(NULL) ) );

		// finds a floating point number between 0 and 1.0
		_float randVar = ( static_cast<_float>( rand() )/RAND_MAX );

		// maps the number onto the set from 0 to upperBound
		randVar *= Abs(lowerBound - upperBound ) + 1;

		//translate to the proper range
		randVar += lowerBound;

		return randVar;
	}
}


//////////////////////////////////////////////////////////////////////
//    NAME: Random
// PURPOSE: returns a random number between lowerBound and upperBound
//   NOTES: returns -1 if lowerBound >= upperBound
//
_int MathUtil::Random(const _int &lowerBound,const _int &upperBound)
{
	if( lowerBound >= upperBound)
		return -1;
	else
	{
		// seed generator with current time
		srand( static_cast<unsigned>( time(NULL) ) );

		// find a random variable between 0 and range size
		_int randVar = rand()%( Abs(upperBound - lowerBound) + 1);

		// translate to proper range
		randVar += lowerBound;

		return randVar;
	}
}
*/

//////////////////////////////////////////////////////////
//    NAME: Round
// PURPOSE: Rounds a number.
//   NOTES: Round rounds to the nearest representable number
//           float version.
//
_int MathUtil::Round(const float &num)
{
	if( num > 0 )
		return static_cast<_int>(num + .5);
	else if (num < 0 )
		return static_cast<_int>(num - .5);
	else
		return 0;
}


//////////////////////////////////////////////////////////
//    NAME: Round
// PURPOSE: Rounds a number.
//   NOTES: Round rounds to the nearest representable number
//           double version.
//
_int MathUtil::Round(const double &num)
{
	if( num > 0 )
		return static_cast<_int>(num + .5);
	else if (num < 0 )
		return static_cast<_int>(num - .5);
	else
		return 0;
}


//////////////////////////////////////////////////////////////////////
//    NAME: SignedModulus
// PURPOSE: returns a mathematically correct modulus for int
//
_int MathUtil::SignedModulus(const _int &num, const _int &n)
{
	if( num >= 0 )
		return num%n;
	else
	{
		// the % operator reflects the range if num < 0, so
		// we have to multiply by -1 to reflect it back.  This method
		// is faster than calling Abs() and then doing the modulus.
		_int Tnum = -1*(num%n);
		
		// if num%n equals 0, then n - Tnum will be n, which, logically
		// speaking, is 0 in a different form, but we have to make sure it's
		// in the form 0, and not n.  Therefore, if Tnum = 0, simply leave
		// it like that.
		if( Tnum != 0)			
			Tnum = n - Tnum;

		return Tnum;
	}
}


//////////////////////////////////////////////////////////////////////
//    NAME: SignedModulus
// PURPOSE: returns a mathematically correct modulus for long
//
_long MathUtil::SignedModulus(const _long &num, const _long &n)
{
	if( num >= 0 )
		return num%n;
	else
	{
		// the % operator reflects the range if num < 0, so
		// we have to multiply by -1 to reflect it back.  This method
		// is faster than calling Abs() and then doing the modulus.
		_long Tnum = -1*(num%n);

		// if num%n equals 0, then n - Tnum will be n, which, logically
		// speaking, is 0 in a different form, but we have to make sure it's
		// in the form 0, and not n.  Therefore, if Tnum = 0, simply leave
		// it like that.
		if( Tnum != 0)			
			Tnum = n - Tnum;

		return Tnum;
	}
}


//////////////////////////////////////////////////////////////////////
//    NAME: SignedModulus
// PURPOSE: returns a mathematically correct modulus for float
//   NOTES: uses fmod() in math.h, which returns the modulus of floats
//
_float MathUtil::SignedModulus(const _float &num, const _float &n)
{
	if( num >=0 )
		return static_cast<_float>( fmod(num,n) );
	else
	{
		// the % operator reflects the range if num < 0, so
		// we have to multiply by -1 to reflect it back.  This method
		// is faster than calling Abs() and then doing the modulus.
		_float Tnum = -1*( static_cast<_float>( fmod(num,n) ) );

		// if num%n equals 0, then n - Tnum will be n, which, logically
		// speaking, is 0 in a different form, but we have to make sure it's
		// in the form 0, and not n.  Therefore, if Tnum = 0, simply leave
		// it like that.
		if( Tnum != 0)
			Tnum = n - Tnum;

		return Tnum;
	}

}


//////////////////////////////////////////////////////////////////////
//    NAME: SignedModulus
// PURPOSE: returns a mathematically correct modulus for double
//   NOTES: uses fmod() in math.h, which returns the modulus of floats
//
_double MathUtil::SignedModulus(const _double &num, const _double &n)
{
	if( num >=0 )
		return fmod(num,n);
	else
	{
		// the % operator reflects the range if num < 0, so
		// we have to multiply by -1 to reflect it back.  This method
		// is faster than calling Abs() and then doing the modulus.
		_double Tnum = -1*( fmod(num,n) );

		// if num%n equals 0, then n - Tnum will be n, which, logically
		// speaking, is 0 in a different form, but we have to make sure it's
		// in the form 0, and not n.  Therefore, if Tnum = 0, simply leave
		// it like that.
		if( Tnum != 0)
			Tnum = n - Tnum;

		return Tnum;
	}

}
#endif