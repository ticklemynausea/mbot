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

#ifndef _H_SCRIPTCMD
#define _H_SCRIPTCMD

#include "mbot.h"

#define LEVEL_MODLIST 10
#define LEVEL_MODADD 10
#define LEVEL_MODDEL 10
#define LEVEL_QUIT 10
#define LEVEL_SERVER 10
#define LEVEL_SET 10
#define LEVEL_LOADCONF 10

#define LEVEL_AWAY 10
#define LEVEL_NICK 10
#define LEVEL_UNBIND 10
#define LEVEL_CMDLEVEL 10
#define LEVEL_JOIN 9
#define LEVEL_PART 9
#define LEVEL_STATUS 7
#define LEVEL_TOPIC 7
#define LEVEL_OPME 5
#define LEVEL_OP 5
#define LEVEL_DEOP 5
#define LEVEL_KICK 5
#define LEVEL_BAN 5
#define LEVEL_UNBAN 5
#define LEVEL_BK 5
#define LEVEL_INVITE 5
#define LEVEL_VOICE 3
#define LEVEL_DEVOICE 3
#define LEVEL_SAY 2
#define LEVEL_HELP 0

#define LEVEL_DCCKILL 9
#define LEVEL_DCCLIST 5
#define LEVEL_CHAT 0
#define LEVEL_GET 0

#define LEVEL_RANDKICK 5
#define LEVEL_TIME 0
#define LEVEL_ROSE 0
#define LEVEL_REVERSE 0

#define LEVEL_COP 7
#define LEVEL_CDEOP 7

#define LEVEL_USERADD 9
#define LEVEL_USERDEL 9
#define LEVEL_USERPASS 9
#define LEVEL_USERMSG 9
#define LEVEL_USERMASK 9
#define LEVEL_USERLEVEL 9
#define LEVEL_ID 7
#define LEVEL_USERLIST 3
#define LEVEL_SETPASS 2
#define LEVEL_SETMSG 2
#define LEVEL_PASS 0
#define LEVEL_LEVEL 0

#define HELP_MODLIST "List all loaded modules."
#define HELP_MODADD "!modadd path - Load the module at the given <path>."
#define HELP_MODDEL "!moddel name - Unload the module with <name>, this must be a name returned by !modlist"
#define HELP_QUIT "!quit [msg] - Make all bots in the current process quit with <msg>, else uses the default."
#define HELP_SERVER "!server next | <hostname [<port>]> - Make the bot jump to the next server in the server list, or to the specified <hostname> and <port>."
#define HELP_SET "!set option [value] - If <value> is specified, sets <option> to it. Otherwise just show the content of <option>."
#define HELP_LOADCONF "!loadconf [filename] - Reload the current configuration file or use <filename>. Note that the changes only take effect after reloading the module(s). USE WITH CARE."

#define HELP_AWAY "!away [msg] - Put the bot away with <msg>, or without parameters, remove the away."
#define HELP_NICK "!nick nick - Change the bot's nick to <nick>."
#define HELP_UNBIND "!unbind !command - Disable the given <!command>. Note that it can only be recovered by restarting the bot or reloading that command's module."
#define HELP_CMDLEVEL "!cmdlevel !command level - Set <!command>'s required userlevel to <level>."
#define HELP_JOIN "!join #channel - Add <#channel> to the channel list."
#define HELP_PART "!part #channel - Remove <#channel> from the channel list."
#define HELP_STATUS "Show some bot status."
#define HELP_TOPIC "!topic [#channel] text - Change the topic in the current or in the specified <#channel>, to <text>."
#define HELP_OPME "Give op to who executed the command. Can only be used inside a channel."
#define HELP_OP "!op nick [#channel] - Give op to <nick> on the specified <#channel>."
#define HELP_DEOP "!deop nick [#channel] - Take op from <nick> on the specified <#channel>."
#define HELP_KICK "!kick nick [#channel] [reason] - Kick <nick> on the specified <#channel>, with <reason>."
#define HELP_BAN "!ban nick | mask [#channel] - Ban <nick> or <mask> on the specified <#channel>."
#define HELP_UNBAN "!unban nick | mask [#channel] - Unban <nick> or <mask> on the specified <#channel>."
#define HELP_BK "!bk nick [#channel] - Deop, ban and kick <nick> on the specified <#channel>."
#define HELP_INVITE "!invite nick [#channel] - Invite <nick> to <#channel>."
#define HELP_VOICE "!voice nick [#channel] - Give voice to <nick> on the specified <#channel>."
#define HELP_DEVOICE "!devoice nick [#channel] - Take voice from <nick> on the specified <#channel>."
#define HELP_SAY "!say [#channel | $nick] text - Send <text> to a <#channel> or <nick>."

