#include "precompiled.h"
# include "sr_tree.h"

//# define SR_USE_TRACE1   
# include "sr_trace.h"

# define BLACK(x) ((x)->color==SrTreeNode::Black)
# define RED(x)   ((x)->color==SrTreeNode::Red)
# define NIL      SrTreeNode::null

/*==========================================================================
  Note:
  A binary search tree is a red-black tree if it satisfies : 
  1. Every node is either red or black
  2. Every null leaf is black
  3. If a node is red then both its children are black
  4. Every simple path from a node to a descendant leaf contains the
     same number of black nodes
============================================================================*/

//=============================== SrTreeNode ====================================

static SrTreeNode static_null(SrTreeNode::Black);

SrTreeNode* SrTreeNode::null = &static_null;

//=============================== SrTreeBase ====================================

//----------------------------- private methods ------------------------------

/*! Returns 0  and leaves _cur pointing to the found matching node if there is one.
    Returns >0 if the key is to be inserted under _cur->right.
    Returns <0 if the key is to be inserted under _cur->left.
    If the tree is empty, _cur will be null and the integer returned is 1. */
int SrTreeBase::_search_node ( const SrTreeNode *key )
 {
   int cmp;
   _cur = _root;

   if ( _cur==NIL ) return 1;

   while ( true )
    {  
      cmp = _man->compare(key,_cur);
      if ( cmp>0 ) 
       { if ( _cur->right!=NIL ) _cur=_cur->right; else return cmp;
       }
      else if ( cmp<0 )
       { if ( _cur->left!=NIL ) _cur=_cur->left; else return cmp;
       }
      else return cmp;
   }
 }

/*! Method for right rotation of the tree about a given node. */
void SrTreeBase::_rotate_right ( SrTreeNode *x )
 {
   SR_TRACE1("Rotate Right");

   SrTreeNode *y = x->left;

   x->left = y->right;
   if ( y->right!=NIL ) y->right->parent=x;
   y->parent = x->parent;

   if ( x->parent!=NIL )
    { if ( x==x->parent->right ) x->parent->right=y;
       else x->parent->left=y;
    }
   else _root = y;

   y->right = x;
   x->parent = y;
 }

/*! Method for left rotation of the tree about a given node. */
void SrTreeBase::_rotate_left ( SrTreeNode *x )
 {
   SR_TRACE1("Rotate Left");

   SrTreeNode *y = x->right;

   x->right = y->left;
   if ( y->left!=NIL ) y->left->parent=x;
   y->parent = x->parent;

   if ( x->parent!=NIL )
    { if ( x==x->parent->left ) x->parent->left=y;
       else x->parent->right=y;
    }
   else _root = y;

   y->left = x;
   x->parent = y;
 }

/*! Rebalance the tree after insertion of a node. */
void SrTreeBase::_rebalance ( SrTreeNode *x )
 {
   SR_TRACE1("Rebalance");

   SrTreeNode *y;

   while ( x!=_root && RED(x->parent) )
    { // if ( !x->parent->parent ) REPORT_ERROR
      if ( x->parent==x->parent->parent->left )
       { y = x->parent->parent->right;
         if ( RED(y) )
          { // handle case 1 (see CLR book, pp. 269)
            x->parent->color = SrTreeNode::Black;
            y->color = SrTreeNode::Black;
            x->parent->parent->color = SrTreeNode::Red;
            x = x->parent->parent;
          }
         else
          { if ( x==x->parent->right )
             { // transform case 2 into case 3 (see CLR book, pp. 269)
               x = x->parent;
               _rotate_left ( x );
             }
            // handle case 3 (see CLR book, pp. 269)
            x->parent->color = SrTreeNode::Black;
            x->parent->parent->color = SrTreeNode::Red;
            _rotate_right ( x->parent->parent );
          }
       }
      else
       { y = x->parent->parent->left;
         if ( RED(y) )
          { // handle case 1 (see CLR book, pp. 269)
            x->parent->color = SrTreeNode::Black;
            y->color = SrTreeNode::Black;
            x->parent->parent->color = SrTreeNode::Red;
            x = x->parent->parent;
          }
         else
          { if ( x==x->parent->left )
             { // transform case 2 into case 3 (see CLR book, pp. 269)
               x = x->parent;
               _rotate_right ( x );
             }
            // handle case 3 (see CLR book, pp. 269)
            x->parent->color = SrTreeNode::Black;
            x->parent->parent->color = SrTreeNode::Red;
            _rotate_left ( x->parent->parent );
          }
       }
    }
 }

