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

#include "Script.h"

Script::Script (NetServer *server) : server_jumped (false), ctcp_version (CTCP_VERSION, MSG_SIZE),
  ctcp_finger (CTCP_FINGER, MSG_SIZE), ctcp_source (CTCP_SOURCE, MSG_SIZE),
  s (server)
{
  source[0] = 0;
  dest[0] = 0;
  cmd_bind (scriptcmd_modlist, LEVEL_MODLIST, "!modlist", NULL, HELP_MODLIST);
  cmd_bind (scriptcmd_modadd, LEVEL_MODADD, "!modadd", NULL, HELP_MODADD);
  cmd_bind (scriptcmd_moddel, LEVEL_MODDEL, "!moddel", NULL, HELP_MODDEL);
  cmd_bind (scriptcmd_quit, LEVEL_QUIT, "!quit", NULL, HELP_QUIT);
  cmd_bind (scriptcmd_server, LEVEL_SERVER, "!server", NULL, HELP_SERVER);
  cmd_bind (scriptcmd_set, LEVEL_SET, "!set", NULL, HELP_SET);
  cmd_bind (scriptcmd_loadconf, LEVEL_LOADCONF, "!loadconf", NULL, HELP_LOADCONF);
  cmd_bind (scriptcmd_away, LEVEL_AWAY, "!away", NULL, HELP_AWAY);
  cmd_bind (scriptcmd_nick, LEVEL_NICK, "!nick", NULL, HELP_NICK);
  cmd_bind (scriptcmd_unbind, LEVEL_UNBIND, "!unbind", NULL, HELP_UNBIND);
  cmd_bind (scriptcmd_cmdlevel, LEVEL_CMDLEVEL, "!cmdlevel", NULL, HELP_CMDLEVEL);
  cmd_bind (scriptcmd_join, LEVEL_JOIN, "!join", NULL, HELP_JOIN);
  cmd_bind (scriptcmd_part, LEVEL_PART, "!part", NULL, HELP_PART);
  cmd_bind (scriptcmd_status, LEVEL_STATUS, "!status", NULL, HELP_STATUS);
  cmd_bind (scriptcmd_topic, LEVEL_TOPIC, "!topic", NULL, HELP_TOPIC);
  cmd_bind (scriptcmd_opme, LEVEL_OPME, "!opme", NULL, HELP_OPME);
  cmd_bind (scriptcmd_op, LEVEL_OP, "!op", NULL, HELP_OP);
  cmd_bind (scriptcmd_deop, LEVEL_DEOP, "!deop", NULL, HELP_DEOP);
  cmd_bind (scriptcmd_kick, LEVEL_KICK, "!kick", NULL, HELP_KICK);
  cmd_bind (scriptcmd_ban, LEVEL_BAN, "!ban", NULL, HELP_BAN);
  cmd_bind (scriptcmd_unban, LEVEL_UNBAN, "!unban", NULL, HELP_UNBAN);
  cmd_bind (scriptcmd_bk, LEVEL_BK, "!bk", NULL, HELP_BK);
  cmd_bind (scriptcmd_invite, LEVEL_INVITE, "!invite", NULL, HELP_INVITE);
  cmd_bind (scriptcmd_voice, LEVEL_VOICE, "!voice", NULL, HELP_VOICE);
  cmd_bind (scriptcmd_devoice, LEVEL_DEVOICE, "!devoice", NULL, HELP_DEVOICE);
  cmd_bind (scriptcmd_say, LEVEL_SAY, "!say", NULL, HELP_SAY);
  cmd_bind (scriptcmd_help, LEVEL_HELP, "!help", NULL, NULL);
  cmd_bind (scriptcmd_dcckill, LEVEL_DCCKILL, "!dcckill", NULL, HELP_DCCKILL);
  cmd_bind (scriptcmd_dcclist, LEVEL_DCCLIST, "!dcclist", NULL, HELP_DCCLIST);
  cmd_bind (scriptcmd_chat, LEVEL_CHAT, "!chat", NULL, HELP_CHAT);
  cmd_bind (scriptcmd_randkick, LEVEL_RANDKICK, "!randkick", NULL, HELP_RANDKICK);
  cmd_bind (scriptcmd_time, LEVEL_TIME, "!time", NULL, HELP_TIME);
  cmd_bind (scriptcmd_rose, LEVEL_ROSE, "!rose", NULL, HELP_ROSE);
  cmd_bind (scriptcmd_reverse, LEVEL_REVERSE, "!reverse", NULL, HELP_REVERSE);
  cmd_bind (scriptcmd_cop, LEVEL_COP, "!cop", NULL, HELP_COP);
  cmd_bind (scriptcmd_cdeop, LEVEL_CDEOP, "!cdeop", NULL, HELP_CDEOP);
  cmd_bind (scriptcmd_useradd, LEVEL_USERADD, "!useradd", NULL, HELP_USERADD);
  cmd_bind (scriptcmd_userdel, LEVEL_USERDEL, "!userdel", NULL, HELP_USERDEL);
  cmd_bind (scriptcmd_userpass, LEVEL_USERPASS, "!userpass", NULL, HELP_USERPASS);
  cmd_bind (scriptcmd_usermsg, LEVEL_USERMSG, "!usermsg", NULL, HELP_USERMSG);
  cmd_bind (scriptcmd_usermask, LEVEL_USERMASK, "!usermask", NULL, HELP_USERMASK);
  cmd_bind (scriptcmd_userlevel, LEVEL_USERLEVEL, "!userlevel", NULL, HELP_USERLEVEL);
  cmd_bind (scriptcmd_id, LEVEL_ID, "!id", NULL, HELP_ID);
  cmd_bind (scriptcmd_userlist, LEVEL_USERLIST, "!userlist", NULL, HELP_USERLIST);
  cmd_bind (scriptcmd_setpass, LEVEL_SETPASS, "!setpass", NULL, HELP_SETPASS);
  cmd_bind (scriptcmd_setmsg, LEVEL_SETMSG, "!setmsg", NULL, HELP_SETMSG);
  cmd_bind (scriptcmd_pass, LEVEL_PASS, "!pass", NULL, HELP_PASS);
  cmd_bind (scriptcmd_level, LEVEL_LEVEL, "!level", NULL, HELP_LEVEL);
  replies.add ((void *)scriptcmd_dcc_event);
}

