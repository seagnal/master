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

/**
 * @file string.cc
 *
 * @author SEAGNAL (johann.baudy@seagnal.fr)
 * @date Feb 10, 2013
 *
 * @version 1.0 Original version
 */

/***********************************************************************
 * Includes
 ***********************************************************************/
#include <c/common.h>
#include <cpp/string.hh>
#include <cpp/debug.hh>
#include <string.h>
#include <stdarg.h>

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <string>
#include <wchar.h>
#include <math.h>

std::string f_string_expand (std::string const & in) {
	if (in.size () < 1) return in;
	const char * home = getenv ("HOME");
	if (home == NULL) {
		_DBG << "error: HOME variable not set.";
		throw std::invalid_argument ("error: HOME environment variable not set.");
	}

	std::string s = in.c_str ();
	if (s[0] == '~') {
		s = std::string(home) + s.substr (1, s.size () - 1);
		return s;
	} else {
		return in;
	}
}

std::string f_string_tolower(std::string const & in_str_source) {
	std::string str_tmp = in_str_source;
	std::transform(str_tmp.begin(), str_tmp.end(), str_tmp.begin(), ::tolower);
	return str_tmp;
}
std::string f_string_toupper(std::string const & in_str_source) {
	std::string str_tmp = in_str_source;
	std::transform(str_tmp.begin(), str_tmp.end(), str_tmp.begin(), ::toupper);
	return str_tmp;
}

std::string f_string_replace(std::string const & in_str_source,
		const char * in_str_find, const char * in_str_replace) {
	size_t i = 0;
	std::string str_dest = in_str_source;
	int i_offset = 0;

	while ((i = (in_str_source).find(in_str_find, i)) != std::string::npos) {
		str_dest.replace(i + i_offset, strlen(in_str_find), in_str_replace);
		i_offset += strlen(in_str_replace) - strlen(in_str_find);
		i++;
	}

	return str_dest;
}

std::string f_string_xml2name(std::string const & in_str_name) {
	std::string str_tmp = in_str_name;
	str_tmp = f_string_replace(str_tmp, "__space__", " ");
	str_tmp = f_string_replace(str_tmp, "__pct__", "%");
	str_tmp = f_string_replace(str_tmp, "__lp__", "(");
	str_tmp = f_string_replace(str_tmp, "__rp__", ")");
	str_tmp = f_string_replace(str_tmp, "__sharp__", "#");
	str_tmp = f_string_replace(str_tmp, "__inf__", "<");
	str_tmp = f_string_replace(str_tmp, "__sup__", ">");
	str_tmp = f_string_replace(str_tmp, "__comma__", ",");
	str_tmp = f_string_replace(str_tmp, "__dot__", ".");
	return f_string_replace(str_tmp, "__trail__", "");
}

std::string f_string_name2xml(std::string const & in_str_name) {
	std::string str_tmp = in_str_name;
	if (str_tmp.size() && isdigit(str_tmp[0])) {
		str_tmp = std::string("__trail__") + str_tmp;
	}

	str_tmp = f_string_replace(str_tmp, "%", "__pct__");
	str_tmp = f_string_replace(str_tmp, "(", "__lp__");
	str_tmp = f_string_replace(str_tmp, ")", "__rp__");
	str_tmp = f_string_replace(str_tmp, "#", "__sharp__");
	str_tmp = f_string_replace(str_tmp, "<", "__inf__");
	str_tmp = f_string_replace(str_tmp, ">", "__sup__");
	str_tmp = f_string_replace(str_tmp, ",", "__comma__");
	str_tmp = f_string_replace(str_tmp, ".", "__dot__");
	return f_string_replace(str_tmp, " ", "__space__");
}

std::string f_string_remove_redundant_char(std::string const & in_str,
		char const in_char) {
	std::string::size_type pos;
	std::string::size_type pos_start = 0;

	std::string str_tmp = in_str;

	while (1) {
		pos = str_tmp.find(in_char, pos_start);
		if (pos == std::string::npos)
			break;
		if (pos + 1 >= str_tmp.size())
			break;

		while (str_tmp[pos + 1] == in_char) {
			str_tmp.erase(pos + 1, 1);
		}
		pos_start = pos + 1;
	}
	return str_tmp;
}

std::string f_string_remove_char(std::string const & in_str,
		char const in_char) {
	std::string::size_type pos = 0;
	bool spacesLeft = true;
	std::string str_tmp = in_str;
	while (spacesLeft) {
		pos = str_tmp.find(in_char);
		if (pos != std::string::npos)
			str_tmp.erase(pos, 1);
		else
			spacesLeft = false;
	}

	return str_tmp;
}

