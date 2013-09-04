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

#include "Net.h"

Net::Net (int socket) : bytesout (0), bytesin (0),
  time_read (bot->time_now), time_write (bot->time_now), sock (socket),
  connecting (0), connected (0), bufpos (0)
{
  bufread = (char *)my_malloc (MSG_SIZE+1);
}

Net::~Net (void)
{
  my_free (bufread);
  closesock ();
}

void
Net::writeln (const char *format, ...)
{
  if (sock == -1)
    return;
  char bufwrite[MSG_SIZE+1];
  va_list args;
  va_start (args, format);
  vsnprintf (bufwrite, MSG_SIZE-1, format, args);
  va_end (args);
  if (bot->debug)
    cout << "write: " << sock << ": " << bufwrite << endl;
  size_t len = strlen (bufwrite);
  if (len > 500)		// ptnet *ignores* lines with 500+ chars
    len = 500;

  /* 1337 1xpl01d f1x */
  for (size_t i = 0; i < len; i++) {
    if (bufwrite[i] == 0x0A || bufwrite[i] == 0x0D) {
      bufwrite[i] = 0x20;
    }
  }

  bytesout += len + 2;		// +2 because a '\r\n' will be appended
  my_strncpy (bufwrite+len, "\r\n", 3);
  send (sock, bufwrite, len+2, 0);
  time_write = bot->time_now;
}

// read as many bytes as the socket has, until a \n or bufpos has max size
// return -1 on error, 0 to ignore, 1 if a new string is ready
int
Net::readln (void)
{
  if (sock == -1)
    return 0;
  int i = readok (sock);
  while (i == 1)		// while there's something to read
    {
      i = recv (sock, bufread + bufpos, 1, 0);
      if (i == 1)					// success?
        {
          bytesin++;
          if (bufread[bufpos] == '\n' || bufpos == MSG_SIZE)	// end msg
            {
              bufread[bufpos] = 0;
              strip_crlf (bufread);
              if (bot->debug)
                cout << "read: " << sock << ": " << bufread << endl;
              bufpos = 0;
              time_read = bot->time_now;
              return 1;
            }
          bufpos++;			// next char in buffer
        }
      else if (i == 0 && bufpos != 0)	// usually end of stream
        {
          bufread[bufpos] = 0;
          strip_crlf (bufread);
          if (bot->debug)
            cout << "read: " << sock << ": " << bufread << endl;
          bufpos = 0;
          time_read = bot->time_now;
          return 1;
        }
      else				// empty 0 or -1
        {
#ifdef WINDOZE
          errno = WSAGetLastError ();
		  if (errno == WSAEWOULDBLOCK)
#else
          if (i == -1 && errno == EAGAIN)	// non-blocking socket
#endif
            return 0;
          else		// -1 on error, 0 when the socket dies
            return -1;
        }
    }
  return i;
}

// prepare sock and sa to openhost()
// make sure you called resolvehost() to set addr and socket/vhost type!
// return 0 on error
bool
Net::create_tcp (int port, c_char vhost, bool block)
{
  sock = socket (addr.inet, SOCK_STREAM, IPPROTO_TCP);
  if (sock == -1)
    return 0;
  if (vhost != NULL)
    {
      struct addr_type a;
      if (!resolvehost (vhost, &a, addr.inet))
        {
          close (sock);
          return 0;
        }

      int result;
#ifdef ENABLE_IPV6
      if (a.inet == AF_INET6)
        result = bind (sock, (struct sockaddr *) &a.sa6, sizeof (a.sa6));
      else
#endif
        result = bind (sock, (struct sockaddr *) &a.sa, sizeof (a.sa));

      if (result == -1)
        {
          close (sock);
          return 0;
        }
    }
#ifdef ENABLE_IPV6
  if (addr.inet == AF_INET6)
    {
      addr.sa6.sin6_family = AF_INET6;
      addr.sa6.sin6_port = htons (port);
    }
  else
#endif
    {
      addr.sa.sin_family = AF_INET;
      addr.sa.sin_port = htons (port);
    }
  int parm = 1;
  setsockopt (sock, IPPROTO_TCP, TCP_NODELAY, (c_char)&parm, sizeof (int));
  if (!block)
    blocksock (0);
  connected = 0;
  connecting = 1;
  return 1;
}

