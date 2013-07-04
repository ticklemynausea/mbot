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

#include "scriptcmd.h"

#define SOURCE s->script.source
#define DEST s->script.dest
#define BUF s->script.buf
#define SEND_TEXT s->script.send_text

// !modlist
void
scriptcmd_modlist (NetServer *s)
{
  if (bot->modules.count () == 0)
    {
      SEND_TEXT (DEST, "No modules loaded.");
      return;
    }
  Module *m;
  StringFixed tmp (MSG_SIZE);
  bot->modules.rewind ();
  while ((m = (Module *)bot->modules.next ()) != NULL)
    {
      tmp += (c_char)*m;
      tmp += " ";
    }
  SEND_TEXT (DEST, "Loaded Modules: %s", (c_char)tmp);
}

// !modadd path
void
scriptcmd_modadd (NetServer *s)
{
  strsplit (CMD[3], BUF, 1);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_MODADD);
      return;
    }
  Module *mod = bot->add_module (BUF[1]);
  if (mod == NULL)
    {
      SEND_TEXT (DEST, "Failed to load module.");
      return;
    }
  NetServer *server = NULL;
  const char *bufline;
  int server_cur = -1;

  bot->line = 0;
  bot->conf.rewind_text ();
  while ( (bufline = bot->conf.get_line ()) != NULL)
    {
      bot->line++;
      if (bufline[0] == 0 || bufline[0] == '#' || bufline[0] == 10)
        continue;
      strsplit (bufline, BUF, 2);
      if (strcasecmp (BUF[0], "bot") == 0)
        {
          server_cur++;
          if (server_cur < bot->server_num)
            server = bot->servers[server_cur];
        }
      if (server == NULL)
        continue;
      if (strcasecmp (BUF[0], "set") == 0)
        {
          if (!server->vars.var_set (BUF[1], BUF[2]))
            {
              snprintf (BUF[3], MSG_SIZE, "inexistant variable \"%s\".", BUF[1]);
              bot->conf_warn (BUF[3]);
            }
        }
      mod->conf (server, bufline);
    }
  SEND_TEXT (DEST, "Module loaded.");
}

// !moddel name
void
scriptcmd_moddel (NetServer *s)
{
  strsplit (CMD[3], BUF, 1);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_MODDEL);
      return;
    }
  if (bot->del_module (BUF[1]))
    SEND_TEXT (DEST, "Module unloaded.");
  else
    SEND_TEXT (DEST, "Failed to unload module.");
}

// !quit [reason]
void
scriptcmd_quit (NetServer *s)
{
  strsplit (CMD[3], BUF, 1);
  for (int i = 0; i < bot->server_num; i++)
    if (BUF[1][0] == 0)
      bot->servers[i]->irc_quit (NULL);
    else
      bot->servers[i]->irc_quit (BUF[1]);
  mbot_exit ();
}

// !server next | <hostname [<port> [<password>]]>
void
scriptcmd_server (NetServer *s)
{
  strsplit (CMD[3], BUF, 3);

  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_SERVER);
      return;
    }

  if (strcasecmp (BUF[1], "next") != 0)		// a hostname was specified
    {
      s->host_current = s->host_add (s->hosts, BUF[1],
                                     BUF[2][0] == 0 ? 6667 : atoi (BUF[2]),
                                     BUF[3][0] == 0 ? NULL : BUF[3]);
    }
  s->script.server_jumped = true;
  s->irc_quit ("switching servers");
}

// !set option [value]
void
scriptcmd_set (NetServer *s)
{
  strsplit (CMD[3], BUF, 2);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_SET);
      return;
    }
  if (BUF[2][0] != 0)
    {
      if (s->vars.var_set (BUF[1], BUF[2]))
        {
          s->vars.var_get (BUF[1], BUF[2], MSG_SIZE);
          SEND_TEXT (DEST, "Value of \002%s\002 changed to \"%s\".", BUF[1], BUF[2]);
          return;
        }
    }
  else if (s->vars.var_get (BUF[1], BUF[2], MSG_SIZE))
    {
      SEND_TEXT (DEST, "Value of \002%s\002 is \"%s\".", BUF[1], BUF[2]);
      return;
    }
  SEND_TEXT (DEST, "Inexistant variable \002%s\002.", BUF[1]);
}

// !loadconf
void
scriptcmd_loadconf (NetServer *s)
{
  strsplit (CMD[3], BUF, 2);

  if (BUF[1][0] != 0)
    {
      bot->conf_file = BUF[1];
      SEND_TEXT (DEST, "Configuration file set to \"%s\".", (c_char)bot->conf_file);
    }
  bot->conf.delete_lines ();
  if (bot->conf.read_file (bot->conf_file))
    SEND_TEXT (DEST, "Configuration loaded.");
  else
    SEND_TEXT (DEST, "Couldn't load configuration!");
}

