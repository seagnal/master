/***********************************************************************
 ** event
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
#ifndef EVENT_TCP_HH_
#define EVENT_TCP_HH_

/**
 * @file event.hh
 *
 * @author SEAGNAL (johann.baudy@seagnal.fr)
 * @date Nov 25, 2013
 *
 * @version 1.0 Original version
 */


/***********************************************************************
 * Defines
 ***********************************************************************/
#ifdef ID_DEFINITION
namespace master {
namespace plugins {
namespace transport {

enum ET_ID {
	/* Status */
	E_ID_CONNECTED,
	E_ID_DISCONNECTED,
	E_ID_CONNECT_FAILURE,

	/* Order */
	E_ID_CONNECT,
	E_ID_DISCONNECT,
	E_ID_SESSION_INFO,

	E_ID_MAC_FROM,
	E_ID_MAC_TO,
};

}
}
}
#else
#include <transport-common/api.autogen.hh>
#endif

namespace master {
namespace plugins {
namespace transport {

struct ST_MAC {
	unsigned char ac_data[6];

	ST_MAC() {
		memset(ac_data, 0, sizeof(ac_data));
	}
	ST_MAC(unsigned char in_c_0, unsigned char in_c_1, unsigned char in_c_2,
			unsigned char in_c_3, unsigned char in_c_4, unsigned char in_c_5) {
		ac_data[0] = in_c_0;
		ac_data[1] = in_c_1;
		ac_data[2] = in_c_2;
		ac_data[3] = in_c_3;
		ac_data[4] = in_c_4;
		ac_data[5] = in_c_5;
	}

	unsigned char & operator[](int in_i_index) {
		return ac_data[in_i_index];
	}
} M_PACKED;

struct ST_PACKET_HEADER {
	ST_MAC s_dst;
	ST_MAC s_src;
	uint16_t i_proto;
	uint16_t i_dummy;
} M_PACKED;

#define C_NB_SWITCH 4
struct ST_ROUTING_TABLE {
	unsigned char ac_ports[C_NB_SWITCH];
	ST_ROUTING_TABLE() {
		memset(ac_ports, 0xff, sizeof(ac_ports));
	}
	ST_ROUTING_TABLE(char in_i_port_0, char in_i_port_1 = 0xff, char in_i_port_2 =
			0xff, char in_i_port_3 = 0xff) {
		ac_ports[0] = in_i_port_0;
		ac_ports[1] = in_i_port_1;
		ac_ports[2] = in_i_port_2;
		ac_ports[3] = in_i_port_3;
	}

	ST_ROUTING_TABLE(uint32_t in_i_routing) {
		ac_ports[3] = (in_i_routing >> 24) & 0xFF;
		ac_ports[2] = (in_i_routing >> 16) & 0xFF;
		ac_ports[1] = (in_i_routing >> 8) & 0xFF;
		ac_ports[0] = (in_i_routing >> 0) & 0xFF;
	}

	ST_ROUTING_TABLE(std::vector<unsigned char> const & in_v_port) {
		for (uint i = 0; i < C_NB_SWITCH; i++) {
			if (in_v_port.size() > i) {
				ac_ports[i] = in_v_port[i];
			} else {
				ac_ports[i] = 0xff;
			}
		}
	}

	operator bool() const {
		if (ac_ports[0] != 0xff) {
			return true;
		}
		if (ac_ports[1] != 0xff) {
			return true;
		}
		if (ac_ports[2] != 0xff) {
			return true;
		}
		if (ac_ports[3] != 0xff) {
			return true;
		}
		return false;
	}
	bool operator==(ST_ROUTING_TABLE const & other) const {
		if (ac_ports[0] == other.ac_ports[0])
			if (ac_ports[1] == other.ac_ports[1])
				if (ac_ports[2] == other.ac_ports[2])
					if (ac_ports[3] == other.ac_ports[3])
						return true;

		return false;
	}
	bool operator<(ST_ROUTING_TABLE const & other) const {

		if (ac_ports[0] < other.ac_ports[0]) {
			return true;
		}
		if (ac_ports[0] > other.ac_ports[0]) {
			return false;
		}
		if (ac_ports[1] < other.ac_ports[1]) {
			return true;
		}
		if (ac_ports[1] > other.ac_ports[1]) {
			return false;
		}
		if (ac_ports[2] < other.ac_ports[2]) {
			return true;
		}
		if (ac_ports[2] > other.ac_ports[2]) {
			return false;
		}
		if (ac_ports[3] < other.ac_ports[3]) {
			return true;
		}
		//if (ac_ports[3] > other.ac_ports[3]) {
		//	return false;
		//}
		return false;
	}

	friend std::ostream& operator<<(std::ostream& os,
			const ST_ROUTING_TABLE& dt) {
				if(dt) {
				os << std::hex  << (int) dt.ac_ports[0] << '/' << std::hex << (int) dt.ac_ports[1] << '/'
						<< std::hex << (int) dt.ac_ports[2] << '/' << std::hex << (int) dt.ac_ports[3];
				} else {
					os << "NONE";
				}
				return os;
			}

} M_PACKED;

}
}
}

#endif /* EVENT_HH_ */