// perform connect() on current addr & socket
// make sure you call create_tcp() before this!
// return 0 on error, 1 if it's still connecting or already connected
bool
Net::openhost (void)
{
  int result;
#ifdef ENABLE_IPV6
  if (addr.inet == AF_INET6)
    result = connect (sock, (struct sockaddr *)&addr.sa6, sizeof addr.sa6);
  else
#endif
    result = connect (sock, (struct sockaddr *)&addr.sa, sizeof addr.sa);
  if (result == -1)
    {
#ifdef WINDOZE
      errno = WSAGetLastError ();
#endif		// !WINDOZE
      if (errno == EISCONN)
        {
          // we're connected
          sock_linger (sock, 60 * 100); // wait up to one minute in close()
          connected = 1;
          connecting = 0;
          return 1;
        }
      if (errno != EWOULDBLOCK && errno != EINPROGRESS && errno != EALREADY)
        {
          closesock ();
          connecting = 0;
          return 0;
        }
      return 1;
    }
  // we're connected
  sock_linger (sock, 60 * 100); // wait up to one minute in close()
  connected = 1;
  connecting = 0;
  return 1;
}

// return 1 if there's new data, 0 if not, -1 if failed
int
Net::readok (int sock2)
{
  fd_set rfds;
  struct timeval tv;
  FD_ZERO (&rfds);
  FD_SET ((unsigned)sock2, &rfds);
  tv.tv_sec = 0;
  tv.tv_usec = 2;
  int i = select (sock2+1, &rfds, NULL, &rfds, &tv);
  FD_ZERO (&rfds);
  return i;
}

// return 1 if there's new data, 0 if not, -1 if failed
int
Net::writeok (int sock)
{
  fd_set wfds;
  struct timeval tv;
  FD_ZERO (&wfds);
  FD_SET ((unsigned)sock, &wfds);
  tv.tv_sec = 0;
  tv.tv_usec = 1;
  int i = select (sock+1, NULL, &wfds, &wfds, &tv);
  FD_ZERO (&wfds);
  return i;
}

// force the socket to flush buffers on a close()
void
Net::sock_linger (int sock, int timeout)
{
  struct linger ling;
  ling.l_onoff = 1;
  ling.l_linger = timeout;
  setsockopt (sock, SOL_SOCKET, SO_LINGER, (c_char)&ling, sizeof (&ling));
}

// create a nonblocking socket and bind it to the specified port
// if port=0, use any free one. return 0 on error.
bool
Net::bindsock (int port)
{
#ifdef ENABLE_IPV6
  struct sockaddr_in6 local;
#else
  struct sockaddr_in local;
#endif

  memset (&local, 0, sizeof (local));
#ifdef ENABLE_IPV6
  local.sin6_family = AF_INET6;
  local.sin6_port = htons (port);
  local.sin6_addr = in6addr_any;
#else
  local.sin_family = AF_INET;
  local.sin_port = htons (port);
  local.sin_addr.s_addr = INADDR_ANY;
  memset (&(local.sin_zero), 0, 8);
#endif
  sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock == -1)
    return 0;
  int parm = 1;
  setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (c_char)&parm, sizeof (int));
  setsockopt (sock, IPPROTO_TCP, TCP_NODELAY, (c_char)&parm, sizeof (int));
  blocksock (0);

  if (bind (sock, (struct sockaddr *)&local, sizeof (struct sockaddr)) == -1)
    {
      closesock ();
      return 0;
    }
  if (listen (sock, 1) == -1)
    {
      closesock ();
      return 0;
    }
  return 1;
}

void
Net::blocksock (bool block)
{
#ifdef WINDOZE
  unsigned long opt = !block;
  ioctlsocket (sock, FIONBIO, &opt);
#else
  fcntl (sock, F_SETFL, block ? 0 : O_NONBLOCK);
#endif          // !WINDOZE
}

u_short
Net::sock2port (int sock)
{
  struct sockaddr_in addr;
  socklen_t size = sizeof (struct sockaddr);

  memset (&addr, 0, sizeof (struct sockaddr_in));
  getsockname (sock, (struct sockaddr *)&addr, (socklen_t *)&size);

  return ntohs (addr.sin_port);
}

