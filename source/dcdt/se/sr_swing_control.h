
# include <SR/sr_vec2.h>
# include <SR/sr_array.h>

# include <FL/Fl_Widget.H>

//============================ SrSwingControl ============================

/*! Controls a 2D panel allowing to specify a swing rotation.
    The 2D panel uses the axis-angle parameterization of the
    swing, and ellipse-like joint limits are supported */
class SrSwingControl : public Fl_Widget
 { private :
    float _sx, _sy; // the axis angle representation of the swing
    float _ex, _ey; // ellipse limits (default PI)
    bool _swap;
    int _crossr;
    SrArray<SrPnt2> _points;

   public :

    /*! Construtor as required by fltk */
    SrSwingControl ( int x, int y, int w, int h, const char *l=0 );
    
    /*! Swap mode (default is true) puts the X (red) axis vertical and
        the Y (green) axis horizontal, better to visualize swing rotations */
    void swap_axis ( bool b ) { _swap=b; }
    
    /*! Set radius of the cross marking the current value (default is 3)*/
    void cross_radius ( int r ) { _crossr=r; }
    
    /*! Set new value, ensuring that ellipse limits are respected.
        True is returned if a new value is accepted */
    bool value ( float x, float y );
    
    /*! Returns the x value as float */
    float xval() const { return _sx; }

    /*! Returns the x value as float */
    float yval() const { return _sy; }

    /*! Define the X and Y radius of the ellipse limiting the swing.
        The default value is (pi,pi). Values are ensured to be <=pi */
    void ellipse ( float ex, float ey );
    
    /*! Returns the limiting ellipse x radius */
    float ellipsex () const { return _swap? _ey:_ex; }

    /*! Returns the limiting ellipse y radius */
    float ellipsey () const { return _swap? _ex:_ey; }
    
    /*! Send segments (as a point list) to be drawn in the swing pannel */
    void draw_segments ( const SrArray<SrPnt2>& pl ) { _points=pl; }

   protected :
    virtual void draw ();
    virtual int handle ( int e );
 };

//================================ End of File =================================================
