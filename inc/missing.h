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

#ifndef _H_MISSING
#define _H_MISSING

#include "system.h"
#include "defines.h"
#include "utils.h"

#ifndef HAVE_GETOPT

extern char *optarg;
extern int optind;
extern int opterr;
extern int optopt;

struct option
{
  const char *name;
  int has_arg;
  int *flag;
  int val;
};

#define	no_argument		0
#define required_argument	1
#define optional_argument	2

int _getopt_internal (int argc, char *const *argv,
			     const char *shortopts,
		             const struct option *longopts, int *longind,
			     int long_only);

EXPORT int getopt (int argc, char *const *argv, const char *shortopts);
EXPORT int getopt (int, char *const *, const char *);
EXPORT int getopt_long (int argc, char *const *argv, const char *shortopts,
		        const struct option *longopts, int *longind);
EXPORT int getopt_long_only (int argc, char *const *argv,
			     const char *shortopts,
		             const struct option *longopts, int *longind);

#endif	// !HAVE_GETOPT

#ifndef HAVE_STRCASECMP
EXPORT int strcasecmp (c_char, c_char);
#endif	// !HAVE_STRCASECMP

#ifndef HAVE_STRNCASECMP
EXPORT int strncasecmp (c_char, c_char, size_t);
#endif	// !HAVE_STRNCASECMP

#ifndef HAVE_MEMCPY
EXPORT void *memcpy (void *, const void *, size_t);
#endif	// !HAVE_MEMCPY

#ifndef MBOT_CRYPT
EXPORT char *crypt (c_char, c_char);
#endif	// !MBOT_CRYPT

#ifndef HAVE_VSNPRINTF
EXPORT int vsnprintf (char *str, size_t size, const char *format, va_list ap);
#endif	// !HAVE_VSNPRINTF

#ifndef HAVE_SNPRINTF
EXPORT int snprintf (char *str, size_t size, const char *format, ...);
#endif	// !HAVE_SNPRINTF

#ifndef HAVE_GETPID
EXPORT pid_t getpid (void);
#endif	// !HAVE_GETPID

#ifndef HAVE_TRUNCATE
EXPORT int truncate (c_char, long);
#endif	// !HAVE_TRUNCATE

#ifndef HAVE_STRSEP
EXPORT char *strsep (char **, c_char);
#endif	// !HAVE_STRSEP

#endif	// _H_MISSING
