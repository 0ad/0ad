
# ifndef SR_TIME_H
# define SR_TIME_H

/** \file sr_time.h 
 * Manipulate time elements.*/

# include "sr.h"

class SrString;
# include "sr_output.h"

/*! \class SrTime sr_time.h
    \brief Manipulate a time in the format hh:mm:ss

    SrTime is intended to help adding / subtracting, etc times, keeping
    the format hh:mm:ss. */
class SrTime
 { private :
    int _h, _m;
    float _s;

   public :

    /*! Constructs with time 0:0:0 */ 
    SrTime ();

    /*! Reset time to 0:0:0 */ 
    void init () { _h=_m=0; _s=0.0f; }
    
    /*! Returns the current hour */ 
    int h() const { return _h; }

    /*! Returns the current minutes */ 
    int m() const { return _m; }

    /*! Returns the current seconds */ 
    float s() const { return _s; }

    /*! Sets a time. Parameters can have any positive values, which will
        be normalized to ensure m<60 and s<60. */
    void set ( int h, int m, float s );
 
    /*! Sets a time with float parameters. Parameters can have any positive
        values, which will be normalized to ensure m<60 and s<60. */
    void set ( float h, float m, float s );

    /*! Add two times, normalizing the results. */
    void add ( const SrTime& t );

    /*! Add the given amount of hours and normalize the results. */
    void add_hours ( float h );
 
    /*! Add the given amount of minutes and normalize the results. */
    void add_minutes ( float m );

    /*! Add the given amount of seconds and normalize the results. */
    void add_seconds ( float s );

    /*! Accumulates the given time, same as add() method. */
    SrTime& operator += ( const SrTime& t );

    /*! Set the string s to contain the time in format hh:mm:ss,
        or format hh:mm:ss.xxxx if milisecs is set to true. */
    SrString& print ( SrString& s, bool milisecs=false ) const;

    /*! Outputs the time in format hh:mm:ss. */
    friend SrOutput& operator<< ( SrOutput& o, const SrTime& t );
 };

//============================= end of file ==========================

# endif // SR_TIME_H
