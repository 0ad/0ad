#include "precompiled.h"
# include <stdlib.h>
# include "sr_graph.h"
# include "sr_heap.h"

//============================== SrGraphLink ===============================================




//============================== SrGraphNode ===============================================

SrGraphNode::~SrGraphNode ()
 {
   SrClassManagerBase* lman = _graph->link_class_manager();
   if ( _links.size()>0 )
    lman->free ( _links.pop() );
 }

SrGraphLink* SrGraphNode::linkto ( SrGraphNode* n, float cost )
 {
   SrGraphLink* l = (SrGraphLink*) _graph->link_class_manager()->alloc();
   l->_node = n;
   l->_index = 0;
   l->_cost = cost;

   _links.push () = l;

   return l;
 }

void SrGraphNode::unlink ( int ni )
 {
   SrClassManagerBase* lman = _graph->link_class_manager();
   lman->free ( _links[ni] );
   _links[ni] = _links.pop();
 }

int SrGraphNode::search_link ( SrGraphNode* n ) const
 {
   int i;
   for ( i=0; i<_links.size(); i++ )
     if ( _links[i]->node()==n ) return i;
   return -1;
 }

void SrGraphNode::output ( SrOutput& o ) const
 {
   float cost;
   int i, max;
   max = _links.size()-1;
   if ( max<0 ) return;

   SrClassManagerBase* lman = _graph->link_class_manager();

   o<<'(';
   for ( i=0; i<=max; i++ )
    { // output blocked status
      o << _links[i]->_blocked << srspc;

      // output index to target node
      o << _links[i]->_node->_index << srspc;

      // output link cost
      cost = _links[i]->_cost;
      if ( cost==SR_TRUNC(cost) ) o << int(cost);
       else o << cost;

      // output user data
      o << srspc;
      lman->output(o,_links[i]);

      if ( i<max ) o<<srspc;
    }
   o<<')';
 }

//============================== SrGraphPathTree ===============================================

class SrGraphPathTree
 { public :
    struct Node { int parent; float cost; SrGraphNode* node; };
    struct Leaf { int l; int d; }; // the leaf index in nodes array, and its depth
    SrArray<Node> nodes;
    SrHeap<Leaf,float> leafs;
    SrGraphBase* graph;
    SrGraphNode* closest;
    int iclosest;
    float cdist;
    float (*distfunc) ( const SrGraphNode*, const SrGraphNode*, void* udata );
    void *udata;
    bool bidirectional_block;

   public :
    SrGraphPathTree ()
     { bidirectional_block = false;
     }

    void init ( SrGraphBase* g, SrGraphNode* n )
     { nodes.size(1);
       nodes[0].parent = -1;
       nodes[0].cost = 0;
       nodes[0].node = n;
       Leaf l;
       l.l = l.d = 0;
       leafs.insert ( l, 0 );
       graph = g;
       distfunc=0;
       udata = 0;
       closest = 0;
       iclosest = 0;
       cdist = 0;
     }

    bool has_leaf ()
     { return leafs.size()==0? false:true;
     }

    bool expand_lowest_cost_leaf ( SrGraphNode* goalnode )
     { int i;
       int n = leafs.top().l;
       int d = leafs.top().d;
       leafs.remove ();
       SrGraphNode* node = nodes[n].node;
       Leaf leaf;
       const SrArray<SrGraphLink*>& a = node->links();
       for ( i=0; i<a.size(); i++ )
        { //sr_out<<a.size()<<srnl;
          if ( graph->marked(a[i]) ||
               a[i]->blocked() ||
               a[i]->node()->blocked() ) continue;
          if ( bidirectional_block )
           { if ( a[i]->node()->link(node)->blocked() ) continue; }
          nodes.push();
          nodes.top().parent = n;
          nodes.top().cost = nodes[n].cost + a[i]->cost();
          nodes.top().node = a[i]->node();
          leaf.l = nodes.size()-1;
          leaf.d = d+1;
          leafs.insert ( leaf, nodes.top().cost );
          graph->mark ( a[i] );
          if ( distfunc )
           { float d = distfunc ( nodes.top().node, goalnode, udata );
             if ( !closest || d<cdist )
              { closest=nodes.top().node; iclosest=nodes.size()-1; cdist=d; }
           }
          if ( a[i]->node()==goalnode ) return true;
        }
       return false;
     }
 };

