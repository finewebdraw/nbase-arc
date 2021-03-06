/*
 * Copyright 2015 Naver Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Below codes are from hiredis.c source with some modifications
 *
 * Midofications:
 * - make all function to be local (this file is #inlucde'd)
 * - indentation
 * - add cmdcb_t to the arguments of redis[v]FormatCommand functions
 */

/*
 * Copyright (c) 2009-2011, Salvatore Sanfilippo <antirez at gmail dot com>
 * Copyright (c) 2010-2011, Pieter Noordhuis <pcnoordhuis at gmail dot com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include "win32fixes.h"
#endif
#include "arcci_common.h"

/* Calculate the number of bytes needed to represent an integer as string. */
static int
intlen (int i)
{
  int len = 0;
  if (i < 0)
    {
      len++;
      i = -i;
    }
  do
    {
      len++;
      i /= 10;
    }
  while (i);
  return len;
}

/* Helper that calculates the bulk length given a certain string length. */
static size_t
bulklen (size_t len)
{
  return (size_t) (1 + intlen ((int) len) + 2 + (int) len + 2);
}

static int
redisvFormatCommand (cmdcb_t cb, void *cbarg, char **target,
		     const char *format, va_list ap)
{
  const char *c = format;
  char *cmd = NULL;		/* final command */
  int pos;			/* position in final command */
  sds curarg, newarg;		/* current argument */
  int touched = 0;		/* was the current argument touched? */
  char **curargv = NULL, **newargv = NULL;
  int argc = 0;
  int totlen = 0;
  int j;

  /* Abort if there is not target to set */
  if (target == NULL)
    return -1;

  /* Build the command string accordingly to protocol */
  curarg = sdsempty ();
  if (curarg == NULL)
    return -1;

  while (*c != '\0')
    {
      if (*c != '%' || c[1] == '\0')
	{
	  if (*c == ' ')
	    {
	      if (touched)
		{
		  newargv = realloc (curargv, sizeof (char *) * (argc + 1));
		  if (newargv == NULL)
		    goto err;
		  curargv = newargv;
		  curargv[argc++] = curarg;
		  totlen += (int) bulklen (sdslen (curarg));

		  /* curarg is put in argv so it can be overwritten. */
		  curarg = sdsempty ();
		  if (curarg == NULL)
		    goto err;
		  touched = 0;
		}
	    }
	  else
	    {
	      newarg = sdscatlen (curarg, c, 1);
	      if (newarg == NULL)
		goto err;
	      curarg = newarg;
	      touched = 1;
	    }
	}
      else
	{
	  char *arg;
	  size_t size;

	  /* Set newarg so it can be checked even if it is not touched. */
	  newarg = curarg;

	  switch (c[1])
	    {
	    case 's':
	      arg = va_arg (ap, char *);
	      size = strlen (arg);
	      if (size > 0)
		newarg = sdscatlen (curarg, arg, size);
	      break;
	    case 'b':
	      arg = va_arg (ap, char *);
	      size = va_arg (ap, size_t);
	      if (size > 0)
		newarg = sdscatlen (curarg, arg, size);
	      break;
	    case '%':
	      newarg = sdscat (curarg, "%");
	      break;
	    default:
	      /* Try to detect printf format */
	      {
		static const char intfmts[] = "diouxX";
		char _format[16];
		const char *_p = c + 1;
		size_t _l = 0;
		va_list _cpy;

		/* Flags */
		if (*_p != '\0' && *_p == '#')
		  _p++;
		if (*_p != '\0' && *_p == '0')
		  _p++;
		if (*_p != '\0' && *_p == '-')
		  _p++;
		if (*_p != '\0' && *_p == ' ')
		  _p++;
		if (*_p != '\0' && *_p == '+')
		  _p++;

		/* Field width */
		while (*_p != '\0' && isdigit (*_p))
		  _p++;

		/* Precision */
		if (*_p == '.')
		  {
		    _p++;
		    while (*_p != '\0' && isdigit (*_p))
		      _p++;
		  }

		/* Copy va_list before consuming with va_arg */
		va_copy (_cpy, ap);

		/* Integer conversion (without modifiers) */
		if (strchr (intfmts, *_p) != NULL)
		  {
		    va_arg (ap, int);
		    goto fmt_valid;
		  }

		/* Double conversion (without modifiers) */
		if (strchr ("eEfFgGaA", *_p) != NULL)
		  {
		    va_arg (ap, double);
		    goto fmt_valid;
		  }

		/* Size: char */
		if (_p[0] == 'h' && _p[1] == 'h')
		  {
		    _p += 2;
		    if (*_p != '\0' && strchr (intfmts, *_p) != NULL)
		      {
			va_arg (ap, int);	/* char gets promoted to int */
			goto fmt_valid;
		      }
		    goto fmt_invalid;
		  }

		/* Size: short */
		if (_p[0] == 'h')
		  {
		    _p += 1;
		    if (*_p != '\0' && strchr (intfmts, *_p) != NULL)
		      {
			va_arg (ap, int);	/* short gets promoted to int */
			goto fmt_valid;
		      }
		    goto fmt_invalid;
		  }

		/* Size: long long */
		if (_p[0] == 'l' && _p[1] == 'l')
		  {
		    _p += 2;
		    if (*_p != '\0' && strchr (intfmts, *_p) != NULL)
		      {
			va_arg (ap, long long);
			goto fmt_valid;
		      }
		    goto fmt_invalid;
		  }

		/* Size: long */
		if (_p[0] == 'l')
		  {
		    _p += 1;
		    if (*_p != '\0' && strchr (intfmts, *_p) != NULL)
		      {
			va_arg (ap, long);
			goto fmt_valid;
		      }
		    goto fmt_invalid;
		  }

	      fmt_invalid:
		va_end (_cpy);
		goto err;

	      fmt_valid:
		_l = (_p + 1) - c;
		if (_l < sizeof (_format) - 2)
		  {
		    memcpy (_format, c, _l);
		    _format[_l] = '\0';
		    newarg = sdscatvprintf (curarg, _format, _cpy);

		    /* Update current position (note: outer blocks
		     * increment c twice so compensate here) */
		    c = _p - 1;
		  }

		va_end (_cpy);
		break;
	      }
	    }

	  if (newarg == NULL)
	    goto err;
	  curarg = newarg;

	  touched = 1;
	  c++;
	}
      c++;
    }

  /* Add the last argument if needed */
  if (touched)
    {
      newargv = realloc (curargv, sizeof (char *) * (argc + 1));
      if (newargv == NULL)
	goto err;
      curargv = newargv;
      curargv[argc++] = curarg;
      totlen += (int) bulklen (sdslen (curarg));
    }
  else
    {
      sdsfree (curarg);
    }

  /* Clear curarg because it was put in curargv or was free'd. */
  curarg = NULL;

  /* Add bytes needed to hold multi bulk count */
  totlen += 1 + intlen (argc) + 2;

  /* Build the command at protocol level */
  cmd = (char *) malloc (totlen + 1);
  if (cmd == NULL)
    goto err;

  pos = sprintf (cmd, "*%d\r\n", argc);
  for (j = 0; j < argc; j++)
    {
#ifdef _WIN32
      pos +=
	sprintf (cmd + pos, "$%llu\r\n",
		 (unsigned long long) sdslen (curargv[j]));
#else
      pos += sprintf (cmd + pos, "$%zu\r\n", sdslen (curargv[j]));
#endif
      /* Invoke command callback */
      if (cb != NULL)
	{
	  cb (j, pos, curargv[j], sdslen (curargv[j]), cbarg);
	}
      memcpy (cmd + pos, curargv[j], sdslen (curargv[j]));
      pos += (int) sdslen (curargv[j]);
      sdsfree (curargv[j]);
      cmd[pos++] = '\r';
      cmd[pos++] = '\n';
    }
  assert (pos == totlen);
  cmd[pos] = '\0';

  free (curargv);
  *target = cmd;
  return totlen;

err:
  while (argc--)
    sdsfree (curargv[argc]);
  free (curargv);

  if (curarg != NULL)
    sdsfree (curarg);

  /* No need to check cmd since it is the last statement that can fail,
   * but do it anyway to be as defensive as possible. */
  if (cmd != NULL)
    free (cmd);

  return -1;
}

