/*

  Here I try to replace some essencial functions in case they are missing
in your system. Most of them were taken from somewhere else.


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

#include "missing.h"

#ifndef HAVE_GETOPT

/* Getopt for GNU.
   NOTE: getopt is now part of the C library, so if you don't know what
   "Keep this file name-space clean" means, talk to roland@gnu.ai.mit.edu
   before changing it!

   Copyright (C) 1987, 88, 89, 90, 91, 92, 93, 94
   	Free Software Foundation, Inc.
*/

#define _(str) (str)

char *optarg = NULL;
int optind = 0;
static char *nextchar;
int opterr = 1;
int optopt = '?';
static enum
{
  REQUIRE_ORDER, PERMUTE, RETURN_IN_ORDER
} ordering;
static char *posixly_correct;

/* Avoid depending on library functions or files
   whose names are inconsistent.  */

char *getenv ();

static char *
my_index (const char *str, int chr)
{
  while (*str)
    {
      if (*str == chr)
	return (char *) str;
      str++;
    }
  return 0;
}

/* Handle permutation of arguments.  */

/* Describe the part of ARGV that contains non-options that have
   been skipped.  `first_nonopt' is the index in ARGV of the first of them;
   `last_nonopt' is the index after the last of them.  */

static int first_nonopt;
static int last_nonopt;

/* Exchange two adjacent subsequences of ARGV.
   One subsequence is elements [first_nonopt,last_nonopt)
   which contains all the non-options that have been skipped so far.
   The other is elements [last_nonopt,optind), which contains all
   the options processed since those non-options were skipped.

   `first_nonopt' and `last_nonopt' are relocated so that they describe
   the new indices of the non-options in ARGV after they are moved.  */

static void
exchange (char **argv)
{
  int bottom = first_nonopt;
  int middle = last_nonopt;
  int top = optind;
  char *tem;

  /* Exchange the shorter segment with the far end of the longer segment.
     That puts the shorter segment into the right place.
     It leaves the longer segment in the right place overall,
     but it consists of two parts that need to be swapped next.  */

  while (top > middle && middle > bottom)
    {
      if (top - middle > middle - bottom)
	{
	  /* Bottom segment is the short one.  */
	  int len = middle - bottom;
	  register int i;

	  /* Swap it with the top part of the top segment.  */
	  for (i = 0; i < len; i++)
	    {
	      tem = argv[bottom + i];
	      argv[bottom + i] = argv[top - (middle - bottom) + i];
	      argv[top - (middle - bottom) + i] = tem;
	    }
	  /* Exclude the moved bottom segment from further swapping.  */
	  top -= len;
	}
      else
	{
	  /* Top segment is the short one.  */
	  int len = top - middle;
	  register int i;

	  /* Swap it with the bottom part of the bottom segment.  */
	  for (i = 0; i < len; i++)
	    {
	      tem = argv[bottom + i];
	      argv[bottom + i] = argv[middle + i];
	      argv[middle + i] = tem;
	    }
	  /* Exclude the moved top segment from further swapping.  */
	  bottom += len;
	}
    }

  /* Update records for the slots the non-options now occupy.  */

  first_nonopt += (optind - last_nonopt);
  last_nonopt = optind;
}

/* Initialize the internal data when the first call is made.  */

static const char *
_getopt_initialize (const char *optstring)
{
  /* Start processing options with ARGV-element 1 (since ARGV-element 0
     is the program name); the sequence of previously skipped
     non-option ARGV-elements is empty.  */

  first_nonopt = last_nonopt = optind = 1;

  nextchar = NULL;

  posixly_correct = NULL;

  /* Determine how to handle the ordering of options and nonoptions.  */

  if (optstring[0] == '-')
    {
      ordering = RETURN_IN_ORDER;
      ++optstring;
    }
  else if (optstring[0] == '+')
    {
      ordering = REQUIRE_ORDER;
      ++optstring;
    }
  else if (posixly_correct != NULL)
    ordering = REQUIRE_ORDER;
  else
    ordering = PERMUTE;

  return optstring;
}

/* Scan elements of ARGV (whose length is ARGC) for option characters
   given in OPTSTRING.

   If an element of ARGV starts with '-', and is not exactly "-" or "--",
   then it is an option element.  The characters of this element
   (aside from the initial '-') are option characters.  If `getopt'
   is called repeatedly, it returns successively each of the option characters
   from each of the option elements.

   If `getopt' finds another option character, it returns that character,
   updating `optind' and `nextchar' so that the next call to `getopt' can
   resume the scan with the following option character or ARGV-element.

   If there are no more option characters, `getopt' returns `EOF'.
   Then `optind' is the index in ARGV of the first ARGV-element
   that is not an option.  (The ARGV-elements have been permuted
   so that those that are not options now come last.)

   OPTSTRING is a string containing the legitimate option characters.
   If an option character is seen that is not listed in OPTSTRING,
   return '?' after printing an error message.  If you set `opterr' to
   zero, the error message is suppressed but we still return '?'.

   If a char in OPTSTRING is followed by a colon, that means it wants an arg,
   so the following text in the same ARGV-element, or the text of the following
   ARGV-element, is returned in `optarg'.  Two colons mean an option that
   wants an optional arg; if there is text in the current ARGV-element,
   it is returned in `optarg', otherwise `optarg' is set to zero.

   If OPTSTRING starts with `-' or `+', it requests different methods of
   handling the non-option ARGV-elements.
   See the comments about RETURN_IN_ORDER and REQUIRE_ORDER, above.

   Long-named options begin with `--' instead of `-'.
   Their names may be abbreviated as long as the abbreviation is unique
   or is an exact match for some defined option.  If they have an
   argument, it follows the option name in the same ARGV-element, separated
   from the option name by a `=', or else the in next ARGV-element.
   When `getopt' finds a long-named option, it returns 0 if that option's
   `flag' field is nonzero, the value of the option's `val' field
   if the `flag' field is zero.

   The elements of ARGV aren't really const, because we permute them.
   But we pretend they're const in the prototype to be compatible
   with other systems.

   LONGOPTS is a vector of `struct option' terminated by an
   element containing a name which is zero.

   LONGIND returns the index in LONGOPT of the long-named option found.
   It is only valid when a long-named option has been found by the most
   recent call.

   If LONG_ONLY is nonzero, '-' as well as '--' can introduce
   long-named options.  */

