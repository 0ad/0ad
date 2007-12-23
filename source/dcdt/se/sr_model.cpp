#include "precompiled.h"
#include "0ad_warning_disable.h"
# include <stdlib.h>

# include "sr_model.h"
# include "sr_tree.h"
# include "sr_sphere.h"
# include "sr_cylinder.h"
# include "sr_string_array.h"

//# define SR_USE_TRACE1 // IO
//# define SR_USE_TRACE2 // Validation of normals materials, etc
# include "sr_trace.h"
 
//=================================== SrModel =================================================

const char* SrModel::class_name = "Model";

SrModel::SrModel ()
 {
   culling = true;
 }

SrModel::~SrModel ()
 {
 }

void SrModel::init ()
 {
   culling = true;
   M.capacity ( 0 );
   V.capacity ( 0 );
   N.capacity ( 0 );
   T.capacity ( 0 );
   F.capacity ( 0 );
   Fm.capacity ( 0 );
   Fn.capacity ( 0 );
   Ft.capacity ( 0 );
   mtlnames.capacity ( 0 );
   name = "";
 }

void SrModel::compress ()
 {
   M.compress();
   V.compress();
   N.compress();
   T.compress();
   F.compress();
   Fm.compress();
   Fn.compress();
   Ft.compress();
   mtlnames.compress();
   name.compress();
 }

void SrModel::validate ()
 {
   int i, j;
   SrArray<int> iarray;
   SrStringArray sarray;

   int fsize = F.size();

   // check if the model is empty
   if ( fsize==0 || V.size()==0 )
    { init ();
      compress ();
      return;
    }

   // check size of Fn
   if ( Fn.size()!=fsize || N.size()==0 )
    { Fn.size(0);
      N.size(0);
    }

   // check size of Ft
   if ( Ft.size()!=fsize || T.size()==0 )
    { Ft.size(0);
      T.size(0);
    }

   // check size of Fm
   if ( M.size()==0 )
    { Fm.size(0);
    }
   else if ( Fm.size()!=fsize )
    { j = Fm.size();
      Fm.size ( fsize );
      for ( i=j; i<fsize; i++ ) Fm[i]=0;
    }
 }

void SrModel::remove_redundant_materials ()
 {
   int i, j, k;
   SrArray<int> iarray;

   int fsize = F.size();

   if ( M.size()==0 )
    { Fm.size(0);
    }
   else 
    { j = Fm.size();
      Fm.size ( fsize );
      for ( i=j; i<fsize; i++ ) Fm[i]=0;

      // remove references to duplicated materials
      int msize = M.size();
      for ( i=0; i<msize; i++ ) 
       { for ( j=i+1; j<msize; j++ ) 
          { if ( M[i]==M[j] )
             { SR_TRACE2 ( "Detected material "<<i<<" equal to "<<j );
               for ( k=0; k<fsize; k++ ) // replace references to j by i
                 if ( Fm[k]==j ) Fm[k]=i;
             }
          }
       }

      // check for nonused materials
      iarray.size ( M.size() );
      iarray.setall ( -1 );

      for ( i=0; i<fsize; i++ ) 
       { if ( Fm[i]>=0 && Fm[i]<M.size() )
          iarray[Fm[i]] = 0; // mark used materials
         else Fm[i] = -1;
       }
      int toadd = 0;
      for ( i=0; i<iarray.size(); i++ ) 
       { if ( iarray[i]<0 )
          { SR_TRACE2 ( "Detected unused material "<<i );
            //sarray.set ( i, 0 ); 
            toadd++;
          }
         else
          iarray[i] = toadd;
       }

/*      for ( i=0; i<iarray.size(); i++ ) // update material names indices
       { if ( iarray[i]<0 ) continue;
         mtlnames.set ( i-iarray[i], mtlnames[i] );
         //material_names << i-iarray[i] << srspc << sarray[i] << srspc;
       }*/
      //k = material_names.len();
      //if ( k>0 ) material_names[k-1]=0; // remove the last space

      for ( i=0; i<fsize; i++ ) // update indices
       { if ( Fm[i]<0 ) continue;
         Fm[i] -= iarray[Fm[i]];
       }
      for ( i=0,j=0; i<iarray.size(); i++ ) // compress materials
       { if ( iarray[i]<0 )
          { M.remove(j);
            mtlnames.remove(j);
          }
         else
          { j++; }
       }
    }
 }

