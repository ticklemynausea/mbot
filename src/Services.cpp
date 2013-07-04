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

#include "Services.h"

Services::Services (NetServer *server) : exist (0), identified (0),
  services_privmsg (0), nickserv_pass (MSG_SIZE), nickserv_mask (MASK_SIZE),
  nickserv_auth (MSG_SIZE), s (server)
{
}

void
Services::irc_services (c_char service, c_char msg) const
{
  if (services_privmsg)
    s->irc_privmsg (service, "%s", msg);
  else
    s->writeln ("%s %s", service, msg);
}

void
Services::irc_nickserv (c_char msg) const
{
  irc_services (NICKSERV, msg);
}

void
Services::nick_identify (void)
{
  snprintf (buf, MSG_SIZE, "identify %s", (c_char)nickserv_pass);
  irc_nickserv (buf);
}

void
Services::irc_chanserv (c_char msg) const
{
  irc_services (CHANSERV, msg);
}

void
Services::chan_invite (c_char channel)
{
  snprintf (buf, MSG_SIZE, "invite %s", (c_char)channel);
  irc_chanserv (buf);
}

void
Services::chan_unban (c_char channel)
{
  snprintf (buf, MSG_SIZE, "unban %s", (c_char)channel);
  irc_chanserv (buf);
}

void
Services::chan_op (c_char channel, c_char nick)
{
  snprintf (buf, MSG_SIZE, "op %s %s", (c_char)channel, (c_char)nick);
  irc_chanserv (buf);
}

void
Services::chan_deop (c_char channel, c_char nick)
{
  snprintf (buf, MSG_SIZE, "deop %s %s", (c_char)channel, (c_char)nick);
  irc_chanserv (buf);
}

