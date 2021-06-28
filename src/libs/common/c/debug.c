/***********************************************************************
 ** debug.c
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

/**
 * @file debug.c
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

#include <c/debug.h>
#include <time.h>
#include <signal.h>
#ifndef __UCLIBC__
#include <execinfo.h>
#include <syslog.h>
#endif

struct ST_DEBUG gs_debug = { NULL, f_debug_va, NULL };
struct ST_DEBUG * gps_debug = &gs_debug;

void f_debug_xterm_setcolor(FILE * fd, enum ET_DEBUG_XTERM_ATTR in_e_attr,
		enum ET_DEBUG_XTERM_COLOR in_e_fg, enum ET_DEBUG_XTERM_COLOR in_e_bg);
void f_debug_xterm_reset(FILE * fd);

void f_debug_register(struct ST_DEBUG * in_ps_debug) {
	struct ST_DEBUG * ps_debug = gps_debug;
	/* Insert in list */
	in_ps_debug->ps_next = ps_debug->ps_next;
	ps_debug->ps_next = in_ps_debug;
}

void f_debug_unregister(struct ST_DEBUG * in_ps_debug) {
	struct ST_DEBUG * ps_debug = gps_debug;
	printf("UNREGISTER %p\n", in_ps_debug);
	while (ps_debug) {
		/* If match remove from list */
		if (ps_debug->ps_next == in_ps_debug) {
			ps_debug->ps_next = ps_debug->ps_next->ps_next;
			in_ps_debug->ps_next = NULL;
		}

		ps_debug = ps_debug->ps_next;
	}
}

struct ST_DEBUG * f_debug_get(void) {
	return gps_debug;
}
/**
 * Main debug time
 *
 */
uint64_t f_debug_get_time(void) {
	uint64_t i_tmp;

#ifndef _WIN32
	struct timespec s_tv;
	/* Get time of acknoledge */
	if (clock_gettime(CLOCK_REALTIME, &s_tv) == -1) {
		M_ABORT();

	} else {
		i_tmp = (((uint64_t) s_tv.tv_nsec) / 1000000);
		i_tmp += (((uint64_t) s_tv.tv_sec)) * 1000;
	}
#else
	SYSTEMTIME s_sys_time;
	FILETIME s_file_time;
	ULARGE_INTEGER s_tmp;
	GetSystemTime(&s_sys_time);
	SystemTimeToFileTime(&s_sys_time, &s_file_time);
	s_tmp.LowPart = s_file_time.dwLowDateTime;
	s_tmp.HighPart = s_file_time.dwHighDateTime;
	i_tmp = (uint64_t)(s_tmp.QuadPart - 116444736000000000LL)/10000;
#endif
	return i_tmp;
}

/**
 * Main debug callback
 *
 */
int f_debug(const char * in_str_file, int in_i_line, const char * in_str_func,
		enum ET_DEBUG_TYPE in_e_level, const char *in_str_format, ...) {
	int ec = 0;
	struct ST_DEBUG * ps_debug = gps_debug;
	va_list ap;
	va_start(ap, in_str_format);
	time_t s_time;
	s_time = (uint64_t) f_debug_get_time() / 1000;

	/* Parse debug list */
	while (ps_debug) {

		va_list ap_local;
		__va_copy(ap_local, ap);
		ec = ps_debug->pf_cb_va(ps_debug->pv_arg, s_time, in_str_file,
				in_i_line, in_str_func, in_e_level, in_str_format, ap_local);
		ps_debug = ps_debug->ps_next;
		va_end(ap_local);
	}

	va_end(ap);
	return ec;
}

/**
 * Send attributes to xterm terminal.
 *
 * \param[in] attr common attribute
 * \param[in] fg foreground color
 * \param[in] bg background color
 */
void f_debug_xterm_setcolor(FILE * fd, enum ET_DEBUG_XTERM_ATTR in_e_attr,
		enum ET_DEBUG_XTERM_COLOR in_e_fg, enum ET_DEBUG_XTERM_COLOR in_e_bg) {
	/* Command is the control command to the terminal */
	if (in_e_bg) {
		fprintf(fd, "%c[%dm%c[%d;%dm", 0x1B, in_e_bg + 40, 0x1B, in_e_attr,
				in_e_fg + 30);
	} else {
		fprintf(fd, "%c[%d;%dm", 0x1B, in_e_attr, in_e_fg + 30);
	}
}
/**
 * Reset attributes to xterm terminal.
 */
void f_debug_xterm_reset(FILE * fd) {
	fprintf(fd, "%c[0m", 0x1B);
}

