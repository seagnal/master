/***********************************************************************
 ** context
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
 * @file resource.cc
 *
 * @author SEAGNAL (johann.baudy@seagnal.fr)
 * @date Jan 27, 2013
 *
 * @version 1.0 Original version
 */

/***********************************************************************
 * Includes
 ***********************************************************************/
#include <context.hh>
#include <host.hh>

/***********************************************************************
 * CT_RESSOURCE CLASS
 ***********************************************************************/
__thread CT_HOST_CONTEXT * CT_HOST_CONTEXT::context = NULL;
__thread CT_PORT * CT_HOST_CONTEXT::context_port = NULL;
__thread uint32_t CT_HOST_CONTEXT::context_reply_id = 0;
#define LF_ABORT_SHIELD

CT_HOST_CONTEXT::CT_HOST_CONTEXT() {
	_b_stop = true;
	_e_status = E_CONTEXT_STATUS_STOPPED;
	_i_timeout = 100;

	//_DBG << _V(this);
	//CT_DEBUG::f_print_backtrace_cxx();

	/* Registering context */
	if (CT_HOST::host) {
		int ec;
		ec = CT_HOST::host->f_context_register(this);
		M_ASSERT(ec == EC_SUCCESS);
	}
}

CT_HOST_CONTEXT::~CT_HOST_CONTEXT() {
	/* UnRegistering context */
	if (this != CT_HOST::host) {

		int ec;
		ec = CT_HOST::host->f_context_unregister(this);
		M_ASSERT(ec == EC_SUCCESS);
	}
	/* Context must be stopped in derived class */
}

int CT_HOST_CONTEXT::f_start(CT_CONTEXT_BIND_CORE const & in_c_db) {
	_c_cb = in_c_db;
	return f_init();
}

int CT_HOST_CONTEXT::f_init(void) {
	M_BUG();
	return EC_FAILURE;
}
int CT_HOST_CONTEXT::f_join(void) {
	M_BUG();
	return EC_FAILURE;
}

int CT_HOST_CONTEXT::f_set_priority(int16_t in_i_priority) {
	M_BUG();
	return EC_FAILURE;
}


int CT_HOST_CONTEXT::f_push_extra(struct ST_HOST_CONTEXT_CB_EXTRA & in_s_cb) {
	CT_GUARD_LOCK c_guard(_c_lock);
	//_DBG << *in_s_cb.pc_data;
	_l_list_extra.push_back(in_s_cb);

	//_DBG << "Push extra: "<< _V(_l_list_extra.size())<< _V( in_s_cb.s_sb.pc_core)<< _V( in_s_cb.s_sb.pc_port);
	//D("Push back %d (%s,%s) %x", _l_list_extra.size(), in_s_cb.s_sb.pc_port->f_get_name().c_str(), in_s_cb.s_sb.pc_core->f_get_name().c_str(),  in_s_cb.pc_data->get_id());
	return EC_SUCCESS;
}

int CT_HOST_CONTEXT::f_push(CT_PORT_BIND_CORE const * in_pc_cb,
		CT_GUARD<CT_PORT_NODE> const & in_pc_data) {
	struct ST_HOST_CONTEXT_CB_EXTRA s_cb { in_pc_cb, in_pc_data };
	CT_HOST_CONTEXT::context->f_push_extra(s_cb);
	return EC_SUCCESS;
}