Script::~Script (void)
{
  struct cmd_type *c;
  cmds.rewind ();
  while ((c = (cmd_type *)cmds.next ()) != NULL)
    delete c;
}

void
Script::cmd_bind (void (*action)(NetServer *), int level, c_char cmd, Module *module, c_char help)
{
  if (level > LEVEL_MAX)
    level = LEVEL_MAX;
  if (level < 0)
    level = 0;
  cmds.add ((void *)new cmd_type (cmd, level, action, module, help));
}

bool
Script::cmd_unbind (c_char cmd)
{
  cmd_type *c;
  cmds.rewind ();
  while ((c = (cmd_type *)cmds.next ()) != NULL)
    {
      if (c->name |= cmd)
        {
          cmds.del ((void *)c);
          delete c;
          return 1;
        }
    }
  return 0;
}

// return the cmd_type of <cmd>, NULL if nonexistant
Script::cmd_type *
Script::cmd_get (c_char cmd)
{
  cmd_type *c;
  cmds.rewind ();
  while ((c = (cmd_type *)cmds.next ()) != NULL)
    if (c->name |= cmd)
      return c;
  return NULL;
}

void
Script::send_partyline (c_char format, ...)
{
  char buftext[MSG_SIZE+1];
  va_list args;
  va_start (args, format);
  vsnprintf (buftext, MSG_SIZE, format, args);
  va_end (args);
  for (int i = 0; i < DCC_NUM; i++)		// check all dccs
    if (DCCS[i]->status == DCC_CHAT)		// if it's a dcc chat
      DCCS[i]->dcc_chat_write (buftext);	// send to it
}

void
Script::send_text (c_char to, c_char format, ...)
{
  char buftext[MSG_SIZE+1];
  va_list args;
  va_start (args, format);
  vsnprintf (buftext, MSG_SIZE, format, args);
  va_end (args);
  if (isdigit (CMD[2][0]) && strcasecmp (source, dest) == 0)
    DCCS[atoi (CMD[2])]->dcc_chat_write (buftext);
  else
    s->irc_privmsg (to, "%s", buftext);
}