/*! Method for restoring red-black properties after deletion. */
void SrTreeBase::_fix_remove ( SrTreeNode *x )
 {
   SR_TRACE1("Fix Remove");

   while ( x!=_root && BLACK(x) )
    {
      if ( x==x->parent->left )
       { SrTreeNode *w = x->parent->right;
         if ( RED(w) )
          { w->color = SrTreeNode::Black;
            x->parent->color = SrTreeNode::Red;
            _rotate_left ( x->parent );
            w = x->parent->right;
          }
         if ( BLACK(w->left) && BLACK(w->right) )
          { w->color = SrTreeNode::Red;
            x = x->parent;
          } 
         else
          { if ( BLACK(w->right) )
             { w->left->color = SrTreeNode::Black;
               w->color = SrTreeNode::Red;
               _rotate_right ( w );
               w = x->parent->right;
             }
            w->color = x->parent->color;
            x->parent->color = SrTreeNode::Black;
            w->right->color = SrTreeNode::Black;
            _rotate_left ( x->parent );
            x = _root;
          }
       }
      else
       { SrTreeNode *w = x->parent->left;
         if ( RED(w) )
          { w->color = SrTreeNode::Black;
            x->parent->color = SrTreeNode::Red;
            _rotate_right ( x->parent );
            w = x->parent->left;
          }
         if ( BLACK(w->left) && BLACK(w->right) )
          { w->color = SrTreeNode::Red;
            x = x->parent;
          }
         else
          { if ( BLACK(w->left) )
             { w->right->color = SrTreeNode::Black;
               w->color = SrTreeNode::Red;
               _rotate_left ( w );
               w = x->parent->left;
             }
            w->color = x->parent->color;
            x->parent->color = SrTreeNode::Black;
            w->left->color = SrTreeNode::Black;
            _rotate_right ( x->parent );
            x = _root;
          }
       }
    }

   x->color = SrTreeNode::Black;
 }

//----------------------------- constructors ------------------------------

SrTreeBase::SrTreeBase ( SrClassManagerBase* m )
 {
   _root = _cur = NIL;
   _elements = 0;
   _man = m;
   _man->ref();
 }

SrTreeBase::SrTreeBase ( const SrTreeBase& t )
 {
   _root = _cur = NIL;
   _elements = 0;
   _man = t._man;
   _man->ref();
   insert_tree ( t );
 }

SrTreeBase::~SrTreeBase ()
 {
   init ();
   _man->unref();
 }

void SrTreeBase::init ()
 {
   _cur = _root;
   SrTreeNode *curp;

   while ( _cur!=NIL )
    { // 1. descend _cur to a leaf
      while ( _cur->left!=NIL || _cur->right!=NIL ) // while cur is not a leaf 
        _cur = _cur->left!=NIL? _cur->left:_cur->right; 

      // 2. unlink _cur
      curp = _cur->parent;
      if ( _cur!=_root ) 
       { if ( curp->left==_cur ) curp->left=NIL;
          else curp->right=NIL;
       }
      
      // 3. delete and update _cur
      _man->free ( _cur );
      _cur = curp;
    }
    
   _elements = 0;
   _root = _cur = SrTreeNode::null;
 }

SrTreeNode *SrTreeBase::get_min ( SrTreeNode *x ) const
 {
   if ( x==NIL ) return x;
   while ( x->left!=NIL ) x=x->left;
   return x;
 }

SrTreeNode *SrTreeBase::get_max ( SrTreeNode *x ) const
 {
   if ( x==NIL ) return x;
   while ( x->right!=NIL ) x=x->right;
   return x;
 }

SrTreeNode *SrTreeBase::get_next ( SrTreeNode *x ) const
 {
   if ( x->right!=NIL ) return get_min ( x->right );

   SrTreeNode *y = x->parent;
   while ( y!=NIL && x==y->right )
    { x = y;
      y = y->parent;
    }

   return y;
 }

SrTreeNode *SrTreeBase::get_prior ( SrTreeNode *x ) const
 {
   if ( x->left!=NIL ) return get_max ( x->left );

   SrTreeNode *y = x->parent;
   while ( y!=NIL && x==y->left )
    { x = y;
      y = y->parent;
    }

   return y;
 }

