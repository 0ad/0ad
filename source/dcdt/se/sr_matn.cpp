#include "precompiled.h"
# include <string.h>
# include <math.h>

# include "sr_random.h"
# include "sr_output.h"
# include "sr_matn.h"

//# define SR_USE_TRACE1 // const/dest
# include "sr_trace.h"

# define MAT(m,n) _data[_col*m+n]

SrMatn::SrMatn () : _lin(0), _col(0), _data(0)
 {
   SR_TRACE1 ("Default Constructor");
 }

SrMatn::SrMatn ( const SrMatn &m ) : _lin(m._lin), _col(m._col)
 {
   _data = new double[_lin*_col];
   // reminder: memcpy does not work with overlap, but memmove yes.
   memcpy ( _data, m._data, sizeof(double)*_lin*_col );
   SR_TRACE1 ("Copy Constructor\n");
 }

SrMatn::SrMatn ( int m, int n ) : _lin(m), _col(n)
 {
   _data = new double[_lin*_col];
   SR_TRACE1 ("Size Constructor\n");
 }

SrMatn::SrMatn ( double *data, int m, int n ) : _lin(m), _col(n)
 {
   _data = data;
   SR_TRACE1 ("Take Data Constructor\n");
 }

SrMatn::~SrMatn ()
 {
   delete [] _data;
   SR_TRACE1 ("Destructor\n");
 }

void SrMatn::size ( int m, int n )
 {
   SR_POS(m);
   SR_POS(n);
   int s = m*n;

   if ( s==0 )
    { delete [] _data;
      _data=0;
      m = n = 0;
    }
   else if ( !_data )
    { _data = new double[s];
    }
   else if ( s!=_lin*_col ) 
    { delete [] _data;
      _data = new double[s];
    }

   _lin=m; _col=n;
 }

void SrMatn::resize ( int m, int n )
 {
   SrMatn mat ( m, n );
   int lin = SR_MIN(m,_lin);
   int col = SR_MIN(n,_col);

   for ( int i=0; i<lin; i++ )
    for ( int j=0; j<col; j++ )
      mat(i,j) = _data[_col*j+i];

   take_data ( mat );
 }

void SrMatn::set_as_sub_mat ( const SrMatn &m, int li, int le, int ci, int ce )
 {
   size ( le-li+1, ce-ci+1 );

   for ( int i=li; i<=le; i++ )
    for ( int j=ci; j<=ce; j++ )
      set ( i-li, j-ci, m.get(i,j) );
 }

void SrMatn::set_sub_mat ( int l, int c, const SrMatn &m,  int li, int le, int ci, int ce )
 {
   for ( int i=li; i<=le; i++ )
    for ( int j=ci; j<=ce; j++ )
      set ( l+i-li, c+j-ci, m.get(i,j) );
 }

void SrMatn::set_as_column ( const SrMatn &m, int s )
 {
   size ( m.lin(), 1 );

   for ( int i=0; i<_lin; i++ )
     set ( i, 0, m.get(i,s) );
 }

void SrMatn::set_column ( int c, const SrMatn &m, int mc )
 {
   for ( int i=0; i<_lin; i++ )
     set ( i, c, m.get(i,mc) );
 }

void SrMatn::identity ()
 {
   int s=_lin*_col, d=_col+1;
   for ( int i=0; i<s; i++ )
    _data[i] = i%d? 0.0:1.0;
 }

void SrMatn::transpose ()
 {
   int i, j;

   if ( _lin==_col )
    { double tmp;
      for ( i=0; i<_lin; i++ )
       for ( j=i+1; j<_col; j++ )
         SR_SWAP ( _data[_col*i+j], _data[_col*j+i] );
    }
   else if ( _lin==1 || _col==1 )
    { int tmp;
      SR_SWAP ( _lin, _col );
    }
   else
    { SrMatn m ( _col, _lin );
      for ( i=0; i<_lin; i++ )
       for ( j=0; j<_col; j++ )
        m(j,i) = get(i,j);
      take_data ( m );
    }
 }