void
Script::irc_reply (void)
{
  switch (REPLY)
    {
      case INIT:
        reply_init ();
        break;
      case RPL_NOTOPIC:
        reply_notopic ();
        break;
      case RPL_TOPIC:
        reply_topic ();
        break;
      case RPL_WHOREPLY:
        reply_whoreply ();
        break;
      case RPL_ENDOFWHO:
        reply_endofwho ();
        break;
      case ERR_NOSUCHNICK:
        reply_nosuchnick ();
        break;
      case ERR_NOSUCHCHANNEL:
        reply_nosuchchannel ();
        break;
      case ERR_TOOMANYCHANNELS:
        reply_toomanychannels ();
        break;
      case ERR_UNKNOWNCOMMAND:
        reply_unknowncommand ();
        break;
      case ERR_NICKNAMEINUSE:
        reply_nicknameinuse ();
        break;
      case ERR_INVITEONLYCHAN:
        reply_inviteonlychan ();
        break;
      case ERR_BANNEDFROMCHAN:
        reply_bannedfromchan ();
        break;
      case ERR_CHANOPRIVSNEEDED:
        reply_chanoprivsneeded ();
        break;
      case RPL_WATCHOFFLINE:
        reply_watchoffline ();
        break;
      default:
        break;
    }

  void (*f)(NetServer *);
  replies.rewind ();
  while ((f = (void (*)(NetServer *))replies.next ()) != NULL)
    f (s);
}

void
Script::reply_init (void)
{
  // identify, join channels, set away
  if (s->services.exist && !s->services.identified)
    {
      s->services.nick_identify ();
      s->services.identified = 1;
    }
  for (int i = 0; i < s->channel_num; i++)
    CHANNELS[i]->irc_join ();
  if (s->away)
    s->irc_away (s->away);
}

void
Script::reply_ison (void)
{
  // check if the services are active
  if (strcasecmp (CMD[3], NICKSERV) == 0)
    s->services.exist = 1;
}

void
Script::reply_notopic (void)
{
  Channel *channel = s->channel_get (CMD[3]);
  if (channel != NULL)
    channel->topic = NULL;
}

void
Script::reply_topic (void)
{
  Channel *channel = s->channel_get (CMD[3]);
  if (channel != NULL)
    channel->topic = CMD[4];
}

void
Script::reply_whoreply (void)
{
  Channel *channel = s->channel_get (CMD[3]);
  if (channel == NULL)
    return;
  snprintf (buf[0], MSG_SIZE, "%s!%s@%s", CMD[7], CMD[4], CMD[5]);
  channel->user_add (buf[0], CMD[8][1] == '@', CMD[8][1] == '+');

  if (channel->kick_nick == CMD[7])		// if the bot was kicked by him
    {
      channel->irc_kick (CMD[7], NULL);		// kick him back
      channel->kick_nick = NULL;
    }
}

void
Script::reply_endofwho (void)
{
  Channel *channel = s->channel_get (CMD[3]);
  if (channel == NULL)
    return;
  if (channel->kick_nick)
    channel->kick_nick = NULL;	// stop waiting for that nick
}

void
Script::reply_nosuchnick (void)
{
  // check if services are missing
  if (strcasecmp (CMD[3], NICKSERV) == 0
      || strcasecmp (CMD[3], CHANSERV) == 0
      || strcasecmp (CMD[3], MEMOSERV) == 0
      || strcasecmp (CMD[3], OPERSERV) == 0)
    s->services.exist = 0;
}

void
Script::reply_nosuchchannel (void)
{
  // happens with an invalid name in !join or .conf file
  s->channel_del (CMD[3]);
  s->write_botlog ("warning: can't join %s, no such channel", CMD[3]);
}

void
Script::reply_toomanychannels (void)
{
  s->channel_del (CMD[3]);
  s->write_botlog ("warning: can't join %s, too many channels", CMD[3]);
}

