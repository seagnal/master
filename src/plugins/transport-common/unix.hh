/***********************************************************************
 ** unix.hh
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
#ifndef UNIX_SOCKET_HH_
#define UNIX_SOCKET_HH_
/**
 * @file unix.hh
 * UNIX SOCKET Plugin transport definition.
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
#include <boost/asio/local/stream_protocol.hpp>

#include <c/common.h>
#include <plugin.hh>
#include <host.hh>
#include <list>

/***********************************************************************
 * Defines
 ***********************************************************************/

/***********************************************************************
 * Types
 ***********************************************************************/

 class CT_UNIX_SESSION;

class CT_UNIX : public CT_TRANSPORT {
public:
    CT_UNIX(void);
    ~CT_UNIX();

    int f_apply_config(std::string &in_str_url);
    void f_session_remove(CT_UNIX_SESSION *in_pc_session);
    void f_session_add(CT_UNIX_SESSION *in_pc_session);
    void f_bind(void);
    void f_accept(void);
    void on_accept(CT_UNIX_SESSION *in_pc_session, const boost::system::error_code &in_error);
    void f_connect(void);
    void on_connect(CT_UNIX_SESSION *in_pc_session, const boost::system::error_code &in_error);
    int f_run(CT_HOST_CONTEXT &);
    void f_session_notify(CT_GUARD<CT_PORT_NODE> const &in_c_msg);
    void on_post_send(CT_GUARD<CT_PORT_NODE> const in_pc_msg);
    int f_send(CT_GUARD<CT_PORT_NODE> const &in_c_node);
	boost::asio::io_service& f_get_service() {
		return _c_io_service;
	}
private:
    boost::asio::io_context _c_io_service;
    boost::asio::local::stream_protocol::socket _c_socket;
    boost::asio::local::stream_protocol::acceptor _c_acceptor;
    bool _b_server_mode;
    CT_HOST_CONTEXT *_pc_context;
    std::string _str_dst;
    std::string _str_service;
    std::list<CT_UNIX_SESSION *> _l_sessions;
    std::map<std::string, CT_UNIX_SESSION *> _m_clients;
    CT_UNIX_SESSION *_pc_tmp_session;
    CT_SPINLOCK _c_lock;
};

class CT_UNIX_SESSION : bml::node_parser<CT_GUARD> {
public:
    CT_UNIX_SESSION(CT_UNIX &in_c_server);
    ~CT_UNIX_SESSION();

    boost::asio::local::stream_protocol::socket &f_get_socket() { return _c_socket; }
    void f_set_endpoint(std::string const &in_str_address, std::string const &in_str_service);
    void f_start();
    void f_async_read(void);
    int f_sync_read(void);
    int parse_data(char *in_pc_buffer, size_t in_sz_buffer);
    void f_send(CT_GUARD<CT_PORT_NODE> const &in_pc_node);
    void f_send(void);
    void f_disconnect(void);
    void on_send(const boost::system::error_code &error);
    void on_read_sync(const boost::system::error_code &error, size_t bytes_transferred);
    void on_read(const boost::system::error_code &error, size_t bytes_transferred);
    void f_notify_link_failure();
    void f_notify_link_established(void);
	std::string f_get_address_full() {
		return _str_address_full;
	}

private:
    boost::asio::local::stream_protocol::socket _c_socket;
    CT_UNIX &_c_server;
    std::string _str_address;
    std::string _str_service;
    std::string _str_address_full;
    std::vector<char> _v_buffer_recv;
    std::vector<char> _v_buffer_send;
    size_t _i_offset_read;
    size_t _i_offset_parser;
    size_t _sz_available;
    bool _b_connected;
    bool _b_connect_in_progress;
    std::list<CT_GUARD<CT_PORT_NODE>> _l_msg;
    CT_SPINLOCK _c_msg_lock;
    boost::system::error_code _ec_sync_boost;
};

#endif /* UNIX_SOCKET_HH_ */
