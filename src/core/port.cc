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

/**
 * @file port.cc
 *
 * @author SEAGNAL (johann.baudy@seagnal.fr)
 * @date Jan 22, 2013
 *
 * @version 1.0 Original version
 */

/***********************************************************************
 * Includes
 ***********************************************************************/
#include <boost/pool/singleton_pool.hpp>
#include <event_base.hh>
#include <cpp/guard.hh>
#include <host.hh>
#include <iostream>

/***********************************************************************
 * bml node specialization for ID string
 ***********************************************************************/
using namespace bml;

template<>
int node<std::string, CT_GUARD>::from_parser_id(
		node_parser<CT_GUARD> & in_c_parser, uint64_t in_sz_id) {
	int ec;
	char str_tmp[in_sz_id];
	/* Read ID field */
	ec = in_c_parser.parse_data((char*) str_tmp, in_sz_id);
	if (ec != EC_BML_SUCCESS) {
		return ec;
	}
	_id = str_tmp;
	return EC_BML_SUCCESS;
}

template<>
int node<std::string, CT_GUARD>::to_writer_id(
		node_writer<CT_GUARD> & in_c_writer, uint64_t in_sz_id) {
	return in_c_writer.write_data((char*) _id.c_str(), in_sz_id);
}

template<>
int node<std::string, CT_GUARD>::get_id_size(void) const {
	return _id.size() + 1;
}
/* Customize GUARD init - speed improvement due to usage allocation */
template<>
CT_GUARD<CT_PORT_NODE>::CT_GUARD(CT_PORT_NODE * in_pointer) {
	init(&(in_pointer->s_usage));
}

/***********************************************************************
 * CT_PORT_NODE POOL
 ***********************************************************************/

struct ST_PORT_NODE_TAG {
};
typedef boost::singleton_pool<ST_PORT_NODE_TAG, sizeof(CT_PORT_NODE)> C_PORT_NODE_POOL;
uint32_t gi_port_node_cnt = 0;
std::map<uint32_t, const char *> gm_port_node_name;

/***********************************************************************
 * CT_PORT_NODE CLASS
 ***********************************************************************/
CT_GUARD<node<uint32_t, CT_GUARD> > CT_PORT_NODE::on_new(void) {
	CT_PORT_NODE * pc_node = new CT_PORT_NODE();
	if (!pc_node) {
		std::bad_alloc();
	}
	//_DBG << pc_node;
	return CT_GUARD<node<uint32_t, CT_GUARD> >(pc_node);
}

void CT_PORT_NODE::on_delete(
		CT_GUARD<node<uint32_t, CT_GUARD> > * in_pc_child) {

}

node_resource_segment<CT_GUARD> CT_PORT_NODE::on_new_resource(
		size_t in_i_size) {
	//D("NEW CUST resource: %p", pc_tmp);
	return CT_HOST::host->f_get_cpu_resource().f_alloc(in_i_size);
}

void * CT_PORT_NODE::operator new(size_t in_i_size) {
	/* Allocate node from pool */
	CT_PORT_NODE* pc_tmp = (CT_PORT_NODE*) C_PORT_NODE_POOL::malloc();
	//CT_MEMTRACK::f_register_malloc(pc_tmp, in_i_size);
	if (!pc_tmp) {
		throw std::bad_alloc();
	}
	gi_port_node_cnt++;

	//D("NEW node %p (%d)", pc_tmp, gi_port_node_cnt);
	return pc_tmp;
}
void CT_PORT_NODE::operator delete(void * in_pv) throw () {
	if (in_pv == 0)
		return;

	gi_port_node_cnt--;
	//CT_MEMTRACK::f_register_free(in_pv);
	C_PORT_NODE_POOL::free(in_pv);
	//D("Delete node %p (%d)", in_pv, gi_port_node_cnt);
}
#if 0
CT_PORT_NODE & CT_PORT_NODE::operator()(uint32_t const & in_id,
		uint64_t in_i_indice) {
	node<port_node_id_t, CT_GUARD> & c_tmp = get(in_id, in_i_indice, true);
	return static_cast<CT_PORT_NODE&>(c_tmp);
}

CT_PORT_NODE & CT_PORT_NODE::get(uint32_t const & in_id, uint64_t in_i_indice,
		bool in_b_create) {
	node<port_node_id_t, CT_GUARD> & c_tmp =
			node<port_node_id_t, CT_GUARD>::get(in_id, in_i_indice,
					in_b_create);
	return static_cast<CT_PORT_NODE&>(c_tmp);
}

CT_PORT_NODE::childs_result CT_PORT_NODE::get_childs(uint32_t in_i_id) {
	CT_PORT_NODE::childs_result v_tmp;
	std::pair<it, it> c_ret = _m_childs.equal_range(in_i_id);

	for (it pc_it = c_ret.first; pc_it != c_ret.second; ++pc_it) {
		v_tmp.push_back(static_cast<CT_PORT_NODE*>(pc_it->second.get()));
	}

	return v_tmp;
}
#else

