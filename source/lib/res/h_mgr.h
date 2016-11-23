/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * handle manager for resources.
 */

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
of this type. we will call ours "Res1"; all below occurrences of this
must be replaced with the actual name (exact spelling).
why? the vtbl builder defines its functions as e.g. Res1_reload;
your actual definition must match.

2) declare its control block:
struct Res1
{
	void* data;		// data loaded from file
	size_t flags;		// set when resource is created
};

Note that all control blocks are stored in fixed-size slots
(HDATA_USER_SIZE bytes), so squeezing the size of your data doesn't
help unless yours is the largest.

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

static Status Type_reload(Res1* r, const VfsPath& pathname, Handle);
{
	// already loaded; done
	if(r->data)
		return 0;

	r->data = malloc(100);
	if(!r->data)
		WARN_RETURN(ERR::NO_MEM);
	// (read contents of <pathname> into r->data)
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
frees all data allocated by init and reload. called after reload has
indicated failure, before reloading a resource, after h_free,
or at exit (if the resource is still extant).
except when reloading, the control block will be zeroed afterwards.

static void Type_dtor(Res1* r);
{
	free(r->data);
}

again no provision for reporting errors - there's no one to act on it
if called at exit. you can ENSURE or log the error, though.

be careful to correctly handle the different cases in which this routine
can be called! some flags should persist across reloads (e.g. choices made
during resource init time that must remain valid), while everything else
*should be zeroed manually* (to behave correctly when reloading).
be advised that this interface may change; a "prepare for reload" method
or "compact/free extraneous resources" may be added.

--

validate:
makes sure the resource control block is in a valid state. returns 0 if
all is well, or a negative error code.
called automatically when the Handle is dereferenced or freed.

static Status Type_validate(const Res1* r);
{
	const int permissible_flags = 0x01;
	if(debug_IsPointerBogus(r->data))
		WARN_RETURN(ERR::_1);
	if(r->flags & ~permissible_flags)
		WARN_RETURN(ERR::_2);
	return 0;
}


5) provide your layer on top of the handle manager:
Handle res1_load(const VfsPath& pathname, int my_flags)
{
	// passes my_flags to init
	return h_alloc(H_Res1, pathname, 0, my_flags);
}

Status res1_free(Handle& h)
{
	// control block is automatically zeroed after this.
	return h_free(h, H_Res1);
}

(this layer allows a res_load interface on top of all the loaders,
and is necessary because your module is the only one that knows H_Res1).

6) done. the resource will be freed at exit (if not done already).

here's how to access the control block, given a <Handle h>:
a)
	H_DEREF(h, Res1, r);

creates a variable r of type Res1*, which points to the control block
of the resource referenced by h. returns "invalid handle"
(a negative error code) on failure.
b)
	Res1* r = h_user_data(h, H_Res1);
	if(!r)
		; // bail

useful if H_DEREF's error return (of type signed integer) isn't
acceptable. otherwise, prefer a) - this is pretty clunky, and
we could switch H_DEREF to throwing an exception on error.

*/

#ifndef INCLUDED_H_MGR
#define INCLUDED_H_MGR

// do not include from public header files!
// handle.h declares type Handle, and avoids making
// everything dependent on this (rather often updated) header.


#include <stdarg.h>		// type init routines get va_list of args

#ifndef INCLUDED_HANDLE
#include "handle.h"
#endif

#include "lib/file/vfs/vfs.h"

extern void h_mgr_init();
extern void h_mgr_shutdown();


// handle type (for 'type safety' - can't use a texture handle as a sound)

// registering extension for each module is bad - some may use many
// (e.g. texture - many formats).
// handle manager shouldn't know about handle types


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
	void (*init)(void* user, va_list);
	Status (*reload)(void* user, const PIVFS& vfs, const VfsPath& pathname, Handle);
	void (*dtor)(void* user);
	Status (*validate)(const void* user);
	Status (*to_string)(const void* user, wchar_t* buf);
	size_t user_size;
	const wchar_t* name;
};

typedef H_VTbl* H_Type;

#define H_TYPE_DEFINE(type)\
	/* forward decls */\
	static void type##_init(type*, va_list);\
	static Status type##_reload(type*, const PIVFS&, const VfsPath&, Handle);\
	static void type##_dtor(type*);\
	static Status type##_validate(const type*);\
	static Status type##_to_string(const type*, wchar_t* buf);\
	static H_VTbl V_##type =\
	{\
		(void (*)(void*, va_list))type##_init,\
		(Status (*)(void*, const PIVFS&, const VfsPath&, Handle))type##_reload,\
		(void (*)(void*))type##_dtor,\
		(Status (*)(const void*))type##_validate,\
		(Status (*)(const void*, wchar_t*))type##_to_string,\
		sizeof(type),	/* control block size */\
		WIDEN(#type)			/* name */\
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
//
// note: don't use STMT - var decl must be visible to "caller"
#define H_DEREF(h, type, var)\
	/* h already indicates an error - return immediately to pass back*/\
	/* that specific error, rather than only ERR::INVALID_HANDLE*/\
	if(h < 0)\
		WARN_RETURN((Status)h);\
	type* const var = H_USER_DATA(h, type);\
	if(!var)\
		WARN_RETURN(ERR::INVALID_HANDLE);


// all functions check the passed tag (part of the handle) and type against
// the internal values. if they differ, an error is returned.




// h_alloc flags
enum
{
	// alias for RES_TEMP scope. the handle will not be kept open.
	RES_NO_CACHE = 0x01,

	// not cached, and will never reuse a previous instance
	RES_UNIQUE = RES_NO_CACHE|0x10,

	// object is requesting it never be reloaded (e.g. because it's not
	// backed by a file)
	RES_DISALLOW_RELOAD = 0x20
};

const size_t H_STRING_LEN = 256;



// allocate a new handle.
// if key is 0, or a (key, type) handle doesn't exist,
//   some free entry is used.
// otherwise, a handle to the existing object is returned,
//   and HDATA.size != 0.
//// user_size is checked to make sure the user data fits in the handle data space.
// dtor is associated with type and called when the object is freed.
// handle data is initialized to 0; optionally, a pointer to it is returned.
extern Handle h_alloc(H_Type type, const PIVFS& vfs, const VfsPath& pathname, size_t flags = 0, ...);
extern Status h_free(Handle& h, H_Type type);


// Forcibly frees all handles of a specified type.
void h_mgr_free_type(const H_Type type);


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

extern VfsPath h_filename(Handle h);


extern Status h_reload(const PIVFS& vfs, const VfsPath& pathname);

// force the resource to be freed immediately, even if cached.
// tag is not checked - this allows the first Handle returned
// (whose tag will change after being 'freed', but remaining in memory)
// to later close the object.
// this is used when reinitializing the sound engine -
// at that point, all (cached) OpenAL resources must be freed.
extern Status h_force_free(Handle h, H_Type type);

// increment Handle <h>'s reference count.
// only meant to be used for objects that free a Handle in their dtor,
// so that they are copy-equivalent and can be stored in a STL container.
// do not use this to implement refcounting on top of the Handle scheme,
// e.g. loading a Handle once and then passing it around. instead, have each
// user load the resource; refcounting is done under the hood.
extern void h_add_ref(Handle h);

// retrieve the internal reference count or a negative error code.
// background: since h_alloc has no way of indicating whether it
// allocated a new handle or reused an existing one, counting references
// within resource control blocks is impossible. since that is sometimes
// necessary (always wrapping objects in Handles is excessive), we
// provide access to the internal reference count.
extern intptr_t h_get_refcnt(Handle h);

#endif	// #ifndef INCLUDED_H_MGR
