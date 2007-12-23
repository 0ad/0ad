#include "precompiled.h" 
#include "0ad_warning_disable.h"
# include <math.h>
# include "sr_box.h"
# include "sr_geo2.h"
# include "sr_polygon.h"

//# define SR_USE_TRACE1 
# include "sr_trace.h"
 
//=================================== SrPolygon =================================================

const char* SrPolygon::class_name = "Polygon";

SrPolygon::SrPolygon ( int s, int c ) : SrArray<SrVec2> ( s, c )
 {
   _open = 0;
 }

SrPolygon::SrPolygon ( const SrPolygon& p ) : SrArray<SrVec2> ( p )
 { 
   _open = p._open;
 }

SrPolygon::SrPolygon ( SrVec2* pt, int s, int c ) : SrArray<SrVec2> ( pt, s, c )
 {
   _open = 0;
 }

void SrPolygon::set_from_float_array ( const float* pt, int numv )
 {
   if ( numv<0 ) return;
   size ( numv );
   for ( int i=0; i<numv; i++ )
    (*this)[i].set( pt[i*2], pt[i*2+1] );
   _open = 0;
 }

bool SrPolygon::is_simple () const
 {
   int i, j, j1, end;

   if ( size()<3 ) return false; // degenerated polygon with 1 or 2 edges

   for ( i=0; i<size(); i++ )
    { end = i+size()-1;
      for ( j=i+2; j<size(); j++ )
       { j1 = (j+1)%size();
         if ( j1!=i && segments_intersect(get(i),get(i+1),get(j),get(j1)) )
          return false; // non adjacent edges crossing
       }
    }
   return true; // is simple
 }

bool SrPolygon::is_convex () const
 {
   int n, i, i1, i2;

   if ( size()<3 ) return false; // degenerated polygon with 1 or 2 edges

   float o, ordering;

   for ( i=0; i<size(); i++ )
    { i1 = (i+1)%size();
      i2 = (i+2)%size();
      ordering = ccw ( get(i), get(i1), get(i2) );
      if ( ordering!=0 ) break;
    }

   if ( ordering==0 ) return false; // not even a simple polygon

   for ( n=0; n<size(); n++ )
    { i1 = (i+1)%size();
      i2 = (i+2)%size();
      o = ccw ( get(i), get(i1), get(i2) );
      if ( o*ordering<0 ) return false; // not a convex angle
      i++;
    }

   return true;
 }

float SrPolygon::area () const // oriented: >0 if ccw
 {
   float sum=0;
   int i, j;
  
   if ( size()<=2 ) return 0; // degenerated polygon with 1 or 2 edges

   for ( i=0; i<size(); i++ )
    { j = validate(i+1);
      sum += get(i).x*get(j).y - get(j).x*get(i).y;
    }

   return sum/2.0f;
 }

static int interhoriz ( const SrVec2 &p, SrVec2 p1, SrVec2 p2 )
 {
   if ( p1.y>p2.y ) { SrVec2 tmp; SR_SWAP(p1,p2); }
   if ( p1.y>=p.y ) return false; // not intercepting
   if ( p2.y<p.y  ) return false; // not intercepting or = max 
   float x2 = p1.x + (p.y-p1.y) * (p2.x-p1.x) / (p2.y-p1.y);
   return (p.x<x2)? true:false;
 }

bool SrPolygon::contains ( const SrVec2& p ) const
 {
   int cont=0, i1, i2;
   for ( i1=0; i1<size(); i1++ )
    { i2 = (i1+1)%size();
      cont ^= interhoriz ( p, get(i1), get(i2) );
    }
   return cont? true:false; 
 }

bool SrPolygon::contains ( const SrPolygon& pol ) const
 {
   int i;
   for ( i=0; i<pol.size(); i++ )
    { if ( !contains(pol[i]) ) return false; }
   return true;
 }

