/*
 * linebuf.cc: Part of GNU CSSC.
 *
 *  Copyright (C) 1997, 1998, 2007, 2009, 2010, 2011, 2014 Free Software Foundation, Inc.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * CSSC was originally Based on MySC, by Ross Ridge, which was
 * placed in the Public Domain.
 *
 *
 * Members of the class cssc_linebuf.
 *
 */
#include "config.h"

#include <cstdio>
#include <cstring>
#include <climits>

#include "cssc.h"
#include "linebuf.h"
#include "cssc-assert.h"
#include "ioerr.h"


// Use a small chunk size for testing...
#define CONFIG_LINEBUF_CHUNK_SIZE (1024u)

cssc_linebuf::cssc_linebuf()
  : buf(new char[CONFIG_LINEBUF_CHUNK_SIZE]),
    buflen(CONFIG_LINEBUF_CHUNK_SIZE)
{
}


int
cssc_linebuf::read_line(FILE *f)
{
  buf[buflen - 2u] = '\0';

  ASSERT(buflen < INT_MAX);
  char *s = fgets(buf, static_cast<int>(buflen), f);
  while (s != NULL)
    {
      char c = buf[buflen - 2u];
      if (c == '\0' || c == '\n')
	return 0;

//
// Add another chunk
//

      char *temp_buf = new char[CONFIG_LINEBUF_CHUNK_SIZE + buflen];
      memcpy( temp_buf, buf, buflen);
      delete [] buf;
      buf = temp_buf;

      s = buf + buflen - 1u;
      buflen += CONFIG_LINEBUF_CHUNK_SIZE;
      buf[buflen - 2u] = '\0';

      s = fgets(s, CONFIG_LINEBUF_CHUNK_SIZE + 1u, f); // fill the new chunk
    }

  return 1;
}


int cssc_linebuf::write(FILE *f) const
{
  size_t len = strlen(buf);
  return fwrite_failed(fwrite(buf, sizeof(char), len, f), len);
}

int
cssc_linebuf::split(int offset, char **args, int len, char c)
{
  char *start = buf + offset;
  char *end = strchr(start, c);
  int i;

  for (i = 0; i < len; i++)
    {
      args[i] = start;
      if (0 == end)
	{
	  if (start[0] != '\0')
	    i++;
	  return i;		// no more delimiters.
	}
      *end++ = '\0';
      start = end;
      end = strchr(start, c);
    }

  return i;
}

void cssc_linebuf::
set_char(unsigned offset, char value)
{
  ASSERT(offset < buflen);
  buf[offset] = value;
}


/* Local variables: */
/* mode: c++ */
/* End: */