// !away [msg]
void
scriptcmd_away (NetServer *s)
{
  strsplit (CMD[3], BUF, 1);
  s->away = BUF[1];
  s->irc_away (s->away);
}

// !nick nick
void
scriptcmd_nick (NetServer *s)
{
  strsplit (CMD[3], BUF, 2);
  if (BUF[1][0] == 0)
    SEND_TEXT(DEST, "%s", HELP_NICK);
  else
    s->irc_nick (BUF[1]);
}

// !unbind !command
void
scriptcmd_unbind (NetServer *s)
{
  strsplit (CMD[3], BUF, 2);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_UNBIND);
      return;
    }
  if (s->script.cmd_unbind (BUF[1]))
    SEND_TEXT (DEST, "Command unbinded.");
  else
    SEND_TEXT (DEST, "Command \002%s\002 does not exist.", BUF[1]);
}

// !cmdlevel !command level
void
scriptcmd_cmdlevel (NetServer *s)
{
  strsplit (CMD[3], BUF, 3);
  if (BUF[2][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_CMDLEVEL);
      return;
    }
  Script::cmd_type *c = s->script.cmd_get (BUF[1]);
  if (c == NULL)
    {
      SEND_TEXT (DEST, "Command \002%s\002 does not exist.", BUF[1]);
      return;
    }
  int level = atoi (BUF[2]);
  if (level > LEVEL_MAX)
    c->level = LEVEL_MAX;
  else if (level < 0)
    c->level = 0;
  else
    c->level = level;
  SEND_TEXT (DEST, "Command \002%s\002's level set to \002%d\002.", BUF[1], c->level);
}

// !join #channel [key]
void
scriptcmd_join (NetServer *s)
{
  strsplit (CMD[3], BUF, 3);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_JOIN);
      return;
    }
  int chan_num = CHANNEL_INDEX (BUF[1]);
  if (chan_num != -1)		// if not already in the channel
    {
      SEND_TEXT (DEST, "I'm already on \002%s\002.", BUF[1]);
      return;
    }
  if (!s->channel_add (BUF[1], BUF[2]))
    {
      SEND_TEXT (DEST, "Maximum channels reached.");
      return;
    }
  chan_num = CHANNEL_INDEX (BUF[1]);	// search it's position again
  CHANNELS[chan_num]->irc_join ();	// join it
}

// !part #channel
void
scriptcmd_part (NetServer *s)
{
  strsplit (CMD[3], BUF, 2);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_PART);
      return;
    }
  int i = CHANNEL_INDEX (BUF[1]);
  if (i != -1)
    {
      CHANNELS[i]->irc_part ();
      s->channel_del (BUF[1]);
    }
  else
    SEND_TEXT (DEST, "I'm not on \002%s\002.", BUF[1]);
}

// !status
void
scriptcmd_status (NetServer *s)
{
  SEND_TEXT (DEST,
             "Status: %s - %s:%d - %s!%s (%s) - %d users, %d/%d channels, %d bytes read, %d written. Up since %s, running on %s (%d bits)",
             VERSION_STRING, (c_char)s->host_current->host,
             s->host_current->port, (c_char)s->nick, (c_char)s->user,
             (c_char)s->name, USERS.count (), s->channel_num, CHANNEL_MAX,
             s->bytesin, s->bytesout,
             get_asctime (s->uptime, BUF[2], MSG_SIZE),
             HOST, 8*sizeof (int *));
}

// !topic [#channel] text
void
scriptcmd_topic (NetServer *s)
{
  strsplit (CMD[3], BUF, 1);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_TOPIC);
      return;
    }
  int i;
  if (BUF[1][0] != '#')
    {
      i = CHANNEL_INDEX (CMD[2]);
      if (i != -1)
        CHANNELS[i]->irc_topic (BUF[1]);
      else
        SEND_TEXT (DEST, "%s", SCRIPT_INVALID_CHAN);
    }
  else
    {
      strsplit (CMD[3], BUF, 2);
      i = CHANNEL_INDEX (BUF[1]);
      if (i != -1)
        CHANNELS[i]->irc_topic (BUF[2]);
      else
        SEND_TEXT (DEST, "%s", SCRIPT_INVALID_CHAN);
    }
}

// !opme
void
scriptcmd_opme (NetServer *s)
{
  if (CMD[2][0] != '#')
    {
      SEND_TEXT (DEST, "This command only works inside channels.");
      return;
    }
  mask2nick (CMD[0], BUF[0]);
  int i = CHANNEL_INDEX (CMD[2]);
  if (i != -1)
    CHANNELS[i]->irc_op (BUF[0]);
  else
    SEND_TEXT (DEST, "%s", SCRIPT_INVALID_CHAN);
}

// !op nick [#channel]
void
scriptcmd_op (NetServer *s)
{
  strsplit (CMD[3], BUF, 3);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_OP);
      return;
    }
  if (BUF[2][0] != '#')
    my_strncpy (BUF[2], CMD[2], MSG_SIZE);
  int i = CHANNEL_INDEX (BUF[2]);
  if (i != -1)
    CHANNELS[i]->irc_op (BUF[1]);
  else
    SEND_TEXT (DEST, "%s", SCRIPT_INVALID_CHAN);
}

