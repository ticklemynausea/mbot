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

#ifndef _H_LOG
#define _H_LOG

#define LOG_MAX 10			// logs per server
#define LOG_HOUR 6			// renew the logs everyday at 6am
#define LOG_SIZE 1048576		// each log's maximum size in bytes

#define TIME_SIZE 100

class Log;

#include "Bot.h"

class Log {
public:
  Log (Bot *);
  ~Log (void);

  bool conf (c_char, c_char, bool, int);
  EXPORT friend Log &operator << (Log &, c_char);
  EXPORT friend Log &operator << (Log &, char);
  EXPORT friend Log &operator << (Log &, const String &);

  String mail;				// mail where to send the log

private:

  void change_log (void);
  u_char get_hour (void);

  bool writing;
  Bot *b;				// bot where we belong
  fstream f;				// stream to write to
  int timemode;				// whether to write date or time
  bool flushmode;			// whether to flush after each write
  String filename,			// current log file
          old_filename;			// old log file
  u_char hour;				// last made log's time
  char st_time[TIME_SIZE];

};

#endif	// _H_LOG
