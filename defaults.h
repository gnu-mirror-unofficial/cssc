/* 
 * defaults.h: Part of GNU CSSC.
 * 
 * 
 *    Copyright (C) 1997,1998 Free Software Foundation, Inc. 
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
 * Sets the default values of configuration macros left undefined. 
 *
 * @(#) CSSC defaults.h 1.1 93/11/09 17:17:46
 *
 */

#ifndef CSSC__DEFAULTS_H__
#define CSSC__DEFAULTS_H__

#ifndef LIDENT
#define LIDENT(ident) cssc_##ident
#endif

#ifndef NORETURN
#ifdef __GNUC__
#define NORETURN volatile void
#else
#define NORETURN void
#endif
#endif /* NORETURN */

#ifndef POSTDECL_NORETURN
#ifdef __GNUC__
// GNU C
#define POSTDECL_NORETURN __attribute__ ((noreturn))
#else
// Not GNU C
#define POSTDECL_NORETURN /* does not return */
#endif
#endif


#ifndef CDECL
#ifdef __BORLANDC__
#define CDECL __cdecl
#else
#define CDECL 
#endif
#endif /* CDECL */

#if !defined(START_RESULT_DECL) && !defined(END_RESULT_DECL)
#ifdef __GNUC__
#define START_RESULT_DECL(type, var)	return var {
#define END_RESULT_DECL(var)		return; }
#define RETURN_RESULT(var)		return
#else
#define START_RESULT_DECL(type, decl)	{ type decl;
#define END_RESULT_DECL(var)		return (var); }
#define RETURN_RESULT(var)		return (var)
#endif
#endif /* !defined(START_RESULT_DECL) && !defined(END_RESULT_DECL) */


#if !defined(CONFIG_NO_LOCKING) && !defined(CONFIG_SHARE_LOCKING) \
	&& !defined(CONFIG_PID_LOCKING) && !defined(CONFIG_DUMB_LOCKING)
#ifdef CONFIG_MSDOS_FILES
#ifdef CONFIG_DJGPP
#define CONFIG_DUMB_LOCKING
#else
#define CONFIG_SHARE_LOCKING
#endif
#else /* CONFIG_MSDOS_FILES */
#define CONFIG_PID_LOCKING
#endif /* CONFIG_MSDOS_FILES */
#endif /* !defined(CONFIG_NO_LOCKING) && ... */

#ifndef CONFIG_NULL_FILENAME
#ifdef CONFIG_MSDOS_FILES
#define CONFIG_NULL_FILENAME "NUL"
#else
#define CONFIG_NULL_FILENAME "/dev/null"
#endif
#endif /* CONFIG_NULL_FILENAME */

#ifndef CONFIG_EOL_CHARACTER
#define CONFIG_EOL_CHARACTER ('\n')
#endif

#ifndef CONFIG_CONTROL_CHARACTER
#define CONFIG_CONTROL_CHARACTER ('\001')
#endif

#ifndef CONFIG_WHAT_BUFFER_SIZE
#define CONFIG_WHAT_BUFFER_SIZE (16*1024)
#endif

#endif /* __DEFAULTS_H__ */

/* Local variables: */
/* mode: c++ */
/* End: */
