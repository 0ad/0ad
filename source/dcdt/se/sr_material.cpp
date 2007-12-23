#include "precompiled.h"
#include "0ad_warning_disable.h"
# include "sr_material.h"

//# define SR_USE_TRACE1 
# include "sr_trace.h"

SrMaterial::SrMaterial () :
            ambient  (  51,  51,  51, 255 ),
            diffuse  ( 204, 204, 204, 255 ),
            specular (   0,   0,   0, 255 ),
            emission (   0,   0,   0, 255 )
 {
   shininess = 0;
 }

void SrMaterial::init () 
 { 
   ambient.set  (  51,  51,  51, 255 );
   diffuse.set  ( 204, 204, 204, 255 );
   specular.set (   0,   0,   0, 255 );
   emission.set (   0,   0,   0, 255 );
   shininess = 0;
 }

bool operator == ( const SrMaterial& m1, const SrMaterial& m2 )
 {
   return ( m1.ambient==m2.ambient &&
            m1.diffuse==m2.diffuse &&
            m1.specular==m2.specular &&
            m1.emission==m2.emission &&
            m1.shininess==m2.shininess )? true:false;
 }

bool operator != ( const SrMaterial& m1, const SrMaterial& m2 )
 {
   return ( m1.ambient==m2.ambient &&
            m1.diffuse==m2.diffuse &&
            m1.specular==m2.specular &&
            m1.emission==m2.emission &&
            m1.shininess==m2.shininess )? false:true;
 }

SrOutput& operator<< ( SrOutput& o, const SrMaterial& m )
 {
   return o << m.ambient  <<' '<< 
               m.diffuse  <<' '<< 
               m.specular <<' '<< 
               m.emission <<' '<< 
               m.shininess;
 }

SrInput& operator>> ( SrInput& in, SrMaterial& m )
 {
   return in >> m.ambient  >>
                m.diffuse  >>
                m.specular >>
                m.emission >>
                m.shininess;
 }

//================================ End of File =================================================
