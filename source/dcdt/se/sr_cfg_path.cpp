#include "precompiled.h"
# include "sr_cfg_path.h"
# include "sr_random.h"

//# define SR_USE_TRACE1    // 
# include "sr_trace.h"

//================================ SrCfgPathBase ========================================

SrCfgPathBase::SrCfgPathBase ( SrCfgManagerBase* cman )
 {
   _cman = cman;
   _cman->ref();
   _size = 0;
   _interp_start = 0;
   _interp_startdist = 0;
 }

SrCfgPathBase::SrCfgPathBase ( const SrCfgPathBase& p )
 {
   _cman = p._cman;
   _cman->ref();
   _size = 0;
   insert_path ( 0, p );
   _interp_start = 0;
   _interp_startdist = 0;
 }
 
SrCfgPathBase::~SrCfgPathBase ()
 {
   init ();
   compress ();
   _cman->unref();
 }

void SrCfgPathBase::init ()
 {
   _size = 0;
   _interp_start = 0;
   _interp_startdist = 0;
 }

void SrCfgPathBase::compress ()
 {
   while ( _buffer.size()>_size ) _cman->free ( _buffer.pop() );
 }

void SrCfgPathBase::push ( const srcfg* c )
 {
   if ( _buffer.size()==_size ) // add new buffer entry
    { _buffer.push() = _cman->alloc();
    }
   if ( c ) _cman->copy ( _buffer[_size], c );
   _size++;
 }

void SrCfgPathBase::pop ()
 {
   if ( _size>0 ) _size--;
   _interp_start = 0;
   _interp_startdist = 0;
 }

void SrCfgPathBase::remove ( int i )
 {
   srcfg* cfg = _buffer[i];
   _buffer.move ( i/*dest*/, i+1/*src*/, _size-(i+1)/*n*/ );
   _size--;
   _buffer[_size] = cfg;
   _interp_start = 0;
   _interp_startdist = 0;
 }

void SrCfgPathBase::remove ( int i, int dp )
 {
   int bsize = _buffer.size();
   int idp = i+dp;
   _buffer.size ( bsize+dp );
   _buffer.move ( bsize/*dest*/, i/*src*/, dp/*n*/ ); // copy part to remove to the buffer end
   _buffer.move ( i/*dest*/, idp/*src*/, _size-idp/*n*/ ); // remove
   _buffer.move ( _size-dp/*dest*/, bsize/*src*/, dp/*n*/ ); // keep the removed part
   _buffer.size ( bsize );
   _size -= dp;
   _interp_start = 0;
   _interp_startdist = 0;
 }

void SrCfgPathBase::insert ( int i, const srcfg* c )
 {
   if ( i>=_size ) { push(c); return; }
   push ( c );
   srcfg* newcfg = top();
   _buffer.move ( i+1/*dest*/, i/*src*/, _size-(i+1)/*n*/ );
   _buffer[i] = newcfg;
   _interp_start = 0;
   _interp_startdist = 0;
 }

void SrCfgPathBase::insert_path ( int i, const SrCfgPathBase& p )
 {
   if ( p.size()<1 ) return;
   if ( i>_size ) i=_size;
   
   // open space in the end:
   int oldsize = _size;
   int newsize = _size+p.size();
   while ( _size!=newsize ) push(0);

   // move new space to the middle:
   int n, n2=newsize-(oldsize-i);
   srcfg* tmp;
   for ( n=i; n<oldsize; n++ )
    { SR_SWAP(_buffer[n],_buffer[n2]);
    }

   // copy contents:
   for ( n=0; n<p.size(); n++ )
    { _cman->copy ( _buffer[i+n], p._buffer[n] );
    }

   _interp_start = 0;
   _interp_startdist = 0;
 }

void SrCfgPathBase::append_path ( SrCfgPathBase& p ) 
 {
   // open space here:
   _buffer.insert ( _size, p.size() );
   
   // transfer nodes from p:
   int i;
   for ( i=0; i<p.size(); i++ ) _buffer[_size+i] = p._buffer[i];
   _size += p.size();
   
   // close space there:
   p._buffer.remove ( 0, p.size() );
   p._size=0;
 }

void SrCfgPathBase::swap ( int i, int j )
 {
   srcfg* tmp;
   SR_SWAP(_buffer[i],_buffer[j]);
   _interp_start = 0;
   _interp_startdist = 0;
 }

void SrCfgPathBase::revert ()
 {
   int i;
   int end = _size-1;
   int mid = _size/2;
   for ( i=0; i<mid; i++ )
    { swap ( i, end );
      end--;
    }
   _interp_start = 0;
   _interp_startdist = 0;
 }

