/***********************************************************************
 ** tcp.cc
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
 * @file tcp.cc
 * Plugin tcp.
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
#include <tcp.hh>
#include "api.hh"
#include <sstream>

using namespace std;
using namespace master::plugins::transport::tcp;
using namespace bml;

CT_TCP_SESSION::CT_TCP_SESSION(CT_TCP& in_c_server) :
    node_parser<CT_GUARD>(), _c_socket(in_c_server.f_get_service()), _c_server(
        in_c_server) {

  _i_offset_read = 0;
  _i_offset_parser = 0;
  //_sz_buffer = C_PORT_NODE_BUFFER_SIZE;
  //_pc_buffer_recv = NULL;
  _v_buffer_recv.resize(C_PORT_NODE_BUFFER_SIZE);
  _v_buffer_send.resize(10*C_PORT_NODE_BUFFER_SIZE);
  //_pc_buffer_send = NULL;
  //_pc_buffer_send = new char[_sz_buffer];
  _b_connected = false;
  _b_connect_in_progress = false;
  _sz_available = 0;
//#define LF_DEBUG_TCP
#ifdef LF_DEBUG_TCP
  _WARN << "TCP server :" << &_c_server << " " << this;
#endif

  //if (!_pc_buffer_send) {
  //  throw std::bad_alloc();
  //}
  /* Set recv timeout */
