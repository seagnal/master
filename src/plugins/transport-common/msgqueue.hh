/***********************************************************************
 ** msgqueue.hh
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
#ifndef MSGQUEUE_HH_
#define MSGQUEUE_HH_
/**
 * @file msgqueue.hh
 * MSG queue pLugin definition.
 *
 * @author SEAGNAL (johann.baudy@seagnal.fr)
 * @date 2013
 *
 * @version 1.0 Original version
 */

/***********************************************************************
 * Includes
 ***********************************************************************/
#include <boost/interprocess/ipc/message_queue.hpp>
#include <c/common.h>
#include <plugin.hh>
#include <host.hh>


/***********************************************************************
 * Defines
 ***********************************************************************/

/***********************************************************************
 * Types
 ***********************************************************************/


class CT_MSGQUEUE: public CT_TRANSPORT {
	/* Queue name */
	std::string _str_name;

	/* Meaage queue pointer */
	boost::interprocess::message_queue * _pc_mq;

	/* Context of msqqueue. Msgqueue has its own context */
	CT_HOST_CONTEXT * _pc_context;

	/* Queue msg size */
	size_t _sz_msg;

	/* Queue msg offset */
	int _i_offset;

	/* Queue temp buffer */
	char *_pc_buffer;

	/* Queue tmp buffer max size */
	size_t _sz_buffer;

	/* Queue tmp buffer size */
	size_t _sz_available;

  /* Number of element in queue */
  uint16_t _i_msg_cnt;
public:
	CT_MSGQUEUE();
	~CT_MSGQUEUE();

	/* Applying config */
	int f_apply_config(std::string & in_str_url);

	/* Use classic node -> buffer conversion */
	int f_send(char * in_pc_buffer, size_t in_sz_buffer, size_t & out_sz_sent);

	int f_run(CT_HOST_CONTEXT&);
};

#endif /* MSGQUEUE_HH_ */
