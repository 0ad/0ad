
# ifndef SR_SA_EPS_EXPORT_H
# define SR_SA_EPS_EXPORT_H

/** \file sr_sa_eps_export.h 
 * Encapsulated Postscript Export
 */

# include "sr_sa.h"

/*! Export the scene graph to a .eps file.
    Note that not all nodes are supported, mainly only 2d shapes are supported*/
class SrSaEpsExport : public SrSa
 { public :
    typedef void (*export_function)(SrSnShapeBase*,SrOutput&,const SrSaEpsExport*);

   private :
    struct RegData { const char* class_name;
                     export_function efunc;
                   };
    static SrArray<RegData> _efuncs;
    SrOutput& _output;
    float _page_width;
    float _page_height;
    float _page_margin;
    float _bbox_margin;
    float _scale_factor;
    SrVec2 _translation;
    
   public :
    /*! Constructor */
    SrSaEpsExport ( SrOutput& o );

    /*! Virtual destructor. */
    virtual ~SrSaEpsExport ();

    float scale_factor () const { return _scale_factor; }

    /*! Set print page dimensions in cms. Default is w=21.59 and h=27.94 */
    void set_page ( float w, float h ) { _page_width=w; _page_height=h; }

    /*! Set the page margin in cms to use, default is 4cms */
    void set_page_margin ( float m ) { _page_margin=m; }

    /*! The eps bounding box might need to be increased to ensure that the
        style of drawn shapes will not generate drawings outside the bouding
        box limits. The default value is 0.1 cms. */
    void set_bbox_margin ( float m ) { _bbox_margin=m; }

    /*! Registration is kept in a static array, shared by all instances
        of SrSaEpsExport. Currently only SrLine export is available.
        Registration must be explicitly called for user-defined shapes. 
        In case a name already registered is registered again, the new
        one replaces the old one. */
    friend void register_export_function ( const char* class_name, export_function efunc );

    /*! Makes the node to start applying the action. If the action is not
        applied to the entire scene, false is returned. */
    bool apply ( SrSn* n );

   private :
    virtual void mult_matrix ( const SrMat& mat );
    virtual void push_matrix ();
    virtual void pop_matrix ();
    virtual bool shape_apply ( SrSnShapeBase* s );
 };

//================================ End of File =================================================

# endif  // SR_SA_EPS_EXPORT_H

