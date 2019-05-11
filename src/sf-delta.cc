/*
 * sf-delta.cc: Part of GNU CSSC.
 *
 *
 *  Copyright (C) 1997, 1998, 1999, 2001, 2002, 2007, 2008, 2009, 2010,
 *  2011, 2014, 2019 Free Software Foundation, Inc.
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
 * CSSC was originally based on MySC, by Ross Ridge, which was
 * placed in the Public Domain.
 *
 *
 * Members of sccs_file for adding a delta to the SCCS file.
 *
 */
#include <config.h>
#include <string>

#include <errno.h>
#include <unistd.h>

#include "cssc.h"
#include "sccsfile.h"
#include "pfile.h"
#include "seqstate.h"
#include "run.h"
#include "linebuf.h"
#include "delta.h"
#include "delta-table.h"
#include "delta-iterator.h"
#include "bodyio.h"
#include "filediff.h"
#include "file.h"

#undef JAY_DEBUG


class diff_state
{
public:
  enum state { START, NOCHANGE, DELETE, INSERT, END };

private:
  enum state _state;
  int in_lineno, out_lineno;
  int lines_left;
  int change_left;
  bool echo_diff_output;

  FILE *in;
  cssc_linebuf linebuf;

  NORETURN diff_output_corrupt() POSTDECL_NORETURN;
  NORETURN diff_output_corrupt(const char *msg) POSTDECL_NORETURN;

  void next_state();
  int read_line()
    {
      int read_ret = linebuf.read_line(in);

      if (echo_diff_output)
        {
          // If and only if we read in a new line, echo it.
          if (0 == read_ret)
            {
              printf("%s", linebuf.c_str());
            }
        }
      return read_ret;
    }

public:
  diff_state(FILE *f, bool echo)
    : _state(START),
      in_lineno(0), out_lineno(0),
      lines_left(0), change_left(0),
      echo_diff_output(echo),
      in(f)
    {
    }

  enum state process(FILE *out, seq_no seq);

  const char *
  get_insert_line()
    {
      ASSERT(_state == INSERT);
      ASSERT(linebuf[0] == '>' && linebuf[1] == ' ');
      return linebuf.c_str() + 2;
    }

  int in_line() { return in_lineno; }
  int out_line() { return out_lineno; }
};


/* Quit with an appropriate error message when a read operation
   on the diff output fails. */

NORETURN
diff_state::diff_output_corrupt() {
        if (ferror(in)) {
                fatal_quit(errno, "(diff output): Read error.");
        }
        fatal_quit(-1, "(diff output): Unexpected EOF.");
}


/* Quit with a cryptic error message indicating that something
   is wrong with the diff output. */

NORETURN
diff_state::diff_output_corrupt(const char *msg)
{
  fatal_quit(-1, "Diff output corrupt. (%s)", msg);
}


/* Figure out what the new state should be by processing the
   diff output. */