void CT_PORT_NODE::set_id(uint32_t const & in_id) {
#ifdef FF_MEMTRACK
	/* Updating Port node ID */
	{
		std::string str_tmp;
		gm_port_node_name_iterator_t pc_it = gm_port_node_name.find(in_id);
		if(gm_port_node_name.end()== pc_it) {
			std::string str_tmp;
			str_tmp = f_string_format("CT_PORT_NODE %08x", in_id);
			gm_port_node_name[in_id] = strdup(str_tmp.c_str());
			pc_it = gm_port_node_name.find(in_id);
			s_memtrack_info.str_name = pc_it->second;
		}
	}
#endif

	node<port_node_id_t,
	CT_GUARD>::set_id(in_id);
}
CT_PORT_NODE & CT_PORT_NODE::operator()(uint32_t const & in_id,
		uint64_t in_i_indice) {
	CT_GUARD<node<uint32_t, CT_GUARD> > c_tmp =
			node<port_node_id_t, CT_GUARD>::get(in_id, in_i_indice, true);
	return static_cast<CT_PORT_NODE &>(*c_tmp);
}

CT_GUARD<CT_PORT_NODE> CT_PORT_NODE::get(uint32_t const & in_id,
		uint64_t in_i_indice, bool in_b_create) {
	try {
		CT_GUARD<node<uint32_t, CT_GUARD> > c_tmp = node<port_node_id_t,
				CT_GUARD>::get(in_id, in_i_indice, in_b_create);
		return dynamic_pointer_cast<CT_PORT_NODE>(c_tmp);
	} catch (std::exception const & e) {
		throw _CRIT << "Unable to get id: " << e.what();
	}
}

//TODO REMOVE VECTOR - BURK REALOC
// CAST ITERATOR FROM BML NODE IN ORDER TO MATCH FIND FUNCTION
CT_PORT_NODE::childs_result CT_PORT_NODE::get_childs(uint32_t in_i_id) {
	CT_PORT_NODE::childs_result v_tmp;
	std::pair<it, it> c_ret = _m_childs.equal_range(in_i_id);

	for (it pc_it = c_ret.first; pc_it != c_ret.second; ++pc_it) {
		v_tmp.push_back(static_cast<CT_PORT_NODE*>(pc_it->second.get()));
	}

	return v_tmp;
}
#endif

CT_PORT_NODE::childs_result CT_PORT_NODE::get_childs(void) {
	CT_PORT_NODE::childs_result v_tmp;
	for (it pc_it = _m_childs.begin(); pc_it != _m_childs.end(); ++pc_it) {
		v_tmp.push_back(static_cast<CT_PORT_NODE*>(pc_it->second.get()));
	}
	return v_tmp;
}

CT_GUARD<CT_PORT_NODE> CT_PORT_NODE::add(uint32_t const & in_id) {
	/* Insert created node */
	return dynamic_pointer_cast<CT_PORT_NODE>(
			node<uint32_t, CT_GUARD>::add(in_id));
	//return dynamic_cast<CT_PORT_NODE&>(*(node<uint32_t,CT_GUARD>::add(in_id)));
}
CT_GUARD<CT_PORT_NODE> CT_PORT_NODE::add_from(uint32_t const & in_id,CT_GUARD<CT_PORT_NODE> & in_pc_node) {
	/* Insert created node */
	CT_GUARD<node<uint32_t, CT_GUARD>> pc_tmp = dynamic_pointer_cast<node<uint32_t, CT_GUARD>>(in_pc_node);

	return dynamic_pointer_cast<CT_PORT_NODE>(
			node<uint32_t, CT_GUARD>::add_from(in_id,pc_tmp));
}

void CT_PORT_NODE::display(int i_shift) {

	std::string str_indent(i_shift*2, ' ');
  std::string str_name = CT_HOST::host->f_id_name(this->get_id());
  /*
  if ("NONE" == str_name){
    char buffer [15];
    sprintf(buffer, "%x", this->get_id());
    str_name = buffer;
  }
  */
	size_t sz_pos = str_name.find_last_of("::");
	size_t sz_str = str_name.substr(sz_pos+1).size();

	size_t sz_allign = 1000 - sz_str - i_shift*2;
	sz_allign = sz_allign % 40;
	std::string str_allign(sz_allign, ' ');
	if(this->get_size() == 4){
		_DBG << str_indent  <<  str_name.substr(sz_pos+1) << /* " ("  << std::uppercase  << std::hex << this->get_id() << ") "  << */ str_allign <<  "    SIZE:" << this->get_size()<<"  uint32_t:"<<std::setw(20)<<this->get_data<uint32_t>()<<"   int32_t:" << std::setw(20)<<this->get_data<int32_t>()<<"   float:"<<this->get_data<float>();
	}
	else if(this->get_size() == 2){
		_DBG << str_indent  <<  str_name.substr(sz_pos+1) << /* " ("  << std::uppercase  << std::hex << this->get_id() << ") "  << */ str_allign <<  "    SIZE:" << this->get_size()<<"  uint16_t:"<<std::setw(20)<<this->get_data<uint16_t>()<<"   int16_t:"<< std::setw(20)<<this->get_data<int16_t>();
	}
	else if(this->get_size() == 8){
		_DBG << str_indent  <<  str_name.substr(sz_pos+1) << /* " ("  << std::uppercase  << std::hex << this->get_id() << ") "  << */ str_allign <<  "    SIZE:" << this->get_size()<<"  uint64_t:"<<std::setw(20)<<this->get_data<uint64_t>()<<"   int64_t:"<< std::setw(20)<<this->get_data<int64_t>()<<"   double:"<<this->get_data<double>();
	}
	else if(this->get_size() == 1){
		_DBG << str_indent  <<  str_name.substr(sz_pos+1) << /* " ("  << std::uppercase  << std::hex << this->get_id() << ") "  << */ str_allign <<  "    SIZE:" << this->get_size()<<"  uint8_t :" <<std::setw(20)<<(uint16_t)this->get_data<uint8_t>() <<"   char:"  <<this->get_data<char>();
	}
	else{
		_DBG << str_indent  <<  str_name.substr(sz_pos+1) << /* " ("  << std::uppercase  << std::hex << this->get_id() << ") "  << */ str_allign <<  "    SIZE:" << this->get_size();
	}

	for(auto && rc_child : this->get_childs()) {

		rc_child->display(i_shift+1);

	}
}


