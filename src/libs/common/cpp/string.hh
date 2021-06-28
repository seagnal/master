/***********************************************************************
 ** string
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
#ifndef STRING_HH_
#define STRING_HH_

/**
 * @file string.hh
 *
 * @author SEAGNAL (johann.baudy@seagnal.fr)
 * @date Feb 10, 2013
 *
 * @version 1.0 Original version
 */

/***********************************************************************
 * Includes
 ***********************************************************************/
#include <string>
#include <vector>
#include <stdarg.h>

/***********************************************************************
 * Defines
 ***********************************************************************/
enum ET_STRING_HUMAN_READABLE_MODE {
	E_STRING_HUMAN_READABLE_MODE_DEFAULT = 0,
	E_STRING_HUMAN_READABLE_MODE_OCTET,
	E_STRING_HUMAN_READABLE_MODE_IB,
};
/***********************************************************************
 * Types
 ***********************************************************************/

/***********************************************************************
 * Functions
 ***********************************************************************/

std::string f_string_expand (std::string const & in);

std::string f_string_replace(std::string const & in_str_source,
		const char * in_str_find, const char * in_str_replace);

std::string f_string_xml2name(std::string const & in_str_name);
std::string f_string_name2xml(std::string const & in_str_name);

std::string f_string_remove_redundant_char(std::string const & in_str,
		char const in_char);
std::string f_string_remove_char(std::string const & in_str,
		char const in_char);
std::string f_string_remove_whitespaces(std::string const & in_str);

std::vector<std::string> f_string_split(std::string const &in_str,
		std::string const &in_str_delim);

std::string f_string_toupper(std::string const & in_str_source);
std::string f_string_tolower(std::string const & in_str_source);

void f_string_split_ext_and_base(std::string & out_str_base,
		std::string & out_str_ext, std::string const & in_str);

void f_string_split_on_last_delim(std::string & out_str_base,
		std::string & out_str_ext, std::string const & in_str, std::string const &in_str_delim);

std::string f_string_format(const char * in_str_replace, ...);
std::string f_string_format_va(const char * in_str_fmt, va_list in_s_ap);

std::string f_string_strftime_64ns(const char * in_str_fmt,
		uint64_t in_i_time);

void f_string_human_readable_number(std::string & out_str, uint64_t in_i_number, ET_STRING_HUMAN_READABLE_MODE in_e_mode = E_STRING_HUMAN_READABLE_MODE_DEFAULT);
void f_string_human_readable_time(std::string & out_str, uint64_t in_i_time);
std::string const f_string_human_readable_number(uint64_t in_i_number,
		ET_STRING_HUMAN_READABLE_MODE in_e_mode = E_STRING_HUMAN_READABLE_MODE_DEFAULT);
std::string const f_string_human_readable_time(uint64_t in_i_time);

std::wstring f_wstring_format(const wchar_t * in_str_replace, ...);
std::wstring f_wstring_format_va(const wchar_t * in_str_fmt, va_list in_s_ap);

//std::string f_string_from_buffer(uint8_t * in_pc_buffer, size_t in_sz_buffer);
template <typename T>
std::string f_string_from_buffer(T * in_pc_buffer, size_t in_sz_buffer);
#endif /* STRING_HH_ */
