#include "precompiled.h"
# include "sr_box.h"
# include "sr_vec2.h"
# include "sr_points.h"

//# define SR_USE_TRACE1 // Constructor and Destructor
# include "sr_trace.h"

//======================================= SrPoints ====================================

const char* SrPoints::class_name = "Points";


SrPoints::SrPoints ()
 {
   SR_TRACE1 ( "Constructor" );
   A = 0;
 }

SrPoints::~SrPoints ()
 {
   SR_TRACE1 ( "Destructor" );
   delete A;
 }

void SrPoints::init ()
 {
   P.size(0);
 }

void SrPoints::init_with_attributes ()
 {
   P.size(0);
   if ( A )
    A->size(0);
   else A = new SrArray<Atrib>;
 }

void SrPoints::compress ()
 {
   P.compress();
   if ( A ) A->compress();
 }

void SrPoints::push ( const SrPnt& p )
 { 
   P.push() = p;
   if ( A ) A->push().s=1;
 }

void SrPoints::push ( const SrPnt2& p )
 { 
   P.push().set ( p.x, p.y, 0 ); 
   if ( A ) A->push().s=1;
 }

void SrPoints::push ( float x, float y, float z )
 { 
   P.push().set(x,y,z); 
   if ( A ) A->push().s=1;
 }

void SrPoints::push ( const SrPnt& p, SrColor c, float size )
 { 
   if ( !A ) return;
   P.push() = p;
   A->push().s=size;
   A->top().c = c;
 }

void SrPoints::push ( const SrPnt2& p, SrColor c, float size )
 { 
   if ( !A ) return;
   P.push().set ( p.x, p.y, 0 ); 
   A->push().s=size;
   A->top().c = c;
 }

void SrPoints::push ( float x, float y, SrColor c, float size )
 { 
   if ( !A ) return;
   P.push().set(x,y,0); 
   A->push().s=size;
   A->top().c = c;
 }

void SrPoints::push ( float x, float y, float z, SrColor c, float size )
 { 
   if ( !A ) return;
   P.push().set(x,y,z); 
   A->push().s=size;
   A->top().c = c;
 }

void SrPoints::get_bounding_box ( SrBox& b ) const
 { 
   int i;
   b.set_empty ();
   for ( i=0; i<P.size(); i++ ) 
    { b.extend ( P[i] );
    }
 }

//================================ EOF =================================================
