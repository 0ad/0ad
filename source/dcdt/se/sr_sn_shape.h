
# ifndef SR_SN_SHAPE_H
# define SR_SN_SHAPE_H

/** \file sr_sn_shape.h 
 * shape base and template class
 */

# include "sr_sn.h"

//================================ SrSnShapeBase ====================================

enum srRenderMode { srRenderModeDefault,
                    srRenderModeSmooth,
                    srRenderModeFlat,
                    srRenderModeLines,
                    srRenderModePoints };

/*! \class SrSnShapeBase sr_scene.h
    \brief general scene shape element

    Base class for scene shape nodes. This class may be derived for
    creating shape nodes. A template class SrSnShape is provided
    to easily create shape nodes, and is used for all SR buit-in shape
    node types by means of typedefs, as for instance, SrSceneModel is
    a typedef of SrSnShape<SrModel> (note that, in the example,
    sr_model.h must be included before sr_scene.h) */
class SrSnShapeBase : public SrSn
 { public :
    class RenderLibData { public : virtual ~RenderLibData() {}; };
   private :
    RenderLibData* _renderlib_data;
    char  _changed;
    char  _render_mode;
    char  _overriden_render_mode;
    char  _can_override_render_mode;
    float _resolution; // default is 1
    SrMaterial _material;
    SrMaterial _overriden_material;
    char _material_is_overriden;

   protected :

    /*! Constructor requires the name of the derived class. */
    SrSnShapeBase ( const char* name );

    /* Destructor only accessible through unref().*/
    virtual ~SrSnShapeBase ();

   public :

    /*! Associates a user-derived class from RenderLibData to this node.
        This function can be only called one time. If it is being called by the
        second time, false is returned and nothing is done.
        The pointer rdata should be allocated using operator new and SrSnShapeBase
        will delete it in its destructor */
    bool set_renderlib_data ( RenderLibData* rdata );

    /*! Retrieves the data related to the used rendering library */
    RenderLibData* get_renderlib_data () { return _renderlib_data; }

    /*! By default the render mode is srRenderModeDefault, that is specific to the
        shape type. For example a SrModel has as default smooth render mode, while
        SrLines has line mode as default. */
    srRenderMode render_mode () const { return (srRenderMode)_render_mode; }
    void render_mode ( srRenderMode m );

    /*! Saves the current render mode and then replaces it by m. If
        can_override_render_mode() is false, then nothing is done. */
    void override_render_mode ( srRenderMode m );

    /*! Restore the render mode that was used before overriding it. */
    void restore_render_mode ();

    /*! If false, the render action will not be able to override the render mode
        of this shape node. Default is true. */
    void can_override_render_mode ( bool b ) { _can_override_render_mode = (char)b; }
    bool can_override_render_mode () { return _can_override_render_mode? true:false; }

    /*! Saves the current material and then replaces it by m. */
    void override_material ( SrMaterial m );

    /*! Restore the material that was used before overriding it. */
    void restore_material ();

    /*! Returns true if the material is being overrriden, and false otherwise. */
    bool material_is_overriden () { return _material_is_overriden? true:false; }

    /*! The default resolution is 1, a greater value will render more accurate
        parametric objects, 0 represents the coarser possible representation.
        Values cannot be <0. */
    float resolution () const { return _resolution; }
    void resolution ( float r );

    /*! Sets the material to be used for the shape */
    void material ( const SrMaterial& m );
    const SrMaterial& material () const { return _material; }

    /*! Sets the diffuse color of the shape material. Note that when rendering
        shapes without lighting, only the diffuse color is used. */
    void color ( const SrColor& c );
    const SrColor& color () const { return _material.diffuse; }

    /*! Returns true if the node parameters have changed, requiring
        the re-generation of the associated render lists. */
    bool haschanged () const { return _changed? true:false; }

    /*! Sets the changed / unchanged state. If true, it will force the
        node to regenerate its render lists. */
    void changed ( bool b ) { _changed = (char)b; }

    /*! Calculates the bounding box of the shape. */
    virtual void get_bounding_box ( SrBox &box ) const=0;
 };

//================================= SrSnShapeTpl ====================================

/*! \class SrSnShape sr_scene.h
    \brief general scene shape template

    SrSnShape is used for all SR buit-in shape
    node types by means of typedefs. For example, SrSnModel is
    a typedef of SrSnShape<SrModel> (note that, in the example,
    sr_model.h must be included before instantiating SrSnModel) */
template <class X>
class SrSnShape : public SrSnShapeBase
 { protected :
    X* _data;

   protected :
    /*! Constructor from a given pointer to a shape. If the pointer is null,
        a new shape will be allocated and used. */
    SrSnShape ( X* pt ) : SrSnShapeBase(X::class_name)
     { _data = pt? pt: new X; }

   public :
    SrSnShape() : SrSnShapeBase(X::class_name) { _data = new X; }

    /* Virtual Destructor .*/
    virtual ~SrSnShape () { delete _data; }

    /*! Get a const reference to the shape data, without setting the state
        of the node as changed. */
    const X& const_shape () const { return *_data; }

    /*! Get a reference to the shape data, and sets the state of the node
        as changed, implying that display lists should be regenerated. */
    X& shape () { changed(true); return *_data; }

    /*! Set data using the copy constructor of X. */
    void shape ( const X& data ) { changed(true); *_data=data; }

    /*! Deletes the internal data and starts using the new one pt.
        Pointer pt must not be null. Use with care. */
    void replace_shape_pointer ( X* pt ) { changed(true); delete _data; _data=pt; }

    /*! Calculates the bounding box of the shape. */
    virtual void get_bounding_box ( SrBox &box ) const { _data->get_bounding_box(box); }
 };

/*! \class SrSnSharedShape sr_scene.h
    \brief template for shared scene shapes

    SrSnSharedShape is used only for shapes deriving SrSharedClass,
    so that a pointer to the shape and ref()/unref() methods are used.
    Typedefs are used similarly to SrSnShape */
template <class X>
class SrSnSharedShape : public SrSnShape<X>
 { public :
    /*! Constructor receives a pointer to a shape. If the pointer is null,
        a new shape will be allocated and used. */
    SrSnSharedShape ( X* pt=0 ) : SrSnShape<X> (pt) { _data->ref(); }

    /* Virtual Destructor .*/
    virtual ~SrSnSharedShape ()
      { _data->unref();
        _data=0; // to cope with base class destructor
      }

    /*! Reference a new shape, and unreference the old one. */
    void shape ( X* pt ) { changed(true); _data->unref(); _data=pt; _data->ref(); }

    /*! Get a reference to the shape data, and sets the state of the node
        as changed, implying that display lists should be regenerated. */
    X& shape () { changed(true); return *_data; }
 };

class SrModel;
class SrLines;
class SrPoints;
class SrSphere;
class SrCylinder;
class SrPolygon;
class SrPolygons;

typedef SrSnShape<SrBox>      SrSnBox;
typedef SrSnShape<SrSphere>   SrSnSphere;
typedef SrSnShape<SrCylinder> SrSnCylinder;
typedef SrSnShape<SrLines>    SrSnLines;
typedef SrSnShape<SrPoints>   SrSnPoints;
typedef SrSnShape<SrModel>    SrSnModel;
typedef SrSnShape<SrPolygon>  SrSnPolygon;
typedef SrSnShape<SrPolygons> SrSnPolygons;

typedef SrSnSharedShape<SrModel>    SrSnSharedModel;
typedef SrSnSharedShape<SrPolygons> SrSnSharedPolygons;

//================================ End of File =================================================

# endif  // SR_SN_SHAPE_H

