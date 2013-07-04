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

#ifndef _H_NETSERVER
#define _H_NETSERVER

class NetServer;

#include "mbot.h"
#include "Text.h"
#include "Channel.h"
#include "ListVars.h"
#include "ListUsers.h"
#include "Net.h"
#include "NetDCC.h"
#include "Services.h"
#include "Script.h"

class NetServer : public Net {
public:
  NetServer (void);
  ~NetServer (void);

  EXPORT void irc_pong (c_char);
  EXPORT void irc_quit (c_char);
  EXPORT void irc_privmsg (c_char, c_char, ...);
  EXPORT void irc_notice (c_char, c_char, ...);
  EXPORT void irc_pass (c_char);
  EXPORT void irc_nick (c_char);
  EXPORT void irc_user (c_char, c_char);
  EXPORT void irc_whois (c_char);
  EXPORT void irc_ison (c_char);
  EXPORT void irc_watch (c_char, bool);
  EXPORT void irc_who (c_char);
  EXPORT void irc_oper (c_char, c_char);
  EXPORT void irc_kill (c_char, c_char);
  EXPORT void irc_away (c_char);

  EXPORT void write_botlog (c_char, ...);

  void irc_connect (void);
  void irc_restart (void);
  void work (void);

  bool channel_add (c_char, c_char);
  bool channel_del (c_char);
  EXPORT int channel_index (c_char);
  EXPORT Channel *channel_get (c_char);

  void irc_parse (void);

  // circular list of hosts
  struct host_type {
    String host;
    int port;
    String password;
    host_type *next;
    host_type (c_char h, int p, c_char pw) : host (h, HOST_SIZE),
      port (p), password (pw, SERVER_PASS_SIZE), next (this) {}
    host_type (c_char h, int p, c_char pw, host_type *n) :
      host (h, HOST_SIZE), port (p), password (pw, SERVER_PASS_SIZE),
      next (n) {}
  } *hosts,				// hosts list
    *host_current;			// current host, points to one of them

  host_type *host_add (host_type *, c_char, int, c_char);

  String virtualhost,
         nick,
         nick_orig,
         user,
         name,
         away,
         quit;

  time_t last_try; 	// last try to connect to the server

  int change_time;	// time (in seconds) to be added to local time
  time_t time_now;	// current local time, with change_time applied

  ListUsers users;				// registered users

  Channel *channels[CHANNEL_MAX];		// channels
  int channel_num;				// channels number

  NetDCC *dccs[DCC_MAX];			// dccs
  int dcc_num,					// dccs number
      dcc_port;					// port for dccs, 0 to any free
  String dcc_file,				// file with !get definitions
         dcc_motdfile;				// file with dcc chat's motd
  Text *dcc_motd;				// opened motd or NULL

  ListVars vars;				// server variables
  Services services;				// nick/chan/oper/memoserv etc
  Script script;				// events and replies handler
  int reply,					// atoi() of server reply
      event;					// #define of server event
  
  char cmd[CMD_SIZE][MSG_SIZE+1];		// parsed message
  int cmd_i;					// components
  char buf[CMD_SIZE*(MSG_SIZE+1)];
  time_t uptime;				// when the bot connected

  List works;
  Bot *b;

private:

};

#endif	// _H_NETSERVER
