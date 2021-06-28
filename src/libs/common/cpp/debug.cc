/***********************************************************************
 ** debug
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
 * @file debug.cc
 * Debug C++.
 *
 * @author SEAGNAL (johann.baudy@seagnal.fr)
 * @date 2013
 *
 * @version 1.0 Original version
 */

/***********************************************************************
 * Includes
 ***********************************************************************/
#include <boost/stacktrace.hpp>
#include "debug.hh"
#include <cxxabi.h>
#include <dlfcn.h>
#include <cpp/string.hh>


#ifndef FF_NO_BT
#include <execinfo.h>
#endif
#include <signal.h>
#include <list>

#ifndef FF_NO_BT
#define LF_BFD
#ifdef LF_BFD
#include <bfd.h>
#endif
#endif

void posix_death_signal(int signum) {
	switch (signum) {
	case SIGSEGV:
		printf("SIGNAL: SIGSEGV\n");
		break;
	case SIGABRT:
		printf("SIGNAL: SIGABRT\n");
		break;
	default:
		printf("SIGNAL: %d\n", signum);
		break;
	}
	{
		std::cout << "BOOST BACKTRACE"<<std::endl;
		std::cout << boost::stacktrace::stacktrace();
	}
	{
		std::cout << "-"<<std::endl;
		std::cout << "MASTER BACKTRACE"<<std::endl;
		CT_DEBUG::f_print_backtrace_cxx();
	}
	signal(signum, SIG_IGN);
	//signal(SIGABRT, SIG_DFL);
	//abort();
	throw std::runtime_error("POSIX DEATH SIGNAL");
}

void CT_DEBUG::f_signal_init(void) {
	signal(SIGSEGV, posix_death_signal);
	signal(SIGABRT, posix_death_signal);
}

void CT_DEBUG::f_get_backtrace(ST_DEBUG_BACKTRACE & out_s_bt) {

#ifndef FF_NO_BT
//#if 0
	out_s_bt.sz_buffer = backtrace(out_s_bt.av_buffer, C_DEBUG_BACKTRACE_SIZE);
#else
	out_s_bt.sz_buffer = 0;
#endif
}

size_t CT_DEBUG::f_get_backtrace(void ** out_pv_bt, size_t in_sz_bt) {

#ifndef FF_NO_BT
	return backtrace(out_pv_bt, in_sz_bt);
#else
	return 0;
#endif
}

void CT_DEBUG::f_print_backtrace_cxx(void) {
	ST_DEBUG_BACKTRACE s_bt;

	f_get_backtrace(s_bt);
	printf("- backtrace() returned %d addresses\n", s_bt.sz_buffer);

	/* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
	 would produce similar output to the following: */

	f_print_backtrace_cxx(s_bt);
}

void CT_DEBUG::f_print_backtrace_cxx_simple(void) {
	ST_DEBUG_BACKTRACE s_bt;

	f_get_backtrace(s_bt);
	printf("- backtrace() returned %d addresses\n", s_bt.sz_buffer);

	/* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
	 would produce similar output to the following: */

	f_print_backtrace_cxx_simple(s_bt);
}

std::string CT_DEBUG::f_demangle_cxx(const char* in_str_name) {

	int status = -4;

	char* res = abi::__cxa_demangle(in_str_name, NULL, NULL, &status);

	const char* const demangled_name = (status == 0) ? res : in_str_name;

	std::string ret_val(demangled_name);

	free(res);

	return ret_val;
}

void CT_DEBUG::f_print_backtrace_cxx_simple(ST_DEBUG_BACKTRACE & in_s_bt) {
#ifndef FF_NO_BT
	int nptrs = in_s_bt.sz_buffer;

	char **strings;
	strings = backtrace_symbols(in_s_bt.av_buffer, nptrs);
	if (strings == NULL) {
		perror("backtrace_symbols");
		exit(EXIT_FAILURE);
	}
#if 0
	printf("Back Trace:\n");
	for (int j = 0; j < nptrs; j++)
	printf(" #%2d %s\n", j, strings[j]);
#endif

	printf("Call stack:\n");

	for (int i = 0; i < nptrs; ++i) {
		Dl_info dlinfo;
		const char *symname;
		char *demangled;
		int status;

		if (!dladdr(in_s_bt.av_buffer[i], &dlinfo))
			continue;

		symname = dlinfo.dli_sname;

		demangled = abi::__cxa_demangle(symname, NULL, 0, &status);
		if (status == 0 && demangled)
			symname = demangled;

		printf(" - %s, %s\n", dlinfo.dli_fname, symname);

		if (demangled)
			free(demangled);
	}
	printf("\n");

	free(strings);
#else
	printf("BT not supported\n");
#endif
}