int SrPolygon::has_in_boundary ( const SrVec2& p, float ds ) const
 {
   int i1, i2;
   for ( i1=0; i1<size(); i1++ )
    { i2 = (i1+1)%size();
      if ( in_segment ( get(i1), get(i2), p, ds ) ) return i1;
    }
   return -1;
 }

void SrPolygon::circle_approximation ( const SrVec2& center, float radius, int nvertices )
 {
   SrVec2 p;
   float ang=0, incang = ((float)SR_2PI) / nvertices;
   size(nvertices); // reserve memory
   size(0);
   while ( nvertices>0 )
    { p.set ( radius*sinf(ang), radius*cosf(ang) );
      p += center;
      push ( p );
      ang += incang;
      nvertices--;
    }
   _open = 0;
 }

float SrPolygon::perimeter () const
 {
   if ( size()<2 ) return 0;
   int i;
   float len=0;
   for ( i=1; i<size(); i++ ) len += dist ( const_get(i-1), const_get(i) );
   if ( !_open ) len += dist ( const_get(size()-1), const_get(0) );
   return len;
 }

SrPnt2 SrPolygon::interpolate_along_edges ( float t ) const
 {
   int i1, i2, ilast;
   float len1, len2=0;

   SrVec2 v;
   if ( size()==0 ) return v;
   if ( size()==1 || t<0 ) return const_get(0);
   ilast = _open? size()-1:0;

   v = const_get(0);
   if ( t<=0 ) return v;

   for ( i1=0; i1<size(); i1++ )
    { if ( i1==size()-1 )
       { if ( _open ) break;
         i2 = 0;
       }
      else i2 = i1+1;
      len1 = len2;
      len2 += dist ( const_get(i1), const_get(i2) );
      if ( t<len2 ) break;
    }
   
   if ( _open )
    { if ( i1==ilast ) return const_get(ilast); }
   else
    { if ( i2==0 ) return const_get(0); }

   len2 -= len1;
   t -= len1;
   t /= len2;
   v = lerp ( const_get(i1), const_get(i2), t );
   return v;
 }

void SrPolygon::resample ( float maxlen )
 {
   int i, nsub;
   SrVec2 v1, v2;
   float len;
   SrPolygon obsamp;
   obsamp.size(size());
   obsamp.size(0);

   for ( i=0; i<size(); i++ )
    { if ( i+1==size() && _open ) { obsamp.push()=top(); break; }
      v1 = get(i);
      v2 = get ( (i+1)%size() );
      obsamp.push() = v1;
      len = dist(v1,v2);
      if ( len>maxlen )
       { nsub = (int)(len/maxlen);
         len = len/(nsub+1);
         v2 = v2 - v1;
         v2.len(len);
         while ( nsub>0 )
          { v1+=v2; obsamp.push()=v1; nsub--; }
       }
    }
   take_data ( obsamp );
 }

void SrPolygon::remove_duplicated_vertices ( float epsilon )
 {
   int i1, i2;
   SrVec2 v1, v2;
   epsilon *= epsilon;
   i1 = 0;
   while ( i1<size() )
    { if ( i1+1==size() && _open ) break;
      i2 = (i1+1)%size();
      v1 = get(i1);
      if ( dist2(v1,v2)<=epsilon ) remove(i1);
       else i1++;
    }
 }

void SrPolygon::remove_collinear_vertices ( float epsilon )
 {
   int i=0, i1, i2;
   while ( i<size() )
    { if ( _open && i>=size()-2 ) break;
      i1 = (i+1)%size();
      i2 = (i+2)%size();
      if ( sr_point_line_dist ( get(i).x, get(i).y,
                                get(i1).x, get(i1).y,
                                get(i2).x, get(i2).y )<=epsilon )
       remove(i1);
      else
       i++;
    }
 }

//# define SE_CCW(ax,ay,bx,by,cx,cy) ((ax*by) + (bx*cy) + (cx*ay) - (bx*ay) - (cx*by) - (ax*cy))
//# define VCCW(p1,p2,p3) SE_CCW(p1.x,p1.y,p2.x,p2.y,p3.x,p3.y)

