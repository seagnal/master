/***********************************************************************
 ** wol
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
 * @file wol.cc
 *
 * @author SEAGNAL (johann.baudy@seagnal.fr)
 * @date Apr 7, 2014
 *
 * @version 1.0 Original version
 */

/***********************************************************************
 * Includes
 ***********************************************************************/
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/udp.hpp>

#include <iomanip>

#include "wol.hh"
#include "debug.hh"


using boost::asio::ip::udp;

CT_WOL::CT_WOL() {

}


CT_WOL::CT_WOL(const std::string & in_str_mac) {
	_str_mac_address = in_str_mac;
}

void CT_WOL::f_set_mac_address(const std::string & in_str_mac) {
	_str_mac_address = in_str_mac;
}

void CT_WOL::f_format() {
    _c_packet.fill(0);
    typedef std::vector<std::string> MacAddressParts;
    MacAddressParts parts;
    boost::split(parts, _str_mac_address, boost::is_any_of(":"));
    if (parts.size() == MAC_ADDRESS_LENGTH) {
        for(int i = 0; i < MAC_ADDRESS_LENGTH; ++i) {
        	_c_packet[i] = static_cast<unsigned char>(0xFF);
        }

        for(int i = 1; i <= 16; ++i) {
            for(int j = 0; j < MAC_ADDRESS_LENGTH; ++j) {
            	_c_packet[i * MAC_ADDRESS_LENGTH + j] = static_cast<unsigned char>(strtol(parts[j].c_str(), NULL, 16));
            }
        }
    }
}

bool CT_WOL::f_broadcast() {
    boost::asio::io_service io_service;

    boost::system::error_code error;
    boost::asio::ip::udp::socket socket(io_service);
    /*
		std::cout << "Send WOL to " << std::hex
																<< std::setfill ('0')
																<< std::setw(2) << ((int)_c_packet[0+MAC_ADDRESS_LENGTH]&0xFF) << ":"
		 														<< std::setw(2) << ((int)_c_packet[1+MAC_ADDRESS_LENGTH]&0xFF) << ":"
																<< std::setw(2) << ((int)_c_packet[2+MAC_ADDRESS_LENGTH]&0xFF) << ":"
																<< std::setw(2) << ((int)_c_packet[3+MAC_ADDRESS_LENGTH]&0xFF) << ":"
																<< std::setw(2) << ((int)_c_packet[4+MAC_ADDRESS_LENGTH]&0xFF) << ":"
																<< std::setw(2) << ((int)_c_packet[5+MAC_ADDRESS_LENGTH]&0xFF) << std::setfill (' ') << std::endl;
    */

    socket.open(udp::v4(), error);
    if (!error) {
        socket.set_option(boost::asio::socket_base::broadcast(true));
        f_format();
        socket.send_to(boost::asio::buffer(_c_packet),  boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 9));
        socket.close();
    } else {
      std::cout << "Unable to open socket" << std::endl;
    }

    if (error)
        return false;

    return true;
}
