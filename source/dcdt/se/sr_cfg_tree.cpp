#include "precompiled.h"
#include "0ad_warning_disable.h"
# include "sr_cfg_tree.h"

//# define SR_USE_TRACE1    // expand node
# include "sr_trace.h"

//=============================== SrCfgNode ========================================

float SrCfgNode::prec ( int i ) const
 {
   int lev = level ( i );
   return dist(i)/(float)sr_pow(2,SR_ABS(lev));
 }

void SrCfgNode::get_subtree ( SrArray<SrCfgNode*>& nodes )
 {
   int i;
   nodes.push() = this;
   for ( i=0; i<_children.size(); i++ ) _children[i].node->get_subtree ( nodes );
 }

void SrCfgNode::_deledge ( int e )
 {
   _children.remove ( e );

   int i, size = _children.size();
   for ( i=e; i<size; i++ )
      _children[i].node->_parentlink = i;
 }

void SrCfgNode::_fixplinks ()
 {
   int i;
   for ( i=0; i<_children.size(); i++ )
     _children[i].node->_parentlink = i;
   for ( i=0; i<_children.size(); i++ )
     _children[i].node->_fixplinks();
 }

void SrCfgNode::_reroot ()
 {
   if ( !_parent ) return;

   // new parent will have its old parent as new child:
   SrCfgNode* curnode = this;
   SrCfgNode* curparent = _parent;
   int newparentlink = _children.size();
   int oldparentlink = _parentlink;
   _children.push() = _parent->_children[oldparentlink];
   _children.top().node = _parent;
   _parent = 0;
   _parentlink = -1;
   
   // walk towards the old root, swaping old/new parents:
   int tmp;
   SrCfgNode* newparent = this;
   curnode = curparent;
   curparent = curnode->_parent; // move to parent
   while ( curparent )
    { // modify curnode:
      curnode->_children[oldparentlink] = curparent->_children[curnode->_parentlink];
      curnode->_children[oldparentlink].node = curparent; // parent becomes child
      tmp = oldparentlink;
      oldparentlink = curnode->_parentlink;
      curnode->_parentlink = newparentlink;
      newparentlink = tmp;
      curnode->_parent = newparent;
      // move to parent:
      newparent = curnode;
      curnode = curparent;
      curparent = curparent->_parent;
    }

   // delete extra child of the old root:        
   curnode->_deledge ( oldparentlink );
   curnode->_parentlink = newparentlink;
   curnode->_parent = newparent;
 }

//============================= SrCfgTreeBase ======================================

SrCfgTreeBase::SrCfgTreeBase ( SrCfgManagerBase* cman )
 {
   _cman = cman;
   _cman->ref ();
   _root = 0;
 }

SrCfgTreeBase::~SrCfgTreeBase ()
 {
   int i;
   for ( i=0; i<_buffer.size(); i++ )
    { _cman->free ( _buffer[i]->_cfg );
      delete _buffer[i];
    }
   _cman->unref();
 }

bool SrCfgTreeBase::check_all ( SrOutput& o )
 {
   int i, j;
   o << "Starting check:\n";

   if ( !_root ) { o<<"Check ok, but empty.\n"; return true; }
   
   o << "Buffer size...\n";
   _nodes.size ( 0 );
   _root->get_subtree ( _nodes );
   if ( _nodes.size()!=_buffer.size()-_freepos.size() ) goto error;

   o << "Buffer indices...\n";
   for ( i=0; i<_buffer.size(); i++ )
    if ( _buffer[i]->_bufferid!=i ) goto error;
    
   o << "Parent-child pointers...\n";
   for ( i=0; i<_nodes.size(); i++ )
    { if ( !_nodes[i]->_parent )
       { if ( _nodes[i]!=_root ) { o<<"wrong root "; goto error; } }
      else
       { if ( _nodes[i]->_parent->child(_nodes[i]->_parentlink)!=_nodes[i] )
           { o<< "wrong parent link n:" << 
                _nodes[i]->id()<<" p:"<<_nodes[i]->_parent->id()<<srspc; goto error; }
       }
    }

   o << "Children pointers...\n";
   for ( i=0; i<_nodes.size(); i++ )
    { for ( j=0; j<_nodes[i]->children(); j++ )
       { if ( _nodes[i]->_children[j].node->_parent!=_nodes[i] )
           { o<<"wrong child->_parent pointer "; goto error; }
         if ( _nodes[i]->_children[j].node->_parentlink!=j )
           { o<<"wrong child->_parentlink index "<<
                _nodes[i]->id()<<"/"<<j<<srspc; goto error; }
       }
    }

   o << "Check ok.\n";
   return true;
   
   error:
   o << "error!\n";
   return false;
 }

