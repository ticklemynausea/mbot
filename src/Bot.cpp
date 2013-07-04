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

#include "Bot.h"

Bot::Bot (void) : conf_file (CONF_FILE, PATH_MAX),
  pid_file (PID_FILE, PATH_MAX)
{
  line = 0;
  debug = 0;
  for (int i = 0; i < SERVER_MAX; i++)
    servers[i] = NULL;
  server_num = 0;
  time_now = get_time ();
  log_list = NULL;
}

Bot::~Bot (void)
{
  // delete modules
  Module *m;
  modules.rewind ();
  while ((m = (Module *)modules.next ()) != NULL)
    {
      delete m;
    }
  // delete servers
  for (int i = 0; i < server_num; i++)
    delete servers[i];
  server_num = 0;

  log_delall ();
  remove (pid_file);
  write_botlog ("bot exited");
}

// create log with <name>, return it or NULL on error
Log *
Bot::log_add (c_char name)
{
  struct log_type *log = new struct log_type;
  if (log == NULL)
    return NULL;
  log->l = new Log (this);
  if (log->l == NULL)
    return NULL;
  log->name = name;
  log->next = log_list;
  log_list = log;
  return log_list->l;
}

// return the log with <name> or NULL if not found
Log *
Bot::log_get (c_char name)
{
  struct log_type *log = log_list;
  while (log != NULL)
    {
      if (log->name == name)
        return log->l;
      log = log->next;
    }
  return NULL;
}

// close all logs
void
Bot::log_delall (void)
{
  struct log_type *log = log_list, *lognext;
  while (log != NULL)
    {
      lognext = log->next;
      delete log->l;
      delete log;
      log = lognext;
    }
}

// write <msg> to botlog
void 
Bot::write_botlog (c_char format, ...)
{
  char st_time[TIME_SIZE+1];
  get_asctime (time_now, st_time, TIME_SIZE);
  cerr << st_time << " ";
  va_list args;
  va_start (args, format);
  char buf[MSG_SIZE+1];
  vsnprintf (buf, MSG_SIZE, format, args);
  va_end (args);
  cerr << buf << endl;
}

// create a log and load it, return its pointer
Module *
Bot::add_module (c_char filename)
{
  Module *m_temp = new Module (filename);
  if (m_temp == NULL)
    return NULL;
  if (!m_temp->initialized)
    {
      delete m_temp;
      return NULL;
    }
  modules.add ((void *)m_temp);
  return m_temp;
}

// return the module with <name>, NULL if nonexistant
Module *
Bot::get_module (c_char name)
{
  Module *m;
  modules.rewind ();
  while ((m = (Module *)modules.next ()) != NULL)
    if (strcmp (*m, name) == 0)
      return m;
  return NULL;
}

// delete a module from the bot's module list and unload it, return 1 if ok
bool
Bot::del_module (c_char name)
{
  Module *m = get_module (name);
  if (m == NULL)
    return 0;
  delete m;
  modules.del ((void *)m);
  return 1;
}

