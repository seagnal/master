/***********************************************************************
 ** port
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
 ***********************************************************************/

/* define against mutual inclusion */
#ifndef PORT_HH_
#define PORT_HH_

/**
 * @file port.hh
 *
 * @author SEAGNAL (johann.baudy@seagnal.fr)
 * @date Jan 22, 2013
 *
 * @version 1.0 Original version
 */

/***********************************************************************
 * Includes
 ***********************************************************************/

#include <c/common.h>
#include <c/atomic.h>
#include <cpp/guard.hh>
#include <cpp/bind.hh>
#include <cpp/string.hh>
#include <mutex>
#include <list>
#include <vector>
#include <node.hh>
#include <plugin.hh>

//#include <boost/pool/pool.hpp>

typedef uint32_t port_node_id_t;
/***********************************************************************
 * Defines
 ***********************************************************************/
#define C_PORT_NODE_BUFFER_SIZE (1024*1024)

/***********************************************************************
 * Types
 ***********************************************************************/
class CT_CORE;
class CT_PORT;
class CT_HOST_LOCK;
class CT_PORT_NODE;

/* Type of data callbacks */
//typedef int (*FT_CB_DATA)(CT_CORE * in_pc_core,
//		CT_GUARD<CT_PORT_NODE> & in_c_data, CT_PORT * in_i_port);
//struct ST_CB_DATA {
//	FT_CB_DATA pf_cb;
//	CT_PORT * pc_port;
//};
/***********************************************************************
 * PROT NODE CLASS
 ***********************************************************************/

extern std::map<uint32_t, const char *> gm_port_node_name;
typedef std::map<uint32_t, const char *>::iterator gm_port_node_name_iterator_t;

class CT_PORT_NODE: public bml::node<port_node_id_t, CT_GUARD> {
public:
	enum ET_NODE_EXT {
		E_EXT_TIME = 0, E_EXT_HEADER, E_EXT_ROUTING_MAP,
	};
public:
	ST_GUARD_USAGE s_usage;

	/* Memtrack info */
	ST_MEMTRACK_INFO s_memtrack_info;

public:
	typedef std::vector<CT_PORT_NODE*> childs_result;
	typedef std::vector<CT_PORT_NODE*>::iterator childs_iterator;

public:
	CT_PORT_NODE() :
			bml::node<port_node_id_t, CT_GUARD>((uint32_t)-1), s_usage(false, this) {
#ifdef FF_MEMTRACK
		CT_MEMTRACK::f_register_malloc(s_memtrack_info, this, sizeof(CT_PORT_NODE), "CT_PORT_NODE", 1);
#endif
		//D("Constructor+ %p", this);
	}

	CT_PORT_NODE(uint32_t const & in_id) :
			bml::node<port_node_id_t, CT_GUARD>(in_id), s_usage(false, this) {

#ifdef FF_MEMTRACK
		std::string str_tmp;
		gm_port_node_name_iterator_t pc_it = gm_port_node_name.find(in_id);
		if(gm_port_node_name.end()== pc_it) {
			std::string str_tmp;
			str_tmp = f_string_format("CT_PORT_NODE %08x", in_id);
			gm_port_node_name[in_id] = strdup(str_tmp.c_str());
			pc_it = gm_port_node_name.find(in_id);
		}
		CT_MEMTRACK::f_register_malloc(s_memtrack_info, this, sizeof(CT_PORT_NODE), pc_it->second, 1);
#endif
		//D("Constructor+ %p", this);
	}

	template<typename T2> CT_PORT_NODE(uint32_t const & in_id,
			T2 const & in_data) :
			bml::node<port_node_id_t, CT_GUARD>(in_id), s_usage(false, this) {

#ifdef FF_MEMTRACK
		std::string str_tmp;
		gm_port_node_name_iterator_t pc_it = gm_port_node_name.find(in_id);
		if(gm_port_node_name.end()== pc_it) {
			std::string str_tmp;
			str_tmp = f_string_format("CT_PORT_NODE %08x", in_id);
			gm_port_node_name[in_id] = strdup(str_tmp.c_str());
			pc_it = gm_port_node_name.find(in_id);
		}
		CT_MEMTRACK::f_register_malloc(s_memtrack_info, this, sizeof(CT_PORT_NODE), pc_it->second, 1);
#endif
		this->set_data<T2>(in_data);
		//D("Constructor+ %p", this);
	}