void SrCfgTreeBase::init ()
 {
   _root = 0;
   _freepos.size ( _buffer.size() );
   int i, max = _buffer.size()-1;
   for ( i=0; i<=max; i++ ) _freepos[i] = max-i;
 }

void SrCfgTreeBase::init ( const srcfg* cfg )
 {
   init ();
   add_node ( 0, cfg, 0 );
 }

SrCfgNode* SrCfgTreeBase::_newnode ()
 { 
   if ( _freepos.size()>0 )
    { return _buffer[_freepos.pop()];
    }
   else
    { int id = _buffer.size();
      _buffer.push() = new SrCfgNode;
      _buffer[id]->_cfg = _cman->alloc();
      _buffer[id]->_bufferid = id;
      return _buffer[id];
    }
 }

void SrCfgTreeBase::_delnode ( SrCfgNode* n )
 { 
   _freepos.push() = n->_bufferid;
 }

SrCfgNode* SrCfgTreeBase::add_node ( SrCfgNode* parent, const srcfg* cfg, float dist )
 {
   SrCfgNode* newn = _newnode ();

   if ( parent ) // set parent data
    { newn->_parentlink = parent->children();
      SrCfgNode::Link& l = parent->_children.push();
      if ( dist<0 ) dist = _cman->dist ( parent->_cfg, cfg );
      l.node = newn;
      l.dist = dist;
      l.level = 0;
    }
   else // this is the root node
    { _root = newn;
      newn->_parentlink = -1;
    }

   _cman->copy ( newn->_cfg, cfg );
   newn->_parent = parent;
   newn->_children.size(0);
 
   return newn;
 }

void SrCfgTreeBase::_nearest ( SrCfgNode* n, const srcfg* c, SrCfgNode*& nearest, float& mindist )
 {
   // check distance:
   float dist = _cman->dist ( n->cfg(), c );
   if ( dist<mindist ) 
    { nearest = n;
      mindist = dist;
    }

   // recurse:
   int i, chsize=n->children();
   for ( i=0; i<chsize; i++ )
    _nearest ( n->child(i), c, nearest, mindist );
 }

SrCfgNode* SrCfgTreeBase::search_nearest ( const srcfg* c, float* d )
 {
   if ( !_root ) return _root;
   
   SrCfgNode* nearest;
   float mindist = 1E+30f; // float range in visualc is: 3.4E +/- 38 

   _nearest ( _root, c, nearest, mindist );

   if (d) *d = mindist;
   return nearest;
 }

SrCfgNode* SrCfgTreeBase::expand_node ( SrCfgNode* source, const srcfg* direction,
                                        float step, int maxtries, float dist )
 {
   srcfg* csource = source->cfg();
   if ( dist<0 ) dist = _cman->dist ( csource, direction );
   if ( dist<0.00001f ) return 0; // too close

   SrCfgNode* nnew = _newnode();
   srcfg* cnew = nnew->cfg();
   float t = step/dist;
   if ( t>1.0f ) t = 1.0f;
 
   while ( maxtries-->0 )
    { SR_TRACE1 ( "INS: trying to insert...");

      _cman->interp ( csource, direction, t, cnew );
      t /= 2;
      
      if ( _cman->valid(cnew) )
      { SR_TRACE1 ( "INS: inserting 1 node.");
         _delnode ( nnew );
         return add_node ( source, cnew, -1 ); // instead of -1, could use aprox distance dist*t...
       }
    }

   SR_TRACE1 ( "INS: no nodes inserted.");
   _delnode ( nnew );
   return 0;
 }