void SrModel::remove_redundant_normals ( float dang )
 {
   int i, j, k;
   float ang;
   SrArray<int> iarray;

   int fsize = F.size();

   if ( N.size()==0 || Fn.size()!=fsize )
    { N.size(0); 
      Fn.size(0);
    }
   else 
    { // remove references to duplicated normals
      int nsize = N.size();
      for ( i=0; i<nsize; i++ ) 
       { for ( j=i+1; j<nsize; j++ ) 
          { ang = angle ( N[i],N[j] );
            if ( ang<=dang )
             { SR_TRACE2 ( "Detected normal "<<i<<" close to "<<j );
               for ( k=0; k<fsize; k++ ) // replace references to j by i
                { if ( Fn[k].a==j ) Fn[k].a=i;
                  if ( Fn[k].b==j ) Fn[k].b=i;
                  if ( Fn[k].c==j ) Fn[k].c=i;
                }
             }
          }
       }

      // check for nonused materials
      iarray.size ( nsize );
      iarray.setall ( -1 );

      for ( i=0; i<fsize; i++ )  // mark used materials
       { iarray[Fn[i].a] = 1;
         iarray[Fn[i].b] = 1;
         iarray[Fn[i].c] = 1;
       }

      int toadd = 0;
      for ( i=0; i<iarray.size(); i++ ) 
       { if ( iarray[i]<0 )
          { SR_TRACE2 ( "Detected unused normal "<<i );
            toadd++;
          }
         else
          iarray[i] = toadd;
       }

      for ( i=0; i<fsize; i++ ) // update indices
       { Fn[i].a -= iarray[Fn[i].a];
         Fn[i].b -= iarray[Fn[i].b];
         Fn[i].c -= iarray[Fn[i].c];
       }

      for ( i=0,j=0; i<iarray.size(); i++ ) // compress N
       { if ( iarray[i]<0 )
          { N.remove(j); }
         else
          { j++; }
       }
    }
 }

void SrModel::merge_redundant_vertices ( float prec )
 {
   prec = prec*prec;
   
   int fsize = F.size();
   int vsize = V.size();
   int i, j;

   // build iarray marking replacements:
   SrArray<int> iarray;
   iarray.size ( vsize );
   for ( i=0; i<vsize; i++ ) iarray[i]=i;
   
   for ( i=0; i<vsize; i++ )
    for ( j=0; j<vsize; j++ )
     { if ( i==j ) break; // keep i < j
       if ( dist2(V[i],V[j])<prec ) // equal
        { iarray[j]=i;
        }
     }

   // fix face indices:
   for ( i=0; i<fsize; i++ )
    { F[i].a = iarray[ F[i].a ];
      F[i].b = iarray[ F[i].b ];
      F[i].c = iarray[ F[i].c ];
    }

   // compress indices:   
   int ind=0;
   bool newv;
   for ( i=0; i<vsize; i++ )
    { newv = iarray[i]==i? true:false;
      V[ind] = V[i];
      iarray[i] = ind;
      if ( newv ) ind++;
    }
   V.size ( ind );

   // fix face indices again:
   for ( i=0; i<fsize; i++ )
    { F[i].a = iarray[ F[i].a ];
      F[i].b = iarray[ F[i].b ];
      F[i].c = iarray[ F[i].c ];
    }
 }