int
_getopt_internal (int argc, char *const *argv, const char *optstring, const struct option *longopts, int *longind, int long_only)
{
  optarg = NULL;

  if (optind == 0)
    optstring = _getopt_initialize (optstring);

  if (nextchar == NULL || *nextchar == '\0')
    {
      /* Advance to the next ARGV-element.  */

      if (ordering == PERMUTE)
	{
	  /* If we have just processed some options following some non-options,
	     exchange them so that the options come first.  */

	  if (first_nonopt != last_nonopt && last_nonopt != optind)
	    exchange ((char **) argv);
	  else if (last_nonopt != optind)
	    first_nonopt = optind;

	  /* Skip any additional non-options
	     and extend the range of non-options previously skipped.  */

	  while (optind < argc
		 && (argv[optind][0] != '-' || argv[optind][1] == '\0'))
	    optind++;
	  last_nonopt = optind;
	}

      /* The special ARGV-element `--' means premature end of options.
	 Skip it like a null option,
	 then exchange with previous non-options as if it were an option,
	 then skip everything else like a non-option.  */

      if (optind != argc && !strcmp (argv[optind], "--"))
	{
	  optind++;

	  if (first_nonopt != last_nonopt && last_nonopt != optind)
	    exchange ((char **) argv);
	  else if (first_nonopt == last_nonopt)
	    first_nonopt = optind;
	  last_nonopt = argc;

	  optind = argc;
	}

      /* If we have done all the ARGV-elements, stop the scan
	 and back over any non-options that we skipped and permuted.  */

      if (optind == argc)
	{
	  /* Set the next-arg-index to point at the non-options
	     that we previously skipped, so the caller will digest them.  */
	  if (first_nonopt != last_nonopt)
	    optind = first_nonopt;
	  return EOF;
	}

      /* If we have come to a non-option and did not permute it,
	 either stop the scan or describe it to the caller and pass it by.  */

      if ((argv[optind][0] != '-' || argv[optind][1] == '\0'))
	{
	  if (ordering == REQUIRE_ORDER)
	    return EOF;
	  optarg = argv[optind++];
	  return 1;
	}

      /* We have found another option-ARGV-element.
	 Skip the initial punctuation.  */

      nextchar = (argv[optind] + 1
		  + (longopts != NULL && argv[optind][1] == '-'));
    }

  /* Decode the current option-ARGV-element.  */

  /* Check whether the ARGV-element is a long option.

     If long_only and the ARGV-element has the form "-f", where f is
     a valid short option, don't consider it an abbreviated form of
     a long option that starts with f.  Otherwise there would be no
     way to give the -f short option.

     On the other hand, if there's a long option "fubar" and
     the ARGV-element is "-fu", do consider that an abbreviation of
     the long option, just like "--fu", and not "-f" with arg "u".

     This distinction seems to be the most useful approach.  */

  if (longopts != NULL
      && (argv[optind][1] == '-'
	  || (long_only && (argv[optind][2] || !my_index (optstring, argv[optind][1])))))
    {
      char *nameend;
      const struct option *p;
      const struct option *pfound = NULL;
      int exact = 0;
      int ambig = 0;
      int indfound = 0;
      int option_index;

      for (nameend = nextchar; *nameend && *nameend != '='; nameend++)
	/* Do nothing.  */ ;

      /* Test all long options for either exact match
	 or abbreviated matches.  */
      for (p = longopts, option_index = 0; p->name; p++, option_index++)
	if (!strncmp (p->name, nextchar, nameend - nextchar))
	  {
	    if (nameend - nextchar == (signed)strlen (p->name))
	      {
		/* Exact match found.  */
		pfound = p;
		indfound = option_index;
		exact = 1;
		break;
	      }
	    else if (pfound == NULL)
	      {
		/* First nonexact match found.  */
		pfound = p;
		indfound = option_index;
	      }
	    else
	      /* Second or later nonexact match found.  */
	      ambig = 1;
	  }

      if (ambig && !exact)
	{
	  if (opterr)
	    fprintf (stderr, _("%s: option `%s' is ambiguous\n"),
		     argv[0], argv[optind]);
	  nextchar += strlen (nextchar);
	  optind++;
	  return '?';
	}

      if (pfound != NULL)
	{
	  option_index = indfound;
	  optind++;
	  if (*nameend)
	    {
	      /* Don't test has_arg with >, because some C compilers don't
		 allow it to be used on enums.  */
	      if (pfound->has_arg)
		optarg = nameend + 1;
	      else
		{
		  if (opterr)
		    {
		      if (argv[optind - 1][1] == '-')
			/* --option */
			fprintf (stderr,
				 _("%s: option `--%s' doesn't allow an argument\n"),
				 argv[0], pfound->name);
		      else
			/* +option or -option */
			fprintf (stderr,
			     _("%s: option `%c%s' doesn't allow an argument\n"),
			     argv[0], argv[optind - 1][0], pfound->name);
		    }
		  nextchar += strlen (nextchar);
		  return '?';
		}
	    }
	  else if (pfound->has_arg == 1)
	    {
	      if (optind < argc)
		optarg = argv[optind++];
	      else
		{
		  if (opterr)
		    fprintf (stderr, _("%s: option `%s' requires an argument\n"),
			     argv[0], argv[optind - 1]);
		  nextchar += strlen (nextchar);
		  return optstring[0] == ':' ? ':' : '?';
		}
	    }
	  nextchar += strlen (nextchar);
	  if (longind != NULL)
	    *longind = option_index;
	  if (pfound->flag)
	    {
	      *(pfound->flag) = pfound->val;
	      return 0;
	    }
	  return pfound->val;
	}

      /* Can't find it as a long option.  If this is not getopt_long_only,
	 or the option starts with '--' or is not a valid short
	 option, then it's an error.
	 Otherwise interpret it as a short option.  */
      if (!long_only || argv[optind][1] == '-'
	  || my_index (optstring, *nextchar) == NULL)
	{
	  if (opterr)
	    {
	      if (argv[optind][1] == '-')
		/* --option */
		fprintf (stderr, _("%s: unrecognized option `--%s'\n"),
			 argv[0], nextchar);
	      else
		/* +option or -option */
		fprintf (stderr, _("%s: unrecognized option `%c%s'\n"),
			 argv[0], argv[optind][0], nextchar);
	    }
	  nextchar = (char *) "";
	  optind++;
	  return '?';
	}
    }

  /* Look at and handle the next short option-character.  */

  {
    char c = *nextchar++;
    char *temp = my_index (optstring, c);

    /* Increment `optind' when we start to process its last character.  */
    if (*nextchar == '\0')
      ++optind;

    if (temp == NULL || c == ':')
      {
	if (opterr)
	  {
	    if (posixly_correct)
	      /* 1003.2 specifies the format of this message.  */
	      fprintf (stderr, _("%s: illegal option -- %c\n"), argv[0], c);
	    else
	      fprintf (stderr, _("%s: invalid option -- %c\n"), argv[0], c);
	  }
	optopt = c;
	return '?';
      }
    if (temp[1] == ':')
      {
	if (temp[2] == ':')
	  {
	    /* This is an option that accepts an argument optionally.  */
	    if (*nextchar != '\0')
	      {
		optarg = nextchar;
		optind++;
	      }
	    else
	      optarg = NULL;
	    nextchar = NULL;
	  }
	else
	  {
	    /* This is an option that requires an argument.  */
	    if (*nextchar != '\0')
	      {
		optarg = nextchar;
		/* If we end this ARGV-element by taking the rest as an arg,
		   we must advance to the next element now.  */
		optind++;
	      }
	    else if (optind == argc)
	      {
		if (opterr)
		  {
		    /* 1003.2 specifies the format of this message.  */
		    fprintf (stderr, _("%s: option requires an argument -- %c\n"),
			     argv[0], c);
		  }
		optopt = c;
		if (optstring[0] == ':')
		  c = ':';
		else
		  c = '?';
	      }
	    else
	      /* We already incremented `optind' once;
		 increment it again when taking next ARGV-elt as argument.  */
	      optarg = argv[optind++];
	    nextchar = NULL;
	  }
      }
    return c;
  }
}