static void grow_corner
       ( const SrVec2& v1, const SrVec2& v2, const SrVec2& v3,
         float r, float maxang, bool grow, SrPolygon& obs )
 {
   enum MoreOrLessThanPi { LESS_THAN_PI, MORE_THAN_PI };
   MoreOrLessThanPi type;
   SrVec2 vec1, vec2;

   if ( grow )
    type = ccw(v1,v2,v3)>=0? MORE_THAN_PI:LESS_THAN_PI;
   else
    type = ccw(v1,v2,v3)>=0? LESS_THAN_PI:MORE_THAN_PI;

   vec1 = v1-v2; vec1.normalize();
   vec2 = v3-v2; vec2.normalize();

   // get corner separator points:
   if ( type==LESS_THAN_PI ) // get the bissector point
    { float ang = angle_fornormvecs(vec1,vec2) / 2.0f; // angle divided by 2
      float sinang = sinf(ang);
      if ( !grow ) ang=-ang;
      SrVec2 x (vec1);
      x.rot ( sinf(ang), cosf(ang) );
      x *= r/sinang;
      x += v2;

/*      if ( grow )
       { type = ccw(v1,x,v3)>0? MORE_THAN_PI:LESS_THAN_PI;
         if ( type==LESS_THAN_PI ) obs.push().set ( x.x, x.y );
       }
      else
       { obs.push().set ( x.x, x.y );
       }
*/
      obs.push().set ( x.x, x.y );
    }
   else // MORE_THAN_PI
    { SrVec2 sp1, sp2;
      sp1.set ( -vec1.y, vec1.x ); // 90 degrees rotation
      sp2.set ( vec2.y, -vec2.x ); // 90 degrees rotation
      float f = r;
      if ( !grow ) { vec1*=-1; vec2*=-1; f=-r; }
      sp1.len(f);
      sp2.len(f);
      sp1 += v2;
      sp2 += v2;

      // smooth:
      obs.push() = sp1;
      vec1 = sp1-v2; vec1.normalize();
      vec2 = sp2-v2; vec2.normalize();
      float ang = angle_fornormvecs(vec1,vec2);
      if ( ang>maxang )
       { int nsub = (int)(ang/maxang);
         float angstep = ang/(nsub+1);
         if ( !grow ) angstep*=-1;
         float s=sinf(angstep);
         float c=cosf(angstep);
         vec1.len ( r );
         while ( nsub-- )
          { vec1.rot(s,c);
            vec2 = v2 + vec1;
            obs.push() = vec2;
          }
       }
      obs.push() = sp2;
    }
 }

void SrPolygon::grow ( float radius, float maxangrad )
 {
   if ( radius==0 || size()<2 ) return;

   const float MIN = SR_TORAD(1);
   if ( maxangrad<MIN ) maxangrad=MIN;

   bool grow = radius>0? true:false;
   if ( !grow ) radius = -radius;

   int i;
   if ( _open )
    { for ( i=size()-02; i>0; i-- ) push()=get(i);
      _open=0;
      grow = true;
    }
   else
    { if ( size()<3 ) return;}

   SrVec2 v1, v2, v3;
   SrPolygon obgrow;
   obgrow.size(size()*2); obgrow.size(0);

   for ( i=0; i<size(); i++ )
    { v1 = get ( i );
      v2 = get ( (i+1)%size() );
      v3 = get ( (i+2)%size() );
      grow_corner ( v1, v2, v3, radius, maxangrad, grow, obgrow );
    }

   take_data ( obgrow );
 }

SrVec2 SrPolygon::centroid () const
 {
   SrVec2 c;
   int i;
   for ( i=0; i<size(); i++ ) c += get(i);
   c /= (float)size();
   return c;
 }