void CT_PORT_NODE::f_update_node(std::list<std::pair<uint32_t, uint32_t>> &rlsi_id) {

	for (const auto & rsi_id : rlsi_id) {
		if (rsi_id.first == this->get_id()) {
			this->set_id(rsi_id.second);
			continue;
		}
	}

	for(auto && rc_child : this->get_childs()) {

		rc_child->f_update_node(rlsi_id);

	}
}


template<>
void CT_PORT_NODE::set_data<>(std::string const & in) {
	size_t sz_str = in.size();
	resize(sz_str);
	if (sz_str) {
		memcpy_from_buffer(in.c_str(), sz_str);
	}
}

template<>
void CT_PORT_NODE::set_data<>(std::vector<char> const & in) {
	size_t sz_data = in.size();
	resize(sz_data);
	if (sz_data) {
		memcpy_from_buffer(in.data(), sz_data);
	}
}
template<>
void CT_PORT_NODE::set_data<>(std::vector<int64_t> const & in) {
	size_t sz_data = in.size()*sizeof(int64_t);
	resize(sz_data);
	if (sz_data) {
		memcpy_from_buffer((char*)in.data(), sz_data);
	}
}
template<>
void CT_PORT_NODE::set_data<>(std::vector<int32_t> const & in) {
	size_t sz_data = in.size()*sizeof(int32_t);
	resize(sz_data);
	if (sz_data) {
		memcpy_from_buffer((char*)in.data(), sz_data);
	}
}
template<>
void CT_PORT_NODE::set_data<>(std::vector<int16_t> const & in) {
	size_t sz_data = in.size()*sizeof(int16_t);
	resize(sz_data);
	if (sz_data) {
		memcpy_from_buffer((char*)in.data(), sz_data);
	}
}
template<>
void CT_PORT_NODE::set_data<>(std::vector<uint64_t> const & in) {
	size_t sz_data = in.size()*sizeof(uint64_t);
	resize(sz_data);
	if (sz_data) {
		memcpy_from_buffer((char*)in.data(), sz_data);
	}
}
template<>
void CT_PORT_NODE::set_data<>(std::vector<uint32_t> const & in) {
	size_t sz_data = in.size()*sizeof(uint32_t);
	resize(sz_data);
	if (sz_data) {
		memcpy_from_buffer((char*)in.data(), sz_data);
	}
}
template<>
void CT_PORT_NODE::set_data<>(std::vector<float> const & in) {
	size_t sz_data = in.size()*sizeof(float);
	resize(sz_data);
	if (sz_data) {
		memcpy_from_buffer((char*)in.data(), sz_data);
	}
}
template<>
void CT_PORT_NODE::set_data<>(std::vector<double> const & in) {
	size_t sz_data = in.size()*sizeof(double);
	resize(sz_data);
	if (sz_data) {
		memcpy_from_buffer((char*)in.data(), sz_data);
	}
}

template<>
void CT_PORT_NODE::set_data<>(std::vector<uint16_t> const & in) {
	size_t sz_data = in.size()*sizeof(uint16_t);
	resize(sz_data);
	if (sz_data) {
		memcpy_from_buffer((char*)in.data(), sz_data);
	}
}

template<>
std::string const CT_PORT_NODE::get_data<>(void) const {
	size_t sz_str = _s_segment.size();
	M_ASSERT(sz_str < 64*1024);
	if (sz_str) {
		char str_tmp[sz_str];
		memcpy_to_buffer(str_tmp, sz_str);
		return std::string(str_tmp, sz_str);
	} else {
		return std::string("");
	}
}

#if 0
template<typename T>
void CT_PORT_NODE::set_data<>(std::vector<T> const & in) {
	size_t sz_data = in.size()*sizeof(T);
	resize(sz_data);
	if (sz_data) {
		memcpy_from_buffer((char*)in.data(), sz_data);
	}
}

template<typename T>
std::vector<T> const CT_PORT_NODE::get_data<>(void) const {
	size_t sz_str = _s_segment.size();
	M_ASSERT(sz_str < 64*1024);
	if (sz_str) {
		std::vector<T> c_tmp;
		c_tmp.resize(sz_str/sizeof(T));
		memcpy_to_buffer(c_tmp.data(), sz_str);
		return c_tmp;
	} else {
		return std::vector<T>();
	}
}
#endif