bool SrModel::load ( SrInput &in )
 {
   int i;
   SrString s;

   if ( !in.valid() ) return false;
   // ensure proper comment style
   in.comment_style ( '#' );

   // check signature
   in >> s;
   if ( s!="SrModel" ) return false;

   // clear arrays and set culling to true
   init ();

   while ( !in.finished() )
    { 
      if ( !in.read_field(s) )
       { if ( in.finished() ) break; // EOF reached
         return false;
       }

      SR_TRACE1 ( '<' << s << "> Field found...\n" );

      if ( s=="culling" ) // read culling state
       { in >> i; 
         culling = i? true:false;
         if ( !in.close_field(s) ) return false;
       }
      else if ( s=="name" ) // read name (is a SrInput::String type )
       { in.get_token();
         name = in.last_token();
         if ( !in.close_field(s) ) return false;
       }
      else if ( s=="vertices" ) // read vertices: x y z
       { in >> i; V.size(i);
         for ( i=0; i<V.size(); i++ ) 
          fscanf ( in.filept(), "%f %f %f", &V[i].x, &V[i].y, &V[i].z ); // equiv to: in >> V[i];
         if ( !in.close_field(s) ) return false;
       }
      else if ( s=="vertices_per_face" ) // read F: a b c
       { in >> i; F.size(i);
         for ( i=0; i<F.size(); i++ )
          { fscanf ( in.filept(), "%d %d %d", &F[i].a, &F[i].b, &F[i].c ); // equiv to: in >> F[i];
            F[i].validate(); 
          }
         if ( !in.close_field(s) ) return false;
       }
      else if ( s=="normals_per_face" ) // read Fn: a b c
       { in >> i; Fn.size(i);
         for ( i=0; i<Fn.size(); i++ )
          fscanf ( in.filept(), "%d %d %d", &Fn[i].a, &Fn[i].b, &Fn[i].c ); // equiv to: in >> Fn[i];
         if ( !in.close_field(s) ) return false;
       }
      else if ( s=="textcoords_per_face" ) // read Ft: a b c
       { in >> i; Ft.size(i);
         for ( i=0; i<Ft.size(); i++ ) in >> Ft[i];
         if ( !in.close_field(s) ) return false;
       }
      else if ( s=="materials_per_face" ) // read Fm: i
       { in >> i; Fm.size(i);
         for ( i=0; i<Fm.size(); i++ ) fscanf ( in.filept(), "%d", &Fm[i] ); // equiv to: in >> Fm[i];
         if ( !in.close_field(s) ) return false;
       }
      else if ( s=="normals" ) // read N: x y z
       { in >> i; N.size(i);
         for ( i=0; i<N.size(); i++ )
          fscanf ( in.filept(), "%f %f %f", &N[i].x, &N[i].y, &N[i].z ); // equiv to: in >> N[i];
         if ( !in.close_field(s) ) return false;
       }
      else if ( s=="textcoords" ) // read T: x y
       { in >> i; T.size(i);
         for ( i=0; i<T.size(); i++ ) in >> T[i];
         if ( !in.close_field(s) ) return false;
       }
      else if ( s=="materials" ) // read M: mtls
       { in >> i; M.size(i);
         for ( i=0; i<M.size(); i++ ) in >> M[i];
         if ( !in.close_field(s) ) return false;
       }
      else if ( s=="material_names" ) // read materials
       { SrString buf1, buf2;
         mtlnames.capacity ( 0 ); // clear all
         mtlnames.size ( M.size() ); // realloc
         while ( 1 )
          { if ( in.get_token()!=SrInput::Integer ) { in.unget_token(); break; }
            i = atoi ( in.last_token() );
            in.get_token();
            mtlnames.set ( i, in.last_token() );
          }
         if ( !in.close_field(s) ) return false;
       }
      else // unknown field, just skip it:
       { if ( !in.skip_field(s) ) return false;
       }
    }

   SR_TRACE1 ( "OK.\n" );

   return true;
 }

