// handle manager
//
// Copyright (c) 2003 Jan Wassenberg
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

#ifndef H_MGR_H__
#define H_MGR_H__

// do not include from public header files!
// handle.h declares type Handle, and avoids making
// everything dependent on this (rather often updated) header.


#include <stdarg.h>		// type init routines get va_list of args

#include "lib.h"

#ifndef HANDLE_DEFINED
#include "handle.h"
#endif



// handle type (for 'type safety' - can't use a texture handle as a sound)

	
//
// rationale: we could use the destructor passed to h_alloc to identify
// the handle, but it's good to have a list of all types, and we avoid having
// to create empty destructors for handle types that wouldn't need them.
// finally, we save memory - this fits in a few bits, vs. needing a pointer.

// registering extension for each module is bad - some may use many
// (e.g. texture - many formats).
// handle manager shouldn't know about handle types

/*
enum H_Type
{
	H_Mem      = 1,
	H_ZArchive = 2,
	H_ZFile    = 3,
	H_VFile    = 4,
	H_VRead    = 5,

	H_Tex      = 6,
	H_Font     = 7,
	H_Sound    = 8,

	NUM_HANDLE_TYPES
};
*/

/*
///xxx advantage of manual vtbl:
no boilerplate init, h_alloc calls ctor directly, make sure it fits in the memory slot
vtbl contains sizeof resource data, and name!
but- has to handle variable params, a bit ugly
*/

// 'manual vtbl' type id
// handles have a type, to prevent using e.g. texture handles as a sound.
//
// alternatives:
// - enum of all handle types (smaller, have to pass all methods to h_alloc)
// - class (difficult to compare type, handle manager needs to know of all users)
//
// checked in h_alloc:
// - user_size must fit in what the handle manager provides
// - name must not be 0
//
// init: user data is initially zeroed
// dtor: user data is zeroed afterwards
// reload: if this resource type is opened by another resource's reload,
// our reload routine MUST check if already opened! This is relevant when
// a file is reloaded: if e.g. a sound object opens a file, the handle
// manager calls the reload routines for the 2 handles in unspecified order.
// ensuring the order would require a tag field that can't overflow -
// not really guaranteed with 32-bit handles. it'd also be more work
// to sort the handles by creation time, or account for several layers of
// dependencies.
struct H_VTbl
{
	void(*init)(void* user, va_list);
	int(*reload)(void* user, const char* fn, Handle);
	void(*dtor)(void* user);
	size_t user_size;
	const char* name;
};

typedef H_VTbl* H_Type;

#define H_TYPE_DEFINE(type)\
	/* forward decls */\
	static void type##_init(type*, va_list);\
	static int type##_reload(type*, const char*, Handle);\
	static void type##_dtor(type*);\
	static H_VTbl V_##type =\
	{\
		(void(*)(void*, va_list))type##_init,\
		(int(*)(void*, const char*, Handle))type##_reload,\
		(void(*)(void*))type##_dtor,\
		sizeof(type),	/* control block size */\
		#type			/* name */\
	};\
	static H_Type H_##type = &V_##type

	// note: we cast to void* pointers so the functions can be declared to
	// take the control block pointers, instead of requiring a cast in each.
	// the forward decls ensure the function signatures are correct.


// convenience macro for h_user_data:
// casts its return value to the control block type.
// use if H_DEREF's returning a negative error code isn't acceptable.
#define H_USER_DATA(h, type) (type*)h_user_data(h, H_##type)

// even more convenient wrapper for h_user_data:
// declares a pointer (<var>), assigns it H_USER_DATA, and has
// the user's function return a negative error code on failure.
#define H_DEREF(h, type, var)\
	/* don't use STMT - var decl must be visible to "caller" */\
	type* const var = H_USER_DATA(h, type);\
	if(!var)\
		return ERR_INVALID_HANDLE;


// all functions check the passed tag (part of the handle) and type against
// the internal values. if they differ, an error is returned.



