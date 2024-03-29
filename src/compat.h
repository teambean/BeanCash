// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2015-2022 Bean Core www.beancash.org
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _BITBEAN_COMPAT_H
#define _BITBEAN_COMPAT_H

#ifdef WIN32
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0501
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifdef FD_SETSIZE
#undef FD_SETSIZE // prevent redefinition compiler warning
#endif
#define FD_SETSIZE 4096 // Max number of File Descriptors for Windows

#include <winsock2.h>  // Must be included before mswsock.h and windows.h

#include <mswsock.h>
#include <windows.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>

#include <ifaddrs.h>
#include <limits.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>  // Use sockaddr_in (POSIX.1-2001)

#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef WIN32
#define MSG_NOSIGNAL        0
#define MSG_DONTWAIT        0
#else
typedef u_int SOCKET;
#include "errno.h"
#define WSAGetLastError()   errno
#define WSAEINVAL           EINVAL
#define WSAEALREADY         EALREADY
#define WSAEWOULDBLOCK      EWOULDBLOCK
#define WSAEMSGSIZE         EMSGSIZE
#define WSAEINTR            EINTR
#define WSAEINPROGRESS      EINPROGRESS
#define WSAEADDRINUSE       EADDRINUSE
#define WSAENOTSOCK         EBADF
#define INVALID_SOCKET      (SOCKET)(~0)
#define SOCKET_ERROR        -1
#endif

#endif