int
getopt (int argc, char *const *argv, const char *optstring)
{
  return _getopt_internal (argc, argv, optstring,
			   (const struct option *) 0,
			   (int *) 0,
			   0);
}

#endif 		// !HAVE_GETOPT

#ifndef HAVE_STRCASECMP

/* Compare S1 and S2, ignoring case, returning less than, equal to or
   greater than zero if S1 is lexiographically less than,
   equal to or greater than S2.
   
   Copyright (C) 1991, 1992, 1998 Free Software Foundation, Inc.
*/

int
strcasecmp (c_char s1, c_char s2)
{
  register const unsigned char *p1 = (const unsigned char *) s1;
  register const unsigned char *p2 = (const unsigned char *) s2;
  unsigned char c1, c2;

  if (p1 == p2)
    return 0;

  do
    {
      c1 = tolower (*p1++);
      c2 = tolower (*p2++);
      if (c1 == '\0')
	break;
    }
  while (c1 == c2);

  return c1 - c2;
}

#endif		// !HAVE_STRCASECMP

#ifndef HAVE_STRNCASECMP

/*
 * Author: Tero Kivinen <kivinen@iki.fi>
 * 
 * Copyright (c) 1996 SSH Communications Security Oy <info@ssh.fi>
 */

int
strncasecmp (const char *s1, const char *s2, size_t len)
{
  while (len-- > 1 && *s1 && (*s1 == *s2 || tolower(*s1) == tolower(*s2)))
    {
      s1++;
      s2++;
    }
  return (int) *(unsigned char *)s1 - (int) *(unsigned char *)s2;
}

#endif		// !HAVE_STRNCASECMP

#ifndef HAVE_MEMCPY

/*
 * Copyright (c) 1990, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 */

typedef int word;               /* "word" used for optimal copy speed */

#define wsize   sizeof(word)
#define wmask   (wsize - 1)

void *
memcpy (void *dst0, const void *src0, register size_t length)
{
        register char *dst = (char *)dst0;
        register const char *src = (const char *)src0;
        register size_t t;

        if (length == 0 || dst == src)          /* nothing to do */
                goto done;

        /*
         * Macros: loop-t-times; and loop-t-times, t>0
         */
#define TLOOP(s) if (t) TLOOP1(s)
#define TLOOP1(s) do { s; } while (--t)

        if ((unsigned long)dst < (unsigned long)src) {
                /*
                 * Copy forward.
                 */
                t = (int)src;   /* only need low bits */
                if ((t | (int)dst) & wmask) {
                        /*
                         * Try to align operands.  This cannot be done
                         * unless the low bits match.
                         */
                        if ((t ^ (int)dst) & wmask || length < wsize)
                                t = length;
                        else
                                t = wsize - (t & wmask);
                        length -= t;
                        TLOOP1(*dst++ = *src++);
                }
                /*
                 * Copy whole words, then mop up any trailing bytes.
                 */
                t = length / wsize;
                TLOOP(*(word *)dst = *(word *)src; src += wsize; dst += wsize);
                t = length & wmask;
                TLOOP(*dst++ = *src++);
        } else {
                /*
                 * Copy backwards.  Otherwise essentially the same.
                 * Alignment works as before, except that it takes
                 * (t&wmask) bytes to align, not wsize-(t&wmask).
                 */
                src += length;
                dst += length;
                t = (int)src;
                if ((t | (int)dst) & wmask) {
                        if ((t ^ (int)dst) & wmask || length <= wsize)
                                t = length;
                        else
                                t &= wmask;
                        length -= t;
                        TLOOP1(*--dst = *--src);
                }
                t = length / wsize;
                TLOOP(src -= wsize; dst -= wsize; *(word *)dst = *(word *)src);
                t = length & wmask;
                TLOOP(*--dst = *--src);
        }
done:
        return (dst0);
}

#endif		// !HAVE_MEMCPY

#ifndef MBOT_CRYPT

/*
 * UFC-crypt: ultra fast crypt(3) implementation
 *
 * Copyright (C) 1991, 1992, 1993, 1996 Free Software Foundation, Inc.
*/

typedef uint_fast32_t ufc_long;
typedef uint32_t long32;

struct crypt_data
  {
    char keysched[16 * 8];
    char sb0[32768];
    char sb1[32768];
    char sb2[32768];
    char sb3[32768];
    /* end-of-aligment-critical-data */
    char crypt_3_buf[14];
    char current_salt[2];
    long int current_saltbits;
    int  direction, initialized;
  };

struct crypt_data _ufc_foobar;

#define SBA(sb, v) (*(long32*)((char*)(sb)+(v)))

static const ufc_long BITMASK[24] = {
  0x40000000, 0x20000000, 0x10000000, 0x08000000, 0x04000000, 0x02000000,
  0x01000000, 0x00800000, 0x00400000, 0x00200000, 0x00100000, 0x00080000,
  0x00004000, 0x00002000, 0x00001000, 0x00000800, 0x00000400, 0x00000200,
  0x00000100, 0x00000080, 0x00000040, 0x00000020, 0x00000010, 0x00000008
};

static ufc_long do_pc1[8][2][128];
static ufc_long do_pc2[8][128];

static const int pc1[56] = {
  57, 49, 41, 33, 25, 17,  9,  1, 58, 50, 42, 34, 26, 18,
  10,  2, 59, 51, 43, 35, 27, 19, 11,  3, 60, 52, 44, 36,
  63, 55, 47, 39, 31, 23, 15,  7, 62, 54, 46, 38, 30, 22,
  14,  6, 61, 53, 45, 37, 29, 21, 13,  5, 28, 20, 12,  4
};

static const int pc2[48] = {
  14, 17, 11, 24,  1,  5,  3, 28, 15,  6, 21, 10,
  23, 19, 12,  4, 26,  8, 16,  7, 27, 20, 13,  2,
  41, 52, 31, 37, 47, 55, 30, 40, 51, 45, 33, 48,
  44, 49, 39, 56, 34, 53, 46, 42, 50, 36, 29, 32
};

static const int esel[48] = {
  32,  1,  2,  3,  4,  5,  4,  5,  6,  7,  8,  9,
   8,  9, 10, 11, 12, 13, 12, 13, 14, 15, 16, 17,
  16, 17, 18, 19, 20, 21, 20, 21, 22, 23, 24, 25,
  24, 25, 26, 27, 28, 29, 28, 29, 30, 31, 32,  1
};

static const int perm32[32] = {
  16,  7, 20, 21, 29, 12, 28, 17,  1, 15, 23, 26,  5, 18, 31, 10,
  2,   8, 24, 14, 32, 27,  3,  9, 19, 13, 30,  6, 22, 11,  4, 25
};

static ufc_long efp[16][64][2];

