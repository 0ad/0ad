
# ifndef SR_SHARED_CLASS_H
# define SR_SHARED_CLASS_H

/*! \file sr_shared_class.h 
    reference counter for smart-pointer like behavior.
    Note: attention is required to avoid circular references */
class SrSharedClass
 { private :
    int _ref;

   protected :

    /*! Constructor initializes the reference counter as 0 */
    SrSharedClass () { _ref=0; };

    /*! Destructor in derived classes should always be declared as
        protected in order to oblige users to call always unref(),
        instead of delete */
    virtual ~SrSharedClass() {};

   public :

    /*! Returns the current reference counter value. */
    int getref () const { return _ref; }

    /*! Increments the reference counter. */
    void ref () { _ref++; }

    /*! Decrements the reference counter (if >0), and if the
        counter becomes 0, the class is automatically self deleted. */
    void unref() { if(_ref>0) _ref--; if(_ref==0) delete this; }
 };

//============================== end of file ===============================

# endif  // SR_SHARED_CLASS_H
