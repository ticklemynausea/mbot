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

#include "Strings.h"

String::String (void)
{
  str = NULL;
  len = 0;
  maxlen = (u_short)-1;
}

String::String (u_short l)
{
  str = NULL;
  len = 0;
  maxlen = l;
}

String::String (c_char s, u_short l)
{
  str = NULL;
  maxlen = l;
  strset (&str, s, maxlen);
  if (str != NULL)
    len = strlen (str);
  else
    len = 0;
}

String::~String (void)
{
  my_free (str);
}

void
String::operator = (const String &right)
{
  *this = (c_char)right;
}

void
String::operator = (c_char s)
{
  strset (&str, s, maxlen);
  if (str != NULL)
    len = strlen (str);
  else
    len = 0;
}

void
String::operator += (c_char s)
{
  if (s == NULL || s[0] == 0)
    return;
  if (str == NULL)
    {
      *this = s;
      return;
    }
  size_t newlen = strlen (str) + strlen (s);
  if (newlen > maxlen)
    newlen = maxlen;
  char *tmpstr = (char *)my_malloc (newlen+1);
  snprintf (tmpstr, newlen+1, "%s%s", str, s);
  free (str);
  str = tmpstr;
  len = strlen (str);
}

ostream &
operator << (ostream &out, const String &s)
{
  return out << s.str;
}

bool
String::operator == (const String &right) const
{
  if (len != right.len)
    return 0;
  return strcmp (str, right.str) == 0;
}

bool
String::operator == (c_char right) const
{
  if (str == NULL)
    return right == NULL;
  if (right == NULL)
    return 0;	// it's false because we already know that str != NULL
  if (len != strlen (right))
    return 0;
  return strcmp (str, right) == 0;
}

bool
String::operator |= (const String &right) const
{
  if (len != right.len)
    return 0;
  return strcasecmp (str, right.str) == 0;
}

bool
String::operator |= (c_char right) const
{
  if (str == NULL || right == NULL)
    return str == right;
  if (len != strlen (right))
    return 0;
  return strcasecmp (str, right) == 0;
}

bool
String::operator != (c_char right) const
{
  if (right == NULL)
    return str != NULL;
  if (len != strlen (right))
    return 1;
  return strcmp (str, right) != 0;
}

void
String::strip_crlf (void)
{
  if (str == NULL)
    return;
  for (int i = len - 1; i >= 0; i--)
    {
      if (str[i] != '\n' && str[i] != '\r')
        break;
      str[i] = 0;
    }
  len = strlen (str);
}

void
String::lower (void)
{
  if (str == NULL)
    return;
  char *tmp = str;
  while (tmp[0] != 0)
    {
      tmp[0] = tolower (tmp[0]);
      tmp++;
    }
}

/*

  StringFixed

*/

StringFixed::StringFixed (u_short l)
{
  str = (char *)my_malloc (l+1);
  str[0] = 0;
  len = 0;
  maxlen = l;
}

void
StringFixed::operator = (const String &right)
{
  *this = (c_char)right;
}

void
StringFixed::operator = (c_char s)
{
  if (s != NULL)
    {
      my_strncpy (str, s, maxlen);
      len = strlen (str);
    }
  else
    {
      str[0] = 0;
      len = 0;
    }
}

void
StringFixed::operator += (c_char s)
{
  if (s == NULL || s[0] == 0 || maxlen-len == 0)
    return;
  my_strncpy (str+len, s, maxlen-len);
  len = strlen (str);
}

