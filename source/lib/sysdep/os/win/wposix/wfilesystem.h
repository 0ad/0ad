/* Copyright (C) 2010 Wildfire Games.
* This file is part of 0 A.D.
*
* 0 A.D. is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* (at your option) any later version.
*
* 0 A.D. is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INCLUDED_WFILESYSTEM
#define INCLUDED_WFILESYSTEM

//
// sys/stat.h
//

#include <sys/stat.h>	// for S_IFREG etc.

#if MSC_VERSION
typedef unsigned int mode_t;	// defined by MinGW but not VC
#define stat _stat64	// we need 64-bit st_size and time_t
#endif

// permission masks when creating files (_wsopen_s doesn't distinguish
// between owner/user/group)
#define S_IRWXO _S_IREAD|_S_IWRITE
#define S_IRWXU _S_IREAD|_S_IWRITE
#define S_IRWXG _S_IREAD|_S_IWRITE

#define S_ISDIR(m) (m & S_IFDIR)
#define S_ISREG(m) (m & S_IFREG)

#endif	// #ifndef INCLUDED_WFILESYSTEM