void SrPolygon::reverse ()
 {
   SrVec2 v;
   int i, j, m, s;
   s = size();
   m = s/2;
   for ( i=0; i<m; i++ )
    { j = s-1-i;
      v = get(i);
      (*this)[i] = (*this)[j];
      (*this)[j] = v;
    }
 }

void SrPolygon::translate ( const SrVec2& dv )
 {
   int i;
   for ( i=0; i<size(); i++ ) set ( i, get(i)+dv );
 }

void SrPolygon::rotate ( const SrVec2& center, float radians )
 {
   int i;
   float s = sinf ( radians );
   float c = cosf ( radians );

   for ( i=0; i<size(); i++ )
    { SrVec2& v = (*this)[i];
      v.rot ( center, s, c );
    }
 }

SrVec2 SrPolygon::south_pole ( int* index ) const
 {
   int i;
   SrVec2 p;
   if ( size()==0 ) return p;
   
   p=get(0); if (index) *index=0; 

   for ( i=1; i<size(); i++ )
    if ( get(i).y<p.y )
     { p=get(i); if (index) *index=i; }

   return p;
 }

void SrPolygon::convex_hull ( SrPolygon& pol ) const
  {
    int i, ini, i1, i2;
    float ang, angini;

    if ( size()<=3 ) { pol=*this; return; }
    south_pole(&i2);
    pol.size(size());
    pol.size(0);
    pol.push() = get(i2);

    i1 = validate ( i2-1 );
    i  = validate ( i2+1 );
    angini = ang = angle_max_ori ( get(i2)-get(i1), get(i)-get(i1) );
    ini = i2;

    while ( true )
     { 
       //sr_out<<ang<<": i1,i2,i = "<< i1 <<srspc<< i2 <<srspc<< i <<srnl;

       if ( angini*ang>=0 )
        { pol.push()=get(i); i1=i2; i2=i; if (i!=ini) i=validate(i+1);
        }
       else
        { pol.pop(); i2=i1; i1=(validate(i1-1)); }
       
       ang = angle_max_ori ( get(i2)-get(i1), get(i)-get(i1) );
       if ( i==ini && angini*ang>=0 ) break;
 
     }
    //sr_out<<"END: "<<ang<<": i1,i2,i = "<< i1 <<srspc<< i2 <<srspc<< i <<srnl;

  }

int SrPolygon::pick_vertex ( const SrPnt2& p, float epsilon )
 {
   int i, imin=-1;
   float dist, distmin=0;

   if ( size()==0 ) return imin;

   for ( i=1; i<size(); i++ )
    { dist = dist2(get(i),p);
      if ( dist<distmin || imin<0 ) { distmin=dist; imin=i; }
    }

   if ( distmin<=epsilon )
    return imin;
   else
    return -1;
 }

int SrPolygon::pick_edge ( const SrVec2& p, float epsilon, float& dist2 ) const
 { 
   int i, i2, iresult;
   float d;

   iresult=-1;
   dist2=-1;

   int s = size();
   for ( i=0; i<s; i++ )
    { i2 = (i+1)%s;
      if ( i2==0 && open() ) break;
      //sr_out<<const_get(i)<<srspc<<const_get((i+1)%size())<<srspc<<p<<srnl;
      if ( in_segment(const_get(i),const_get(i2),p,epsilon,d) )
       { if ( dist2<0 || d<dist2 )
          { iresult = i;
            dist2 = d;
          }
       }
    }
   //sr_out<<"result: "<<iresult<<srnl;
   return iresult;
 }