	CT_PORT_NODE(uint32_t const & in_id,
			bml::node_resource_segment<CT_GUARD> const & in_pc_seg,
			off_t in_off_data, size_t in_sz_data) :
			bml::node<port_node_id_t, CT_GUARD>(in_id, in_pc_seg, in_off_data,
					in_sz_data), s_usage(false, this) {
#ifdef FF_MEMTRACK
		std::string str_tmp;
		gm_port_node_name_iterator_t pc_it = gm_port_node_name.find(in_id);
		if(gm_port_node_name.end()== pc_it) {
			std::string str_tmp;
			str_tmp = f_string_format("CT_PORT_NODE %08x", in_id);
			gm_port_node_name[in_id] = strdup(str_tmp.c_str());
			pc_it = gm_port_node_name.find(in_id);
		}
		CT_MEMTRACK::f_register_malloc(s_memtrack_info, this, sizeof(CT_PORT_NODE), pc_it->second, 1);
#endif
	}

	virtual ~CT_PORT_NODE() {
		clear();
#ifdef LF_NODE_DBG
		if (f_node_dbg_in(this)) {
			D("Destructor of node %x", get_id());
		}
#endif
#ifdef FF_MEMTRACK
		CT_MEMTRACK::f_register_free(s_memtrack_info);
#endif
	}

	void set_id(uint32_t const & in_c_id);

	static void * operator new(size_t in_i_size);
	static void operator delete(void * in_pv) throw ();

	CT_GUARD<bml::node<uint32_t, CT_GUARD> > on_new(void);
	bml::node_resource_segment<CT_GUARD> on_new_resource(size_t in_i_size);
	virtual void on_delete(CT_GUARD<bml::node<uint32_t, CT_GUARD> > * in_pc_child);

	/* parenthesis operators */
	CT_PORT_NODE & operator()(uint32_t const & in_id, uint64_t in_i_indice = 0);

	/* Get methods */
	CT_GUARD<CT_PORT_NODE> get(uint32_t const & in_id, uint64_t in_i_indice = 0,
			bool in_b_create = false);
	/* Add method */
	CT_GUARD<CT_PORT_NODE> add(uint32_t const & in_id);

  /* add_from method */
  CT_GUARD<CT_PORT_NODE> add_from(uint32_t const & in_id,CT_GUARD<CT_PORT_NODE> & in_pc_node);

	/* Add display method */
	void display(int i_shift=0);

	/* Update node ID (from old master to master-core) */
	void f_update_node(std::list<std::pair<uint32_t, uint32_t>> &rlsi_id);

	/* Set data overload */
	template<typename T2> void set_data(T2 const & in) {
		bml::node<uint32_t, CT_GUARD>::set_data(in);
#ifdef FF_MEMTRACK_NAME_PORT_NODE
		if (get_segment().isglobal()) {
			CT_MEMTRACK::f_set_name(get_segment()._pc_resource->mmap(),
					typeid(T2).name());
		}
#endif
	}
	template<typename T2> T2 const get_data(void) const {
		if(get_size() != sizeof(T2)) {
			throw _CRIT << "Data in node 0x"<< std::hex <<get_id() << " does not match size: "<< std::dec << sizeof(T2) << " != " << get_size();
		}
		return bml::node<uint32_t, CT_GUARD>::get_data<T2>();
	}
#if 0
	template<typename T2> T2 const get_data_check(void) const {
		if(get_size() != sizeof(T2)) {
			throw _CRIT << "Data in node 0x"<< std::hex <<get_id() << " does not match size: "<< std::dec << sizeof(T2) << " != " << get_size();
		}
		return bml::node<uint32_t, CT_GUARD>::get_data<T2>();
	}
#endif