// command line parser
void
Bot::parse_cmd (int argc, char *argv[])
{
  int i, id;
  for (i = 1; i < argc; i++)
    while (1)
      {
        if ((strcmp (argv[i], "-c") == 0)
            || (strcmp (argv[i], "--conf") == 0))
          {
            if (argv[++i] == NULL)
              {
                cout << "error: missing file for -c\n\n";
                exit (1);
              }
            conf_file = argv[i];
            break;
          }
        else if ((strcmp (argv[i], "-h") == 0)
                 || (strcmp (argv[i], "--help") == 0))
          {
            cout << " Options:\n"
                 << "  -h,  --help               this help\n"
                 << "  -c,  --conf <file>        load configuration file\n"
                 << "  -d,  --debug              enter debug mode\n"
                 << "  -v,  --version            show version and exit\n"
#if HAS_SETUGID
                 << "  -u,  --uid <username>     set user identity of process\n"
                 << "  -g,  --gid <group>        set group identity of process\n"
#endif
                 << "\n Report bugs to <mirage@kaotik.org>\n";
            exit (0);
          }
        else if ((strcmp (argv[i], "-d") == 0)
                 || (strcmp (argv[i], "--debug") == 0))
          {
            debug = 1;
            break;
          }
        else if ((strcmp (argv[i], "-v") == 0)
                 || (strcmp (argv[i], "--version") == 0))
          {
            cout << "  Copyright (C) 1998-2004 Tiago Sousa <mirage@kaotik.org> (under the GNU GPL)\n\n";
            exit (0);
          }
#if HAS_SETUGID
        else if ((strcmp (argv[i], "-u") == 0)
                 || (strcmp (argv[i], "--uid") == 0))
          {
            if (argv[++i] == NULL)
              {
                cout << "error: missing option for -u\n\n";
                exit (1);
              }
            if (argv[i][0] >= '0' && argv[i][0] <= '9')
              id = atoi (argv[i]);
            else
              {
                struct passwd *pwd = getpwnam (argv[i]);
                if (pwd == NULL)
                  {
                    cout << "error: user '" << argv[i] << "' unknown\n\n";
                    exit (1);
                  }
                id = pwd->pw_uid;
              }
            if (setuid (id) == -1)
              {
                cout << "error: failed setuid(" << id << ")\n\n";
                exit (1);
              }
            break;
          }
        else if ((strcmp (argv[i], "-g") == 0)
                 || (strcmp (argv[i], "--gid") == 0))
          {
            if (argv[++i] == NULL)
              {
                cout << "error: missing option for -g\n\n";
                exit (1);
              }
            if (argv[i][0] >= '0' && argv[i][0] <= '9')
              id = atoi (argv[i]);
            else
              {
                struct group *pwd = getgrnam (argv[i]);
                if (pwd == NULL)
                  {
                    cout << "error: group '" << argv[i] << "' unknown\n\n";
                    exit (1);
                  }
                id = pwd->gr_gid;
              }
            if (setgid (id) == -1)
              {
                cout << "error: failed setgid(" << id << ")\n\n";
                exit (1);
              }
            break;
          }
#endif	// HAS_SETUGID
        cerr << "error: unknown option \"" << argv[i] << "\", try --help for help.\n\n",
        exit (1);
      }
}

// check if other bot is running
void
Bot::check_pid (void)
{
  ifstream f ((c_char)pid_file, ios::in | IOS_NOCREATE);
  if (f)
    {
      pid_t pid;
      f >> pid;
#ifdef WINDOZE
      HANDLE handle = OpenProcess (0, FALSE, pid);
	  if (handle != NULL)
#else		// !WINDOZE
      kill (pid, SIGCHLD);
      if (errno != ESRCH)
#endif		// !WINDOZE
        {
          cout << "error: another copy of the bot is running\n\n";
          exit (1);
        }
    }
}

// save file with the pid
void
Bot::write_pid (void)
{
  ofstream f ((c_char)pid_file);
  if (f)
    f << getpid () << endl;
  else
    {
      write_botlog ("error: can't write to %s: %s", (c_char)pid_file, strerror (errno));
      exit (1);
    }
}
  
// show each server's info
void
Bot::server_info (void)
{
  NetServer *s;
  for (int i = 0; i < server_num; i++)
    {
      s = servers[i];
      cout << "  " << s->host_current->host << " " << s->host_current->port
           << "\n  " << s->nick << "!" << s->user << "(" << s->name
           << ")\n  " << s->users.count ()
           << " users, " << s->channel_num << "/" << CHANNEL_MAX
           << " channels\n\n";
    }
}

// show the error and exit
void
Bot::conf_error (c_char error)
{
  write_botlog ("error in configuration (line %d): %s", line, error);
  exit (1);
}

// show the warning
void
Bot::conf_warn (c_char warning)
{
  write_botlog ("warning in configuration (line %d): %s", line, warning);
}

