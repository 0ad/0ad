#include "precompiled.h"

# include "se_dcdt.h"

//============================== SeDcdtVertex ==================================

static void insert_if_not_there ( SrArray<int>& a, int id )
 {
   int i=0;
   for ( i=0; i<a.size(); i++ )
    if ( a[i]==id ) return;
   a.push() = id;
 }

static void insert_ids ( SrArray<int>& a, const SrArray<int>& ids )
 {
   int i;
   for ( i=0; i<ids.size(); i++ ) insert_if_not_there ( a, ids[i] );
 }

void SeDcdtVertex::get_references ( SrArray<int>& ids )
 {
   SeDcdtEdge *e;
   SeDcdtSymEdge *si, *s;

   ids.size ( 0 );
   si = s = se();
   do { e = s->edg();
        if ( e->is_constrained() ) insert_ids ( ids, e->ids );
        s = s->rot();
      } while ( s!=si );
 }

SrOutput& operator<< ( SrOutput& out, const SeDcdtVertex& v )
 {
   return out << v.p;
 }

SrInput& operator>> ( SrInput& inp, SeDcdtVertex& v )
 { 
   return inp >> v.p;
 }

//=============================== SeDcdtEdge ==================================

bool SeDcdtEdge::has_id ( int id ) const
 {
   for ( int i=0; i<ids.size(); i++ )
    if ( ids[i]==id ) return true;
   return false;
 }

bool SeDcdtEdge::has_other_id_than ( int id ) const
 {
   for ( int i=0; i<ids.size(); i++ )
    if ( ids[i]!=id ) return true;
   return false;
 }

bool SeDcdtEdge::remove_id ( int id ) // remove all occurences
 {
   bool removed=false;
   int i=0;
   while ( i<ids.size() )
    { if ( ids[i]==id )
       { ids[i]=ids.pop(); removed=true; }
      else
       i++;
    }
   return removed;
 }

void SeDcdtEdge::add_constraints ( const SrArray<int>& cids )
 {
   int i;
   for ( i=0; i<cids.size(); i++ ) ids.push() = cids[i];
 }

SrOutput& operator<< ( SrOutput& out, const SeDcdtEdge& e )
 {
   return out << e.ids;
 }

SrInput& operator>> ( SrInput& inp, SeDcdtEdge& e )
 {
   return inp >> e.ids;
 }

//============================== SeDcdtTriManager ==================================

void SeDcdtTriManager::get_vertex_coordinates ( const SeVertex* v, double& x, double & y )
 {
   x = (double) ((SeDcdtVertex*)v)->p.x;
   y = (double) ((SeDcdtVertex*)v)->p.y;
 }

void SeDcdtTriManager::set_vertex_coordinates ( SeVertex* v, double x, double y )
 {
   ((SeDcdtVertex*)v)->p.x = (float) x;
   ((SeDcdtVertex*)v)->p.y = (float) y;
 }

bool SeDcdtTriManager::is_constrained ( SeEdge* e )
 {
   return ((SeDcdtEdge*)e)->is_constrained();
 }

void SeDcdtTriManager::set_unconstrained ( SeEdge* e )
 {
   ((SeDcdtEdge*)e)->set_unconstrained();
 }

void SeDcdtTriManager::get_constraints ( SeEdge* e, SrArray<int>& ids )
 {
   ids = ((SeDcdtEdge*)e)->ids;
 }

void SeDcdtTriManager::add_constraints ( SeEdge* e, const SrArray<int>& ids )
 {
   ((SeDcdtEdge*)e)->add_constraints ( ids );
 }

//============================== SeDcdtTriManager ==================================

SrOutput& operator<< ( SrOutput& o, const SeDcdtInsPol& ob )
 {
   if (ob.open) o << "(open) ";
   return o;// << (const SrArray<SeDcdtVertex*>)ob;
 }

SrInput& operator>> ( SrInput& in, SeDcdtInsPol& ob )
 {
   in.get_token();
/*
   if ( in.last_token()[0]=='(' )
    { ob.open = true;
      in.get_token();
      in.get_token();
    }
   else
    { ob.open = false;
      in.unget_token();
    }
*/
   return in;// >> (SrArray<SeDcdtVertex*>)ob;
 }

//=============================== End of File ===============================
