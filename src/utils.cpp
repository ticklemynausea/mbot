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

#include "utils.h"

bool
file_exists (c_char name)
{
  struct stat s;
  return !stat (name, &s);
}

void *
my_malloc (size_t size)
{
  void *buf = malloc (size);
  if (buf == NULL)
    {
      cerr << "PANIC: failed malloc(" << size << ")\n";
      exit (1);
    }
  return buf;
}

void
my_free (void *ptr)
{
  if (ptr != NULL)
    free (ptr);
}

// malloc enough space in <*dest> (up to <maxlen>) to copy <src> into it,
// freeing <*dest> first if not NULL. if malloc() fails, <*dest> is preserved.
// to free() up <*dest>, use a NULL <src>
void
strset (char **dest, c_char src, size_t maxlen)
{
  if (src == NULL)
    {
      if (*dest != NULL)
        {
          free (*dest);
          *dest = NULL;
        }
      return;
    }
  size_t len = strlen (src);
  if (len > maxlen)
    len = maxlen;
  char *tmp = (char *)my_malloc (len+1);
  if (tmp == NULL)
    return;
  if (*dest != NULL)
    free (*dest);
  *dest = tmp;
  my_strncpy (*dest, src, len);
}

time_t
get_time (void)
{
  time_t t;
  return time (&t);
}

tm
get_localtime (void)
{
  time_t t = get_time ();
  return *localtime (&t);
}

char *
get_asctime (time_t t, char *buf, size_t len)
{
  my_strncpy (buf, asctime (localtime (&t)), len);
  return strip_crlf (buf);
}

// write in <str> the time <t>, in the format "hh:mm"
void
string_time (time_t t, char *str)
{
  tm time1;
  time1 = *localtime (&t);
  snprintf (str, 6, "%02d:%02d", time1.tm_hour, time1.tm_min);
}

// convert the long integer <n> to the string <buf> up to <len> chars
char *
ltoa (long int n, char *buf, size_t len)
{
  long int n2 = n;
  bool neg = 0;
  size_t i, size = 0;

  if (len == 0)
    {
      buf[0] = 0;
      return buf;
    }

  if (n2 < 0)
    {
      neg = 1; 
      n2 = -n2; 
      buf[0] = '-';
      len--;
    }

  do
    {
      n = n / 10;
      size++; 
    }
  while (n != 0);
  if (size > len)
    size = len;

  for (i = 1; i <= size; i++)
    {
      buf[neg+size-i] = n2%10 + 48;
      n2 =n2 / 10;
    }

  buf[neg+size] = 0;
  return buf;
}

// convert the integer <n> to the string <buf> up to <len> chars
char *
itoa (int n, char *buf, size_t len)
{
  int n2 = n;
  bool neg = 0;
  size_t i, size = 0;

  if (len == 0)
    {
      buf[0] = 0;
      return buf;
    }

  if (n2 < 0)
    {
      neg = 1; 
      n2 = -n2; 
      buf[0] = '-';
      len--;
    }

  do
    {
      n = n / 10;
      size++; 
    }
  while (n != 0);
  if (size > len)
    size = len;

  for (i = 1; i <= size; i++)
    {
      buf[neg+size-i] = n2%10 + 48;
      n2 =n2 / 10;
    }

  buf[neg+size] = 0;
  return buf;
}

// convert the unsigned integer <n> to the string <buf>
char *
utoa (u_int n, char *buf, size_t len)
{
  u_int n2 = n, i, size = 0;

  if (len == 0)
    {
      buf[0] = 0;
      return buf;
    }

  do
    {
      n = n / 10;
      size++; 
    }
  while (n != 0);
  if (size > len)
    size = len;

  for (i = 1; i <= size; i++)
    {
      buf[size-i] = n2%10 + 48;
      n2 = n2 / 10;
    }

  buf[size] = 0;
  return buf;
}

void
random_init (void)
{
  time_t t = get_time ();
  struct tm *t2 = localtime (&t);
  srand (t2->tm_sec);
}

long int
random_num (long int max)
{
  if (max == 0)
    return 0;
  return rand () % max;
}

