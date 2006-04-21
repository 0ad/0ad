#ifndef SHAREDMEMORY_H__
#define SHAREDMEMORY_H__

// We want to use placement new, which breaks when compiling Debug configurations
// in the game and in wx, and they both need different workarounds.
// (Duplicated in Shareable.h)
#ifdef new
# define SHAREABLE_USED_NOMMGR
# ifdef __WXWINDOWS__
#  undef new
# else
#  include "nommgr.h"
# endif
#endif

namespace AtlasMessage
{

// Shared pointers need to be allocated and freed from the same heap.
// So, both sides of the Shareable interface should set these function pointers
// to point to the same function. (The game will have to dynamically load them
// from the DLL.)
extern void* (*ShareableMallocFptr) (size_t n);
extern void (*ShareableFreeFptr) (void* p);

// Implement shared new/delete on top of those
template<typename T> T* ShareableNew()
{
	T* p = (T*)ShareableMallocFptr(sizeof(T));
	new (p) T;
	return p;
}
template<typename T> void ShareableDelete(T* p)
{
	p->~T();
	ShareableFreeFptr(p);
}
// Or maybe we want to use a non-default constructor
#define SHAREABLE_NEW(T, data) (new ( (T*)AtlasMessage::ShareableMallocFptr(sizeof(T)) ) T data)

}


#ifdef SHAREABLE_USED_NOMMGR
// # ifdef __WXWINDOWS__ // TODO: portability to non-Windows wx
// #  define new  new( _NORMAL_BLOCK, __FILE__, __LINE__)
// # else
// #  include "mmgr.h"
// # endif
// Actually, we don't want to redefine 'new', because it conflicts with all users
// of SHAREABLE_NEW. So just leave it undefined, and put up with the less
// informative leak messages.
# undef SHAREABLE_USED_NOMMGR
// Oh, but we don't want other game headers to include mmgr.h again.
// So let's just cheat horribly and remove the options which cause it to
// redefine new.
# undef  HAVE_VC_DEBUG_ALLOC
# define HAVE_VC_DEBUG_ALLOC 0
# undef  CONFIG_USE_MMGR
# define CONFIG_USE_MMGR 0
#endif


#endif // SHAREDMEMORY_H__