static const int final_perm[64] = {
  40,  8, 48, 16, 56, 24, 64, 32, 39,  7, 47, 15, 55, 23, 63, 31,
  38,  6, 46, 14, 54, 22, 62, 30, 37,  5, 45, 13, 53, 21, 61, 29,
  36,  4, 44, 12, 52, 20, 60, 28, 35,  3, 43, 11, 51, 19, 59, 27,
  34,  2, 42, 10, 50, 18, 58, 26, 33,  1, 41,  9, 49, 17, 57, 25
};

static ufc_long eperm32tab[4][256][2];

static const unsigned char bytemask[8]  = {
  0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
};

static const ufc_long longmask[32] = {
  0x80000000, 0x40000000, 0x20000000, 0x10000000,
  0x08000000, 0x04000000, 0x02000000, 0x01000000,
  0x00800000, 0x00400000, 0x00200000, 0x00100000,
  0x00080000, 0x00040000, 0x00020000, 0x00010000,
  0x00008000, 0x00004000, 0x00002000, 0x00001000,
  0x00000800, 0x00000400, 0x00000200, 0x00000100,
  0x00000080, 0x00000040, 0x00000020, 0x00000010,
  0x00000008, 0x00000004, 0x00000002, 0x00000001
};

static const int sbox[8][4][16]= {
        { { 14,  4, 13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7 },
          {  0, 15,  7,  4, 14,  2, 13,  1, 10,  6, 12, 11,  9,  5,  3,  8 },
          {  4,  1, 14,  8, 13,  6,  2, 11, 15, 12,  9,  7,  3, 10,  5,  0 },
          { 15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13 }
        },

        { { 15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10 },
          {  3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5 },
          {  0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15 },
          { 13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9 }
        },

        { { 10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8 },
          { 13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1 },
          { 13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7 },
          {  1, 10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12 }
        },

        { {  7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15 },
          { 13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9 },
          { 10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4 },
          {  3, 15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14 }
        },

        { {  2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9 },
          { 14, 11,  2, 12,  4,  7, 13,  1,  5,  0, 15, 10,  3,  9,  8,  6 },
          {  4,  2,  1, 11, 10, 13,  7,  8, 15,  9, 12,  5,  6,  3,  0, 14 },
          { 11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3 }
        },

        { { 12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11 },
          { 10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8 },
          {  9, 14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6 },
          {  4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13 }
        },

        { {  4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1 },
          { 13,  0, 11,  7,  4,  9,  1, 10, 14,  3,  5, 12,  2, 15,  8,  6 },
          {  1,  4, 11, 13, 12,  3,  7, 14, 10, 15,  6,  8,  0,  5,  9,  2 },
          {  6, 11, 13,  8,  1,  4, 10,  7,  9,  5,  0, 15, 14,  2,  3, 12 }
        },

        { { 13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7 },
          {  1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2 },
          {  7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8 },
          {  2,  1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11 }
        }
};

static const int rots[16] = {
  1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1
};

#define s_lookup(i,s) sbox[(i)][(((s)>>4) & 0x2)|((s) & 0x1)][((s)>>1) & 0xf];
#define ascii_to_bin(c) ((c)>='a'?(c-59):(c)>='A'?((c)-53):(c)-'.')
#define bin_to_ascii(c) ((c)>=38?((c)-38+'a'):(c)>=12?((c)-12+'A'):(c)+'.')

void
_ufc_mk_keytab_r(c_char key, struct crypt_data *__data)
{
  ufc_long v1, v2, *k1;
  int i;
  long32 v, *k2;
  k2 = (long32*)__data->keysched;

  v1 = v2 = 0; k1 = &do_pc1[0][0][0];
  for(i = 8; i--;) {
    v1 |= k1[*key   & 0x7f]; k1 += 128;
    v2 |= k1[*key++ & 0x7f]; k1 += 128;
  }

  for(i = 0; i < 16; i++) {
    k1 = &do_pc2[0][0];

    v1 = (v1 << rots[i]) | (v1 >> (28 - rots[i]));
    v  = k1[(v1 >> 21) & 0x7f]; k1 += 128;
    v |= k1[(v1 >> 14) & 0x7f]; k1 += 128;
    v |= k1[(v1 >>  7) & 0x7f]; k1 += 128;
    v |= k1[(v1      ) & 0x7f]; k1 += 128;

    *k2++ = (v | 0x00008000);
    v = 0;

    v2 = (v2 << rots[i]) | (v2 >> (28 - rots[i]));
    v |= k1[(v2 >> 21) & 0x7f]; k1 += 128;
    v |= k1[(v2 >> 14) & 0x7f]; k1 += 128;
    v |= k1[(v2 >>  7) & 0x7f]; k1 += 128;
    v |= k1[(v2      ) & 0x7f];

    *k2++ = (v | 0x00008000);
  }

  __data->direction = 0;
}

void
shuffle_sb(long32 *k, ufc_long saltbits)
{
  ufc_long j;
  long32 x;
  for(j=4096; j--;) {
    x = (k[0] ^ k[1]) & (long32)saltbits;
    *k++ ^= x;
    *k++ ^= x;
  }
}

void
_ufc_clearmem(char *start, int cnt)
{
  while(cnt--)
    *start++ = '\0';
}

