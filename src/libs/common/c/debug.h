/***********************************************************************
 ** debug.h
 ***********************************************************************
 ** Copyright (c) SEAGNAL SAS
 **
 ** This library is free software; you can redistribute it and/or
 ** modify it under the terms of the GNU Lesser General Public
 ** License as published by the Free Software Foundation; either
 ** version 2.1 of the License, or (at your option) any later version.
 **
 ** This library is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 ** Lesser General Public License for more details.
 **
 ** You should have received a copy of the GNU Lesser General Public
 ** License along with this library; if not, write to the Free Software
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 ** MA 02110-1301 USA
 **
 ** -------------------------------------------------------------------
 **
 ** As a special exception to the GNU Lesser General Public License, you
 ** may link, statically or dynamically, a "work that uses this Library"
 ** with a publicly distributed version of this Library to produce an
 ** executable file containing portions of this Library, and distribute
 ** that executable file under terms of your choice, without any of the
 ** additional requirements listed in clause 6 of the GNU Lesser General
 ** Public License.  By "a publicly distributed version of this Library",
 ** we mean either the unmodified Library as distributed by the copyright
 ** holder, or a modified version of this Library that is distributed under
 ** the conditions defined in clause 3 of the GNU Lesser General Public
 ** License.  This exception does not however invalidate any other reasons
 ** why the executable file might be covered by the GNU Lesser General
 ** Public License.
 **
 ***********************************************************************/

/* define against mutual inclusion */
#ifndef DEBUG_H_
#define DEBUG_H_

/**
 * @file debug.h
 * Debug methods.
 *
 * @author SEAGNAL (johann.baudy@seagnal.fr)
 * @date 2012
 *
 * @version 1.0 Original version
 */

/***********************************************************************
 * Includes
 ***********************************************************************/
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

/***********************************************************************
 * Defines
 ***********************************************************************/



#define M_ABORT() abort()
#define DBG(in_str_format, ...) f_debug (__FILE__, __LINE__, __PRETTY_FUNCTION__, E_DEBUG_TYPE_INFO, in_str_format, ##__VA_ARGS__)
#define D(in_str_format,...) DBG (in_str_format, ##__VA_ARGS__)
//#define D(in_str_format, ...) f_debug (__FILE__, __LINE__, __FUNCTION__, E_DEBUG_TYPE_INFO, in_str_format, ##__VA_ARGS__);

#define CRIT(in_str_format, ...) f_debug (__FILE__, __LINE__, __PRETTY_FUNCTION__, E_DEBUG_TYPE_CRITICAL,  in_str_format, ##__VA_ARGS__)
#define WARN(in_str_format, ...) f_debug (__FILE__, __LINE__, __PRETTY_FUNCTION__, E_DEBUG_TYPE_WARNING,  in_str_format, ##__VA_ARGS__)
#define FATAL(in_str_format, ...) f_debug (__FILE__, __LINE__, __PRETTY_FUNCTION__, E_DEBUG_TYPE_FATAL, in_str_format, ##__VA_ARGS__); M_ABORT()
#define DBG_LINE() printf("%s -- %d\n", __PRETTY_FUNCTION__, __LINE__);  fflush(0)

enum ET_DEBUG_XTERM_ATTR {
	E_DEBUG_XTERM_ATTR_RESET = 0,
	E_DEBUG_XTERM_ATTR_BRIGHT,
	E_DEBUG_XTERM_ATTR_DIM,
	E_DEBUG_XTERM_ATTR_UNDERLINE,
	E_DEBUG_XTERM_ATTR_BLINK,
	E_DEBUG_XTERM_ATTR_REVERSE = 7,
	E_DEBUG_XTERM_ATTR_HIDDEN = 8
};

enum ET_DEBUG_XTERM_COLOR {
	E_DEBUG_XTERM_COLOR_BLACK = 0,
	E_DEBUG_XTERM_COLOR_RED,
	E_DEBUG_XTERM_COLOR_GREEN,
	E_DEBUG_XTERM_COLOR_YELLOW,
	E_DEBUG_XTERM_COLOR_BLUE,
	E_DEBUG_XTERM_COLOR_MAGENTA,
	E_DEBUG_XTERM_COLOR_CYAN,
	E_DEBUG_XTERM_COLOR_WHITE,
};

enum ET_DEBUG_TYPE {
	E_DEBUG_TYPE_FATAL = -4,
	E_DEBUG_TYPE_CRITICAL = -3,
	E_DEBUG_TYPE_WARNING = -2,
	E_DEBUG_TYPE_IMP = -1,
	E_DEBUG_TYPE_INFO = 0,
	E_DEBUG_TYPE_GOOD = 1,
};



/***********************************************************************
 * Types
 ***********************************************************************/
typedef int (*FT_DEBUG_CB_VA)(void * in_pv_arg, time_t in_s_time, const char * in_str_file, int in_i_line,
		const char * in_str_func, enum ET_DEBUG_TYPE in_e_level,
		const char * in_str_format, va_list in_s_ap);

struct ST_DEBUG {
	struct ST_DEBUG * ps_next;
	FT_DEBUG_CB_VA pf_cb_va;
	void * pv_arg;
};

/***********************************************************************
 * Functions
 ***********************************************************************/
#ifdef __cplusplus
extern "C" {
#define _Noreturn [[noreturn]]
#endif
void f_debug_register(struct ST_DEBUG * in_ps_debug);
void f_debug_unregister(struct ST_DEBUG * in_ps_debug);
struct ST_DEBUG * f_debug_get(void);
int f_debug(const char * in_str_file, int in_i_line, const char * in_str_func,
		enum ET_DEBUG_TYPE in_e_level, const char *in_str_format, ...);
int f_debug_va(void * in_pv_arg, time_t in_s_time, const char * in_str_file, int in_i_line,
		const char * in_str_func, enum ET_DEBUG_TYPE in_e_level,
		const char *in_str_format, va_list in_s_ap);
void f_debug_print_backtrace(void);
_Noreturn void f_debug_assert_fail (const char *__assertion, const char *__file,
               unsigned int __line, const char *__function);
#ifdef __cplusplus
}
;
#endif

#endif /* DEBUG_H_ */
