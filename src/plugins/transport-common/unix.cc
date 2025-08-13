/***********************************************************************
 ** unix.cc
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
 * @file unix.cc
 * Plugin unix transport.
 *
 * @author SEAGNAL (johann.baudy@seagnal.fr)
 * @date 2025
 *
 * @version 1.0 Original version
 */

/***********************************************************************
 * Includes
 ***********************************************************************/

#include <boost/asio.hpp>
#include <boost/optional.hpp>
#include <cpp/string.hh>
#include <c/misc.h>
#include <boost/asio/local/stream_protocol.hpp>
#include <sstream>
#include <unix.hh>
#include "api.hh"

using namespace std;
using namespace bml;
using namespace boost::asio;

CT_UNIX_SESSION::CT_UNIX_SESSION(CT_UNIX &in_c_server)
    : node_parser<CT_GUARD>(), _c_socket(in_c_server.f_get_service()), _c_server(in_c_server) {
    _i_offset_read = 0;
    _i_offset_parser = 0;
    _v_buffer_recv.resize(C_PORT_NODE_BUFFER_SIZE);
    _v_buffer_send.resize(10 * C_PORT_NODE_BUFFER_SIZE);
    _b_connected = false;
    _b_connect_in_progress = false;
    _sz_available = 0;
}

CT_UNIX_SESSION::~CT_UNIX_SESSION() {
    boost::system::error_code error;
    _c_socket.close(error);
    if (error) {
        std::cerr << error.message() << std::endl;
    }
}

void CT_UNIX_SESSION::f_set_endpoint(std::string const &in_str_address, std::string const &in_str_service) {
    _str_address = in_str_address;
    _str_service = in_str_service;
}

void CT_UNIX_SESSION::f_start() {
    _b_connected = true;
    _str_address_full = _str_address + std::string(":") + _str_service;
    f_notify_link_established();
    f_async_read();
}