// resource scope
// used together with flags (e.g. in mem), so no separate type
enum
{
	RES_TEMP   = 1,
	RES_LEVEL  = 2,
	RES_STATIC = 4
};

#define RES_SCOPE_MASK 7

// h_alloc flags
enum
{
	// alias for RES_TEMP scope. the handle will not be kept open.
	RES_NO_CACHE = 1,

	// not cached, and will never reuse a previous instance
	RES_UNIQUE = 1|16,

	// the resource isn't backed by a file. the fn parameter is treated as the search key (uintptr_t)
	// currently only used by mem manager
	RES_KEY = 8
};



// allocate a new handle.
// if key is 0, or a (key, type) handle doesn't exist,
//   the first free entry is used.
// otherwise, a handle to the existing object is returned,
//   and HDATA.size != 0.
//// user_size is checked to make sure the user data fits in the handle data space.
// dtor is associated with type and called when the object is freed.
// handle data is initialized to 0; optionally, a pointer to it is returned.
extern Handle h_alloc(H_Type type, const char* fn, uint flags = 0, ...);
extern int h_free(Handle& h, H_Type type);


// find and return a handle by key (typically filename hash)
// currently O(log n).
//
// HACK: currently can't find RES_UNIQUE handles, because there
// may be multiple instances of them, breaking the lookup data structure.
extern Handle h_find(H_Type type, uintptr_t key);

// returns a void* pointer to the control block of the resource <h>,
// or 0 on error (i.e. h is invalid or of the wrong type).
// prefer using H_DEREF or H_USER_DATA.
extern void* h_user_data(Handle h, H_Type type);

extern const char* h_filename(Handle h);


extern int h_reload(const char* fn);

extern int res_cur_scope;

// force the resource to be freed immediately, even if cached.
// tag is not checked - this allows the first Handle returned
// (whose tag will change after being 'freed', but remaining in memory)
// to later close the object.
// this is used when reinitializing the sound engine -
// at that point, all (cached) OpenAL resources must be freed.
extern int h_force_free(Handle h, H_Type type);

// increment Handle <h>'s refcount.
// only meant to be used for objects that free a Handle in their dtor,
// so that they are copy-equivalent and can be stored in a STL container.
// do not use this to implement refcounting on top of the Handle scheme,
// e.g. loading a Handle once and then passing it around. instead, have each
// user load the resource; refcounting is done under the hood.
extern void h_add_ref(Handle h);

extern void h_mgr_shutdown(void);


#endif	// #ifndef H_MGR_H__


