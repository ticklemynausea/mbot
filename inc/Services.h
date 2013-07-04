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

#ifndef _H_SERVICES
#define _H_SERVICES

#include "Strings.h"
#include "defines.h"

class NetServer;

class Services {
public:
  Services (NetServer *);

  void irc_services (c_char, c_char) const;

  void irc_nickserv (c_char) const;
  void nick_identify (void);

  void irc_chanserv (c_char) const;
  void chan_invite (c_char);
  void chan_unban (c_char);
  void chan_op (c_char, c_char);
  void chan_deop (c_char, c_char);

  bool exist,				// if there are irc services
       identified,			// if it's identified
       services_privmsg;		// if they use privmsg

  String nickserv_pass,			// nickserv password
         nickserv_mask,			// nickserv's mask
         nickserv_auth;			// start of string asking to identify


private:

  NetServer *s;			// server to which belongs
  char buf[MSG_SIZE+1];

};

#include "mbot.h"

#endif	// _H_SERVICES
