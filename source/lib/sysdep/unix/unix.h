#ifndef _sysdep_unix_H
#define _sysdep_unix_H

/*
This implementation raises the SIGTRAP signal which should be caught by your
debugger
*/
extern void unix_debug_break();

#endif