bool SrModel::save ( SrOutput &o ) const
 {
   int i;

   // save header as a comment
   o << "# SR - Simulation and Representation Toolkit\n" <<
        "# Marcelo Kallmann 1996-2004\n\n";

   // save signature
   o << "SrModel\n\n";

   // save name
   if ( name.len()>0 )
    { o << "<name> \"" << name << "\" </name>\n\n"; }

   // save culling state
   if ( !culling )
    { o << "<culling> 0 </culling>\n\n"; }

   // save vertices (V)
   if ( V.size() ) 
    { o << "<vertices> " << V.size() << srnl;
      for ( i=0; i<V.size(); i++ ) o << V[i] << srnl;
      o << "</vertices>\n\n";
    }

   // save faces (F)
   if ( F.size() )
    { o << "<vertices_per_face> " << F.size() << srnl;
      for ( i=0; i<F.size(); i++ ) o << F[i] << srnl;
      o << "</vertices_per_face>\n\n";
    }

   // save normals (N)
   if ( N.size() )
    { o << "<normals> " << N.size() << srnl;
      for ( i=0; i<N.size(); i++ ) o << N[i] << srnl;
      o << "</normals>\n\n";
    }

   // save normals per face (Fn)
   if ( Fn.size() )
    { o << "<normals_per_face> " << Fn.size() << srnl;
      for ( i=0; i<Fn.size(); i++ ) o << Fn[i] << srnl;
      o << "</normals_per_face>\n\n";
    }

   // save texture coordinates (T)
   /* Not yet supported so not saved
   if ( T.size() )
    { o << "<textcoords> " << N.size() << srnl;
      for ( i=0; i<T.size(); i++ ) o << T[i] << srnl;
      o << "</textcoords>\n\n";
    } */

   // save texture coordinates per face (Ft)
   /* Not yet supported so not saved
   if ( Ft.size() )
    { o << "<textcoords_per_face> " << Ft.size() << srnl;
      for ( i=0; i<Ft.size(); i++ ) o << Ft[i] << srnl;
      o << "</textcoords_per_face>\n\n";
    } */

   // save materials (M)
   if ( M.size() )
    { o << "<materials> " << M.size() << srnl;
      for ( i=0; i<M.size(); i++ ) o << M[i] << srnl;
      o << "</materials>\n\n";
    }

   // save materials per face (Fm)
   if ( Fm.size() )
    { o << "<materials_per_face> " << Fm.size() << srnl;
      for ( i=0; i<Fm.size(); i++ ) o << Fm[i] << srnl;
      o << "</materials_per_face>\n\n";
    }

   // save material names if there is one:
   bool savemtl=false;
   for ( i=0; i<mtlnames.size(); i++ )
    { if ( mtlnames[i][0] ) { savemtl=true; break; }
    }
   
   if ( savemtl )
    { o << "<material_names> " << srnl;
      for ( i=0; i<mtlnames.size(); i++ )
       { if ( mtlnames[i][0] ) o << i << srspc << mtlnames[i] << srnl;
       }
      o << "</material_names>\n\n";
    }

   // done.
   return true;
 }

struct EdgeNode : public SrTreeNode // EdgeNode is only internally used :
 { int a, b;
   EdgeNode ( int x, int y ) : a(x), b(y) {}
   EdgeNode () : a(0), b(0) {}
   EdgeNode ( const EdgeNode& e ) : a(e.a), b(e.b) {}
  ~EdgeNode () {}
   friend SrOutput& operator<< ( SrOutput& out, const EdgeNode& e ) { return out; };
   friend SrInput& operator>> ( SrInput& inp, EdgeNode& e ) { return inp; }
   friend int sr_compare ( const EdgeNode* e1, const EdgeNode* e2 )
    { return e1->a!=e2->a ? e1->a-e2->a : e1->b-e2->b; }
 };

void SrModel::make_edges ( SrArray<int> &E )
 {
   int i;

   SrTree<EdgeNode> t;

   E.size(0);
   if ( F.empty() ) return;
   
   for ( i=0; i<F.size(); i++ )
    { t.insert_or_del ( new EdgeNode ( SR_MIN(F[i].a,F[i].b), SR_MAX(F[i].a,F[i].b) ) );
      t.insert_or_del ( new EdgeNode ( SR_MIN(F[i].b,F[i].c), SR_MAX(F[i].b,F[i].c) ) );
      t.insert_or_del ( new EdgeNode ( SR_MIN(F[i].c,F[i].a), SR_MAX(F[i].c,F[i].a) ) );
    }
   E.size ( 2*t.elements() );
   E.size ( 0 );
   for ( t.gofirst(); t.cur()!=SrTreeNode::null; t.gonext() )
    { E.push() = t.cur()->a;
      E.push() = t.cur()->b;
    }
 }

