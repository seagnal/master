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

/**
 * @file serial.cc
 *
 * @author SEAGNAL (johann.baudy@seagnal.fr)
 * @date 2020
 *
 * @version 1.0 Original version
 */

/***********************************************************************
 * Includes
 ***********************************************************************/
 #include "serial.hh"
 #include <boost/asio/read.hpp>
 #include <boost/asio/placeholders.hpp>

 CT_SERIAL::CT_SERIAL(const char * in_str_port_name,
                      uint32_t in_i_baudrate,
                      uint8_t in_i_char_size,
                      ET_PARITY_MODE in_e_parity,
                      ET_STOP_MODE in_e_stop,
                      ET_FLOW_CONTROL_MODE in_e_flow
                    ) :
                       _c_port(_c_io_service),
                       _c_timer(_c_io_service),
                       _b_read_error(true),
                       _i_baudrate(in_i_baudrate),
                       _i_char_size(in_i_char_size),
                       _e_parity(in_e_parity),
                       _e_stop(in_e_stop),
                       _e_flow(in_e_flow)
{
   boost::system::error_code ec;

   _c_port.open(in_str_port_name, ec);
   if (ec) {
     throw _CRIT << "error : port_->open() failed...com_port_name="
     << in_str_port_name << ", e=" << ec.message().c_str();
   }

   // option settings...
   _c_port.set_option(boost::asio::serial_port_base::baud_rate(in_i_baudrate));
   _c_port.set_option(boost::asio::serial_port_base::character_size(in_i_char_size));

   switch(_e_stop) {
     case E_STOP_MODE_TWO_BIT:
     _c_port.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::two));
     break;
     case E_STOP_MODE_ONE_BIT:
     _c_port.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
     break;
   }

   switch(_e_parity) {
     case E_PARITY_MODE_ODD:
     _c_port.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::odd));
     break;
     case E_PARITY_MODE_EVEN:
     _c_port.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::even));
     break;
     case E_PARITY_MODE_NONE:
     _c_port.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
     break;
   }


   switch(_e_flow) {
     case E_FLOW_CONTROL_NONE:
     _c_port.set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));
    break;
     case E_FLOW_CONTROL_RTSCTS:
     _c_port.set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::hardware));
     break;
   }

 }



 // Called when an async read completes or has been cancelled
 void CT_SERIAL::read_complete(const boost::system::error_code& in_error,
           size_t in_sz_bytes_transferred) {
   _b_read_error = (in_error || in_sz_bytes_transferred == 0);
   _c_timer.cancel();
 }

 // Called when the timer's deadline expires.
 void CT_SERIAL::time_out(const boost::system::error_code& in_error) {

   // Was the timeout was cancelled?
   if (in_error) {
     return;
   }
   _c_port.cancel();
 }


 bool CT_SERIAL::f_write(std::string const & in_str_value, bool in_b_debug) {
   boost::system::error_code ec;

	if (in_str_value.size() == 0)  {
    return 0;
  }
   if(in_b_debug) {
   _DBG << "Sending to serial port: "<<in_str_value;
   }
   _c_port.write_some(boost::asio::buffer(in_str_value.c_str(), in_str_value.size()), ec);

   if(ec) {
     return EC_FAILURE;
   } else {
     return EC_SUCCESS;
   }
 }


 bool CT_SERIAL::f_read_char(char& out_c_val, int32_t in_i_timeout_ms) {

   out_c_val = _c_tmp = '\0';

   // After a timeout & cancel it seems we need
   // to do a reset for subsequent reads to work.
   _c_io_service.reset();

   // Asynchronously read 1 character.
   boost::asio::async_read(_c_port, boost::asio::buffer(&_c_tmp, 1),
       boost::bind(&CT_SERIAL::read_complete,
           this,
           boost::asio::placeholders::error,
           boost::asio::placeholders::bytes_transferred));

   // Setup a deadline time to implement our timeout.
   _c_timer.expires_from_now(boost::posix_time::milliseconds(in_i_timeout_ms));
   _c_timer.async_wait(boost::bind(&CT_SERIAL::time_out,
               this, boost::asio::placeholders::error));

   // This will block until a character is read
   // or until the it is cancelled.
   _c_io_service.run();

   if (!_b_read_error) {
     out_c_val = _c_tmp;
  }

   return !_b_read_error;
 }