template<>
std::vector<char> const CT_PORT_NODE::get_data<>(void) const {
	size_t sz_str = _s_segment.size();
	M_ASSERT(sz_str < 64*1024);
	if (sz_str) {
		std::vector<char> c_tmp;
		c_tmp.resize(sz_str/sizeof(char));
		memcpy_to_buffer(c_tmp.data(), sz_str);
		return c_tmp;
	} else {
		return std::vector<char>();
	}
}

template<>
std::vector<uint16_t> const CT_PORT_NODE::get_data<>(void) const {
	size_t sz_str = _s_segment.size();
	M_ASSERT(sz_str < 64*1024);
	if (sz_str) {
		std::vector<uint16_t> c_tmp;
		c_tmp.resize(sz_str/sizeof(uint16_t));
		memcpy_to_buffer((char*)c_tmp.data(), sz_str);
		return c_tmp;
	} else {
		return std::vector<uint16_t>();
	}
}
template<>
std::vector<uint32_t> const CT_PORT_NODE::get_data<>(void) const {
	size_t sz_str = _s_segment.size();
	M_ASSERT(sz_str < 64*1024);
	if (sz_str) {
		std::vector<uint32_t> c_tmp;
		c_tmp.resize(sz_str/sizeof(uint32_t));
		memcpy_to_buffer((char*)c_tmp.data(), sz_str);
		return c_tmp;
	} else {
		return std::vector<uint32_t>();
	}
}
template<>
std::vector<uint64_t> const CT_PORT_NODE::get_data<>(void) const {
	size_t sz_str = _s_segment.size();
	M_ASSERT(sz_str < 64*1024);
	if (sz_str) {
		std::vector<uint64_t> c_tmp;
		c_tmp.resize(sz_str/sizeof(uint64_t));
		memcpy_to_buffer((char*)c_tmp.data(), sz_str);
		return c_tmp;
	} else {
		return std::vector<uint64_t>();
	}
}

template<>
std::vector<int16_t> const CT_PORT_NODE::get_data<>(void) const {
	size_t sz_str = _s_segment.size();
	M_ASSERT(sz_str < 64*1024);
	if (sz_str) {
		std::vector<int16_t> c_tmp;
		c_tmp.resize(sz_str/sizeof(int16_t));
		memcpy_to_buffer((char*)c_tmp.data(), sz_str);
		return c_tmp;
	} else {
		return std::vector<int16_t>();
	}
}
template<>
std::vector<int32_t> const CT_PORT_NODE::get_data<>(void) const {
	size_t sz_str = _s_segment.size();
	M_ASSERT(sz_str < 64*1024);
	if (sz_str) {
		std::vector<int32_t> c_tmp;
		c_tmp.resize(sz_str/sizeof(int32_t));
		memcpy_to_buffer((char*)c_tmp.data(), sz_str);
		return c_tmp;
	} else {
		return std::vector<int32_t>();
	}
}
template<>
std::vector<int64_t> const CT_PORT_NODE::get_data<>(void) const {
	size_t sz_str = _s_segment.size();
	M_ASSERT(sz_str < 64*1024);
	if (sz_str) {
		std::vector<int64_t> c_tmp;
		c_tmp.resize(sz_str/sizeof(int64_t));
		memcpy_to_buffer((char*)c_tmp.data(), sz_str);
		return c_tmp;
	} else {
		return std::vector<int64_t>();
	}
}

template<>
std::vector<float> const CT_PORT_NODE::get_data<>(void) const {
	size_t sz_str = _s_segment.size();
	if (sz_str) {
		std::vector<float> c_tmp;
		c_tmp.resize(sz_str/sizeof(float));
		memcpy_to_buffer((char*)c_tmp.data(), sz_str);
		return c_tmp;
	} else {
		return std::vector<float>();
	}
}

template<>
std::vector<double> const CT_PORT_NODE::get_data<>(void) const {
	size_t sz_str = _s_segment.size();
	if (sz_str) {
		std::vector<double> c_tmp;
		c_tmp.resize(sz_str/sizeof(double));
		memcpy_to_buffer((char*)c_tmp.data(), sz_str);
		return c_tmp;
	} else {
		return std::vector<double>();
	}
}



int CT_PORT_NODE::from_parser_raw(node_parser<CT_GUARD> & in_c_parser) {
	clear();
	bool b_loop = true;
	int ec;
	set_id(E_ID_COMMON_ID_RAW);
	//node_resource_segment<CT_GUARD> c_tmp;
	std::vector<char> v_tmp;

	//c_tmp.resize(1024);
	v_tmp.reserve(1e6);
	//uint32_t i_offset = 0;
	//	char *pi_buffer = c_tmp.mmap();

	while (b_loop) {
		char i_data;
		/* Read flags */
		ec = in_c_parser.parse_data(&i_data, 1);
		if (ec != EC_BML_SUCCESS) {
			return ec;
		}

		/* Copy data into buffer */
		//pi_buffer[i_offset] = i_data;
		v_tmp.push_back(i_data);
		//i_offset++;

		/* Stop on buffer full */
		if(v_tmp.size()/*i_offset*/ == v_tmp.capacity()/*c_tmp.size()*/) {
			b_loop = false;
		}

		/* Stop on separator char */
		if(i_data == '\n') {
			b_loop = false;
		}
	}

	/* Update data */
	//set_segment(c_tmp, 0, i_offset);
	memcpy_from_buffer(v_tmp.data(),v_tmp.size());

	on_read();

	return EC_BML_SUCCESS;
}