#if defined OS_WINDOWS
  int32_t timeout = 1000;
  #if BOOST_VERSION < 104700
  setsockopt(_c_socket.native(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
  setsockopt(_c_socket.native(), SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
  #else
  setsockopt(_c_socket.native_handle(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
  setsockopt(_c_socket.native_handle(), SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
  #endif
#else
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 250000;
  #if BOOST_VERSION < 104700
  setsockopt(_c_socket.native(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  setsockopt(_c_socket.native(), SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
  #else
  setsockopt(_c_socket.native_handle(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  setsockopt(_c_socket.native_handle(), SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
  #endif
#endif

  if(_c_server.f_get_config().has("ttl")) {
    char i_ttl = uint64_t(*_c_server.f_get_config().get("ttl"));

    #if BOOST_VERSION < 104700
    setsockopt(_c_socket.native(), IPPROTO_IP, IP_TTL, &i_ttl, sizeof(i_ttl));
    #else
    setsockopt(_c_socket.native_handle(), IPPROTO_IP, IP_TTL, &i_ttl, sizeof(i_ttl));
    #endif
     _DBG << "setting ttl:" << (int) i_ttl;
  }
}

CT_TCP_SESSION::~CT_TCP_SESSION() {
#if 1
#if 1
  {
    boost::system::error_code error;
    _c_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
    if (error) {
      _CRIT << error.message();
    }
  }
  {
    boost::system::error_code error;
    _c_socket.cancel(error);
    if (error) {
      _CRIT << error.message();
    }
  }
#endif
  {
    boost::system::error_code error;
    _c_socket.close(error);
    if (error) {
      _CRIT << error.message();
    }
  }
#else
  boost::system::error_code shutdownErrorCode;
  boost::system::error_code closeErrorCode;
  try {

    socket.shutdown(boost::asio::socket_base::shutdown_both,
        shutdownErrorCode);
    socket.close(closeErrorCode);
  }
  //never caught:
  catch (std::exception& e) {
    _CRIT << "socket.shutdown() or socket.close() failed with exception"
    << e.what();
  }

  if (shutdownErrorCode.value() != 0) {
    //Linux shows this "Transport endpoint is not connected" pretty frequently so mute it on it:
#if !defined(LINUX)
    printf("socket shutdown failed with \"%s\".\n",
        shutdownErrorCode.message());
#endif
  }
  if (closeErrorCode.value() != 0) {
    //never printed:
    printf("socket close failed with \"%s\".\n", closeErrorCode.message());
  }
#endif

  _WARN << "~TCP server :" << &_c_server << " " << this << " " << _V(_str_address_full);

}

void CT_TCP_SESSION::f_set_endpoint(std::string const & in_str_address,
    std::string const & in_str_service) {
  _str_address = in_str_address;
  _str_service = in_str_service;
}

void CT_TCP_SESSION::f_start() {
  _c_remote = _c_socket.remote_endpoint();
  _str_address = _c_remote.address().to_string();
  _i_port = _c_remote.port();
  _b_connected = true;
  _str_address_full = _str_address + std::string(":")
      + f_string_format("%d", _i_port);

  //_WARN << "Session started from " << _str_address << ":" << _i_port;
  //TODO port notification CONNECTED

  f_notify_link_established();

  /* Initiate read */
  f_async_read();
}

void CT_TCP_SESSION::f_async_read(void) {
  //int ec;
  //M_ASSERT(_i_offset_parser == 0);
  size_t sz_buffer = _v_buffer_recv.size();
  size_t sz_remain = sz_buffer - _i_offset_read - _sz_available;
  //_DBG << _V(_sz_buffer)<< _V(_i_offset_read) << _V(_sz_available)<< _V(sz_remain);

  if (!sz_remain) {
    if (!_i_offset_read) {
      _CRIT << "Buffer is full";
      //_i_offset_read_tmp = 0;
      _i_offset_read = 0;
      _sz_available = 0;
    } else {
      /* Check remaining size */
      _DBG << "Copying ASYNC " << _sz_available << " bytes";
      memmove(&_v_buffer_recv[0], &_v_buffer_recv[_i_offset_read],
          _sz_available);
      _i_offset_read = 0;
      sz_remain = sz_buffer - _i_offset_read - _sz_available;
    }
  }

  /* Async read between nodes -> Node parse on completion */
  _c_socket.async_read_some(
      boost::asio::buffer(
          &_v_buffer_recv[_i_offset_read + _sz_available],
          sz_remain),
      boost::bind(&CT_TCP_SESSION::on_read, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));

}

int CT_TCP_SESSION::f_sync_read(void) {
  int ec;
  size_t sz_buffer = _v_buffer_recv.size();
  size_t sz_remain = sz_buffer - _i_offset_read - _sz_available;
  //_DBG << _V(_sz_buffer)<< _V(_i_offset_read) << _V(_sz_available)<< _V(sz_remain);

  if (!sz_remain) {
    if (!_i_offset_read) {
      if(sz_buffer < 512*1024*1024) {
        _WARN << "Buffer is full, Allocating more data" <<_V(sz_buffer);
        _v_buffer_recv.resize(2*sz_buffer);
        sz_buffer = _v_buffer_recv.size();
        sz_remain = sz_buffer - _i_offset_read - _sz_available;
      } else {
        _CRIT << "Buffer is full " <<_V(sz_buffer);
        //_i_offset_read_tmp = 0;
        _i_offset_read = 0;
        _sz_available = 0;
        ec = EC_FAILURE;
        goto out_err;
      }
    } else {
      /* Check remaining size */
      //_DBG << "Copying SYNC " << _sz_available << " bytes";
      memmove(&_v_buffer_recv[0], &_v_buffer_recv[_i_offset_read],
          _sz_available);
      _i_offset_read = 0;
      sz_remain = sz_buffer - _i_offset_read - _sz_available;
    }
  }

  if (!sz_remain) {
    ec = EC_BML_OVERFLOW;
    goto out_err;
  }

  {

    /* sync read within node */
#if 0
    boost::system::error_code in_c_error;
    size_t sz_read = _c_socket.read_some(
        boost::asio::buffer(
            &_v_buffer_recv[_i_offset_read + _sz_available],
            sz_remain), in_c_error);
    if (in_c_error) {
      _CRIT << "Error during sync read :" << in_c_error.message();
      ec = EC_FAILURE;
      goto out_err;
    }
#endif


    size_t sz_current = _sz_available;
    _c_socket.async_read_some(
        boost::asio::buffer(
            &_v_buffer_recv[_i_offset_read + _sz_available],
            sz_remain),
        boost::bind(&CT_TCP_SESSION::on_read_sync, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));



    while (sz_current == _sz_available) {
      boost::system::error_code ec_boost;

      /* Run one */
#if BOOST_VERSION < 107000
      auto & refIO = _c_socket.get_io_service();
#else
      boost::asio::io_context& refIO = static_cast<boost::asio::io_context&>(_c_socket.get_executor().context());
#endif
      if(!refIO.poll_one(ec_boost)) {
        //_DBG << _V(this)<< " Reset IO SERVICE";
        usleep(0);
      }
      if (ec_boost) {
        _CRIT << _V(this)<< " Run error : " << ec_boost.message();
      }
    }

    //printf("NEW %p %d\n", &_pc_buffer_recv[_i_offset_read + _sz_available], (int)sz_read);

#ifdef LF_DEBUG_TCP
    //_DBG << " Received " << sz_read << " bytes";
#endif
    if (_ec_sync_boost) {
      _CRIT << "Error during sync read :" << _ec_sync_boost.message();
      ec = EC_FAILURE;
      goto out_err;
    }
  }

  ec = EC_SUCCESS;
  out_err: return ec;
}

int CT_TCP_SESSION::parse_data(char* in_pc_buffer, size_t in_sz_buffer) {
  int ec;
#ifdef LF_DEBUG_TCP
  _DBG << " Request " << in_sz_buffer << " bytes";
#endif

  M_ASSERT(_i_offset_parser <= _sz_available);
  M_ASSERT(_v_buffer_recv.size() >= _i_offset_read + _sz_available);

  while (in_sz_buffer + _i_offset_parser > _sz_available) {
    ec = f_sync_read();
    if (ec != EC_SUCCESS) {
      return ec;
    }
  }

  M_ASSERT(_i_offset_parser <= _sz_available);
  M_ASSERT(_v_buffer_recv.size() >= _i_offset_read + _sz_available);

  /* Copy from buffer */
  memcpy(in_pc_buffer, &_v_buffer_recv[_i_offset_read + _i_offset_parser],
      in_sz_buffer);

  for (uint i = 0; i < in_sz_buffer; i++) {
    //printf("%p %02x\n",&_pc_buffer_recv[_i_offset_read + _i_offset_parser], in_pc_buffer[i]);
  }

  _i_offset_parser += in_sz_buffer;

  M_ASSERT(_i_offset_parser <= _sz_available);
  M_ASSERT(_v_buffer_recv.size() >= _i_offset_read + _sz_available);

  return EC_BML_SUCCESS;
}

void CT_TCP_SESSION::f_send(CT_GUARD<CT_PORT_NODE> const & in_pc_node) {
  CT_GUARD_LOCK c_guard(_c_msg_lock);
  bool b_inactive = _l_msg.empty();
  _l_msg.push_back(in_pc_node);
  //_DBG << this<< " Pushing " <<  _l_msg.empty() << b_inactive;
  if (b_inactive) {
    //  _DBG << "Send " <<  _l_msg.empty() << b_inactive;
    f_send();
  }
}

void CT_TCP_SESSION::f_send(void) {
  int ec;
  //M_ASSERT(_pc_buffer_send != NULL);
  M_ASSERT(_l_msg.size());
  /* Get pc_node */
  CT_GUARD<CT_PORT_NODE> & pc_node = _l_msg.front();

  //_DBG << "Send " << std::hex << pc_node->get_id();
  size_t sz_whole = pc_node->get_whole_size();
  //_DBG <<_V(sz_whole);/////////////////////////////////////////////////////////////////////////////////////////////////////////

  if(_v_buffer_send.size() <= sz_whole) {
    size_t sz_buffer = (sz_whole*3)/2;
    _WARN << "Reallocating buffer from " << _v_buffer_send.size() << " to " << _V(sz_buffer) << " bytes";
    _v_buffer_send.resize(sz_buffer);
  }

  M_ASSERT(_v_buffer_send.size() > pc_node->get_size());
  M_ASSERT(_v_buffer_send.size() == _v_buffer_send.size());

  /** Write node to buffer */
  node_buffer_writer<CT_GUARD> c_writer(&_v_buffer_send[0], _v_buffer_send.size());
  //_DBG << _V((void*) _pc_buffer_send)<< _V(_sz_buffer) << _V(pc_node->get_size())<< _V(pc_node->usage());
  ec = pc_node->to_writer(c_writer);
  if (ec != EC_BML_SUCCESS) {
    if (ec == EC_BML_OVERFLOW) {
      CRIT("Buffer overflow");
    }
    FATAL("Unable to convert message to buffer");
    ec = EC_FAILURE;
    goto out_err;
  }

  //std::cout << *pc_node << std::endl;
  //memset(&_pc_buffer_send[c_writer.get_offset()], 0,
  //    _sz_buffer - c_writer.get_offset());
#if 0
  {

    node_buffer_parser<CT_GUARD> c_parser(_pc_buffer_send, c_writer.get_offset());
    CT_PORT_NODE_GUARD c_node;

    c_node->from_parser(c_parser);
    _DBG << *c_node;
    _DBG << *pc_node;
  }
#endif
  M_ASSERT(c_writer.get_offset());

  //_DBG <<_V(pc_node->get_whole_size()) <<_V(c_writer.get_offset()) <<_V(sz_whole);////////////////////////////////////////////////////////////////////////////
  M_ASSERT(sz_whole == (size_t)c_writer.get_offset());

  {
#ifdef LF_DEBUG_TCP
    _DBG << "Sending " << c_writer.get_offset() << " bytes";
#endif
#if 0
    for(int i=0; i<c_writer.get_offset(); i++) {
      printf("%02x", _pc_buffer_send[i]);
    }
    printf("\n");
#endif
    boost::asio::async_write(_c_socket,
        boost::asio::buffer(&_v_buffer_send[0], c_writer.get_offset()),
        boost::bind(&CT_TCP_SESSION::on_send, this,
            boost::asio::placeholders::error));
  }

  out_err:
#ifdef LF_DEBUG_TCP
  _DBG << "end send";
#endif
  return;

}

void CT_TCP_SESSION::f_disconnect(void) {

  if (_b_connected) {
    f_notify_link_failure();
  }
  _b_connected = false;
  _c_server.f_session_remove(this);
}

void CT_TCP_SESSION::on_send(const boost::system::error_code& error) {
  CT_GUARD_LOCK c_guard(_c_msg_lock);
  if (!error) {
#ifdef LF_DEBUG_TCP
    _DBG << "Message has been sent" << _l_msg.empty();
#endif
    _l_msg.pop_front();
    if (!_l_msg.empty()) {
      //_DBG << "Empty ";
      f_send();
    }
  } else {
    _WARN << this;
    _CRIT << "An error has occurred during send: " << error.message();
    // DO NOT ERASE OBJECT ON SEND ERROR, RECV EVENT REMAINS AFTER CLOSE!!!
    _c_socket.close();
    _l_msg.clear();
    //f_disconnect();
  }
}

void CT_TCP_SESSION::on_read_sync(const boost::system::error_code& error,
    size_t bytes_transferred) {
  _ec_sync_boost = error;
  if (!error) {
    _sz_available += bytes_transferred;
  }
}

void CT_TCP_SESSION::on_read(const boost::system::error_code& error,
    size_t bytes_transferred) {

  int ec;

  if (boost::asio::error::eof == error) {
    //_WARN << this;
    _WARN << "Client closed connection:" << error.message();
    ec = EC_FAILURE;
  } else if (boost::asio::error::connection_reset == error) {
    //_WARN << this;
    _WARN << "Client disconnect:" << error.message();
    //f_disconnect();
    ec = EC_FAILURE;
  } else if (!error) {
#if 0
    _DBG << "Received " << bytes_transferred << " bytes";
#endif
#if 0
    {
      for(int i=0; i<c_writer.get_offset(); i++) {
        printf("%02x", _pc_buffer_recv[i]);
        _pc_buffer_recv[_i_offset_read
      }
      printf("\n");
    }
#endif
    {
      M_ASSERT(_v_buffer_recv.size());

      //size_t i_read;
      //_DBG << "Available: " << _sz_available << " bytes";
      _sz_available += bytes_transferred;

      while (_sz_available) {
        //_DBG << "Available: " << _sz_available << " bytes";

        CT_PORT_NODE_GUARD c_node;

        /* Reset parser offset */
        _i_offset_parser = 0;
        /* Trying to decode buffer */
        try {
          node_parser<CT_GUARD> * pc_tmp = this;
          node_parser<CT_GUARD> &c_tmp = *pc_tmp;
          ec = c_node->from_parser(c_tmp);
          /* Decoder needs more data */
          if (ec == EC_BML_NODATA) {
            _WARN << "Sync timeout launch async read";
            break;
            /* Error during decoding */
          } else if (ec != EC_SUCCESS) {
            _CRIT << "Unable to handle received data";
            _i_offset_read = 0;
            _sz_available = 0;
            ec = EC_FAILURE;
            goto out_err;

            /* Decoding success*/
          } else {
            //_DBG << "Message read !!!: " << i_read << " bytes";
            if (c_node->get_id() == (port_node_id_t) E_ID_CONNECT) {
              c_node->get(E_ID_SESSION_INFO, 0, true)->set_data<
                  std::string>(f_get_address_full());
            }
            //std::cout << *c_node << std::endl;
            _c_server.f_received(c_node);

            M_ASSERT(_i_offset_parser <= _sz_available);
            _i_offset_read += _i_offset_parser;
            _sz_available -= _i_offset_parser;
            //_DBG << "Read " << _i_offset_parser << "bytes";
          }
        } catch (std::exception const & e) {
          _CRIT << e.what();
          _i_offset_read = 0;
          _sz_available = 0;
          ec = EC_FAILURE;
          goto out_err;
        }
      }

      /* Triggering new async read */
      f_async_read();
    }

    ec = EC_SUCCESS;
  } else {
    _CRIT << "An error has occurred :" << error.message();
    ec = EC_FAILURE;
  }
  out_err: if (ec == EC_FAILURE) {
    _CRIT << "An error has occurred !";
    f_disconnect();
  }
}
#if 0
void CT_TCP_SESSION::on_timeout(
    boost::optional<boost::system::error_code>* in_res,
    boost::system::error_code in_ec) {
  _DBG << "Timeout SESSION";
}
#endif

void CT_TCP_SESSION::f_notify_link_failure() {
  CT_PORT_NODE_GUARD c_guard(master::plugins::transport::E_ID_DISCONNECTED);
  c_guard->get(E_ID_SESSION_INFO, 0, true)->set_data<std::string>(
      f_get_address_full());
  _c_server.f_session_notify(c_guard);
}
void CT_TCP_SESSION::f_notify_link_established(void) {
  CT_PORT_NODE_GUARD c_guard(master::plugins::transport::E_ID_CONNECTED);
  c_guard->get(E_ID_SESSION_INFO, 0, true)->set_data<std::string>(
      f_get_address_full());
  _c_server.f_session_notify(c_guard);
}

CT_TCP::CT_TCP(void) :
    CT_TRANSPORT(), _c_io_service(), _c_socket(_c_io_service), _c_acceptor(
        _c_io_service) {

  _b_server_mode = false;

  _pc_context = NULL;
  _i_port = 0;

  _b_support_linked_transport = false;
  _b_linked_transport = false;
  _pc_tmp_session = NULL;
}

CT_TCP::~CT_TCP() {
//  _WARN << "Deleting TCP :" << this;

  /* Stop service */
  _c_io_service.stop();
  _c_socket.close();

  /* Stop and join context */
  if (_pc_context) {
    delete _pc_context;
  }

  /* Delete ongoing sessions */
  for (std::list<CT_TCP_SESSION*>::iterator pc_it = _l_sessions.begin();
      pc_it != _l_sessions.end(); pc_it++) {
    delete *pc_it;
  }
  if (_pc_tmp_session) {
    delete _pc_tmp_session;
  }
}

int CT_TCP::f_apply_config(std::string & in_str_url) {
  int ec;
  M_ASSERT(!_pc_context);
  /** Create context */
  _pc_context = CT_HOST::host->f_new_context();
  M_ASSERT(_pc_context);

  /* Erase previous message queue */
  std::vector<std::string> _v_str = f_string_split(in_str_url, ":");

  /* Store queue name */
  if (_v_str.size() < 2) {
    _CRIT << "TCP url is incorrect: " << in_str_url;
  }

  /* Destination address or server mode */
  {
    _str_dst = _v_str[0];
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
    _str_service = _v_str[1];
  }

  /* Start waiting connection or establishing connection according to mode */
  if (_b_server_mode) {
    f_bind();
  } else {
    f_connect();
  }

  /** Start IO service context */
  ec = _pc_context->f_start(M_CONTEXT_BIND(CT_TCP,f_run,this));
  if (ec != EC_SUCCESS) {
    CRIT("Unable to start context");
    ec = EC_FAILURE;
    goto out_err;
  }
  ec = EC_SUCCESS;
  out_err: return ec;
}

void CT_TCP::f_session_remove(CT_TCP_SESSION * in_pc_session) {
  CT_GUARD_LOCK c_guard(_c_lock);

  _DBG << "Deleting session: " << in_pc_session;

  /* Remove from client map */
  M_ASSERT(_m_clients.erase(in_pc_session->f_get_address_full()));
  _l_sessions.remove(in_pc_session);
  delete in_pc_session;
}
void CT_TCP::f_session_add(CT_TCP_SESSION * in_pc_session) {
  CT_GUARD_LOCK c_guard(_c_lock);

  _l_sessions.push_back(in_pc_session);

  /* Start session */
  in_pc_session->f_start();

  /* Add to client map */
  _m_clients[in_pc_session->f_get_address_full()] = in_pc_session;
}

void CT_TCP::f_bind(void) {

  boost::asio::ip::tcp::endpoint c_endpoint;
  //boost::system::error_code c_error;

  //_DBG << "Start binding : " << _str_service;
  try {

    //  int64_t i_tmp;
    //std::istringstream instr(_str_service);
    //instr >> i_tmp;
    //_DBG << i_tmp;

    //if (i_tmp) {
    //c_endpoint = tcp::endpoint(tcp::v4(), i_tmp);
    //} else {

    tcp::resolver resolver(_c_io_service);
    tcp::resolver::query query(tcp::v4(), _str_service,
        boost::asio::ip::resolver_query_base::all_matching
            | boost::asio::ip::resolver_query_base::passive);
    tcp::resolver::iterator iterator = resolver.resolve(query);
    c_endpoint = (*iterator);
    //}

    _c_acceptor.open(c_endpoint.protocol());
    _c_acceptor.set_option(
        boost::asio::ip::tcp::acceptor::reuse_address(true));
    _c_acceptor.bind(c_endpoint);
    _c_acceptor.listen();
  } catch (std::exception const & e) {
    _FATAL << e.what();
  }
  //_DBG << "End Binding";

  /* Launch accept */
  f_accept();
}

void CT_TCP::f_accept(void) {
  //_DBG << "Waiting new connection";

//_c_acceptor.listen();

  {
    _pc_tmp_session = new CT_TCP_SESSION(*this);
    _c_acceptor.async_accept(_pc_tmp_session->f_get_socket(),
        boost::bind(&CT_TCP::on_accept, this, _pc_tmp_session,
            boost::asio::placeholders::error));
  }
}

void CT_TCP::on_accept(CT_TCP_SESSION* in_pc_session,
    const boost::system::error_code& in_error) {
  _DBG << "Handling new connection";
  if (!in_error) {
    /* Register new session */
    f_session_add(in_pc_session);

    /* Waiting new connection */
    f_accept();
  } else {
    _CRIT << "An error has occurred during accept:" << in_error.message();
    delete in_pc_session;
  }
}

void CT_TCP::f_connect(void) {
  /* Ignoring if destination == auto */
  if (_str_dst.find("auto") == std::string::npos) {
    //_DBG << "Handling new connection callback";

    boost::system::error_code c_error;
    tcp::resolver resolver(_c_io_service);
    tcp::resolver::query query(tcp::v4(), _str_dst, _str_service,
        boost::asio::ip::resolver_query_base::all_matching
            | boost::asio::ip::resolver_query_base::passive);

    tcp::resolver::iterator iterator = resolver.resolve(query, c_error);
    if (c_error) {
      _CRIT << "Unable to resolve url:" << _str_dst << ":"<< _str_service << " : "<< c_error.message();
    } else {

      CT_TCP_SESSION* pc_session = new CT_TCP_SESSION(*this);

#if LF_ASYNC_CONNECT
      pc_session->f_get_socket().async_connect(*iterator,
          boost::bind(&CT_TCP::on_connect, this, pc_session,
              boost::asio::placeholders::error));
#else
      boost::system::error_code in_c_error;
      pc_session->f_get_socket().connect(*iterator, in_c_error);
      on_connect(pc_session, in_c_error);
#endif
    }
  }
}

void CT_TCP::on_connect(CT_TCP_SESSION* in_pc_session,
    const boost::system::error_code& in_c_error) {
  if (!in_c_error) {
    f_session_add(in_pc_session);

    _DBG << "Connection is now ready";
    // Connect succeeded.
  } else {
    _CRIT << "Connect failed";
    delete in_pc_session;

    /* Notify failure */
    CT_PORT_NODE_GUARD c_guard(
        master::plugins::transport::E_ID_CONNECT_FAILURE);
    f_session_notify(c_guard);
  }
  //_b_connect_in_progress = false;
}

void CT_TCP::on_timeout(boost::optional<boost::system::error_code>* a,
    const boost::system::error_code& in_c_error) {
  if (!in_c_error) {
    //_CRIT << "Reset" << in_c_error.message();
    a->reset(in_c_error);
    //_DBG << _V(this)<< "Timout cb";
  } else {
    //_CRIT << _V(this)<< "Timer error : " << in_c_error.message();
  }

}

int CT_TCP::f_run(CT_HOST_CONTEXT&) {
  int ec;

  boost::optional<boost::system::error_code> timer_result;
  boost::asio::deadline_timer c_timer(_c_io_service,
      boost::posix_time::milliseconds(500));
  //_DBG << _V(this) << " Timer Start";
  c_timer.async_wait(
      boost::bind(&CT_TCP::on_timeout, this, &timer_result, _1));
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

    //D("Timer STOP 2");
    if (timer_result) {
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
  //_DBG << _V(this) << "Timer STOP";

  return ec;
}

void CT_TCP::f_session_notify(CT_GUARD<CT_PORT_NODE> const & in_c_msg) {
  f_send_cb(in_c_msg);
}

void CT_TCP::on_post_send(CT_GUARD<CT_PORT_NODE> const in_pc_msg) {
  if (_l_sessions.empty()) {
    if (_b_server_mode) {
      //_DBG << "Server has no client";
    } else {
      //_DBG << "Client not connected";
      if (in_pc_msg->get_id()
          == (port_node_id_t) master::plugins::transport::E_ID_CONNECT) {
        /* Update destination with ip */
        _str_dst = in_pc_msg->get_data<std::string>();
        M_ASSERT(_str_dst.size() >= 1);
        if(_str_dst.size()){
          std::vector<std::string> _v_str_tmp = f_string_split(_str_dst, ":");
          if(_v_str_tmp.size() >= 1){
            _str_dst = _v_str_tmp[0];
            if (_v_str_tmp.size() > 1) {
              _str_service = _v_str_tmp[1];
            }
          }
        }
      }

      f_connect();
    }
  }

  /* Check if node has routing info */
  if (in_pc_msg->has(master::plugins::transport::E_ID_SESSION_INFO)) {
    _DBG << "Node " << std::hex << in_pc_msg->get_id();
    CT_PORT_NODE::childs_result c_res = in_pc_msg->get_childs(
        master::plugins::transport::E_ID_SESSION_INFO);

    for (CT_PORT_NODE::childs_iterator pc_it = c_res.begin();
        pc_it != c_res.end(); pc_it++) {

      std::string str_dest = (*pc_it)->get_data<std::string>();

      {
        std::map<std::string, CT_TCP_SESSION*>::iterator pc_it_tmp =
            _m_clients.find(str_dest);
        if (pc_it_tmp != _m_clients.end()) {
          _CRIT << "Route for " << str_dest << " not found";
          M_BUG();
        } else {
          /* Send message */
#ifdef LF_DEBUG_TCP
          _DBG << "Sending to session" << pc_it_tmp->second;
#endif
          pc_it_tmp->second->f_send(in_pc_msg);
        }
      }
    }
  } else {
    CT_GUARD_LOCK c_guard(_c_lock);

    for (std::list<CT_TCP_SESSION*>::iterator pc_it = _l_sessions.begin();
        pc_it != _l_sessions.end(); pc_it++) {
#ifdef LF_DEBUG_TCP
      _DBG << "Send msg to TCP session:" << (*pc_it)->f_get_address_full();
#endif
      (*pc_it)->f_send(in_pc_msg);
    }
  }

}
int CT_TCP::f_send(CT_GUARD<CT_PORT_NODE> const & in_c_node) {
  int ec;
#ifdef LF_DEBUG_TCP
  _DBG << "Sending node: " << std::hex << in_c_node->get_id();
#endif
  /* post node on io service */
  _c_io_service.post(boost::bind(&CT_TCP::on_post_send, this, in_c_node));

  ec = EC_SUCCESS;
  return ec;

}
M_PLUGIN_TRANSPORT(CT_TCP, "TCP", M_STR_VERSION);
