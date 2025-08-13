/***********************************************************************
 ** misc
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
 * @file misc.cc
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
#include <cpp/debug.hh>
#include "misc.hh"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <utime.h>

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <cpp/string.hh>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <regex>

static int __f_file_dump(char const * in_str_file, char * out_buffer, size_t sz_buffer) {
  int fd = open(in_str_file, O_RDONLY);
  int ec = EC_FAILURE;
  if (fd >= 0) {
    int ret = read(fd, out_buffer, sz_buffer);
#if 0
    int i;
    DBG("%d", ret);
    for( i=0; i<ret; i++) {
      DBG("%d %x %c", i, out_buffer[i], out_buffer[i]);
    }
#endif
    out_buffer[ret - 1] = 0;
    if (ret == -1) {
      ec = EC_FAILURE;
    } else {
      ec = EC_SUCCESS;
    }
    close(fd);
  }
  return ec;
}


// Function to generate a random string of given length
std::string f_misc_generate_random_string(int in_i_length)
{
	std::string str = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	int n = str.length();

	std::string res = "";
	for (int i = 0; i < in_i_length; i++)
		res = res + str[rand() % n];

	return res;
}


std::string f_misc_get_output(const std::string & in_str_cmd) {
  std::string str_tmp;
  FILE *fp;

  /* Open the command for reading. */
  fp = popen(in_str_cmd.c_str(), "r");
  if (fp == NULL) {
    return "";
  }

  /* Read the output a line at a time - output it. */
  char path[1024];
  while (fgets(path, sizeof(path), fp) != NULL) {
    str_tmp += std::string(path);
  }

  /* close */
  pclose(fp);

  return str_tmp;
}
bool f_file_exits(const std::string& in_str_name) {
	struct stat s_buffer;
	return (stat(in_str_name.c_str(), &s_buffer) == 0);
}

time_t f_file_touch(const std::string& in_str_pathname) {
	int ec;
	/* Create file if needed */
	{
		int fd = open(in_str_pathname.c_str(),
				O_WRONLY | O_CREAT | O_NOCTTY | O_NONBLOCK, 0666);
		if (fd < 0) // Couldn't open that path.
				{
			throw _CRIT << ": Couldn't open() path \"" << in_str_pathname;
		}
		close(fd);
	}


	struct stat s_stats;
	time_t s_now = time(NULL);

	/* Get current time of file */
	ec = stat(in_str_pathname.c_str(), &s_stats);
	M_ASSERT(ec == 0);

	/* Update time, if older */
	if (s_now > s_stats.st_mtim.tv_sec) {
		struct utimbuf s_puttime;
		s_puttime.modtime = s_now;
		int rc = utime(in_str_pathname.c_str(), &s_puttime);
		if (rc) {
			throw _CRIT << "Couldn't utimensat() path \"" << in_str_pathname;
		}
		return s_now;
	} else {
		return s_stats.st_mtim.tv_sec;
	}

}

static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";

static inline bool f_is_base64(char c) {
	return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string f_base64_encode(char const* in_pc_buf, size_t in_sz_buf) {
	std::string ret;
	int i = 0;
	int j = 0;
	char char_array_3[3];
	char char_array_4[4];

	while (in_sz_buf--) {
		char_array_3[i++] = *(in_pc_buf++);
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4)
					+ ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2)
					+ ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for (i = 0; (i < 4); i++)
				ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if (i) {
		for (j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4)
				+ ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2)
				+ ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; (j < i + 1); j++)
			ret += base64_chars[char_array_4[j]];

		while ((i++ < 3))
			ret += '=';
	}

	return ret;
}

std::vector<char> f_base64_decode(std::string const& in_str) {
	int in_len = in_str.size();
	int i = 0;
	int j = 0;
	int in_ = 0;
	char char_array_4[4], char_array_3[3];
	std::vector<char> ret;

	while (in_len-- && (in_str[in_] != '=') && f_is_base64(in_str[in_])) {
		char_array_4[i++] = in_str[in_];
		in_++;
		if (i == 4) {
			for (i = 0; i < 4; i++)
				char_array_4[i] = base64_chars.find(char_array_4[i]);

			char_array_3[0] = (char_array_4[0] << 2)
					+ ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4)
					+ ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
				ret.push_back(char_array_3[i]);
			i = 0;
		}
	}

	if (i) {
		for (j = i; j < 4; j++)
			char_array_4[j] = 0;

		for (j = 0; j < 4; j++)
			char_array_4[j] = base64_chars.find(char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2)
				+ ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4)
				+ ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++)
			ret.push_back(char_array_3[j]);
	}

	return ret;
}

std::string f_misc_get_ip_address(std::string const & in_str_device){
  int fd;
  int ec;
  std::string res;
  if(in_str_device == "lo") {
    res =  "127.0.0.1";
    goto out_err;
  } else {
    struct ifreq ifr;
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0){
      goto out_err;
    }
    /* I want to get an IPv4 IP address */
    ifr.ifr_addr.sa_family = AF_INET;
    /* I want IP address attached to device */
    size_t sz_len = M_MIN(IFNAMSIZ-1,int(in_str_device.size()));
    strncpy(ifr.ifr_name, in_str_device.c_str(), sz_len);
    ifr.ifr_name[sz_len] = 0;


    ec  = ioctl(fd, SIOCGIFADDR, &ifr);
    if (ec < 0){
      _CRIT << "Unable to get SIOCGIFADDR of" << ifr.ifr_name;
      goto out_free_socket;
    }
    if(inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr) != NULL){
      res = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
    }
    _DBG << "ip address of "<< ifr.ifr_name << "is" << res;
  }
  out_free_socket:
  close(fd);
  out_err:

  return res;
}

