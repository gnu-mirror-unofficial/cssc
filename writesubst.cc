/*
 * writesubst.cc: Part of GNU CSSC.
 * 
 * 
 *    Copyright (C) 2001, Free Software Foundation, Inc. 
 * 
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 * 
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 * 
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 *
 * sccsfile::write_subst()
 *
 */

#include "cssc.h"
#include "sccsfile.h"
#include "delta.h"

// #include "pfile.h"
// #include "seqstate.h"
// #include "delta-iterator.h"
// #include "delta-table.h"


#include <ctype.h>

#ifdef CONFIG_SCCS_IDS
static const char rcs_id[] = "CSSC $Id: writesubst.cc,v 1.1 2001/07/31 08:28:07 james_youngman Exp $";
#endif

/* Write a line of a file after substituting any id keywords in it.
   Returns true if an error occurs. */

int
sccs_file::write_subst(const char *start,
                       struct subst_parms *parms,
                       const delta& d) const
{
  FILE *out = parms->out;

  const char *percent = strchr(start, '%');
  while (percent != NULL)
    {
      char c = percent[1];
      if (c != '\0' && percent[2] == '%')
        {
          if (start != percent
              && fwrite(start, percent - start, 1, out) != 1)
            {
              return 1;
            }

          percent += 3;

          int err = 0;

          switch (c)
            {
              const char *s;

            case 'M':
              {
                const char *mod = get_module_name().c_str();
                err = fputs_failed(fputs(mod, out));
              }
            break;
                        
            case 'I':
              err = d.id.print(out);
              break;

            case 'R':
              err = d.id.printf(out, 'R', 1);
              break;

            case 'L':
              err = d.id.printf(out, 'L', 1);
              break;

            case 'B':
              err = d.id.printf(out, 'B', 1);
              break;

            case 'S':
              err = d.id.printf(out, 'S', 1);
              break;

            case 'D':
              err = parms->now.printf(out, 'D');
              break;
                                
            case 'H':
              err = parms->now.printf(out, 'H');
              break;

            case 'T':
              err = parms->now.printf(out, 'T');
              break;

            case 'E':
              err = d.date.printf(out, 'D');
              break;

            case 'G':
              err = d.date.printf(out, 'H');
              break;

            case 'U':
              err = d.date.printf(out, 'T');
              break;

            case 'Y':
              if (flags.type)
                {
                  err = fputs_failed(fputs(flags.type->c_str(), out));
                }
              break;

            case 'F':
              err =
                fputs_failed(fputs(base_part(name.sfile()).c_str(),
                                   out));
              break;

            case 'P':
              if (1) // introduce new scope...
                {
                  mystring path(canonify_filename(name.c_str()));
                  err = fputs_failed(fputs(path.c_str(), out));
                }
              break;

            case 'Q':
              if (flags.user_def)
                {
                  err = fputs_failed(fputs(flags.user_def->c_str(), out));
                }
              break;

            case 'C':
              err = printf_failed(fprintf(out, "%d",
                                          parms->out_lineno));
              break;

            case 'Z':
              if (fputc_failed(fputc('@', out))
                  || fputs_failed(fputs("(#)", out)))
                {
                  err = 1;
                }
              else
                {
                  err = 0;
                }
              break;

            case 'W':
              s = parms->wstring;
              if (0 == s)
                {
                  /* At some point I had been told that SunOS 4.1.4
                   * apparently uses a space rather than a tab here.
                   * However, a test on 4.1.4 shows otherwise.
                   *
                   * From: "Carl D. Speare" <carlds@attglobal.net>
                   * Subject: RE: SunOS 4.1.4 
                   * To: 'James Youngman' <jay@gnu.org>,
                   *         "cssc-users@gnu.org" <cssc-users@gnu.org>
                   * Date: Wed, 11 Jul 2001 01:07:36 -0400
                   * 
                   * Ok, here's what I got:
                   * 
                   * %W% in a file called test.c expanded to:
                   * 
                   * @(#)test.c<TAB>1.1
                   * 
                   * Sorry, but my SunOS machine is lacking a network
                   * connection, so I can't bring it over into
                   * mail-land. But, there you are, for what it's
                   * worth.
                   * 
                   * --Carl
                   * 
                   */
                  s = "%Z" "%%M" "%\t%" "I%";
                  /* NB: strange foroatting of the string above is 
                   * to preserve it unchanged even if this source code does 
                   * itself get checked into SCCS or CSSC.
                   */
                }
              else
                {
                  /* protect against recursion */
                  parms->wstring = 0; 
                }
              err = write_subst(s, parms, d);
              if (0 == parms->wstring)
                {
                  parms->wstring = s;
                }
              break;

            case 'A':
              err = write_subst("%Z""%%Y""% %M""% %I"
                                "%%Z""%",
                                parms, d);
              break;

            default:
              start = percent - 3;
              percent = percent - 1;
              continue;
            }

          parms->found_id = 1;

          if (err)
            {
              return 1;
            }
          start = percent;
        }
      else
        {
          percent++;
        }
      percent = strchr(percent, '%');
    }

  return fputs_failed(fputs(start, out));
}