void
__init_des_r(struct crypt_data *__data)
{
  int comes_from_bit;
  int bit, sg;
  ufc_long j;
  ufc_long mask1, mask2;
  int e_inverse[64];
  static volatile int small_tables_initialized = 0;

  long32 *sb[4];
  sb[0] = (long32*)__data->sb0; sb[1] = (long32*)__data->sb1;
  sb[2] = (long32*)__data->sb2; sb[3] = (long32*)__data->sb3;

  if(small_tables_initialized == 0) {

    /*
     * Create the do_pc1 table used
     * to affect pc1 permutation
     * when generating keys
     */
    _ufc_clearmem((char*)do_pc1, (int)sizeof(do_pc1));
    for(bit = 0; bit < 56; bit++) {
      comes_from_bit  = pc1[bit] - 1;
      mask1 = bytemask[comes_from_bit % 8 + 1];
      mask2 = longmask[bit % 28 + 4];
      for(j = 0; j < 128; j++) {
        if(j & mask1)
          do_pc1[comes_from_bit / 8][bit / 28][j] |= mask2;
      }
    }

    /*
     * Create the do_pc2 table used
     * to affect pc2 permutation when
     * generating keys
     */
    _ufc_clearmem((char*)do_pc2, (int)sizeof(do_pc2));
    for(bit = 0; bit < 48; bit++) {
      comes_from_bit  = pc2[bit] - 1;
      mask1 = bytemask[comes_from_bit % 7 + 1];
      mask2 = BITMASK[bit % 24];
      for(j = 0; j < 128; j++) {
        if(j & mask1)
          do_pc2[comes_from_bit / 7][j] |= mask2;
      }
    }

    /*
     * Now generate the table used to do combined
     * 32 bit permutation and e expansion
     *
     * We use it because we have to permute 16384 32 bit
     * longs into 48 bit in order to initialize sb.
     *
     * Looping 48 rounds per permutation becomes
     * just too slow...
     *
     */

    _ufc_clearmem((char*)eperm32tab, (int)sizeof(eperm32tab));
    for(bit = 0; bit < 48; bit++) {
      ufc_long mask1,comes_from;
      comes_from = perm32[esel[bit]-1]-1;
      mask1      = bytemask[comes_from % 8];
      for(j = 256; j--;) {
        if(j & mask1)
          eperm32tab[comes_from / 8][j][bit / 24] |= BITMASK[bit % 24];
      }
    }

    /*
     * Create an inverse matrix for esel telling
     * where to plug out bits if undoing it
     */
    for(bit=48; bit--;) {
      e_inverse[esel[bit] - 1     ] = bit;
      e_inverse[esel[bit] - 1 + 32] = bit + 48;
    }

    /*
     * create efp: the matrix used to
     * undo the E expansion and effect final permutation
     */
    _ufc_clearmem((char*)efp, (int)sizeof efp);
    for(bit = 0; bit < 64; bit++) {
      int o_bit, o_long;
      ufc_long word_value, mask1, mask2;
      int comes_from_f_bit, comes_from_e_bit;
      int comes_from_word, bit_within_word;

      /* See where bit i belongs in the two 32 bit long's */
      o_long = bit / 32; /* 0..1  */
      o_bit  = bit % 32; /* 0..31 */

      /*
       * And find a bit in the e permutated value setting this bit.
       *
       * Note: the e selection may have selected the same bit several
       * times. By the initialization of e_inverse, we only look
       * for one specific instance.
       */
      comes_from_f_bit = final_perm[bit] - 1;         /* 0..63 */
      comes_from_e_bit = e_inverse[comes_from_f_bit]; /* 0..95 */
      comes_from_word  = comes_from_e_bit / 6;        /* 0..15 */
      bit_within_word  = comes_from_e_bit % 6;        /* 0..5  */

      mask1 = longmask[bit_within_word + 26];
      mask2 = longmask[o_bit];

      for(word_value = 64; word_value--;) {
        if(word_value & mask1)
          efp[comes_from_word][word_value][o_long] |= mask2;
      }
    }
    small_tables_initialized = 1;
  }

  /*
   * Create the sb tables:
   *
   * For each 12 bit segment of an 48 bit intermediate
   * result, the sb table precomputes the two 4 bit
   * values of the sbox lookups done with the two 6
   * bit halves, shifts them to their proper place,
   * sends them through perm32 and finally E expands
   * them so that they are ready for the next
   * DES round.
   *
   */

  _ufc_clearmem((char*)__data->sb0, (int)sizeof(__data->sb0));
  _ufc_clearmem((char*)__data->sb1, (int)sizeof(__data->sb1));
  _ufc_clearmem((char*)__data->sb2, (int)sizeof(__data->sb2));
  _ufc_clearmem((char*)__data->sb3, (int)sizeof(__data->sb3));

  for(sg = 0; sg < 4; sg++) {
    int j1, j2;
    int s1, s2;

    for(j1 = 0; j1 < 64; j1++) {
      s1 = s_lookup(2 * sg, j1);
      for(j2 = 0; j2 < 64; j2++) {
        ufc_long to_permute, inx;

        s2         = s_lookup(2 * sg + 1, j2);
        to_permute = (((ufc_long)s1 << 4)  |
                      (ufc_long)s2) << (24 - 8 * (ufc_long)sg);

        inx = ((j1 << 6)  | j2) << 1;
        sb[sg][inx  ]  = eperm32tab[0][(to_permute >> 24) & 0xff][0];
        sb[sg][inx+1]  = eperm32tab[0][(to_permute >> 24) & 0xff][1];
        sb[sg][inx  ] |= eperm32tab[1][(to_permute >> 16) & 0xff][0];
        sb[sg][inx+1] |= eperm32tab[1][(to_permute >> 16) & 0xff][1];
        sb[sg][inx  ] |= eperm32tab[2][(to_permute >>  8) & 0xff][0];
        sb[sg][inx+1] |= eperm32tab[2][(to_permute >>  8) & 0xff][1];
        sb[sg][inx  ] |= eperm32tab[3][(to_permute)       & 0xff][0];
        sb[sg][inx+1] |= eperm32tab[3][(to_permute)       & 0xff][1];
      }
    }
  }

  __data->initialized++;
}

void
_ufc_dofinalperm_r(ufc_long *res, struct crypt_data *__data)
{
  ufc_long v1, v2, x;
  ufc_long l1,l2,r1,r2;

  l1 = res[0]; l2 = res[1];
  r1 = res[2]; r2 = res[3];

  x = (l1 ^ l2) & __data->current_saltbits; l1 ^= x; l2 ^= x;
  x = (r1 ^ r2) & __data->current_saltbits; r1 ^= x; r2 ^= x;

  v1=v2=0; l1 >>= 3; l2 >>= 3; r1 >>= 3; r2 >>= 3;

  v1 |= efp[15][ r2         & 0x3f][0]; v2 |= efp[15][ r2 & 0x3f][1];
  v1 |= efp[14][(r2 >>= 6)  & 0x3f][0]; v2 |= efp[14][ r2 & 0x3f][1];
  v1 |= efp[13][(r2 >>= 10) & 0x3f][0]; v2 |= efp[13][ r2 & 0x3f][1];
  v1 |= efp[12][(r2 >>= 6)  & 0x3f][0]; v2 |= efp[12][ r2 & 0x3f][1];

  v1 |= efp[11][ r1         & 0x3f][0]; v2 |= efp[11][ r1 & 0x3f][1];
  v1 |= efp[10][(r1 >>= 6)  & 0x3f][0]; v2 |= efp[10][ r1 & 0x3f][1];
  v1 |= efp[ 9][(r1 >>= 10) & 0x3f][0]; v2 |= efp[ 9][ r1 & 0x3f][1];
  v1 |= efp[ 8][(r1 >>= 6)  & 0x3f][0]; v2 |= efp[ 8][ r1 & 0x3f][1];

  v1 |= efp[ 7][ l2         & 0x3f][0]; v2 |= efp[ 7][ l2 & 0x3f][1];
  v1 |= efp[ 6][(l2 >>= 6)  & 0x3f][0]; v2 |= efp[ 6][ l2 & 0x3f][1];
  v1 |= efp[ 5][(l2 >>= 10) & 0x3f][0]; v2 |= efp[ 5][ l2 & 0x3f][1];
  v1 |= efp[ 4][(l2 >>= 6)  & 0x3f][0]; v2 |= efp[ 4][ l2 & 0x3f][1];

  v1 |= efp[ 3][ l1         & 0x3f][0]; v2 |= efp[ 3][ l1 & 0x3f][1];
  v1 |= efp[ 2][(l1 >>= 6)  & 0x3f][0]; v2 |= efp[ 2][ l1 & 0x3f][1];
  v1 |= efp[ 1][(l1 >>= 10) & 0x3f][0]; v2 |= efp[ 1][ l1 & 0x3f][1];
  v1 |= efp[ 0][(l1 >>= 6)  & 0x3f][0]; v2 |= efp[ 0][ l1 & 0x3f][1];

  res[0] = v1; res[1] = v2;
}

