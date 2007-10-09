
# ifndef SR_RANDOM_H
# define SR_RANDOM_H

/** \file sr_random.h 
 * Random number generation */

//================================= SrRandom ====================================

/*! Random methods use a custom random generator with resolution of 2^32-1 */
class SrRandom
 { private:
    double _inf, _sup, _dif;
    char _type; // 'd', 'f', or 'i'

   public :

    /*! Default constructor sets the random generator to have
       limits [0,1] and type float */
    SrRandom ();

    /*! Constructor with given limits in type double, with 53-bit resolution */
    SrRandom ( double inf, double sup ) { limits(inf,sup); }
 
    /*! Constructor with given limits in type float, with 32-bit resolution */
    SrRandom ( float inf, float sup ) { limits(inf,sup); }

    /*! Constructor with given integer limits. */
    SrRandom ( int inf, int sup ) { limits(inf,sup); }

    /*! Sets limits in type double, with 53-bit resolution */
    void limits ( double inf, double sup );

    /*! Sets limits in type float, with 32-bit resolution. */
    void limits ( float inf, float sup, int br=15 );

    /*! Sets integer limits.. */
    void limits ( int inf, int sup );

    /*! Returns the type on which the limits were set: 'd', 'f', or 'i' */
    char type () { return _type; }

    /*! Returns the superior limit. */
    double inf () { return _inf; }

    /*! Returns the inferior limit. */
    double sup () { return _sup; }

    /*! Returns a random integer number in [inf,sup]. */
    int geti ();
    
    /*! Returns a random float number in [inf,sup], with 32-bit resolution. */
    float getf ();

    /*! Returns a random double number in [inf,sup], with 53-bit resolution. */
    double getd ();

    /*! Set limits and returns one corresponding random value. */
    int get ( int inf, int sup ) { limits(inf,sup); return geti(); }

    /*! Set limits and returns one corresponding random value, with 32-bit resolution. */
    float get ( float inf, float sup ) { limits(inf,sup); return getf(); }

    /*! Set limits and returns one corresponding random value, with 53-bit resolution. */
    double get ( double inf, double sup ) { limits(inf,sup); return getd(); }

    /*! Sets the starting point for generating random numbers. The seed is 1 by default. */
    static void seed ( unsigned long seed );
    
    /*! Returns a random float in [0,1], with 32-bit resolution */
    static float randf ();
 };

//============================== end of file ======================================

# endif  // SR_RANDOM_H