float SrModel::count_mean_vertex_degree ()
 {
   int i;
   if ( F.empty() ) return 0.0f;

   SrArray<int> vi(V.size());
   for ( i=0; i<vi.size(); i++ ) vi[i]=0;

   for ( i=0; i<F.size(); i++ )
    { vi[F[i].a]++; vi[F[i].b]++; vi[F[i].c]++; }

   double k=0;
   for ( i=0; i<vi.size(); i++ ) k += double(vi[i]);
   return float( k/double(vi.size()) );

   /* old way:
   SrTree<srEdgeNode> t;
   
   for ( i=0; i<F.size(); i++ )
    { t.insert_or_del ( new srEdgeNode ( F[i].a, F[i].b ) );
      t.insert_or_del ( new srEdgeNode ( F[i].b, F[i].a ) );
      t.insert_or_del ( new srEdgeNode ( F[i].b, F[i].c ) );
      t.insert_or_del ( new srEdgeNode ( F[i].c, F[i].b ) );
      t.insert_or_del ( new srEdgeNode ( F[i].c, F[i].a ) );
      t.insert_or_del ( new srEdgeNode ( F[i].a, F[i].c ) );
    }

   return (float)t.size() / (float)V.size(); 
   */
 }

void SrModel::get_bounding_box ( SrBox &box ) const
 {
   box.set_empty ();
   if ( V.empty() ) return;
   int i, s=V.size();
   for ( i=0; i<s; i++ ) box.extend ( V[i] );
 }

void SrModel::translate ( const SrVec &tr )
 {
   int i, s=V.size();
   for ( i=0; i<s; i++ ) V[i]+=tr;
 }

void SrModel::scale ( float factor )
 {
   int i, s=V.size();
   for ( i=0; i<s; i++ ) V[i]*=factor;
 }

void SrModel::centralize ()
 {
   SrVec v; SrBox box;

   get_bounding_box(box);

   v = box.center() * -1.0;
   translate ( v );
 }

void SrModel::normalize ( float maxcoord )
 {
   SrVec p; SrBox box;

   get_bounding_box(box);

   p = box.center() * -1.0;
   translate ( p );

   box+=p;
   SR_POS(box.a.x); SR_POS(box.a.y); SR_POS(box.a.z);
   SR_POS(box.b.x); SR_POS(box.b.y); SR_POS(box.b.z);

   p.x = SR_MAX(box.a.x,box.b.x);
   p.y = SR_MAX(box.a.y,box.b.y);
   p.z = SR_MAX(box.a.z,box.b.z);

   float maxactual = SR_MAX3(p.x,p.y,p.z);

   scale ( maxcoord/maxactual );  // Now we normalize to get the desired radius
 }

struct VertexNode : public SrTreeNode // only internally used
 { int v, i, f;
   VertexNode ( int a, int b, int c ) : v(a), i(b), f(c) {}
   VertexNode () { v=i=f=0; }
   VertexNode ( const VertexNode& x ) : v(x.v), i(x.i), f(x.f) {}
  ~VertexNode () {}
   friend SrOutput& operator<< ( SrOutput& out, const VertexNode& v ) { return out; };
   friend SrInput& operator>> ( SrInput& inp, VertexNode& v ) { return inp; }
   friend int sr_compare ( const VertexNode* v1, const VertexNode* v2 )
    { return v1->v!=v2->v ? v1->v-v2->v   // vertices are different
                          : v1->i-v2->i;  // vertices are equal: use index i
    }
 };

static void insertv ( SrTree<VertexNode>& t, SrArray<int>& vi, int v, int f )
 {
   // array vi is only used to generated a suitable tree key sorting the vertices.
   VertexNode *n = new VertexNode(v,++vi[v],f);
   if ( !t.insert(n) ) sr_out.fatal_error("Wrong faces in SrModel::smooth ()!\n");
 }

int SrModel::common_vertices_of_faces ( int i1, int i2 )
 {
   int i, j, c=0;
   int *f1 = &(F[i1].a);
   int *f2 = &(F[i2].a);
   for ( i=0; i<3; i++ )
    { for ( j=0; j<3; j++ )
       { if ( f1[i]==f2[j] ) c++; //sr_out<<i<<","<<j<<srspc;
       }
    }
   return c;
 }

void SrModel::flat ()
 {
   N.size(0);
   Fn.size(0);
   compress ();
 }

void SrModel::set_one_material ( const SrMaterial& m )
 {
   int i;
   mtlnames.capacity(1);
   mtlnames.size(1);
   M.size(1);
   Fm.size ( F.size() );
   M[0] = m;
   for ( i=0; i<Fm.size(); i++ ) Fm[i]=0;
   compress ();
 }

