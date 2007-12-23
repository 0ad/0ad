#include "precompiled.h"
#include "0ad_warning_disable.h"
# include <math.h>
# include "sr_bv_nbody.h"
# include "sr_model.h"
# include "sr_box.h"
# include "sr_mat.h"
# include "sr_trace.h"

# define SR_USE_TRACE1 // function calls trace

//============================== SrBvNBody::EndPoint ====================================

/* EndPoint stores either the (min_x, min_y, min_z)
   or the (max_x, max_y, max_z) end-point of an AABB.
   Each instance of EndPoint appears simultaneously in three sorted linked
   lists, one for each dimension, as required by the sweep and prune algorithm.
   A "min" and the correspoding "max" end-point give us three intervals, one
   for each axis, which are actually the projections of the AABB on each of the
   three co-ordinate axii. */
enum MinMax { MIN=1, MAX=2 };
struct SrBvNBody::EndPoint
{ char        minmax;     //whether it represents a "MIN" or a "MAX" end-point
  srbvreal    val[3];     //the coordinates of the EndPoint.
  EndPoint*   prev[3];    //for maintaining the three linked
  EndPoint*   next[3];    //lists.
  AABB        *aabb;      //back pointer to the parent AABB.
};

//============================== SrBvNBody::AABB ====================================

/* Class AABB is used to store information about an axis aligned bounding box.
   The AABB here has the same "radius" along all the three axii. As a result
   of this, even if the object rotates about some arbitrary point and axis, 
   the radius of the enclosing AABB need not be changed.
   Computing the center and radius of the AABBs:
   Let P1, P2, ... , Pn be the position vectors of the end-points of the given
   object. Then, the center of the enclosing AABB is the centroid of the object:
      center = (P1 + P2 + ... + Pn) / n
   The radius is given by: radius = max_i (dist(Pi, center)) */
struct SrBvNBody::AABB
{ int          id;        // the id of the enclosed object
  srbvreal     center[3]; // center of the AABB
  srbvreal     radius;    // radius of the AABB
  EndPoint     *lo;       // the (min_x, min_y, min_z) and  
  EndPoint     *hi;       // (max_x, max_y, max_z) corners of the AABB. These point to
                          //  the corresponding nodes in the NBody linked lists.

  AABB ( EndPoint* l, EndPoint* h )
   { lo = l;
     lo->minmax = MIN;
     lo->aabb = this;
     hi = h;
     hi->minmax = MAX;
     hi->aabb = this;
   }

  AABB ()
   { lo = new EndPoint;
     lo->minmax = MIN;
     lo->aabb = this;
     hi = new EndPoint;
     hi->minmax = MAX;
     hi->aabb = this;
   }
   
 ~AABB ()
   { delete lo;
     delete hi;
   }
   
  /* min/max holds the min/max endpoints of the box, but considering the
     maximum radius as the size in all directions; this allows rotation
     of the models w/o the need to recompute the boxes */
  void compute_min_max ()
   { lo->val[0] = center[0] - radius; 
     lo->val[1] = center[1] - radius;
     lo->val[2] = center[2] - radius;
     hi->val[0] = center[0] + radius;
     hi->val[1] = center[1] + radius;
     hi->val[2] = center[2] + radius;
   }
   
  friend bool overlaps ( AABB* obj1, AABB* obj2 )//check it the two AABBs overlap
    {
      int coord;
      for (coord=0; coord<3; coord++)
       { if (obj1->lo->val[coord] < obj2->lo->val[coord])
          { if (obj2->lo->val[coord] > obj1->hi->val[coord]) return false; }
         else
          { if (obj1->lo->val[coord] > obj2->hi->val[coord]) return false; }
       }
      return true;
    }
};         

//============================== SrBvNBody ====================================

const int REVERSE  = 1; //whether the direction of movement of the interval
const int FORWARD  = 2; //along the list is forward (FORWARD), reverse (REVERSE)
const int NOCHANGE = 3; //or there is no movement at all (NOCHANGE).

SrBvNBody::SrBvNBody()  //constructor.
{
  _elist[0] = NULL;
  _elist[1] = NULL;
  _elist[2] = NULL;
}

SrBvNBody::~SrBvNBody()  //destructor.
{
  init ();
}

void SrBvNBody::init()
{
  while ( _aabb.size()>0 )
   { delete _aabb.pop();
   }

  _elist[0] = NULL;
  _elist[1] = NULL;
  _elist[2] = NULL;

  overlapping_pairs.init ();
}