void
Script::reply_unknowncommand (void)
{
  if (strcasecmp (CMD[3], NICKSERV) == 0
      || strcasecmp (CMD[3], CHANSERV) == 0
      || strcasecmp (CMD[3], MEMOSERV) == 0
      || strcasecmp (CMD[3], OPERSERV) == 0)
    s->services.exist = 0;
}

void
Script::reply_nicknameinuse (void)
{
  // nick in use, therefore another copy of the bot is running, so exit
  s->write_botlog ("warning: nick %s already in use, reconnecting", (c_char)s->nick);
  s->irc_restart ();
  s->irc_connect ();
}

void
Script::reply_inviteonlychan (void)
{
  if (!s->services.exist)
    return;

  Channel *channel = s->channel_get (CMD[3]);
  if (channel != NULL)
    {
      s->services.chan_invite (CMD[3]);
      sleep (1);
      channel->irc_join ();
    }
}

void
Script::reply_bannedfromchan (void)
{
  if (!s->services.exist)
    return;

  Channel *channel = s->channel_get (CMD[3]);
  if (channel != NULL)
    {
      s->services.chan_unban (CMD[3]);
      sleep (1);
      channel->irc_join ();
    }
}

void
Script::reply_chanoprivsneeded (void)
{
  if (s->services.exist)
    s->services.chan_op (CMD[3], s->nick);
}

void
Script::reply_watchoffline (void)
{
  snprintf (buf[0], MSG_SIZE, "%s!%s@*", CMD[3], CMD[4]);
  s->users.match_set_id (buf[0], 0);
  s->irc_watch (CMD[3], 0);
}

void
Script::irc_event (void)
{
  if (strcmp (CMD[0], "PING") == 0)
    {
      EVENT = EVENT_PING;
      event_ping ();
    }
  else if (strcmp (CMD[0], "ERROR") == 0)
    {
      EVENT = EVENT_ERROR;
      event_error ();
    }
  else if (strcmp (CMD[1], "NOTICE") == 0)
    {
      EVENT = EVENT_NOTICE;
      event_notice ();
    }
  else if (strcmp (CMD[1], "JOIN") == 0)
    {
      EVENT = EVENT_JOIN;
      event_join ();
    }
  else if (strcmp (CMD[1], "QUIT") == 0)
    {
      EVENT = EVENT_QUIT;
      event_quit ();
    }
  else if (strcmp (CMD[1], "NICK") == 0)
    {
      EVENT = EVENT_NICK;
      event_nick ();
    }
  else if (strcmp (CMD[1], "KICK") == 0)
    {
      EVENT = EVENT_KICK;
      event_kick ();
    }
  else if (strcmp (CMD[1], "PART") == 0)
    {
      EVENT = EVENT_PART;
      event_part ();
    }
  else if (strcmp (CMD[1], "MODE") == 0)
    {
      EVENT = EVENT_MODE;
      event_mode ();
    }
  else if (strcmp (CMD[1], "PRIVMSG") == 0)
    {
      EVENT = EVENT_PRIVMSG;
      event_privmsg ();
    }
  else if (strcmp (CMD[1], "TOPIC") == 0)
    {
      EVENT = EVENT_TOPIC;
      event_topic ();
    }

  void (*f)(NetServer *);
  events.rewind ();
  while ((f = (void (*)(NetServer *))events.next ()) != NULL)
    f (s);
}

void
Script::event_ping (void)
{
  s->irc_pong (CMD[1]);
  if (s->services.exist && !s->services.identified)
    {
      s->services.nick_identify ();
      s->services.identified = 1;
    }
  for (int i = 0; i < s->channel_num; i++)
    if (!CHANNELS[i]->joined)
      CHANNELS[i]->irc_join ();
}

void
Script::event_error (void)
{
  s->write_botlog ("error from server: %s", CMD[1]);
  s->irc_restart ();
  s->host_current = s->host_current->next;
  if (server_jumped)	// if !server was used, don't wait before connecting
    {
      server_jumped = false;
      s->last_try = s->time_now - TIME_RETRY - 1;
    }
  else
    s->last_try = s->time_now;	// wait TIME_RETRY before connecting
  s->irc_connect ();
}

