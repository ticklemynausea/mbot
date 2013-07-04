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

#ifndef _H_DEFINES
#define _H_DEFINES

// general

#define MODULE_VERSION 6		// version saved in each module
#define CONF_FILE "mbot.conf"		// default configuration file
#define DEFAULT_USER_PASS "_dark"	// default !useradd password
#define CHANNEL_MAX 10			// max channels per server
#define C_USERS_MAX 500			// max users inside a channel (!)
#define SERVER_MAX 50			// max servers in the process
#define DCC_MAX 20			// max general porpuse dccs
#define LEVEL_MAX 10
#define LEVEL_MIN -1
#define PID_FILE "mbot.pid"		// file with current pid
#define TIME_RETRY 90	// seconds between each failed connection attempt
#define TIME_STONED 10*60 	// seconds of inactivity to consider the
				// server "stoned", ie, dead, and reconnect
#define TIME_CONNECT 30		// seconds to connect()
#define TIME_WARP 10		// seconds to consider the system date changed

#ifdef _H_CHANNEL
static c_char kicks[]={
  "shit happens",
  "go sit in a corner",
  "go away, we're like closed or something",
  "fuq off",
  "lamerz not allowed",
  "\"Those who fear darkness have never seen what light can do.\" -Selenia, dark angel"
};
u_char kicks_num=6;
#endif	// _H_CHANNEL

#ifdef ECHO_PATH
#  ifdef CAT_PATH
#    ifdef SENDMAIL_PATH
#      define USE_MAIL		// all those are needed to send mail
#    endif
#  endif
#endif

#define CTCP_VERSION VERSION_STRING
#define CTCP_FINGER "Finger?! Stick it up your ass!"
#define CTCP_SOURCE "Source available at http://darksun.com.pt/mbot/"

// services

#define NICKSERV "NickServ"			// case sensitive
#define CHANSERV "ChanServ"
#define MEMOSERV "MemoServ"
#define OPERSERV "OperServ"

// sizes

#define NICK_SIZE 32
#define USER_SIZE 12
#define HOST_SIZE 64
#define MASK_SIZE (NICK_SIZE+USER_SIZE+HOST_SIZE+5)
#define NAME_SIZE 50
#define MAIL_SIZE (NAME_SIZE+HOST_SIZE+2)
#define CHANNEL_SIZE 32
#define CHANNEL_KEY_SIZE 25
#define TOPIC_SIZE MSG_SIZE
#define QUIT_SIZE 160
#define MSG_SIZE 512			// to hold the server's strings
#define AWAY_SIZE MSG_SIZE
#define KICK_SIZE MSG_SIZE
#define PASS_SIZE 8			// user password
#define ENC_PASS_SIZE 34		// encrypted user password
#define SERVER_PASS_SIZE MSG_SIZE	// server password size
#define DCC_BUF_SIZE 2048		// size of dcc send blocks
#define BUF_SIZE 1024			// to lots of command buffers
#define CMD_SIZE 10			// to parse received commands
#define LINE_SIZE 10*1024		// maximum chars per line (CText)
#define VAR_SIZE 64			// a variable's maximum size

#endif