bool SrBvNBody::insert_object ( int id, const SrModel& m )
{
  SR_TRACE1 ( "insert_object" );

  // Increase the dynamic array if necessary, and store the new aabb
  while ( id>=_aabb.size() ) _aabb.push()=0;
  if ( _aabb[id] ) return false; // error: id already exists, so just return
  AABB* aabb = new AABB;
  _aabb[id] = aabb;

  // Set the id to the given value
  aabb->id = id;
  
  // Get the center of the AABB, instead of the centroid as vcollide does
  SrBox box;
  m.get_bounding_box(box);
  SrPnt boxc = box.center();
  aabb->center[0] = boxc.x;
  aabb->center[1] = boxc.y;
  aabb->center[2] = boxc.z;

  // The "radius" of the AABB is computed as the maximum distance of the AABB
  // center from any of the vertices of the object.
  float dist2_, distmax=0;
  int i, mvsize=m.V.size();
  for ( i=0; i<mvsize; i++ )
   { dist2_ = dist2 ( boxc, m.V[i] );
     if ( dist2_>distmax ) distmax=dist2_;
   }
  aabb->radius = (srbvreal) sqrt(double(distmax));
  aabb->radius *= (srbvreal)1.0001;  //add a 0.01% buffer.
  aabb->compute_min_max ();

  // Now, check the overlap of this AABB with with all other AABBs and
  // add the pair to the set of overlapping pairs if reqd.
  int size = _aabb.size();
  for ( i=0; i<size; i++ ) 
   { if ( _aabb[i] )      
	  if ( overlaps(aabb,_aabb[i]) )
	    _add_pair ( aabb->id, i );
    }

  // Now, for each of the three co-ordinates, insert the interval
  // in the correspoding list. 
  int coord;
  for ( coord=0; coord<3; coord++ )
   { //first insert the "hi" endpoint.
     if ( _elist[coord]==NULL ) // if the list is empty, insert in front.
	  { _elist[coord] = aabb->hi;
	    aabb->hi->prev[coord] = aabb->hi->next[coord] = NULL;
	  }
     else  //otherwise insert in the correct location (the list is sorted)
      { _list_insert ( coord, aabb->hi ); }

     // Now, insert the "lo" endpoint. The list cannot be empty since we
     // have already inserted the "hi" endpoint
     _list_insert ( coord, aabb->lo );
   }

  SR_TRACE1 ( "insert_object end" );
  return true;
}