void SrCfgPathBase::size ( int s )
 {
   while ( size()<s ) push(0);
   while ( size()>s ) pop();
   _interp_start = 0;
   _interp_startdist = 0;
 }

float SrCfgPathBase::len ( int i1, int i2 ) const
 {
   float l=0;
   int i;
   if ( _size<2 ) return l;
   for ( i=i1; i<i2; i++ ) l += _cman->dist(_buffer[i],_buffer[i+1]);
   return l;
 }

void SrCfgPathBase::interp ( float t, srcfg* c )
 {
   if ( _buffer.size()<2 ) { _interp_start=0; _interp_startdist=0; return; }

   // parameters _interp_start and _interp_startdist are used to optimize
   // the time search during sequential play, here we check if they can
   // be used or if we should recount the distance form the first node
   if ( t<_interp_startdist )
    { _interp_start=0; _interp_startdist=0; }

   float dt;
   float d = _interp_startdist;
   int i;

   for ( i=_interp_start+1; i<_buffer.size(); i++ )
    { dt = _cman->dist ( _buffer.const_get(i-1), _buffer.const_get(i) );
      if ( d+dt>=t ) break;
      d += dt;
    }

   if ( i==_buffer.size() ) // may happen because of imprecisions
    { i--; t=1; }
   else
    { t -= d; t /= dt; if ( t>1 ) t=1; }

   _interp_start = i-1;
   _interp_startdist = d;

   _cman->interp ( _buffer.const_get(i-1), _buffer.const_get(i), t, c );
 }

void SrCfgPathBase::temporal_interp ( float t, srcfg* c )
 {
   if ( _buffer.size()<2 ) { _interp_start=0; _interp_startdist=0; return; }

   // here _interp_startdist is in fact the "start time"
   if ( t<_interp_startdist )
    { _interp_start=0; _interp_startdist=0; }

   float dt; // delta time
   float nt; // next time
   float ct = _interp_startdist; // current time
   int i;

   for ( i=_interp_start+1; i<_buffer.size(); i++ )
    { nt = _cman->time ( _buffer.const_get(i) );
      dt = nt-ct;
      if ( nt>=t ) break;
      ct = nt;
    }

   if ( i==_buffer.size() ) // may happen because of imprecisions
    { i--; t=1; }
   else
    { t -= ct; t /= dt; if ( t>1 ) t=1; }

   _interp_start = i-1;
   _interp_startdist = ct;

   _cman->interp ( _buffer.const_get(i-1), _buffer.const_get(i), t, c );
 }

void SrCfgPathBase::smooth_random ( float prec, float& len )
 {
   if ( len<0 ) len = SrCfgPathBase::len(0,_size-1);
   float t1 = SrRandom::randf()*len;
   float t2 = SrRandom::randf()*len;
   linearize ( prec, len, t1, t2 );
 }
 
int SrCfgPathBase::linearize ( float prec, float& len, float t1, float t2 )
 {
   srcfg* cfg1; // 1st random cfg along the path
   srcfg* cfg2; // 2nd random cfg along the path
   srcfg* citp; // cfg used during interpolation

   // get buffer space for cfg1 and cfg2:
   if ( _buffer.size()<_size+3 ) // add new buffer entries
    { push(0); push(0); push(0); pop(); pop(); pop(); }
   citp = _buffer[_size];
   cfg1 = _buffer[_size+1];
   cfg2 = _buffer[_size+2];

   // ensure t1<t2:
   float tmp;
   if ( t1>len ) t1=len;
   if ( t2>len ) t2=len;
   if ( t1>t2 ) SR_SWAP(t1,t2);

   // get configurations cfg1 and cfg2 at t1 and t2:
   _interp_start = 0;
   _interp_startdist = 0;
   interp ( t1, cfg1 );
   int i1 = _interp_start+1; // node after t1
   interp ( t2, cfg2 );
   int i2 = _interp_start; // node prior t2

   if ( i1>i2 ) return 0; // samples t1 and t2 are in the same edge

   if ( !_cman->visible(cfg1,cfg2,citp,prec) ) return -1; // cannot smooth
   
   len -= SrCfgPathBase::len(i1-1,i2+1); // remove the older part length from len

   if ( i1==i2 ) // only 1 vertex between cfg1 and cfg2: insert 1 space
    { insert ( i1+1, 0 );
      i2 = i1+1;
    }
    
   // positions i1 and i2 become cfg1 and cfg2:
   _cman->copy ( _buffer[i1], cfg1 );
   _cman->copy ( _buffer[i2], cfg2 );

   // delete non-used intermediate vertices:
   remove ( i1+1, (i2-i1)-1 );
   len += SrCfgPathBase::len(i1-1,i1+2); // add the new part length to len
   
   return 1;
 }

