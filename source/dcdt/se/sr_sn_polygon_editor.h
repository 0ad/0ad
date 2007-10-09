
# ifndef SR_SN_POLYGON_EDITOR_H
# define SR_SN_POLYGON_EDITOR_H

/** \file sr_sn_polygon_editor.h 
 * edits polygons
 */

# include "sr_sn_shape.h"
# include "sr_sn_editor.h"
# include "sr_lines.h"
# include "sr_polygons.h"

//==================================== SrSnPolygonEditor ====================================

/*! \class SrSnPolygonEditor sr_scene_polygons_editor.h
    \brief edit polygons
     */
class SrSnPolygonEditor : public SrSnEditor
 { public :
    enum Mode { ModeAdd, ModeEdit, ModeMove, ModeOnlyMove, ModeNoEdition };
    static const char* class_name;

   private :
    Mode _mode;
    bool _draw_vertices;
    float _precision;
    float _precision_in_pixels;
    int _selpol, _selvtx;
    int _max_polys;
    SrColor _creation_color;
    SrColor _edition_color;
    SrColor _selection_color;
    SrColor _no_edition_color;
    SrSnSharedPolygons* _polygons;
    SrSnLines* _creation;
    SrSnLines* _selection;
    SrSnLines* _picking;
    bool _solid;

   protected :
    /*! Destructor only accessible through unref() */
    virtual ~SrSnPolygonEditor ();

   public :
    /*! Constructor */
    SrSnPolygonEditor ();

    void init ();

    void polygons_changed () { _polygons->changed(true); }

    void set_polygons_to_share ( SrPolygons* p );

    void resolution ( float r ) { _polygons->resolution(r); }

    void show_polygons ( bool b ) { _polygons->visible(b); }

    /*! Determines if should draw the interiorior of the polygon or not */
    void solid ( bool b );

    /*! precision is in number of pixels (aprox.). Default is 7. */
    void set_picking_precision ( float prec ) { _precision_in_pixels=prec; }

    void set_creation_color ( SrColor c );
    void set_edition_color ( SrColor c );
    void set_selection_color ( SrColor c );
    void set_no_edition_color ( SrColor c );
    
    /*! Limits the maximum allowed number of polygons to be created */
    void set_maximum_number_of_polygons ( int i );
    
    const SrPolygons& const_shape () const { return _polygons->const_shape(); }
    SrPolygons& shape () { return _polygons->shape(); }
    void mode ( SrSnPolygonEditor::Mode m );
    int get_selected_polygon () const { return _selpol; }

   private :
    bool pick_polygon_vertex ( const SrVec2& p );
    bool pick_polygon ( const SrVec2& p );
    bool subdivide_polygon_edge ( const SrVec2& p );

    void create_vertex_selection ( const SrVec2& p );
    void create_vertex_selection ( int i, int j );

    void add_polygon_selection ( int i );
    void create_polygon_selection ( int i );
    void add_centroid_selection ( int i );
    void translate_polygon ( int i, const SrVec2& lp, const SrVec2& p );
    void rotate_polygon ( int i, const SrVec2& lp, const SrVec2& p );
    void remove_selected_polygon ();

    int handle_keyboard ( const SrEvent& e );
    int handle_only_move_event ( const SrEvent& e, const SrVec2& p, const SrVec2& lp );
    int handle_event ( const SrEvent &e );
 };

//================================ End of File =================================================

# endif // SR_SN_POLYGON_EDITOR_H


