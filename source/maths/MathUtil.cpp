#include "precompiled.h"

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
bool MathUtil::CompareFloat(const double &num1, const double &num2)
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
inline double MathUtil::RadiansToDegrees(const double &num)
{
	return num*(PI/180);
}


//////////////////////////////////////////////////////////////////////
//    NAME: RadiansToDegrees
// PURPOSE: Converts from Degrees to Radians
//
inline double MathUtil::DegreesToRadians(const double &num)
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
float MathUtil::Random(const float &lowerBound, const float &upperBound)
{

	if( lowerBound >= upperBound)
		return -1;
	else
	{
		// seed generator with current time
		srand( static_cast<unsigned>( time(NULL) ) );

		// finds a floating point number between 0 and 1.0
		float randVar = ( static_cast<float>( rand() )/RAND_MAX );

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
int MathUtil::Random(const int &lowerBound,const int &upperBound)
{
	if( lowerBound >= upperBound)
		return -1;
	else
	{
		// seed generator with current time
		srand( static_cast<unsigned>( time(NULL) ) );

		// find a random variable between 0 and range size
		int randVar = rand()%( Abs(upperBound - lowerBound) + 1);

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
int MathUtil::Round(const float &num)
{
	if( num > 0 )
		return static_cast<int>(num + .5);
	else if (num < 0 )
		return static_cast<int>(num - .5);
	else
		return 0;
}


//////////////////////////////////////////////////////////
//    NAME: Round
// PURPOSE: Rounds a number.
//   NOTES: Round rounds to the nearest representable number
//           double version.
//
int MathUtil::Round(const double &num)
{
	if( num > 0 )
		return static_cast<int>(num + .5);
	else if (num < 0 )
		return static_cast<int>(num - .5);
	else
		return 0;
}


//////////////////////////////////////////////////////////////////////
//    NAME: SignedModulus
// PURPOSE: returns a mathematically correct modulus for int
//
int MathUtil::SignedModulus(const int &num, const int &n)
{
	if( num >= 0 )
		return num%n;
	else
	{
		// the % operator reflects the range if num < 0, so
		// we have to multiply by -1 to reflect it back.  This method
		// is faster than calling Abs() and then doing the modulus.
		int Tnum = -1*(num%n);
		
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
long MathUtil::SignedModulus(const long &num, const long &n)
{
	if( num >= 0 )
		return num%n;
	else
	{
		// the % operator reflects the range if num < 0, so
		// we have to multiply by -1 to reflect it back.  This method
		// is faster than calling Abs() and then doing the modulus.
		long Tnum = -1*(num%n);

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
float MathUtil::SignedModulus(const float &num, const float &n)
{
	if( num >=0 )
		return static_cast<float>( fmod(num,n) );
	else
	{
		// the % operator reflects the range if num < 0, so
		// we have to multiply by -1 to reflect it back.  This method
		// is faster than calling Abs() and then doing the modulus.
		float Tnum = -1*( static_cast<float>( fmod(num,n) ) );

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
double MathUtil::SignedModulus(const double &num, const double &n)
{
	if( num >=0 )
		return fmod(num,n);
	else
	{
		// the % operator reflects the range if num < 0, so
		// we have to multiply by -1 to reflect it back.  This method
		// is faster than calling Abs() and then doing the modulus.
		double Tnum = -1*( fmod(num,n) );

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
