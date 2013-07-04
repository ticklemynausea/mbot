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

#include "Channel.h"

Channel::Channel (c_char channel, c_char ckey, NetServer *server) :
  name (channel, CHANNEL_SIZE), topic (TOPIC_SIZE), kick_nick (NICK_SIZE),
  last_deleted_nick (NICK_SIZE), key (ckey, CHANNEL_KEY_SIZE)
{
  s = server;
  last_msg[0] = 0;
  last_mask[0] = 0;
  msg_num = 0;
  mask_num = 0;
  warn_num = 0;
  joined = 0;
  for (int i = 0; i < C_USERS_MAX; i++)
    users[i] = NULL;
  user_num = 0;
}

Channel::~Channel (void)
{
  for (int i = 0; i < user_num; i++)		// delete all users
    delete users[i];
}

void
Channel::irc_join (void)
{
  s->writeln ("JOIN %s %s", (c_char)name, key ? (c_char)key : "");
}

void
Channel::irc_part (void)
{
  s->writeln ("PART %s", (c_char)name);
}

void
Channel::irc_topic (c_char t)
{
  s->writeln ("TOPIC %s :%s", (c_char)name, t);
}

void
Channel::irc_invite (c_char nick)
{
  s->writeln ("INVITE %s %s", nick, (c_char)name);
}

// if <reason> is NULL, use a random reason
void
Channel::irc_kick (c_char nick, c_char reason)
{
  s->writeln ("KICK %s %s :%s", (c_char)name, nick,
              reason != NULL ? reason : kicks[random_num (kicks_num)]);
}

void
Channel::irc_mode (c_char mode, c_char data)
{
  s->writeln ("MODE %s %s %s", (c_char)name, mode, data);
}

void
Channel::irc_op (c_char nick)
{
  irc_mode ("+o", nick);
}

void
Channel::irc_deop (c_char nick)
{
  irc_mode ("-o", nick);
}

void
Channel::irc_voice (c_char nick)
{
  irc_mode ("+v", nick);
}

void
Channel::irc_devoice (c_char nick)
{
  irc_mode ("-v", nick);
}

void
Channel::irc_deopban (c_char nick, c_char mask)
{
  char buf[MSG_SIZE+1];
  snprintf (buf, MSG_SIZE, "%s %s", nick, mask);
  irc_mode ("-o+b", buf);
}

void
Channel::irc_ban_nick (c_char nick)
{
  int i = user_index (nick);
  if (i != -1)
    {
      char buf[MSG_SIZE+1];
      make_generic_mask (users[i]->mask, buf);
      irc_ban_mask (buf);
    }
}

void
Channel::irc_ban_mask (c_char mask)
{
  irc_mode ("+b", mask);
}

void
Channel::irc_unban_nick (c_char nick)
{
  int i = user_index (nick);
  if (i != -1)
    {
      char buf[MSG_SIZE+1];
      make_generic_mask (users[i]->mask, buf);
      irc_unban_mask (buf);
    }
}

void
Channel::irc_unban_mask (c_char mask)
{
  irc_mode ("-b", mask);
}

// return the nick's position on the list, -1 if nonexistant
int
Channel::user_index (c_char nick)
{
  for (int i = 0; i < user_num; i++)
    if (users[i]->nick |= nick)
      return i;
  return -1;
}

// add a user to the list, ignore if the nick already exists
void
Channel::user_add (c_char mask, bool op, bool voice)
{
  char buf[NICK_SIZE+1];
  mask2nick (mask, buf);
  if (user_index (buf) == -1)				// if nonexistant
    {
      if (user_num == C_USERS_MAX)
        {
          s->write_botlog ("ERROR: reached maximum number of users inside a channel");
          return;
        }
      users[user_num] = new user_type;			// add it to the end
      users[user_num]->nick = buf;
      users[user_num]->mask = mask;
      users[user_num]->is_op = op;
      users[user_num]->is_voice = voice;
      user_num++;
    }
}

void
Channel::user_del (c_char nick)
{
  int i = user_index (nick);
  if (i != -1)						// if exists
    {
      delete users[i];					// release memory
      for (int i2 = i; i2 < user_num; i2++)		// delete from the list
        users[i2] = users[i2+1];
      user_num--;
      last_deleted_nick = nick;
    }
}

void
Channel::user_change_op (c_char nick, bool op)
{
  int i = user_index (nick);
  if (i != -1)
    users[i]->is_op = op;
}

void
Channel::user_change_voice (c_char nick, bool voice)
{
  int i = user_index (nick);
  if (i != -1)
    users[i]->is_voice = voice;
}

void
Channel::user_change_nick (c_char nick, c_char new_mask)
{
  int i = user_index (nick);
  if (i != -1)
    {
      users[i]->mask = new_mask;
      char buf[NICK_SIZE+1];
      mask2nick (new_mask, buf);
      users[i]->nick = buf;
      last_deleted_nick = nick;
    }
}

Channel::user_type *
Channel::user_get (c_char nick)
{
  int i = user_index (nick);
  if (i == -1)
    return NULL;
  return users[i];
}