	template<typename T2> T2 * mmap(void) const {
#ifdef FF_MEMTRACK_NAME_PORT_NODE
		if (get_segment().isglobal()) {
			CT_MEMTRACK::f_set_name(get_segment()._pc_resource->mmap(),
					typeid(T2).name());
		}
#endif
		return bml::node<uint32_t, CT_GUARD>::mmap<T2>();
	}
	template<typename T2> T2 & map(void) const {
#ifdef FF_MEMTRACK_NAME_PORT_NODE
		if (get_segment().isglobal()) {
			CT_MEMTRACK::f_set_name(get_segment()._pc_resource->mmap(),
					typeid(T2).name());
		}
#endif
		return bml::node<uint32_t, CT_GUARD>::map<T2>();
	}

	template<typename T2> T2 * force_resource(void) const {
		if (typeid(*_s_segment._pc_resource) != typeid(T2)) {
			T2 * pc_tmp = new T2();

		}
		return NULL;
	}

	CT_PORT_NODE::childs_result get_childs(uint32_t in_i_id);
	CT_PORT_NODE::childs_result get_childs(void);

	/* SRaw mode parser */
	int from_parser_raw(bml::node_parser<CT_GUARD> & in_c_parser);
	int to_writer_raw(bml::node_writer<CT_GUARD> & in_c_writer, int in_i_align = 0);

};

/* Customize GUARD init - speed improvement due to usage allocation */
template<>
CT_GUARD<CT_PORT_NODE>::CT_GUARD(CT_PORT_NODE * in_pointer);

/***********************************************************************
 * PORT NODE GUARD CLASS
 * Allow friendly stack code and smart pointer usage
 ***********************************************************************/

class CT_PORT_NODE_GUARD: public CT_GUARD<CT_PORT_NODE> {

public:
	CT_PORT_NODE_GUARD() :
			CT_GUARD<CT_PORT_NODE>(new CT_PORT_NODE((uint32_t)-1)) {
	}

	CT_PORT_NODE_GUARD(uint32_t const & in_id) :
			CT_GUARD<CT_PORT_NODE>(new CT_PORT_NODE(in_id)) {
	}
	CT_PORT_NODE_GUARD(ST_GUARD_USAGE * in_ps_usage) :
			CT_GUARD<CT_PORT_NODE>(in_ps_usage) {
	}

	CT_PORT_NODE_GUARD(CT_PORT_NODE_GUARD const & other) {
		*this = other;
	}

	template<typename T2> CT_PORT_NODE_GUARD(uint32_t const & in_id,
			T2 const & in_data) :
			CT_GUARD<CT_PORT_NODE>(new CT_PORT_NODE(in_id, in_data)) {
	}

	CT_PORT_NODE_GUARD(uint32_t const & in_id,
			bml::node_resource_segment<CT_GUARD> const & in_pc_seg,
			off_t in_off_data, size_t in_sz_data) :
			CT_GUARD<CT_PORT_NODE>(
					new CT_PORT_NODE(in_id, in_pc_seg, in_off_data, in_sz_data)) {

	}

	void display();

	using CT_GUARD<CT_PORT_NODE>::operator->;
#if 0
	operator CT_GUARD<CT_PORT_NODE>&() {
		return *this;
	}
	operator const CT_GUARD<CT_PORT_NODE>&() const {
		return *this;
	}
  #endif
};

/***********************************************************************
 * BIND CLASS
 ***********************************************************************/

#if 0
template <typename T>
using CT_PORT_BIND = CT_BIND< int, T, CT_GUARD<CT_PORT_NODE>& >;
#endif
#if 0
using TOTO = std::map<int,int>;
using CT_PORT_BIND_CORE = CT_BIND_CORE< int, CT_GUARD<CT_PORT_NODE>& >;
using CT_PORT_SLOT = CT_SLOT<int, CT_GUARD<CT_PORT_NODE>&>;
#endif

#if 0
template<typename T>
struct ST_PORT_TYPE {
	typedef CT_BIND< int, T, CT_GUARD<CT_PORT_NODE>& > bind_t;
	typedef CT_BIND_CORE< int, CT_GUARD<CT_PORT_NODE>& > bind_core_t;
	typedef CT_SLOT<int, CT_GUARD<CT_PORT_NODE>&> slot_t;
};
#endif

