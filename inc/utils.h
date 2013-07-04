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

#ifndef _H_UTILS
#define _H_UTILS

#include "system.h"
#include "defines.h"
#include "missing.h"

EXPORT bool file_exists (c_char name);
EXPORT void *my_malloc (size_t size);
EXPORT void my_free (void *ptr);
EXPORT void strset (char **dest, c_char src, size_t maxlen);
EXPORT time_t get_time (void);
EXPORT tm get_localtime (void);
EXPORT char *get_asctime (time_t t, char *buf, size_t len);
EXPORT void string_time (time_t t, char *str);
EXPORT char *ltoa (long int n, char *buf, size_t len);
EXPORT char *itoa (int n, char *buf, size_t len);
EXPORT char *utoa (u_int n, char *buf, size_t len);
EXPORT void random_init (void);
EXPORT long int random_num (long int max);
EXPORT c_char random_salt (void);
EXPORT size_t num_spaces (c_char str);
EXPORT size_t num_notspaces (c_char str);
EXPORT void mask2nick (c_char mask, char *nick);
EXPORT void mask2user (c_char mask, char *user);
EXPORT void mask2host (c_char mask, char *host);
EXPORT void mask_split (c_char mask, char *nick, char *user, char *host);
EXPORT bool match_mask (c_char mask2, c_char mask1);
EXPORT void make_generic_mask (c_char mask, char *gen_mask);
EXPORT void strsplit (c_char src, char dest[][MSG_SIZE+1], u_char n);
EXPORT char *my_strncpy (char *dest, c_char src, size_t n);
EXPORT char *my_null_strncpy (char *dest, c_char src, size_t n);
EXPORT c_char fullpath2file (c_char path);
EXPORT char *strip_crlf (char *str);
EXPORT void upper (char *str);
EXPORT void lower (char *str);
EXPORT int my_close (int fd);
EXPORT bool daemonize (void);

class fstreamtmp : public fstream {
public:
  EXPORT fstreamtmp (void);
};

#endif