// !deop nick [#channel]
void
scriptcmd_deop (NetServer *s)
{
  strsplit (CMD[3], BUF, 3);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_DEOP);
      return;
    }
  if (BUF[2][0] != '#')
    strncpy (BUF[2], CMD[2], MSG_SIZE);
  if (strcasecmp (BUF[1], s->nick) != 0)
    {
      int i = CHANNEL_INDEX (BUF[2]);
      if (i != -1)
        CHANNELS[i]->irc_deop (BUF[1]);
      else
        SEND_TEXT (DEST, "%s", SCRIPT_INVALID_CHAN);
    }
  else
    SEND_TEXT (DEST, "%s", SCRIPT_HURT_BOT);
}

// !kick nick [#channel] [reason]
void
scriptcmd_kick (NetServer *s)
{
  strsplit (CMD[3], BUF, 2);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_KICK);
      return;
    }
  if (strcasecmp (BUF[1], s->nick) == 0)	// doesn't kick the bot
    {
      SEND_TEXT (DEST, "%s", SCRIPT_HURT_BOT);
      return;
    }
  int i;
  if (BUF[2][0] != '#')
    {
      i = CHANNEL_INDEX(CMD[2]);
      if (i != -1)
        CHANNELS[i]->irc_kick (BUF[1], BUF[2]);
      else
        SEND_TEXT (DEST, "%s", SCRIPT_INVALID_CHAN);
    }
  else
    {
      strsplit (CMD[3], BUF, 3);
      i = CHANNEL_INDEX (BUF[2]);
      if (i != -1)
        CHANNELS[i]->irc_kick (BUF[1], BUF[3]);
      else
        SEND_TEXT (DEST, "%s", SCRIPT_INVALID_CHAN);
    }
}

// !ban nick | mask [#channel]
void
scriptcmd_ban (NetServer *s)
{
  strsplit (CMD[3], BUF, 3);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT(DEST, "%s", HELP_BAN);
      return;
    }
  if (BUF[2][0] != '#')
    my_strncpy (BUF[2], CMD[2], MSG_SIZE);
  int i;
  if (strchr (BUF[1], '@') != NULL)		// check if it's a mask
    {
      i = CHANNEL_INDEX (BUF[2]);
      if (i != -1)
        CHANNELS[i]->irc_ban_mask (BUF[1]);
      else
        SEND_TEXT (DEST, "%s", SCRIPT_INVALID_CHAN);
      return;
    }
  if (strcasecmp (BUF[1], s->nick) != 0)	// doesn't ban the bot
    {
      i = CHANNEL_INDEX (BUF[2]);
      if (i != -1)
        CHANNELS[i]->irc_ban_nick (BUF[1]);
      else
        SEND_TEXT (DEST, "%s", SCRIPT_INVALID_CHAN);
    }
  else
    SEND_TEXT (DEST, "%s", SCRIPT_HURT_BOT);
}

// !unban nick | mask [#channel]
void
scriptcmd_unban (NetServer *s)
{
  strsplit (CMD[3], BUF, 3);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_UNBAN);
      return;
    }
  if (BUF[2][0] != '#')
    my_strncpy (BUF[2], CMD[2], MSG_SIZE);
  int i;
  if (strchr (BUF[1], '@') != NULL)		// check if it's a mask
    {
      i = CHANNEL_INDEX (BUF[2]);
      if (i != -1)
        CHANNELS[i]->irc_unban_mask (BUF[1]);
      else
        SEND_TEXT (DEST, "%s", SCRIPT_INVALID_CHAN);
      return;
    }
  i = CHANNEL_INDEX (BUF[2]);
  if (i != -1)
    CHANNELS[i]->irc_unban_nick (BUF[1]);
  else
    SEND_TEXT (DEST, "%s", SCRIPT_INVALID_CHAN);
}

// !bk nick [#channel] [reason]
void
scriptcmd_bk (NetServer *s)
{
  strsplit (CMD[3], BUF, 2);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT(DEST, "%s", HELP_BK);
      return;
    }
  if (strcasecmp (BUF[1], s->nick) == 0)		// doesn't bk the bot
    {
      SEND_TEXT (DEST, "%s", SCRIPT_HURT_BOT);
      return;
    }
  if (BUF[2][0] != '#')		// if the channel was not specified
    {
      my_strncpy (BUF[3], BUF[2], MSG_SIZE);		// reason
      my_strncpy (BUF[2], CMD[2], MSG_SIZE);		// channel
    }
  else
    strsplit (CMD[3], BUF, 3);
  int i = CHANNEL_INDEX (BUF[2]);
  if (i == -1)			// if the channel exists
    {
      SEND_TEXT (DEST, "%s", SCRIPT_INVALID_CHAN);
      return;
    }
  int i2 = CHANNELS[i]->user_index (BUF[1]);
  if (i2 != -1)		// and the user too
    {
      // make mask
      make_generic_mask (CHANNELS[i]->users[i2]->mask, BUF[4]);
      // deop/ban
      CHANNELS[i]->irc_deopban (BUF[1], BUF[4]);
      // kick
      CHANNELS[i]->irc_kick (BUF[1], BUF[3][0] == 0 ? NULL : BUF[3]);
    }
  else
    SEND_TEXT (DEST, "Nick %s is not inside %s.", BUF[1], BUF[2]);
}

