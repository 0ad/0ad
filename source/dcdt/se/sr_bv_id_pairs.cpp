# include "precompiled.h"
# include "sr_bv_id_pairs.h"

# define DEFAULT_SIZE 10

//============================== SrBvIdPairs ====================================

# define ORDER_IDS(id1,id2) { if (id1>id2) { SR_SWAPX(id1,id2); } } // make id1<id2

SrBvIdPairs::SrBvIdPairs ()
{
}

SrBvIdPairs::~SrBvIdPairs ()
{
  init ();
}

void SrBvIdPairs::add_pair ( int id1, int id2 ) //add a pair to the set.
{
  ORDER_IDS ( id1, id2 );  // order the ids
  if ( id1<0 ) return;
  
  while ( id1>=_arr.size() ) // increase the size of "_arr"
   { _arr.push() = 0; // reallocation is efficiently done when needed
   }
  
  Elem *current = _arr[id1]; //select the right list from "_arr".
  
  if ( !current )  //if the list is empty, insert the element in the front.
   { current = new Elem;
     current->id = id2;
     current->next = NULL;
     _arr[id1] = current;
   }
  else if (current->id > id2) //if the list is not empty but all
   {                          //elements are greater than id2, then
     current = new Elem;      //insert id2 in the front.
     current->id = id2;
     current->next = _arr[id1];
     _arr[id1] = current;
   }
  else
   { while (current->next != NULL)   // otherwise, find the correct location
	  {                              // in the sorted list (ascending order) 
	    if (current->next->id > id2) //and insert id2 there.
	      break;
	    current = current->next;
	  }
     if (current->id == id2) // already there
	  { return;
	  }
     else
	  { Elem *temp = new Elem;
	    temp->id = id2;
	    temp->next = current->next;
	    current->next = temp;
	  }
   }
}
  
void SrBvIdPairs::del_pair ( int id1, int id2 ) //delete a pair from the set.
{
  ORDER_IDS(id1, id2); //order the ids.
  
  if ( id1<0 || id1>=_arr.size() ) return; //the pair does not exist in the set, so return
  
  Elem *current = _arr[id1]; //otherwise, select the correct list.
  
  if ( !current ) return; //if this list is empty, the pair doesn't exist, so return
  
  if (current->id == id2)   //otherwise, if id2 is the first element, delete it
    { _arr[id1] = current->next;
      delete current;
      return;
    }
  else
    { while (current->next != NULL)    //if id2 is not the first element,
	   {                               //start traversing the sorted list.
	     if (current->next->id > id2)  //if you have moved too far away
	      {                            //without hitting id2, then the pair
	        return;                    //pair doesn't exist. So, return.
	      }
	     else if (current->next->id == id2)  //otherwise, delete id2.
	      { Elem *temp = current->next;
	        current->next = current->next->next;
	        delete temp;
	        return;
	      }
	     current = current->next;
       }
   }
}

void SrBvIdPairs:: del_pairs_with_id ( int id )  //delete all pairs containing id.
{
  int i;
  Elem *t;
  Elem *temp;
  
  if ( id<_arr.size() )
   { temp = _arr[id];
     while ( temp )
	  {	t = temp;
	    temp = temp->next;
	    delete t;
	  }
     _arr[id] = 0;
      
     for ( i=0; i<id; i++ )	del_pair(i,id);
   }
  else
   { for ( i=0; i<_arr.size(); i++ ) del_pair(i,id);
   }
}

void SrBvIdPairs::init ()  //delete all pairs from the set.
 {
   int i;
   int size = _arr.size();
   Elem *current;

   for ( i=0; i<size; i++ )
    { while ( _arr[i] )
       { current = _arr[i];
	     _arr[i] = current->next;
	     delete current;
	   }
    }

   _arr.size(0);
 };

bool SrBvIdPairs::pair_exists ( int id1, int id2 )
{
  ORDER_IDS ( id1, id2 );
  
  if ( id1>=_arr.size() || id1<0 || id2<0 ) return false; // the pair cannot exist
  
  Elem *current = _arr[id1];  //otherwise, get the correct list and look for id2
  while ( current )
   { if ( current->id==id2) return true;
     if ( current->id>id2)	return false;
     current = current->next;
   }
  return false;
}

int SrBvIdPairs::count_pairs ()
{
  int i, n = 0;
  
  Elem *current;
  for ( i=0; i<_arr.size(); i++ )
   { current = _arr[i];
     while ( current )
      { n++; current = current->next; }
   }

  return n;
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
