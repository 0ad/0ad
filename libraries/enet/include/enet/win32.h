/** 
 @file  win32.h
 @brief ENet Win32 header
*/
#ifndef __ENET_WIN32_H__
#define __ENET_WIN32_H__

#ifdef ENET_BUILDING_LIB
#pragma warning (disable: 4996) // 'strncpy' was declared deprecated
#pragma warning (disable: 4267) // size_t to int conversion
#pragma warning (disable: 4244) // 64bit to 32bit int
#pragma warning (disable: 4018) // signed/unsigned mismatch
#endif

#include <stdlib.h>

#if 0
#include <winsock2.h>
#else
// When this header is included in Pyrogenesis, it needs to avoid pulling in the
// standards Win32 headers (else there are conflicts), but we need to define a
// couple of Win32-specific socket values so that ENet still compiles
typedef uintptr_t        SOCKET;
#define INVALID_SOCKET  (SOCKET)(~0)
#endif

typedef SOCKET ENetSocket;

enum
{
    ENET_SOCKET_NULL = INVALID_SOCKET
};

#define ENET_HOST_TO_NET_16(value) (htons (value))
#define ENET_HOST_TO_NET_32(value) (htonl (value))

#define ENET_NET_TO_HOST_16(value) (ntohs (value))
#define ENET_NET_TO_HOST_32(value) (ntohl (value))

typedef struct
{
    size_t dataLength;
    void * data;
} ENetBuffer;

#define ENET_CALLBACK __cdecl

#if defined ENET_DLL
#if defined ENET_BUILDING_LIB
#define ENET_API __declspec( dllexport )
#else
#define ENET_API __declspec( dllimport )
#endif /* ENET_BUILDING_LIB */
#else /* !ENET_DLL */
#define ENET_API extern
#endif /* ENET_DLL */

#endif /* __ENET_WIN32_H__ */


