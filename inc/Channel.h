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

#ifndef _H_CHANNEL
#define _H_CHANNEL

class Channel;

#include "mbot.h"

class Channel {
public:
  Channel (c_char, c_char, NetServer *);
  ~Channel (void);

  void irc_join (void);
  void irc_part (void);
  void irc_topic (c_char);
  EXPORT void irc_invite (c_char);
  EXPORT void irc_kick (c_char, c_char);
  void irc_mode (c_char, c_char);
  void irc_op (c_char);
  void irc_deop (c_char);
  void irc_voice (c_char);
  void irc_devoice (c_char);
  void irc_deopban (c_char, c_char);
  void irc_ban_nick (c_char);
  void irc_ban_mask (c_char);
  void irc_unban_nick (c_char);
  void irc_unban_mask (c_char);
  
  EXPORT int user_index (c_char);
  void user_add (c_char, bool, bool);
  void user_del (c_char);
  void user_change_op (c_char, bool);
  void user_change_voice (c_char, bool);
  void user_change_nick (c_char, c_char);

  String name;				// channel name
  String topic;				// channel topic

  char last_msg[MSG_SIZE+1];		// last privmsg
  char last_mask[MASK_SIZE+1];		// mask who said it
  u_int msg_num;			// num of equal msg/mask in a row
  u_int mask_num;			// num of equal masks in a row
  u_int warn_num;	// num of msgs, after the others, to kick
			// if !0, send flood_kick_msg to pvt on the 1st time
  bool joined;				// if it's inside the channel

  String kick_nick;			// to kick who kicked it
  String last_deleted_nick;		// mainly to log quit and nick events

  struct user_type {
    String nick;			// nick
    String mask;			// mask
    bool is_op;				// if has op
    bool is_voice;			// if has voice
    user_type () : nick (NICK_SIZE), mask (MASK_SIZE) {}
  } *users[C_USERS_MAX];		// users inside the channel
  int user_num;				// how many

  EXPORT user_type *user_get (c_char);

private:

  String key;				// channel key
  NetServer *s;				// server to which belongs

};

#endif	// _H_CHANNEL
