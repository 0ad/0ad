#include "precompiled.h"
//# include <stdlib.h>

# include "sr_sa.h"
# include "sr_sn_group.h"
# include "sr_sn_editor.h"
# include "sr_sn_matrix.h"

//# define SR_USE_TRACE1 // constructor / destructor
# include "sr_trace.h"

//=============================== SrSa ====================================

SrSa::SrSa () 
 { 
   SR_TRACE1 ( "Constructor" );

   _matrix_stack.capacity ( 8 );
   _matrix_stack.push ( SrMat::id );
 }

SrSa::~SrSa ()
 {
   SR_TRACE1 ( "Destructor" );
 }

bool SrSa::apply ( SrSn* n, bool init )
 {
   if ( init ) init_matrix ();

   apply_begin ();

   bool result;
   int t = n->type();

   if ( t==SrSn::TypeMatrix )
    {
      result = matrix_apply ( (SrSnMatrix*) n );
    }
   else if ( t==SrSn::TypeGroup )
    {
      result = group_apply ( (SrSnGroup*) n );
    }
   else if ( t==SrSn::TypeShape )
    {
      result = shape_apply ( (SrSnShapeBase*) n );
    }
   else if ( t==SrSn::TypeEditor )
    {
      result = editor_apply ( (SrSnEditor*) n );
    }
   else
    { sr_out.fatal_error ( "Undefined type &d in SrSa::apply()!", t );
      result = false;
    }

   apply_end ();
   return result;
 }

//--------------------------------- virtuals -----------------------------------

void SrSa::get_top_matrix ( SrMat& mat )
 {
   mat = _matrix_stack.top();
 }

int SrSa::matrix_stack_size ()
 {
   return _matrix_stack.size();
 }

void SrSa::init_matrix ()
 {
   _matrix_stack.size ( 1 );
   _matrix_stack[0] = SrMat::id;
 }

void SrSa::mult_matrix ( const SrMat& mat )
 {
   SrMat stackm = _matrix_stack.top(); 
   _matrix_stack.top().mult ( mat, stackm ); // top = mat * top
 }

void SrSa::push_matrix ()
 {
   _matrix_stack.push();
   _matrix_stack.top() = _matrix_stack[ _matrix_stack.size()-2 ];
 }

void SrSa::pop_matrix ()
 {
   _matrix_stack.pop();
 }

bool SrSa::matrix_apply ( SrSnMatrix* m )
 {
   if ( !m->visible() ) return true;
   mult_matrix ( m->get() );
   return true;
 }

bool SrSa::group_apply ( SrSnGroup* g )
 {
   bool b=true;
   int i, s;

   if ( !g->visible() ) return true;

   if ( g->separator() ) push_matrix();

   s = g->size();

   for ( i=0; i<s; i++ )
    { b = apply ( g->get(i), false );
      if ( !b ) break;
    }

   if ( g->separator() ) pop_matrix();
   return b;
 }

bool SrSa::shape_apply ( SrSnShapeBase* s )
 {
   return true; // implementation specific to the derived class
 }

bool SrSa::editor_apply ( SrSnEditor* e )
 {
   SrSnGroup* h = e->helpers();
   SrSn* c = e->child();
   if ( !c ) return true;

   push_matrix ();
   mult_matrix ( e->mat() );

   bool b = apply ( c, false );

   if ( b && h && e->visible() )
    { int i, s = h->size();
      for ( i=0; i<s; i++ )
       { b = apply ( h->get(i), false );
         if ( !b ) break;
       }
    }

   pop_matrix();
   return b;
 }

//======================================= EOF ====================================