u_long
Net::sock2addr (int sock)
{
  struct sockaddr_in addr;
  socklen_t size = sizeof (struct sockaddr);

  memset (&addr, 0, sizeof (struct sockaddr_in));
  getsockname (sock, (struct sockaddr *)&addr, (socklen_t *)&size);

  return ntohl (addr.sin_addr.s_addr);
}


bool
Net::resolvehost (c_char name, struct addr_type *a, int inet)
{
#ifdef ENABLE_IPV6

/*
getaddrinfo/freeaddrinfo produce a one-time memory leak (regardless of
the number of times used) as shown by valgrind. i believe this is a bug in
my glibc and thus i can't fixe it. the non-ipv6-enabled codepath doesn't
contain this leak, so you can use --disable-ipv6 if it really bothers you.
*/

  int error;
  struct addrinfo hints, *result, *result_orig;
  struct sockaddr_in *in;
  struct sockaddr_in6 *in6;
  bool ret = 0;

  memset (a, 0, sizeof (addr_type));
  memset (&hints, 0, sizeof (hints));
  hints.ai_socktype = SOCK_STREAM;
  if ((error = getaddrinfo (name, "domain", &hints, &result)) < 0)
    return 0;
  result_orig = result;

  while (result != NULL)
    {
      if (inet == 0 || result->ai_family == inet)
        {
          if (result->ai_family == AF_INET6)
            {
              a->inet = AF_INET6;
              in6 = (struct sockaddr_in6 *) result->ai_addr;
              memcpy (&a->sa6.sin6_addr, &in6->sin6_addr, sizeof (struct in6_addr));
              ret = 1;
              break;
            }
          else if (result->ai_family == AF_INET)
            {
              a->inet = AF_INET;
              in = (struct sockaddr_in *) result->ai_addr;
              memcpy (&a->sa.sin_addr, &in->sin_addr, sizeof (struct in_addr));
              ret = 1;
              break;
            }
        }
      result = result->ai_next;
    }

  freeaddrinfo (result_orig);

  return ret;

#else

  if (inet != 0 && inet != AF_INET)
    return 0;
  a->inet = AF_INET;
  struct hostent *he;
  if ((he = gethostbyname (name)) == NULL)
    {
      a->sa.sin_addr.s_addr = inet_addr (name);
      if (a->sa.sin_addr.s_addr == (unsigned int)-1)
        return 0;
    }
  else
    memcpy (&a->sa.sin_addr.s_addr, he->h_addr_list[0], he->h_length);

  return 1;

#endif	// ENABLE_IPV6

}

// close the socket, 0 if not open
bool
Net::closesock (void)
{
  connected = connecting = 0;
  if (sock == -1)
    return 0;
#ifdef WINDOZE
  closesocket (sock);
#else
  close (sock);
#endif
  sock = -1;
  return 1;
}

/*

  NetHttp

*/

NetHttp::NetHttp (c_char h, c_char f, int l) : Net (), html (l),
  host (h, HOST_SIZE), filename (f, MSG_SIZE), size (l), pos (0)
{ 
}

/*
  reads <filename> from <host> to <html>, maximum <size> bytes.
  non-blocking.

  return values:
  0 - in progress
  1 - done
  2 - invalid host
  3 - socket error
  4 - connect error   
  5 - read error
  6 - timeout
*/
int
NetHttp::work (void)
{
  if (!connected)
    {
      if (!connecting)
        {
          time_read = bot->time_now;
          if (!resolvehost (host, &addr))
            return 2;
          if (!create_tcp (80))
            return 3;
        }
      if (!openhost ())
        return 4;
      if (!connected)
        return 0;
      writeln ("GET %s HTTP/1.0", (c_char)filename);
      writeln ("Host: %s\r\n", (c_char)host);   
      reading_header = 1;
    }

  int i = readln ();
  if (i == -1)
    return 1; 
  if (i == 0) 
    {
      if (difftime (bot->time_now, time_read) > 60)
        return 6;
      return 0;  
    }

  if (reading_header)
    {
      if (bufread[0] == 0)
        reading_header = 0;
      return 0;
    }

  html += bufread;
  html+="\n";
  return 0;  
}
