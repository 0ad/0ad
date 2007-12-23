#include "precompiled.h"
#include "0ad_warning_disable.h"
# include <string.h>

# include "sr_image.h"

//=========================== SrImage ============================

SrImage::SrImage ()
 {
   _w = _h = _tw = 0;
   _data = 0;
 }

SrImage::~SrImage ()
 {
   delete _data;
 }

void SrImage::alloc ( int w, int h )
 {
   int newsize = w*h*3;

   if ( newsize!=_tw*_h )
    {
      delete _data;

      if ( w<=0 || h<=0 ) w=h=0;

      if ( w )
       _data = new srbyte [ newsize ]; // to hold rgb values
      else
       _data = 0;
    }

   _tw = 3*w;
   _w = w;
   _h = h;
 }

void SrImage::vertical_mirror ()
 {
   int i, ie, mid;
   mid = _h/2;
 
   srbyte* buf = new srbyte [ _tw ];

   for ( i=0,ie=_h-1; i<mid; i++,ie-- )
    { memcpy ( buf,      line(i),  _tw );
      memcpy ( line(i),  line(ie), _tw );
      memcpy ( line(ie), buf,      _tw );
    }

   delete buf;
 }

# define PutInt(i)   fwrite(&i,4/*bytes*/,1/*num items*/,f)
# define PutShort(s) fwrite(&s,2/*bytes*/,1/*num items*/,f)

bool SrImage::save_as_bmp ( const char* filename )
 {
   FILE* f = fopen ( filename, "wb" );
   if ( !f ) return false;

   int i = 0;
   int offset = 14+40;
   //int dw = 4-(_w%4); if ( dw==4 ) dw=0;
   int dw = (_w%4);

   int filesize = 14 /*header*/ + 40 /*info*/ + (_w*_h*3) +(_h*dw);

   // 14 bytes of header:
   fprintf ( f, "BM" ); // 2 bytes : signature
   PutInt ( filesize ); // file size
   PutInt ( i );        // reserved (zeros)
   PutInt ( offset );   // offset to the data

   // 40 bytes of info header:
   int infosize = 40;
   short planes = 1;
   short bits = 24;
   int compression = 0; // no compression
   int compsize = 0;    // no compression
   int hres = 600;
   int vres = 600;
   int colors = 0;
   int impcolors = 0;   // important colors: all
   PutInt ( infosize ); // size of info header
   PutInt ( _w );       // width
   PutInt ( _h );       // height
   PutShort ( planes );
   PutShort ( bits );
   PutInt ( compression );
   PutInt ( compsize );
   PutInt ( hres );
   PutInt ( vres );
   PutInt ( colors );
   PutInt ( impcolors );

   int w, h;
   srbyte* scanline;
   for ( h=_h-1; h>=0; h-- )
    { scanline = line(h);
      for ( w=0; w<_tw; w+=3 )
       { fputc ( scanline[w+2], f ); // B
         fputc ( scanline[w+1], f ); // G
         fputc ( scanline[w], f );   // R
       }
      for ( w=0; w<dw; w++ ) fputc ( 0, f );
    }

   fclose ( f );
   return true;
 }

//============================= end of file ==========================

