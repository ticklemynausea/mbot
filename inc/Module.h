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

#ifndef _H_MODULE
#define _H_MODULE

class Module;

#include "Bot.h"

class Module {
public:

  Module (c_char);
  ~Module (void);

  EXPORT void *getsym (c_char) const;
  EXPORT c_char geterror (void) const;
  void conf (NetServer *, c_char) const;
  EXPORT operator c_char (void) const;

  struct module_type {
    u_char version;			// module version
    c_char name;			// module name
    void (*start) (void);		// executed when loading the module
    void (*stop) (void);		// executed when unloading the module
    void (*conf) (NetServer *, c_char);// configuration parser
    Module *mod;		// filled by us to be used by the module
  };

  bool initialized;			// if the constructor was succesful
  Bot *b;

private:

  String filename;			// path, as provided to Module()
  struct module_type *module;
#ifdef WINDOZE
  HINSTANCE handle;			// handle returned by LoadLibrary()
#else		// !WINDOZE
  void *handle;				// handle returned by dlopen()
#endif		// !WINDOZE

};

#endif	// _H_MODULE