void SrModel::clear_materials ()
 {
   mtlnames.capacity (0);
   M.size (0);
   Fm.size (0);
   compress ();
 }

void SrModel::clear_textures ()
 {
   T.size(0);
   Ft.size(0);
   compress ();
 }

//v:current vtx, vi:vertices around v indicating the faces around v, vec:just a buffer
static void gen_normal ( int v, SrArray<SrVec>& vec, SrArray<int>& vi, SrModel *self, float crease_angle )
 {
   int i, j, tmp;
   float ang;

   vec.size(vi.size());

   //sr_out<<"original:\n";
   //for ( i=0; i<vi.size(); i++ ) sr_out<<self->F[vi[i]].a<<","<<self->F[vi[i]].b<<","<<self->F[vi[i]].c<<srnl;

   // order faces around vertex (could use qsort in SrArray):
   for ( i=0; i<vi.size(); i++ )
    { for ( j=i+1; j<vi.size(); j++ )
       { if ( self->common_vertices_of_faces(vi[i],vi[j])==2 ) // share an edge
	      { SR_SWAP(vi[i+1],vi[j]);
            break;
	      }
       }
    }

   // gen normals for each face around v:
   for ( i=0; i<vi.size(); i++ ) 
    { vec[i]= self->face_normal ( vi[i] ); }

   // search for the first edge with a big angle and rearrange array, so
   // that the array starts with a "crease angled edge":
   bool angfound = false;
   for ( i=0; i<vec.size(); i++ )
    { ang = angle ( vec[i], vec[(i+1)%vec.size()]);
      if ( ang>crease_angle ) 
       { for ( j=0; j<=i; j++ ) { vec.push()=vec[j]; vi.push()=vi[j]; }
	     vec.remove ( 0, i+1 );
	     vi.remove ( 0, i+1 );
         angfound = true;
         break;
       }
    }
   if ( !angfound ) return; // no crease angles in this face cluster

   // Finally set the normals:
   SrVec n;
   float x=1.0f;
   int init=0;
   SrArray<SrVec>& N = self->N;
   for ( i=0; i<vec.size(); i++ )
    { ang = angle ( vec[i], vec[(i+1)%vec.size()]);

      if ( ang>crease_angle )
       { n = SrVec::null;
         x = 0.0f;
         for ( j=init; j<=i; j++ ) { n+=vec[j]; x=x+1.0f; }
         n /= x; // n is the mean normal of the previous set of smoothed faces around v

         for ( j=init; j<=i; j++ ) 
          { SrModel::Face &fn=self->Fn[vi[j]];
            /*if ( f.n<0 ) 
              { f.n=N.size(); N.insert(N.size(),3); N[f.n]=N[f.a]; N[f.n+1]=N[f.b]; N[f.n+2]=N[f.c]; }
            if ( v==f.a ) N[f.n]=n;
	         else if ( v==f.b ) N[f.n+1]=n;
	          else N[f.n+2]=n;
            */
            if ( v==fn.a ) fn.a = N.size();
	         else if ( v==fn.b ) fn.b = N.size();
	          else fn.c = N.size();
            N.push() = n;
          }

         init = i+1;
       }
    } 
 }

void SrModel::smooth ( float crease_angle )
 {
   int v, i;
   SrTree<VertexNode> t;
   SrArray<int> vi;
   SrArray<SrVec> vec; // this is just a buffer to be used in gen_normal()

   if ( !V.size() || !F.size() ) return;

   Fn.size ( F.size() );

   vi.size(V.size());
   for ( i=0; i<vi.size(); i++ ) vi[i]=0;

   for ( i=0; i<F.size(); i++ )
    { insertv ( t, vi, F[i].a, i );
      insertv ( t, vi, F[i].b, i );
      insertv ( t, vi, F[i].c, i );
      Fn[i].a = F[i].a;
      Fn[i].b = F[i].b;
      Fn[i].c = F[i].c;
    }

   // first pass will interpolate face normals around each vertex:
   N.size ( V.size() );
   vi.size(0);
   t.gofirst ();
   while ( t.cur()!=SrTreeNode::null )
    { v = t.cur()->v;
      vi.push() = t.cur()->f;
      t.gonext();
      if ( t.cur()==SrTreeNode::null || v!=t.cur()->v )
       { SrVec n = SrVec::null;
         for ( i=0; i<vi.size(); i++ ) n += face_normal ( vi[i] );
         N[v] = n / (float)vi.size();
         vi.size(0);
       }
    }

   if ( crease_angle<0 ) return; // only smooth everything

   // second pass will solve crease angles:
   vi.size(0);
   t.gofirst ();
   while ( t.cur()!=SrTreeNode::null )
    { v = t.cur()->v;
      vi.push() = t.cur()->f;
      t.gonext();
      if ( t.cur()==SrTreeNode::null || v!=t.cur()->v )
       { gen_normal ( v, vec, vi, this, crease_angle );
         vi.size(0);
       }
    }

   remove_redundant_normals ();
   
   compress ();
 }