inline void
diff_state::next_state()
{
  if (_state == DELETE && change_left != 0)
    {
      if (read_line())
        {
          diff_output_corrupt();
        }
#ifdef JAY_DEBUG
      fprintf(stderr, "next_state(): read %s", linebuf.c_str());
#endif

      if (strcmp(linebuf.c_str(), "---\n") != 0)
        {
          diff_output_corrupt("expected ---");
        }
      lines_left = change_left;
      change_left = 0;
      _state = INSERT;
#ifdef JAY_DEBUG
      fprintf(stderr, "next_state(): returning INSERT [1]\n");
#endif
      return;
    }

  if (_state != NOCHANGE)
    {
      if (read_line())
        {
          if (ferror(in))
            {
              diff_output_corrupt();
            }
          _state = END;
#ifdef JAY_DEBUG
      fprintf(stderr, "next_state(): returning END [2]");
#endif
      return;
        }
#ifdef JAY_DEBUG
      fprintf(stderr, "next_state()[3]: read %s", linebuf.c_str());
#endif

      /* Ignore "\ No newline at end of file" if it appears
         at the end of the diff output. */

      if (linebuf[0] == '\\')
        {
          if (!read_line())
            {
              diff_output_corrupt("Expected EOF");
            }
          if (ferror(in))
            {
              diff_output_corrupt();
            }
          _state = END;
#ifdef JAY_DEBUG
      fprintf(stderr, "next_state(): returning END [4]\n");
#endif
          return;
        }

    }

  char *s = NULL;
  int line1, line2, line3, line4;
  char c;

  line1 = (int) strtol(linebuf.c_str(), &s, 10);
  line2 = line1;
  if (*s == ',')
    {
      line2 = (int) strtol(s + 1, &s, 10);
      if (line2 <= line1)
        {
          diff_output_corrupt("left end line");
        }
    }

  c = *s;

  ASSERT(c != '\0');

  if (c == 'a')
    {
      if (line1 >= in_lineno)
        {
          _state = NOCHANGE;
          lines_left = line1 - in_lineno + 1;
#ifdef JAY_DEBUG
      fprintf(stderr, "next_state(): returning NOCHANGE [5]\n");
#endif
          return;
        }
      if (line1 + 1 != in_lineno)
        {
          diff_output_corrupt("left start line [case 1]");
        }
    }
  else
    {
      if (line1 > in_lineno)
        {
          _state = NOCHANGE;
          lines_left = line1 - in_lineno;
#ifdef JAY_DEBUG
      fprintf(stderr, "next_state(): returning END\n");
#endif
          return;
        }
      if (line1 != in_lineno)
        {
          diff_output_corrupt("left start line [case 2]");
        }
    }

  line3 = (int) strtol(s + 1, &s, 10);
  if (c == 'd')
    {
      if (line3 != out_lineno)
        {
          diff_output_corrupt("right start line [case 1]");
        }
    }
  else
    {
      if (line3 != out_lineno + 1)
        {
          diff_output_corrupt("right start line [case 2]");
        }
    }

  line4 = line3;
  if (*s == ',')
    {
      line4 = (int) strtol(s + 1, &s, 10);
      if (line4 <= line3)
        {
          diff_output_corrupt("right end line");
        }
    }

  if (*s != '\n')
    {
      diff_output_corrupt("EOL");
    }

  switch (c)
    {
    case 'a':
      _state = INSERT;
      lines_left = line4 - line3 + 1;
#ifdef JAY_DEBUG
      fprintf(stderr, "next_state(): returning INSERT [6]\n");
#endif
      break;

    case 'd':
      _state = DELETE;
      lines_left = line2 - line1 + 1;
#ifdef JAY_DEBUG
      fprintf(stderr, "next_state(): returning DELETE [7]\n");
#endif
      break;

    case 'c':
      _state = DELETE;
      lines_left = line2 - line1 + 1;
      change_left = line4 - line3 + 1;
#ifdef JAY_DEBUG
      fprintf(stderr, "next_state(): returning DELETE [8]\n");
#endif
      break;

    default:
      diff_output_corrupt("unknown operation");
    }
}


/* Figure out whether a line is being inserted, deleted or left unchanged.
   Output new control lines accordingly. */

inline /* enum */ diff_state::state
diff_state::process(FILE *out, seq_no seq)
{
  if (_state != INSERT)
    {
      in_lineno++;
    }

  if (_state != END)
    {
      ASSERT(lines_left >= 0);
      if (lines_left == 0)
        {
          if (_state == DELETE || _state == INSERT)
            {
              fprintf(out, "\001E %d\n", seq);
            }
          next_state();
          if (_state == INSERT)
            {
              fprintf(out, "\001I %d\n", seq);
            }
          else if (_state == DELETE)
            {
              fprintf(out, "\001D %d\n", seq);
            }
        }
      lines_left--;
    }

  if (_state == DELETE)
    {
      if (read_line())
        {
          diff_output_corrupt();
        }
      if (linebuf[0] != '<' || linebuf[1] != ' ')
        {
          diff_output_corrupt("expected <");
        }
    }
  else
    {
      if (_state == INSERT)
        {
          if (read_line())
            {
              diff_output_corrupt();
            }
          if (linebuf[0] != '>' || linebuf[1] != ' ')
            {
              diff_output_corrupt("expected >");
            }
        }
      out_lineno++;
    }

  return _state;
}

