#include "precompiled.h"
#include "0ad_warning_disable.h"
# include <stdlib.h>
# include <string.h>
# include <ctype.h>

# include "sr_mem_control.h"
# include "sr_output.h"

//=== Memory Control =======================================================================

// the folowing structure is used only internally
// I could have used a balanced tree, but since this is only for debug
// it is better to maintain it as simple as possible and as a stand alone file.
struct srMemData
 { const char* type;
   const char* file;
   int   line;
   int   size;
   void* addr;
   void set ( const char* c, char* f, int l, int s, void* a )
    { type=c; file=f; line=l; size=s; addr=a; }
 };

// it is not nice to use such static things intead of classes,
// but this is the only way to let the macro mechanism work.
static bool changed=false;
static srMemData *mem_data=0;
static int mem_data_size=0;
static int mem_data_capacity=0;
static unsigned mem_bytes_used=0;
static SrOutput& output=sr_out;

static int cmp_func ( const void* pt1, const void* pt2 )
 { 
   const srMemData *a = (const srMemData*)pt1;
   const srMemData *b = (const srMemData*)pt2; 

   int c = strcmp ( a->file, b->file );
   if ( c!=0 ) return c;

   c = strcmp ( a->type, b->type );
   if ( c!=0 ) return c;

   return b->size-a->size;
 }

static void sort ()
 {
   qsort ( mem_data, mem_data_size, sizeof(srMemData), cmp_func );
 }

void *sr_mem_control_alloc ( const char* type, char* file, int line, int size, void* addr )
 {
   if ( !addr ) 
    { //lineout ();
      output<<"\nsr_control_malloc: Zero Pointer Allocated!\n" << 
              "File:"<<file<<" Line:"<<line<<" Elem Size:"<<size<<" Address:"<<(intptr_t)addr<<'\n';
      //lineout ();
    }

   if (  mem_data_size==mem_data_capacity )
    { mem_data_capacity += 128;
      mem_data = (srMemData*) realloc ( mem_data, sizeof(srMemData)*mem_data_capacity );
    }

   // the following is to get rid of the full path of files (in visualc++):
   int i = (int)strlen(file);
   file += (i-1);
   while ( i>0 && *file!='\\' && *file!='/' ) { file--; i--; }
   if ( i>0 ) file++;

   changed=true;
   mem_data[mem_data_size++].set ( type, file, line, size, addr );
   mem_bytes_used += size;
   return addr;
 }

bool sr_mem_control_free ( char* file, int line, void* addr )
 {
   if ( !addr ) return true;

   for ( int i=0; i<mem_data_size; i++ )
    if ( mem_data[i].addr==addr )
     { mem_bytes_used -=  mem_data[i].size;
       mem_data[i]=mem_data[--mem_data_size];
       changed=true;
       return true;
     }

   output.fatal_error ( "sr_control_free: Not found address to delete!\n"
                        "File:%d Line:%d Address:%d", file, line, addr );
   return false;
 }

void* sr_mem_control_realloc ( char* file, int line, int size, void* addr )
 {
   sr_mem_control_free ( file, line, addr );
   addr = realloc ( addr, size );
   return sr_mem_control_alloc ( "realloc", file, line, size, addr );
 }

void sr_memory_report ()
 {
   sort ();
   output<<"Memory Report\n";
   output<<"Bytes not deleted: "<<mem_bytes_used<<srnl;

   if ( mem_data_size>0 ) 
     output.put ( "     File                      Type            Line   Size     Addr\n");

   for ( int i=0; i<mem_data_size; i++ )
    { srMemData &m = mem_data[i];
      output.putf ( "%3d: %-25s %-15s %-6d %-8d %d\n", i+1, m.file, m.type, m.line, m.size, (intptr_t)m.addr );
    }

   output << srnl;
 }

unsigned sr_memory_allocated ()
 {
   return mem_bytes_used;
 }

void sr_memory_report_output ( SrOutput& o )
 {
   output = o;
 }

//=== End of File =======================================================================