int CT_PORT_NODE::to_writer_raw(node_writer<CT_GUARD> & in_c_writer, int in_i_align) {
	int ec;
	size_t sz_buffer = _s_segment._pc_resource ? _s_segment._sz_data : 0;

	on_write();

	M_ASSERT(sz_buffer == _s_segment.size());
	ec = in_c_writer.write_resource(_s_segment);
	if (ec != EC_BML_SUCCESS) {
		DBG_LINE();
		return ec;
	}

	return EC_BML_SUCCESS;
}


/***********************************************************************
 * CT_TRANSPORT CLASS
 ***********************************************************************/

CT_TRANSPORT::CT_TRANSPORT() {
	/* transport option availability */
	_b_support_linked_transport = true;

	/* transport option enable */
	_b_linked_transport = true;

	/* Disable Raw mode */
	_b_raw_mode = false;
}

CT_TRANSPORT::~CT_TRANSPORT() {
	//CRIT("Deleting transport %p", this);
}

int CT_TRANSPORT::f_register_cb(CT_PORT_BIND_CORE const * in_ps_cb) {
	//D("MUTEX LOCK: %p %p", _pc_lock, this);
	_c_lock.lock();
	_l_cbs.push_back(in_ps_cb);
	_c_lock.unlock();
	return EC_SUCCESS;
}
int CT_TRANSPORT::f_unregister_cb(CT_PORT_BIND_CORE const * in_ps_cb) {
	_c_lock.lock();
	_l_cbs.remove(in_ps_cb);
	_c_lock.unlock();
	return EC_SUCCESS;
}

void CT_TRANSPORT::f_send_cb(CT_GUARD<CT_PORT_NODE> const & in_c_node) {
	_c_lock.lock();

	/* Send data to core */
	for (_l_cbs_iterator_t ps_it = _l_cbs.begin(); ps_it != _l_cbs.end();
			ps_it++) {

		//D("CB:%p CORE:%p PORT:%p",(*ps_it)->pf_cb, (*ps_it)->pc_core, (*ps_it)->pc_port);
		/* Execute cb */
		(*ps_it)->operator ()(in_c_node);

	}

	_c_lock.unlock();
}

void CT_TRANSPORT::f_push_cb(CT_GUARD<CT_PORT_NODE> const & in_c_node) {
	_c_lock.lock();

	/* Send data to push callback */
	for (_l_cbs_iterator_t ps_it = _l_cbs.begin(); ps_it != _l_cbs.end();
			ps_it++) {
		//D("Push node %d", in_c_node.get_id());

		CT_HOST_CONTEXT::context->f_push((*ps_it), in_c_node);
	}

	_c_lock.unlock();
}

int CT_TRANSPORT::f_send(CT_GUARD<CT_PORT_NODE> const & in_c_node) {
	int ec;

	char pc_buffer[C_PORT_NODE_BUFFER_SIZE];

	//CT_PORT_NODE * pc_node = &in_c_node;
	/** Write node to buffer */
	node_buffer_writer<CT_GUARD> c_writer(pc_buffer, sizeof(pc_buffer));

	/** TODO FLAT OPTION **/

	//std::cout << *pc_node << std::endl;
	/****** FLAT MODE *******/
	if (_b_linked_transport) {
		//CT_PORT_NODE *pc_tmp = &in_c_node; //new CT_PORT_NODE();
		//pc_tmp->copy_from(in_c_node);
		//pc_tmp->use();
		//this->use();
		ST_GUARD_USAGE * ps_usage = in_c_node.get_usage();
		ps_usage->use();

		CT_PORT_NODE_GUARD c_guard(E_NODE_ID_POINTER, (uintptr_t) ps_usage);

#if 0
		D("Send reference %p (this:%p, Id:%x, Size:%d) U:%d", pc_tmp, this, in_c_node.get_id() , in_c_node.get_size(), pc_tmp->usage());
#endif
		ec = c_guard->to_writer(c_writer);
		if (ec != EC_BML_SUCCESS) {
			//pc_tmp->unuse();
			//this->unuse();

			FATAL("Unable to convert message to buffer");
			ec = EC_FAILURE;
			goto out_err;
		}

		/* Send nessage */
		ec = f_send(pc_buffer, c_writer.get_offset());
		if (ec != EC_SUCCESS) {
			//pc_tmp->unuse();
			//this->unuse();

			CRIT("Unable to send buffer");
			ec = EC_FAILURE;
			goto out_err;
		}

	} else {

		ec = in_c_node->to_writer(c_writer);
		if (ec != EC_BML_SUCCESS) {
			FATAL("Unable to convert message to buffer");
			ec = EC_FAILURE;
			goto out_err;
		}

		/* Send nessage */
		ec = f_send(pc_buffer, c_writer.get_offset());
		if (ec != EC_SUCCESS) {
			CRIT("Unable to send buffer");
			ec = EC_FAILURE;
			goto out_err;
		}
	}
	M_ASSERT(c_writer.get_offset());
	M_ASSERT(pc_buffer != NULL);

	ec = EC_SUCCESS;
	out_err: return ec;
}

