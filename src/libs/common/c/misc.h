/***********************************************************************
 ** misc.h
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

#ifndef MISC_H_
#define MISC_H_


/**
 * @file misc.h
 * Misc methods.
 *
 * @author SEAGNAL (johann.baudy@seagnal.fr)
 * @date 2013
 *
 * @version 1.0 Original version
 */

/***********************************************************************
 * Includes
 ***********************************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/***********************************************************************
 * Defines
 ***********************************************************************/


/***********************************************************************
 * Types
 ***********************************************************************/


/***********************************************************************
 * Functions
 ***********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
uint64_t f_get_time_ns64( void);
uint64_t f_get_time_ns64_from_time_t(time_t * in_i_time);
uint64_t f_get_time_ns64_from_timespec(struct timespec * in_s_time);
void f_dump_stat(struct stat * in_s_sb);
int f_file_dump( char const * in_str_file, char * out_buffer, size_t sz_buffer);
size_t f_misc_get_total_system_memory(void);
size_t f_misc_get_total_free_mem(void);
size_t f_misc_get_only_free_mem(void);
int f_set_file_content(char const * in_str_file, char const * out_buffer, size_t sz_buffer);
int f_misc_file_exists (const char * in_str_file);
int f_misc_folder_exists (const char * in_str_file);
pid_t f_misc_popen2(const char *command, int *infp, int *outfp);
int f_misc_check_env_i(const char * in_str_varenv, uint32_t const in_f_default_value);
float f_misc_get_cpu_load(void);
size_t f_misc_get_remaining_space(const char * ac_folder);
int f_misc_execute(/*std::string const &*/const char * in_str_cmd, int8_t in_b_display);
int f_misc_is_mounted(const char * in_str_mnt);
#ifdef __cplusplus
}
;
#endif


#endif /* MUTEX_H_ */