void
_ufc_output_conversion_r(ufc_long v1, ufc_long v2, c_char salt, struct crypt_data *__data)
{
  int i, s, shf;

  __data->crypt_3_buf[0] = salt[0];
  __data->crypt_3_buf[1] = salt[1] ? salt[1] : salt[0];

  for(i = 0; i < 5; i++) {
    shf = (26 - 6 * i); /* to cope with MSC compiler bug */
    __data->crypt_3_buf[i + 2] = bin_to_ascii((v1 >> shf) & 0x3f);
  }

  s  = (v2 & 0xf) << 2;
  v2 = (v2 >> 2) | ((v1 & 0x3) << 30);

  for(i = 5; i < 10; i++) {
    shf = (56 - 6 * i);
    __data->crypt_3_buf[i + 2] = bin_to_ascii((v2 >> shf) & 0x3f);
  }

  __data->crypt_3_buf[12] = bin_to_ascii(s);
  __data->crypt_3_buf[13] = 0;
}

void
_ufc_setup_salt_r(c_char s, struct crypt_data *__data)
{
  ufc_long i, j, saltbits;

  if(__data->initialized == 0)
    __init_des_r(__data);

  if(s[0] == __data->current_salt[0] && s[1] == __data->current_salt[1])
    return;
  __data->current_salt[0] = s[0]; __data->current_salt[1] = s[1];

  /*
   * This is the only crypt change to DES:
   * entries are swapped in the expansion table
   * according to the bits set in the salt.
   */
  saltbits = 0;
  for(i = 0; i < 2; i++) {
    long c=ascii_to_bin(s[i]);
    for(j = 0; j < 6; j++) {
      if((c >> j) & 0x1)
        saltbits |= BITMASK[6 * i + j];
    }
  }

  /*
   * Permute the sb table values
   * to reflect the changed e
   * selection table
   */

#define LONGG long32*

  shuffle_sb((LONGG)__data->sb0, __data->current_saltbits ^ saltbits);
  shuffle_sb((LONGG)__data->sb1, __data->current_saltbits ^ saltbits);
  shuffle_sb((LONGG)__data->sb2, __data->current_saltbits ^ saltbits);
  shuffle_sb((LONGG)__data->sb3, __data->current_saltbits ^ saltbits);

  __data->current_saltbits = saltbits;
}


void
_ufc_doit_r(ufc_long itr, struct crypt_data *__data, ufc_long *res)
{
  int i;
  long32 s, *k;
  long32 *sb01 = (long32*)__data->sb0;
  long32 *sb23 = (long32*)__data->sb2;
  long32 l1, l2, r1, r2;

  l1 = (long32)res[0]; l2 = (long32)res[1];
  r1 = (long32)res[2]; r2 = (long32)res[3];

  while(itr--) {
    k = (long32*)__data->keysched;
    for(i=8; i--; ) {
      s = *k++ ^ r1;
      l1 ^= SBA(sb01, s & 0xffff); l2 ^= SBA(sb01, (s & 0xffff)+4);
      l1 ^= SBA(sb01, s >>= 16  ); l2 ^= SBA(sb01, (s         )+4);
      s = *k++ ^ r2;
      l1 ^= SBA(sb23, s & 0xffff); l2 ^= SBA(sb23, (s & 0xffff)+4);
      l1 ^= SBA(sb23, s >>= 16  ); l2 ^= SBA(sb23, (s         )+4);

      s = *k++ ^ l1;
      r1 ^= SBA(sb01, s & 0xffff); r2 ^= SBA(sb01, (s & 0xffff)+4);
      r1 ^= SBA(sb01, s >>= 16  ); r2 ^= SBA(sb01, (s         )+4);
      s = *k++ ^ l2;
      r1 ^= SBA(sb23, s & 0xffff); r2 ^= SBA(sb23, (s & 0xffff)+4);
      r1 ^= SBA(sb23, s >>= 16  ); r2 ^= SBA(sb23, (s         )+4);
    }
    s=l1; l1=r1; r1=s; s=l2; l2=r2; r2=s;
  }
  res[0] = l1; res[1] = l2; res[2] = r1; res[3] = r2;
}


char *
__crypt_r (c_char key, c_char salt, struct crypt_data *data)
{
  ufc_long res[4];
  char ktab[9];
  ufc_long xx = 25; /* to cope with GCC long long compiler bugs */

  /*
   * Hack DES tables according to salt
   */
  _ufc_setup_salt_r (salt, data);

  /*
   * Setup key schedule
   */
  _ufc_clearmem (ktab, (int) sizeof (ktab));
  (void) strncpy (ktab, key, 8);
  _ufc_mk_keytab_r (ktab, data);

  /*
   * Go for the 25 DES encryptions
   */
  _ufc_clearmem ((char*) res, (int) sizeof (res));
  _ufc_doit_r (xx,  data, &res[0]);

  /*
   * Do final permutations
   */
  _ufc_dofinalperm_r (res, data);

  /*
   * And convert back to 6 bit ASCII
   */
  _ufc_output_conversion_r (res[0], res[1], salt, data);
  return data->crypt_3_buf;
}

char *
crypt (c_char key, c_char salt)
{
  return __crypt_r (key, salt, &_ufc_foobar);
}

#endif		// !MBOT_CRYPT

#ifndef HAVE_VSNPRINTF

/*
  Author: Tomi Salo <ttsalo@ssh.fi>

  Copyright (C) 1996 SSH Communications Security Oy, Espoo, Finland
  All rights reserved.

  Implementation of functions snprintf() and vsnprintf()

  */

#define MINUS_FLAG 0x1
#define PLUS_FLAG 0x2
#define SPACE_FLAG 0x4
#define HASH_FLAG 0x8
#define CONV_TO_SHORT 0x10
#define IS_LONG_INT 0x20
#define IS_LONG_DOUBLE 0x40
#define X_UPCASE 0x80
#define IS_NEGATIVE 0x100
#define UNSIGNED_DEC 0x200
#define ZERO_PADDING 0x400

/* Convert a integer from unsigned long int representation 
   to string representation. This will insert prefixes if needed 
   (leading zero for octal and 0x or 0X for hexadecimal) and
   will write at most buf_size characters to buffer.
   tmp_buf is used because we want to get correctly truncated
   results.
   */

