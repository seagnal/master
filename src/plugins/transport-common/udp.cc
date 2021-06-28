/***********************************************************************
 ** udp.cc
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
 * @file udp.cc
 * Plugin udp transport.
 *
 * @author SEAGNAL (johann.baudy@seagnal.fr)
 * @date 2013
 *
 * @version 1.0 Original version
 */

/***********************************************************************
 * Includes
 ***********************************************************************/
#include <boost/bind.hpp>
#include <cpp/string.hh>
#include <c/misc.h>
#include <udp.hh>
#include <sstream>

using namespace std;
using namespace bml;

CT_UDP::CT_UDP(void) :
		CT_TRANSPORT(), _c_io_service(), _c_socket(_c_io_service) {

	/* Point context init */
	_pc_context = NULL;

	/* Linked flags */
	_b_support_linked_transport = false;
	_b_linked_transport = false;

	/* Server mode */
	_b_server_mode = false;

	/* Buffer init */
	_i_offset_read = 0;
	_sz_buffer = C_PORT_NODE_BUFFER_SIZE;
	_pc_buffer_snd = NULL;
	_pc_buffer_snd = new char[_sz_buffer];
	_pc_buffer_rcv = NULL;
	_pc_buffer_rcv = new char[_sz_buffer];
	M_ASSERT(_pc_buffer_rcv);
	M_ASSERT(_pc_buffer_snd);
	_sz_available = 0;
}

CT_UDP::~CT_UDP() {
	/* Stop service */
	_c_io_service.stop();
	_c_socket.close();

	/* Stop and join context */
	if (_pc_context) {
		delete _pc_context;
	}

	/* Delete snd buffer */
	if (_pc_buffer_snd) {
		delete[] _pc_buffer_snd;
		_pc_buffer_snd = NULL;
	}

	/* Delete rcv buffer */
	if (_pc_buffer_rcv) {
		delete[] _pc_buffer_rcv;
		_pc_buffer_rcv = NULL;
	}
}

int CT_UDP::f_apply_config(std::string & in_str_url) {
	int ec;
	M_ASSERT(!_pc_context);
	/** Create context */
	_pc_context = CT_HOST::host->f_new_context();
	M_ASSERT(_pc_context);

	/* Erase previous message queue */
	std::vector<std::string> v_str = f_string_split(in_str_url, ":");

	/* Store queue name */
	if (v_str.size() != 2) {
		_CRIT << "UDP url is incorrect: " << in_str_url;
	}
	//_DBG << _V(in_str_url);

	_b_server_mode = false;

	/* Destination address or server mode */
	{
		_str_dst = v_str[0];
		if (_str_dst == "server") {
			_b_server_mode = true;

			/* Get port number from name */
			std::istringstream c_is(_str_dst);
			try {
				c_is >> _i_port;
			} catch (...) {
				_CRIT << "Incorrect port format: " << _str_dst;
			}
		}
	}

	/* Save service */
	{
		_str_service = v_str[1];
	}


	//_DBG << _V(_str_dst)<< _V(_str_service) << _V(_b_server_mode);

	/* Binding local or remote according to mode */
	f_bind();

	/* No reading thread if not in server mode */
	if (_b_server_mode) {
		/** Start IO service context */
		ec = _pc_context->f_start(M_CONTEXT_BIND(CT_UDP,f_run, this));
		if (ec != EC_SUCCESS) {
			CRIT("Unable to start context");
			ec = EC_FAILURE;
			goto out_err;
		}
	}
	ec = EC_SUCCESS;
	out_err: return ec;
}

void CT_UDP::f_bind(void) {
	udp::resolver resolver(_c_io_service);

	_c_socket.open(udp::v4());
	_c_socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));

  if(f_get_config().has("ttl")) {
     char i_ttl = uint64_t(*f_get_config().get("ttl"));
     boost::asio::ip::unicast::hops option(i_ttl);
     _c_socket.set_option(option);
      _DBG << "UDP setting ttl:" << (int) i_ttl;
  }

	int sz_buffer = 50 * 1024 * 1024;
	{
		boost::asio::socket_base::receive_buffer_size option(sz_buffer);
		_c_socket.set_option(option);
	}
	{
		boost::asio::socket_base::send_buffer_size option(sz_buffer);
		_c_socket.set_option(option);
	}