// !invite nick [#channel]
void
scriptcmd_invite (NetServer *s)
{
  strsplit (CMD[3], BUF, 3);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_INVITE);
      return;
    }
  if (BUF[2][0] != '#')
    my_strncpy (BUF[2], CMD[2], MSG_SIZE);
  int i = CHANNEL_INDEX (BUF[2]);
  if (i != -1)
    CHANNELS[i]->irc_invite (BUF[1]);
  else
    SEND_TEXT (DEST, "%s", SCRIPT_INVALID_CHAN);
}

// !voice nick [#channel] 
void
scriptcmd_voice (NetServer *s)
{
  strsplit (CMD[3], BUF, 3);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_VOICE);
      return;
    }
  if (BUF[2][0] != '#')
    my_strncpy (BUF[2], CMD[2], MSG_SIZE);
  int i = CHANNEL_INDEX (BUF[2]);
  if (i != -1)
    CHANNELS[i]->irc_voice (BUF[1]);
  else
    SEND_TEXT (DEST, "%s", SCRIPT_INVALID_CHAN);
}

// !devoice nick [#channel]
void
scriptcmd_devoice (NetServer *s)
{
  strsplit (CMD[3], BUF, 3);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT(DEST, "%s", HELP_DEVOICE);
      return;
    }
  if (BUF[2][0] != '#')
    my_strncpy (BUF[2], CMD[2], MSG_SIZE);
  if (strcasecmp (BUF[1], s->nick) != 0)
    {
      int i = CHANNEL_INDEX (BUF[2]);
      if (i != -1)
        CHANNELS[i]->irc_devoice (BUF[1]);
      else
        SEND_TEXT (DEST, "%s", SCRIPT_INVALID_CHAN);
    }
  else
    SEND_TEXT (DEST, "%s", SCRIPT_HURT_BOT);
}

// !say [#channel | $nick] text
void
scriptcmd_say (NetServer *s)
{
  strsplit (CMD[3], BUF, 1);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_SAY);
      return;
    }
  if (BUF[1][0] == '#')
    {
      int i = 0;
      while (BUF[1][i] != ' ' && BUF[1][i] != 0)
        i++;
      my_strncpy (DEST, BUF[1], i);
      my_strncpy (BUF[2], BUF[1] + i + num_spaces (BUF[1]+i), MSG_SIZE);
      my_strncpy (BUF[1], BUF[2], MSG_SIZE);
    }
  else if (BUF[1][0] == '$')
    {
      int i = 0;
      while (BUF[1][i] != ' ' && BUF[1][i] != 0)
        i++;
      my_strncpy (DEST, BUF[1], i);
      my_strncpy (DEST, DEST+1, CHANNEL_SIZE);
      if (strcasecmp (NICKSERV, DEST) == 0
          || strcasecmp (CHANSERV, DEST) == 0
          || strcasecmp (MEMOSERV, DEST) == 0
          || strcasecmp (OPERSERV, DEST) == 0)
        {
          SEND_TEXT (SOURCE, "Cannot send to IRC Services.");
          return;
        }
      my_strncpy (BUF[2], BUF[1] + i + num_spaces (BUF[1] + i), MSG_SIZE);
      my_strncpy (BUF[1], BUF[2], MSG_SIZE);
    }
  SEND_TEXT (DEST, "%s", BUF[1]);
}

// !help
void
scriptcmd_help (NetServer *s)
{
  int level = USERS.match_mask_level (CMD[0]);
  Script::cmd_type *c;

  strsplit (CMD[3], BUF, 1);
  if (BUF[1][0] != 0)	// a command was specified
    {
      c = s->script.cmd_get (BUF[1]);
      if (c == NULL || c->level > level)
        {
          SEND_TEXT (SOURCE, "Unknown command \002%s\002. Try \002!help\002 for a list of available commands.", BUF[1]);
          return;
        }
      if (c->help == NULL)
        {
          SEND_TEXT (SOURCE, "No help available for \002%s\002.", BUF[1]);
          return;
        }
      SEND_TEXT (SOURCE, "%s", c->help);
      return;
    }

  bool has_help;
  SEND_TEXT (SOURCE, "Commands (do \002!help command\002 to see help on it):");
  for (int i = level; i >= 0; i--)
    {
      snprintf (BUF[1], MSG_SIZE, "\002Level %d\002:", i);
      has_help = 0;
      s->script.cmds.rewind ();
      while ((c = (Script::cmd_type *)s->script.cmds.next ()) != NULL)
        if (c->level == i && (c_char)c->help != NULL)
          {
            has_help = 1;
            snprintf (BUF[1] + strlen (BUF[1]), MSG_SIZE, " %s", (c_char)c->name);
          }
      if (has_help)
        SEND_TEXT (SOURCE, "%s", BUF[1]);
    }
}

