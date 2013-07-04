/*

This file is part of mbot.

mbot is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

mbot is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with mbot; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef _H_SYSTEM
#define _H_SYSTEM

#ifdef _MSC_VER		// ms vc++

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <iostream.h>
#include <iomanip.h>
#include <fstream.h>
#include <strstrea.h>
#include <limits.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <winsock2.h>
#include <io.h>

#undef HELP_QUIT	// defined by winuser.h or whatever
#define WINDOZE
#define VERSION "unknown"
#define VERSION_STRING "mirage irc bot (win32)"
#define HOST "i386-pc-win32"
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define mktemp _mktemp
#define isnan _isnan
#define usleep Sleep
#define sleep(x) Sleep ((x)*1000)
#define EOL (c_char)0
#define HAS_IOS_BINARY 1
#define HAVE_MEMCPY 1
#define HAVE_VSNPRINTF 1
#define HAVE_SNPRINTF 1
#define HAVE_VSNPRINTF_T 1
#define HAVE_SNPRINTF_T 1
#define PATH_MAX 255
#define EISCONN WSAEISCONN
#define EWOULDBLOCK WSAEWOULDBLOCK
#define EINPROGRESS WSAEINPROGRESS
#define EALREADY WSAEALREADY
#define EXPORT __declspec(dllexport)
#define IOS_NOCREATE ios::nocreate
#define IOS_NOREPLACE ios::noreplace
#define HAVE_FSTREAM_ATTACH 1

typedef unsigned char u_char;
typedef const char * c_char;
typedef int socklen_t;
typedef unsigned int uint_fast32_t;
typedef unsigned int uint32_t;
typedef DWORD pid_t;

#else	// !_MSC_VER

#include "config.h"

typedef const char * c_char;
#define EOL (c_char)0		// for operator<<
#define EXPORT

#ifdef __CYGWIN__
# undef HAVE_GETOPT
# undef MBOT_CRYPT
# undef HAVE_CRYPT_H
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <sys/types.h>
#if HAVE_PWD_H
# include <pwd.h>
#endif
#if HAVE_GRP_H
# include <grp.h>
#endif

#if HAVE_IOSTREAM
# include <iostream>
#else
# include <iostream.h>
#endif
#if HAVE_IOMANIP
# include <iomanip>
#else
# include <iomanip.h>
#endif
#if HAVE_FSTREAM
# include <fstream>
#else
# include <fstream.h>
#endif
#if HAVE_OSTREAM
# include <ostream>
using namespace std;
#elif HAVE_OSTREAM_H
# include <ostream.h>
#endif
//#include <strstream>

#include <limits.h>
#include <math.h>

#include <sys/wait.h>
#include <errno.h>
#include <dlfcn.h>
#include <ctype.h>
#include <signal.h>

#if HAVE_UNISTD_H
# ifndef __USE_XOPEN
#  define __USE_XOPEN
# endif
# ifndef _XOPEN_SOURCE_EXTENDED
#  define _XOPEN_SOURCE_EXTENDED
# endif
# include <unistd.h>
// TRU64 madness
# define _Egetsockname getsockname
# define _Eaccept accept
# define _Epipe pipe
#endif

#if HAVE_SYS_INTTYPES_H
# include <sys/inttypes.h>		// netbsd & uint32_t
#endif

#if HAVE_SYS_BITYPES_H
# include <sys/bitypes.h>		// OSF1 & uint32_t
#endif

#if HAVE_STRING_H
# include <string.h>
#endif
#if HAVE_STRINGS_H
# include <strings.h>
#endif
#ifndef HAVE_MEMCPY
# define memcpy(d, s, n) bcopy ((s), (d), (n))
# define memmove(d, s, n) bcopy ((s), (d), (n))
#endif

#if HAVE_CRYPT_H
# include <crypt.h>
#endif

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#if HAVE_FCNTL_H
# include <fcntl.h>
#endif

#include <arpa/inet.h>
#include <netdb.h>

#if HAVE_NETINET_IN_SYSTM_H
# include <netinet/in_systm.h>
#endif

#include <netinet/in.h>
#include <netinet/ip.h>

#if HAVE_NETINET_TCP_H
# include <netinet/tcp.h>
#endif

#if HAVE_PTHREAD_H
# if HAVE_LIBPTHREAD
#  define THREADED 1
#  include <pthread.h>
# endif
#endif

#if HAS_IOS_NOCREATE
# define IOS_NOCREATE ios::nocreate
#else
# define IOS_NOCREATE ios::in	// i always use nocreate with ios:in, so it
#endif				//   should be ok

#if HAS_IOS_NOREPLACE
# define IOS_NOREPLACE ios::noreplace
#else
# define IOS_NOREPLACE ios::in	// i always use noreplace with ios:in, so it
#endif				//   should be ok

#include <cstdio>

#if HAVE_VSNPRINTF
# ifndef HAVE_VSNPRINTF_T
//int vsnprintf (char *str, size_t size, const char  *format, va_list ap);
#  undef HAVE_VSNPRINTF
# endif
#endif

#if HAVE_SNPRINTF
# ifndef HAVE_SNPRINTF_T
//int snprintf(char *str, size_t size, const  char  *format, ...);
#  undef HAVE_SNPRINTF
# endif
#endif

// broken on x86-64-g++ 3.4 (cvs)
#ifndef PATH_MAX
#define PATH_MAX 255
#endif

#endif	// !_MSC_VER

#endif

