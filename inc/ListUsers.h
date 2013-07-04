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

#ifndef _H_LISTUSERS
#define _H_LISTUSERS

#include "List.h"
#include "Strings.h"
#include "defines.h"

class NetServer;

class ListUsers : public List {
public:
  ListUsers (NetServer *);
  ~ListUsers (void);

  struct user_type {
    String mask;			// mask (nick!user@host)
    String cur_mask;			// current mask, complete
    String pass;			// encrypted password
    short int level;			// user level
    bool id;				// if it's identified
    String msg;			// join msg
    user_type (c_char m, c_char p, short int l, c_char ms) :
      mask (m, MASK_SIZE), cur_mask (MASK_SIZE), pass (p, ENC_PASS_SIZE),
      level (l), id (0), msg (ms, MSG_SIZE) {}
  };

  EXPORT bool load_users (c_char);
  EXPORT bool load_users (void);
  EXPORT bool save_users (void);
  EXPORT bool add_user (c_char, c_char, int, c_char);
  EXPORT bool del_user (c_char);
  EXPORT bool match_check_pass (c_char, c_char);
  EXPORT bool abs_check_pass (c_char, c_char);
  EXPORT bool match_set_pass (c_char, c_char);
  EXPORT bool abs_set_pass (c_char, c_char);
  EXPORT bool match_set_msg (c_char, c_char);
  EXPORT bool abs_set_msg (c_char, c_char);
  EXPORT bool abs_set_mask (c_char, c_char);
  EXPORT bool match_set_level (c_char, int);
  EXPORT bool abs_set_level (c_char, int);
  EXPORT int abs_user_index(c_char);
  EXPORT bool match_set_id (c_char, bool);
  EXPORT user_type *match_mask2user (c_char);
  EXPORT user_type *abs_mask2user (c_char);
  EXPORT int match_mask_level (c_char);
  EXPORT int match_mask_reallevel (c_char);

  String filename;

private:

  void addlist_user (c_char, c_char, int, c_char);
  bool dellist_user (c_char);

  NetServer* s;			// server to which belongs

};

#include "mbot.h"

#endif	// _H_LISTUSERS
