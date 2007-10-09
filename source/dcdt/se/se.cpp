#include "precompiled.h"

# include "se.h"

//================================= SeElement ====================================

SeElement::SeElement () 
 { 
   _index=0; 
   _symedge=0;
   _next=_prior=this;
 } 

SeElement* SeElement::_remove ()
 { 
   _next->_prior = _prior; 
   _prior->_next = _next; 
   return this; 
 }

// same insert_prior() implementation as in SrListNode
SeElement* SeElement::_insert ( SeElement* n )
 { 
   n->_prior = _prior;
   _prior->_next = n;
   n->_next = this;
   _prior = n;
   return n;
 } 

//=== End of File ===================================================================
