/***********************************************************************
 ** msgqueue.cc
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
 * @file msgqueue.cc
 * Plugin msgqueue.
 *
 * @author SEAGNAL (johann.baudy@seagnal.fr)
 * @date 2013
 *
 * @version 1.0 Original version
 */

/***********************************************************************
 * Includes
 ***********************************************************************/

#include <msgqueue.hh>

using namespace std;

using namespace boost::interprocess;

CT_MSGQUEUE::CT_MSGQUEUE(void) :
		CT_TRANSPORT() {
	_pc_mq = NULL;
	_pc_context = NULL;
	_sz_msg = 4096;
	_i_offset = 0;
	_sz_available = 0;
  _i_msg_cnt = 100;
	_pc_buffer = new char[C_PORT_NODE_BUFFER_SIZE];
	M_ASSERT(_pc_buffer);
	_sz_buffer = C_PORT_NODE_BUFFER_SIZE;
}

CT_MSGQUEUE::~CT_MSGQUEUE() {

	/* Stop and join context */
	if (_pc_context) {
		delete _pc_context;
	}
	/* delete Msg queue */
	if (_pc_mq) {
		M_ASSERT(_pc_context);
		delete _pc_mq;
	}
	/* delete buffer */
	if (_pc_buffer) {
		delete[] _pc_buffer;
		_pc_buffer = NULL;
	}
}

int CT_MSGQUEUE::f_apply_config(std::string & in_str_url) {
	int ec;
	M_ASSERT(!_pc_context);
	/** Create context */
	_pc_context = CT_HOST::host->f_new_context();
	M_ASSERT(_pc_context);

	/* Erase previous message queue */
	message_queue::remove(in_str_url.c_str());

  /* Get Size info */
  if(_c_config.has("size")) {
      _i_msg_cnt = (uint64_t)(_c_config("size"));
  }

	/* Store queue name */
	_str_name = in_str_url.c_str();

	/*Create a message_queue. */
	_pc_mq = new message_queue(create_only //only create
			, in_str_url.c_str() //name
			, _i_msg_cnt //max message number
			, _sz_msg //max message size
			);
	if (!_pc_mq) {
		CRIT("Unable to allocate msg queue");
		ec = EC_FAILURE;
		goto out_err;
	}
	//D("Creating message queue: %s", in_str_url.c_str());

	/** Start context */
	ec = _pc_context->f_start(M_CONTEXT_BIND(CT_MSGQUEUE, f_run, this));
	if (ec != EC_SUCCESS) {
		CRIT("Unable to start context");
		ec = EC_FAILURE;
		goto out_err;
	}
	ec = EC_SUCCESS;
	out_err: return ec;
}

int CT_MSGQUEUE::f_run(CT_HOST_CONTEXT&) {
	int ec;
	unsigned int i_priority;
	size_t sz_recv;

	/*Receive 100 numbers */

	char pc_buffer[C_PORT_NODE_BUFFER_SIZE];

	size_t sz_min = _sz_msg;

	/* Check remaining size */

	size_t sz_remain = _sz_buffer - _i_offset - _sz_available;
	{
		if ((sz_remain < sz_min) && _i_offset) {
			memmove(&pc_buffer[0], &pc_buffer[_i_offset], _sz_available);
			_i_offset = 0;
		}
		sz_remain = _sz_buffer - _i_offset - _sz_available;
		if (sz_remain < sz_min) {
			_CRIT << "MSG queue " << _str_name << " is full";
		}
	}
	/* Read msg queue */
	//using namespace boost::posix_time;
	boost::posix_time::ptime c_now(
			boost::posix_time::microsec_clock::universal_time());
	boost::posix_time::ptime c_timeout = c_now
			+ boost::posix_time::milliseconds(1000);

	ec = _pc_mq->timed_receive(&pc_buffer[_i_offset + _sz_available], sz_remain,
			sz_recv, i_priority, c_timeout);
	if (ec) {
		//D("Received %d bytes", sz_recv);
		/* Execute classic receive data */
		size_t i_read;
		_sz_available += sz_recv;
		/* Trying to decode buffer */
		ec = f_received(&pc_buffer[_i_offset], _sz_available, i_read);
		if (ec != EC_SUCCESS) {
			CRIT("Unable to handle received data");
			_i_offset = 0;
			_sz_available = 0;
		} else {
			M_ASSERT(i_read <= sz_recv);
			_i_offset += i_read;
			_sz_available -= i_read;
		}
		ec = EC_SUCCESS;
	} else {
		//D("Timeout MSGQUEUE: %s ", _str_name.c_str());
		_i_offset = 0;
		_sz_available = 0;
		ec = EC_BYPASS;
	}
	//DBG_LINE();

	return ec;
}

int CT_MSGQUEUE::f_send(char * in_pc_buffer, size_t in_sz_buffer,
		size_t & out_sz_sent) {
	int ec;
	size_t i_sent = M_MIN(_sz_msg, in_sz_buffer);

	/* Send nessage */
	//D("Send %d bytes", in_sz_buffer);
	_pc_mq->send(in_pc_buffer, i_sent, 0);
	out_sz_sent = i_sent;

	ec = EC_SUCCESS;
	return ec;
}

M_PLUGIN_TRANSPORT(CT_MSGQUEUE, "MSG", M_STR_VERSION);