//============================== SrGraphBase ===============================================

# define MARKFREE 0
# define MARKING  1
# define INDEXING 2

SrGraphBase::SrGraphBase ( SrClassManagerBase* nm, SrClassManagerBase* lm )
            :_nodes(nm)
 {
   _curmark = 1;
   _mark_status = MARKFREE;
   _pt = 0; // allocated only if shortest path is called
   _lman = lm;
   _lman->ref(); // nm is managed by the list _nodes
   _leave_indices_after_save = 0;
 }

SrGraphBase::~SrGraphBase ()
 {
   _nodes.init (); // Important: this ensures that _lman is used before _lman->unref()
   delete _pt;
   _lman->unref();
 }

void SrGraphBase::init ()
 {
   _nodes.init();
   _curmark = 1;
   _mark_status = MARKFREE;
 }

void SrGraphBase::compress ()
 {
   if ( _nodes.empty() ) return;
   _nodes.gofirst();
   do { _nodes.cur()->compress();
      } while ( _nodes.notlast() );
 }

int SrGraphBase::num_links () const
 {
   if ( _nodes.empty() ) return 0;

   int n=0;

   SrGraphNode* first = _nodes.first();
   SrGraphNode* cur = first;
   do { n += cur->num_links();
        cur = cur->next();
      } while ( cur!=first );

   return n;
 }

//----------------------------------- marking --------------------------------

void SrGraphBase::begin_marking ()
 {
   if ( _mark_status!=MARKFREE ) sr_out.fatal_error("SrGraphBase::begin_mark() is locked!");
   _mark_status = MARKING;
   if ( _curmark==sruintmax ) _normalize_mark ();
    else _curmark++;
 }

void SrGraphBase::end_marking ()
 {
   _mark_status=MARKFREE;
 }

bool SrGraphBase::marked ( SrGraphNode* n ) 
 {
   if ( _mark_status!=MARKING ) sr_out.fatal_error ( "SrGraphBase::marked(n): marking is not active!\n" );
   return n->_index==_curmark? true:false;
 }

void SrGraphBase::mark ( SrGraphNode* n ) 
 { 
   if ( _mark_status!=MARKING ) sr_out.fatal_error ( "SrGraphBase::mark(n): marking is not active!\n" );
   n->_index = _curmark;
 }

void SrGraphBase::unmark ( SrGraphNode* n ) 
 { 
   if ( _mark_status!=MARKING ) sr_out.fatal_error ( "SrGraphBase::unmark(n): marking is not active!\n");
   n->_index = _curmark-1;
 }

bool SrGraphBase::marked ( SrGraphLink* l ) 
 {
   if ( _mark_status!=MARKING ) sr_out.fatal_error ( "SrGraphBase::marked(l): marking is not active!\n" );
   return l->_index==_curmark? true:false;
 }

void SrGraphBase::mark ( SrGraphLink* l ) 
 { 
   if ( _mark_status!=MARKING ) sr_out.fatal_error ( "SrGraphBase::mark(l): marking is not active!\n" );
   l->_index = _curmark;
 }

void SrGraphBase::unmark ( SrGraphLink* l ) 
 { 
   if ( _mark_status!=MARKING ) sr_out.fatal_error ( "SrGraphBase::unmark(l): marking is not active!\n");
   l->_index = _curmark-1;
 }

//----------------------------------- indexing --------------------------------