int f_debug_va(void * in_v_arg, time_t in_s_time, const char * in_str_file,
		int in_i_line, const char * in_str_func, enum ET_DEBUG_TYPE in_e_level,
		const char *in_str_format, va_list in_s_ap) {

	FILE * fd;
	char str_buffer[4096];
	int i_nb_char = 0;
	char * pc_type;

	switch (in_e_level) {
	case E_DEBUG_TYPE_FATAL:
		pc_type = "FATAL ERROR";
		fd = stderr;
		break;
	case E_DEBUG_TYPE_CRITICAL:
		pc_type = "CRITICAL ERROR";
		fd = stderr;
		break;
	case E_DEBUG_TYPE_WARNING:
		pc_type = "WARN";
		fd = stderr;
		break;
	case E_DEBUG_TYPE_IMP:
		pc_type = "IMP";
		fd = stdout;
		break;
	case E_DEBUG_TYPE_GOOD:
		pc_type = "GOOD";
		fd = stdout;
		break;
	case E_DEBUG_TYPE_INFO:
		pc_type = "INFO";
		fd = stdout;
		break;
	default:
		M_ABORT();
		break;
	}

	/* print time */
	{
		struct tm * ps_tm;
		ps_tm = localtime(&in_s_time);
		if (1 || ps_tm) {
			i_nb_char += strftime(&str_buffer[i_nb_char],
					sizeof(str_buffer) - i_nb_char, "[%H:%M:%S]", ps_tm);
		}
	}

	/* print file info */
  if (in_e_level != E_DEBUG_TYPE_INFO) {
  	i_nb_char += snprintf(&str_buffer[i_nb_char],
  			sizeof(str_buffer) - i_nb_char, "%s:%s:%d:", pc_type, in_str_func,
  			in_i_line);
  }

	/* print format with args */
	i_nb_char += vsnprintf(&str_buffer[i_nb_char],
			sizeof(str_buffer) - i_nb_char, in_str_format, in_s_ap);

#ifndef _WIN32
	/* Adding color prefix */
	switch (in_e_level) {
	case E_DEBUG_TYPE_FATAL:
		f_debug_xterm_setcolor(fd, E_DEBUG_XTERM_ATTR_BLINK,
				E_DEBUG_XTERM_COLOR_RED, E_DEBUG_XTERM_COLOR_BLACK);
		break;
	case E_DEBUG_TYPE_CRITICAL:
		f_debug_xterm_setcolor(fd, E_DEBUG_XTERM_ATTR_BRIGHT,
				E_DEBUG_XTERM_COLOR_RED, E_DEBUG_XTERM_COLOR_BLACK);
		break;
	case E_DEBUG_TYPE_WARNING:
		f_debug_xterm_setcolor(fd, E_DEBUG_XTERM_ATTR_BRIGHT,
				E_DEBUG_XTERM_COLOR_YELLOW, E_DEBUG_XTERM_COLOR_BLACK);
		break;
	case E_DEBUG_TYPE_IMP:
		f_debug_xterm_setcolor(fd, E_DEBUG_XTERM_ATTR_BRIGHT,
				E_DEBUG_XTERM_COLOR_BLUE, E_DEBUG_XTERM_COLOR_BLACK);
		break;
	case E_DEBUG_TYPE_GOOD:
		f_debug_xterm_setcolor(fd, E_DEBUG_XTERM_ATTR_BRIGHT,
				E_DEBUG_XTERM_COLOR_GREEN, E_DEBUG_XTERM_COLOR_BLACK);
		break;
	case E_DEBUG_TYPE_INFO:
		f_debug_xterm_setcolor(fd, E_DEBUG_XTERM_ATTR_RESET,
				E_DEBUG_XTERM_COLOR_WHITE, E_DEBUG_XTERM_COLOR_BLACK);
		break;
	}
#endif

  {
    int i_syslog_pri;
    char b_syslog = 0;
    switch (in_e_level) {
    case E_DEBUG_TYPE_FATAL:
      i_syslog_pri = LOG_ALERT;
      b_syslog = 1;
      break;
    case E_DEBUG_TYPE_CRITICAL:
      i_syslog_pri = LOG_CRIT;
      b_syslog = 1;
      break;
    case E_DEBUG_TYPE_WARNING:
      i_syslog_pri = LOG_WARNING;
      b_syslog = 1;
      break;
    case E_DEBUG_TYPE_IMP:
      i_syslog_pri = LOG_NOTICE;
      break;
    default:
      i_syslog_pri = LOG_INFO;
      break;
    }
    if(b_syslog) {
      syslog(i_syslog_pri, "[MASTER] %s", str_buffer);
    }
  }

	/* print new line */
	fprintf(fd, "%s\n", str_buffer);

#ifndef _WIN32
	/* Adding color suffix */
	f_debug_xterm_reset(fd);
#endif

	return 0;
}

void f_debug_print_backtrace(void) {
#ifndef __UCLIBC__
	int j, nptrs;
#define SIZE 100
	void *buffer[100];
	char **strings;

	nptrs = backtrace(buffer, SIZE);
	printf("- backtrace() returned %d addresses\n", nptrs);

	/* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
	 would produce similar output to the following: */

	strings = backtrace_symbols(buffer, nptrs);
	if (strings == NULL) {
		perror("backtrace_symbols");
		exit(EXIT_FAILURE);
	}

	printf("Back Trace:\n");
	for (j = 0; j < nptrs; j++)
		printf(" - %s\n", strings[j]);
	free(strings);
	printf("\n");
#endif

  CRIT("SEGFAULT");

}

void abort (void) {
	raise(SIGABRT);
	exit(-1);
}

void f_debug_assert_fail (const char *__assertion, const char *__file,
			   unsigned int __line, const char *__function) {
		f_debug (__file, __line, __PRETTY_FUNCTION__, E_DEBUG_TYPE_FATAL, "%s", __assertion);
	M_ABORT();
}
