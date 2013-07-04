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

#ifndef _H_NETDCC
#define _H_NETDCC

class NetDCC;

#include "mbot.h"

enum dcc_type {
  DCC_STOP,			// nothing set, exit
  DCC_SEND_INIT,		// socket and file open for dcc send
  DCC_SEND,			// dcc send active
  DCC_CHAT_INIT,		// open socket for dcc chat
  DCC_CHAT_AUTH,		// ask password in chat
  DCC_CHAT			// dcc chat active
};

class NetDCC : public Net {
public:
  NetDCC (NetServer *, const char *, int, int);
  ~NetDCC (void);

  void dcc_error (const char *);
  bool dcc_chat_start (void);
  bool dcc_send_start (const char *);
  void dcc_start_check (void);
  void dcc_chat_write (const char *);
  void dcc_chat_pass (void);
  void dcc_chat (void);
  void dcc_send (void);
  void dcc_stop (void);
  bool dcc_work (void);
  char *dcc_make_ctcp_chat (char *, size_t);
  char *dcc_make_ctcp_send (char *, size_t);

  char nick[NICK_SIZE+1];		// nick from the given mask
  String mask;				// mask of who made the dcc
  dcc_type status;			// dcc status
  int index,				// this dcc's table index
      dcc_from_index;	// index of dcc where _this_ dcc was started
  			// -1 if it was on a pvt

private:

  NetServer *s;			// server to which belongs

  FILE *dfd;
  int filesize,				// total file size
      filesent,				// number of bytes sent
      socklocal;
  time_t last_time;			// time of last change
  String filename;			// name of file to send
  char dcc_buf[DCC_BUF_SIZE+1];
  char buf[BUF_SIZE+1];

};

#endif	// _H_NETDCC
