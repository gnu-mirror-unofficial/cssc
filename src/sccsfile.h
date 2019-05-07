/*
 * sccsfile.h: Part of GNU CSSC.
 *
 *  Copyright (C) 1997-2003, 2004, 2007, 2008, 2009, 2010, 2011, 2014 Free Software Foundation, Inc.
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
 * Definition of the class sccs_file.
 */

#ifndef CSSC__SCCSFILE_H__
#define CSSC__SCCSFILE_H__

#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include "sccsname.h"
#include "sid.h"
#include "sccsdate.h"
#include "rel_list.h"
#include "delta.h"
#include "pfile.h"

class seq_state;        /* seqstate.h */
class cssc_linebuf;
class FilePosSaver;             // filepos.h

struct delta;
class cssc_delta_table;
class delta_iterator;

class sccs_file
{
public:
  enum _mode { READ, UPDATE, CREATE, FIX_CHECKSUM };

private:

  sccs_name& name;
  FILE *f;
  bool checksum_valid;
  enum _mode mode;
  int lineno;
  long body_offset;
  int body_lineno;
  bool xfile_created;
  bool is_bk_file;
  bool sfile_executable;

  cssc_delta_table *delta_table;
  cssc_linebuf     *plinebuf;

  std::vector<std::string> users;	// FIXME: consider something more efficient.
  struct sccs_file_flags
  {
    // TODO: consider std::unique_ptr<std::string> instead of std::string*.
    std::string *type;
    std::string *mr_checker;
    int no_id_keywords_is_fatal;
    int branch;
    std::string *module;
    release floor;
    release ceiling;
    sid default_sid;
    int null_deltas;
    int joint_edit;
    release_list locked;
    int all_locked;
    std::string *user_def;
    std::string *reserved;

    int encoded;
    int executable;
    std::set<char> substitued_flag_letters; // "y" flag (Solaris 8 only)
  } flags;

  std::vector<std::string> comments;

  static FILE *open_sccs_file(const char *name, enum _mode mode,
                              int *sump, bool *is_bk_file);
  NORETURN corrupt(const char *fmt, ...) const POSTDECL_NORETURN;
  void check_arg() const;
  void check_noarg() const;
  unsigned short strict_atous(const char *s) const;
  unsigned long strict_atoul_idu(const char *s) const;

  int read_line_param(FILE *f);

  bool read_line(char* line_type);
  void read_delta();
  bool seek_to_body();

  /* Support for BitKeeper files */
  void check_bk_flag(char flagchar) const;
  void check_bk_comment(char ch, char arg) const;
  bool edit_mode_ok(bool editing) const;

  void set_sfile_executable(bool state);
  bool sfile_should_be_executable() const;

public:

  // sccs_file::sccs_file(sccs_name&, enum _mode) MUST
  // take a non-const reference to an sccs_name as an
  // argument in order to get the semantics of lock
  // ownership correct; an sccs_name carries with it a
  // lock, so if we copy it, either the copy does not
  // have a lock or we have too many locks in total.
  sccs_file(sccs_name &name, enum _mode mode);
  bool checksum_ok() const;

  sid find_most_recent_sid(sid id) const;
  bool find_most_recent_sid(sid& s, sccs_date& d) const;

  int is_delta_creator(const char *user, sid id) const;


  /* Forwarding functions for the delta table.
   */
  const delta *find_delta(sid id) const;
  const delta *find_any_delta(sid id) const;
  delta *find_delta(sid id);
  seq_no highest_delta_seqno() const;
  sid highest_delta_release() const;
  sid seq_to_sid(seq_no) const;


  /* Forwarding functinos for the line buffer.
   */
  char bufchar(int pos) const;

  ~sccs_file();

  /* sf-get.c */
private:
  struct subst_parms
  {
    FILE *out;
    const char *wstring;
    struct delta const &delta;
    unsigned out_lineno;
    sccs_date now;
    int found_id;

    subst_parms(FILE *o, const char *w, struct delta const &d,
                unsigned int l, sccs_date n)
      : out(o), wstring(w), delta(d), out_lineno(l), now(n),
        found_id(0) {}
  };

  typedef int (sccs_file::*subst_fn_t)(const char *,
                                       struct subst_parms *,
                                       struct delta const&) const;

  bool get(const std::string& name, class seq_state &state,
           struct subst_parms &parms,
           bool do_kw_subst,
           int show_sid = 0, int show_module = 0, int debug = 0,
           bool no_decode = false, bool for_edit = false);
  enum { GET_NO_DECODE = true };
  bool print_subsituted_flags_list(FILE *out, const char* separator) const;
  static bool is_known_keyword_char(char c);

  /* sf-get2.c */
  int write_subst(const char *start,
                  struct subst_parms *parms,
                  struct delta const& gotten_delta,
		  bool force_expansion) const;

  bool sid_matches(const sid& requested,
		   const sid& found,
		   bool get_top_delta) const;

public:
  struct get_status
  {
    unsigned lines;
    std::vector<sid> included, excluded;
    bool     success;
  };

  bool find_requested_sid(sid requested, sid &found,
                          bool include_branches=false) const ;
  bool find_requested_seqno(seq_no n, sid &found) const ;
  sid find_next_sid(sid requested, sid got, int branch,
                    const sccs_pfile &pfile, int *failed) const;
  bool test_locks(sid got, const sccs_pfile&) const;