SrVec SrModel::face_normal ( int f ) const
 { 
   SrVec n; 
   const Face& fac = F[f];
   n.cross ( V[fac.b]-V[fac.a], V[fac.c]-V[fac.a] ); 
   n.normalize(); 
   return n; 
 }

void SrModel::invert_faces ()
 {
   int i, tmp;
   for ( i=0; i<F.size(); i++ ) SR_SWAP ( F[i].b, F[i].c );
   for ( i=0; i<Fn.size(); i++ ) SR_SWAP ( Fn[i].b, Fn[i].c );
 }

void SrModel::invert_normals ()
 {
   int i;
   for ( i=0; i<N.size(); i++ ) N[i]*=-1.0;
 }

void SrModel::apply_transformation ( const SrMat& mat )
 {
   int i, size;
   SrMat m = mat;

   size = V.size();
   for ( i=0; i<size; i++ ) V[i] = V[i] * m;

   m.setl4 ( 0, 0, 0, 1 ); // remove translation to normals
   size = N.size();
   for ( i=0; i<size; i++ ) N[i] = N[i] * m;
 }

void SrModel::add_model ( const SrModel& m )
 {
   int origv = V.size();
   int origf = F.size();
   int orign = N.size();
   int origm = M.size();
   int mfsize = m.F.size();
   int i;

   if ( m.V.size()==0 || mfsize==0 ) return;

   // add vertices and faces:
   V.size ( origv+m.V.size() );
   for ( i=0; i<m.V.size(); i++ ) V[origv+i] = m.V.get(i);

   F.size ( origf+mfsize );
   for ( i=0; i<mfsize; i++ )
    { const Face& f = m.F.get(i);
      F[origf+i].set ( f.a+origv, f.b+origv, f.c+origv );
    }
   
   // add the normals:
   if ( m.Fn.size()>0 )
    { N.size ( orign+m.N.size() );
      for ( i=0; i<m.N.size(); i++ ) N[orign+i] = m.N.get(i);

      Fn.size ( origf+mfsize );
      for ( i=0; i<mfsize; i++ )
       { const Face& f = m.Fn.get(i);
         Fn[origf+i].set ( f.a+orign, f.b+orign, f.c+orign );
       }
    }

   // add the materials:
   if ( m.Fm.size()>0 )
    { M.size ( origm+m.M.size() );
      for ( i=0; i<m.M.size(); i++ ) M[origm+i] = m.M.get(i);

      Fm.size ( origf+mfsize );
      for ( i=0; i<mfsize; i++ )
       { Fm[origf+i] = m.Fm[i]+origm;
       }
    }

   // add material names
   for ( i=0; i<m.mtlnames.size(); i++ )
     mtlnames.push ( m.mtlnames[i] );
    
//    save(sr_out);
 }

void SrModel::operator = ( const SrModel& m )
 {
   M = m.M;
   V = m.V;
   N = m.N;
   T = m.T;
   F = m.F;
   Fm = m.Fm;
   Fn = m.Fn;
   Ft = m.Ft;

   culling = m.culling;

   mtlnames = m.mtlnames;
    
   name = m.name;
 }

