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

#ifndef _H_LISTVARS
#define _H_LISTVARS

#include "List.h"
#include "Strings.h"
#include "defines.h"

class NetServer;

class ListVars : public List {
public:

  ListVars (NetServer *);
  ~ListVars (void);

  struct var_type {
    String name;
    void (*handler)(NetServer *, c_char, char *, size_t);
    var_type (c_char n, void (*h)(NetServer *, c_char, char *, size_t)) :
      name (n, VAR_SIZE), handler (h) {}
  };

  EXPORT bool var_add (c_char, void (*)(NetServer *, c_char, char *, size_t));
  EXPORT bool var_del (c_char);
  EXPORT bool var_get (c_char, char *, size_t);
  EXPORT bool var_set (c_char, char *);

private:

  struct var_type *var_exist (c_char);
  void var_delall (void);

  NetServer *s;

};

#include "mbot.h"

#endif	// _H_LISTVARS
