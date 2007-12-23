#include "precompiled.h"
#include "0ad_warning_disable.h"
# include "sr_bv_coldet.h"

//============================= SrBvColdet::Object ===========================

/*! Contains required data to be stored in each BvTree. */
class SrBvColdet::Object
 { public :
    bool active;
    srbvmat R;
    srbvvec T;
    SrBvTree tree;
   public :
    Object ()
     { SrBvMath::Midentity ( R ); // set to id matrix
       SrBvMath::Videntity ( T ); // set to a null vector
       active = true;
     }

   ~Object () {};

    void set_transformation ( const SrMat& srm )
     {
       # define SRM(i) (srbvreal)srm.get(i)
       // here we pass from column-major to line-major order:
       R[0][0]=SRM(0); R[1][0]=SRM(1); R[2][0]=SRM(2);
       R[0][1]=SRM(4); R[1][1]=SRM(5); R[2][1]=SRM(6);
       R[0][2]=SRM(8); R[1][2]=SRM(9); R[2][2]=SRM(10);
       T[0]=SRM(12); T[1]=SRM(13); T[2]=SRM(14);
       # undef SRM
     }

    void make_tree ( const SrModel& model )
     { tree.make ( model );
     }
 };

class SrBvColdetObjectManager : public SrClassManagerBase
 { public :
    virtual void* alloc () { return (void*) new SrBvColdet::Object; }
    virtual void* alloc ( const void* obj ) { return (void*) 0; } // copy not used/supported
    virtual void free ( void* obj ) { delete (SrBvColdet::Object*) obj; }
 };

//================================ SrBvColdet =====================================

SrBvColdet::SrBvColdet ()
 {
   _set = new SrSet<Object> ( new SrBvColdetObjectManager );
 }
  
SrBvColdet::~SrBvColdet ()
 {
   delete _set;
 }

void SrBvColdet::init ()
 { 
   _set->init ();
   _nbody.init ();
   _disabled.init ();
   _pairs.size (0);
 }

int SrBvColdet::insert_object ( const SrModel& m )
 {
   int id = _set->insert ();
   Object* o = _set->get ( id );
   o->make_tree ( m );
   _nbody.insert_object ( id, m );
   return id;
 }

void SrBvColdet::remove_object ( int id )
 {
   Object* o = _getobj ( id );
   if ( !o ) return;
   _set->remove(id); // will also delete o
   _disabled.del_pairs_with_id ( id );
   _nbody.remove_object ( id );
 }

void SrBvColdet::update_transformation ( int id, const SrMat& m )
 {
   Object* o = _getobj ( id );
   if ( !o ) return;
   o->set_transformation ( m );
   _nbody.update_transformation ( id, m );
 }

void SrBvColdet::activate_object ( int id )
 {
   Object* o = _getobj ( id );
   if ( !o ) return;
   o->active = true;
 }

void SrBvColdet::deactivate_object ( int id )
 {
   Object* o = _getobj ( id );
   if ( !o ) return;
   o->active = false;
 }

void SrBvColdet::activate_pair ( int id1, int id2 )
 {
   _disabled.del_pair ( id1, id2 );
 }
    
void SrBvColdet::deactivate_pair ( int id1, int id2 )
 {
   _disabled.add_pair ( id1, id2 );
 }

//============================= private method ================================

bool SrBvColdet::_collide ( SrBvTreeQuery::CollideFlag flag )
 {
   _pairs.size ( 0 );
      
   const SrArray<SrBvIdPairs::Elem*>& overlap = _nbody.overlapping_pairs.elements();
   const SrArray<SrBvIdPairs::Elem*>& disabled = _disabled.elements();
   int osize = overlap.size();
   int dsize = disabled.size();
   
   bool call_collide;
   Object* o1;
   Object* o2;
   const SrBvIdPairs::Elem* curr_overlap;
   const SrBvIdPairs::Elem* curr_disabled;
   
   // Simultaneously traverse overlap and disabled,
   // and make collision queries only when required.
   int i;
   for ( i=0; i<osize; i++ )
    { o1 = _set->get(i); //overlap[i]->id);
      if ( !o1 ) continue;
      if ( !o1->active ) continue; // this objects is not active

      curr_disabled = i<dsize? disabled[i]:0;
      curr_overlap = overlap[i];

      while ( curr_overlap )
	   { o2 = _set->get(curr_overlap->id);

         // update curr_disabled position:
		 while ( curr_disabled )
		  { if ( curr_disabled->id<curr_overlap->id )
		     curr_disabled = curr_disabled->next;
		    else break;
		  }
		  
		 call_collide = false;
		 if ( !curr_disabled ) call_collide=true;
		  else
		 if ( curr_disabled->id>curr_overlap->id ) call_collide=true;
		  else
		 curr_disabled=curr_disabled->next;

         //sr_out<<"COLLIDING "<<i<<" and "<<curr_overlap->id<<srnl;
		  
		 if ( call_collide && o2->active )
		  {
		    _query.collide ( o1->R, o1->T, &(o1->tree), o2->R, o2->T, &(o2->tree), flag );
		    _query.colliding_pairs().size();
		    if ( _query.colliding_pairs().size()>0 ) // had collisions
		     { _pairs.push()=i;
		       _pairs.push()=curr_overlap->id;
		       if ( flag==SrBvTreeQuery::CollideFirstContact ) return true;
		     }
		   }
		 curr_overlap = curr_overlap->next;
	   }
    }
  return _pairs.size()>0? true:false;
 }

bool SrBvColdet::_collide_tolerance ( float toler )
 {
   const SrArray<SrBvIdPairs::Elem*>& overlap = _nbody.overlapping_pairs.elements();
   const SrArray<SrBvIdPairs::Elem*>& disabled = _disabled.elements();
   int osize = overlap.size();
   int dsize = disabled.size();
   
   bool call_tolerance;
   Object* o1;
   Object* o2;
   const SrBvIdPairs::Elem* curr_overlap;
   const SrBvIdPairs::Elem* curr_disabled;
   
   // Simultaneously traverse overlap and disabled,
   // and make collision queries only when required.
   int i;
   for ( i=0; i<osize; i++ )
    { o1 = _set->get(i); //overlap[i]->id);
      if ( !o1 ) continue;
      if ( !o1->active ) continue; // this objects is not active

      curr_disabled = i<dsize? disabled[i]:0;
      curr_overlap = overlap[i];

      while ( curr_overlap )
	   { o2 = _set->get(curr_overlap->id);

         // update curr_disabled position:
		 while ( curr_disabled )
		  { if ( curr_disabled->id<curr_overlap->id )
		     curr_disabled = curr_disabled->next;
		    else break;
		  }
		  
		 call_tolerance = false;
		 if ( !curr_disabled ) call_tolerance=true;
		  else
		 if ( curr_disabled->id>curr_overlap->id ) call_tolerance=true;
		  else
		 curr_disabled=curr_disabled->next;
		  
		 if ( call_tolerance && o2->active )
		  { if ( _query.tolerance( o1->R, o1->T, &(o1->tree), o2->R, o2->T, &(o2->tree), toler ) )
		     return true;
		  }
		 curr_overlap = curr_overlap->next;
	   }
    }
  return false;
 }

//=================================== EOF =====================================

