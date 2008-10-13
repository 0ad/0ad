#ifndef INCLUDED_WUTSNAME
#define INCLUDED_WUTSNAME

//
// <sys/utsname.h>
//

struct utsname
{
	char sysname[9];   // Name of this implementation of the operating system. 
	char nodename[16]; // Name of this node within an implementation-defined communications network. 
	char release[9];   // Current release level of this implementation. 
	char version[16];  // Current version level of this release. 
	char machine[9];   // Name of the hardware type on which the system is running. 
};

LIB_API int uname(struct utsname*);


#endif	// #ifndef INCLUDED_WUTSNAME
