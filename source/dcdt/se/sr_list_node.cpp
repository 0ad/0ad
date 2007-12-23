#include "precompiled.h"
#include "0ad_warning_disable.h"
# include "sr_list_node.h"

//=============================== SrListNode ================================

SrListNode *SrListNode::remove_next ()
 { 
   SrListNode *n = _next;  
   _next = _next->_next; 
   _next->_next->_prior = this;
   return n;
 }

SrListNode *SrListNode::remove_prior ()
 { 
   SrListNode *n = _prior; 
   _prior = _prior->_prior; 
   _prior->_prior->_next = this;
   return n;
 }

SrListNode *SrListNode::remove ()
 { 
   _next->_prior = _prior; 
   _prior->_next = _next; 
   return this; 
 }

SrListNode *SrListNode::replace ( SrListNode *n )
 { 
   _next->_prior = n;
   _prior->_next = n;
   n->_next  = _next;
   n->_prior = _prior;
   return this;
 } 

SrListNode *SrListNode::insert_next ( SrListNode *n )
 { 
   n->_next = _next;
   n->_next->_prior = n;
   n->_prior = this;
   _next = n;
   return n;
 } 

SrListNode *SrListNode::insert_prior ( SrListNode *n )
 { 
   n->_prior = _prior;
   _prior->_next = n;
   n->_next = this;
   _prior = n;
   return n;
 } 

void SrListNode::splice ( SrListNode *n ) 
 { 
   _next->_prior = n;
   n->_next->_prior = this;
   SrListNode *nxt=_next;
   _next = n->_next;
   n->_next = nxt;

/* This has the same result as:
   SrListNode *tmp;
   SR_SWAP ( _next->_prior, n->_next->_prior );
   SR_SWAP ( _next,         n->_next         );
*/
 }

void SrListNode::swap ( SrListNode *n )
 { 
   SrListNode *nn=n->_next, *p=_prior;

   if ( _next!=n ) { _prior=n->_prior; n->_next=_next; } else { _prior=n; n->_next=this;  }
   if ( nn!=this ) { _next=nn;         n->_prior=p;    } else { _next=n;  n->_prior=this; }

   _prior->_next=this;
   _next->_prior=this;
   n->_prior->_next=n;
   n->_next->_prior=n;
 }

//============================== end of file ===============================

