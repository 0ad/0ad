#include "precompiled.h" 
# include "sr_color.h"

//========================================= static =======================================

const SrColor SrColor::black  (0,0,0);
const SrColor SrColor::red    (255,0,0);
const SrColor SrColor::green  (0,255,0);
const SrColor SrColor::yellow (255,255,0);
const SrColor SrColor::blue   (0,0,255);
const SrColor SrColor::magenta(255,0,255);
const SrColor SrColor::cyan   (0,255,255);
const SrColor SrColor::white  (255,255,255);
const SrColor SrColor::gray   (127,127,127);

//========================================= SrColor =======================================

void SrColor::set ( srbyte x, srbyte y, srbyte z, srbyte w )
 {
   r=x; g=y; b=z; a=w;
 }

void SrColor::set ( int x, int y, int z, int w )
 {
   r=(srbyte)x;
   g=(srbyte)y;
   b=(srbyte)z;
   a=(srbyte)w;
 }

void SrColor::set ( float x, float y, float z, float w )
 {
   r = (srbyte) ( x*255.0f );
   g = (srbyte) ( y*255.0f );
   b = (srbyte) ( z*255.0f );
   a = (srbyte) ( w*255.0f );
 }

void SrColor::set ( const char* s )
 {
   switch ( s[0] )
    { case 'b' : *this = s[2]=='a'? black:blue; break;
      case 'r' : *this = red; break;
      case 'g' : *this = s[2]=='e'? green:gray; break;
      case 'y' : *this = yellow; break;
      case 'm' : *this = magenta; break;
      case 'c' : *this = cyan; break;
      case 'w' : *this = white; break;
    }
 }

// we dont do double versions to avoid automatic typecasts complications...
void SrColor::get ( float f[4] ) const
 {
   f[0] = ((float)r) / 255.0f;
   f[1] = ((float)g) / 255.0f;
   f[2] = ((float)b) / 255.0f;
   f[3] = ((float)a) / 255.0f;
 }

void SrColor::get ( int i[4] ) const
 { 
   i[0] = (int)r;
   i[1] = (int)g;
   i[2] = (int)b;
   i[3] = (int)a;
 }

void SrColor::get ( srbyte x[4] ) const
 { 
   x[0] = r;
   x[1] = g;
   x[2] = b;
   x[3] = a;
 }

bool operator == ( const SrColor &c1, const SrColor &c2 )
 {
   return c1.r==c2.r && c1.g==c2.g &&c1.b==c2.b && c1.a==c2.a? true:false;
 }

bool operator != ( const SrColor &c1, const SrColor &c2 )
 {  
   return c1.r==c2.r && c1.g==c2.g &&c1.b==c2.b && c1.a==c2.a? false:true;
 }

SrColor lerp ( const SrColor &c1, const SrColor &c2, float t )
 {
   SrColor c;

   c.r = (srbyte) (SR_LERP ( float(c1.r), float(c2.r), t ) + 0.5f);
   c.g = (srbyte) (SR_LERP ( float(c1.g), float(c2.g), t ) + 0.5f);
   c.b = (srbyte) (SR_LERP ( float(c1.b), float(c2.b), t ) + 0.5f);
   c.a = (srbyte) (SR_LERP ( float(c1.a), float(c2.a), t ) + 0.5f);
   return c;
 }

SrOutput& operator<< ( SrOutput& o, const SrColor& c )
 {
   return o << c.r <<' '<< c.g <<' '<< c.b <<' '<< c.a;
 }

SrInput& operator>> ( SrInput& in, SrColor& c )
 {
   return in >> c.r >> c.g >> c.b >> c.a;
 }

//=================================== End of File ==========================================
