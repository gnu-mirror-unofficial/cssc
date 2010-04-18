/*
 * sccs-delta.cc: Part of GNU CSSC.
 * 
 * 
 *    Copyright (C) 1997,2007 Free Software Foundation, Inc. 
 * 
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *    
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *    
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * CSSC was originally Based on MySC, by Ross Ridge, which was 
 * placed in the Public Domain.
 *
 *
 * Members of class delta.
 *
 */

#include "cssc.h"
#include "sccsfile.h"
#include "delta.h"

delta &
delta::operator =(delta const &it)
{
  inserted = it.inserted;
  deleted = it.deleted;
  unchanged = it.unchanged;
  type = it.type;
  id = it.id;
  date = it.date;
  user = it.user;
  seq = it.seq;
  prev_seq = it.prev_seq;
  
  included = it.included;
  excluded = it.excluded;
  ignored = it.ignored;
  have_includes = it.have_includes;
  have_excludes = it.have_excludes;
  have_ignores  = it.have_ignores;
  
  mrs = it.mrs;
  comments = it.comments;
  return *this;
}

bool delta::removed() const
{
  return 'R' == type;
}



// Explicit template instantiations.
template class mylist<delta>;
template class mylist<mystring>;


/* Local variables: */
/* mode: c++ */
/* End: */