SrTreeNode *SrTreeBase::search ( const SrTreeNode *key )
 {
   if ( _root==NIL ) return 0;
   return _search_node(key)==0? _cur:0;
 }

SrTreeNode *SrTreeBase::insert ( SrTreeNode *key )
 {
   int cmp = _search_node ( key );

   if ( _cur!=NIL )
    { if ( cmp>0 ) // key>_cur
       { // if (_cur->right) REPORT_ERROR;
         _cur->right = key;
         key->parent = _cur;
         _rebalance ( key );
         _root->color = SrTreeNode::Black;
         _elements++;
         return key;
       }
      else if ( cmp<0 ) // key<_cur
       { // if (_cur->left) REPORT_ERROR
         _cur->left = key;
         key->parent = _cur;
         _rebalance ( key );
         _root->color = SrTreeNode::Black;
         _elements++;
         return key;
       }
      else return 0; // not inserted, already in the tree
    }
   else // tree empty
    { _root = key;
      _root->init ();
      _root->color = SrTreeNode::Black;
      _elements++;
      return key;
    }
 }

SrTreeNode *SrTreeBase::insert_or_del ( SrTreeNode *key )
 {
   if ( !insert(key) )
    { _man->free ( key );
      return 0;
    }
   return key;
 }

void SrTreeBase::insert_tree ( const SrTreeBase& t )
 { 
   if ( this==&t ) return;

   SrTreeIteratorBase it(t); 
   for ( it.first(); it.inrange(); it.next() )
    { insert_or_del ( (SrTreeNode*)_man->alloc(it.get()) );
    }
 }

SrTreeNode *SrTreeBase::extract ( SrTreeNode *z )
 {
   SrTreeNode *x, *y;

   y = ( z->left==NIL || z->right==NIL )? z : get_next(z);

   x = ( y->left!=NIL )? y->left : y->right;

   x->parent = y->parent; 
   
   if ( y->parent!=NIL )
    { if ( y==y->parent->left ) y->parent->left=x; 
       else y->parent->right=x;
    }
   else _root = x;

   SrTreeNode::Color ycolor = y->color;

   if ( y!=z ) // make y be z 
    { y->left=z->left; y->right=z->right; y->parent=z->parent; y->color=z->color;
      if ( z->left ) z->left->parent=y;
      if ( z->right ) z->right->parent=y;
      if ( z->parent )
       { if ( z->parent->left==z ) z->parent->left=y; 
          else z->parent->right=y;
       }
      if ( _root==z ) _root=y;
    }

   if ( ycolor==SrTreeNode::Black ) _fix_remove ( x );

   _elements--;
   _cur = z;
   z->init();
   return z;
 }

void SrTreeBase::remove ( SrTreeNode* z )
 {
   extract ( z );
   _man->free ( z );
 }

SrTreeNode* SrTreeBase::search_and_extract ( const SrTreeNode* key )
 {
   int cmp = _search_node ( key );
   if ( cmp!=0 ) return 0; // not found
   return extract ( _cur );
 }

bool SrTreeBase::search_and_remove ( const SrTreeNode* key )
 {
   int cmp = _search_node ( key );
   if ( cmp==0 ) { remove ( _cur ); return true; }
   return false;
 }

void SrTreeBase::take_data ( SrTreeBase& t )
 {
   _root     = t._root;      t._root      = 0;
   _cur      = t._cur;       t._cur       = 0;
   _elements = t._elements;  t._elements  = 0;
 }

void SrTreeBase::operator= ( const SrTreeBase& t )
 {
   init ();
   insert_tree ( t ); // need to write a copy routine instead of inserting all nodes...
 }

SrOutput& operator<< ( SrOutput& o, const SrTreeBase& t )
 { 
   o<<'[';
   SrTreeIteratorBase it(t); 
   for ( it.first(); it.inrange(); it.next() )
    { t._man->output ( o, it.cur() );
      if ( !it.inlast() ) o << ' ';
    }
   return o<<']';
 }

//=========================== SrTreeIteratorBase ================================

SrTreeIteratorBase::SrTreeIteratorBase ( const SrTreeBase& t ) : _tree(t)
 { 
   reset ();
 }

void SrTreeIteratorBase::reset ()
 {
   _first = _cur = _tree.first();
   _last = _tree.last();
 }

//============================ End of File =================================
