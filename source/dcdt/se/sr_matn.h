
/** \file sr_matn.h 
 * n dimensional matrix */

# ifndef SR_MATN_H
# define SR_MATN_H

class SrInput;
class SrOutput;

/*! \class SrMatn sr_matrix.h
    \brief resizeable, n-dimensional matrix of double elements

    All coordinates are considered starting from index 0. */
class SrMatn
 { private :
    double *_data;
    int _lin, _col;
   public :
    SrMatn ();
    SrMatn ( const SrMatn &m );
    SrMatn ( int m, int n );
    SrMatn ( double *data, int m, int n );
   ~SrMatn ();

    /*! Old elements are not preserved when realloc is done. */
    void size ( int m, int n );

    /*! returns the size of the matirx, ie, lin*col. */
    int size () const { return _lin*_col; }

    /*! resize also copies the old values to the new resized matrix. */
    void resize ( int m, int n );

    /*! resizes SrMatn and copy the contents of the submatrix [li..le,ci..ce]
        of m to SrMatn. */
    void set_as_sub_mat ( const SrMatn &m, int li, int le, int ci, int ce );

    /*! Copies the submatrix [li..le,ci..ce] of m to the SrMatn, without 
        reallocation, and putting the submatrix starting at coordinates 
        [l,c] of SrMatn. */
    void set_sub_mat ( int l, int c, const SrMatn &m,  int li, int le, int ci, int ce );

    /*! Resizes SrMatn as a column vector containing the data of the 
        s column of m. */
    void set_as_column ( const SrMatn &m, int s );

    /*! Copies the data of the column mc of m to the column c of SrMat. */
    void set_column ( int c, const SrMatn &m, int mc );

    int lin () const { return _lin; }
    int col () const { return _col; }
    void identity ();
    void transpose ();
    void swap_lines ( int l1, int l2 );
    void swap_columns ( int c1, int c2 );
    void set_all ( double r );
    void set_random ( double inf, double sup );

    /*! Returns the (lin*col)-dimensional vector norm. */
    double norm () const;

    operator const double* () const { return _data; }

    /*! indices start from 0. */
    double& operator [] ( int p ) { return _data[p]; }

    /*! indices start from 0. */
    double& operator () ( int i, int j ) { return _data[_col*i+j]; }

    void set ( int p, double r ) { _data[p]=r; }
    void set ( int i, int j, double r ) { _data[_col*i+j]=r; }

    double get ( int p ) const { return _data[p]; }
    double get ( int i, int j ) const { return _data[_col*i+j]; }

    void add ( const SrMatn& m1, const SrMatn& m2 );
    void sub ( const SrMatn& m1, const SrMatn& m2 );

    /*! Makes SrMatn be m1*m2. This method works properly with calls like 
        a.mult(a,b), a.mult(b,a), or a.mult(a,a). */
    void mult ( const SrMatn& m1, const SrMatn& m2 );

    void leave_data ( double *&m );
    void take_data ( SrMatn &m );

    SrMatn& operator =  ( const SrMatn& m );
    SrMatn& operator += ( const SrMatn& m );
    SrMatn& operator -= ( const SrMatn& m );

    friend SrOutput& operator << ( SrOutput &o, const SrMatn &m );

    /*! Returns the norm of the matrix a-b. */
    friend double dist ( const SrMatn &a, const SrMatn &b );

    /* Transforms a into its encoded LU decomposition. The L and U are returned
       in the same matrix a. L can be retrieved by making L=a and then setting 
       all elements of the diagonal to 1 and those above the diagonal to 0. U is 
       retrieved by making U=a, and then setting all elements below the diagonal
       to 0. Then the multiplication LU will give a', where a' is a with some
       row permutations. The row permutation history is stored in the returned
       int array, that tells, beginning from the first row, the other row index
       to swap sequentially the rows. This swap sequence will transform a to a'. 
       If pivoting is set to false, then no pivoting is done and a' will be equal
       to a. Parameter d returns +1 or -1, depending on whether the number of 
       row interchanges was even or odd, respectively. The function returns null
       if a is singular. See Numerical Recipes page 46. */
    friend const int* ludcmp ( SrMatn &m, double *d=0, bool pivoting=true );

    /*! Returns the explicit LU decomposition of a. Here we decompose the result
        of the encoded ludcmp version (without pivoting), returning the exact 
        l and u matrices so that lu=a.*/
    friend bool ludcmp ( const SrMatn &a, SrMatn &l, SrMatn &u );

    /*! Solves the linear equations ax=b. Here a is a n dimension square matrix,
        given in its LU encoded decomposition, b is a n dimensional column vector,
        that will be changed to return the solution vector x. indx is the row 
        permutation vector returned by ludcmp. */
    friend void lubksb ( const SrMatn &a, SrMatn &b, const int *indx );

    /*! Solve the system of linear equations ax=b using a LU decomposition. 
        Matrix a is changed to its encoded LU decomposition, and matrix b
        will be changed to contain the solution x. */
    friend bool lusolve ( SrMatn &a, SrMatn &b );

    /*! Same as the other lusolve(), but here const qualifiers are respected,
        what implies some extra temporary buffers allocation deallocation. */ 
    friend bool lusolve ( const SrMatn &a, const SrMatn &b, SrMatn &x );

    /*! Will change a to its encoded LU decomposition and return in inva
        the inverse matrix of a. */
    friend bool inverse ( SrMatn &a, SrMatn &inva );

    /*! Will put in a its inverse. */
    friend bool invert ( SrMatn &a );

    /* Returns the determinant of a using the LU decomposition method. The 
       given matrix a is transformed into the result of the ludcmp() function.
       See Numerical Recipes page 49. */
    friend double det ( SrMatn &a );

    /*! Solve the system ax=b, using the Gauss Jordan method. */
    friend bool gauss ( const SrMatn &a, const SrMatn &b, SrMatn &x );
 };

//============================== end of file ===============================

# endif // SR_MATN_H