void SrGraphBase::begin_indexing ()
 {
   if ( _mark_status!=MARKFREE ) sr_out.fatal_error("SrGraphBase::begin_indexing() is locked!");
   _mark_status = INDEXING;
 }

void SrGraphBase::end_indexing ()
 {
   _normalize_mark ();
   _mark_status=MARKFREE;
 }

sruint SrGraphBase::index ( SrGraphNode* n )
 {
   if ( _mark_status!=INDEXING ) sr_out.fatal_error ("SrGraphBase::index(n): indexing is not active!");
   return n->_index;
 }

void SrGraphBase::index ( SrGraphNode* n, sruint i )
 {
   if ( _mark_status!=INDEXING ) sr_out.fatal_error ("SrGraphBase::index(n,i): indexing is not active!");
   n->_index = i;
 }

sruint SrGraphBase::index ( SrGraphLink* l )
 {
   if ( _mark_status!=INDEXING ) sr_out.fatal_error ("SrGraphBase::index(l): indexing is not active!");
   return l->_index;
 }

void SrGraphBase::index ( SrGraphLink* l, sruint i )
 {
   if ( _mark_status!=INDEXING ) sr_out.fatal_error ("SrGraphBase::index(l,i): indexing is not active!");
   l->_index = i;
 }

//----------------------------------- construction --------------------------------

SrGraphNode* SrGraphBase::insert ( SrGraphNode* n )
 {
   _nodes.insert_next ( n );
   n->_graph = this;
   return n;
 }

SrGraphNode* SrGraphBase::extract ( SrGraphNode* n )
 {
   _nodes.cur(n);
   return _nodes.extract();
 }

void SrGraphBase::remove_node ( SrGraphNode* n )
 {
   _nodes.cur(n);
   _nodes.remove();
 }

int SrGraphBase::remove_link ( SrGraphNode* n1, SrGraphNode* n2 )
 {
   int i;
   int n=0;

   while ( true )
    { i = n1->search_link(n2);
      if ( i<0 ) break;
      n++;
      n1->unlink(i);
    }

   while ( true )
    { i = n2->search_link(n1);
      if ( i<0 ) break;
      n++;
      n2->unlink(i);
    }

   return n;
 }

void SrGraphBase::link ( SrGraphNode* n1, SrGraphNode* n2, float c )
 {
   n1->linkto(n2,c);
   n2->linkto(n1,c);
 }

//----------------------------------- get edges ----------------------------------

void SrGraphBase::get_directed_edges ( SrArray<SrGraphNode*>& edges )
 {
   edges.size ( num_nodes() );
   edges.size ( 0 );

   int i;
   SrGraphNode* n;
   SrListIterator<SrGraphNode> it(_nodes);

   for ( it.first(); it.inrange(); it.next() )
    { n = it.get();
      for ( i=0; i<n->num_links(); i++ )
       { edges.push() = n;
         edges.push() = n->link(i)->node();
       }
    }
 }

void SrGraphBase::get_undirected_edges ( SrArray<SrGraphNode*>& edges )
 {
   edges.size ( num_nodes() );
   edges.size ( 0 );

   int i, li;
   SrGraphNode* n;
   SrListIterator<SrGraphNode> it(_nodes);

   begin_marking();

   for ( it.first(); it.inrange(); it.next() )
    { n = it.get();
      for ( i=0; i<n->links().size(); i++ )
       { if ( !marked(n->link(i)) )
          { edges.push() = n;
            edges.push() = n->link(i)->node();
            mark ( n->link(i) );
            li = edges.top()->search_link(n);
            if ( li>=0 ) mark ( edges.top()->link(li) );
          }
       }
    }

   end_marking();
 }

//----------------------------------- components ----------------------------------