  struct get_status get(FILE *out, const std::string& name,
			FILE *summary_file,
			sid id,
                        sccs_date cutoff_date = sccs_date(),
                        sid_list include = sid_list(""),
                        sid_list exclude = sid_list(""),
                        int keywords = 0, const char *wstring = NULL,
                        int show_sid = 0, int show_module = 0,
                        int debug = 0, bool for_edit = false);

private:

  void saw_unknown_feature(const char *fmt, ...) const;

  /* sf-get3.c */
  bool prepare_seqstate(seq_state &state, seq_no seq,
                        sid_list include,
                        sid_list exclude, sccs_date cutoff_date);
  bool prepare_seqstate_1(seq_state &state, seq_no seq);
  bool prepare_seqstate_2(seq_state &state, sid_list include,
                        sid_list exclude, sccs_date cutoff_date);
  bool authorised() const;

  /* sf-write.c */
private:
  void xfile_error(const char *msg) const;
  FILE *start_update();         // this opens the x-file
  int write_delta(FILE *out, struct delta const &delta) const;
  int write(FILE *out) const;
  bool end_update(FILE **out);  // NB: this closes the x-file too.
  int rehack_encoded_flag(FILE *out, int *sum) const;

public:
  bool update_checksum();
  bool update();

  /* sf-add.c */

  FILE *start_update(struct delta const &new_delta);
  bool end_update(FILE **out, struct delta const &new_delta);

  /* sf-delta.c */
private:
  bool check_keywords_in_file(const char *name);

public:
  int
  mr_required() const
  {
    if (flags.mr_checker)
      return 1;
    else
      return 0;
  }

  int check_mrs(const std::vector<std::string>& mrs);

  bool add_delta(const std::string& gname,
		 sccs_pfile &pfile,
		 sccs_pfile::iterator it,
                 const std::vector<std::string>& mrs, const std::vector<std::string>& comments,
                 bool display_diff_output);

  /* sccsfile.cc */
  void set_mr_checker_flag(const char *s);
  void set_module_flag(const char *s);
  void set_user_flag(const char *s);
  void set_reserved_flag(const char *s);
  void set_expanded_keyword_flag(const char *s);
  void set_type_flag(const char *s);
  bool gfile_should_be_executable() const;


  /* Used by get.cc (implemented in sccsfile.cc) */
  bool branches_allowed() const;

  /* val.cc */
  const std::string  get_module_type_flag();

  /* sf-admin.c */
  bool admin(const char *file_comment,
             bool force_binary,
             const std::vector<std::string>& set_flags, const std::vector<std::string>& unset_flags, // FIXME: consider something more efficient
             const std::vector<std::string>& add_users,
	     const std::unordered_set<std::string>& erase_users);
  bool create(const sid &initial_sid,
              const char *iname,
              const std::vector<std::string>& mrs,
              std::vector<std::string>* comments,
              int suppress_comments,
              bool force_binary);

  /* sf-prs.c */
private:
  bool get(FILE *out, const std::string& name, seq_no seq, bool for_edit);
  void print_flags(FILE *out) const;
  void print_delta(FILE *out, const char *format,
                   struct delta const &delta);

  /* sf-kw.cc */
  void no_id_keywords(const char name[]) const;

public:
  enum when { EARLIER, SIDONLY, LATER };
  struct cutoff
  {
    bool enabled;
    bool most_recent_sid_only;
    sid  cutoff_sid;
    const struct delta *cutoff_delta;
    sccs_date first_accepted;
    sccs_date last_accepted;

    cutoff();
    bool excludes_delta(sid, sccs_date, bool& stop_now) const;
    void print(FILE *out) const;
  };


  bool prs(FILE *out, const std::string& format, sid rid, sccs_date cutoff_date,
           enum when when, bool all_deltas, bool *matched);

  bool prt(FILE *out, struct cutoff exclude, int all_deltas,
           //
           int print_body, int print_delta_table, int print_flags,
           int incl_excl_ignore, int first_line_only, int print_desc,
           int print_users) const;

  std::string get_module_name() const;

  /* sf-cdc.c */

  bool cdc(sid id, const std::vector<std::string>& mrs, const std::vector<std::string>& comments);

  /* sf-rmdel.c */

  bool rmdel(sid rid);

  /* sf-val.cc */
  bool validate() const;
  bool validate_seq_lists(const delta_iterator& d) const;
  bool validate_isomorphism() const;

  // Implementation is protected; in the existing [MySC]
  // implementation some of the implementation is private where
  // it might better be protected.
protected:
  bool sid_in_use(sid id, const sccs_pfile& p) const;

private:
  // Because we now have a pointer member, don't use the compiler's
  // default assignment and constructor.
  const sccs_file& operator=(const sccs_file&); // not allowed to use!
  sccs_file(const sccs_file&);  // not allowed to use!
};

/* sf-prt.cc */
void print_flag(FILE *out, const char *fmt,  release flag, int& count);

/* sf-prs.cc */
void print_flag2(FILE *out, const char *s, const sid& it);
void print_flag2(FILE *out, const char *s, const release_list& it);

/* l-split.c */

std::vector<std::string> split_mrs(const std::string& mrs);
std::vector<std::string> split_comments(const std::string& comments);

#endif /* __SCCSFILE_H__ */

/* Local variables: */
/* mode: c++ */
/* End: */