std::string f_string_remove_whitespaces(std::string const & in_str) {
	std::string str_tmp = in_str;
	str_tmp = f_string_remove_char(str_tmp, '\n');
	str_tmp = f_string_remove_char(str_tmp, '\r');
	str_tmp = f_string_remove_char(str_tmp, '\t');
	str_tmp = f_string_remove_redundant_char(str_tmp, ' ');

	while (str_tmp.size() && str_tmp[0] == ' ') {
		str_tmp.erase(0, 1);
	}

	while (str_tmp.size() && str_tmp[str_tmp.size() - 1] == ' ') {
		str_tmp.erase(str_tmp.size() - 1, 1);
	}

	return str_tmp;
}

void f_string_split(std::string const &in_str, std::string const &in_str_delim,
		std::vector<std::string> &in_v_elems) {
	std::string str_tmp = in_str;

	unsigned i_found_pos = str_tmp.find(in_str_delim);

	while (1) {
		/* Looking for pattern */
		i_found_pos = str_tmp.find(in_str_delim);
		/* Check if delim is located into string */
		if (i_found_pos == std::string::npos) {
			in_v_elems.push_back(str_tmp);
			break;
		}

		//D("Found: %d %s in %s (ls:%d, %d > %d)", i_found_pos, in_str_delim.c_str(), str_tmp.c_str(), in_v_elems.size(), i_found_pos + in_str_delim.size(),str_tmp.size());
		/* Added first part of string in list */
		in_v_elems.push_back(str_tmp.substr(0, i_found_pos));
		/* Check if delim is at the end of string */
		if (i_found_pos + in_str_delim.size() > str_tmp.size()) {
			break;
		}
		/* Remove first part of string */
		str_tmp = str_tmp.substr(i_found_pos + in_str_delim.size(),
				str_tmp.size());
	}
}

std::vector<std::string> f_string_split(std::string const &in_str,
		std::string const &in_str_delim) {
	std::vector<std::string> v_elems;
	f_string_split(in_str, in_str_delim, v_elems);
	return v_elems;
}

std::string f_string_format(const char * in_str_fmt, ...) {

	std::string str;
	va_list ap;

	va_start(ap, in_str_fmt);
	str = f_string_format_va(in_str_fmt, ap);
	va_end(ap);

	return str;
}

std::string f_string_format_va(const char * in_str_fmt, va_list in_s_ap) {
	int size = 100;
	std::string str;
	//va_list ap = in_s_ap;
	//printf("size format: %d\n",(int)strlen(in_str_fmt));
	while (1) {
		va_list ap_tmp;
		va_copy(ap_tmp, in_s_ap);
		str.resize(size);

		//printf("size: %d\n",size);
		int n = vsnprintf((char *) str.c_str(), size, in_str_fmt, ap_tmp);
		//printf("n: %d\n",n);
		if (n > -1 && n < size) {
			str.resize(n);
			return str;
		}
		if (n > -1)
			size = n + 1;
		else
			size *= 2;

		va_end(ap_tmp);
	}
	return str;
}

void f_string_split_ext_and_base(std::string & out_str_base,
		std::string & out_str_ext, std::string const & in_str) {
	f_string_split_on_last_delim(out_str_base, out_str_ext, in_str, ".");
}

void f_string_split_on_last_delim(std::string & out_str_base,
		std::string & out_str_ext, std::string const & in_str,
		std::string const &in_str_delim) {
	std::string::size_type idx = in_str.rfind(in_str_delim);

	if (idx != std::string::npos) {
		out_str_ext = in_str.substr(idx + 1);
		out_str_base = in_str.substr(0, idx);
	} else {
		out_str_ext = "";
		out_str_base = in_str;
	}
}

void f_string_human_readable_number(std::string & out_str, uint64_t in_i_number,
		ET_STRING_HUMAN_READABLE_MODE in_e_mode) {
	const char * str_suffix;
	double d_lim;
	double d_value;

	switch (in_e_mode) {

	case E_STRING_HUMAN_READABLE_MODE_IB: {
		d_lim = 1024;
		str_suffix = "ib";
		break;
	}
	case E_STRING_HUMAN_READABLE_MODE_OCTET: {
		d_lim = 1000;
		str_suffix = "o";
		break;
	}
	default: {
		d_lim = 1000;
		str_suffix = "";
		break;
	}
	}

	d_value = double(in_i_number) / (d_lim * d_lim * d_lim * d_lim);
	if (d_value > 1) {
		out_str = f_string_format("%0.2f T%s", d_value, str_suffix);
		return;
	}
	d_value *= d_lim;
	if (d_value > 1) {
		out_str = f_string_format("%0.2f G%s", d_value, str_suffix);
		return;
	}
	d_value *= d_lim;
	if (d_value > 1) {
		out_str = f_string_format("%0.2f M%s", d_value, str_suffix);
		return;
	}
	d_value *= d_lim;
	if (d_value > 1) {
		out_str = f_string_format("%0.2f K%s", d_value, str_suffix);
		return;
	}

	d_value *= d_lim;
	out_str = f_string_format("%u %s", uint(d_value), str_suffix);
}
std::string const f_string_human_readable_number(uint64_t in_i_number,
		ET_STRING_HUMAN_READABLE_MODE in_e_mode) {
      std::string str_tmp;
      f_string_human_readable_number(str_tmp, in_i_number, in_e_mode);
      return str_tmp;
  }


  std::string const f_string_human_readable_time(uint64_t in_i_time) {
        std::string str_tmp;
        f_string_human_readable_time(str_tmp, in_i_time);
        return str_tmp;
    }