class FileDeleter
{
  std::string name;
  bool as_real_user;
  bool armed;

public:
  FileDeleter(const std::string& s, bool realuser)
    : name(s), as_real_user(realuser), armed(true) { }

  ~FileDeleter()
    {
      if (armed)
        {
          const char *s = name.c_str();
          bool bOK;

          if (as_real_user)
            bOK = unlink_file_as_real_user(s);
          else
            bOK = (remove(s) == 0);
          if (!bOK)
            perror(s);
        }
    }
  void disarm() { armed = false; }
};



static void line_too_long(long int max_len, size_t actual_len)
{
  errormsg("Input line is %lu characters long but "
           "the maximum allowed SCCS file line length "
           "is %ld characters (see output of delta -V)\n",
           (unsigned long) actual_len,
           max_len);
}



/* Adds a new delta to the SCCS file.  It doesn't add the delta to the
   delta list in sccs_file object, so this should be the last operation
   performed before the object is destroyed. */

bool
sccs_file::add_delta(const std::string& gname,
		     sccs_pfile& pfile,
		     sccs_pfile::iterator it,
                     const std::vector<std::string>& new_mrs,
		     const std::vector<std::string>& new_comments,
                     bool display_diff_output)
{
  const char *pline;
  size_t len;
  const unsigned long int len_max = max_sfile_line_len();
  ASSERT(mode == UPDATE);

  if (!check_keywords_in_file(gname.c_str()))
    return false;

  if (!authorised())
     return false;

  /*
   * At this point, encode the contents of "gname", and pass
   * the name of this encoded file to diff, instead of the
   * name of the binary file itself.
   */
  std::string file_to_diff;
  bool bFileIsInWorkingDir;

  if (flags.encoded)
    {
      std::string uname(name.sub_file('u'));
      if (0 != encode_file(gname.c_str(), uname.c_str()))
        {
          return false;
        }
      file_to_diff = uname;
      bFileIsInWorkingDir = false;
      const char *s = file_to_diff.c_str();

#ifdef CONFIG_UIDS
      if (!is_readable(s))
        {
          errormsg("File %s is not readable by user %d!",
                   s, (int) geteuid());
          return false;
        }
#endif
    }
  else
    {
      file_to_diff = gname;
      bFileIsInWorkingDir = true;
    }

  /* When this function exits, delete the temporary file.
   */
  FileDeleter the_cleaner(file_to_diff, bFileIsInWorkingDir);
  if (!flags.encoded)
    the_cleaner.disarm();


  seq_state sstate(highest_delta_seqno());
  const std::string sid_name = it->got.as_string();
  const delta *got_delta = find_delta(it->got);
  if (got_delta == NULL)
    {
        errormsg("Locked delta %s doesn't exist!", sid_name.c_str());
        return false;
    }

  // Remember seq number that will be the predecessor of the
  // one for the delta.
  seq_no predecessor_seq = got_delta->seq();


  if (!prepare_seqstate(sstate, predecessor_seq,
                        it->include, it->exclude,
                        sccs_date()))
  {
      return false;
  }

  /* The d-file is created in the SCCS directory (XXX: correct?) */
  std::string dname(name.sub_file('d'));

  /* We used to use fcreate here but as shown by the tests in
   * tests/delta/errorcase.sh, the prior existence of the
   * d-file doesn't cause an error.
   *
   * XXX: slight departure from SCCS behaviour here.  The real thing
   * appears to issue an unlink(2) followed by a create(2) to create
   * the file.  If there is a setuid wrapper, but some ordinary user
   * has sufficient priveleges to create a symlink in the project
   * directory, it should be possible to exploit that race condition
   * to create a file of their choice with contents of their choice,
   * as the user to which the wrapper program is set-user or set-group
   * ID.  I believe that using the flag O_EXCL as fcreate() does resolves
   * that problem.
   */
  const int xmode = gfile_should_be_executable() ? CREATE_EXECUTABLE : 0;
  FILE *get_out = fcreate(dname, CREATE_EXCLUSIVE | xmode);
  if (NULL == get_out)
    {
      remove(dname.c_str());
      get_out = fcreate(dname, CREATE_EXCLUSIVE | xmode);
    }

  if (NULL == get_out)
    {
      errormsg_with_errno("Cannot create file %s", dname.c_str());
      return false;
    }
  FileDeleter another_cleaner(dname, false);

  const struct delta blankdelta;
  struct subst_parms parms(get_out, (const char*)NULL, blankdelta,
                           0, sccs_date());
  seq_state gsstate(sstate);

  get(dname, gsstate, parms, 0, 0, 0, 0, GET_NO_DECODE);

  if (fclose_failed(fclose(get_out)))
    {
      errormsg_with_errno("Failed to close temporary file");
      return false;
    }


  FileDiff differ(dname.c_str(), file_to_diff.c_str());
  FILE *diff_out = differ.start();


  class diff_state dstate(diff_out, display_diff_output);


  // The delta operation consists of:-
  // 1. Writing out the information for the new delta.
  // 2. Writing out any automatic null deltas.
  // 3. Copying the body of the s-file to the output file,
  //    modifying it as indicated by the output of the diff
  //    program.

  if (false == seek_to_body())  // prepare to read the body for predecessor.
    return false;


  // The new delta header includes info about what deltas
  // are included, excluded, ignored.   Compute that now.
  std::set<seq_no> included, excluded;
  for (seq_no iseq = 1; iseq < highest_delta_seqno(); iseq++)
    {
    if (sstate.is_explicitly_tagged(iseq))
      {
      if (sstate.is_included(iseq))
        {
	  included.insert(iseq);
        }
      else if (sstate.is_excluded(iseq))
        {
          excluded.insert(iseq);
        }
      }
    }

  // Create any required null deltas if we need to.
  if (flags.null_deltas)
    {
      // figure out how many null deltas to make to fill the gap
      // between the highest current trunk release and the one
      // belonging to the new delta.

      // use our own comment.
      std::vector<std::string> auto_comment;
      auto_comment.push_back(std::string("AUTO NULL DELTA"));

      release last_auto_rel = release(it->delta);
      // --last_auto_rel;

      sid id(got_delta->id());
      release null_rel = release(id);
      ++null_rel;

      ASSERT(id.valid());

      int infinite_loop_escape = 10000;

      while (null_rel < last_auto_rel)
        {
          ASSERT(id.valid());
          // add a new automatic "null" release.  Use the same
          // MRs as for the actual delta (is that right?) but
          seq_no new_seq = delta_table->next_seqno();

          // Set up for adding the next release.
          id = release(null_rel);
          id.next_level();

          delta null_delta('D', id, sccs_date::now(),
                           get_user_name(), new_seq, predecessor_seq,
                           new_mrs, auto_comment);
	  ASSERT (null_delta.inserted() == 0);
	  ASSERT (null_delta.deleted() == 0);
	  ASSERT (null_delta.unchanged() == 0);

          delta_table->prepend(null_delta);

          predecessor_seq = new_seq;

          ++null_rel;

          --infinite_loop_escape;
          ASSERT(infinite_loop_escape > 0);
        }
    }
  // assign a sequence number.
  seq_no new_seq = delta_table->next_seqno();

#if 1
  /* 2002-03-21: James Youngman: we already did this, above */

  // copy the list of excluded and included deltas from the p-file
  // into the delta.  it->include is a range_list<sid>,
  // but what we actually want to create is a list of seq_no.
  if (!it->include.empty())
    {
      const_delta_iterator iter(delta_table);
      while (iter.next())
        {
          if (it->include.member(iter->id()))
            {
              included.insert(iter->seq());
            }
        }
    }
  if (!it->exclude.empty())
    {
      const_delta_iterator iter(delta_table);
      while (iter.next())
        {
          if (it->exclude.member(iter->id()))
            {
              excluded.insert(iter->seq());
            }
        }
    }
#endif

  // Construct the delta information for the new delta.
  delta new_delta('D', it->delta, sccs_date::now(),
                  get_user_name(), new_seq, predecessor_seq,
                  included, excluded, new_mrs, new_comments);

  // We don't know how many lines will be added/changed yet.
  // end_update() fixes that.
  ASSERT (new_delta.inserted() == 0);
  ASSERT (new_delta.deleted() == 0);
  ASSERT (new_delta.unchanged() == 0);

  new_delta.id().print(stdout);
  printf("\n");

  // Begin the update by writing out the new delta.
  // This also writes out the information for all the
  // earlier deltas.
  FILE *out = start_update(new_delta);
  if (NULL == out)
    return false;

#undef DEBUG_FILE
#ifdef DEBUG_FILE
  FILE *df = fopen_as_real_user("delta.dbg", "w");
#endif

  // We have to continue while there is data on the input s-file,
  // or data fro the diff, so we don't just stop when read_line()
  // returns -1.
  while (1)
    {
      char c;
      // read line from the old body.
      const bool got_line = read_line(&c);

#ifdef JAY_DEBUG
      fprintf(stderr, "input: %s\n", plinebuf->c_str());
#endif
      if (got_line && c != 0)
        {
	  // it's a control line.
	    seq_no seq = strict_atous(here(), plinebuf->c_str() + 3);

#ifdef JAY_DEBUG
          fprintf(stderr, "control line: %c %lu\n", c, (unsigned)seq);
#endif

          if (seq < 1 || seq > highest_delta_seqno())
            {
		corrupt(here(), "Invalid sequence number %u", unsigned(seq));
            }

          const char *msg = NULL;

          switch (c)
            {
            case 'E':
              msg = sstate.end(seq);
              break;

            case 'D':
            case 'I':
              msg = sstate.start(seq, c);
              break;

            default:
	      corrupt(here(), "Unexpected control line '%c'", c);
              break;
            }

          if (msg != NULL)
            {
		corrupt(here(), "%s", msg);
            }
        }
      else if (sstate.include_line())
        {
#ifdef JAY_DEBUG
          fprintf(stderr, "body line, inserting\n");
#endif


          // We just read a body line and prev delta is in insert
          // mode.  We need to decide if this line must also go into
          // this version.  If not, we need to emit delete commands.
          // On the other hand, we may need to insert data before it.
          // But if we just want to insert it into this version too,
          // we still need to count it as an unchanged line.

          /* enum */ diff_state::state action;

          do
            {
              // decide what to do with this line.
              // process() also emits the neccesary command
              // (insert, delete, end).
              action = dstate.process(out, new_delta.seq());
              switch (action)
                {

                case diff_state::DELETE:
                  // signal that we want to delete that line,
                  // and break out of this inner loop (we do
                  // that by leaving the INSERT state).  The
                  // outer loop will deal with copying the line
                  // into the output, after we've emitted our
                  // delete marker; dstate.process() already
                  // did that.  We still need to copy the line
                  // into the output because even though the line
                  // is deleted in this delta, it still needs to
                  // be there for previous deltas.  That's what
                  // history files are for.

                  // Sanity check: if we're deleting a line, there
                  // must have been one on the input.
                  ASSERT(c != -1);
#ifdef JAY_DEBUG
                  fprintf(stderr, "diff_state::DELETE\n");
#endif
#ifdef DEBUG_FILE
                  fprintf(df, "%4d %4d - %s\n",
                          dstate.in_line(),
                          dstate.out_line(),
                          linebuf.c_str());
                  fflush(df);
#endif
		  new_delta.increment_deleted();
                  break;

                case diff_state::INSERT:
                  // We're inserting some data.   Emit that
                  // data.  When we've done that, we'll either
                  // go to the DELETE state (i.e. changed text)
                  // or the NOCHANGE state (simple insertion).
                  // Until then, copy data from the diff output
                  // into our own output.
#ifdef JAY_DEBUG
          fprintf(stderr, "diff_state::INSERT\n");
#endif
#ifdef DEBUG_FILE
                  fprintf(df, "%4d %4d + %s",
                          dstate.in_line(),
                          dstate.out_line(),
                          dstate.get_insert_line());
                  fflush(df);
#endif
                  new_delta.increment_inserted();

                  pline = dstate.get_insert_line();
                  len = strlen(pline);
                  if (len)
                    len -= 1u;  // newline char should not contribute.

                  if (0 == len_max || len < len_max)
                    {
                      if (fputs_failed(fputs(pline, out)))
                        {
                          return false;
                        }
                    }
                  else
                    {
                      // The line is too long.
                      line_too_long(len_max, len);
                      return false;
                    }
                  break;

                case diff_state::END:
#ifdef JAY_DEBUG
          fprintf(stderr, "diff_state::END\n");
#endif
                  if (c == -1)
                    {
                      break;
                    }
                  /* FALLTHROUGH */
                case diff_state::NOCHANGE:
                  // line unchanged - so there must have been an input line,
                  // so we cannot be at the end of the data.
                  ASSERT(c != -1);
#ifdef DEBUG_FILE
                  fprintf(df, "%4d %4d   %s\n",
                          dstate.in_line(),
                          dstate.out_line(),
                          linebuf.c_str());
                  fflush(df);
#endif
                  new_delta.increment_unchanged();
                  break;

                default:
                  abort();
                }
            } while (action == diff_state::INSERT);

#ifdef JAY_DEBUG
          fprintf(stderr, "while (action==diff_state::INSERT) loop ended.\n");
#endif
        }

      if (!got_line)
        {
          // If we've exhausted the input we may still have a block to
          // insert at the end.
          while (diff_state::INSERT == dstate.process(out, new_delta.seq()))
            {
              new_delta.increment_inserted();

              pline = dstate.get_insert_line();
              len = strlen(pline);
              if (len)
                len -= 1u;      // newline char should not contribute.

              if (0 == len_max
                  || len < len_max
                  )
                {
                  if (fputs_failed(fputs(pline, out)))
                    {
                      return false;
                    }
                }
              else
                {
                  // The line is too long.
                  line_too_long(len_max, len);
                  return false;
                }
            }

          break;
        }


#ifdef JAY_DEBUG
      fprintf(stderr, "-> %s\n", plinebuf->c_str());
#endif
      fputs(plinebuf->c_str(), out);
      putc('\n', out);
    }

#ifdef DEBUG_FILE
  fclose(df);
#endif


  // The order of things that we do at this point is quite
  // important; we want only to update the s- and p- files if
  // everything worked.
  differ.finish(diff_out); // "give back" the FILE pointer.
  ASSERT(0 == diff_out);

  // It would be nice to know if the diff failed, but actually
  // diff's return value indicates if there was a difference
  // or not.

  pfile.delete_lock(it);

  if (!end_update(&out, new_delta))
    return false;

  printf("%lu inserted\n%lu deleted\n%lu unchanged\n",
         new_delta.inserted(), new_delta.deleted(), new_delta.unchanged());

  if (pfile.update(true))
    return true;
  else
    return false;
}

/* Local variables: */
/* mode: c++ */
/* End: */
