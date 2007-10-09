
# ifndef SR_SN_H
# define SR_SN_H

/** \file sr_sn.h 
 * base scene node class
 */

# include "sr_box.h"
# include "sr_mat.h"
# include "sr_event.h"
# include "sr_array.h"
# include "sr_material.h"
# include "sr_shared_class.h"

//======================================= SrSn ====================================

/*! \class SrSn sr_sn.h
    \brief scene node base class

    Defines a base class for all scene nodes, which might be of
    four types: group, editor, matrix, or shape.
    1. It cannot be directly instantiated by the user, and so there is not 
       a public constructor available.
    2. The scene graph was designed to be simple and small. There are only
       few basic nodes defined in the sr library.
    3. A scene node does not have any information about its parent in the
       scene graph, and node sharing is supported. A reference counter is
       used for automatically deleting unused nodes, see ref() and unref()
       methods in the parent class SrSharedClass. */
class SrSn : public SrSharedClass
 { public :
    enum Type { TypeMatrix, TypeGroup, TypeEditor, TypeShape };

   private :
    int   _ref;
    char  _visible;
    char  _type;
    char* _label;
    const char* _inst_class_name;

   protected :
    /* Constructor requires the definition of the node type, and a const string
       containing the name of the instantiated node. The convention for the class_name
       is to take exactly the name of the class, without the leading "SrSn"
       word. The type parameter indicates how the node should behave when the 
       scene graph is traversed by an action. */
    SrSn ( Type t, const char* class_name );

    /*! This is a virtual destructor, so each derived class is responsible to 
        delete/unref its internal data. */
    virtual ~SrSn ();
 
   public :
    /*! Returns a const string with the name of the instantiated node class.
        The convention is exactly the name of the instantiated class,
        without the leading "SrSn" word. */
    const char* inst_class_name () const { return _inst_class_name; }

    /*! Returns the type of this node. The type is set at instantiation
        and cannot be changed later. */
    Type type () const { return (Type) _type; }

    /*! Gets the visible state of this node. */
    bool visible () const { return _visible? true:false; }

    /*! Changes the active state of this node. */
    void visible ( bool b ) { _visible = (char)b; }

    /*! Defines any label to be kept within the node. */
    void label ( const char* s );

    /*! Returns the associated label, or an empty string. */
    const char* label () const;
 };

//================================ End of File =================================================

# endif  // SR_SN_H

