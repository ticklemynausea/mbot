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

#ifndef _H_NET
#define _H_NET

#include "defines.h"
#include "system.h"
#include "utils.h"

class Bot;

class Net {
public:
  EXPORT Net (int = -1);
  EXPORT ~Net (void);

  EXPORT ostream &operator << (c_char);

  EXPORT void writeln (c_char, ...);
  EXPORT int readln (void);

  struct addr_type {
    int inet;
#ifdef ENABLE_IPV6
    struct sockaddr_in6 sa6;
#endif
    struct sockaddr_in sa;
  } addr;

  EXPORT bool create_tcp (int, c_char vhost = NULL, bool block = 0);
  EXPORT bool openhost (void);
  EXPORT static int readok (int);
  EXPORT static int writeok (int);
  EXPORT void sock_linger (int, int);
  EXPORT bool bindsock (int);
  EXPORT void blocksock (bool);
  EXPORT static u_short sock2port (int sock);
  EXPORT static u_long sock2addr (int sock);
  EXPORT bool resolvehost (c_char name, struct addr_type *a, int inet = 0);
  EXPORT bool closesock (void);

  long unsigned int bytesout;
  long unsigned int bytesin;
  time_t time_read, time_write;
  char *bufread;
  int sock;
  bool connecting, connected;		// connect() helpers
  int bufpos;

};

#include "Strings.h"

class NetHttp : public Net {
public:
  NetHttp (c_char, c_char, int);

  int work (void);

  StringFixed html;
  String host;

private:

  String filename;
  int size, pos;
  bool reading_header;

};

#include "Bot.h"

#endif	// _H_NET
