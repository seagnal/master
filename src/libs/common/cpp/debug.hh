/***********************************************************************
 ** debug.hh
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

#ifndef DEBUG_HH_
#define DEBUG_HH_

/**
 * @file debug.hh
 * Debug ostream definition.
 *
 * @author SEAGNAL (johann.baudy@seagnal.fr)
 * @date 2013
 *
 * @version 1.0 Original version
 */

/***********************************************************************
 * Includes
 ***********************************************************************/

#include <c/debug.h>
#include <c/common.h>
#include <string>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <syslog.h>
/***********************************************************************
 * Defines
 ***********************************************************************/
#define _V(x) (" " M_STR(x) ":") << (x)
#define _CRIT CT_DEBUG::in(__FILE__, __LINE__, __PRETTY_FUNCTION__, E_DEBUG_TYPE_CRITICAL)
#define _DBG CT_DEBUG::in(__FILE__, __LINE__, __PRETTY_FUNCTION__, E_DEBUG_TYPE_INFO)
#define _WARN CT_DEBUG::in(__FILE__, __LINE__, __PRETTY_FUNCTION__, E_DEBUG_TYPE_WARNING)
#define _FATAL CT_DEBUG::in(__FILE__, __LINE__, __PRETTY_FUNCTION__, E_DEBUG_TYPE_FATAL)
#define _RED ("\x1B[1;31m")
#define _GREEN ("\x1B[1;32m")
#define _YELLOW ("\x1B[1;33m")
#define _BLUE ("\x1B[1;34m")
#define _MAGNENTA ("\x1B[1;35m")
#define _CYAN ("\x1B[1;36m")
#define _WHITE ("\x1B[1;37m")
#define _RESET ("\x1B[0m")
/***********************************************************************
 * Types
 ***********************************************************************/
#define C_DEBUG_BACKTRACE_SIZE 100
struct ST_DEBUG_BACKTRACE {
	void * av_buffer[C_DEBUG_BACKTRACE_SIZE];
	int sz_buffer;
};

class CT_DEBUG: public std::exception {
	std::string _str_file;
	uint _i_line;
	std::string _str_function;
	enum ET_DEBUG_TYPE _e_type;
	bool _b_print;
	std::ostringstream _s;
	std::string _str_tmp;
public:
	CT_DEBUG(char const * in_str_file, uint in_i_line,
			char const * in_str_function, enum ET_DEBUG_TYPE in_e_type) {
		_str_file = in_str_file;
		_i_line = in_i_line;
		_str_function = in_str_function;
		_e_type = in_e_type;
		_b_print = true;
	}

	CT_DEBUG(const CT_DEBUG& in_other) {
		_str_file = in_other._str_file;
		_i_line = in_other._i_line;
		_str_function = in_other._str_function;
		_e_type = in_other._e_type;
		_b_print = false;
		_s << in_other._s.str();
	}

	~CT_DEBUG() throw () {
		if (_b_print) {
			f_debug(_str_file.c_str(), _i_line, _str_function.c_str(), _e_type,
					"%s", _s.str().c_str());
			if (_e_type == E_DEBUG_TYPE_FATAL) {
				abort();
			}
		}
	}

	template<class T>
	inline CT_DEBUG &operator<<(const T &in) {
		_s << in;
		return *this;
	}

	static CT_DEBUG in(char const * in_str_file, uint in_i_line,
			char const * in_str_function, enum ET_DEBUG_TYPE in_e_type) {
		CT_DEBUG c_tmp(in_str_file, in_i_line, in_str_function, in_e_type);
		return c_tmp;
	}

	const char* what() const throw () {
		CT_DEBUG* self = const_cast<CT_DEBUG*>(this);
		self->_str_tmp = _s.str();
		//_DBG << _s.str() << " "<< this << " " << std::hex << (uintptr_t) _str_tmp.c_str();
		const char* pc_tmp = _str_tmp.c_str();
		return pc_tmp;
	}

#if 0
	operator const {
		return std::runtime_error(_s.str());
	}
#endif
	static void f_print_backtrace_cxx(void);
	static void f_print_backtrace_cxx_simple(void);
	static void f_signal_init(void);

	static size_t f_get_backtrace(void ** out_pv_bt, size_t in_sz_bt);
	static void f_get_backtrace(ST_DEBUG_BACKTRACE & out_s_bt);
	static void f_print_backtrace_cxx_simple(ST_DEBUG_BACKTRACE & in_s_bt);
	static void f_print_backtrace_cxx(ST_DEBUG_BACKTRACE & in_s_bt);
	static void f_print_backtrace_cxx(void * const * in_pv_address,size_t in_sz_address, bool in_b_full=true);
	static std::string f_print_backtrace_cxx_single(void * in_pv_address, bool in_b_full = true);
	static const char * f_print_backtrace_cxx_single_symname(void * in_pv_address, bool in_b_full = true);
	static std::string f_demangle_cxx(const char* in_str_name);


	static int f_vprintf(int in_i_prio, const char * in_str_msg, va_list ap);
	static int f_printf(int in_i_prio, const char * in_str_msg, ...);
};


void f_guard_dbg_push(void const * in_ptr);
void f_guard_dbg_remove(void const * in_ptr);
bool f_guard_dbg_in(void const * in_ptr);
std::ostream& operator<<(std::ostream& os, const double3_t& dt);
std::ostream& operator<<(std::ostream& os, const float3_t& dt);

#if 0
#define CRIT_EXCEPTION
class CT_DEBUG_EXCEPTION : public std::runtime_error {
	CT_DEBUG c_debug;
public:
	CT_DEBUG_EXCEPTION(char const * in_str_file, uint in_i_line,
			char const * in_str_function, enum ET_DEBUG_TYPE in_e_type): std::runtime_error(_s.str()) {
		c_debug = CT_DEBUG::in(in_str_file, in_i_line, in_str_function, in_e_type);
	}

	const char* what() const throw();
};

throw _CRIT_EXCEPTION <<
#endif
#endif