void SrPolygon::ear_triangulation ( SrArray<SrPnt2>& tris ) const
 {
   enum Case { TriangleOk, TriangleIntersects, DisconsiderPoint };
   Case c;
   int a1, a2, a3, b, size;
   float prec = 0.000001f;
   //sr_out<<pol<<srnl; printf("size %d\n",pol.size());
   tris.size(0); 

   SrPolygon pol(*this);
   if ( !pol.is_ccw() ) pol.reverse();

   while ( pol.size()>3 )
    { size = pol.size();
      for ( a2=pol.size()-1; a2>=0; a2-- )
       { a1 = pol.validate(a2-1);
         a3 = pol.validate(a2+1);
         //printf ("%d:\n",a1);
         if ( ccw ( pol[a1], pol[a2], pol[a3] )<=0 ) continue; // not ccw
         //printf ("CCW\n");

         c = TriangleOk;
         for ( b=0; b<pol.size(); b++ )
          { if ( b==a1 || b==a2 || b==a3 ) continue;

            if ( next(pol[a1],pol[b],prec) ||
                 next(pol[a2],pol[b],prec) ||
                 next(pol[a3],pol[b],prec) )
                { c=DisconsiderPoint; break; }

            if ( sr_in_triangle( pol[a1].x,pol[a1].y, pol[a2].x,pol[a2].y, pol[a3].x,pol[a3].y,
                                 pol[b].x,pol[b].y ) )
             { c=TriangleIntersects; break; }
          }
         
         if ( c==TriangleOk )
          { tris.push()=pol[a1]; tris.push()=pol[a2]; tris.push()=pol[a3];
            //sr_out<<pol[a1]<<srspc<<pol[a2]<<srspc<<pol[a3]<<srspc<<pol[b]<<srnl; 
            pol.remove(a2);
            break;
          }
         else if ( c==DisconsiderPoint )
          { pol.remove(b);
            break;
          }
       }
      if ( size==pol.size() ) break; // LOOP!!!
   }

  tris.push()=pol[0]; tris.push()=pol[1]; tris.push()=pol[2];
 }

void SrPolygon::get_bounding_box ( SrBox& b ) const
 { 
   int i;
   SrVec p;
   b.set_empty ();
   for ( i=0; i<size(); i++ )
    { p.set ( get(i).x, get(i).y, 0 );      
      b.extend ( p );
    }
 }

//================================ configs =================================================

void SrPolygon::get_configuration ( float& x, float& y, float& a ) const
 {
   SrVec2 c = centroid();
   x=c.x; y=c.y;
   a = angle_ori ( SrVec2::i, get(0)-c );
   if ( a<0 ) a += sr2pi;
 }

void SrPolygon::set_configuration ( float x, float y, float a )
 {
   SrVec2 c = centroid();
   SrVec2 nc(x,y);

   translate ( nc-c );

   float b = angle_ori ( SrVec2::i, get(0)-nc );
   if ( b<0 ) b += sr2pi;
   a = a-b;

   rotate ( nc, a );
 }

bool SrPolygon::intersects ( const SrPolygon& p ) const
 {
   int i, i2, s = p.size();

   for ( i=0; i<s; i++ )
    { i2 = (i+1)%s;
      if ( i2==0 && p.open() ) break;
      if ( intersects(p[i],p[i2]) ) return true;
    }

   return false;
 }

bool SrPolygon::intersects ( const SrVec2& p1, const SrVec2& p2 ) const
 {
   int i, i2, s = size();

   for ( i=0; i<s; i++ )
    { i2 = (i+1)%s;
      if ( i2==0 && open() ) break;
      if ( segments_intersect(p1,p2,get(i),get(i2)) ) return true;
    }

   return false;
 }

int sr_compare ( const SrPolygon* p1, const SrPolygon* p2 )
 {
   return 1;
 }

SrOutput& operator<< ( SrOutput& out, const SrPolygon& p )
 {
   if ( p.open() ) out<<"open ";
   return out << (const SrArray<SrPnt2>&)p;
 }

SrInput& operator>> ( SrInput& inp, SrPolygon& p )
 {
   inp.get_token();
   if ( inp.last_token_type()==SrInput::Name && inp.last_token()=="open" )
    p.open ( true );
   else
    inp.unget_token();

   return inp >> (SrArray<SrPnt2>&)p;
 }

//================================ End of File =================================================

