/**************************************************

         mbot  -  mirage irc bot
         
***************************************************

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

#include "mbot.h"

Bot* bot;

void
mbot_exit (void)
{
  delete bot;
  exit (0);
}

void
handler_sigterm (int)
{
  for (int i = 0; i < bot->server_num; i++)
    bot->servers[i]->irc_quit ("received SIGTERM");
  bot->write_botlog ("FATAL: received SIGTERM");
  mbot_exit ();
}

void
handler_sigsegv (int)
{
  static bool double_segfault = 0;
  if (double_segfault)
    exit (1);
  double_segfault = 1;
#ifndef WINDOZE
  if (fork() > 0)
    raise (SIGQUIT);		// to dump core
#endif		// !WINDOZE
  fflush (NULL);
  for (int i = 0; i < bot->server_num; i++)
    bot->servers[i]->irc_quit ("received SIGSEGV");
  bot->write_botlog ("FATAL: received SIGSEGV");
  mbot_exit ();
}

void
handler_sigfpe (int)
{
#ifndef WINDOZE
  if (fork() > 0)
    raise (SIGQUIT);		// to dump core
#endif		// !WINDOZE
  fflush (NULL);
  for (int i = 0; i < bot->server_num; i++)
    bot->servers[i]->irc_quit ("received SIGFPE");
  bot->write_botlog ("FATAL: received SIGFPE");
  mbot_exit ();
}

int 
main (int argc, char *argv[])
{
  cout << setiosflags (ios::left);	// just to make seen.so load
  cout << "\n  " << VERSION_STRING << "\n\n";

  random_init ();

  signal (SIGTERM, handler_sigterm);
#ifndef NDEBUG		// WINDOZE
  signal (SIGSEGV, handler_sigsegv);
#endif	// !NDEBUG
  signal (SIGFPE, handler_sigfpe);
#ifndef WINDOZE
  signal (SIGHUP, SIG_IGN);
  signal (SIGPIPE, SIG_IGN);
#endif // !WINDOZE

  bot = new Bot ();

  bot->parse_cmd (argc, argv);
  bot->parse_conf ();
  bot->check_pid ();
  bot->server_info ();

  if (!bot->debug)
    {
#ifdef WINDOZE
      cout << "Running with PID " << getpid () << ".\n" << endl;
#else		// !WINDOZE
      cout << "Daemonizing mbot.\n\n";
      if (!daemonize ())
        {
          cout << "error: can't daemonize into background: " << strerror (errno) << "\n\n";
          exit (1);
        }
#endif		// !WINDOZE
    }

#ifdef WINDOZE
  WSADATA WSData;
  if (WSAStartup (MAKEWORD (2,0), &WSData))
    {
      bot->write_botlog ("error: winsock initialization failed");
      exit (1);
    }
#endif		// WINDOZE

  bot->write_pid ();

  bot->work ();
  return 0;
}

