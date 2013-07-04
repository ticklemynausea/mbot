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

#include "Log.h"

Log::Log (Bot *bot) : mail (MAIL_SIZE), writing (0), b (bot),
  filename (PATH_MAX), old_filename (PATH_MAX)
{
}

Log::~Log (void)
{
  f << "Log stopped." << endl;
} 

// open <file>, return 0 on error.
// if <flush>, flush the stream on every write
// <time> can be 0 to write whole date or 1 to write only the time
bool
Log::conf (c_char file, c_char oldfile, bool flush, int time)
{
  filename = file;
  f.open (filename, ios::out|ios::app);
  if (!f)
    {
      b->write_botlog ("error opening %s: %s", (c_char)filename, strerror (errno));
      return 0;
    }
  if (oldfile != NULL)
    old_filename = oldfile;
  flushmode = flush;
  timemode = time;
  hour = get_hour ();
  return 1;
}

Log &
operator<< (Log &log, c_char str)
{
  if (str != EOL)
    {
      if (!log.writing)
        {
          if (log.old_filename)
            log.change_log ();
          if (log.timemode == 0)
            string_time (log.b->time_now, log.st_time);
          else
            get_asctime (log.b->time_now, log.st_time, TIME_SIZE);
          log.f << log.st_time << " ";
          log.writing = 1;
        }
      log.f << str;
    }
  else
    {
      if (log.flushmode)
        log.f << endl;
      else
        log.f << '\n';
      log.hour = log.get_hour ();
      log.writing = 0;
    }
  return log;
}

Log &
operator<< (Log &log, char c)
{
  if (!log.writing)
    {
      if (log.old_filename)
        log.change_log ();
      if (log.timemode == 0)
        string_time (log.b->time_now, log.st_time);
      else
        get_asctime (log.b->time_now, log.st_time, TIME_SIZE);
      log.f << log.st_time << " ";
      log.writing = 1;
    }
  log.f << c;
  return log;
}

Log &
operator<< (Log &log, const String &str)
{
  if (!log.writing)
    {
      if (log.old_filename)
        log.change_log ();
      if (log.timemode == 0)
        string_time (log.b->time_now, log.st_time);
      else
        get_asctime (log.b->time_now, log.st_time, TIME_SIZE);
      log.f << log.st_time << " ";
      log.writing = 1;
    }
  log.f << str;
  return log;
}

// renew log if it's time or if the file is too big
void
Log::change_log (void)
{
  if ((hour < LOG_HOUR && get_hour () >= LOG_HOUR)
      || f.tellp () >= LOG_SIZE)
    {
      char buf[PATH_MAX+1], buf2[PATH_MAX+1];
      f.close ();
      rename (filename, old_filename);
#ifdef USE_MAIL
      if (mail)			// send log by mail, if specified
        {
          snprintf (buf, PATH_MAX, 
		    "(%s && %s %s) | %s %s &\n",
//		    "(%s && %s %s) | %s -O NoRecipientAction=add-to %s &\n",
                    ECHO_PATH, CAT_PATH, (c_char)old_filename, SENDMAIL_PATH,
                    (c_char)mail);
          system (buf);
        }
#endif
      my_strncpy (buf, filename, PATH_MAX);
      if (old_filename)
        my_strncpy (buf2, old_filename, PATH_MAX);
      conf (buf, (old_filename) ? buf2 : NULL, timemode, flushmode);
      f << get_asctime (b->time_now, buf, PATH_MAX) << endl;
    }
}

// return current time
u_char
Log::get_hour (void)
{
  tm time1;
  time_t time2 = b->time_now;
  time1 = *localtime (&time2);
  return time1.tm_hour;
}