void
Script::event_notice (void)
{
  mask2nick (CMD[0], source);
  if (CMD[2][0] == '#')
    my_strncpy (dest, CMD[2], CHANNEL_SIZE);
  else
    mask2nick (CMD[0], dest);
  strcpy (CMD[3], CMD[3]+num_spaces (CMD[3]));	// remove spaces from beginning

  // to when nickserv restarts
  if ((bool)s->services.nickserv_auth
    && (s->services.nickserv_mask |= CMD[0])
    && strncasecmp (CMD[3], s->services.nickserv_auth, s->services.nickserv_auth.getlen ()) == 0)
    {
      s->services.exist = 1;
      s->services.nick_identify ();
    }
}

void
Script::event_join (void)
{
  mask2nick (CMD[0], buf[0]);
  int level = USERS.match_mask_level (CMD[0]);
  Channel *channel = s->channel_get (CMD[2]);

  if (channel == NULL)
    return;

  channel->user_add (CMD[0], 0, 0);		// add it to the list

  if (s->nick |= buf[0])			// when the bot joins
    {
      channel->joined = 1;
      s->irc_who (CMD[2]);			// get users
    }

  if (level < 0)				// shitlist
    {
      struct ListUsers::user_type *u = USERS.match_mask2user (CMD[0]);
      channel->irc_deopban (buf[0], u->mask);
      channel->irc_kick (buf[0], u->msg);
    }

  if (level > 0)				// welcome msg
    {
      struct ListUsers::user_type *u = USERS.match_mask2user (CMD[0]);
      if ((bool)u->msg && u->msg[0] != 0)
        send_text (CMD[2], "%s", (c_char)u->msg);
    }

  if (level >= 5)				// op
    channel->irc_op (buf[0]);
}

void
Script::event_quit (void)
{
  int level = USERS.match_mask_level (CMD[0]);
  int i, i2;
  mask2nick (CMD[0], buf[0]);

  for (i = 0; i < s->channel_num; i++)		// search in all channels
    {
      i2 = CHANNELS[i]->user_index (buf[0]);
      if (i2 != -1)				// if it was here  
        CHANNELS[i]->user_del (buf[0]);		// delete
    }

  if (level > 0)				// deactivate id
    USERS.match_set_id (CMD[0], 0);
}

void
Script::event_nick (void)
{
  int level = USERS.match_mask_level (CMD[0]);
  int i;
  // build new mask
  mask_split (CMD[0], buf[0], buf[1], buf[2]);
  snprintf (buf[3], MSG_SIZE, "%s!%s@%s", CMD[2], buf[1], buf[2]);

  for (i = 0; i < s->channel_num; i++)	// refresh in all channels
    CHANNELS[i]->user_change_nick (buf[0], buf[3]);

  if (level != 0)				// deactivate id
    USERS.match_set_id (CMD[0], 0);

  if (s->nick |= buf[0])	// check if bot's nick changed
    s->nick = CMD[2];

  if (USERS.match_mask_level (buf[3]) < 0)	// shitlist
    {
      struct ListUsers::user_type *u = USERS.match_mask2user (buf[3]);
      for (i = 0; i < s->channel_num; i++)		// search all channels
        if (CHANNELS[i]->user_index (CMD[2]) != -1)	// if it's there
          {
            CHANNELS[i]->irc_deopban (CMD[2], u->mask);
            CHANNELS[i]->irc_kick (CMD[2], u->msg);
          }
    }
}

void
Script::event_kick (void)
{
  Channel *channel = s->channel_get (CMD[2]);

  if (channel == NULL)				// should never fail
    return;

  mask2nick(CMD[0], buf[0]);

  channel->user_del (CMD[3]);			// delete kicked user

  if (s->nick |= CMD[3])			// if bot is kicked
    {
      for (int i = 0; channel->user_num > i; i++)
        delete channel->users[i];		// delete all users
      channel->user_num = 0;
      channel->joined = 0;			// the bot is out
      channel->kick_nick = buf[0];		// remember to kick the other
      channel->irc_join ();			// join channel
    }
}