// check that basic parameters of a server are specified 
void
Bot::check_server (NetServer *s)
{
  if (s != NULL && s->hosts == NULL)
    conf_error ("no hosts were defined, can't continue");
}

// configuration line parser
void
Bot::parse_conf (void)
{
  c_char bufline;
  char buf[10][MSG_SIZE+1];
  server_num = -1;
  NetServer *s = NULL;
  bool changed = 0, bot_defined = 0;

  if (!conf.read_file (conf_file))
    conf_error ("can't open configuration file");
  while ((bufline = conf.get_line ()) != NULL)
    {
      line++;
      if (bufline[0] == 0 || bufline[0] == '#')
        continue;
      strsplit (bufline, buf, 2);
      changed = 0;

      if (strcasecmp (buf[0], "pid") == 0)
        {
          pid_file = buf[1];
          changed = 1;
        }

      else if (strcasecmp (buf[0], "debug") == 0)
        debug = changed = 1;

      else if (strcasecmp (buf[0], "module") == 0)
        {
          if (!add_module (buf[1]))
            conf_error ("can't load module");
          changed = 1;
        }

      else if (strcasecmp (buf[0], "bot") == 0)
        {
          check_server (s);
          if (server_num + 1 < SERVER_MAX)
            {
              servers[++server_num] = new NetServer ();
              s = servers[server_num];
            }
          else
            conf_error ("maximum servers exceeded");
          changed = bot_defined = 1;
          s->vars.var_add ("ctcp_version", scriptcmd_ctcp_var);
          s->vars.var_add ("ctcp_finger", scriptcmd_ctcp_var);
          s->vars.var_add ("ctcp_source", scriptcmd_ctcp_var);
        }

      else if (strcasecmp (buf[0], "botlog") == 0)
        {
          if (freopen (buf[1], "a", stderr) == NULL)
            {
              // conf_error() doesn't work because stderr is now closed
              cout << "can't open botlog's file for writing (" << buf[1] << ")\n";
              exit (1);
            }
          changed = 1;
        }

      else if (strcasecmp (buf[0], "logcreate") == 0)
        {
          strsplit (bufline, buf, 5);
          if (log_get (buf[1]) != NULL)
            conf_error ("log name already in use");
          Log *log = log_add (buf[1]);
          if (log == NULL)
            conf_error ("log creation failed");
          if (!log->conf (buf[4], buf[5][0] == 0 ? NULL : buf[5], atoi (buf[2]), atoi (buf[3])))
            conf_error ("log configuration failed");
          changed = 1;
        }

      else if (strcasecmp (buf[0], "logmail") == 0)
        {
#ifdef USE_MAIL
          strsplit (bufline, buf, 3);
          Log *log = log_get (buf[1]);
          if (log == NULL)
            {
              snprintf (buf[0], MSG_SIZE, "inexistant loghandle: %s", buf[1]);
              conf_error (buf[0]);
            }
          log->mail = buf[2];
          changed = 1;
#else
          conf_error ("can't send mails - either 'cat', 'echo' or 'sendmail' wasn't found by configure");
#endif
        }

      if (! (changed || bot_defined))
        conf_error("bot configuration before a bot is initialized (check the \"bot\" line)");

      if (strcasecmp (buf[0], "host") == 0)
        {
          strsplit (bufline, buf, 4);
          s->hosts = s->host_add (s->hosts, buf[1],
                                  buf[2][0] == 0 ? 6667 : atoi (buf[2]),
                                  buf[3][0] == 0 ? NULL : buf[3]);
        }

      else if (strcasecmp (buf[0], "vhost") == 0)
        s->virtualhost = buf[1];

      else if (strcasecmp (buf[0], "nick") == 0)
        {
          s->nick = buf[1];
          s->nick_orig = buf[1];
        }

      else if (strcasecmp (buf[0], "user") == 0)
        s->user = buf[1];

      else if (strcasecmp (buf[0], "name") == 0)
        {
          strsplit (bufline, buf, 1);
          s->name = buf[1];
        }

      else if (strcasecmp (buf[0], "nickserv_pass") == 0)
        {
          if (!s->services.exist)
            {
              s->services.nickserv_pass = buf[1];
              s->services.exist = 1;
            }
          else
            conf_error ("nickserv already defined");
        }

      else if (strcasecmp (buf[0], "nickserv_mask") == 0)
        {
          strsplit (bufline, buf, 1);
          s->services.nickserv_mask = buf[1];
        }

      else if (strcasecmp (buf[0], "nickserv_auth") == 0)
        {
          strsplit (bufline, buf, 1);
          s->services.nickserv_auth = buf[1];
        }

      else if (strcasecmp (buf[0], "servicesmsg") == 0)
        s->services.services_privmsg = 1;

      else if (strcasecmp (buf[0], "away") == 0)
        {
          strsplit (bufline, buf, 1);
          s->away = buf[1];
        }

      else if (strcasecmp (buf[0], "quit") == 0)
        {
          strsplit (bufline, buf, 1);
          s->quit = buf[1];
        }

      else if (strcasecmp (buf[0], "unbind") == 0)
        {
          if (!s->script.cmd_unbind (buf[1]))
            conf_warn ("inexistent command");
        }

      else if (strcasecmp (buf[0], "cmdlevel") == 0)
        {
          strsplit (bufline, buf, 2);
          if (buf[2][0] == 0)
            conf_error ("sintax error: use \"cmddel !command level\"");
          Script::cmd_type *c = s->script.cmd_get (buf[1]);
          if (c != NULL)
            {
              int level = atoi (buf[2]);
              if (level > LEVEL_MAX)
                c->level = LEVEL_MAX;
              else if (level < 0)
                c->level = 0;
              else
                c->level = level;
            }
          else
            conf_warn ("inexistent command in cmdlevel");
        }

      else if (strcasecmp (buf[0], "changetime") == 0)
        s->change_time = atoi (buf[1]) * 60;

      else if (strcasecmp (buf[0], "channel") == 0)
        {
          strsplit (bufline, buf, 3);
          if (CHANNEL_INDEX (buf[1]) != -1)
            conf_error ("channel already defined");
          if (!s->channel_add (buf[1], buf[2]))
            conf_error ("maximum channels exceeded");
        }

      else if (strcasecmp (buf[0], "users") == 0)
        {
          if (!s->users.load_users (buf[1]))
            mbot_exit ();
        }

      else if (strcasecmp (buf[0], "set") == 0)
        {
          strsplit (bufline, buf, 2);
          if (!s->vars.var_set (buf[1], buf[2]))
            {
              snprintf (buf[3], MSG_SIZE, "inexistant variable \"%s\"", buf[1]);
              conf_warn (buf[3]);
            }
        }

      else if (strcasecmp (buf[0], "dccget") == 0)
        {
          s->dcc_file = buf[1];
          s->script.cmd_bind (scriptcmd_get, LEVEL_GET, "!get", NULL, HELP_GET);
        }

      else if (strcasecmp (buf[0], "dccport") == 0)
        s->dcc_port = atoi (buf[1]);

      else if (strcasecmp (buf[0], "dccmotd") == 0)
        {
          s->dcc_motdfile = buf[1];
          s->dcc_motd = new Text ();
          if (!s->dcc_motd->read_file (s->dcc_motdfile))
            {
              s->write_botlog ("error opening %s: %s", (c_char)s->dcc_motdfile,
                               strerror (errno));
              delete s->dcc_motd;
              s->dcc_motd = NULL;
            }
        }

      // pass control to the modules
      Module *m;
      modules.rewind ();
      while ((m = (Module *)modules.next ()) != NULL)
        m->conf (s, bufline);

    }
  check_server (s);
  server_num++;
}

void
Bot::work (void)
{
  while (1)			// main cycle
    {
      time_now = get_time ();
      usleep (100);
      for (int i = 0; i < server_num; i++)
        servers[i]->work ();
    }
}

