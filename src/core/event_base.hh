/***********************************************************************
 ** event_base.hh
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
#ifndef EVENT_BASE_HH_
#define EVENT_BASE_HH_

/**
 * @file event_base.hh
 * Event base definition.
 *
 * @author SEAGNAL (johann.baudy@seagnal.fr)
 * @date 2013
 *
 * @version 1.0 Original version
 */
/***********************************************************************
 * Includes
 ***********************************************************************/
#include <cstdint>

/***********************************************************************
 * Defines
 ***********************************************************************/
#define C_ID_OFFSET_BASE 16
#define C_ID_CAVEO_OFFSET_BASE 28


#define E_ID_COMMON_BASE ((uint32_t)0xFFFF)
#define E_ID_TASK_BASE 0xFFFE

enum ET_ID_LINK {
	E_ID_LINK_CONNECT,
	E_ID_LINK_CONNECTED,
	E_ID_LINK_DISCONNECTED,
};

enum ET_ID_COMMON {
	E_ID_COMMON_TIME64NS = E_ID_COMMON_BASE << C_ID_OFFSET_BASE,
	E_ID_COMMON_FILE,
	E_ID_COMMON_VECTORXF,
	E_ID_COMMON_VERSION,

	E_ID_COMMON_ARRAY_SIZE_X, // uint32_t
	E_ID_COMMON_ARRAY_SIZE_Y, // uint32_t
	E_ID_COMMON_ARRAY_ID,     // uint32_t
	E_ID_COMMON_ID_POINTER = 0xFFFFFFFF,
	E_ID_COMMON_ID_RAW = 0xFFFFFFFE,
};

#endif /* EVENT_BASE_HH_ */