#define HELP_DCCKILL "!dcckill dcc_index - Terminate the DCC specified by <dcc_index>, use !dcclist to see the indexes."
#define HELP_DCCLIST "Show the list of active DCCs (Chat and Send)."
#define HELP_CHAT "Make the bot start a DCC Chat with who executed it."
#define HELP_GET "!get [file] - Without parameters, show the files available through DCC Send, else requests the specified <file>."

#define HELP_RANDKICK "Kick a random non-op user where the command is made."
#define HELP_TIME "Show current time and date."
#define HELP_ROSE "!rose nick - Show a rose to <nick>."
#define HELP_REVERSE "!reverse text - Show <text> with the letters reversed."

#define HELP_COP "!cop nick [#channel] - Use chanserv to op <nick> in the specified <#channel>."
#define HELP_CDEOP "!cdeop nick [#channel] - Use chanserv to deop <nick> in the specified <#channel>."

#define HELP_USERADD "!useradd mask level [msg] - Add a user with <mask> and <level>. If <msg> is not specified, turns off the join msg. If <mask> already exists, overwrites the old one."
#define HELP_USERDEL "!userdel mask - Delete the user with <mask>."
#define HELP_USERPASS "!userpass mask pass - Change user's password with <mask> to <pass>."
#define HELP_USERMSG "!usermsg mask msg | <none> - Change user's join msg with <mask> to <msg>, or turns it off with \"<none>\"."
#define HELP_USERMASK "!usermask mask newmask - Change user's mask from <mask> to <newmask>."
#define HELP_USERLEVEL "!userlevel mask level - Change user's access level with <mask> to <level>."
#define HELP_ID "Show the currently identified users with the bot."
#define HELP_USERLIST "Show the list of registered users in the bot."
#define HELP_SETPASS "!setpass password - Change to <password> the password of who executed it."
#define HELP_SETMSG "!setmsg msg | <none> - Change the join msg of who executed it to <msg>, or turn it off with \"<none>\"."
#define HELP_PASS "!pass password - Authenticate with the bot. Cannot be used inside a channel."
#define HELP_LEVEL "Show the access level of who executed it."

void scriptcmd_modlist (NetServer *);
void scriptcmd_modadd (NetServer *);
void scriptcmd_moddel (NetServer *);
void scriptcmd_quit (NetServer *);
void scriptcmd_server (NetServer *);
void scriptcmd_set (NetServer *);
void scriptcmd_loadconf (NetServer *);

void scriptcmd_away (NetServer *);
void scriptcmd_nick (NetServer *);
void scriptcmd_unbind (NetServer *);
void scriptcmd_cmdlevel (NetServer *);
void scriptcmd_join (NetServer *);
void scriptcmd_part (NetServer *);
void scriptcmd_status (NetServer *);
void scriptcmd_topic (NetServer *);
void scriptcmd_opme (NetServer *);
void scriptcmd_deopme (NetServer *);
void scriptcmd_op (NetServer *);
void scriptcmd_deop (NetServer *);
void scriptcmd_kick (NetServer *);
void scriptcmd_ban (NetServer *);
void scriptcmd_unban (NetServer *);
void scriptcmd_bk (NetServer *);
void scriptcmd_invite (NetServer *);
void scriptcmd_voice (NetServer *);
void scriptcmd_devoice (NetServer *);
void scriptcmd_say (NetServer *);
void scriptcmd_help (NetServer *);

void scriptcmd_dcclist (NetServer *);
void scriptcmd_dcckill (NetServer *);
void scriptcmd_chat (NetServer *);
void scriptcmd_get (NetServer *);
void scriptcmd_dcc_event (NetServer *);

void scriptcmd_randkick (NetServer *);
void scriptcmd_time (NetServer *);
void scriptcmd_rose (NetServer *);
void scriptcmd_reverse (NetServer *);

void scriptcmd_cop (NetServer *);
void scriptcmd_cdeop (NetServer *);

void scriptcmd_useradd (NetServer *);
void scriptcmd_userdel (NetServer *);
void scriptcmd_userpass (NetServer *);
void scriptcmd_usermsg (NetServer *);
void scriptcmd_usermask (NetServer *);
void scriptcmd_userlevel (NetServer *);
void scriptcmd_id (NetServer *);
void scriptcmd_userlist (NetServer *);
void scriptcmd_setpass (NetServer *);
void scriptcmd_setmsg (NetServer *);
void scriptcmd_pass (NetServer *);
void scriptcmd_level (NetServer *);

void scriptcmd_ctcp_var (NetServer *, c_char, char *, size_t);

#endif