bool SrBvNBody::update_transformation ( int id, const SrMat& m )
{
  SR_TRACE1 ( "update_transformation" );

  if ( id>=_aabb.size() ) return false;
  if ( !_aabb[id] ) return false;
  
  AABB *aabb = _aabb[id]; // the given object exists
  
  // compute the new position of the AABB center
  SrPnt c ( aabb->center );
  srbvvec new_center;
  new_center[0] = m.get(0)*c.x + m.get(4)*c.y + m.get(8)*c.z + m.get(12);
  new_center[1] = m.get(1)*c.x + m.get(5)*c.y + m.get(9)*c.z + m.get(13);
  new_center[2] = m.get(2)*c.x + m.get(6)*c.y + m.get(10)*c.z + m.get(14);

  // vcollide version:
  //double new_center[3]; // multiply: [trans] * [center]
  //new_center[0] = aabb->center[0] * trans[0][0] + aabb->center[1] * trans[0][1] + aabb->center[2] * trans[0][2] + trans[0][3];
  //new_center[1] = aabb->center[0] * trans[1][0] + aabb->center[1] * trans[1][1] + aabb->center[2] * trans[1][2] + trans[1][3];
  //new_center[2] = aabb->center[0] * trans[2][0] + aabb->center[1] * trans[2][1] + aabb->center[2] * trans[2][2] + trans[2][3];

  // compute the new min and max endpoints
  // (aabb min/max values are updated later on)
  srbvreal min[3], max[3];
  min[0] = new_center[0] - aabb->radius;
  min[1] = new_center[1] - aabb->radius;
  min[2] = new_center[2] - aabb->radius;
  max[0] = new_center[0] + aabb->radius;
  max[1] = new_center[1] + aabb->radius;
  max[2] = new_center[2] + aabb->radius;
  
  // we need these so that we can use the same function overlaps(AABB *, AABB *)
  // to check overlap of the newly transformed object with other objects
  EndPoint lo, hi;
  AABB dummy(&lo,&hi);
  lo.val[0]=min[0]; lo.val[1]=min[1]; lo.val[2]=min[2];
  hi.val[0]=max[0]; hi.val[1]=max[1]; hi.val[2]=max[2];

  //update all the three lists by moving the endpoint to correct position.
  EndPoint *temp;
  int direction;
  int coord;
  for ( coord=0; coord<3; coord++ )
   { // set the direction of motion of the endpoint along the list
     if ( aabb->lo->val[coord] > min[coord] )
	  direction = REVERSE;
     else if ( aabb->lo->val[coord] < min[coord] )
	  direction = FORWARD;
     else
	  direction = NOCHANGE;
      
     if ( direction==REVERSE ) // backward motion
	  { // first update the "lo" endpoint of the interval
	    if ( aabb->lo->prev[coord]!=NULL )
	     { temp = aabb->lo;
	       while ( (temp != NULL) && (temp->val[coord] > min[coord]))
		    { if (temp->minmax == MAX)
		       if (overlaps(temp->aabb, &dummy))
		         _add_pair(temp->aabb->id, aabb->id);
		  	  temp = temp->prev[coord];
		    }
	       if (temp == NULL)
		    { aabb->lo->prev[coord]->next[coord] = aabb->lo->next[coord];
		      aabb->lo->next[coord]->prev[coord] = aabb->lo->prev[coord];
		      aabb->lo->prev[coord] = NULL;
		      aabb->lo->next[coord] = _elist[coord];
		      _elist[coord]->prev[coord] = aabb->lo;
		      _elist[coord] = aabb->lo;
		    }
	       else
		    { aabb->lo->prev[coord]->next[coord] = aabb->lo->next[coord];
		      aabb->lo->next[coord]->prev[coord] = aabb->lo->prev[coord];
		      aabb->lo->prev[coord] = temp;
		      aabb->lo->next[coord] = temp->next[coord];
		      temp->next[coord]->prev[coord] = aabb->lo;
		      temp->next[coord] = aabb->lo;
		    }
	     }
	    aabb->lo->val[coord] = min[coord];
        //then update the "hi" endpoint of the interval.
	    if (aabb->hi->val[coord] != max[coord])
	     { temp = aabb->hi;
	       while (temp->val[coord] > max[coord])
		    { if ( (temp->minmax == MIN) && (overlaps(temp->aabb, aabb)) )
		       _del_pair(temp->aabb->id, aabb->id);
		  	  temp = temp->prev[coord];
		  	}
	       aabb->hi->prev[coord]->next[coord] = aabb->hi->next[coord];
	       if (aabb->hi->next[coord] != NULL)
		     aabb->hi->next[coord]->prev[coord] = aabb->hi->prev[coord];
	       aabb->hi->prev[coord] = temp;
	       aabb->hi->next[coord] = temp->next[coord];
	       if (temp->next[coord] != NULL)
		     temp->next[coord]->prev[coord] = aabb->hi;
	       temp->next[coord] = aabb->hi;
           aabb->hi->val[coord] = max[coord];
	     }
	  }
     else if (direction == FORWARD) // forward motion
	  { // here, we first update the "hi" endpoint
	    if ( aabb->hi->next[coord] != NULL )
	     { temp = aabb->hi;
	       while ( (temp->next[coord] != NULL) && (temp->val[coord] < max[coord]) )
		    { if (temp->minmax == MIN)
		       if (overlaps(temp->aabb, &dummy))
		         _add_pair(temp->aabb->id, aabb->id);
		  	  temp = temp->next[coord];
		    }
	       if (temp->val[coord] < max[coord])
		    { aabb->hi->prev[coord]->next[coord] = aabb->hi->next[coord];
		      aabb->hi->next[coord]->prev[coord] = aabb->hi->prev[coord];
		      aabb->hi->prev[coord] = temp;
		      aabb->hi->next[coord] = NULL;
		      temp->next[coord] = aabb->hi;
		    }
	       else if (aabb->hi->val[coord] != max[coord])
		    { aabb->hi->prev[coord]->next[coord] = aabb->hi->next[coord];
		      aabb->hi->next[coord]->prev[coord] = aabb->hi->prev[coord];
		      aabb->hi->prev[coord] = temp->prev[coord];
		      aabb->hi->next[coord] = temp;
		      temp->prev[coord]->next[coord] = aabb->hi;
		      temp->prev[coord] = aabb->hi;
		    }
	     }
	    aabb->hi->val[coord] = max[coord];
        //then, update the "lo" endpoint of the interval.
	    temp = aabb->lo;
	    while (temp->val[coord] < min[coord])
	     { if ( (temp->minmax == MAX) && (overlaps(temp->aabb, aabb)) )
		    _del_pair(temp->aabb->id, aabb->id);
	       temp = temp->next[coord];
	     }
	    if (aabb->lo->prev[coord] != NULL)
	     { aabb->lo->prev[coord]->next[coord] = aabb->lo->next[coord]; }
	    else
	     { _elist[coord] = aabb->lo->next[coord]; }
	    aabb->lo->next[coord]->prev[coord] = aabb->lo->prev[coord];
	    aabb->lo->prev[coord] = temp->prev[coord];
	    aabb->lo->next[coord] = temp;
	    if (temp->prev[coord] != NULL)
	     { temp->prev[coord]->next[coord] = aabb->lo; }
	    else
	     { _elist[coord] = aabb->lo; }
	    temp->prev[coord] = aabb->lo;
	    aabb->lo->val[coord] = min[coord];
	  }
   }

  // make sure AABB destructor does not delete the used static vars:
  dummy.lo = 0;
  dummy.hi = 0;

  SR_TRACE1 ( "update_transformation end" );

  return true;  
}

