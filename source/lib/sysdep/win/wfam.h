// SGI File Alteration Monitor for Win32
// Copyright (c) 2004 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#ifndef WFAM_H__
#define WFAM_H__


#include "posix.h"	// PATH_MAX


// FAM documentation: http://techpubs.sgi.com/library/tpl/cgi-bin/getdoc.cgi?coll=0650&db=bks&fname=/SGI_Developer/books/IIDsktp_IG/sgi_html/ch08.html


// opaque structs are too hard to keep in sync with the real definition,
// and we don't want to expose the internals. therefore, use the pImpl idiom.

struct FAMRequest
{
	void* internal;
	// reqnum not needed, since FAMMonitorDirectory2 isn't supported.
};

struct FAMConnection
{
	void* internal;
};

enum FAMCodes { FAMDeleted, FAMCreated, FAMChanged };

typedef struct
{
	FAMConnection* fc;
	FAMRequest fr;
	char filename[PATH_MAX];
	void* user;
	FAMCodes code;
}
FAMEvent;


extern int FAMOpen2(FAMConnection*, const char* app_name);
extern int FAMClose(FAMConnection*);

extern int FAMMonitorDirectory(FAMConnection*, const char* dir, FAMRequest* req, void* user);
extern int FAMCancelMonitor(FAMConnection*, FAMRequest* req);

extern int FAMPending(FAMConnection*);
extern int FAMNextEvent(FAMConnection*, FAMEvent* event);


#endif	// #ifndef WFAM_H__