// !dcckill dcc_index
void
scriptcmd_dcckill (NetServer *s)
{
  strsplit (CMD[3], BUF, 1);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_DCCKILL);
      return;
    }
  int dcc_index = atoi (BUF[1]);
  if (dcc_index < 0 || dcc_index > DCC_NUM - 1)
    {
      SEND_TEXT (DEST, "Invalid DCC index.");
      return;
    }
  if (DCCS[dcc_index]->status == 5)
    s->script.send_partyline ("%s killed %s.", SOURCE, DCCS[dcc_index]->nick);
  DCCS[dcc_index]->dcc_stop ();
}

// !dcclist
void
scriptcmd_dcclist (NetServer *s)
{
  if (DCC_NUM == 0)
    {
      SEND_TEXT (DEST, "No DCCs running.");
      return;
    }
  for (int i = 0; i < DCC_NUM; i++)
    {
      switch (DCCS[i]->status)
        {
          case DCC_STOP:
            my_strncpy (BUF[1], "closing", MSG_SIZE);
            break;
          case DCC_SEND_INIT:
            my_strncpy (BUF[1], "Send (not connected)", MSG_SIZE);
            break;
          case DCC_SEND:
            my_strncpy (BUF[1], "Send", MSG_SIZE);
            break;
          case DCC_CHAT_INIT:
            my_strncpy (BUF[1], "Chat (not connected)", MSG_SIZE);
            break;
          case DCC_CHAT_AUTH:
            my_strncpy (BUF[1], "Chat (authenticating)", MSG_SIZE);
            break;
          case DCC_CHAT:
            my_strncpy (BUF[1], "Chat", MSG_SIZE);
            break;
          default:
            my_strncpy (BUF[1], "unknown", MSG_SIZE);
        }
      SEND_TEXT (DEST, "%d: DCC %s with %s", i, BUF[1], (c_char)DCCS[i]->mask);
    }
}

// !chat
void
scriptcmd_chat (NetServer *s)
{
  if (USERS.match_mask_level (CMD[0]) < 1)
    {
      SEND_TEXT (SOURCE, "Sorry, you don't have access.");
      return;
    }
  for (int i = 0; i < DCC_NUM; i++)
    if (strcasecmp (DCCS[i]->nick, SOURCE) == 0
        && DCCS[i]->status >= 3 && DCCS[i]->status <= 5)
      {
        SEND_TEXT (SOURCE, "Another DCC Chat is in progress for you.");
        return;
      }
  if (DCC_NUM == DCC_MAX)
    {
      SEND_TEXT (SOURCE, "No free DCC slots available, please try later.");
      return;
    }
  int dcc_index = ((CMD[2][0] >= '0' && CMD[2][0] <= '9') ? atoi (CMD[2]) : -1);
  DCCS[DCC_NUM] = new NetDCC (s, CMD[0], DCC_NUM, dcc_index);
  // if it gives error, work()'ll delete it
  if (DCCS[DCC_NUM]->dcc_chat_start ())
    SEND_TEXT (SOURCE, DCCS[DCC_NUM]->dcc_make_ctcp_chat (BUF[3], MSG_SIZE));
  DCC_NUM++;
}

// !get [file]
void
scriptcmd_get (NetServer *s)
{
  ifstream f ((c_char)s->dcc_file, ios::in | IOS_NOCREATE);
  if (!f)
    {
      SEND_TEXT (SOURCE, "Error opening configuration file \002%s\002: %s", (c_char)s->dcc_file, strerror (errno));
      s->write_botlog ("error opening configuration file %s: %s", (c_char)s->dcc_file, strerror (errno));
      return;
    }
  strsplit (CMD[3], BUF, 1);
  if (BUF[1][0] == 0)		// if none specified, show help
    {
      while (f.getline (BUF[0], MSG_SIZE))
        if (BUF[0][0] == ':')			// if it's help
          SEND_TEXT (SOURCE, "%s", BUF[0]+1);	// +1 to remove the ':'
      return;
    }

  for (int i = 0; i < DCC_NUM; i++)
    if (strcasecmp (DCCS[i]->nick, SOURCE) == 0 &&
        (DCCS[i]->status == 1 || DCCS[i]->status == 2))
      {
        SEND_TEXT (SOURCE, "Another DCC Send is in progress for you.");
        return;
      }
  if (DCC_NUM == DCC_MAX)
    {
      SEND_TEXT (SOURCE, "No free DCC slots available, please try later.");
      return;
    }

  String file (BUF[1], PATH_MAX);	// because BUF will be destroyed

  while (f.getline (BUF[7], MSG_SIZE))
    {
      if (BUF[7][0] == ':' || BUF[7][0] == '#')	// ignore comments
        continue;
      strip_crlf (BUF[7]);
      strsplit (BUF[7], BUF, 2);
      if (strcasecmp (BUF[0], file) != 0)	// it's not this one
        continue;
      int dcc_index = ((CMD[2][0] >= '0' && CMD[2][0] <= '9') ? atoi (CMD[2]) : -1);
      // create the dcc
      DCCS[DCC_NUM] = new NetDCC (s, CMD[0], DCC_NUM, dcc_index);
      // if it gave error, work()'ll delete it
      if (DCCS[DCC_NUM]->dcc_send_start (BUF[1]))
        SEND_TEXT (SOURCE, "%s", DCCS[DCC_NUM]->dcc_make_ctcp_send (BUF[1], MSG_SIZE));
      DCC_NUM++;
      return;
    }
  SEND_TEXT (SOURCE, "Unknown file \002%s\002.", (c_char)file);
}