bool SrBvNBody::remove_object ( int id )
{
  SR_TRACE1 ( "remove_object" );

  if ( id>=_aabb.size() ) return false;
  if ( !_aabb[id] ) return false;
  
  // 1st get the AABB to be deleted
  AABB *aabb = _aabb[id];

  // Now "remove it" from the AABB array
  if ( id+1==_aabb.size() )
    _aabb.pop();
  else
   _aabb[id] = NULL;
  
  // Now we delete all the three intervals from the corresponding lists
  int coord;
  for ( coord=0; coord<3; coord++ )
   { // first delete the "lo" endpoint of the interval
     if ( aabb->lo->prev[coord]==NULL )
	  _elist[coord] = aabb->lo->next[coord];
     else
	  aabb->lo->prev[coord]->next[coord] = aabb->lo->next[coord];
     aabb->lo->next[coord]->prev[coord] = aabb->lo->prev[coord];
     // then, delete the "hi" endpoint
     if ( aabb->hi->prev[coord]==NULL )
	  _elist[coord] = aabb->hi->next[coord];
     else
	  aabb->hi->prev[coord]->next[coord] = aabb->hi->next[coord];
     if ( aabb->hi->next[coord]!=NULL )
	  aabb->hi->next[coord]->prev[coord] = aabb->hi->prev[coord];
    }
  
  // Delete all entries involving this id from the set of overlapping pairs
  overlapping_pairs.del_pairs_with_id ( id );
  
  // de-allocate the memory
  delete aabb;

  SR_TRACE1 ( "remove_object end" );

  return true;
}

//=========================== private methods ===================================

void SrBvNBody::_list_insert ( int coord, EndPoint* newpt )
 {
   // Find the correct location in the list and insert there (the list is sorted)
   EndPoint* current = _elist[coord];
   while ( current->next[coord] && current->val[coord]<newpt->val[coord] )
	 current = current->next[coord];
   if ( current->val[coord]>=newpt->val[coord] ) // insert before
	{ newpt->prev[coord] = current->prev[coord];
	  newpt->next[coord] = current;
	  if ( !current->prev[coord] )
		_elist[coord] = newpt;
	  else
	    current->prev[coord]->next[coord] = newpt;
      current->prev[coord] = newpt;
	}
   else // insert at the end
    { newpt->prev[coord] = current;
	  newpt->next[coord] = NULL;
	  current->next[coord] = newpt;
	}
 }

/************************************************************************\

  Copyright 1997 The University of North Carolina at Chapel Hill.
  All Rights Reserved.

  Permission to use, copy, modify and distribute this software
  and its documentation for educational, research and non-profit
  purposes, without fee, and without a written agreement is
  hereby granted, provided that the above copyright notice and
  the following three paragraphs appear in all copies.

  IN NO EVENT SHALL THE UNIVERSITY OF NORTH CAROLINA AT CHAPEL
  HILL BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL,
  INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS,
  ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION,
  EVEN IF THE UNIVERSITY OF NORTH CAROLINA HAVE BEEN ADVISED OF
  THE POSSIBILITY OF SUCH DAMAGES.


  Permission to use, copy, modify and distribute this software
  and its documentation for educational, research and non-profit
  purposes, without fee, and without a written agreement is
  hereby granted, provided that the above copyright notice and
  the following three paragraphs appear in all copies.

  THE UNIVERSITY OF NORTH CAROLINA SPECIFICALLY DISCLAIM ANY
  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
  PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
  BASIS, AND THE UNIVERSITY OF NORTH CAROLINA HAS NO OBLIGATION
  TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
  MODIFICATIONS.


   --------------------------------- 
  |Please send all BUG REPORTS to:  |
  |                                 |
  |   geom@cs.unc.edu               |
  |                                 |
   ---------------------------------
  
     
  The authors may be contacted via:

  US Mail:  A. Pattekar/J. Cohen/T. Hudson/S. Gottschalk/M. Lin/D. Manocha
            Department of Computer Science
            Sitterson Hall, CB #3175
            University of N. Carolina
            Chapel Hill, NC 27599-3175
	    
  Phone:    (919)962-1749
	    
  EMail:    geom@cs.unc.edu

\************************************************************************/
