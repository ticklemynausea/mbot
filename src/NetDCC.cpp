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

#include "NetDCC.h"

NetDCC::NetDCC (NetServer *server, c_char dcc_mask, int dcc_index,
  int dcc_from) : Net (), mask (dcc_mask, MASK_SIZE), filename (PATH_MAX)
{
  s = server;
  mask2nick (mask, nick);
  index = dcc_index;
  status = DCC_STOP;
  last_time = server->time_now;
  socklocal = -1;
  dfd = NULL;
  filesent = 0;
  dcc_from_index = dcc_from;
  if (bot->debug)
    cout << "New DCC #" << index << " from " << mask << endl;
}

NetDCC::~NetDCC (void)
{
  dcc_stop ();
}

// stop the dcc and show the error to the nick
void
NetDCC::dcc_error (c_char error)
{
  dcc_stop ();
  s->irc_privmsg (nick, "%s", error);
} 

// open the socket, return 0 on error
bool
NetDCC::dcc_chat_start (void)
{
  if (!bindsock (s->dcc_port))
    {
      s->irc_privmsg (nick, "Error in DCC Chat: can't bind socket, try again later.");
      return 0;
    }
  socklocal = sock;
  status = DCC_CHAT_INIT;
  last_time = s->time_now;
  return 1;
}

// open socket and file, return 0 if there's an error with any of them
bool
NetDCC::dcc_send_start (c_char file)
{
  static char errorfile[] = "Error in DCC Send: can't open file.";
  static char errorbind[] = "Error in DCC Send: can't bind socket.";
  filename = file;
  dfd = fopen (filename, "r");
  if (dfd == NULL)
    {
      if (dcc_from_index == -1)
        s->irc_privmsg (nick, "%s", errorfile);
      else
        s->dccs[dcc_from_index]->dcc_chat_write (errorfile);
      return 0;
    }
  if (!bindsock (s->dcc_port))
    {
      fclose (dfd);
      dfd = NULL;
      if (dcc_from_index == -1)
        s->irc_privmsg (nick, "%s", errorbind);
      else
        s->dccs[dcc_from_index]->dcc_chat_write (errorbind);
      return 0;
    }
  socklocal = sock;
  filesize = lseek (fileno (dfd), 0, SEEK_END);
  fseek (dfd, 0, SEEK_SET);
  status = DCC_SEND_INIT;
  last_time = s->time_now;
  return 1;
}

// check when it can start sendind or chating
void
NetDCC::dcc_start_check (void)
{
  int i = readok (socklocal);
  if (i == -1)						// in case of error
    {
      dcc_error ("Error in starting DCC.");
      return;
    }
  if (i) 
    {
      socklen_t size = sizeof (struct sockaddr_in);
      struct sockaddr_in remote;
      sock = accept (socklocal, (struct sockaddr *)&remote, (socklen_t *)&size);
#ifdef WINDOZE
      closesocket (socklocal);
#else		// !WINDOZE
      close (socklocal);
#endif		// !WINDOZE
      socklocal = -1;
      sock_linger (sock, 5*100);
	  blocksock (0);

      if (status == DCC_CHAT_INIT)	// if it's a dcc chat, ask for password
        {
          dcc_chat_write ("Enter your password.");
          status = DCC_CHAT_AUTH;
        }
      else
        status = DCC_SEND;

      last_time = s->time_now;
    }
  if (difftime(s->time_now, last_time) > 50)	// timeout in 50 seconds
    dcc_error ("Timeout waiting for connection.");
}

void
NetDCC::dcc_chat_write (c_char msg)
{
  if (status != DCC_STOP)			// sometimes it happens
    {
      if (writeok (sock) == -1)
        dcc_error ("Error during DCC Chat.");
      else
        writeln ("%s", msg);
    }
}

void
NetDCC::dcc_chat_pass (void)
{
  int i = readln ();
  if (i == -1)
    {
      dcc_stop ();		// probably the client closed the dcc
      return;
    }
  if (i)
    {
      // check user's password
      if (!s->users.match_check_pass (mask, bufread))
        {
          dcc_chat_write ("Password incorrect.");
          dcc_stop ();
          return;
        }

      // activate user's id
      s->users.match_set_id (mask, 1);
      s->irc_watch (nick, 1);

      // show motd, if one is defined
      if (s->dcc_motd != NULL)
        {
          s->dcc_motd->rewind_text ();
          c_char bufline;
          while ((bufline = s->dcc_motd->get_line ()) != NULL)
            dcc_chat_write (bufline);
        } 
      // otherwise just say this
      else
        dcc_chat_write ("Password correct.");

      s->script.send_partyline ("%s joined the partyline.", nick);

      // dcc chat mode
      status = DCC_CHAT;
      last_time = s->time_now;
    }
  if (difftime (s->time_now, last_time) > 60)	// timeout in one minute
    {
      dcc_chat_write ("Closing idle DCC Chat.");
      dcc_stop ();
    }
}