static void _traverse ( SrGraphBase* graph,
                        SrArray<SrGraphNode*>& stack,
                        SrArray<SrGraphNode*>& nodes )
 {
   int i;
   SrGraphNode *n, *ln;
   while ( stack.size()>0 )
    { n = stack.pop();
      graph->mark ( n );
      nodes.push() = n;
      for ( i=0; i<n->num_links(); i++ )
       { ln = n->link(i)->node();
         if ( !graph->marked(ln) ) stack.push()=ln;
       }
    }
 }

void SrGraphBase::get_connected_nodes ( SrGraphNode* source, SrArray<SrGraphNode*>& nodes )
 {
   nodes.size ( num_nodes() );
   nodes.size ( 0 );

   SrArray<SrGraphNode*>& stack = _buffer;
   stack.size ( 0 );
   
   begin_marking();
   stack.push() = source;
   _traverse ( this, stack, nodes );
   end_marking();
 }

void SrGraphBase::get_disconnected_components ( SrArray<int>& components, SrArray<SrGraphNode*>& nodes )
 {
   nodes.size ( num_nodes() );
   nodes.size ( 0 );

   components.size ( 0 );

   SrArray<SrGraphNode*>& stack = _buffer;
   stack.size ( 0 );
   
   SrGraphNode* n;
   SrListIterator<SrGraphNode> it(_nodes);

   begin_marking();

   for ( it.first(); it.inrange(); it.next() )
    { n = it.get();
      if ( !marked(n) )
       { components.push() = nodes.size();
         stack.push() = n;
         _traverse ( this, stack, nodes );
         components.push() = nodes.size()-1;
       }
    }

   end_marking();
 }

//----------------------------------- shortest path ----------------------------------

float SrGraphBase::get_shortest_path ( SrGraphNode* n1,
                                       SrGraphNode* n2,
                                       SrArray<SrGraphNode*>& path,
                                       float (*distfunc) ( const SrGraphNode*, const SrGraphNode*, void* udata ),
                                       void* udata )
 {
   path.size(0);
   if ( n1==n2 ) { path.push()=n1; return 0; }

   begin_marking ();

   if ( !_pt ) _pt = new SrGraphPathTree;
   _pt->init ( this, n2 );

   if ( distfunc )
    { _pt->distfunc = distfunc;
      _pt->udata = udata;
    }

   int i;
   bool end = false;

   while ( !end )
    { if ( !_pt->has_leaf() )
       { end_marking();
         if ( !distfunc ) return 0;
         i = _pt->iclosest;
         float cost = _pt->nodes[i].cost;   
         while ( i>=0 )
          { path.push () = _pt->nodes[i].node;
            i = _pt->nodes[i].parent;
          }
         return cost;
       }

      end = _pt->expand_lowest_cost_leaf ( n1 );
    }

   end_marking ();

   i = _pt->nodes.size()-1; // last element is n1
   float cost = _pt->nodes[i].cost;   
   while ( i>=0 )
    { path.push () = _pt->nodes[i].node;
      i = _pt->nodes[i].parent;
    }

   return cost;
 }

bool SrGraphBase::local_search 
                  ( SrGraphNode* startn,
                    SrGraphNode* endn,
                    int    maxdepth,
                    float  maxdist,
                    int&   depth,
                    float& dist )
 {
   depth=0;
   dist=0;

   if ( startn==endn ) return true;

   begin_marking ();

   if ( !_pt ) _pt = new SrGraphPathTree;
   _pt->init ( this, startn );

   int i;
   bool not_found = false;
   bool end = false;

   while ( !end )
    { if ( !_pt->has_leaf() ) { not_found=true; break; } // not found!

      dist = _pt->nodes[_pt->leafs.top().l].cost;
      depth = _pt->leafs.top().d;

      if ( maxdepth>0 && depth>maxdepth ) { i=-1; break; } // max depth reached
      if ( maxdist>0 && dist>maxdist ) { i=-1; break; }    // max dist reached

      end = _pt->expand_lowest_cost_leaf ( endn );
    }

   end_marking ();

   if ( not_found ) return false; // not found!
   return true;
 }

