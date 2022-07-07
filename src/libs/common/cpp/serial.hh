/***********************************************************************
** serial
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
#ifndef SERIAL_HH_
#define SERIAL_HH_

/**
* @file serial.hh
*
* @author SEAGNAL (johann.baudy@seagnal.fr)
* @date 2020
*
* @version 1.0 Original version
*/

/***********************************************************************
* Includes
***********************************************************************/
#include <cpp/debug.hh>
#include <boost/asio/io_service.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/bind.hpp>
/***********************************************************************
* Defines
***********************************************************************/
enum ET_PARITY_MODE {
  E_PARITY_MODE_NONE,
  E_PARITY_MODE_ODD,
  E_PARITY_MODE_EVEN,
};
enum ET_STOP_MODE {
  E_STOP_MODE_TWO_BIT,
  E_STOP_MODE_ONE_BIT
};
enum ET_FLOW_CONTROL_MODE {
  E_FLOW_CONTROL_NONE,
  E_FLOW_CONTROL_RTSCTS,
};
/***********************************************************************
* Types
***********************************************************************/
class CT_SERIAL
{
  boost::asio::io_service _c_io_service;
  boost::asio::serial_port _c_port;

  char _c_tmp;
  boost::asio::deadline_timer _c_timer;
  bool _b_read_error;

  uint32_t _i_baudrate;
  uint32_t _i_char_size;
  ET_PARITY_MODE _e_parity;
  ET_STOP_MODE _e_stop;
  ET_FLOW_CONTROL_MODE _e_flow;

private:
  // Called when an async read completes or has been cancelled
  void read_complete(const boost::system::error_code& in_error, size_t in_sz_bytes_transferred);

  // Called when the timer's deadline expires.
  void time_out(const boost::system::error_code& in_error);

public:
  CT_SERIAL(
    const char * in_str_port_name,
    uint32_t in_i_baudrate,
    uint8_t in_i_char_size,
    ET_PARITY_MODE in_e_parity,
    ET_STOP_MODE in_e_stop,
    ET_FLOW_CONTROL_MODE in_e_flow);

  bool f_write(std::string const & in_str_value, bool in_b_debug = false);
  bool f_read_char(char& out_c_val, int32_t in_i_timeout_ms);
};

  #endif /* SERIALIO_HH_ */