int CT_TRANSPORT::f_send(char * in_pc_buffer, size_t in_sz_buffer) {
	int ec;
	size_t sz_remaining = in_sz_buffer;
	size_t i_offset = 0;
	while (sz_remaining) {
		size_t i_send = 0;

		//D("Send remain %p %d", &in_pc_buffer[i_offset], sz_remaining);
		ec = f_send(&in_pc_buffer[i_offset], sz_remaining, i_send);
		if (ec != EC_SUCCESS) {
			FATAL("Error during send, closing connexion");
			//TODO closing connexion
			goto out_err;
			ec = EC_FAILURE;
		}
		//D("Sent %d bytes", i_send);
		i_offset += i_send;
		sz_remaining -= i_send;

	}

	ec = EC_SUCCESS;
	out_err: return ec;
}

int CT_TRANSPORT::f_send(char * in_pc_buffer, size_t in_sz_buffer,
		size_t & out_sz_sent) {
	M_BUG();
	return EC_BML_FAILURE;
}

int CT_TRANSPORT::f_received(CT_GUARD<CT_PORT_NODE> & in_c_node) {

	/* Dereference if pointer ID */
	bool b_linked_node = (in_c_node->get_id() == (uint32_t) E_NODE_ID_POINTER);
	if (b_linked_node) {
		M_ASSERT(_b_support_linked_transport == true);
		ST_GUARD_USAGE * ps_usage = (ST_GUARD_USAGE*) in_c_node->get_data<
				uintptr_t>();
		CT_PORT_NODE_GUARD c_node(ps_usage);
		ps_usage->unuse();
#if 0
		D("Received reference to %p (this:%p, ID:%x, Size:%d) u:%d",
				pc_node, this, pc_node->get_id(), pc_node->get_size(), pc_node->usage());
#endif

		/* Execute callbacks */
		f_send_cb(c_node);

		/* Delete node */
		//this->unuse();
		//pc_node->unuse();
	} else {
		/* Execute callbacks */
		f_send_cb(in_c_node);
	}

	return EC_SUCCESS;
}

int CT_TRANSPORT::f_received(char * in_pc_buffer, size_t in_sz_buffer,
		size_t & out_sz_read) {
	int ec;
	CT_PORT_NODE_GUARD c_guard;

	node_buffer_parser<CT_GUARD> c_buffer_parser(in_pc_buffer, in_sz_buffer);
	//TODO BUFFER as segment
	//D("%p", &c_node);
	/* Read BML node */
	while (c_buffer_parser.get_size() != c_buffer_parser.get_offset()) {
		ec = c_guard->from_parser(c_buffer_parser);
		if (ec == EC_BML_NODATA) {
			D("Not enough data");
			out_sz_read = 0;
			goto out_err;
		} else if (ec != EC_SUCCESS) {
			FATAL("Unable to convert message to node");
			ec = EC_FAILURE;
			goto out_err;
		} else {
			out_sz_read = c_buffer_parser.get_offset();
		}

		f_received(c_guard);
	}

	ec = EC_SUCCESS;
	out_err: return ec;
}

int CT_TRANSPORT::f_received(
		node_resource_segment<CT_GUARD> const & in_s_segment) {
	CT_PORT_NODE_GUARD c_guard;
	node_segment_parser<CT_GUARD> c_buffer_parser(in_s_segment);
	int ec = EC_BML_FAILURE;
	/* Read BML node */
	try {
		ec = c_guard->from_parser(c_buffer_parser);
	} catch (const std::overflow_error& e) {
		CRIT("Overflow");
	} catch (...) {
		FATAL("Unable to read buffer to node");
	}

	if (ec == EC_BML_SUCCESS) {
		/* Execute callbacks */
		_DBG << _V(c_guard->get_id()) << _V(c_guard->get_size());
		f_received(c_guard);

		return EC_SUCCESS;
	} else {
		CRIT("Unable to decode segment");

		return EC_FAILURE;
	}

}

int CT_TRANSPORT::f_apply_config(std::string & in_str_url) {
	M_BUG();
	return EC_FAILURE;
}


CT_NODE & CT_TRANSPORT::f_get_config(void) {
	return _c_config;
}

void CT_TRANSPORT::f_set_config(CT_NODE const & in_c_node) {
	_c_config.copy(in_c_node);

	/* Store raw option in boolean flag to prevent read of node */
	if(_c_config.has("raw")) {
		_b_raw_mode =  true;
		//M_BUG();
	}

}

/***********************************************************************
 * CT_PORT_INTERNAL CLASS
 ***********************************************************************/

CT_PORT_INTERNAL::CT_PORT_INTERNAL() {
	_pc_rx = NULL;
	_pc_tx = NULL;

	_pc_cb_rx = NULL;
	//_ps_cb_tx = NULL;
}