void SrModel::make_box ( const SrBox& b )
 {
   init ();

   name = "box";

   V.size ( 8 );

   // side 4 has all z coordinates equal to a.z, side 5 equal to b.z
   b.get_side ( V[0], V[1], V[2], V[3], 4 );
   b.get_side ( V[4], V[5], V[6], V[7], 5 );

   F.size ( 12 );
   F[0].set(1,0,4); F[1].set(1,4,7); // plane crossing -X
   F[2].set(3,2,6); F[3].set(3,6,5); // plane crossing +X
   F[4].set(7,4,6); F[5].set(4,5,6); // plane crossing +Z
   F[6].set(0,1,2); F[7].set(0,2,3); // plane crossing -Z
   F[8].set(2,1,7); F[9].set(2,7,6); // plane crossing +Y
   F[10].set(0,3,4); F[11].set(4,3,5); // plane crossing -Y

   F.size ( 12 );

 }

void SrModel::make_sphere ( const SrSphere& s, float resolution )
 {
   init ();

   name = "sphere";
 }

void SrModel::make_cylinder ( const SrCylinder& c, float resolution, bool smooth )
 {
   init ();

   name = "cylinder";

   int nfaces = int(resolution*10.0f);
   if ( nfaces<3 ) nfaces = 3;
   
   float dang = sr2pi/float(nfaces);
   SrVec vaxis = c.b-c.a; 
   SrVec va = vaxis; 
   va.normalize(); // axial vector
   SrVec minus_va = va * -1.0f;

   SrVec vr;
   float deg = SR_TODEG ( angle(SrVec::i,va) );

   if ( deg<10 || deg>170 )
     vr = cross ( SrVec::j, va );
   else
     vr = cross ( SrVec::i, va );
   
   vr.len ( c.radius ); // radial vector

   SrMat rot;
   rot.rot ( va, dang );

   SrPnt a1 = c.a+vr;
   SrPnt b1 = c.b+vr;
   SrPnt a2 = a1 * rot;
   SrPnt b2 = a2 + vaxis;

   int i=1;

   do { if ( smooth )
         { N.push()=(a1-c.a)/c.radius; //normalized normal
         }
        V.push()=a1; V.push()=b1;
        if ( i==nfaces ) break;
        a1=a2; b1=b2; a2=a1*rot; b2=a2+vaxis;
        i++;
      } while ( true );

   // make sides:
   int n1, n=0;
   int i1, i2, i3;
   int size = V.size();
   for ( i=0; i<size; i+=2 )
    { i1 = V.validate ( i+1 );
      i2 = V.validate ( i+2 );
      i3 = V.validate ( i+3 );
      F.push().set ( i, i2, i1 );
      F.push().set ( i1, i2, i3 );

      if ( smooth )
       { n1 = N.validate ( n+1 );
         Fn.push().set ( n, n1, n );
         Fn.push().set ( n, n1, n1 );
         n++;
       }
    }

   // make top and bottom:
   n=0;
   size = V.size()+1;
   for ( i=0; i<size; i+=2 )
    { i1 = V.validate ( i+1 );
      i2 = V.validate ( i+2 );
      i3 = V.validate ( i+3 );
      F.push().set ( V.size(), i2, i );
      F.push().set ( V.size()+1, i1, i3 );

      if ( smooth )
       { n1 = N.validate ( n+1 );
         Fn.push().set ( N.size(), N.size(), N.size() );
         Fn.push().set ( N.size()+1, N.size()+1, N.size()+1 );
         n++;
       }
    }

   V.push(c.a);
   V.push(c.b);
   if ( smooth )
    { N.push()=minus_va;
      N.push()=va;
    }

   compress ();
 }

int SrModel::pick_face ( const SrLine& line ) const
 {
   int i;
   float t, u, v;
   SrArray<float> faces;
   faces.capacity(16);
   for ( i=0; i<F.size(); i++ )
    { const Face& f = F[i];
      if ( line.intersects_triangle ( V[f.a], V[f.b], V[f.c], t, u, v ) )
       { faces.push() = (float)i;
         faces.push() = t; // t==0:p1, t==1:p2
       }
    }

   int closest=-1;
   for ( i=0; i<faces.size(); i+=2 )
    { if ( closest<0 || faces[i+1]<faces[closest+1] ) closest=i;
    }

   if ( closest>=0 ) return (int)faces[closest];
   return closest;
 }

//================================ End of File =================================================