#if 0
	{
		boost::asio::socket_base::receive_buffer_size option;
		_c_socket.get_option(option);
		M_ASSERT(sz_buffer == option.value());
	}
	{
		boost::asio::socket_base::send_buffer_size option;
		_c_socket.get_option(option);
		M_ASSERT(sz_buffer == option.value());
	}
#endif

	try {
		if (_b_server_mode) {
			udp::resolver::query query(udp::v4(), _str_service,
					boost::asio::ip::resolver_query_base::all_matching
							| boost::asio::ip::resolver_query_base::passive);
			udp::resolver::iterator iterator = resolver.resolve(query);
			boost::asio::ip::udp::endpoint c_endpoint(*iterator);
			_c_socket.set_option(boost::asio::socket_base::broadcast(true));
			_c_socket.bind(c_endpoint);
		} else {
			if (_str_dst == "broadcast") {
				_c_socket.set_option(boost::asio::socket_base::broadcast(true));

				udp::resolver resolver(_c_io_service);
				try {
					udp::resolver::query query(udp::v4(), "localhost",
							_str_service);
					//boost::system::error_code& c_error;
					udp::resolver::iterator iterator = resolver.resolve(query);
					_c_remote_endpoint = boost::asio::ip::udp::endpoint(
							boost::asio::ip::address_v4::broadcast(),
							iterator->endpoint().port());
				} catch (...) {
					udp::resolver::query query(udp::v4(), "localhost",
							_str_service,
							boost::asio::ip::resolver_query_base::all_matching
									| boost::asio::ip::resolver_query_base::passive);
					udp::resolver::iterator iterator = resolver.resolve(query);
					_c_remote_endpoint = iterator->endpoint();
				}

//				_FATAL << "BROARDCAST: " << c_endpoint;
			} else {
				_DBG << "Resolving host: " << _str_dst << ":" << _str_service;
				udp::resolver resolver(_c_io_service);
				udp::resolver::query query(udp::v4(), _str_dst, _str_service,
						boost::asio::ip::resolver_query_base::all_matching
								| boost::asio::ip::resolver_query_base::passive);
				udp::resolver::iterator iterator = resolver.resolve(query);

				_c_remote_endpoint = iterator->endpoint();
			}
		}
	} catch (std::exception const & e) {
		_FATAL << e.what();
	}

	//_DBG << "Binding";

	/* Launch read */
	//_DBG << "START READING BIND";
	f_async_read();
}

void CT_UDP::on_timeout(boost::optional<boost::system::error_code>* a,
		const boost::system::error_code& in_c_error) {
	if (!in_c_error) {
		a->reset(in_c_error);
		//_DBG << "Timout cb";
	} else {
		//_CRIT << "Timer error : " << in_c_error.message();
	}

}

int CT_UDP::f_run(CT_HOST_CONTEXT&) {
	int ec;
//static int i_loop = 0;

	boost::optional<boost::system::error_code> timer_result;
	boost::asio::deadline_timer c_timer(_c_io_service,
			boost::posix_time::milliseconds(500));
	//_DBG << _V(this) << " Timer Start";
	c_timer.async_wait(
			boost::bind(&CT_UDP::on_timeout, this, &timer_result, _1));
	boost::system::error_code ec_boost;

	while (1) {


		/* Run one */
		if (!_c_io_service.run_one(ec_boost)) {
			//_DBG << _V(this)<< " Reset IO SERVICE";
			_c_io_service.reset();
			continue;
		}
		if (ec_boost) {
			_CRIT << _V(this)<< " Run error : " << ec_boost.message();
		}
		//_DBG << _V(this) << " Timer Stop";
		//D("Timer STOP 2");
		if (timer_result || CT_HOST::host->f_context_is_idling()) {
			//_WARN << _V(this) << " TIMEOUT";
			ec = EC_BYPASS;
			break;
		} else {
			//_WARN << _V(this)<< " Processing context";
			if (_pc_context->f_has_extra()) {
				//_WARN << _V(this)<< " Processing extra" << _V(_pc_context->f_size_extra());
				ec = EC_SUCCESS;
				break;
			}

		}
	}
	return ec;
}

