
# ifndef SR_SA_BBOX_H
# define SR_SA_BBOX_H

/** \file sr_sa_bbox.h 
 * retrives the bbox
 */

# include "sr_sa.h"

/*! \class SrSaBBox sr_sa_bbox.h
    \brief bbox action

    Retrieves the bounding box of a scene */
class SrSaBBox : public SrSa
 { private :
    SrBox _box;

   public :
    void init () { _box.set_empty(); }
    void apply ( SrSn* n ) { init(); SrSa::apply(n); }
    const SrBox& get () const { return _box; }

   private : // virtual methods
    virtual bool shape_apply ( SrSnShapeBase* s );
 };

//================================ End of File =================================================

# endif  // SR_SA_BBOX_H