SrCfgNode* SrCfgTreeBase::expand_node_safe ( SrCfgNode* source, const srcfg* direction,
                                             float step, int maxtries, float prec, float dist )
 {
   srcfg* csource = source->cfg();
   if ( dist<0 ) dist = _cman->dist ( csource, direction );
   if ( dist<0.00001f ) return 0; // too close

   SrCfgNode* nnew = _newnode();
   srcfg* cnew = nnew->cfg();
   SrCfgNode* ntmp = _newnode();
   srcfg* ctmp = ntmp->cfg();

   float t = step/dist;
   if ( t>1.0f ) t = 1.0f;

   if ( 0 ) // test "long" expansion
   { float dt = t;
   maxtries=5;
     while ( t<=1 && maxtries-->0 )
     { SR_TRACE1 ( "INS: trying to insert...");
       _cman->interp ( csource, direction, t, cnew );
       t += dt;
      
      if ( _cman->valid(cnew) )
      if ( _cman->visible(source->cfg(),cnew,ctmp,prec) )
       { SR_TRACE1 ( "INS: inserting 1 node.");
         _delnode ( nnew );
         _delnode ( ntmp );
         nnew = add_node ( source, cnew, -1 ); // instead of -1, could use aprox distance dist*t...
         source->_children.top().level = -1;   // mark as safe (can be any <0 number)
         _cman->child_added ( source->cfg(), cnew ); // notify configuration manager
         SR_TRACE1 ( "INS: Ok.");
         source = nnew;
         if ( t>1 ) return nnew; // end

         nnew = _newnode();
         cnew = nnew->cfg();
         ntmp = _newnode();
         ctmp = ntmp->cfg();
        
       }
     }
    }
   else
   while ( maxtries-->0 )
    { SR_TRACE1 ( "INS: trying to insert...");

      _cman->interp ( csource, direction, t, cnew );
      t /= 2;
      
      if ( _cman->valid(cnew) )
      if ( _cman->visible(source->cfg(),cnew,ctmp,prec) )
       { SR_TRACE1 ( "INS: inserting 1 node.");
         _delnode ( nnew );
         _delnode ( ntmp );
         nnew = add_node ( source, cnew, -1 ); // instead of -1, could use aprox distance dist*t...
         source->_children.top().level = -1;   // mark as safe (can be any <0 number)
         _cman->child_added ( source->cfg(), cnew ); // notify configuration manager
         SR_TRACE1 ( "INS: Ok.");
         return nnew;
         //following test not ok:
         //return expand_node_safe ( nnew, direction, step, maxtries, prec, -1 );
       }
    }

   SR_TRACE1 ( "INS: no nodes inserted.");
   _delnode ( nnew );
   _delnode ( ntmp );
   return 0;
 }

void SrCfgTreeBase::get_branch ( SrCfgNode* n, SrArray<SrCfgNode*>& nodes )
 {
   while ( n )
    { nodes.push() = n;
      n = n->parent();
    }
 }

void SrCfgTreeBase::get_branch ( SrCfgNode* n, SrCfgPathBase& path )
 {
   while ( n )
    { path.push ( n->cfg() );
      n = n->parent();
    }
 }
 
void SrCfgTreeBase::get_nodes ( SrArray<SrCfgNode*>& nodes )
 {
   nodes.size(0);
   if ( !_root ) return;
   _root->get_subtree ( nodes );
 }

// level k : (2^k)+1 tot tests, 2^(k-1) new tests, 2^k segments
// safe edge : dist/segments < collision_precision
bool SrCfgTreeBase::increment_edge_level ( SrCfgNode* n1, SrCfgNode* n2, float prec )
 {
   SrCfgNode::Link& l = n1->_children[n2->parentlink()];
   SrCfgNode* tmpnode = _newnode();
   srcfg* ct = tmpnode->cfg();
   srcfg* ct0 = n1->cfg();
   srcfg* ct1 = n2->cfg();

   // we will test the next level k:
   int k = 1 + l.level;
   //sr_out<<"K: "<<k<<srnl;

   // test the 2^(k-1) new tests of the new level k:
   float segs = (float) sr_pow ( 2, k );
   //float dseg = (l.dist/segs<=prec)? prec : 1.0f/segs; // test if last pass
   float dseg = 1.0f/segs;
   float dt = dseg*2.0f;
   float t;
   for ( t=dseg; t<1.0f; t+=dt )
    { //sr_out<<"t: "<<t<<srnl;
      _cman->interp ( ct0, ct1, t, ct );
      if ( !_cman->valid(ct) ) { _delnode(tmpnode); return false; }
    }

   // ok, promote edge to level k and check if it can be marked as safe:
   l.level = k;
   //sr_out<<"dist:"<<l.dist<<" segs:"<<segs<<" prec:"<<prec<<srnl;
   if ( l.dist/segs<=prec ) l.level=-l.level; // mark as safe

   _delnode(tmpnode);
   return true; 
 }