void CT_UDP::f_sync_send(CT_GUARD<CT_PORT_NODE> const & in_c_node) {
	int ec;
	CT_GUARD_LOCK c_guard(_c_msg_lock);
	//_DBG << "Sync Send START";
	M_ASSERT(_pc_buffer_snd != NULL);

	/** Write node to buffer */
	node_buffer_writer<CT_GUARD> c_writer(_pc_buffer_snd, _sz_buffer);
	//_DBG << _V((void*) _pc_buffer)<< _V(_sz_buffer) << _V(pc_node->get_size());
	if(_b_raw_mode) {
		ec = in_c_node->to_writer_raw(c_writer);
	} else {
		ec = in_c_node->to_writer(c_writer);
	}

	if (ec != EC_BML_SUCCESS) {
		FATAL("Unable to convert message to buffer");
		ec = EC_FAILURE;
		goto out_err;
	}

	M_ASSERT(c_writer.get_offset());
	//_DBG << "Sending " << c_writer.get_offset() << " bytes";
	{
		boost::system::error_code ec_boost;
		size_t sz_tmp = _c_socket.send_to(
				boost::asio::buffer(_pc_buffer_snd, c_writer.get_offset()),
				_c_remote_endpoint, 0, ec_boost);
		if (ec_boost) {
			_CRIT << "Failed to send message:" << ec_boost.message();
		} else if (sz_tmp != (size_t) c_writer.get_offset()) {
			_CRIT << "Whole message not sent";
		}
	}
	out_err:
	//_DBG << "Sync Send STOP";
	return;
}