// look for ctcp dcc chat on privmsg
void
scriptcmd_dcc_event (NetServer *s)
{
  if (CMD[3][0] == '' && strcmp(CMD[1], "PRIVMSG") == 0)
    {
      mask2nick (CMD[0], DEST);
      if (strncmp (CMD[3], "DCC CHAT", 9) == 0)
        {
          s->irc_notice (DEST, "DCC REJECT CHAT chat");
          SEND_TEXT (DEST, "Please use the \002!chat\002 command to start a DCC Chat.");
        }
    }
}

// !randkick's reason, change at will
#define RANDKICK_REASON "9 out of 10 people really hate this kick. how about you?"

// !randkick
void
scriptcmd_randkick (NetServer *s)
{
  if (strcasecmp (CMD[2], s->nick) == 0)
    {
      SEND_TEXT(DEST, "\002!randkick\002 can only be used inside channels.");
      return;
    }
  int i = CHANNEL_INDEX (CMD[2]);
  if (i == -1)
    return;
  struct Channel::user_type *list[C_USERS_MAX];
  int user_num, list_num = 0;

  // build non-ops list
  for (user_num = 0; user_num < CHANNELS[i]->user_num; user_num++)
    if ( ! (CHANNELS[i]->users[user_num]->is_op) )
      list[list_num++] = CHANNELS[i]->users[user_num];

  // if there's at least one
  if (list_num != 0)
    {
      int rand_user = random_num (list_num);
      CHANNELS[i]->irc_kick (list[rand_user]->nick, RANDKICK_REASON);
    }
  else
    SEND_TEXT (DEST, "There's no one to kick.");
}

// !time
void
scriptcmd_time (NetServer *s)
{
  SEND_TEXT (DEST, "%s", get_asctime (s->time_now, s->script.BUF[0], MSG_SIZE));
}

// !rose nick
void
scriptcmd_rose (NetServer *s)
{
  strsplit (CMD[3], BUF, 1);
  if (BUF[1][0] == 0)
    SEND_TEXT (DEST, "%s", HELP_ROSE);
  else
    SEND_TEXT (DEST, "ACTION \002%s\002 offers this 7@3}-,-'-- to the beautiful \002%s\002 :)",
               SOURCE, BUF[1]);
}

// !reverse text
void
scriptcmd_reverse (NetServer *s)
{
  strsplit (CMD[3], BUF, 1);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_REVERSE);
      return;
    }
  char c;
  size_t len = strlen (BUF[1]);
  for (size_t i = 0; i < len/2; i++)
    {
      c = BUF[1][i];
      BUF[1][i] = BUF[1][len-i-1];
      BUF[1][len-i-1] = c;
    }
  SEND_TEXT (DEST, "%s", BUF[1]);
}

// !cop nick [#channel]
void
scriptcmd_cop (NetServer *s)
{
  strsplit (CMD[3], BUF, 3);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_COP);
      return;
    }
  if (BUF[2][0] == 0)
    my_strncpy (BUF[2], CMD[2], MSG_SIZE);
  s->services.chan_op (BUF[2], BUF[1]);
}

// !cdeop nick [#channel]
void
scriptcmd_cdeop (NetServer *s)
{
  strsplit (CMD[3], BUF, 3);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_CDEOP);
      return;
    }
  if (BUF[2][0] == 0)
    my_strncpy (BUF[2], CMD[2], MSG_SIZE);
  if (strcasecmp (BUF[1], s->nick) != 0)
    s->services.chan_deop (BUF[2], BUF[1]);
  else
    SEND_TEXT (DEST, "%s", SCRIPT_HURT_BOT);
}