void f_string_human_readable_time(std::string & out_str, uint64_t in_i_time) {
	std::string str_day;
	double d_value = double(in_i_time) / (1e9 * 60 * 60 * 24);
	uint i_day = floor(d_value);

	uint i_hours;
	uint i_minutes;
	uint i_seconds;
	uint i_msecs;

	d_value -= i_day;
	d_value *= 24;
	i_hours = floor(d_value);

	d_value -= i_hours;
	d_value *= 60;
	i_minutes = floor(d_value);

	d_value -= i_minutes;
	d_value *= 60;
	i_seconds = floor(d_value);

	d_value -= i_seconds;
	d_value *= 1000;
	i_msecs = floor(d_value);

	if (i_day) {
		out_str = f_string_format("%02d:%02d:%02d", i_hours, i_minutes,
				i_seconds);
	} else {
		if (i_hours || i_minutes) {
			out_str = f_string_format("%02d:%02d:%02d.%03d", i_hours, i_minutes,
					i_seconds, i_msecs);
		} else {
			if (i_seconds > 10) {
				out_str = f_string_format("%02d.%03d s", i_seconds, i_msecs);
			} else {
				out_str = f_string_format("%d ms", i_seconds * 1000 + i_msecs);
			}
		}
	}
}

std::string f_string_strftime_64ns(const char * in_str_fmt,
		uint64_t in_i_time) {
	char outstr[1024];
	time_t t_tmp = (time_t)((uint64_t) (in_i_time) / 1000000000LL);
	//uint64_t i_ns = in_i_time % 1000000000LL;
    struct tm tmp;
    if ( NULL == localtime_r(&t_tmp, &tmp)) {
        perror("localtime_r");
		exit(EXIT_FAILURE);
	}

    if (strftime(outstr, sizeof(outstr), in_str_fmt, &tmp) == 0) {
		fprintf(stderr, "strftime returned 0");
		exit(EXIT_FAILURE);
	}
	return std::string(outstr);
}

std::wstring f_wstring_format(const wchar_t * in_str_fmt, ...) {

	std::wstring str;
	va_list ap;

	va_start(ap, in_str_fmt);
	str = f_wstring_format_va(in_str_fmt, ap);
	va_end(ap);

	return str;
}

std::wstring f_wstring_format_va(const wchar_t * in_str_fmt, va_list in_s_ap) {
	int size = 100;
	std::wstring str;
	//va_list ap = in_s_ap;
	//printf("size format: %d\n",(int)strlen(in_str_fmt));
	while (1) {
		va_list ap_tmp;
		va_copy(ap_tmp, in_s_ap);
		str.resize(size);

		//printf("size: %d\n",size);
		int n = vswprintf((wchar_t *) str.c_str(), size, in_str_fmt, ap_tmp);
		//printf("n: %d\n",n);
		if (n > -1 && n < size) {
			str.resize(n);
			return str;
		}
		if (n > -1)
			size = n + 1;
		else
			size *= 2;

		va_end(ap_tmp);
	}
	return str;
}

template<>
std::string f_string_from_buffer(uint8_t * in_pc_buffer, size_t in_sz_buffer) {
	std::stringstream ss;
	//std::cout << in_sz_buffer << std::endl;

	for (size_t i = 0; i < in_sz_buffer; i += 1) {
		ss << std::hex << std::setfill('0') << std::uppercase << std::setw(2)
				<< (int) in_pc_buffer[i] << " ";
	}

	return ss.str();
}

template<>
std::string f_string_from_buffer(uint32_t * in_pc_buffer, size_t in_sz_buffer) {
	std::stringstream ss;
	//std::cout << in_sz_buffer << std::endl;

	for (size_t i = 0; i < in_sz_buffer; i += 1) {
		ss << std::hex << std::setfill('0') << std::uppercase << std::setw(8)
				<< (uint32_t) in_pc_buffer[i] << " ";
	}
	return ss.str();
}