static int
snprintf_convert_ulong(char *buffer, size_t buf_size, int base, char *digits,
                       unsigned long int ulong_val, int flags, int width,
                       int precision)
{
  int tmp_buf_len = 100 + width, len, written = 0;
  char *tmp_buf, *tmp_buf_ptr, prefix[2];
  tmp_buf = (char *)my_malloc(tmp_buf_len);

  prefix[0] = '\0';
  prefix[1] = '\0';
  
  /* Make tmp_buf_ptr point just past the last char of buffer */
  tmp_buf_ptr = tmp_buf + tmp_buf_len;

  if (precision < 0)
    precision = 0;

  /* Main conversion loop */
  do 
    {
      *--tmp_buf_ptr = digits[ulong_val % base];
      ulong_val /= base;
      precision--;
    } 
  while ((ulong_val != 0 || precision > 0) && tmp_buf_ptr > tmp_buf);
 
  /* Get the prefix */
  if (!(flags & IS_NEGATIVE))
    {
      if (base == 16 && (flags & HASH_FLAG))
        {
          if (flags && X_UPCASE)
            {
              prefix[0] = 'x';
              prefix[1] = '0';
            }
          else
            {
              prefix[0] = 'X';
              prefix[1] = '0';
            }
        }
      
      if (base == 8 && (flags & HASH_FLAG))
        prefix[0] = '0';
      
      if (base == 10 && !(flags & UNSIGNED_DEC) && (flags & PLUS_FLAG))
        prefix[0] = '+';
      else
        {
          if (base == 10 && !(flags & UNSIGNED_DEC) && (flags & SPACE_FLAG))
            prefix[0] = ' ';
        }
    }
  else
      prefix[0] = '-';

  if (prefix[0] != '\0' && tmp_buf_ptr > tmp_buf)
    {
      *--tmp_buf_ptr = prefix[0];
      if (prefix[1] != '\0' && tmp_buf_ptr > tmp_buf)
        *--tmp_buf_ptr = prefix[1];
    }
  
  len = (tmp_buf + tmp_buf_len) - tmp_buf_ptr;

  /* Now:
     - len is the length of the actual converted number,
       which is pointed to by tmp_buf_ptr.
     - buf_size is how much space we have.
     - width is the minimum width requested by the user.
     The following code writes the number and padding into
     the buffer and returns the number of characters written.
     If the MINUS_FLAG is set, the number will be left-justified,
     and if it is not set, the number will be right-justified.
     */

  while (buf_size - written > 0)
    {
      /* Write until the buffer is full. If stuff to write is exhausted
         first, return straight from the loop. */
      if (flags & MINUS_FLAG)
        {
          if (written < len)
            buffer[written] = tmp_buf_ptr[written];
          else
            {
              if (written >= width)
                {
                  free(tmp_buf);
                  return written;
                }
              buffer[written] = (flags & ZERO_PADDING) ? '0': ' ';
            }
          written++;
        }
      else
        {
          if (width > len && written < width - len)
            buffer[written] = (flags & ZERO_PADDING) ? '0': ' ';
          else
            {
              if (width > len)
                buffer[written] = tmp_buf_ptr[written - (width - len)];
              else
                buffer[written] = tmp_buf_ptr[written];
            }
          written++;
          if (written >= width && written >= len)
            {
              free(tmp_buf);
              return written;
            }
        }
    }
  free(tmp_buf);
  return written;
}

static int
snprintf_convert_float(char *buffer, size_t buf_size,
                       double dbl_val, int flags, int width,
                       int precision, char format_char)
{
  char print_buf[160], print_buf_len = 0;
  char format_str[80], *format_str_ptr;

  format_str_ptr = format_str;

  if (width > 155) width = 155;
  if (precision < 0)
    precision = 6;
  if (precision > 120)
    precision = 120;

  /* Construct the formatting string and let system's sprintf
     do the real work. */
  
  *format_str_ptr++ = '%';

  if (flags & MINUS_FLAG)
    *format_str_ptr++ = '-';
  if (flags & PLUS_FLAG)
    *format_str_ptr++ = '+';
  if (flags & SPACE_FLAG)
    *format_str_ptr++ = ' ';
  if (flags & ZERO_PADDING)
    *format_str_ptr++ = '0';
  if (flags & HASH_FLAG)
    *format_str_ptr++ = '#';
    
  sprintf(format_str_ptr, "%d.%d", width, precision);
  format_str_ptr += strlen(format_str_ptr);

  if (flags & IS_LONG_DOUBLE)
    *format_str_ptr++ = 'L';
  *format_str_ptr++ = format_char;
  *format_str_ptr++ = '\0';

  sprintf(print_buf, format_str, dbl_val);
  print_buf_len = strlen(print_buf);

  if ((u_char)print_buf_len > buf_size)
    print_buf_len = buf_size;
  strncpy(buffer, print_buf, print_buf_len);
  return print_buf_len;
}