uint64_t f_get_size_file(const std::string& in_str_name){
	struct stat s_buffer;

	bool b_file_exist = f_file_exits(in_str_name);
	M_ASSERT(b_file_exist);

	stat(in_str_name.c_str(), &s_buffer);
	return ((uint64_t)s_buffer.st_size);
}


std::string f_misc_str_remove_start(std::string const & in_str_line, uint16_t i_size_max){
	return f_misc_str_remove_middle(in_str_line, i_size_max, 0, i_size_max - 5);
}

std::string f_misc_str_remove_end(std::string const & in_str_line, uint16_t i_size_max){
	return f_misc_str_remove_middle(in_str_line, i_size_max, i_size_max - 5, 0);
}

/* Replace middle of string with [...] to force maximum string size */
std::string f_misc_str_remove_middle(std::string const & in_str_line, uint16_t i_size_max, uint16_t i_nb_carac_start=0, uint16_t i_nb_carac_end=0){

  std::string str_middle = "[...]";

  assert(i_size_max >= i_nb_carac_start + i_nb_carac_end + str_middle.size());

  if (in_str_line.size() < i_size_max) {
	  return in_str_line;
  } else {
	  if (i_nb_carac_start + str_middle.size() + i_nb_carac_end > i_size_max) {
		  uint16_t i_extra_caracters = i_nb_carac_start + str_middle.size() + i_nb_carac_end - i_size_max;
		  i_nb_carac_start -= (i_extra_caracters+1) / 2;
		  i_nb_carac_end   -= (i_extra_caracters+1) / 2;
	  } else {
		  uint16_t i_extra_caracters = i_size_max - (i_nb_carac_start + str_middle.size() + i_nb_carac_end);
		  i_nb_carac_start += (i_extra_caracters) / 2;
		  i_nb_carac_end   += (i_extra_caracters) / 2;
    }

	  assert(i_nb_carac_start + str_middle.size() + i_nb_carac_end <= i_size_max);

	  std::string str_out = in_str_line.substr(0,i_nb_carac_start);
	  str_out += str_middle;
	  str_out += in_str_line.substr(in_str_line.size()-i_nb_carac_end,in_str_line.size());

	  assert(str_out.size() <= i_size_max);

	  return str_out;
  }

}

float f_misc_get_themal_temp(uint32_t in_i_thermal_zone){
  char buffer[128]; // return load cpu since 1 minutes
  float f_temp = 0.0;
  std::string str_filename = std::string("/sys/class/thermal/thermal_zone")+std::to_string(in_i_thermal_zone)+"/temp";

  int ec = __f_file_dump(str_filename.c_str(),buffer,(size_t)sizeof(buffer));
  if (ec != EC_SUCCESS) {
     f_temp = 0;
  }else{
    f_temp = atof(buffer)/1000.0;
  }
  //printf("\n Valeur lu dans :: </proc/loadavg> == %s \n",buffer_load_cpu );
  return f_temp;
}

std::string f_misc_get_themal_type(uint32_t in_i_thermal_zone){
  char buffer[128]; // return load cpu since 1 minutes
  std::string str_type;
  std::string str_filename = std::string("/sys/class/thermal/thermal_zone")+std::to_string(in_i_thermal_zone)+"/type";

  int ec = __f_file_dump(str_filename.c_str(),buffer,(size_t)sizeof(buffer));
  if (ec != EC_SUCCESS) {

  }else{
    str_type = buffer;
  }
  //printf("\n Valeur lu dans :: </proc/loadavg> == %s \n",buffer_load_cpu );
  return str_type;
}

std::list<std::string> f_misc_file_list(
		std::string const in_str_path, std::string const & in_str_prefix,
		std::string const & in_str_suffix, bool const in_b_debug) {
	std::list<std::string> _l_tmp;
	std::string str_path(in_str_path);

	std::vector<std::string> c_paths;
	boost::split(c_paths, str_path, boost::is_any_of(";"));

	/* Parse all path split with ; */
	for (std::vector<std::string>::const_iterator c_it = c_paths.begin();
			c_it != c_paths.end(); ++c_it) {
		boost::filesystem::path someDir(*c_it);
		boost::filesystem::directory_iterator end_iter;

		auto str_re = f_string_format("(.*)(/)(%s)(.*)(\\.)%s",
				in_str_prefix.c_str(), in_str_suffix.c_str());
		const std::regex c_filter(str_re);

    if(in_b_debug) {
      D("Path: %s %s", someDir.generic_string().c_str(), str_re.c_str());
    }
		if (boost::filesystem::exists(someDir)
				&& boost::filesystem::is_directory(someDir)) {

			/* Parse all file of current folder */
			for (boost::filesystem::directory_iterator dir_iter(someDir);
					dir_iter != end_iter; ++dir_iter) {
				/* Check that current file is regular */
				if (boost::filesystem::is_regular_file(dir_iter->status())) {
					std::string str_tmp = dir_iter->path().generic_string();

          if(in_b_debug) {
            D("Match: %s",str_tmp.c_str());
          }
					/* Match plugin regexp */
					if (std::regex_match(str_tmp.begin(), str_tmp.end(),
							c_filter)) {
						_l_tmp.push_back(str_tmp);
					}
				}
			}
		}
	}

	return _l_tmp;
}

int f_misc_create_folder(std::string const & in_str_path) {
  try {
    boost::filesystem::path c_folder_path(in_str_path);
    if (!boost::filesystem::exists(c_folder_path)) {
        boost::filesystem::create_directory(c_folder_path);
        return EC_SUCCESS;
    }
    return EC_BYPASS;
  } catch (std::exception const & e) {
    _CRIT << "Failed to create folder: " << e.what();
    return EC_FAILURE;
  }
}