void SrCfgTreeBase::transfer_subtree ( SrCfgNode* n1, SrCfgNode* n2, SrCfgNode* joint1,
                                       SrCfgTreeBase& tree2, SrCfgNode* joint2 )
 {
   // delete edge [n1,n2]:
   n1->_deledge ( n2->parentlink() );
   n2->_parent=0; // n2 is now the root of a disconnected subtree, still using _buffer
   n2->_parentlink=-1;

   // save the nodes of subtree n2:
   _nodes.size ( 0 );
   n2->get_subtree ( _nodes );

   // reorder subtree to make joint1 the new root:
   joint1->_reroot();

   // attach joint1 children as children of joint2:
   while ( joint1->_children.size() )
    { SrCfgNode::Link& l = joint1->_children.pop();
      l.node->_parent = joint2;
      l.node->_parentlink = joint2->_children.size();
      joint2->_children.push() = l;
    }

   // finally reorganize buffers; all in _nodes change buffer, except joint1 deleted:
   int tmpi;
   SrCfgNode *n, *newn, *tmpn;
   while ( _nodes.size() )
    { n = _nodes.pop();
      _delnode (n);
      if ( n!=joint1 ) // "swap" with a new entry in tree2.buffer
       { newn = tree2._newnode();
         SR_SWAPT(_buffer[n->_bufferid],tree2._buffer[newn->_bufferid],tmpn);
         SR_SWAPT(n->_bufferid,newn->_bufferid,tmpi);
       }
    }
 }

void SrCfgTreeBase::output ( SrOutput& o, bool printcfg, SrCfgNode* n )
 {
   int i, j;
   
   _nodes.size(0);
   if ( !n ) n = _root;
   n->get_subtree ( _nodes );

   o << "\nSrCfgTree " << _nodes.size() << srnl;
   
   for ( i=0; i<_nodes.size(); i++ )
    {
      n = _nodes[i];
      //t._cman->output ( o, t.cfg(i) );
      o << "Node:" << n->id();
      if ( n->parent() ) o << " parent:"<<n->parent()->id();
       else o<<" root    ";
      
      o << "  Children:";
      for ( j=0; j<n->children(); j++ )
       { o << srspc << n->child(j)->id()
           << ":" << j 
           << "/" << n->child(j)->parentlink();
       }
       
      if ( printcfg )
       { o << "\nData: ";
         _cman->output(o,n->cfg());
         o <<"\n";
       }
      o << srnl;
    }
 }

//================================ friends =================================================


/*
    void _inpnode ( SrInput& i, SrCfgNode* n );
void SrCfgTreeBase::_inpnode ( SrInput& in, SrCfgNode* n )
 {
   int i, size;
   _cman->input ( in, n->_cfg );
   
   in.get_token(); // "p"
   in >> n->_parent;
   in.get_token(); // "c"
   in >> size;
   in.get_token(); // ":"

   n->_children.size(size);
   for ( i=0; i<size; i++ )
    { in >> n->_children[i].dist;
      in >> n->_children[i].node;
      in >> n->_children[i].level;
    }
 }*/

    /*! read the roadmap */
    /*friend SrInput& operator>> ( SrInput& in, SrCfgTreeBase& t );
SrInput& operator>> ( SrInput& in, SrCfgTreeBase& t )
 {
   int i, size;
   
   t.init();
   
   in.get_token();
   if ( in.last_token()!="SrCfgTree" ) return in;
   
   in >> size;
   
   for ( i=0; i<size; i++ )
    {
      // make sure next entry is valid
      if ( i==t._nodes.size() ) t._pushnode();
      
      // read node
      t._cman->input ( in, t.cfg(i) );
      t._inpnode ( in, t.node(i) );
    }

   t._nsize = size;
   
   return in;
 }
*/
//================================ End of File =================================================



/*
 This method takes O(k*n) time. It looks into all nodes of the graph,
        updating the sorted list with the k closest nodes, which is returned.
SrArray<SrCfgTreeBase::NodeDist>& SrCfgTreeBase::search_k_nearests ( const srcfg* c, int k )
 {
   _snodes.ensure_capacity(k);
   _snodes.size(0);
   SrRoadmapNode* n = _graph.first_node();
   if ( !n ) return _snodes;

   NodeDist nd;
   int i, worse=-1;

   SrListIterator<SrRoadmapNode> it(n);
   for ( it.first(); it.inrange(); it.next() )
    { if ( it.get()->blocked() ) continue;
      nd.n = it.get();
      nd.d = _cman->get_distance ( it.get()->c, c );

      if ( _snodes.size()<k )
       { if ( worse<0 ) 
          worse=_snodes.size();
         else
          if ( nd.d < _snodes[worse].d ) worse=_snodes.size();
         _snodes.push() = nd;
       }
      else if ( nd.d < _snodes[worse].d )
       { _snodes[worse] = nd;
         worse=0;
         for ( i=1; i<k; i++ )
          if ( _snodes[i].d > _snodes[worse].d ) worse=i;
       }
    }

   if ( _snodes.size()>1 ) _snodes.sort(nd_compare);

   return _snodes;
 }
 
 
 
 */