void SrMatn::swap_lines ( int l1, int l2 )
 {
   int i;
   double tmp, *p1, *p2;
   p1 = _data+(_col*l1);
   p2 = _data+(_col*l2);
   for ( i=0; i<_col; i++ ) SR_SWAP ( p1[i], p2[i] ); 
 }

void SrMatn::swap_columns ( int c1, int c2 )
 {
   int i;
   double tmp;
   for ( i=0; i<_lin; i++ ) SR_SWAP ( MAT(i,c1), MAT(i,c2) ); 
 }

void SrMatn::set_all ( double r )
 {
   int m=_lin*_col;
   while ( m ) _data[--m] = r;
 }

void SrMatn::set_random ( double inf, double sup )
 {
   SrRandom r(inf,sup);
   int m=_lin*_col;
   while ( m ) _data[--m] = r.getd();
 }

double SrMatn::norm () const
 {
   if ( !_data ) return 0.0;

   int s = size();
   double sum = 0.0;
   for ( int i=0; i<s; i++ ) sum += _data[i]*_data[i];

   return sqrt ( sum );
 }

void SrMatn::add ( const SrMatn& m1, const SrMatn& m2 )
 { 
   size ( m1.lin(), m1.col() );
   int s = size();
   for ( int i=0; i<s; i++ ) _data[i] = m1._data[i] + m2._data[i];
 }

void SrMatn::sub ( const SrMatn& m1, const SrMatn& m2 )
 {
   size ( m1.lin(), m1.col() );
   int s = size();
   for ( int i=0; i<s; i++ ) _data[i] = m1._data[i] - m2._data[i];
 }

void SrMatn::mult ( const SrMatn& m1, const SrMatn& m2 )
 {
   int l, c, i, j, k, klast; 
   double sum;

   SrMatn *m = (&m1==this || &m2==this)? new SrMatn : this;

   l = m1._lin;
   c = m2._col;
   m->size(l,c);

   klast = SR_MIN(m1._col,m2._lin);
   for ( i=0; i<l; i++ )
    for ( j=0; j<c; j++ )
     { sum = 0;
       for ( k=0; k<klast; k++ ) sum += m1.get(i,k) * m2.get(k,j);
       m->set(i,j,sum);
     }

   if ( m!=this ) { *this=*m; delete m; }
 }

void SrMatn::leave_data ( double*& m )
 {
   m = _data;
   _data = 0;
   _lin = _col = 0;
 }

void SrMatn::take_data ( SrMatn &m )
 {
   if ( this==&m ) return;
   if ( _data ) delete []_data;
   _data = m._data; m._data=0;
   _lin  = m._lin;  m._lin=0;
   _col  = m._col;  m._col=0;
 }

//============================ Operators =========================================

SrMatn& SrMatn::operator = ( const SrMatn& m )
 {
   if ( this != &m )
    { size ( m._lin, m._col );
      memcpy ( _data, m._data, sizeof(double)*_lin*_col );
    }
   return *this;
 }

SrMatn& SrMatn::operator += ( const SrMatn& m )
 {
   int s = size();
   for ( int i=0; i<s; i++ ) _data[i]+=m.get(i);
   return *this;
 }

SrMatn& SrMatn::operator -= ( const SrMatn& m )
 {
   int s = size();
   for ( int i=0; i<s; i++ ) _data[i]-=m.get(i);
   return *this;
 }

//================================= friends ==================================

SrOutput& operator << ( SrOutput &o, const SrMatn &m )
 {
   for ( int i=0; i<m.lin(); i++ )
    { for ( int j=0; j<m.col(); j++ )
       { o<<m.get(i,j)<<srspc; }
      o<<srnl;
    }
   return o;
 }

double dist ( const SrMatn &a, const SrMatn &b )
 {
   SrMatn m ( a.lin(), a.col() );
   m.sub ( a, b );
   return m.norm();
 }