void CT_UDP::f_async_send(void) {
	int ec;
	_DBG << "ASync Send START";
	M_ASSERT(_pc_buffer_snd != NULL);
	M_ASSERT(_l_msg.size());
	/* Get pc_node */
	CT_GUARD<CT_PORT_NODE> & pc_node = _l_msg.front();

	/** Write node to buffer */
	node_buffer_writer<CT_GUARD> c_writer(_pc_buffer_snd, _sz_buffer);
	//_DBG << _V((void*) _pc_buffer)<< _V(_sz_buffer) << _V(pc_node->get_size());
	if(_b_raw_mode) {
		ec = pc_node->to_writer_raw(c_writer);
	} else {
		ec = pc_node->to_writer(c_writer);
	}
	if (ec != EC_BML_SUCCESS) {
		FATAL("Unable to convert message to buffer");
		ec = EC_FAILURE;
		goto out_err;
	}

	M_ASSERT(c_writer.get_offset());

	_DBG << "Sending " << c_writer.get_offset() << " bytes";
	_c_socket.async_send_to(
			boost::asio::buffer(_pc_buffer_snd, c_writer.get_offset()),
			_c_remote_endpoint,
			boost::bind(&CT_UDP::on_send, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
	out_err:
	_DBG << "ASync Send STOP";

	return;
}
void CT_UDP::on_send(const boost::system::error_code& error,
		size_t in_i_bytes_transferred) {
	CT_GUARD_LOCK c_guard(_c_msg_lock);
	if (!error) {
		_l_msg.pop_front();
		//_DBG << "Message has been sent";
		if (!_l_msg.empty()) {
			f_async_send();
		}
	} else {
		_l_msg.clear();
		_c_socket.close();
		_CRIT << "An error has occurred during send: " << error.message();
	}
}

void CT_UDP::on_post_async_send(CT_GUARD<CT_PORT_NODE> const in_pc_msg) {
	CT_GUARD_LOCK c_guard(_c_msg_lock);
	bool b_inactive = _l_msg.empty();
	//_WARN << "Sending node:" << in_pc_msg->get_id() << _V(b_inactive);
	_l_msg.push_back(in_pc_msg);
	if (b_inactive) {
		f_async_send();
	}
}

int CT_UDP::f_send(CT_GUARD<CT_PORT_NODE> const & in_c_node) {
	int ec;

	//_WARN << "Sending node:" << in_c_node->get_id();

	/* post node on io service */
	if (_b_server_mode) {
		_c_io_service.post(
				boost::bind(&CT_UDP::on_post_async_send, this, in_c_node));
	} else {
		f_sync_send(in_c_node);
	}
	ec = EC_SUCCESS;
	return ec;

}

int CT_UDP::parse_data(char* in_pc_buffer, size_t in_sz_buffer) {
	int ec;
#if 0
	_DBG << " Request " << in_sz_buffer << " bytes";

	_DBG << _V(_sz_buffer)<< _V(_i_offset_read) << _V(_sz_available)<< _V(_i_offset_parser);
	//M_ASSERT(_sz_buffer >= _i_offset_read + _i_offset_parser + _sz_available);
#endif
	M_ASSERT(_i_offset_parser <= _sz_available);
	M_ASSERT(_sz_buffer >= _i_offset_read + _sz_available);

	while (in_sz_buffer + _i_offset_parser > _sz_available) {
		ec = f_sync_read();
		if (ec != EC_SUCCESS) {
			return ec;
		}
	}

	M_ASSERT(_i_offset_parser <= _sz_available);
	M_ASSERT(_sz_buffer >= _i_offset_read + _sz_available);

	/* Copy from buffer */
	memcpy(in_pc_buffer, &_pc_buffer_rcv[_i_offset_read + _i_offset_parser],
			in_sz_buffer);

#if 0
	for (uint i = 0; i < in_sz_buffer; i++) {
		printf("%02x", (unsigned char)in_pc_buffer[i]);
	}
	printf("\n");
#endif
	_i_offset_parser += in_sz_buffer;
#if 0
	_DBG << _V(_sz_buffer)<< _V(_i_offset_read) << _V(_sz_available)<< _V(_i_offset_parser);
#endif
	M_ASSERT(_i_offset_parser <= _sz_available);
	M_ASSERT(_sz_buffer >= _i_offset_read + _sz_available);
	return EC_BML_SUCCESS;
}

void CT_UDP::f_async_read() {
#if 0
	size_t sz_remain = _sz_buffer - _i_offset_read - _sz_available;
	_DBG << "Async Read START";
	_DBG << _V(_sz_buffer)<< _V(_i_offset_read) << _V(_sz_available) << _V(sz_remain);
	if (!sz_remain) {
		if (!_i_offset_read) {
			_CRIT<< "Buffer is full";
			//_i_offset_read_tmp = 0;
			_i_offset_read = 0;
			_sz_available = 0;
		} else {
			/* Check remaining size */
			_DBG << "Copying ASYNC " << _sz_available << " bytes";

			memmove(&_pc_buffer_rcv[0], &_pc_buffer_rcv[_i_offset_read],
					_sz_available);
			_i_offset_read = 0;
			sz_remain = _sz_buffer - _i_offset_read - _sz_available;
		}
	}

	{
		boost::asio::socket_base::bytes_readable command(true);
		_c_socket.io_control(command);
		std::size_t bytes_readable = command.get();
		_DBG << _V(bytes_readable);
	}
	_c_socket.async_receive_from(
			boost::asio::buffer(&_pc_buffer_rcv[_i_offset_read + _sz_available],
					M_MIN(sz_remain,512)), _c_remote_endpoint,
			boost::bind(&CT_UDP::on_read, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));

	_DBG << "Async Read STOP";
#else
	_c_socket.async_receive(boost::asio::null_buffers(),
			boost::bind(&CT_UDP::on_received, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
#endif

}

int CT_UDP::f_sync_read(void) {
	int ec;
	size_t sz_remain = _sz_buffer - _i_offset_read - _sz_available;
	//if (_c_socket.available() > 4096) {
	//_DBG << "Socket available :" << _c_socket.available();
	//}
#if 0
	_DBG << "Sync Read START";

	_DBG << _V(_sz_buffer)<< _V(_i_offset_read) << _V(_sz_available)<< _V(_i_offset_parser) << _V(sz_remain);
#endif
	M_ASSERT(_i_offset_parser <= _sz_available);
	M_ASSERT(_sz_buffer >= _i_offset_read + _sz_available);
	if (sz_remain < _c_socket.available()) {
		if (!_i_offset_read) {
			_CRIT << "Buffer is full";
			//_i_offset_read_tmp = 0;
			_i_offset_read = 0;
			_sz_available = 0;
			ec = EC_FAILURE;
			goto out_err;
		} else {
			/* Check remaining size */
			if (_sz_available) {
				_DBG << "Copying sync " << _sz_available << " bytes"
						<< _V(sz_remain)<< _V(_c_socket.available());

						memmove(&_pc_buffer_rcv[0], &_pc_buffer_rcv[_i_offset_read],
					_sz_available);
			}
				_i_offset_read = 0;
				sz_remain = _sz_buffer - _i_offset_read - _sz_available;

#if 0
				for (uint i = 0; i < _sz_available; i++) {
					printf("%02x", (unsigned char) _pc_buffer_rcv[i]);
				}
				printf("\n");
#endif
			}
		}

	if (!sz_remain) {
		ec = EC_BML_OVERFLOW;
		goto out_err;
	}
#if 0
	{
		boost::asio::socket_base::bytes_readable command(true);
		_c_socket.io_control(command);
		std::size_t bytes_readable = command.get();
		_DBG << _V(bytes_readable) << _V(sz_remain);
	}
#endif
	{

		/* sync read within node */
		boost::system::error_code in_c_error;
		size_t sz_read = _c_socket.receive_from(
				boost::asio::buffer(
						&_pc_buffer_rcv[_i_offset_read + _sz_available],
						sz_remain), _c_remote_endpoint, 0, in_c_error);
		//printf("NEW %p %d\n", &_pc_buffer_recv[_i_offset_read + _sz_available], (int)sz_read);

		if (in_c_error) {
			_CRIT << "Error during sync read :" << in_c_error.message();
			ec = EC_FAILURE;
			goto out_err;
		}
#if 0
		{
			boost::asio::socket_base::bytes_readable command(true);
			_c_socket.io_control(command);
			std::size_t bytes_readable = command.get();
			_DBG << _V(bytes_readable);
		}
#endif
#if 0
		_DBG << " Received " << sz_read << " bytes";
#endif

#if 0
		for (uint i = 0; i < sz_read; i++) {
			printf("%02x",
					(unsigned char) _pc_buffer_rcv[_i_offset_read
					+ _sz_available + i]);
		}
		printf("\n");
#endif
		_sz_available += sz_read;
	}

	ec = EC_SUCCESS;
	out_err:
#if 0
	_DBG << "Sync Read STOP";
#endif
	return ec;
}

void CT_UDP::on_received(const boost::system::error_code& error, unsigned int) {
	// Standard check: avoid processing anything if operation was canceled
	// Usually happens when closing the socket
	if (error) {
		_CRIT << "An error has occurred :" << error.message();
		return;
	}

	_i_offset_parser = 0;
	f_sync_read();

	while (_sz_available) {
		//uint64_t i_time_start = f_get_time_ns64();

		CT_PORT_NODE_GUARD c_node;

		//uint64_t i_time_alloc_end = f_get_time_ns64();
		//uint64_t i_time_parser_end;
		//uint64_t i_time_send_end;
		/* Reset parser offset */
		_i_offset_parser = 0;
		/* Trying to decode buffer */
		try {
			int ec;
			if(_b_raw_mode) {
				ec = c_node->from_parser_raw(*this);
			} else {
				ec = c_node->from_parser(*this);
			}


			//i_time_parser_end = f_get_time_ns64();
			/* Decoder needs more data */
			if (ec == EC_BML_NODATA) {
				_WARN << "Sync timeout launch async read";
				break;
				/* Error during decoding */
			} else if (ec != EC_SUCCESS) {
				_CRIT << "Unable to handle received data, reseting";
				_i_offset_read = 0;
				_sz_available = 0;
				break;
				/* Decoding success*/
			} else {
				//std::cout << *c_node << std::endl;
				f_received(c_node);
				//i_time_send_end = f_get_time_ns64();

				M_ASSERT(_i_offset_parser <= _sz_available);
				_i_offset_read += _i_offset_parser;
				_sz_available -= _i_offset_parser;
#if 0
				_DBG << "Read " << _i_offset_parser << "bytes";
#endif
			}
		} catch (std::exception const & e) {
			_CRIT << e.what();
			_i_offset_read = 0;
			_sz_available = 0;
			break;
		}

		/* Force pause when idling  */
		if(CT_HOST::host->f_context_is_idling()) {
			break;
		}
		//uint64_t i_time_end = f_get_time_ns64();
		//if(float(i_time_end - i_time_start)/1e6 > 1) {
		//		_DBG << "Time udp :" << float(i_time_end - i_time_start)/1e6 << " ms "  << float(i_time_alloc_end - i_time_start)/1e6 << " ms " << float(i_time_parser_end - i_time_alloc_end)/1e6 << " ms " << float(i_time_send_end-i_time_parser_end)/1e6 << " ms "<< _i_offset_parser << " bytes";
		//}
	}

	/* Triggering new read */
	//_i_offset_parser = 0;
	//f_sync_read();
	f_async_read();

}
#if 0
void CT_UDP::on_read(const boost::system::error_code& error,
		size_t in_i_bytes_transferred) {
	int ec;

	{
		boost::asio::socket_base::bytes_readable command(true);
		_c_socket.io_control(command);
		std::size_t bytes_readable = command.get();
		_DBG << _V(bytes_readable)<< _V(_c_socket.available());
	}

#if 1
	for (uint i = 0; i < in_i_bytes_transferred; i++) {
		printf("%02x",
				(unsigned char) _pc_buffer_rcv[_i_offset_read + _sz_available
				+ i]);
	}
	printf("\n");
#endif

	_DBG << "ON Read START";
	if (!error) {
		_DBG << "Received " << in_i_bytes_transferred << " bytes";

		M_ASSERT(_pc_buffer_rcv);
		//_DBG << "Available: " << _sz_available << " bytes";
		_sz_available += in_i_bytes_transferred;

		while (_sz_available) {

			CT_PORT_NODE_GUARD c_node;

			/* Reset parser offset */
			_i_offset_parser = 0;
			/* Trying to decode buffer */
			try {
				ec = c_node->from_parser(*this);

				/* Decoder needs more data */
				if (ec == EC_BML_NODATA) {
					_WARN << "Sync timeout launch async read";
					break;
					/* Error during decoding */
				} else if (ec != EC_SUCCESS) {
					_CRIT << "Unable to handle received data, reseting";
					_i_offset_read = 0;
					_sz_available = 0;
					break;
					/* Decoding success*/
				} else {
					//std::cout << *c_node << std::endl;
					f_received(c_node);

					M_ASSERT(_i_offset_parser <= _sz_available);
					_i_offset_read += _i_offset_parser;
					_sz_available -= _i_offset_parser;
					_DBG << "Read " << _i_offset_parser << "bytes";
				}
			} catch (std::exception const & e) {
				_CRIT << e.what();
				_i_offset_read = 0;
				_sz_available = 0;
				break;
			}
		}

		/* Triggering new read */
		//_i_offset_parser = 0;
		//f_sync_read();
		f_async_read();
		ec = EC_SUCCESS;
	} else {
		_CRIT << "An error has occurred !";
		ec = EC_FAILURE;
	}
#if 0
	out_err: if (ec == EC_FAILURE) {
		_CRIT<< "An error has occurred !";
		f_notify_link_failure();
	}
#endif
	_DBG << "ON Read STOP";
}
#endif

void CT_UDP::f_notify_link_failure(void) {
	/* Unuse all ongoing message */
	_l_msg.clear();
	_DBG << "TODO LINK FAILURE";
}
void CT_UDP::f_notify_link_established(void) {
	_DBG << "TODO LINK ESTABLISHED";
}
M_PLUGIN_TRANSPORT(CT_UDP, "UDP", M_STR_VERSION);