int
vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
  int status, left = (int)size - 1;
  const char *format_ptr = format;
  int flags, width, precision, i;
  char format_char, *orig_str = str;
  int *int_ptr;
  long int long_val;
  unsigned long int ulong_val;
  char *str_val;
  double dbl_val;

  while (format_ptr < format + strlen(format))
    {
      if (*format_ptr == '%')
        {
          if (format_ptr[1] == '%' && left > 0)
            {
              /* Format `%%' at format string as `%' */
              *str++ = '%';
              left--;
              format_ptr += 2;
            }
          else
            {
              /* Other format directive. */
              if (left <= 0)
                {
                  /* No space left on output */
                  str += left;
                  *str = '\0';
                  return size;
                }
              else
                {
                  int length, value;
                  const char *format_start = format_ptr;

                  flags = 0; 
                  width = 0;
                  precision = -1;
                  format_char = (char)0;

                  /* Get the flags */
                  format_ptr++;
                  while (*format_ptr == '-' || *format_ptr == '+' || 
                         *format_ptr == ' ' || *format_ptr == '#' ||
                         *format_ptr == '0')
                    {
                      switch (*format_ptr)
                        {
                        case '-':
                          flags |= MINUS_FLAG;
                          break;
                        case '+':
                          flags |= PLUS_FLAG;
                          break;
                        case ' ':
                          flags |= SPACE_FLAG;
                          break;
                        case '#':
                          flags |= HASH_FLAG;
                          break;
                        case '0':
                          flags |= ZERO_PADDING;
                          break;
                        }
                      format_ptr++;
                    }

                  /* Don't pad left-justified numbers withs zeros */
                  if ((flags & MINUS_FLAG) && (flags & ZERO_PADDING))
                    flags &= ~ZERO_PADDING;
        
                  /* Is width field present? */
                  if (isdigit(*format_ptr))
                    {
                      for (value = 0; 
                           *format_ptr && isdigit(*format_ptr); 
                           format_ptr++)
                        value = 10 * value + *format_ptr - '0';
                      
                      width = value;
                    }
                  else
                    {
                      if (*format_ptr == '*')
                        {
                          width = va_arg(ap, int);
                          format_ptr++;
                        }
                    }

                  /* Is the precision field present? */
                  if (*format_ptr == '.')
                    {
                      format_ptr++;
                      if (isdigit(*format_ptr))
                        {
                          for (value = 0; 
                               *format_ptr && isdigit(*format_ptr); 
                               format_ptr++)
                            value = 10 * value + *format_ptr - '0';

                          precision = value;
                        }
                      else
                        {
                          if (*format_ptr == '*')
                            {
                              precision = va_arg(ap, int);
                              format_ptr++;
                            }
                          else
                            precision = 0;
                        }
                    }

                  switch (*format_ptr)
                    {
                    case 'h':
                      flags |= CONV_TO_SHORT;
                      format_ptr++;
                      break;
                    case 'l':
                      flags |= IS_LONG_INT;
                      format_ptr++;
                      break;
                    case 'L':
                      flags |= IS_LONG_DOUBLE;
                      format_ptr++;
                      break;
                    default:
                      break;
                    }
      
                  /* Get and check the formatting character */
                  format_char = *format_ptr;
                  format_ptr++;
                  length = format_ptr - format_start;
                        
                  switch (format_char)
                    {
                    case 'c': case 's': case 'p': case 'n':
                    case 'd': case 'i': case 'o': 
                    case 'u': case 'x': case 'X': 
                    case 'f': case 'e': case 'E': case 'g': case 'G': 
                      if (format_char == 'X')
                        flags |= X_UPCASE;
                      if (format_char == 'o')
                        flags |= UNSIGNED_DEC;
                      status = length;
                      break;
                    
                    default:
                      status = 0;
                    }

                  if (status == 0)
                    {
                      /* Invalid format directive. Fail with zero return. */
                      *str = '\0';
                      return 0;
                    }
                  else
                    {
                      /* Print argument according to the directive. */
                      switch (format_char)
                        {
                        case 'i': case 'd':
                          /* Convert to unsigned long int before
                             actual conversion to string */
                          if (flags & IS_LONG_INT)
                            long_val = va_arg(ap, long int);
                          else
                            long_val = (long int) va_arg(ap, int);
                          
                          if (long_val < 0)
                            {
                              ulong_val = (unsigned long int) -long_val;
                              flags |= IS_NEGATIVE;
                            }
                          else
                            {
                              ulong_val = (unsigned long int) long_val;
                            }
                          status = 
                            snprintf_convert_ulong(str, left, 10, "0123456789",
                                                   ulong_val, flags,
                                                   width, precision);
                          str += status;
                          left -= status;
                          break;

                        case 'p':
                          ulong_val = (unsigned long int)
                            va_arg(ap, void *);
                          status =
                            snprintf_convert_ulong(str, left, 16,
                                                   "0123456789abcdef",
                                                   ulong_val, flags,
                                                   width, precision);
                          str += status;
                          left -= status;
                          break;                            

                        case 'x': case 'X':

                          if (flags & IS_LONG_INT)
                            ulong_val = va_arg(ap, unsigned long int);
                          else
                            ulong_val = 
                              (unsigned long int) va_arg(ap, unsigned int);
                          
                          status = 
                            snprintf_convert_ulong(str, left, 16, 
                                                   (format_char == 'x')   ? 
                                                       (char *)"0123456789abcdef" : 
                                                       (char *)"0123456789ABCDEF",
                                                   ulong_val, flags,
                                                   width, precision);
                          str += status;
                          left -= status;
                          
                          break;

                        case 'o':
                          if (flags & IS_LONG_INT)
                            ulong_val = va_arg(ap, unsigned long int);
                          else
                            ulong_val = 
                              (unsigned long int) va_arg(ap, unsigned int);

                          status = snprintf_convert_ulong(str, left, 8,
                                                          "01234567",
                                                          ulong_val, flags,
                                                          width, precision);
                          str += status;
                          left -= status;
                          break;

                        case 'u':
                          if (flags & IS_LONG_INT)
                            ulong_val = va_arg(ap, unsigned long int);
                          else
                            ulong_val = 
                              (unsigned long int) va_arg(ap, unsigned int);

                          status = snprintf_convert_ulong(str, left, 10,
                                                          "0123456789",
                                                          ulong_val, flags,
                                                          width, precision);
                          str += status;
                          left -= status;
                          break;

                        case 'c':
                          if (flags & IS_LONG_INT)
                            ulong_val = va_arg(ap, unsigned long int);
                          else
                            ulong_val = 
                              (unsigned long int) va_arg(ap, unsigned int);
                          *str++ = (unsigned char)ulong_val;
                          left--;
                          break;

                        case 's':
                          str_val = va_arg(ap, char *);

                          if (str_val == NULL)
                            str_val = "(null)";
                          
                          if (precision == -1)
                            precision = strlen(str_val);
                          else
                            {
                              size_t len = strlen(str_val);
                              if ((u_char)len <= precision)
                                precision = len;
                            }
                          if (precision > left)
                            precision = left;

                          if (width > left)
                            width = left;
                          if (width < precision)
                            width = precision;
                          i = width - precision;

                          if (flags & MINUS_FLAG)
                            {
                              strncpy(str, str_val, precision);
                              memset(str + precision,
                                     (flags & ZERO_PADDING)?'0':' ', i);
                            }
                          else
                            {
                              memset(str, (flags & ZERO_PADDING)?'0':' ', i);
                              strncpy(str + i, str_val, precision);
                            }
                          str += width;
                          left -= width;
                          break;

                        case 'n':
                          int_ptr = va_arg(ap, int *);
                          *int_ptr = str - orig_str;
                          break;

                        case 'f': case 'e': case 'E': case 'g': case 'G':
                          if (flags & IS_LONG_DOUBLE)
                            dbl_val = (double) va_arg(ap, long double);
                          else
                            dbl_val = va_arg(ap, double);
                          status =
                            snprintf_convert_float(str, left, dbl_val, flags,
                                                   width, precision,
                                                   format_char);
                          str += status;
                          left -= status;
                          break;
                          
                        default:
                          break;
                        }
                    }
                }
            }
        }
      else
        {
          if (left > 0)
            {
              *str++ = *format_ptr++;
              left--;
            }
          else
            {
              *str = '\0';
              return size;
            }
        }
    }
  if (left < 0)
    str += left;
  *str = '\0';
  return size - left - 1;
}

#endif		// !HAVE_VSNPRINTF

#ifndef HAVE_SNPRINTF

int
snprintf(char *str, size_t size, const char *format, ...)
{
  int ret;
  va_list ap;
  va_start(ap, format);
  ret = vsnprintf(str, size, format, ap);
  va_end(ap);

  return ret;
}

#endif		// !HAVE_SNPRINTF

#ifndef HAVE_GETPID

pid_t
getpid (void)
{
#ifdef WINDOZE
  return GetCurrentProcessId ();
#else
#error no getpid() for this platform
#endif		// !WINDOZE
}

#endif		// !HAVE_GETPID

#ifndef HAVE_TRUNCATE

int
truncate (c_char path, long length)
{
#ifdef WINDOZE
# ifndef INVALID_SET_FILE_POINTER
# define INVALID_SET_FILE_POINTER -1		// this is probably wrong! but it's not defined in VC6's header files...
# endif		// !INVALID_SET_FILE_POINTER
  HANDLE h = CreateFile (path, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (h == INVALID_HANDLE_VALUE)
    return -1;
  if (SetFilePointer (h, length, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    return -1;
  int result = (SetEndOfFile (h) == 0 ? -1 : 0);
  CloseHandle (h);
  return result;
#else
#error no truncate() for this platform
#endif		// !WINDOZE
}

#endif		// !HAVE_TRUNCATE

#ifndef HAVE_STRSEP

char *
strsep (char **stringp, c_char delim)
{
  if (stringp == NULL || *stringp == NULL || delim == NULL)
    return NULL;

  size_t stringp_len = strlen (*stringp), delim_len = strlen (delim), i;
  char *result = *stringp;

  for (i = 0; stringp_len-i+1 >= delim_len; i++)
    {
      if (strncmp (result+i, delim, delim_len) == 0)
     	  {
          *stringp = result+i+delim_len;
          result[i] = 0;
          return result;
        }
      else if (result[i] == 0)
        {
          *stringp = NULL;
          return result;
        }
    }
  return NULL;
}

#endif		// !HAVE_STRSEP
