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

#ifndef _H_STRING
#define _H_STRING

#include "utils.h"

class String {
protected:
  char *str;
  u_short len, maxlen;

public:
  EXPORT String (void);
  EXPORT String (u_short);
  EXPORT String (c_char, u_short);
  EXPORT virtual ~String (void);

  EXPORT virtual void operator = (const String &);
  EXPORT virtual void operator = (c_char);
  EXPORT virtual void operator += (c_char);
  EXPORT friend ostream &operator << (ostream &, const String &);
  EXPORT bool operator == (const String &) const;
  EXPORT bool operator == (c_char) const;
  EXPORT bool operator |= (const String &) const;
  EXPORT bool operator |= (c_char) const;
  EXPORT bool operator != (c_char) const;
  EXPORT operator bool (void) const { return str != NULL; }
  EXPORT operator c_char (void) const { return str; }

  EXPORT char *getstr (void) const { return str; }	// use only when absolutely necessary
  EXPORT u_short getlen (void) const { return len; }
  EXPORT void strip_crlf (void);
  EXPORT void lower (void);

};

class StringFixed : public String {
public:
  EXPORT StringFixed (u_short);

  EXPORT void operator = (const String &);
  EXPORT void operator = (c_char);
  EXPORT void operator += (c_char);
};

#endif	// _H_STRING
