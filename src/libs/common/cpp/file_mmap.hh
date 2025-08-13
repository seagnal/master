/***********************************************************************
 ** file_mmap.hh
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
#ifndef FILE_MMAP_HH_
#define FILE_MMAP_HH_

/**
 * @file file_mmap.hh
 *
 * @author SEAGNAL (nicolas.massot@seagnal.fr)
 * @date Jan , 2020
 *
 * @version 1.0 Original version
 */

/***********************************************************************
 * Includes
 ***********************************************************************/
#include <string>
#include <vector>
#include <limits>
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

class CT_FILE_MMAP
{
	private:
		int _i_size_buffer;
		uint64_t _i_size_file;
		int _i_file_descriptor;

	public:

    CT_FILE_MMAP();
    ~CT_FILE_MMAP();
		void f_open(const std::string& in_str_name);

		template<typename T>
		void f_close(T *& dst);

		template<typename T>
		T *  f_get_buffer();
		uint64_t f_get_size_file_mmap();
		int f_get_file_descriptor();


};

CT_FILE_MMAP::CT_FILE_MMAP(){}
CT_FILE_MMAP::~CT_FILE_MMAP(){}

void CT_FILE_MMAP::f_open(const std::string& in_str_name){
	_i_file_descriptor = open(in_str_name.c_str(), O_LARGEFILE | O_CREAT |O_RDWR, S_IRUSR | S_IWUSR | S_IXUSR | S_IXOTH);
	if (_i_file_descriptor < 0) // Couldn't open that path.
			{
		throw _CRIT << ": Couldn't open() file \"" << in_str_name;
	}

	// Get size
	_i_size_file = f_get_size_file(in_str_name);
}

template<typename T>
void CT_FILE_MMAP::f_close(T *& dst){

 int ec = munmap(dst, _i_size_file);
 if (ec < 0) // Couldn't open that path.
		 {
	 throw _CRIT << ": Couldn't munmap() file \"";
 }

 ec = close(_i_file_descriptor);
 if (ec < 0) // Couldn't close the file
{
	throw _CRIT << ": Couldn't close() the mmap file \"";
 }
}

uint64_t CT_FILE_MMAP::f_get_size_file_mmap(){
	return _i_size_file;
}

int CT_FILE_MMAP::f_get_file_descriptor(){
	return _i_file_descriptor;
}

template<typename T>
T * CT_FILE_MMAP::f_get_buffer(){
	T * dst = NULL;
	dst = (T *)mmap(NULL, _i_size_file, PROT_READ | PROT_WRITE, MAP_SHARED, _i_file_descriptor, 0);
	if(dst == MAP_FAILED){
		std::cout << "ERROR MMAP : " << errno << std::endl;
		int ec = close(_i_file_descriptor);
		if (ec < 0) // Couldn't close the file
	 	{
	 		throw _CRIT << ": Mmap Error : Couldn't close() the mmap file \"";
	  }
		throw _CRIT << ": Mmap Error : Couldn't mmap() the file \"";
	}
 return dst;
}

#endif /* FILE_MMAP_HH_*/