int CT_DEBUG::f_vprintf(int in_i_prio, const char * in_str_msg, va_list ap) {
	int n;
	char str_tmp[4096];
	int i_syslog_pri;
	n = vsnprintf(str_tmp, sizeof(str_tmp), in_str_msg, ap);

  bool b_syslog = false;
  switch (in_i_prio) {
  case E_DEBUG_TYPE_FATAL:
    i_syslog_pri = LOG_ALERT;
    b_syslog = true;
    break;
  case E_DEBUG_TYPE_CRITICAL:
    i_syslog_pri = LOG_CRIT;
    b_syslog = true;
    break;
  case E_DEBUG_TYPE_WARNING:
    i_syslog_pri = LOG_WARNING;
    b_syslog = true;
    break;
  case E_DEBUG_TYPE_IMP:
    i_syslog_pri = LOG_NOTICE;
    break;
  default:
    i_syslog_pri = LOG_INFO;
    break;
  }
  if(b_syslog) {
	   syslog(i_syslog_pri, "[MASTER] %s", str_tmp);
  }
	std::cout << str_tmp << "\n";
	return n;
}

int CT_DEBUG::f_printf(int in_i_prio, const char * in_str_msg, ...) {
	int n;
	va_list ap;
	va_start(ap, in_str_msg);
	n = f_vprintf(in_i_prio, in_str_msg, ap);
	va_end(ap);
	return n;

}

const char * CT_DEBUG::f_print_backtrace_cxx_single_symname(void * in_pv_address, bool in_b_full) {
	Dl_info dlinfo;
	const char *symname;
	const char *out_string = NULL;
	char *demangled;
	int status;

	if (!dladdr(in_pv_address, &dlinfo)) {
		return  "NULL";
	}

	symname = dlinfo.dli_sname;

	demangled = abi::__cxa_demangle(symname, NULL, 0, &status);
	if (status == 0 && demangled)
		symname = demangled;
#ifndef LF_BFD
	printf(" - %s, %s\n", dlinfo.dli_fname, symname);
#else
	//printf("\n\nraw - %s, %s, %s\n", dlinfo.dli_fname, symname, dlinfo.dli_sname);
	uintptr_t offset = (uintptr_t) in_pv_address - (uintptr_t) dlinfo.dli_fbase;
	bool b_default = true;
	//printf(" - %p, %s\n", dlinfo.dli_fname, symname);
	{
		bfd* abfd = 0;
		asymbol **syms = 0;
		asection *text = 0;
		abfd = bfd_openr(dlinfo.dli_fname, 0);
		if (abfd) {

			/* oddly, this is required for it to work... */
			bfd_check_format(abfd, bfd_object);

			unsigned storage_needed = bfd_get_symtab_upper_bound(abfd);
			syms = (asymbol **) malloc(storage_needed);
			bfd_canonicalize_symtab(abfd, syms);

			text = bfd_get_section_by_name(abfd, ".text");

			//printf("b:%p s:%p fb:%p vma:%p offset:%p %u\n",buffer[i], dlinfo.dli_saddr, dlinfo.dli_fbase, (char*)text->vma, (char*)offset, cSymbols);
			long offset_text = ((long) offset) - text->vma;
			if (offset_text <= 0) {
				offset_text = (uintptr_t) in_pv_address - text->vma;
			}
			const char *file = NULL;
			const char *func = NULL;
			unsigned line = 0;
			//printf("%li\n",offset_text);
			if (bfd_find_nearest_line(abfd, text, syms, offset_text, &file,
					&func, &line)
					&& file) {
				if(in_b_full) {
				out_string = symname;
				} else {
					out_string = symname;
				}

				b_default = false;
			}
			//} else {
			//	printf("- %s\n", symname);
			//}

			bfd_close(abfd);
			free(syms);
		}
	}

	if (b_default) {
		out_string = symname;
	}
#endif
	if (demangled)
		free(demangled);

	return out_string;
}