void
NetDCC::dcc_chat (void)
{
  int i = readln ();
  if (i == -1)
    {
      dcc_stop ();		// probably the client closed the dcc
      return;
    }
  if (i)
    {
      // fake a privmsg
      char buf2[20];		// should do to keep dcc chat's index
      snprintf (s->bufread, MSG_SIZE, ":%s PRIVMSG %s :%s", (c_char)mask,
                itoa (index, buf2, 19), bufread);
      s->irc_parse ();
      s->script.irc_event ();
      last_time = s->time_now;
    }
  if (difftime (s->time_now, last_time) > 600)	// timeout in ten minutes
    {
      dcc_chat_write ("Closing idle DCC Chat.");
      dcc_stop ();
    }
}

void
NetDCC::dcc_send (void)
{
  int i = writeok (sock);
  if (i == -1)
    {
      dcc_error ("Error during DCC Send.");
      return;
    }
  if (i)
    {
      i = fread (dcc_buf, 1, DCC_BUF_SIZE, dfd);

      if (readok (sock) != 0)			// if it's 1 or -1
        if (recv (sock, buf, sizeof (buf), 0) == -1)
          {
            dcc_stop ();	// probably the client closed the dcc
            return;
          }

      if (send (sock, dcc_buf, i, 0) != -1)
        {
          filesent += i;
          if (filesent == filesize)
            dcc_stop ();
          last_time = s->time_now;
        }
      else
        dcc_error ("Error during DCC Send.");
    }
  if (difftime (s->time_now, last_time) > 30)  // timeout in 30 seconds
    dcc_error ("Timeout in DCC Send.");
}

// close sockets and file, if open
void
NetDCC::dcc_stop (void)
{
  if (status == DCC_CHAT)			// if it was a dcc chat
    {
      status = DCC_STOP; // must be changed right now to avoid a possible loop
      s->script.send_partyline ("%s left the partyline.", nick);
    }
  if (socklocal != -1)
    {
#ifdef WINDOZE
      closesocket (socklocal);
#else		// !WINDOZE
      close (socklocal);
#endif		// !WINDOZE
      socklocal = -1;
    }
  closesock ();
  if (dfd != NULL)
    {
      fclose (dfd);
      dfd = NULL;
    }
  filesent = 0;
  status = DCC_STOP;
}

// manage the dcc, through its status
bool
NetDCC::dcc_work (void)
{
  switch (status)
    {
      case DCC_STOP:
        return 0;
      case DCC_SEND_INIT:
        dcc_start_check ();
        break;
      case DCC_SEND:
        dcc_send ();
        break;
      case DCC_CHAT_INIT:
        dcc_start_check ();
        break;
      case DCC_CHAT_AUTH:
        dcc_chat_pass ();
        break;
      case DCC_CHAT:
        dcc_chat ();
        break;
      default:			// should never happen
        status=DCC_STOP;
    }
  return 1;
}

// build the ctcp dcc chat
char *
NetDCC::dcc_make_ctcp_chat (char *ctcp_buf, size_t max)
{
  char portbuf[7];
  char addrbuf[12];

  snprintf (ctcp_buf, max, "DCC CHAT CHAT %s %s",
            utoa (sock2addr (s->sock), addrbuf, 11),
            utoa (sock2port (socklocal), portbuf, 6));

  return ctcp_buf;
}

// build the ctcp dcc send
char *
NetDCC::dcc_make_ctcp_send (char *ctcp_buf, size_t max)
{
  char portbuf[12], addrbuf[12];

  snprintf (ctcp_buf, max, "DCC SEND %s %s %s %s",
            fullpath2file (filename),
            utoa (sock2addr (s->sock), addrbuf, 11),
            utoa (sock2port (socklocal), portbuf, 11),
            utoa (filesize, buf, 11));

  return ctcp_buf;
}