void
Script::event_part (void)
{
  Channel *channel = s->channel_get (CMD[2]);

  if (channel == NULL)			// doesn't happen when the bot !part
    return;

  mask2nick (CMD[0], buf[0]);
  if (s->nick |= buf[0])			// if the bot parts
    {
      for (int i = 0; channel->user_num > i; i++)
        delete channel->users[i];		// delete all users
      channel->user_num = 0;
      channel->joined = 0;			// the bot is out
    }
  else
    channel->user_del (buf[0]);			// delete the user
}

void
Script::event_mode (void)
{
  Channel *channel = s->channel_get (CMD[2]);

  if (channel == NULL)				// should never fail
    return;

  bool mode = false;
  int pos = 0;
  mask2nick (CMD[0], buf[0]);

  for (size_t i = 0; i < strlen (CMD[3]); i++)
    switch (CMD[3][i])
      {
        case '-':
          mode = false;
          break;
        case '+':
          mode = true;
          break;
        case 'o':
          if (!mode)
            {
              channel->user_change_op (CMD[4+pos], 0);
              if (s->nick |= CMD[4+pos])	// op the bot when deopped
                {
                  s->services.chan_op (CMD[2], s->nick);
                  mask2nick (CMD[0], buf[0]);
                  s->services.chan_deop (CMD[2], buf[0]); // deop the other
                }
            }
          else
            channel->user_change_op (CMD[4+pos], 1);
          pos++;
          break;
        case 'b':		// XXX update ban list in channel...
          pos++; 
          break;
        case 'v':
          channel->user_change_voice (CMD[4+pos], mode);
          pos++;
          break;
        default:		// irc protocol changed?
          pos++;
      }
}

void
Script::event_privmsg (void)
{
  mask2nick (CMD[0], source);
  if ((CMD[2][0] == '#') || ((CMD[2][0] == '@') && (CMD[2][1] == '#')) && (CMD[3][0] != '\001')) {
    my_strncpy (dest, CMD[2], CHANNEL_SIZE);
  } else {
    mask2nick (CMD[0], dest);
  }
  strcpy (CMD[3], CMD[3]+num_spaces (CMD[3]));	// remove spaces from beginning

  if (CMD[3][0] == '\001')
    {
      if (strcmp (CMD[3], "\001VERSION\001") == 0 && (bool)ctcp_version)
        s->irc_notice (dest, "\001VERSION %s\001", (c_char)ctcp_version);
      else if (strcmp (CMD[3], "\001FINGER\001") == 0 && (bool)ctcp_finger)
        s->irc_notice (dest, "\001FINGER %s\001", (c_char)ctcp_finger);
      else if (strcmp (CMD[3], "\001USERINFO\001") == 0)
        s->irc_notice (dest, "\001USERINFO %s running on %s (%d bits)\001", VERSION_STRING, HOST, 8*sizeof (int *));
      else if (strncmp (CMD[3], "\001PING ", 6) == 0)
        s->irc_notice (dest, CMD[3]);
      else if (strcmp (CMD[3], "\001TIME\001") == 0)
        s->irc_notice (dest, "\001TIME %s\001", get_asctime (s->time_now, buf[2], MSG_SIZE));
      else if (strcmp (CMD[3], "\001SOURCE\001") == 0 && (bool)ctcp_source)
        s->irc_notice (dest, "\001SOURCE %s\001", (c_char)ctcp_source);
      else if (strncmp (CMD[3], "\001ECHO ", 6) == 0)
        s->irc_notice (dest, CMD[3]);
      else if (strcmp (CMD[3], "\001CLIENTINFO\001") == 0)
        s->irc_notice (dest, "\001CLIENTINFO VERSION FINGER USERINFO PING TIME SOURCE ECHO CLIENTINFO\001");
    }

  // check the binded commands
  my_strncpy (buf[8], CMD[3], num_notspaces (CMD[3]));
  cmd_type *c = cmd_get (buf[8]);
  if (c != NULL && USERS.match_mask_level (CMD[0]) >= c->level)
    {
      c->action (s);
      return;
    }

  if (isdigit (CMD[2][0]))	// if it's a dcc chat
    send_partyline ("<%s> %s", source, CMD[3]);
}

void
Script::event_topic (void)
{
  Channel *channel = s->channel_get (CMD[2]);
  if (channel != NULL)
    channel->topic = CMD[3];
}