typedef CT_BIND_CORE<int, CT_GUARD<CT_PORT_NODE> const&> CT_PORT_BIND_CORE;
typedef CT_BIND_NULL<int, CT_GUARD<CT_PORT_NODE> const&> CT_PORT_BIND_NULL;
typedef CT_SLOT<int, CT_GUARD<CT_PORT_NODE> const &> CT_PORT_SLOT;
typedef CT_SIGNAL<CT_GUARD<CT_PORT_NODE> const &> CT_PORT_SIGNAL;
#define M_PORT_BIND(T, F, P) CT_BIND<int, T, CT_GUARD<CT_PORT_NODE> const & >(&T ::F , P)
/***********************************************************************
 * CT_TRANSPORT CLASS
 ***********************************************************************/
class CT_TRANSPORT: public CT_PLUGIN {

public:
	CT_SPINLOCK _c_lock;

	/* Name of transport */
	CT_NODE _c_config;

	/* Raw mode */
	bool _b_raw_mode;

	/* List of callbacks */
	typedef std::list<CT_PORT_BIND_CORE const *>::iterator _l_cbs_iterator_t;
	std::list<CT_PORT_BIND_CORE const *> _l_cbs;

	/* transport option availability */
	bool _b_support_linked_transport;

	/* transport option enable */
	bool _b_linked_transport;

protected:
	enum ET_NODE_ID {
		E_NODE_ID_POINTER = -1, E_NODE_ID_RAW = -2,
	};

protected:

	/* Execute registered callbacks */
	void f_send_cb(CT_GUARD<CT_PORT_NODE> const & in_c_node);
	/* Execute registered callbacks in current context */
	void f_push_cb(CT_GUARD<CT_PORT_NODE> const & in_c_node);

public:
	CT_TRANSPORT();
	virtual ~CT_TRANSPORT();

	/* TX */
	/* Send a complete Node */
	virtual int f_send(CT_GUARD<CT_PORT_NODE> const & in_c_node);
	/* Send a complete Buffer */
	virtual int f_send(char * in_pc_buffer, size_t in_sz_buffer);
	/* Send a part of Buffer */
	virtual int f_send(char * in_pc_buffer, size_t in_sz_buffer,
			size_t & out_sz_sent);

	/* RX */
	virtual int f_received(char * in_pc_buffer, size_t in_sz_buffer,
			size_t & out_sz_read);
	virtual int f_received(
			bml::node_resource_segment<CT_GUARD> const & in_s_segment);
	virtual int f_received(CT_GUARD<CT_PORT_NODE> & in_c_node);

	int f_register_cb(CT_PORT_BIND_CORE const* in_ps_cb);
	int f_unregister_cb(CT_PORT_BIND_CORE const* in_ps_cb);

	/* virtual methods */
	/* URL config */
	virtual int f_apply_config(std::string & in_str_url);

	/* Config get & Set */
	CT_NODE & f_get_config(void);
	virtual void f_set_config(CT_NODE const & in_c_node);

};
/***********************************************************************
 * CT_PORT_INTERNAL CLASS
 ***********************************************************************/
/* Friendly class of CT_PORT, that store RX and TX port info */
class CT_PORT_INTERNAL {
public:
	CT_GUARD<CT_TRANSPORT> _pc_rx;
	CT_GUARD<CT_TRANSPORT> _pc_tx;

	/* Callback pointer RX*/
	CT_PORT_BIND_CORE const * _pc_cb_rx;

	/* Callback pointer TX */
	//ST_CB_DATA * _ps_cb_tx;
	CT_PORT_INTERNAL();
	~CT_PORT_INTERNAL();

	int f_register_cb(CT_PORT_SLOT & in_pc_cb);
	//int f_register_cb_tx(ST_CB_DATA * in_ps_cb);

	int f_set_url(CT_NODE & in_c_url);

	int f_send(CT_GUARD<CT_PORT_NODE> const & in_c_node);
};

class CT_PORT {
	std::vector<CT_PORT_INTERNAL> _v_ports;

	/* Owner of port */
	CT_CORE * _pc_owner;