c_char
random_salt (void)
{
  const char valid[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./";
  static char salt[3];
  salt[0] = valid[random_num (strlen (valid)-1)];
  salt[1] = valid[random_num (strlen (valid)-1)];
  salt[2] = 0;
  return salt;
}

size_t
num_spaces (c_char str)
{
  size_t i = 0;
  while (str[i] == ' ')
    i++;
  return i;
}

size_t
num_notspaces (c_char str)
{
  size_t i = 0;
  while (str[i] != ' ' && str[i] != 0)
    i++;
  return i;
}

void
mask2nick (c_char mask, char *nick)
{
  size_t i = 0;
  while (mask[i] != '!')
    if (mask[i++] == 0)
      {
        nick[0] = 0;
        return;
      }
  my_strncpy (nick, mask, i > NICK_SIZE ? NICK_SIZE : i);
}

void
mask2user (c_char mask, char *user)
{
  int i = 0, i2;
  while (mask[i] != '!')
    if (mask[i++] == 0)
      {
        i = -1;
        break;
      }
  i2 = ++i;
  while (mask[i2] != '@')
    if (mask[i2++] == 0)
      {
        user[0] = 0;
        return;
      }
  my_strncpy (user, mask+i, i2-i > USER_SIZE ? USER_SIZE : i2-i);
}

void
mask2host (c_char mask, char *host)
{
  int i = 0;
  while (mask[i] != '@')
    if (mask[i++] == 0)
      {
        host[0] = 0;
        return;
      }
  my_strncpy (host, mask+i+1, HOST_SIZE);
}

void
mask_split (c_char mask, char *nick, char *user, char *host)
{
  mask2nick (mask, nick);
  mask2user (mask, user);
  mask2host (mask, host);
}

// only mask1 can have wildcards!
bool
match_mask (c_char mask2, c_char mask1)
{
  size_t i = 0, i2 = 0, size1 = strlen (mask1), size2 = strlen (mask2);

  while (i < size1 || i2 < size2)
    {
      if (mask1[i] == '*')
        {
          for (i++; i2 < size2 && mask2[i2] != mask1[i]; i2++)
            if (mask1[i] == '?')
              i++;
          continue;
        }
      if ((mask1[i] == '?' && mask2[i2] == 0)
          || strncasecmp (mask1 + i, mask2 + i2, 1) != 0)
        return 0;
      i++;
      i2++;
    }
  return 1;
}

// transform mirage!mirage@darksun.com.pt into *!*mirage@*.com.pt
// and mirage!mirage@1.2.3.4 into *!*mirage@1.2.3.*
void
make_generic_mask (c_char mask, char *gen_mask)
{
  int i;
  char user[USER_SIZE+1];
  char host[HOST_SIZE+1];

  mask2user (mask, user);
  mask2host (mask, host);

  if (user[0] == 0 || host[0] == 0 || strlen (mask) > MASK_SIZE)
    {
      gen_mask[0] = 0;
      return;
    }

  snprintf (gen_mask, MASK_SIZE, "*!*%s@", user);
  if (host[strlen (host)-1] >= '0' && host[strlen (host)-1] <= '9')
    {
      strcpy (gen_mask + strlen (gen_mask), host);
      i = strlen (gen_mask) - 1;
      while (gen_mask[i] != '.' && gen_mask[i] != '@')
        i--;
      strcpy (gen_mask+i+1, "*");
    }
  else
    {
      i = 0;
      while (host[i] != '.')
        i++;
      sprintf (gen_mask + strlen (gen_mask), "*%s", host + i);
    }
}

// copy the first n words to dest[0..n-1] and the rest to dest[n]
void
strsplit (c_char src, char dest[][MSG_SIZE+1], u_char n)
{
  size_t i, pos = 0, chars;
  for (i = 0; i < n; i++)
    {
      pos += num_spaces (src + pos);
      chars = num_notspaces (src + pos);
      my_strncpy (dest[i], src + pos, chars);
      pos += chars;
    }
  my_strncpy (dest[i], src + pos + num_spaces (src + pos), MSG_SIZE);
}

// like strncpy, but assumes <dest> has <n>+1 bytes and puts a '\0' at the end
char *
my_strncpy (char *dest, c_char src, size_t n)
{
  size_t i = strlen (src);
  if (i > n)
    i = n;
  memmove (dest, src, i);
  dest[i] = 0;
  return dest;
}

// like my_strncpy, but if <src> is NULL, it places a \0 and doesn't crash
char *
my_null_strncpy (char *dest, c_char src, size_t n)
{
  if (src == NULL)
    dest[0] = 0;
  else
    dest = my_strncpy (dest, src, n);
  return dest;
}

// return the file in <path>
c_char
fullpath2file (c_char path)
{
  int pos = 0;
  for (int i = 0; i < (int)strlen (path); i++)
    if (path[i] == '/')
      pos = i + 1;
  return path + pos;
}

// remove any CR/LF ('\n' and '\r') from the end of <str> and return it
char *
strip_crlf (char *str)
{
  for (int i = strlen (str) - 1; i >= 0; i--)
    {
      if (str[i] != '\n' && str[i] != '\r')
        break;
      str[i] = 0;
    }
  return str;
}

// apply toupper() to str
void
upper (char *str)
{
  while (str[0] != 0)
    {
      str[0] = toupper (str[0]);
      str++;
    }
}

// apply tolower() to str
void
lower (char *str)
{
  while (str[0] != 0)
    {
      str[0] = tolower (str[0]);
      str++;
    }
}

// same thing as close(), but usable inside fstream methods
int
my_close (int fd)
{
  return close (fd);
}

// use daemon() or simulate it to put the process in background
bool
daemonize (void)
{
#ifdef HAVE_DAEMON
  return daemon (1, 1) == 0;
#else

#ifdef HAVE_TIOCNOTTY
  int i = open ("/dev/tty", O_RDWR);
  if (i != -1)
    {
      ioctl (i, TIOCNOTTY, 0);
      close (i);
    }
#endif  // HAVE_TIOCNOTTY

  switch (fork ())
    {
      case -1:
        return 0;
      case 0:
#ifdef HAVE_SETSID
        setsid ();
#endif          // HAVE_SETSID
        return 1;
      default:
        exit (0);
    }

#endif		// !HAVE_DAEMON
}

// creates a temporary fstream
fstreamtmp::fstreamtmp (void) : fstream ()
{
  char name[PATH_MAX+1];
  my_strncpy (name, "/tmp/mbot-XXXXXX", PATH_MAX);

#ifdef HAVE_MKSTEMP

  int fd = mkstemp (name);
  if (fd == -1)
    return;
# ifdef HAVE_FSTREAM_ATTACH
  attach (fd);
# else
  this->open (name, ios::in | ios::out);
  my_close (fd);
# endif
  remove (name);

#else		// !HAVE_MKSTEMP

  if (mktemp (name) == NULL)
    return;
  this->open (name, ios::in | ios::out | IOS_NOREPLACE);
  if (this)
    remove (name);

#endif		// HAVE_MKSTEMP
}