std::string CT_DEBUG::f_print_backtrace_cxx_single(void * in_pv_address, bool in_b_full) {
	std::string out_string;
	Dl_info dlinfo;
	const char *symname;
	char *demangled;
	int status;

	if (!dladdr(in_pv_address, &dlinfo)) {
		return  out_string;
	}

	symname = dlinfo.dli_sname;

	demangled = abi::__cxa_demangle(symname, NULL, 0, &status);
	if (status == 0 && demangled)
		symname = demangled;
#ifndef LF_BFD
	printf(" - %s, %s\n", dlinfo.dli_fname, symname);
#else
	//printf("\n\nraw - %s, %s, %s\n", dlinfo.dli_fname, symname, dlinfo.dli_sname);
	uintptr_t offset = (uintptr_t) in_pv_address - (uintptr_t) dlinfo.dli_fbase;
	bool b_default = true;
	//printf(" - %p, %s\n", dlinfo.dli_fname, symname);
	{
		bfd* abfd = 0;
		asymbol **syms = 0;
		asection *text = 0;
		abfd = bfd_openr(dlinfo.dli_fname, 0);
		if (abfd) {

			/* oddly, this is required for it to work... */
			bfd_check_format(abfd, bfd_object);

			unsigned storage_needed = bfd_get_symtab_upper_bound(abfd);
			syms = (asymbol **) malloc(storage_needed);
			bfd_canonicalize_symtab(abfd, syms);

			text = bfd_get_section_by_name(abfd, ".text");

			//printf("b:%p s:%p fb:%p vma:%p offset:%p %u\n",buffer[i], dlinfo.dli_saddr, dlinfo.dli_fbase, (char*)text->vma, (char*)offset, cSymbols);
			long offset_text = ((long) offset) - text->vma;
			if (offset_text <= 0) {
				offset_text = (uintptr_t) in_pv_address - text->vma;
			}
			const char *file = NULL;
			const char *func = NULL;
			unsigned line = 0;
			//printf("%li\n",offset_text);
			if (bfd_find_nearest_line(abfd, text, syms, offset_text, &file,
					&func, &line)
					&& file) {
				if(in_b_full) {
				out_string = f_string_format("%s at %s:%u", symname, file,
						line);
				} else {
					out_string = f_string_format("%s:%u", symname,line);
				}

				b_default = false;
			}
			//} else {
			//	printf("- %s\n", symname);
			//}

			bfd_close(abfd);
			free(syms);
		}
	}

	if (b_default) {
		out_string = f_string_format("%s %s", symname, dlinfo.dli_fname);
	}
#endif
	if (demangled)
		free(demangled);

	return out_string;
}

void CT_DEBUG::f_print_backtrace_cxx(ST_DEBUG_BACKTRACE & in_s_bt) {
	f_print_backtrace_cxx(in_s_bt.av_buffer, in_s_bt.sz_buffer);
}

void CT_DEBUG::f_print_backtrace_cxx(void * const * in_pv_address,size_t in_sz_address, bool in_b_full) {
#ifndef FF_NO_BT
	int nptrs = in_sz_address;

	char **strings;
	strings = backtrace_symbols(in_pv_address, nptrs);
	if (strings == NULL) {
		perror("backtrace_symbols");
		exit(EXIT_FAILURE);
	}

#if 0
	printf("Back Trace:\n");
	for (int j = 0; j < nptrs; j++)
	printf(" #%2d %s\n", j, strings[j]);

#endif
	f_printf(E_DEBUG_TYPE_FATAL, "Call stack:");

	for (int i = 0; i < nptrs; ++i) {
		std::string str_tmp = f_print_backtrace_cxx_single(
				in_pv_address[i]);

		f_printf(E_DEBUG_TYPE_FATAL, " #%2d %s", i, str_tmp.c_str());
	}

	free(strings);
#else
	printf("BT not supported\n");
#endif
}

static std::list<void const *> gl_dbg;

void f_guard_dbg_push(void const * in_ptr) {
	_WARN<< "DBG GUARD ADD " << in_ptr;
	gl_dbg.push_back(in_ptr);
}

void f_guard_dbg_remove(void const * in_ptr) {
	gl_dbg.remove(in_ptr);
}

bool f_guard_dbg_in(void const * in_ptr) {
	for (std::list<void const *>::iterator pc_it = gl_dbg.begin();
			pc_it != gl_dbg.end(); pc_it++) {
		if (*pc_it == in_ptr) {
			return true;
		}
	}
	return false;
}
std::ostream& operator<<(std::ostream& os, const float3_t& dt)
{
    os << dt.x << ',' << dt.y << ',' << dt.z;
    return os;
}
std::ostream& operator<<(std::ostream& os, const double3_t& dt)
{
    os << dt.x << ',' << dt.y << ',' << dt.z;
    return os;
}