// !useradd mask level [msg]
void
scriptcmd_useradd (NetServer *s)
{
  strsplit (CMD[3], BUF, 3);
  if (BUF[2][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_USERADD);
      return;
    }
  if (USERS.abs_mask2user (BUF[1]) != NULL)
    {
      SEND_TEXT (DEST, "A user with mask \002%s\002 already exists.", BUF[1]);
      return;
    }
  if (USERS.match_mask_level (CMD[0]) > USERS.match_mask_reallevel (BUF[1]) &&
      USERS.match_mask_level (CMD[0]) > atoi (BUF[2]))
    {
      if (USERS.add_user (BUF[1], NULL, atoi (BUF[2]), BUF[3]))
        SEND_TEXT (DEST, "User added.");
      else
        SEND_TEXT (DEST, "Error adding user.");
    } 
  else
    SEND_TEXT (DEST, "Not enough access level.");
}

// !userdel mask
void
scriptcmd_userdel (NetServer *s)
{
  strsplit (CMD[3], BUF, 2);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_USERDEL);
      return;
    }
  struct ListUsers::user_type *u = USERS.abs_mask2user (BUF[1]);
  if (u == NULL)
    {
      SEND_TEXT (DEST, "User does not exist.");
      return;
    }
  if (USERS.match_mask_level (CMD[0]) > u->level)
    {
      if (USERS.del_user (BUF[1]))
        SEND_TEXT (DEST, "User deleted.");
      else
        SEND_TEXT (DEST, "Error deleting user.");
    }
  else
    SEND_TEXT (DEST, "Not enough access level.");
}

// !userpass mask pass
void
scriptcmd_userpass (NetServer *s)
{
  strsplit (CMD[3], BUF, 3);
  if (BUF[2][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_USERPASS);
      return;
    }
  struct ListUsers::user_type *u = USERS.abs_mask2user (BUF[1]);
  if (u == NULL)
    {
      SEND_TEXT (DEST, "User does not exist.");
      return;
    }
  if (USERS.match_mask_level (CMD[0]) > u->level )
    {
      USERS.abs_set_pass (BUF[1], BUF[2]);
      SEND_TEXT (DEST, "Password of \002%s\002 set to \002%s\002.", BUF[1], BUF[2]);
    }
  else
    SEND_TEXT (DEST, "Not enough access level.");
}

// !usermsg mask msg|<none>
void
scriptcmd_usermsg (NetServer *s)
{
  strsplit (CMD[3], BUF, 2);
  if (BUF[2][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_USERMSG);
      return;
    }
  struct ListUsers::user_type *u = USERS.abs_mask2user (BUF[1]);
  if (u == NULL)
    {
      SEND_TEXT (DEST, "User does not exist.");
      return;
    }
  if (USERS.match_mask_level (CMD[0]) > u->level)
    {
      if (strcasecmp (BUF[2], "<none>") == 0)
        {
          USERS.abs_set_msg (BUF[1], "");
          SEND_TEXT (DEST, "Message \002disabled\002.");
        }
      else
        {
          USERS.abs_set_msg (BUF[1], BUF[2]);
          SEND_TEXT (DEST, "Message of \002%s\002 set to \002%s\002.", BUF[1], BUF[2]);
        }
    } 
  else
    SEND_TEXT (DEST, "Not enough access level.");
}

// !usermask mask newmask
void
scriptcmd_usermask (NetServer *s)
{
  strsplit (CMD[3], BUF, 3);
  if (BUF[2][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_USERMASK);
      return;
    }
  struct ListUsers::user_type *u = USERS.abs_mask2user (BUF[1]);
  if (u == NULL)
    {
      SEND_TEXT (DEST, "User does not exist.");
      return;
    }
  if (USERS.match_mask_level (CMD[0]) > u->level)
    {
      USERS.abs_set_mask (BUF[1], BUF[2]);
      SEND_TEXT (DEST, "Mask \002%s\002 changed to \002%s\002.", BUF[1], BUF[2]);
    }
  else
    SEND_TEXT (DEST, "Not enough access level.");
}

// !userlevel mask level
void
scriptcmd_userlevel (NetServer *s)
{
  strsplit (CMD[3], BUF, 3);
  if (BUF[2][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_USERLEVEL);
      return;
    }
  struct ListUsers::user_type *u = USERS.abs_mask2user (BUF[1]);
  if (u == NULL)
    {
      SEND_TEXT (DEST, "User does not exist.");
      return;
    }
  if (atoi (BUF[2]) >= LEVEL_MIN && atoi (BUF[2]) <= LEVEL_MAX)
    {
      if (USERS.match_mask_level (CMD[0]) > u->level
          && USERS.match_mask_level (CMD[0]) > atoi (BUF[2]))
        {
          USERS.abs_set_level (BUF[1], atoi (BUF[2]));
          SEND_TEXT (DEST, "Level of \002%s\002 set to \002%s\002.", BUF[1], BUF[2]);
        }
      else
        SEND_TEXT (DEST, "Not enough access level.");
    }
  else
    SEND_TEXT (DEST, "Invalid level.");
}