CT_PORT_INTERNAL::~CT_PORT_INTERNAL() {
	if (_pc_rx) {
		if (_pc_cb_rx) {
			_pc_rx->f_unregister_cb(_pc_cb_rx);
		}
	}

	_pc_rx = NULL;
	_pc_tx = NULL;
}

int CT_PORT_INTERNAL::f_register_cb(CT_PORT_SLOT & in_pc_cb) {
	/* Register CB */
	_pc_cb_rx = in_pc_cb;
	if (_pc_rx) {
		_pc_rx->f_register_cb(_pc_cb_rx);
	}
	return EC_SUCCESS;
}

int CT_PORT_INTERNAL::f_set_url(CT_NODE & in_c_url) {
	int ec;
	bool b_has_rx = in_c_url.has("rx");
	bool b_has_tx = in_c_url.has("tx");

	CT_NODE * pc_config_rx = NULL;
	CT_NODE * pc_config_tx = NULL;

	/* Get RX URL */
	if (b_has_rx) {
		pc_config_rx = &in_c_url("rx");
	}

	/* Get TX URL */
	if (b_has_tx) {
		pc_config_tx = &in_c_url("tx");
	}

	/* Use URL data if no TX or RX */
	if ((!b_has_rx) && (!b_has_tx)) {
		pc_config_rx = pc_config_tx = &in_c_url;
	}

	/* Check URL RX size */
	if (pc_config_rx) {
		b_has_rx = true;
	}

	/* Check URL TX sizw */
	if (pc_config_tx) {
		b_has_tx = true;
	}
#if 0
	D("Url RX: %s,TX: %s",
			pc_config_rx? ((std::string)*pc_config_rx).c_str() : "none", pc_config_tx? ((std::string)*pc_config_tx).c_str(): "none");
#endif
	/* Get RX transport */
	{
		bool b_register = false;
		if (_pc_rx) {
			if (_pc_rx->f_get_config() != (*pc_config_rx)) {
				//D("Releasing transport ...");
				// Unregister
				_pc_rx->f_unregister_cb(_pc_cb_rx);
				_pc_cb_rx = NULL;

				/* clear transport pointer */
				_pc_rx = NULL;
				// Register
				b_register = true;
			}
		} else {

			b_register = true;
		}

		if (b_register) {
			if (b_has_rx) {
				//D("Looking for new RX transport ...");
				/* Get transport */
				_pc_rx = CT_HOST::host->f_transport_get(*pc_config_rx);
				if (!_pc_rx) {
					CRIT("Unable to get port %s",
							std::string(*pc_config_rx).c_str());
					ec = EC_FAILURE;
					goto out_err;
				}
				//D("RX:%p TX:%p", _pc_rx, _pc_tx);

			}
		}
	}
	//D("RX:%p TX:%p", pc_rx, pc_tx);

	/* Get Tx transport */
	{
		bool b_register = false;
		if (_pc_tx) {
			if (_pc_tx->f_get_config() != *pc_config_tx) {
				//D("Releasing transport ...");
				// Unregister
				_pc_tx = NULL;
				// Register
				b_register = true;
			}
		} else {

			b_register = true;
		}

		if (b_register) {
			if (b_has_tx) {
				//D("Looking for new TX transport ...");
				/* Get transport */
				_pc_tx = CT_HOST::host->f_transport_get(*pc_config_tx);
				if (!_pc_tx) {
					CRIT("Unable to get port %s",
							std::string(*pc_config_tx).c_str());
					ec = EC_FAILURE;
					goto out_err;
				}
			}
		}
	}

	//WARN("CREATE TRANSPORT RX:%p TX:%p", _pc_rx.get(), _pc_tx.get());

	ec = EC_SUCCESS;
	out_err: return ec;
}

int CT_PORT_INTERNAL::f_send(CT_GUARD<CT_PORT_NODE> const & in_c_node) {
	int ec;
	if (_pc_tx) {
		ec = _pc_tx->f_send(in_c_node);
		if (ec != EC_SUCCESS) {
			CRIT("Unable to send data on transport %s",
					std::string(_pc_tx->f_get_config()).c_str());
			ec = EC_FAILURE;
			goto out_err;
		}
	}
	ec = EC_SUCCESS;
	out_err: return ec;
}

/***********************************************************************
 * CT_PORT CLASS
 ***********************************************************************/

CT_PORT::CT_PORT(CT_CORE * in_pc_owner) :
				c_receive(M_PORT_BIND(CT_PORT, f_receive, this)) {

	//CT_GUARD_LOCK c_guard(_c_rw_lock.write());
	//_s_cb_rx.pc_core = NULL;
	c_cb_rx = NULL;

	//_s_cb_tx.pc_core = NULL;
	c_cb_tx = NULL;

	_pc_owner = in_pc_owner;

	i_update = 0;
	str_name = "node";
	i_id = 0;
}

CT_PORT::~CT_PORT() {
	f_clear();

#if 1
	/* Clear callback */
	{
		//CT_GUARD_LOCK c_guard(_c_rw_lock.write());
		c_cb_rx = NULL;
		c_cb_tx = NULL;
	}
#endif
}