# define TINY 1.0e-20;

const int *ludcmp ( SrMatn &a, double *d, bool pivoting )
 {
   int i, imax, j, k;
   double big, sum, tmp;
   int n=a._lin;

   static double *vv=0;    // vv stores the implicit scaling of each row
   static int *indx=0;     // row permutation buffer
   static int cur_size=0;

   if ( cur_size<n ) 
    { delete[] indx; indx=new int[n]; 
      delete[] vv; vv=new double[n];
      cur_size=n; 
    }

   if (d) *d=1.0; // no row interchanges yet

   for ( i=0; i<n; i++ ) // loop over rows to get the scaling
    { big = 0.0;
      for ( j=0; j<n; j++ ) { tmp=SR_ABS(a(i,j)); if ( tmp>big ) big=tmp; }
      if ( big==0.0 ) { sr_out.warning("Singular matrix in routine ludcmp"); return 0; }
      vv[i]=1.0/big; // save the scaling
    }

   for ( j=0; j<n; j++ ) // loop over columns of the Crout's method
    { for ( i=0; i<j; i++ ) 
       { sum = a(i,j);
         for ( k=0; k<i; k++ ) sum -= a(i,k)*a(k,j);
         a(i,j)=sum;
       }
      big = 0.0; // search for largest pivot element
      for ( i=j; i<n; i++ ) 
       { sum = a(i,j);
         for ( k=0; k<j; k++ ) sum -= a(i,k)*a(k,j);
         a(i,j) = sum;
         tmp = vv[i]*SR_ABS(sum);
         if ( tmp>=big) { big=tmp; imax=i; }
       }
      
      if ( pivoting )
       { if ( j!=imax ) // interchange rows if needed
          { for ( k=0; k<n; k++ ) SR_SWAP ( a(imax,k), a(j,k) );
            if (d) *d = -*d;
            vv[imax]=vv[j];
          }
         indx[j]=imax;
       }
      else indx[j]=j;

      if ( a(j,j)==0.0 ) a(j,j)=TINY;
      if ( j!=n-1 )
       { tmp = 1.0/a(j,j);
         for ( i=j+1; i<n; i++ ) a(i,j) *= tmp;
       }
    }
   return indx;
 }

bool ludcmp ( const SrMatn &a, SrMatn &l, SrMatn &u )
 {
   u = a;
   const int *indx = ludcmp(u,0,false);
   if ( !indx ) return false;

   int n = a.lin();
   l.size ( n, n );

   for ( int i=0; i<n; i++ )
    for ( int j=0; j<n; j++ )
     { if ( i>j ) { l(i,j)=u(i,j); u(i,j)=0.0; }
        else { l(i,j) = i==j? 1.0:0.0; }
     }

   return true;
 }

void lubksb ( const SrMatn &a, SrMatn &b, const int *indx )
 {
   int i, ii=-1, ip, j;
   double sum;
   int n=a.lin();

   for ( i=0; i<n; i++ )
    { ip = indx[i];
      sum = b[ip];
      b[ip] = b[i];
      if (ii>=0) { for ( j=ii; j<=i-1; j++ ) sum -= a.get(i,j)*b[j]; }
       else if (sum) ii=i;
      b[i]=sum;
	}

   for ( i=n-1; i>=0; i-- ) 
    { sum = b[i];
      for ( j=i+1; j<n; j++ ) sum -= a.get(i,j)*b[j];
      b[i] = sum/a.get(i,i);
    }
 }

bool lusolve ( SrMatn &a, SrMatn &b )
 {
   const int *indx = ludcmp ( a );
   if ( !indx ) return false;
   lubksb ( a, b, indx );
   return true;
 }

bool lusolve ( const SrMatn &a, const SrMatn &b, SrMatn &x )
 {
   SrMatn lu(a);
   x = b;
   return lusolve ( lu, x );
 }