int CT_HOST_CONTEXT::f_execute_single(void) {
	int ec;

	/* Store context in local variable */
	CT_HOST_CONTEXT::context = this;

	_e_status = E_CONTEXT_STATUS_RUNNING;
	//D("%p - Running", this);

	/* Execute main context callback */
#ifdef LF_ABORT_SHIELD
	try {
#endif
		ec = _c_cb(*this);
#ifdef LF_ABORT_SHIELD
	} catch (const std::exception & e) {
		if (getenv("RELEASE_MODE") != NULL) {
			_CRIT << "EXCEPTION: " << e.what(); // uniquement en debug
		} else {
			_FATAL << "EXCEPTION: " << e.what(); // uniquement en debug
		}
		//exit(-1);
		ec = EC_SUCCESS;
	}
#endif

	/* Execute extra on success */
	{
		CT_GUARD_LOCK c_guard(_c_lock);
		if (_l_list_extra.size()) {

			/* Execute extra callbacks */

			/* Loop while extra to execute */
			//_DBG << "Extra:" << _l_list_extra.size();
			while (1) {
				struct ST_HOST_CONTEXT_CB_EXTRA s_cb_extra;
				std::list<struct ST_HOST_CONTEXT_CB_EXTRA>::iterator ps_it;

				/* Break end of list */
				ps_it = _l_list_extra.begin();
				if (ps_it == _l_list_extra.end()) {
					break;
				} else {
					M_ASSERT(ps_it->pc_data);
					M_ASSERT(ps_it->pc_sb);
					s_cb_extra = *ps_it;
					_l_list_extra.pop_front();
				}
#if 0
				// BUG .....
				if(CT_HOST::host->f_context_is_idling()) {
					_CRIT << "Pushed node during idle : "<< s_cb_extra.pc_data->get_id() << _V(this);
				}
#endif

				//D("%p - Pop front %d (%p,%p,%s) %x",this, _l_list_extra.size(), ps_it->pc_data, ps_it->s_sb.pc_port, ps_it->s_sb.pc_core->f_get_name().c_str(), ps_it->pc_data->get_id());
				/* Execute call back */
#ifdef LF_ABORT_SHIELD
				try {
#endif
					/* During Idle ignore nodes that are not dedicated to Core communication */
					if(CT_HOST::host->f_context_is_idling())
					{
						if(s_cb_extra.pc_data->get_id() > E_ID_CORE_NB_ID) {
							continue;
						}
					}

					{
						CT_GUARD_UNLOCK c_guard_unlock(c_guard);
						s_cb_extra.pc_sb->operator ()(s_cb_extra.pc_data);
					}
#ifdef LF_ABORT_SHIELD

				} catch (const std::exception & e) {
					if (getenv("RELEASE_MODE") != NULL) {
						_CRIT << "EXCEPTION PUSH " << e.what();
					} else {
						_FATAL << "EXCEPTION PUSH: " << e.what(); // uniquement en debug
					}
					//exit(-1);
					ec = EC_SUCCESS;
				}
#endif
			}

			//_DBG << _l_list_extra.size();
		}
	}

	return ec;
}

void CT_HOST_CONTEXT::f_execute(void) {
	int ec = EC_BYPASS;
	_b_stop = false;

//_WARN << "Started context "<<_V(this);
//			CT_HOST::host->f_context_is_idling(), _b_stop);
	while (!_b_stop) {
		if (CT_HOST::host->f_context_is_idling() && (ec == EC_BYPASS)) {
			/* Last executed cb return BYPASS  and Idle mode enabled */
			_e_status = E_CONTEXT_STATUS_IDLE;
			//_DBG << "Sleeping Idle: " << this;
			usleep(100000);
		} else {
			ec = f_execute_single();
		}
	}
	_e_status = E_CONTEXT_STATUS_STOPPED;
	_WARN << "End of context "<<_V(this);
//_WARN << "started co"
//	D("End of context i:%d s:%d",
//			CT_HOST::host->f_context_is_idling(), _b_stop);
}

void CT_HOST_CONTEXT::f_stop(void) {
	_b_stop = true;
	f_join();
}

enum ET_CONTEXT_STATUS CT_HOST_CONTEXT::f_get_status() {
	return _e_status;
}

#if 0
int CT_HOST_CONTEXT::f_set_idle_state(bool in_b_idle) {
	_b_idle = in_b_idle;
	return EC_SUCCESS;
}
#endif

void CT_HOST_CONTEXT::f_set_timeout(int in_i_value) {
	_i_timeout = in_i_value;
}

int CT_HOST_CONTEXT::f_get_timeout(void) {
	return _i_timeout;
}
