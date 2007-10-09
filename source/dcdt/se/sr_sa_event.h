
# ifndef SR_SA_EVENT_H
# define SR_SA_EVENT_H

/** \file sr_sa.h 
 * propagates events
 */

# include "sr_sa.h"
# include "sr_event.h"

/*! \class SrSaEvent sr_sa_event.h
    \brief propagates events in a scene

    sends an event to the scene graph */
class SrSaEvent : public SrSa
 { private :
    SrEvent _ev;
    int _result;

   public :
    SrSaEvent ( const SrEvent& e ) { _ev=e; _result=0; }
    SrEvent& get () { return _ev; }
    int result () const { return _result; }

   private : // virtual methods
    virtual bool editor_apply ( SrSnEditor* m );
 };

//================================ End of File =================================================

# endif  // SR_SA_EVENT_H