/*

[KEEP IN SYNC WITH WIKI]

introduction
------------

a resource is an instance of a specific type of game data (e.g. texture),
described by a control block (example fields: format, pointer to tex data).

this module allocates storage for the control blocks, which are accessed
via handle. it also provides support for transparently reloading resources
from disk (allows in-game editing of data), and caches resource data.
finally, it frees all resources at exit, preventing leaks.


handles
-------

handles are an indirection layer between client code and resources
(represented by their control blocks, which contains/points to its data).
they allow an important check not possible with a direct pointer:
guaranteeing the handle references a given resource /instance/.

problem: code C1 allocates a resource, and receives a pointer p to its
control block. C1 passes p on to C2, and later frees it.
now other code allocates a resource, and happens to reuse the free slot
pointed to by p (also possible if simply allocating from the heap).
when C2 accesses p, the pointer is valid, but we cannot tell that
it is referring to a resource that had already been freed. big trouble.

solution: each allocation receives a unique tag (a global counter that
is large enough to never overflow). Handles include this tag, as well
as a reference (array index) to the control block, which isn't directly
accessible. when dereferencing the handle, we check if the handle's tag
matches the copy stored in the control block. this protects against stale
handle reuse, double-free, and accidentally referencing other resources.

type: each handle has an associated type. these must be checked to prevent
using textures as sounds, for example. with the manual vtbl scheme,
this type is actually a pointer to the resource object's vtbl, and is
set up via H_TYPE_DEFINE. this means that types are private to the module
that declared the handle; knowledge of the type ensures the caller
actually declared, and owns the resource.


guide to defining and using resources
-------------------------------------

1) choose a name for the resource, used to represent all resources
   of this type. we will call ours Res1; all occurences of it below
   must be replaced with the actual name (exact spelling).
   why? the vtbl builder defines its functions as e.g. Res1_reload;
   your actual definition must match.

2) declare its control block:
   struct Res1
   {
       void* data1;	// data loaded from file
       uint flags;	// set when resource is created
   };

3) build its vtbl:
   H_TYPE_DEFINE(Res1);

   this defines the symbol H_Res1, which is used whenever the handle
   manager needs its type. it is only accessible to this module
   (file scope). note that it is actually a pointer to the vtbl.
   this must come before uses of H_Res1, and after the CB definition;
   there are no restrictions WRT functions, because the macro
   forward-declares what it needs.

4) implement all 'virtual' functions from the resource interface.
   note that inheritance isn't really possible with this approach -
   all functions must be defined, even if not needed.

   --

   init:
   one-time init of the control block. called from h_alloc.
   precondition: control block is initialized to 0.

   static void Type_init(Res1* r, va_list args)
   {
       r->flags = va_arg(args, int);
   }

   if the caller of h_alloc passed additional args, they are available
   in args. if init references more args than were passed, big trouble.
   however, this is a bug in your code, and cannot be triggered
   maliciously. only your code knows the resource type, and it is the
   only call site of h_alloc.
   there is no provision for indicating failure. if one-time init fails
   (rare, but one example might be failure to allocate memory that is
   for the lifetime of the resource, instead of in reload), it will
   have to set the control block state such that reload will fail.

   --

   reload:
   does all initialization of the resource that requires its source file.
   called after init; also after dtor every time the file is reloaded.

   static int Type_reload(Res1* r, const char* filename, Handle);
   {
       // somehow load stuff from filename, and store it in r->data1.
       return 0;
   }

   reload must abort if the control block data indicates the resource
   has already been loaded! example: if texture's reload is called first,
   it loads itself from file (triggering file.reload); afterwards,
   file.reload will be called again. we can't avoid this, because the
   handle manager doesn't know anything about dependencies
   (here, texture -> file).
   return value: 0 if successful (includes 'already loaded'),
   negative error code otherwise. if this fails, the resource is freed
   (=> dtor is called!).

   note that any subsequent changes to the resource state must be
   stored in the control block and 'replayed' when reloading.
   example: when uploading a texture, store the upload parameters
   (filter, internal format); when reloading, upload again accordingly.

   --

   dtor:
   frees all data allocated by init and reload. called after h_free,
   or at exit. control block will be zeroed afterwards.

   static void Type_dtor (Res1* r);
   {
       // free memory r->data1
   }

   again no provision for reporting errors - there's no one to act on it
   if called at exit. you can assert or log the error, though.

5) provide your layer on top of the handle manager:
   Handle res1_load(const char* filename, int my_flags)
   {
       return h_alloc(H_Res1, filename, 0, my_flags);	// my_flags is passed to init
   }

   int res1_free(Handle& h)
   {
       return h_free(h, H_Res1);
       // zeroes h afterwards
   }

   (this layer allows a res_load interface on top of all the loaders,
   and is necessary because your module is the only one that knows H_Res1).

6) done. the resource will be freed at exit (if not done already).

   here's how to access the control block, given a handle:
   Handle h;
   a) H_DEREF(h, Res1, r);

      creates a variable r of type Res1*, which points to the control block
      of the resource referenced by h. returns "invalid handle"
      (a negative error code) on failure.
   b) Res1* r = h_user_data(h, H_Res1);
      if(!r)
          ; // bail

      useful if H_DEREF's error return (of type signed integer) isn't
      acceptable. otherwise, prefer a) - this is pretty clunky, and
      we could switch H_DEREF to throwing an exception on error.

*/