int CT_PORT::f_set_url(CT_NODE & in_c_url) {
	int ec;
	CT_NODE::childs_result v_tmp = in_c_url.get_childs("url");

	/* Assert if no url found, should be check on upper layer */
	M_ASSERT(v_tmp.size());

	/* Adjust size between vector and c onfig*/
	if (_v_ports.size() != v_tmp.size()) {
		//D("Allocating %d sub ports", v_tmp.size());
		_v_ports.resize(v_tmp.size());
	}

	/* Link transport for each url */
	int i_index = 0;
	for (CT_NODE::childs_iterator pv_it = v_tmp.begin(); pv_it != v_tmp.end();
			pv_it++, i_index++) {
		CT_PORT_INTERNAL & c_int = _v_ports[i_index];
		//D("Configuring sub port %d : %p", i_index, &c_int);

		/* Configure Internal port */
		ec = c_int.f_set_url(**pv_it);
		if (ec != EC_SUCCESS) {
			CRIT("Unable to configure port %s",
					((std::string)(**pv_it)).c_str());
			ec = EC_FAILURE;
			goto out_err;
		}

		/* Set call back of port */
		ec = c_int.f_register_cb(c_receive);
		if (ec != EC_SUCCESS) {
			CRIT("Unable to register cb of port %s",
					((std::string)(**pv_it)).c_str());
			ec = EC_FAILURE;
			goto out_err;
		}

	}
	ec = EC_SUCCESS;
	out_err: return ec;
}

int CT_PORT::f_register_cb_rx(CT_PORT_BIND_CORE const & in_c_bind) {
	//CT_GUARD_LOCK c_guard(_c_rw_lock.write());
	c_cb_rx = in_c_bind;
	return EC_SUCCESS;
}

int CT_PORT::f_register_cb_tx(CT_PORT_BIND_CORE const & in_c_bind) {
	//CT_GUARD_LOCK c_guard(_c_rw_lock.write());
	c_cb_tx = in_c_bind;
	return EC_SUCCESS;
}

int CT_PORT::f_receive(CT_GUARD<CT_PORT_NODE> const & in_c_node) {
	int ec;
	//uint64_t i_time_start = f_get_time_ns64();
	//uint64_t i_time_start_cb, i_time_end_cb;
	//D("Port %s", str_name.c_str());
	/* Backup context */
	CT_PORT * pc_tmp = CT_HOST_CONTEXT::context_port;
	/* Set current port as reply port in this context*/
	CT_HOST_CONTEXT::context_port = this;
	//D("Calling %p CB:%p CORE:%p PORT:%p",this,_s_cb.pf_cb, _s_cb.pc_core, _s_cb.pc_port);

	/* Read lock */
	//CT_GUARD_LOCK c_guard(_c_rw_lock.read());

	/* Execute cb */
	if (c_cb_rx) {
		ec = c_cb_rx(in_c_node);
	} else {
		ec = EC_SUCCESS;
	}
	/* Restore context */
	CT_HOST_CONTEXT::context_port = pc_tmp;

#if 0
	uint64_t i_time_end = f_get_time_ns64();
	if(float(i_time_end - i_time_start)/1e6 > 1) {
		_DBG << "Time receive :" << float(i_time_end - i_time_start)/1e6 << " " << float(i_time_end_cb - i_time_start_cb)/1e6 << " "<< in_c_node.get();
	}
#endif
	return ec;
}

int CT_PORT::f_push(CT_GUARD<CT_PORT_NODE> & in_c_node) {
	int ec;
	//D("Push port node %d %p %p %s %d",in_c_node->get_id(), CT_HOST_CONTEXT::context, (CT_PORT_BIND_CORE const *)c_receive, str_name.c_str(), i_id);

	/* Push node on context */
	ec = CT_HOST_CONTEXT::context->f_push(c_receive, in_c_node);
	if (ec != EC_SUCCESS) {
		CRIT("Unable to push node");
		goto out_err;
	}
	out_err: ec = EC_SUCCESS;
	return ec;
}

int CT_PORT::f_send(CT_GUARD<CT_PORT_NODE> const  & in_c_node) {
	int ec;
	std::vector<CT_PORT_INTERNAL>::iterator pc_it;

	for (pc_it = _v_ports.begin(); pc_it != _v_ports.end(); pc_it++) {
		ec = pc_it->f_send(in_c_node);
		if (ec != EC_SUCCESS) {
			CRIT("Unable to send data on port");
			ec = EC_FAILURE;
			goto out_err;
		}
	}

	if (c_cb_tx) {

		CT_PORT * pc_tmp = CT_HOST_CONTEXT::context_port;
		/* Set current port as reply port in this context*/
		CT_HOST_CONTEXT::context_port = this;

		ec = c_cb_tx(in_c_node);
		if (ec != EC_SUCCESS) {
			CRIT("Unable to run tx cb data on port");
			ec = EC_FAILURE;
			goto out_err;
		}

		/* Restore context */
		CT_HOST_CONTEXT::context_port = pc_tmp;
	}

	ec = EC_SUCCESS;
	out_err: return ec;
}

int CT_PORT::f_clear(void) {
	int ec;

	/* Clear port */
	{
		_v_ports.clear();
	}
	// DO NOT CLEAR CALLBACK

	ec = EC_SUCCESS;
	//out_err:
	return ec;
}