bool inverse ( SrMatn &a, SrMatn &inva )
 {
   int j, k, n = a.lin();
   inva.size(n,n);

   static double *buf=0;
   static int cur_size=0;
   if ( cur_size<n ) { delete[] buf; buf=new double[n]; cur_size=n; }
   SrMatn b(buf,n,1);

   const int *indx = ludcmp ( a );
   if ( !indx ) return false;

   for ( j=0; j<n; j++ )
    { for ( k=0; k<n; k++ ) b[k]=0.0;
      b[j]=1.0;
      lubksb ( a, b, indx );
      for ( k=0; k<n; k++ ) inva(k,j)=b[k]; // same as: inva.set_column(j,b,0);
    }
   
   b.leave_data(buf);
   return true;
 }

bool invert ( SrMatn &a )
 {
   SrMatn inva;
   if ( !inverse ( a, inva ) ) return false;
   a.take_data(inva);
   return true;
 }

double det ( SrMatn &a )
 {
   int n = a.lin();
   double d;
   ludcmp ( a, &d );
   for ( int i=0; i<n; i++ ) d *= a(i,i);
   return d;
 }

/* adapted from num recipes code (but not yet ok) : */
/*
bool gauss2 ( SrMatn &a, SrMatn &b, SrMatn &x )
 {
   int n = a.lin();
   int m = b.col();
   int i, icol, irow, j, k, l, ll;
   double big, pivinv, tmp;

   static int *indxc=0;
   static int *indxr=0;
   static int *ipiv=0;
   static int cur_size=0;
   if ( cur_size<n ) 
    { delete[] indxc; indxc=new int[n]; 
      delete[] indxr; indxr=new int[n]; 
      delete[] ipiv;  ipiv=new int[n]; 
      cur_size=n; 
    }

   for ( j=0; j<n; j++ ) ipiv[j]=0;

   for ( i=0; i<n; i++ )
    { big=0.0;
      for ( j=0; j<n; j++ )
       { if ( ipiv[j]==0 ) continue;
         for ( k=0; k<n; k++ )
          { if ( ipiv[k]==0 ) 
             { tmp=SR_ABS(a(j,k));
               if ( tmp>=big) { big=tmp; irow=j; icol=k; }
             } 
            else if ( ipiv[k]>0 ) 
             { sr_out.warning("gaussj: Singular Matrix-1"); return false; }
          }
		 ++(ipiv[icol]);
		 if ( irow!=icol )
          { for ( l=0; l<n; l++ ) SR_SWAP( a(irow,l), a(icol,l) )
            for ( l=0; l<m; l++ ) SR_SWAP( b(irow,l), b(icol,l) )
          }

		 indxr[i]=irow;
		 indxc[i]=icol;
		 if ( a(icol,icol)==0.0 ) { sr_out.warning("gaussj: Singular Matrix-2"); return false; }

         pivinv = 1.0/a(icol,icol);
         a(icol,icol)=1.0;

         for ( l=0; l<n; l++ ) a(icol,l) *= pivinv;
         for ( l=0; l<m; l++ ) b(icol,l) *= pivinv;

         for ( ll=0; ll<n; ll++ )
          { if ( ll==icol) continue;
            tmp=a(ll,icol);
            a(ll,icol)=0.0;
            for ( l=0; l<n; l++ ) a(ll,l) -= a(icol,l)*tmp;
            for ( l=0; l<m; l++ ) b(ll,l) -= b(icol,l)*tmp;
          }
	   }
      for ( l=n-1; l>=0; l-- ) 
       { if ( indxr[l]==indxc[l] ) continue;
         for ( k=0; k<n; k++ )
           SR_SWAP ( a(k,indxr[l]), a(k,indxc[l]) );
       }
	}
   return true;
 }
*/

