/*
 * prs.c: Part of GNU CSSC.
 * 
 *    Copyright (C) 1997, Free Software Foundation, Inc. 
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
 * CSSC was originally Based on MySC, by Ross Ridge, which was 
 * placed in the Public Domain.
 *
 *
 * Prints selected parts of an SCCS file.
 *
 */

#include "cssc.h"
#include "fileiter.h"
#include "sccsfile.h"
#include "getopt.h"
#include "version.h"

const char main_rcs_id[] = "CSSC $Id: prs.cc,v 1.8 1997/11/18 23:22:28 james Exp $";

void
usage() {
	fprintf(stderr,
"usage: %s [-aelDRV] [-c cutoff] [-d format] [-r SID] file ...\n",
		prg_name);
}

int
main(int argc, char **argv) {
	int c;
	mystring format = ":Dt:\t:DL:\nMRs:\n:MR:COMMENTS:\n:C:";
	sid rid = NULL;
	/* enum */ sccs_file::when selected = sccs_file::SIDONLY;
	int all_deltas = 0;
	sccs_date cutoff_date;
	int default_processing = 1;

	if (argc > 0) {
		set_prg_name(argv[0]);
	} else {
		set_prg_name("prs");
	}

	class getopt opts(argc, argv, "d:Dr:Relc:aV");
	for(c = opts.next(); c != getopt::END_OF_ARGUMENTS; c = opts.next()) {
		switch (c) {
		default:
			quit(-2, "Unsupported option: '%c'", c);

		case 'd':
			format = opts.getarg();
			default_processing = 0;
			break;

		case 'D':
			default_processing = 0;
			break;

		case 'r':
			rid = sid(opts.getarg());
			if (!rid.valid() || rid.partial_sid()) {
				quit(-2, "Invaild SID: '%s'", opts.getarg());
			}
			default_processing = 0;
			break;

		case 'R':
			rid = NULL;
			default_processing = 0;
			break;
			
		case 'c':
			cutoff_date = sccs_date(opts.getarg());
			if (!cutoff_date.valid()) {
				quit(-2, "Invalid cutoff date: '%s'",
				     opts.getarg());
			}
			break;

		case 'e':
			selected = sccs_file::EARLIER;
			default_processing = 0;
			break;

		case 'l':
			selected = sccs_file::LATER;
			default_processing = 0;
			break;

		case 'a':
			all_deltas = 1;
			break;

		case 'V':
			version();
			break;
		}

	}

	if (selected == sccs_file::SIDONLY && cutoff_date.valid()) {
		quit(-2, "Either the -e or -l switch must used with a"
			 " cutoff date.");
	}

	if (default_processing) {
		selected = sccs_file::EARLIER;
	}

	sccs_file_iterator iter(argc, argv, opts.get_index());

	while(iter.next()) {
		sccs_name &name = iter.get_name();
		sccs_file file(name, sccs_file::READ);

		if (default_processing) {
			printf("%s:\n\n", name.c_str());
		}
		file.prs(stdout, format, rid, cutoff_date, selected,
			 all_deltas);
	}

	return 0;
}


// Explicit template instantiations.
template class range_list<sid>;
template class list<mystring>;
template class list<seq_no>;
template class list<sccs_file::delta>;
template class range_list<release>;


#include "stack.h"
template class stack<seq_no>;

/* Local variables: */
/* mode: c++ */
/* End: */
