
# ifndef SR_EVENT_H
# define SR_EVENT_H

/** \file sr_event.h 
 * A user-generated window-event
 */

# include "sr_vec2.h"
# include "sr_line.h"
# include "sr_output.h"

/*! \class SrEvent sr_event.h
    \brief Keeps a window event

    SrEvent is used to describe a mouse or keyboard event, in a
    system-independent way. */
class SrEvent 
 { public :
    /*! Enumerators for the type of event. */
    enum Type { None,    //!< No event occured.
                Push,    //!< A mouse button was pushed.
                Drag,    //!< The mouse moved with a button down.
                Release, //!< A mouse button was released.
                Keyboard //!< A key was pressed.
              };

    /*! Enumerators with codes for special keys. */
    enum KeyCodes { KeyEsc=65307, KeyDel=65535, KeyIns=65379, KeyBack=65288,
                    KeyUp=65362, KeyDown=65364, KeyLeft=65361, KeyRigth=65363,
                    KeyEnd=65367, KeyPgDn=65366, KeyPgUp=65365, KeyHome=65360,
                    KeyShift=653505, KeyCtrl=65307, KeyAlt=65313, KeySpace=32,
                    KeyEnter=65293 };

   public : //--- event occured :
    Type type;    //!< The type of the occured event
    char button;  //!< The button number 1, 2 or 3 if event type was Push or Release, 0 otherwise
    int  key;     //!< The ascii code / code of the key pressed (uppercase) if it was a keyboard event, 0 otherwise
    int  width;   //!< The width of the screen when the event occured
    int  heigth;  //!< The heigth of the screen when the event occured
//    float scenew; //!< Width in the units of the scene
//    float sceneh; //!< Heigth in the units of the scene

   public : //--- states at event time :
    char button1; //!< Contains 1 if the left mouse button state is pressed, 0 otherwise
    char button2; //!< Contains 1 if the middle mouse button state is pressed, 0 otherwise
    char button3; //!< Contains 1 if the right mouse button state is pressed, 0 otherwise
    char alt;     //!< Contains 1 if the Alt modifier key state is pressed, 0 otherwise
    char ctrl;    //!< Contains 1 if the Ctrl modifier key state is pressed, 0 otherwise
    char shift;   //!< Contains 1 if the Shift modifier key state is pressed, 0 otherwise

   public : //--- mouse coordinates :
    SrVec2 mouse;  //!< Current mouse push position in normalized coords [-1,1]
    SrVec2 lmouse; //!< Last mouse x push position in normalized coords [-1,1]

   public : //--- scene information :
    SrLine ray;  
    SrLine lray;
    SrPnt mousep;     //!< mouse point in scene coordinates at plane z=0
    SrPnt lmousep;    //!< last mouse point in scene coordinates at plane z=0
    float pixel_size; //!< 0.05 by default but should be updated according to the camera

   public : //--- methods :

    /*! Initialize as a None event type, by calling init(). */
    SrEvent ();

    /*! Makes the event as the None type, and puts all data with their default values of zero. */
    void init ();

    /*! Puts mouse keyboard information to their default value, but saves the mouse
        values in the lmouse variables. */
    void init_lmouse ();

    /*! Returns a string with the name of the event type. */
    const char *type_name () const;

    /*! Returns the difference: mousex-lmousex. */
    float mousedx () const { return mouse.x-lmouse.x; }

    /*! Returns the difference: mousey-lmousey. */
    float mousedy () const { return mouse.y-lmouse.y; }

    /*! Returns true if the event type is push, drag, or release; and false otherwise. */
    bool mouse_event () const { return type==Push||type==Drag||type==Release? true:false; }

    /*! Outputs data of this event for data inspection. */
    friend SrOutput& operator<< ( SrOutput& out, const SrEvent& e );
 };


//================================ End of File =================================================

# endif // SR_EVENT_H