// my gauss implementation :
bool gauss ( const SrMatn &a, const SrMatn &b, SrMatn &x )
 {
   int i, j, k, n, piv_lin;
   double tmp, piv_val;
   static SrMatn m;

   n = a.lin();
   x.size ( n, 1 );
   m.size ( n, n+1 );
   m.set_sub_mat ( 0, 0, a, 0, n-1, 0, n-1 );
   m.set_column ( n, b, 0 );

   for ( i=0; i<n; i++ ) // loop lines
    {
      piv_lin=i; piv_val=SR_ABS(m(i,i));
      for ( k=i+1; k<n; k++ ) // loop lines below i
        { tmp=SR_ABS(m(k,i));
          if ( tmp>piv_val ) { piv_lin=k; piv_val=tmp; }
        }
      if ( i!=piv_lin ) m.swap_lines(i,piv_lin);

      tmp = m(i,i);
      if ( tmp==0.0 ) { sr_out.warning("singular matrix in gauss\n"); return false; }
      for ( k=i+1; k<n; k++ )
        for ( j=n; j>=i; j-- )
           m(k,j) = m(k,j)-m(i,j)*m(k,i)/tmp;
    }

   for ( i=n-1; i>=0; i-- )
    { tmp = 0.0;
      for ( k=i+1; k<n; k++ ) tmp += m(i,k)*x[k];
      if ( m(i,i)==0.0 ) { sr_out.warning("singular matrix in gauss (2)\n"); return false; }
      x[i] = (m(i,n)-tmp) / m(i,i);
    }

   return true;
 }

/* Adapted from num. rec., it works fine. a,b,c are the vectors of each column
of the tridiagonal matrix m, to find u such that m*u=r. n is the number of lines of m
void tridag ( float* a, float* b, float* c, float* r, float* u, int n )
 {
   int j;
   float bet;

   static int s=0;
   static float* gam=0;

   if ( s<n ) { s=n; gam=(float*)realloc(gam,s*sizeof(float)); }

   if ( b[0]==0 ) sr_out.warning("Error 1 in tridag");

   u[0]=r[0]/(bet=b[0]);

   for ( j=1; j<n; j++ )
    { gam[j] = c[j-1]/bet;
	  bet = b[j]-a[j]*gam[j];
	  if ( bet==0.0 ) sr_out.warning("Error 2 in tridag");
	  u[j] = (r[j]-a[j]*u[j-1])/bet;
	}

   for ( j=(n-2); j>=0; j-- ) u[j] -= gam[j+1]*u[j+1];
 }
*/

/* From my old libraries, should work.
static void gauss_band ( SrMatn& A, SrMatn& B )
 {
   int i, j, k, c;
   int band=(A.col()-2)/2;
   int diag=band+1;       // diagonal
   int last=col-1;    // last column of band TMatrix
   real fact, div;

   if (col%2!=0 || col>lin || pointer==null) return gaBadSize;
   if ( !v.setSize(lin,1) ) return gaNoMemory;

   for ( i=1; i<=lin; i++ )
    { c=col;
      div = MAT(i,band+1);
      if ( div==0.0 ) return gaZeroDivision;
      for ( j=i-band; j<=i+band && j<=lin; j++ )
       { c--;
         if (j<=i || j<1) continue;
         fact = MAT(j,c)/div;
         MAT(j,col) = MAT(j,col)-MAT(i,col)*fact;
         for ( k=last; k>=1; k-- )
           {  if (k+j-i>last) continue;
              MAT(j,k) = MAT(j,k)-MAT(i,k+j-i)*fact;
           }
       }
    }
   for ( j=lin; j>=1; j-- )
    {
      fact = 0.0; c=diag;
      for ( k=j+1; k<=j+band && k<=lin; k++ )
        { ++c; fact += MAT(j,c)*v.get(k); }
      if ( MAT(j,diag)==0.0 ) return gaZeroDivision;
      v(j) = ( (MAT(j,col)-fact) / MAT(j,diag) );
    }
   return gaOk;
 }
*/

//================================= End Of File ==================================
