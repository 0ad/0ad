
# ifndef SR_SA_MODEL_EXPORT_H
# define SR_SA_MODEL_EXPORT_H

/** \file sr_sa_model_exportT.h 
 * Export all models in global coordinates
 */

# include "sr_sa.h"

/*! Export all models in the scene graph in global coordinates
    to several .srm files. */
class SrSaModelExport : public SrSa
 { private :
    SrString _dir;
    SrString _prefix;
    int _num;
   public :
    /*! Constructor */
    SrSaModelExport ( const SrString& dir );

    /*! Virtual destructor. */
    virtual ~SrSaModelExport ();

    /*! Change the directory to save files */
    void directory ( const SrString& dir );

    /*! Change the prefix name of the files */
    void prefix ( const SrString& pref ) { _prefix=pref; }

    /*! Start applying the action. If the action is not
        applied to the entire scene, false is returned. */
    bool apply ( SrSn* n );

   private :
    virtual bool shape_apply ( SrSnShapeBase* s );
 };

//================================ End of File =================================================

# endif  // SR_SA_MODEL_EXPORT_H