	/* RW lock */
	//CT_RW_SPINLOCK _c_rw_lock;
private:
	/* Callback RX of port */
	CT_PORT_SLOT c_cb_rx;

	/* Callback TX of port */
	CT_PORT_SLOT c_cb_tx;

public:
	/* Receive slot handler */
	CT_PORT_SLOT c_receive;

public:
	std::string str_name;
	uint i_id;

	int i_update;

	CT_PORT(CT_CORE *);
	~CT_PORT();

	int f_set_url(CT_NODE & in_c_node);
	int f_register_cb_rx(CT_PORT_BIND_CORE const & in_c_bind);
	int f_register_cb_tx(CT_PORT_BIND_CORE const & in_c_bind);

	int f_receive(CT_GUARD<CT_PORT_NODE> const & in_c_node);
	int f_send(CT_GUARD<CT_PORT_NODE> const & in_c_node);
	int f_clear(void);

	int f_push(CT_GUARD<CT_PORT_NODE> & in_c_node);

	std::string const & f_get_name() {
		return str_name;
	}
	uint f_get_id() {
		return i_id;
	}
};
namespace bml {
template<> int node<std::string, CT_GUARD>::from_parser_id(
		bml::node_parser<CT_GUARD> & in_c_parser, uint64_t in_sz_id);
template<> int node<std::string, CT_GUARD>::to_writer_id(
		bml::node_writer<CT_GUARD> & in_c_writer, uint64_t in_sz_id);
template<> int node<std::string, CT_GUARD>::get_id_size(void) const;
}
template<> CT_GUARD<CT_PORT_NODE>::CT_GUARD(CT_PORT_NODE * in_pointer);

template<> void CT_PORT_NODE::set_data<>(std::string const & in);

template<> void CT_PORT_NODE::set_data<>(std::vector<char> const & in);
template<> void CT_PORT_NODE::set_data<>(std::vector<int16_t> const & in);
template<> void CT_PORT_NODE::set_data<>(std::vector<uint16_t> const & in);
template<> void CT_PORT_NODE::set_data<>(std::vector<int32_t> const & in);
template<> void CT_PORT_NODE::set_data<>(std::vector<uint32_t> const & in);
template<> void CT_PORT_NODE::set_data<>(std::vector<int64_t> const & in);
template<> void CT_PORT_NODE::set_data<>(std::vector<uint64_t> const & in);
template<> void CT_PORT_NODE::set_data<>(std::vector<float> const & in);
template<> void CT_PORT_NODE::set_data<>(std::vector<double> const & in);

template<> std::string const CT_PORT_NODE::get_data<>(void) const;
template<> std::vector<char> const CT_PORT_NODE::get_data<>(void) const;
template<> std::vector<int16_t> const CT_PORT_NODE::get_data<>(void) const;
template<> std::vector<uint16_t> const CT_PORT_NODE::get_data<>(void) const;
template<> std::vector<int32_t> const CT_PORT_NODE::get_data<>(void) const;
template<> std::vector<uint32_t> const CT_PORT_NODE::get_data<>(void) const;
template<> std::vector<int32_t> const CT_PORT_NODE::get_data<>(void) const;
template<> std::vector<uint32_t> const CT_PORT_NODE::get_data<>(void) const;
template<> std::vector<float> const CT_PORT_NODE::get_data<>(void) const;
template<> std::vector<double> const CT_PORT_NODE::get_data<>(void) const;

#if 0
class CT_LINKED_BUFFER_WRITER: public node_buffer_writer {
public:
	CT_LINKED_BUFFER_WRITER(char* in_pc_buffer, size_t in_sz_buffer);
	~CT_LINKED_BUFFER_WRITER();

	virtual void write_resource(node_resource_segment & in_c_resource);
};

class CT_LINKED_BUFFER_PARSER: public node_buffer_parser {
public:
	CT_LINKED_BUFFER_PARSER(char * in_pc_buffer, size_t in_sz_buffer);
	~CT_LINKED_BUFFER_PARSER();

	virtual node_resource_segment parse_resource(size_t in_sz_buffer);
};
#endif

#else
class CT_USAGE;
class CT_TRANSPORT;
class CT_PORT;
#endif /* PORT_HH_ */