#if 0
/* Format a command according to the Redis protocol. This function
 * takes a format similar to printf:
 *
 * %s represents a C null terminated string you want to interpolate
 * %b represents a binary safe string
 *
 * When using %b you need to provide both the pointer to the string
 * and the length in bytes. Examples:
 *
 * len = redisFormatCommand(target, "GET %s", mykey);
 * len = redisFormatCommand(target, "SET %s %b", mykey, myval, myvallen);
 */
static int
redisFormatCommand (cmdcb_t cb, void *cbarg, char **target,
		    const char *format, ...)
{
  va_list ap;
  int len;
  va_start (ap, format);
  len = redisvFormatCommand (cb, cbarg, target, format, ap);
  va_end (ap);
  return len;
}
#endif

/* Format a command according to the Redis protocol. This function takes the
 * number of arguments, an array with arguments and an array with their
 * lengths. If the latter is set to NULL, strlen will be used to compute the
 * argument lengths.
 */
static int
redisFormatCommandArgv (cmdcb_t cb, void *cbarg, char **target, int argc,
			const char **argv, const size_t * argvlen)
{
  char *cmd = NULL;		/* final command */
  int pos;			/* position in final command */
  size_t len;
  int totlen, j;

  /* Calculate number of bytes needed for the command */
  totlen = 1 + intlen (argc) + 2;
  for (j = 0; j < argc; j++)
    {
      len = argvlen ? argvlen[j] : strlen (argv[j]);
      totlen += (int) bulklen (len);
    }

  /* Build the command at protocol level */
  cmd = malloc (totlen + 1);
  if (cmd == NULL)
    return -1;

  pos = sprintf (cmd, "*%d\r\n", argc);
  for (j = 0; j < argc; j++)
    {
      len = argvlen ? argvlen[j] : strlen (argv[j]);
#ifdef _WIN32
      pos += sprintf (cmd + pos, "$%llu\r\n", (unsigned long long) len);
#else
      pos += sprintf (cmd + pos, "$%zu\r\n", len);
#endif
      /* Invoke command call back */
      if (cb != NULL)
	{
	  cb (j, pos, argv[j], len, cbarg);
	}
      memcpy (cmd + pos, argv[j], len);
      pos += (int) len;
      cmd[pos++] = '\r';
      cmd[pos++] = '\n';
    }
  assert (pos == totlen);
  cmd[pos] = '\0';

  *target = cmd;
  return totlen;
}
