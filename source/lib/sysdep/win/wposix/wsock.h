/**
 * =========================================================================
 * File        : wsock.h
 * Project     : 0 A.D.
 * Description : emulate Berkeley sockets on Windows.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2004 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef INCLUDED_WSOCK
#define INCLUDED_WSOCK

#define IMP(ret, name, param) EXTERN_C __declspec(dllimport) ret __stdcall name param;


//
// <unistd.h>
//

IMP(int, gethostname, (char*, int))


//
// <sys/socket.h>
//

typedef unsigned long socklen_t;
typedef unsigned short sa_family_t;

// Win32 values - do not change
#define SOCK_STREAM 1
#define SOCK_DGRAM 2
#define AF_INET 2
#define PF_INET AF_INET
#define AF_INET6        23
#define PF_INET6 AF_INET6

#define SOL_SOCKET      0xFFFF          /* options for socket level */
#define TCP_NODELAY		0x0001

/* This is the slightly unreadable encoded form of the windows ioctl that sets
non-blocking mode for a socket */
#define FIONBIO     0x8004667E

enum {
	SHUT_RD=0,
	SHUT_WR=1,
	SHUT_RDWR=2
};

struct sockaddr;

IMP(int, socket, (int, int, int))
IMP(int, setsockopt, (int, int, int, const void*, socklen_t))
IMP(int, getsockopt, (int, int, int, void*, socklen_t*))
IMP(int, ioctlsocket, (int, int, const void *))
IMP(int, shutdown, (int, int))
IMP(int, closesocket, (int))


//
// <netinet/in.h>
//

typedef unsigned long in_addr_t;
typedef unsigned short in_port_t;

struct in_addr
{
	in_addr_t s_addr;
};

struct sockaddr_in
{
	sa_family_t    sin_family;
	in_port_t      sin_port;
	struct in_addr sin_addr;
	unsigned char  sin_zero[8];
};

#define INET_ADDRSTRLEN 16

#define INADDR_ANY 0
#define INADDR_LOOPBACK 0x7f000001
#define INADDR_NONE ((in_addr_t)-1)

#define IPPROTO_IP 0
#define IP_ADD_MEMBERSHIP 5
#define IP_DROP_MEMBERSHIP 6

struct ip_mreq
{
	struct in_addr imr_multiaddr;   /* multicast group to join */
	struct in_addr imr_interface;   /* interface to join on    */
};


// ==== IPv6 ====


#define in6addr_any PS_in6addr_any
#define in6addr_loopback PS_in6addr_loopback

extern const struct in6_addr in6addr_any;        /* :: */
extern const struct in6_addr in6addr_loopback;   /* ::_1 */

#define IN6ADDR_ANY_INIT { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } }
#define IN6ADDR_LOOPBACK_INIT { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } }
	// struct of array => 2 braces.

struct in6_addr
{
	unsigned char s6_addr[16];
};

struct sockaddr_in6 {
	sa_family_t			sin6_family;     /* AF_INET6 */
	in_port_t			sin6_port;       /* Transport level port number */
	unsigned long		sin6_flowinfo;   /* IPv6 flow information */
	struct in6_addr		sin6_addr;       /* IPv6 address */
	unsigned long		sin6_scope_id;   /* set of interfaces for a scope */
};


//
// <netdb.h>
//

struct hostent
{
	char* h_name;       // Official name of the host. 
	char** h_aliases;   // A pointer to an array of pointers to 
	                    // alternative host names, terminated by a
	                    // null pointer. 
	short h_addrtype;   // Address type. 
	short h_length;     // The length, in bytes, of the address. 
	char** h_addr_list; // A pointer to an array of pointers to network 
	                    // addresses (in network byte order) for the host, 
	                    // terminated by a null pointer. 
};

IMP(struct hostent*, gethostbyname, (const char *name))

#define h_error WSAGetLastError()
#define HOST_NOT_FOUND 11001
#define TRY_AGAIN 11002


// addrinfo struct */
struct addrinfo
{
	int ai_flags;              // AI_PASSIVE, AI_CANONNAME, AI_NUMERICHOST
	int ai_family;             // PF_xxx
	int ai_socktype;           // SOCK_xxx
	int ai_protocol;           // 0 or IPPROTO_xxx for IPv4 and IPv6
	size_t ai_addrlen;         // Length of ai_addr
	char *ai_canonname;        // Canonical name for nodename
	struct sockaddr *ai_addr;  // Binary address
	struct addrinfo *ai_next;  // Next structure in linked list
};

// Hint flags for getaddrinfo
#define AI_PASSIVE     0x1     // Socket address will be used in bind() call

// Flags for getnameinfo()
#define NI_NUMERICHOST  0x02   // Return numeric form of the host's address

#define NI_MAXHOST 1025
#define NI_MAXSERV 32

	// Note that these are function pointers. They will be initialized by the
	// wsock_init in wsock.cpp
	typedef int (__stdcall *fp_getnameinfo_t)(const struct sockaddr *sa, socklen_t salen, char *node,
									socklen_t nodelen, char *serv, socklen_t servlen, unsigned int flags);
	typedef int (__stdcall *fp_getaddrinfo_t)(const char	*nodename, const char *servname,
									const struct addrinfo *hints, struct addrinfo **res);
	typedef void (__stdcall *fp_freeaddrinfo_t)(struct addrinfo *ai);

	extern fp_getnameinfo_t import_getnameinfo();
	extern fp_getaddrinfo_t import_getaddrinfo();
	extern fp_freeaddrinfo_t import_freeaddrinfo();

	#define getnameinfo (import_getnameinfo())
	#define getaddrinfo (import_getaddrinfo())
	#define freeaddrinfo (import_freeaddrinfo())



// getaddr/nameinfo error codes
#define EAI_NONAME HOST_NOT_FOUND



//
// <arpa/inet.h>
//

extern uint16_t htons(uint16_t hostshort);
#define ntohs htons
IMP(unsigned long, htonl, (unsigned long hostlong))


IMP(in_addr_t, inet_addr, (const char*))
IMP(char*, inet_ntoa, (in_addr))
IMP(int, accept, (int, struct sockaddr*, socklen_t*))
IMP(int, bind, (int, const struct sockaddr*, socklen_t))
IMP(int, connect, (int, const struct sockaddr*, socklen_t))
IMP(int, listen, (int, int))
IMP(ssize_t, recv, (int, void*, size_t, int))
IMP(ssize_t, send, (int, const void*, size_t, int))
IMP(ssize_t, sendto, (int, const void*, size_t, int, const struct sockaddr*, socklen_t))
IMP(ssize_t, recvfrom, (int, void*, size_t, int, struct sockaddr*, socklen_t*))


// WSAAsyncSelect event bits
// (values taken from winsock2.h - do not change!)
#define FD_READ    BIT(0)
#define FD_WRITE   BIT(1)
#define FD_ACCEPT  BIT(3)
#define FD_CONNECT BIT(4)
#define FD_CLOSE   BIT(5)


#undef IMP

#endif	// #ifndef INCLUDED_WSOCK