void SrCfgPathBase::smooth_ends ( float prec )
 {
   if ( _size<5 ) return; // not possible if <5
   
   srcfg* citp; // cfg used during interpolation

   // get buffer space for citp:
   if ( _buffer.size()<_size+1 ) { push(0); pop(); }
   citp = _buffer[_size];

   // specify mid node and border nodes i1 and i2:
   int mid = _size/2;
   int i1 = mid;
   int i2 = mid;
   int posmax = _size-1;
   int max = _size-3;
   int min = 2;

   // check second half:
   while ( i2<=max )
    { if ( _cman->visible(_buffer[i2],_buffer[posmax],citp,prec) )
       { remove ( i2+1, (posmax-i2)-1 );
         break;
       }
      i2++;
    }

   // check first half:
   while ( i1>=min )
    { if ( _cman->visible(_buffer[0],_buffer[i1],citp,prec) )
       { remove ( 1,  i1-1 );
         break;
       }
      i1--;
    }
 }

float SrCfgPathBase::_diff ( int i, float prec )
 {
   if ( i<=0 || i>=size()-1 ) return -1; // protection
/*   srcfg* cfg1;
   srcfg* cfg2;

   // get buffer space for cfg1 and cfg2:
   if ( _buffer.size()<_size+2 ) // add new buffer entries
    { push(0); push(0); pop(); pop(); }
   cfg1 = _buffer[_size];
   cfg2 = _buffer[_size+1];
*/

   float d1 = _cman->dist(_buffer.const_get(i-1),_buffer.const_get(i));
   float d2 = _cman->dist(_buffer.const_get(i),_buffer.const_get(i+1));
   float d  = _cman->dist(_buffer.const_get(i-1),_buffer.const_get(i+1));

   float diff = (d1+d2)-d;
   diff /= d;
   /*
   prec*=5.0f;

   d = _cman->dist ( _buffer.const_get(i-1), _buffer.const_get(i) );
   t = prec/d;
   if ( t>1 ) t=1;
   _cman->interp ( _buffer.const_get(i-1), _buffer.const_get(i), t, cfg1 );

   d = _cman->dist ( _buffer.const_get(i), _buffer.const_get(i+1) );
   t = prec/d;
   if ( t>1 ) t=1;
   _cman->interp ( _buffer.const_get(i), _buffer.const_get(i+1), t, cfg2 );

   float diff = (_cman->dist(cfg1,_buffer.const_get(i))+_cman->dist(_buffer.const_get(i),cfg2))
               -_cman->dist(cfg1,cfg2);*/
   return diff;
 }

void SrCfgPathBase::smooth_init ( float prec )
 {
   _sprec = prec;
   //smooth_ends ( prec );
   _slen = len();
   _sbads=0;
   _slastangmax=9999999999.0f;
 }

bool SrCfgPathBase::smooth_step ()
 {
   smooth_random ( _sprec, _slen );
return false;
   int i, imax=1;
   int lasti = size()-2;
   float d;
   float angmax=0;
   float t=0, tmax=0;
   for ( i=1; i<=lasti; i++ )
    { t += _cman->dist ( _buffer.const_get(i-1), _buffer.const_get(i) );
      d = _diff ( i, _sprec );
      if ( d>angmax )
       { angmax=d; imax=i; tmax=t; }
    }

   //sr_out<<"angmax => "<<angmax<<srnl;
   //sr_out<<"prec   => "<<_sprec<<srnl;
   
   float maxradius = _slen/10.0f;
   float r, t1, t2;
   int result;
   int times=0;
   r = maxradius;
   while ( r>_sprec )
    { //r = SrRandom::randf()*maxradius;
      r/=2.0f;
      //sr_out<<times<<": "<<r<<srnl;
      t1 = tmax-r; if ( t1<0 ) t1=0;
      t2 = tmax+r; if ( t2>_slen ) t2=_slen;
      result = linearize ( _sprec, _slen, t1, t2 );
      times++;
      if ( result==0 ) break;
    }
   
   if ( angmax>_slastangmax )
    { _sbads++;
    }
   else
    { _sbads=0;
      _slastangmax=angmax;
    }

   //sr_out<<"BADS: "<<_bads<<srnl;    
   //float freedom = z.freedom / float(times);
    
   if ( _sbads>6 ) return true; // no more easy improvements
   return false;
 }

SrOutput& operator<< ( SrOutput& o, const SrCfgPathBase& p )
 {
   int i;
   for ( i=0; i<p.size(); i++ )
    { o<<"path node "<<i<<":\n";
      p._cman->output ( o, p._buffer[i] );
      o<<srnl;
    }
   return o;
 }

//================================ End of File =================================================