void SrGraphBase::bidirectional_block_test ( bool b )
 {
   if ( !_pt ) _pt = new SrGraphPathTree;
   _pt->bidirectional_block = b;
 }

//------------------------------------- I/O --------------------------------

void SrGraphBase::output ( SrOutput& o )
 {
   SrListIterator<SrGraphNode> it(_nodes);
   SrClassManagerBase* nman = node_class_manager();

   // set indices
   if ( _mark_status!=MARKFREE ) sr_out.fatal_error("SrGraphBase::operator<<(): begin_indexing() is locked!");
   sruint i=0;
   begin_indexing();
   for ( it.first(); it.inrange(); it.next() ) index(it.get(),i++);

   // print
   o<<'[';
   for ( it.first(); it.inrange(); it.next() )
    { o << it->index() << srspc;      // output node index
      o << it->_blocked << srspc;     // output node blocked status
      nman->output ( o, it.get() );   // output user data
      if ( it.get()->num_links()>0 )  // output node links (blocked, ids, cost, and udata)
       { o<<srspc;
         it->SrGraphNode::output ( o );
       }
      if ( !it.inlast() ) o << srnl;
    }
   o<<']';

   if ( !_leave_indices_after_save ) end_indexing();
   _leave_indices_after_save = 0;
 }

static void set_blocked ( int& blocked, const char* last_token )
 {
   if ( last_token[0]=='b' ) // back-compatibility
    blocked = 1;
   else if ( last_token[0]=='f' ) // back-compatibility
    blocked = 0;
   else // should be an integer
    blocked = atoi(last_token);
 }

void SrGraphBase::input ( SrInput& inp )
 {
   SrArray<SrGraphNode*>& nodes = _buffer;
   nodes.size(128);
   nodes.size(0);

   SrClassManagerBase* nman = node_class_manager();
   SrClassManagerBase* lman = link_class_manager();

   init ();

   inp.getd(); // [
   inp.get_token(); // get node counter (which is not needed)

   while ( inp.last_token()[0]!=']' )
    { 
      nodes.push() = _nodes.insert_next(); // allocate one node
      nodes.top()->_graph = this;

      inp.get_token(); // get node blocked status
      set_blocked ( nodes.top()->_blocked, inp.last_token() );

      nman->input ( inp, nodes.top() ); // read node user data
      
      inp.get_token(); // new node counter, or '(', or ']'

      if ( inp.last_token()[0]=='(')
       while ( true )
       { inp.get_token();
         if ( inp.last_token()[0]==')' ) { inp.get_token(); break; }
         SrArray<SrGraphLink*>& la = nodes.top()->_links;
         la.push() = (SrGraphLink*) lman->alloc();
         set_blocked ( la.top()->_blocked, inp.last_token() ); // get link blocked status

         inp.get_token(); // get id
         la.top()->_index = atoi(inp.last_token()); // store id
         inp.getn(); // get cost
         la.top()->_cost = (float) atof(inp.last_token()); // store cost
         lman->input ( inp, la.top() );
       } 
    }   

   // now convert indices to pointers:
   int i, j;
   for ( i=0; i<nodes.size(); i++ )
    { SrArray<SrGraphLink*>& la = nodes[i]->_links;
      for ( j=0; j<la.size(); j++ )
       { la[j]->_node = nodes[ int(la[j]->_index) ];
       }
    }
 }

//---------------------------- private methods --------------------------------

// set all indices (nodes and links) to 0 and curmark to 1
void SrGraphBase::_normalize_mark()
 {
   int i;
   SrGraphNode* n;

   if ( _nodes.empty() ) return;

   _nodes.gofirst();
   do { n = _nodes.cur();
        n->_index = 0;
        for ( i=0; i<n->num_links(); i++ ) n->link(i)->_index=0;
      } while ( _nodes.notlast() );
   _curmark = 1;
 }

//============================== end of file ===============================