void CT_UNIX_SESSION::f_async_read(void) {
    size_t sz_buffer = _v_buffer_recv.size();
    size_t sz_remain = sz_buffer - _i_offset_read - _sz_available;

    if (!sz_remain) {
        if (!_i_offset_read) {
            _i_offset_read = 0;
            _sz_available = 0;
        } else {
            memmove(&_v_buffer_recv[0], &_v_buffer_recv[_i_offset_read], _sz_available);
            _i_offset_read = 0;
            sz_remain = sz_buffer - _i_offset_read - _sz_available;
        }
    }

    _c_socket.async_read_some(
        boost::asio::buffer(&_v_buffer_recv[_i_offset_read + _sz_available], sz_remain),
        boost::bind(&CT_UNIX_SESSION::on_read, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

int CT_UNIX_SESSION::f_sync_read(void) {
    int ec;
    size_t sz_buffer = _v_buffer_recv.size();
    size_t sz_remain = sz_buffer - _i_offset_read - _sz_available;

    if (!sz_remain) {
        if (!_i_offset_read) {
            if (sz_buffer < 512 * 1024 * 1024) {
                _WARN << "Buffer is full, Allocating more data" <<_V(sz_buffer);
                _v_buffer_recv.resize(2 * sz_buffer);
                sz_buffer = _v_buffer_recv.size();
                sz_remain = sz_buffer - _i_offset_read - _sz_available;
            } else {
                _CRIT << "Buffer is full " <<_V(sz_buffer);
                _i_offset_read = 0;
                _sz_available = 0;
                ec = EC_FAILURE;
                goto out_err;
            }
        } else {
            memmove(&_v_buffer_recv[0], &_v_buffer_recv[_i_offset_read], _sz_available);
            _i_offset_read = 0;
            sz_remain = sz_buffer - _i_offset_read - _sz_available;
        }
    }

    if (!sz_remain) {
        ec = EC_BML_OVERFLOW;
        goto out_err;
    }
    {

        size_t sz_current = _sz_available;
        _c_socket.async_read_some(
            boost::asio::buffer(&_v_buffer_recv[_i_offset_read + _sz_available], sz_remain),
            boost::bind(&CT_UNIX_SESSION::on_read_sync, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));

        while (sz_current == _sz_available) {
            boost::system::error_code ec_boost;

    #if BOOST_VERSION < 107000
        auto & refIO = _c_socket.get_io_service();
    #else
        boost::asio::io_context& refIO = static_cast<boost::asio::io_context&>(_c_socket.get_executor().context());
    #endif
            if (!refIO.poll_one(ec_boost)) {
                usleep(0);
            }
            if (ec_boost) {
                std::cerr << "Run error: " << ec_boost.message() << std::endl;
            }
        }

        if (_ec_sync_boost) {
            std::cerr << "Error during sync read: " << _ec_sync_boost.message() << std::endl;
            ec = EC_FAILURE;
            goto out_err;
        }
    }

    ec = EC_SUCCESS;
out_err:
    return ec;
}

int CT_UNIX_SESSION::parse_data(char *in_pc_buffer, size_t in_sz_buffer) {


#ifdef LF_DEBUG_UNIX
  _DBG << " Request " << in_sz_buffer << " bytes";
#endif
    int ec;
    while (in_sz_buffer + _i_offset_parser > _sz_available) {
        ec = f_sync_read();
        if (ec != EC_SUCCESS) {
            return ec;
        }
    }

    memcpy(in_pc_buffer, &_v_buffer_recv[_i_offset_read + _i_offset_parser], in_sz_buffer);
    _i_offset_parser += in_sz_buffer;
    return EC_BML_SUCCESS;
}

void CT_UNIX_SESSION::f_send(CT_GUARD<CT_PORT_NODE> const &in_pc_node) {
    CT_GUARD_LOCK c_guard(_c_msg_lock);
    bool b_inactive = _l_msg.empty();
    _l_msg.push_back(in_pc_node);
    if (b_inactive) {
        f_send();
    }
}

void CT_UNIX_SESSION::f_send(void) {
    int ec;
    CT_GUARD<CT_PORT_NODE> &pc_node = _l_msg.front();
    size_t sz_whole = pc_node->get_whole_size();

    if (_v_buffer_send.size() <= sz_whole) {
        size_t sz_buffer = (sz_whole * 3) / 2;
        _WARN << "Reallocating buffer from " << _v_buffer_send.size() << " to " << _V(sz_buffer) << " bytes";
        _v_buffer_send.resize(sz_buffer);
    }

    node_buffer_writer<CT_GUARD> c_writer(&_v_buffer_send[0], _v_buffer_send.size());
    ec = pc_node->to_writer(c_writer);

    if (ec != EC_BML_SUCCESS) {
        if (ec == EC_BML_OVERFLOW) {
            std::cerr << "Buffer overflow" << std::endl;
        }
        std::cerr << "Unable to convert message to buffer" << std::endl;
        ec = EC_FAILURE;
        goto out_err;
    }
#ifdef LF_DEBUG_UNIX
    _DBG << "Sending " << c_writer.get_offset() << " bytes";
#endif
    boost::asio::async_write(_c_socket, boost::asio::buffer(&_v_buffer_send[0], c_writer.get_offset()),
                             boost::bind(&CT_UNIX_SESSION::on_send, this, boost::asio::placeholders::error));

out_err:
    return;
}

void CT_UNIX_SESSION::f_disconnect(void) {
    if (_b_connected) {
        f_notify_link_failure();
    }
    _b_connected = false;
    _c_server.f_session_remove(this);
}

void CT_UNIX_SESSION::on_send(const boost::system::error_code &error) {
    CT_GUARD_LOCK c_guard(_c_msg_lock);
    if (!error) {
        _l_msg.pop_front();
        if (!_l_msg.empty()) {
            f_send();
        }
    } else {
        std::cerr << "An error has occurred during send: " << error.message() << std::endl;
        _c_socket.close();
        _l_msg.clear();
    }
}

void CT_UNIX_SESSION::on_read_sync(const boost::system::error_code &error, size_t bytes_transferred) {
    _ec_sync_boost = error;
    if (!error) {
        _sz_available += bytes_transferred;
    }
}

void CT_UNIX_SESSION::on_read(const boost::system::error_code &error, size_t bytes_transferred) {
    int ec;
    if (boost::asio::error::eof == error || boost::asio::error::connection_reset == error) {
        std::cerr << "Client closed connection: " << error.message() << std::endl;
        ec = EC_FAILURE;
    } else if (!error) {
        _sz_available += bytes_transferred;
        while (_sz_available) {
            CT_PORT_NODE_GUARD c_node;
            _i_offset_parser = 0;
            try {
                node_parser<CT_GUARD> *pc_tmp = this;
                node_parser<CT_GUARD> &c_tmp = *pc_tmp;
                ec = c_node->from_parser(c_tmp);
                if (ec == EC_BML_NODATA) {
                    break;
                } else if (ec != EC_SUCCESS) {
                    std::cerr << "Unable to handle received data" << std::endl;
                    _i_offset_read = 0;
                    _sz_available = 0;
                    ec = EC_FAILURE;
                    goto out_err;
                } else {
                    if (c_node->get_id() == (port_node_id_t)master::plugins::transport::E_ID_CONNECT) {
                        c_node->get(master::plugins::transport::E_ID_SESSION_INFO, 0, true)->set_data<std::string>(f_get_address_full());
                    }
                    _c_server.f_received(c_node);
                    _i_offset_read += _i_offset_parser;
                    _sz_available -= _i_offset_parser;
                }
            } catch (std::exception const &e) {
                std::cerr << e.what() << std::endl;
                _i_offset_read = 0;
                _sz_available = 0;
                ec = EC_FAILURE;
                goto out_err;
            }
        }
        f_async_read();
        ec = EC_SUCCESS;
    } else {
        std::cerr << "An error has occurred: " << error.message() << std::endl;
        ec = EC_FAILURE;
    }
out_err:
    if (ec == EC_FAILURE) {
        f_disconnect();
    }
}

void CT_UNIX_SESSION::f_notify_link_failure() {
    CT_PORT_NODE_GUARD c_guard(master::plugins::transport::E_ID_DISCONNECTED);
    c_guard->get(master::plugins::transport::E_ID_SESSION_INFO, 0, true)->set_data<std::string>(f_get_address_full());
    _c_server.f_session_notify(c_guard);
}

void CT_UNIX_SESSION::f_notify_link_established(void) {
    CT_PORT_NODE_GUARD c_guard(master::plugins::transport::E_ID_CONNECTED);
    c_guard->get(master::plugins::transport::E_ID_SESSION_INFO, 0, true)->set_data<std::string>(f_get_address_full());
    _c_server.f_session_notify(c_guard);
}

CT_UNIX::CT_UNIX(void) : _c_io_service(), _c_socket(_c_io_service), _c_acceptor(_c_io_service) {
    _b_server_mode = false;
    _pc_context = NULL;
    _pc_tmp_session = NULL;
}

CT_UNIX::~CT_UNIX() {
    _c_io_service.stop();
    _c_socket.close();
    if (_pc_context) {
        delete _pc_context;
    }
    for (std::list<CT_UNIX_SESSION *>::iterator pc_it = _l_sessions.begin(); pc_it != _l_sessions.end(); pc_it++) {
        delete *pc_it;
    }
    if (_pc_tmp_session) {
        delete _pc_tmp_session;
    }
}

int CT_UNIX::f_apply_config(std::string &in_str_url) {
    int ec;
    _pc_context = CT_HOST::host->f_new_context();
    std::vector<std::string> _v_str = f_string_split(in_str_url, ":");
    if (_v_str.size() < 2) {
        std::cerr << "UNIX socket url is incorrect: " << in_str_url << std::endl;
        return EC_FAILURE;
    }
    _str_dst = _v_str[0];
    if (_str_dst == "server") {
      _b_server_mode = true;
    }
    
    _str_service = _v_str[1];
    if (_b_server_mode) {
        f_bind();
    } else {
        f_connect();
    }
    ec = _pc_context->f_start(M_CONTEXT_BIND(CT_UNIX, f_run, this));
    if (ec != EC_SUCCESS) {
        std::cerr << "Unable to start context" << std::endl;
        return EC_FAILURE;
    }
    return EC_SUCCESS;
}

void CT_UNIX::f_session_remove(CT_UNIX_SESSION *in_pc_session) {
    CT_GUARD_LOCK c_guard(_c_lock);
    _m_clients.erase(in_pc_session->f_get_address_full());
    _l_sessions.remove(in_pc_session);
    delete in_pc_session;
}

void CT_UNIX::f_session_add(CT_UNIX_SESSION *in_pc_session) {
    CT_GUARD_LOCK c_guard(_c_lock);
    _l_sessions.push_back(in_pc_session);
    in_pc_session->f_start();
    _m_clients[in_pc_session->f_get_address_full()] = in_pc_session;
}

void CT_UNIX::f_bind(void) {
    local::stream_protocol::endpoint endpoint(_str_service);
    _c_acceptor.open(endpoint.protocol());
    _c_acceptor.set_option(boost::asio::socket_base::reuse_address(true));
    unlink(_str_service.c_str());

    int one = 1;
    setsockopt(_c_acceptor.native_handle(), SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &one, sizeof(one));

    _c_acceptor.bind(endpoint);
    _c_acceptor.listen();
    f_accept();
}

void CT_UNIX::f_accept(void) {
    _pc_tmp_session = new CT_UNIX_SESSION(*this);
    _c_acceptor.async_accept(_pc_tmp_session->f_get_socket(),
                             boost::bind(&CT_UNIX::on_accept, this, _pc_tmp_session, boost::asio::placeholders::error));
}

void CT_UNIX::on_accept(CT_UNIX_SESSION *in_pc_session, const boost::system::error_code &in_error) {
    if (!in_error) {
        f_session_add(in_pc_session);
    } else {
        std::cerr << "An error has occurred during accept: " << in_error.message() << std::endl;
        delete in_pc_session;
    }
    f_accept();
}

void CT_UNIX::f_connect(void) {
    if (_str_dst.find("auto") == std::string::npos) {
        boost::system::error_code c_error;
        local::stream_protocol::endpoint endpoint(_str_service);
        CT_UNIX_SESSION *pc_session = new CT_UNIX_SESSION(*this);
        pc_session->f_get_socket().async_connect(endpoint, boost::bind(&CT_UNIX::on_connect, this, pc_session, boost::asio::placeholders::error));
    }
}

void CT_UNIX::on_connect(CT_UNIX_SESSION *in_pc_session, const boost::system::error_code &in_c_error) {
    if (!in_c_error) {
        f_session_add(in_pc_session);
    } else {
        std::cerr << "Connect failed: " << in_c_error.message() << std::endl;
        delete in_pc_session;
        CT_PORT_NODE_GUARD c_guard(master::plugins::transport::E_ID_CONNECT_FAILURE);
        f_session_notify(c_guard);
    }
}

int CT_UNIX::f_run(CT_HOST_CONTEXT &) {
    boost::system::error_code ec_boost;
    while (true) {
        if (!_c_io_service.run_one(ec_boost)) {
            _c_io_service.reset();
            continue;
        }
        if (ec_boost) {
            std::cerr << "Run error: " << ec_boost.message() << std::endl;
        }
        if (_pc_context->f_has_extra()) {
            return EC_SUCCESS;
        }
    }
}

void CT_UNIX::f_session_notify(CT_GUARD<CT_PORT_NODE> const &in_c_msg) {
    f_send_cb(in_c_msg);
}

void CT_UNIX::on_post_send(CT_GUARD<CT_PORT_NODE> const in_pc_msg) {
    if (_l_sessions.empty()) {
        if (!_b_server_mode) {
            if (in_pc_msg->get_id() == (port_node_id_t)master::plugins::transport::E_ID_CONNECT) {
                _str_dst = in_pc_msg->get_data<std::string>();
                if (_str_dst.size()) {
                    std::vector<std::string> _v_str_tmp = f_string_split(_str_dst, ":");
                    if (_v_str_tmp.size() >= 1) {
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
    if (in_pc_msg->has(master::plugins::transport::E_ID_SESSION_INFO)) {
        CT_PORT_NODE::childs_result c_res = in_pc_msg->get_childs(master::plugins::transport::E_ID_SESSION_INFO);
        for (CT_PORT_NODE::childs_iterator pc_it = c_res.begin(); pc_it != c_res.end(); pc_it++) {
            std::string str_dest = (*pc_it)->get_data<std::string>();
            std::map<std::string, CT_UNIX_SESSION *>::iterator pc_it_tmp = _m_clients.find(str_dest);
            if (pc_it_tmp != _m_clients.end()) {
                pc_it_tmp->second->f_send(in_pc_msg);
            }
        }
    } else {
        CT_GUARD_LOCK c_guard(_c_lock);
        for (std::list<CT_UNIX_SESSION *>::iterator pc_it = _l_sessions.begin(); pc_it != _l_sessions.end(); pc_it++) {
            (*pc_it)->f_send(in_pc_msg);
        }
    }
}

int CT_UNIX::f_send(CT_GUARD<CT_PORT_NODE> const &in_c_node) {
    _c_io_service.post(boost::bind(&CT_UNIX::on_post_send, this, in_c_node));
    return EC_SUCCESS;
}

M_PLUGIN_TRANSPORT(CT_UNIX, "UNIX", M_STR_VERSION);