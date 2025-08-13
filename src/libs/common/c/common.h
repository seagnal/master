/***********************************************************************
 ** common.h
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
#ifndef COMMON_H_
#define COMMON_H_

/**
 * @file common.h
 * Common types, methods and define.
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
#include <assert.h>
#include <unistd.h>
#ifdef FF_VALGRIND
#include <valgrind/memcheck.h>
#else
#define VALGRIND_CHECK_VALUE_IS_DEFINED(a)
#define VALGRIND_CHECK_MEM_IS_DEFINED(a,b)
#endif
/***********************************************************************
 * Defines
 ***********************************************************************/

#define EC_BYPASS 1
#define EC_SUCCESS 0
#define EC_FAILURE -1

/***********************************************************************
 * Endianess stuff
 ***********************************************************************/
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define M_TO_LE32(__x) (__x)
#else
#define M_TO_LE32(__x) (uint32_t)( \
                 (((uint32_t)(__x) & (uint32_t)0x000000ffUL) << 24) | \
                 (((uint32_t)(__x) & (uint32_t)0x0000ff00UL) <<  8) | \
                 (((uint32_t)(__x) & (uint32_t)0x00ff0000UL) >>  8) | \
                 (((uint32_t)(__x) & (uint32_t)0xff000000UL) >> 24))
#endif
#define M_FROM_LE32(__x) M_TO_LE32(__x)

/***********************************************************************
 * Math stuff
 ***********************************************************************/
#define M_DEGTORAD (0.017453292519943295474371680597869271878153085)
#define M_RADTODEG (57.29577951308232286464772187173366546630859375)

#if !defined (__ASSERT_VOID_CAST)
#define __ASSERT_VOID_CAST (void)
#endif

#if !defined(__ASSERT_FUNCTION) && defined(_WIN32)
#define __ASSERT_FUNCTION __FUNCTION__
#endif

#if !defined(__STRING)
#define __STRING(x)	#x
#endif

#define M_ASSERT(expr) 							\
  ((expr)								\
   ? __ASSERT_VOID_CAST (0)						\
   : f_debug_assert_fail (__STRING(expr), __FILE__, __LINE__, __ASSERT_FUNCTION));
#define M_BUG(x) abort()

#define M_XSTR(s) M_STR(s)
#define M_STR(s) #s
#define M_STR_VERSION M_XSTR(BS_VERSION)
#define M_MAX(a,b) ((a)<(b)?(b):(a))
#define M_MIN(a,b) ((a)<(b)?(a):(b))
#define M_CLAMP(a, mi, ma) M_MIN((M_MAX(a,mi)),ma)
#define M_MIX(a, b, alpha) ((b)*(alpha) + (a)*(1.0-(alpha)))
#define M_ABS(a) ((a)<(0)?(-(a)):(a))
#define M_SIGN(a) ((a)<(0)?(-(1)):(1))
#define M_PACKED __attribute__ ((__packed__))
/***********************************************************************
 * Types
 ***********************************************************************/

struct float2_t
{
    float x, y;
} __attribute__ ((aligned (8)));

struct float3_t
{
    float x, y, z;
} __attribute__ ((packed));

struct double2_t
{
    double x, y;
} __attribute__ ((packed));

struct double3_t
{
    double x, y, z;
} /*__attribute__ ((packed))*/;


#endif /* COMMON_H_ */
