#include "precompiled.h"
# include "sr_box.h"
# include "sr_polygons.h"

//# define SR_USE_TRACE1 // Constructor and Destructor
# include "sr_trace.h"

//============================= SrPolygons ==================================

const char* SrPolygons::class_name = "Polygons";

SrPolygons::SrPolygons ()
 {
   SR_TRACE1 ( "Default Constructor" );
 }

SrPolygons::SrPolygons ( const SrPolygons& polys )
 {
   SR_TRACE1 ( "Copy Constructor" );
   int i;
   _data.size ( polys.size() );
   for ( i=0; i<_data.size(); i++ )
    _data[i] = new SrPolygon ( polys.const_get(i) );
 }

SrPolygons::~SrPolygons ()
 {
   SR_TRACE1 ( "Destructor" );
   size (0);
 }

void SrPolygons::size ( int ns )
 { 
   int i, s = _data.size();
   if ( ns>s )
    { _data.size(ns);
      for ( i=s; i<ns; i++ ) _data[i] = new SrPolygon;
    }
   else if ( ns<s )
    { for ( i=ns; i<s; i++ ) delete _data[i];
      _data.size(ns);
    }
 }

void SrPolygons::capacity ( int nc )
 {
   int i, s = _data.size();
   if ( nc<0 ) nc=0;
   if ( nc<s )
    { for ( i=nc; i<s; i++ ) delete _data[i];
    }
   _data.capacity(nc);
 }

void SrPolygons::swap ( int i, int j )
 {
   SrPolygon* tmp;
   SR_SWAP ( _data[i], _data[j] );
 }

void SrPolygons::insert ( int i, const SrPolygon& x )
 {
   _data.insert ( i );
   _data[i] = new SrPolygon(x);
 }

void SrPolygons::insert ( int i, int n )
 {
   _data.insert ( i, n );
   int j;
   for ( j=0; j<n; j++ )
    _data[i+j] = new SrPolygon;
 }

void SrPolygons::remove ( int i, int n )
 {
   int j;
   for ( j=0; j<n; j++ ) delete _data[i+j];
   _data.remove ( i, n );
 }

SrPolygon* SrPolygons::extract ( int i )
 {
   SrPolygon* p = _data[i];
   _data.remove ( i );
   return p;
 }

bool SrPolygons::pick_vertex ( const SrVec2& p, float epsilon, int& pid, int& vid ) const
 { 
   int i, j, s;
   float distmin, dist;

   if ( size()==0 ) return false;

   pid = vid = -1;
   distmin = 0;

   for ( i=0; i<size(); i++ )
    { s = const_get(i).size();
      for ( j=0; j<s; j++ )
       { dist = dist2 ( const_get(i,j), p );
         if ( dist<distmin || pid<0 ) { distmin=dist; pid=i; vid=j; }
       }
    }

   if ( distmin<=epsilon*epsilon )
    return true;
   else
    return false;
 }

int SrPolygons::pick_polygon ( const SrVec2& p ) const
 { 
   int i;
   for ( i=0; i<size(); i++ )
    if ( const_get(i).contains(p) ) return i;
   return -1;
 }

bool SrPolygons::pick_edge ( const SrVec2& p, float epsilon, int& pid, int& vid ) const
 { 
   float mindist2, dist2;
   int i, id;

   pid = vid = -1; 

   for ( i=0; i<size(); i++ )
    { id = const_get(i).pick_edge ( p, epsilon, dist2 );
      if ( id>=0 )
       { if ( vid<0 || dist2<mindist2 )
          { vid = id;
            pid = i;
            mindist2 = dist2;
          }
       }
    }

   return vid<0? false:true;
 }

int SrPolygons::intersects ( const SrVec2& p1, const SrVec2& p2 ) const
 {
   int i;
   for ( i=0; i<size(); i++ )
    if ( const_get(i).intersects(p1,p2) ) return i;

   return -1;
 }

int SrPolygons::intersects ( const SrPolygon& p ) const
 {
   int i;
   for ( i=0; i<size(); i++ )
    if ( const_get(i).intersects(p) ) return i;

   return -1;
 }

void SrPolygons::get_bounding_box ( SrBox& b ) const
 { 
   int i, j, s;
   SrVec p;
   b.set_empty ();
   for ( i=0; i<size(); i++ )
    { s = const_get(i).size();
      for ( j=0; j<s; j++ )
       { const SrVec2& p2 = const_get(i,j);
         p.set ( p2.x, p2.y, 0 );      
         b.extend ( p );
       }
    }
 }

void SrPolygons::operator = ( const SrPolygons& p )
 {
   size ( p.size() );
   int i;
   for ( i=0; i<p.size(); i++ ) get(i)=p.const_get(i);
 }

SrOutput& operator<< ( SrOutput& o, const SrPolygons& p )
 {
   int i, m;
   m = p.size()-1;
   o << '[';
   for ( i=0; i<=m; i++ )
    { o << p.const_get(i);
      if ( i<m ) o<<"\n ";
    }
   return o << "]\n";
 }

SrInput& operator>> ( SrInput& in, SrPolygons& p )
 {
   p.size(0);
   in.get_token();
   while (true)
    { in.get_token();
      if ( in.last_token()[0]==']' ) break;
      in.unget_token();
      p.push();
      in >> p.top();
    }
   return in;
 }

//================================ EOF =================================================