// !id
void
scriptcmd_id (NetServer *s)
{
  StringFixed str (MSG_SIZE);
  struct ListUsers::user_type *u;
  USERS.rewind ();
  while ((u = (struct ListUsers::user_type *)USERS.next ()) != NULL)
    {
      str += " ";
      str += u->cur_mask;
    }
  SEND_TEXT (DEST, "Identified users:%s", (c_char)str);
}

// !userlist
void
scriptcmd_userlist (NetServer *s)
{
  bool has_user;
  struct ListUsers::user_type *u;
  SEND_TEXT (SOURCE, "Users:");
  for (int i = LEVEL_MAX; i >= LEVEL_MIN; i--)
    {
      has_user = 0;
      snprintf (BUF[1], MSG_SIZE, "\002Level %d\002:", i);
      USERS.rewind ();
      while ((u = (struct ListUsers::user_type *)USERS.next ()) != NULL)
        if (u->level == i)
          if (strlen (BUF[1]) + u->mask.getlen () > MSG_SIZE)
            {
              SEND_TEXT (SOURCE, "%s", BUF[1]);
              my_strncpy (BUF[1], u->mask, MSG_SIZE);
            }
          else
            {
              snprintf (BUF[1] + strlen (BUF[1]), MSG_SIZE - strlen (BUF[1]),
                        " %s", (c_char)u->mask);
              has_user = 1;
            }
      if (has_user)
        SEND_TEXT (SOURCE, "%s", BUF[1]);
    }
}

// !setpass password
void
scriptcmd_setpass (NetServer *s)
{
  if (DEST[0] == '#')
    {
      SEND_TEXT (DEST, "Do \002not\002 use this command on a channel.");
      return;
    }
  strsplit (CMD[3], BUF, 2);
  if (BUF[1][0] == 0)
    SEND_TEXT (DEST, "%s", HELP_SETPASS);
  else
    {
      USERS.match_set_pass (CMD[0], BUF[1]);
      SEND_TEXT (DEST, "Password set to \002%s\002 - remember it for later use.", BUF[1]);
    }
}

// !setmsg msg | <none>
void
scriptcmd_setmsg (NetServer *s)
{
  strsplit (CMD[3], BUF, 1);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_SETMSG);
      return;
    }
  if (strcasecmp (BUF[1], "<none>") == 0)
    {
      USERS.match_set_msg (CMD[0], "");
      SEND_TEXT (DEST, "Message \002disabled\002.");
    }
  else
    {
      USERS.match_set_msg (CMD[0], BUF[1]);
      SEND_TEXT (DEST, "Message set to '%s'.", BUF[1]);
    }
}

// !pass password
void
scriptcmd_pass (NetServer *s)
{
  if (DEST[0] == '#')
    {
      SEND_TEXT (DEST, "Do \002not\002 use this command on a channel.");
      return;
    }
  strsplit (CMD[3], BUF, 1);
  if (BUF[1][0] == 0)
    {
      SEND_TEXT (DEST, "%s", HELP_PASS);
      return;
    }
  if (USERS.match_check_pass (CMD[0], BUF[1]))
    {
      USERS.match_set_id (CMD[0], 1);
      mask2nick (CMD[0], BUF[2]);
      s->irc_watch (BUF[2], 1);
      SEND_TEXT (DEST, "Password correct.");
    }
  else
    SEND_TEXT (DEST, "Password \002in\002correct.");
}

// !level
void
scriptcmd_level (NetServer *s)
{
  snprintf (BUF[1], MSG_SIZE, "Your access level is \002%d\002",
            USERS.match_mask_reallevel (CMD[0]));
  if (USERS.match_mask_reallevel (CMD[0]) >= 1)
    {
      if (USERS.match_mask_level (CMD[0]) > 1)
        snprintf (BUF[1] + strlen (BUF[1]), MSG_SIZE - strlen (BUF[1]),
                  ", identified");
      else
        snprintf (BUF[1] + strlen (BUF[1]), MSG_SIZE - strlen (BUF[1]),
                  ", \002un\002identified");
    }
  SEND_TEXT (DEST, "%s.", BUF[1]);
}

void
scriptcmd_ctcp_var (NetServer *s, c_char name, char *data, size_t n)
{
  if (strcasecmp (name, "ctcp_version") == 0)
    {
      if (n != 0)
        my_strncpy (data, s->script.ctcp_version, n);
      else
        s->script.ctcp_version = data;
    }
  else if (strcasecmp (name, "ctcp_finger") == 0)
    {
      if (n != 0)
        my_strncpy (data, s->script.ctcp_finger, n);
      else
        s->script.ctcp_finger = data;
    }
  else if (strcasecmp (name, "ctcp_source") == 0)
    {
      if (n != 0)
        my_strncpy (data, s->script.ctcp_source, n);
      else
        s->script.ctcp_source = data;
    }
}
