
# ifndef SR_GRID_H
# define SR_GRID_H

/** \file sr_grid.h 
 * Manages data in a n-D regular grid */

# include "sr_vec.h"
# include "sr_vec2.h"
# include "sr_array.h"
# include "sr_output.h"
# include "sr_class_manager.h" 

/*! Describes how to subdivide one dimensional axis.
    This class is required to set up the decomposition in SrGrid */
class SrGridAxis
 { public :
    int   segs; // number of segments to divide this axis
    float min;  // minimum coordinate
    float max;  // maximum coordinate

   public :
    SrGridAxis ( int se, float mi, float ma )
     { segs=se; min=mi; max=ma; }

    void set ( int se, float mi, float ma )
     { segs=se; min=mi; max=ma; }

    friend SrOutput& operator<< ( SrOutput& o, const SrGridAxis& a )
     { return o << a.segs << srspc << a.min << srspc << a.max; }

    friend SrInput& operator>> ( SrInput& i, SrGridAxis& a )
     { return i >> a.segs >> a.min >> a.max; }
 };

/*! \class SrGridBase sr_grid.h
    \brief n-D regular grid base class

    SrGridBase has methods to translate n dimensional grid coordinates
    into a one dimension bucket array coordinate. Use the derived SrGrid
    template class to associate user data to the grid. */
class SrGridBase
 { private :
    SrArray<SrGridAxis> _axis;
    SrArray<int> _size; // subspaces sizes for fast bucket determination
    SrArray<float> _seglen; // axis segments lengths for fast cell determination
    int _cells;

   public :

    /*! Constructs a grid of given dimension (dim) and number of
        segments in each axis (ns). The number of cells will then be 
        ns^dim. Axis are normalized in [0,1] */
    SrGridBase ( int dim=0, int ns=0 );

    /*! Init the grid with given dimension and number of
        segments in each axis. The number of cells will then be 
        ns^dim. Axis are normalized in [0,1] */
    void init ( int dim, int ns );

    /*! Destroy the existing grid and initializes one according to
        the descriptions sent in the desc array, which size will
        define the dimension of the grid and thus the dimension
        of the index tuple to specify a grid */
    void init ( const SrArray<SrGridAxis>& axis_desc );

    const SrArray<SrGridAxis>& axis_desc() const { return _axis; }

    float max_coord ( int axis ) const { return _axis[axis].max; }
    float min_coord ( int axis ) const { return _axis[axis].min; }
    int segments ( int axis ) const { return _axis[axis].segs; }
    float seglen ( int axis ) const { return _seglen[axis]; }

    /*! Returns the number of dimensions of the grid */
    int dimensions () const { return _axis.size(); }

    /*! Returns the total number of cells in the grid */
    int cells () const { return _cells; }

    /*! Get the index of a cell from its coordinates. 2D version. */
    int cell_index ( int i, int j ) const;

    /*! Get the index of a cell from its coordinates. 3D version */
    int cell_index ( int i, int j, int k ) const;

    /*! Get the index of a cell from its coordinates. Multidimensional version */
    int cell_index ( const SrArray<int>& coords ) const;

    /*! Get the cell coordinates from its index. 2D version. */
    void cell_coords ( int index, int& i, int& j ) const;

    /*! Get the cell coordinates from its index. 3D version. */
    void cell_coords ( int index, int& i, int& j, int& k ) const;

    /*! Get the cell coordinates from its index. Multidimensional version. */
    void cell_coords ( int index, SrArray<int>& coords ) const;

    /*! Get the lower and upper coordinates of the Euclidian 2D box of cell (i,j). */
    void cell_boundary ( int i, int j, SrPnt2& a, SrPnt2& b ) const;

    /*! Get the lower and upper coordinates of the Euclidian 3D box of cell (i,j,k). */
    void cell_boundary ( int i, int j, int k, SrPnt& a, SrPnt& b ) const;

    /*! Get the indices of all cells intersecting with the given box.
        If a box boundary is exactly at a grid separator, only the cell
        with greater index is considered. Indices are just pushed to
        array cells, which is not emptied before being used */
    void get_intersection ( SrPnt2 a, SrPnt2 b, SrArray<int>& cells ) const;

    /*! 3D version of get_intersection() */
    void get_intersection ( SrPnt a, SrPnt b, SrArray<int>& cells ) const;

    /*! Returns the index of the cell containing a, or -1 if a is outside the grid */
    int get_point_location ( SrPnt2 a ) const;

    /*! Returns the index of the cell containing a, or -1 if a is outside the grid */
    int get_point_location ( SrPnt a ) const;
 };

/*! \class SrGrid sr_grid.h
    \brief n-D regular grid template class

    SrGrid defines automatic type casts to the user type.
    WARNING: SrGrid is designed to efficiently store large
    amount of cells and it uses SrArray<X>, which implies
    that no constructors or destructors of type X are called.
    The user must initialize and delete data properly. */
template <class X>
class SrGrid : public SrGridBase
 { private :
    SrArray<X> _data;

   public :
    SrGrid ( int dim=0, int ns=0 ) : SrGridBase ( dim, ns )
     { _data.size(SrGridBase::cells()); }

    void init ( int dim, int ns )
     { SrGridBase::init ( dim, ns );
       _data.size(SrGridBase::cells());
     }

    void init ( const SrArray<SrGridAxis>& axis_desc )
     { SrGridBase::init ( axis_desc );
       _data.size(SrGridBase::cells());
     }

    /*! Returns the cell of given index, that should be in 0<=i<cells() */
    X& operator[] ( int index ) { return _data[index]; }

    /*! Returns the cell of given 2D coordinates */
    X& operator() ( int i, int j )
     { return _data[SrGridBase::cell_index(i,j)]; }

    /*! Returns the cell of given 3D coordinates */
    X& operator() ( int i, int j, int k )
     { return _data[SrGridBase::cell_index(i,j,k)]; }

    /*! Returns the cell of given n-D coordinates */
    X& operator() ( const SrArray<int>& coords )
     { return _data[SrGridBase::cell_index(coords)]; }
 };

//============================== end of file ===============================

# endif // SR_LIST_H


