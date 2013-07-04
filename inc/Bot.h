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

#ifndef _H_CBOT
#define _H_CBOT

class Bot;

#include "NetServer.h"
#include "Module.h"
#include "ListVars.h"
#include "Log.h"

class Bot {
public:
  Bot (void);
  ~Bot (void);

  Log *log_add (c_char);
  EXPORT Log *log_get (c_char);
  void log_delall (void);
  EXPORT void write_botlog (c_char, ...);
  Module *add_module (c_char);
  Module *get_module (c_char);
  bool del_module (c_char);
  void parse_cmd (int argc, char *argv[]);
  void check_pid (void);
  void write_pid (void);
  void server_info (void);
  EXPORT void conf_error (c_char);
  EXPORT void conf_warn (c_char);
  void check_server (NetServer *);
  void parse_conf (void);
  void work (void);

  NetServer *servers[SERVER_MAX];
  int server_num;

  List modules;

  Text conf;
  String conf_file;
  String pid_file;

  u_int line;		// to store the current configuration line
  char debug;		// if 1, doesn't go into bg and show read/writes

  time_t time_now;

private:

  struct log_type {
    String name;
    Log *l;
    log_type *next;
    log_type () : name (MSG_SIZE) {}
  } *log_list;

};

#endif	// _H_BOT
