/***********************************************************************
 ** task
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
 * @file task.cc
 *
 * @author SEAGNAL (johann.baudy@seagnal.fr)
 * @date Aug 1, 2013
 *
 * @version 1.0 Original version
 */

/**********************************	*************************************
 * Includes
 ***********************************************************************/
#include <task.hh>

using namespace master::core;

std::ostream& operator<<(std::ostream& stream,
		ST_TASK_STATUS const & status) {
	return stream << std::hex << status.i_luid << ":" << std::hex << status.i_muid << "@"<< status.i_step;
}

std::ostream& operator<<(std::ostream& stream,
		CT_TASK::task_key_t const & key) {
	return stream << std::hex << key.first << ":" << std::hex << key.second;
}

CT_TASK::CT_TASK(task_key_t const & in_c_uuid) :
		s_usage(false, this), _c_reply_node(E_ID_TASK_STATUS) {
	_c_uid = in_c_uuid;
	_i_type = 0;
	_b_reply_status = false;
	_i_notification_id = E_ID_TASK_STATUS;
	_pc_notification_external_port = NULL;
	_pc_notification_internal_port = NULL;
	_pc_timeout = CT_HOST::host->f_new_timeout();
	M_ASSERT(_pc_timeout);

	_str_name = "unknown";

	_e_last_status = E_TASK_STATE_INIT;
	_i_last_progress = 0;
	_str_last_msg = "";

	/* Default, Create one step with one part */
	f_step_set_nb(1);
}

CT_TASK::~CT_TASK() {
	//_WARN << "DELETING task :" << this;
	//_DBG << *_c_reply_node;
	delete _pc_timeout;
	_pc_timeout = NULL;
}

void CT_TASK::f_set_name(std::string const & in_str_name) {
	//D("%p:%s", this, in_str_name.c_str());
	_str_name = in_str_name;
	on_update();
}

void CT_TASK::f_set_type(uint32_t const & in_i_type) {
	_i_type = in_i_type;
}

/* Notification methods */
void CT_TASK::f_notification_set_id(uint32_t in_i_notification_id) {
	_i_notification_id = in_i_notification_id;
}

void CT_TASK::f_notification_set_internal_port(
		CT_PORT & in_c_notification_internal_port) {
	_pc_notification_internal_port = &in_c_notification_internal_port;
}
void CT_TASK::f_notification_set_external_port(
		CT_PORT & in_c_notification_external_port) {
	_pc_notification_external_port = &in_c_notification_external_port;
}

void CT_TASK::f_notification_set_cb(CT_PORT_BIND_CORE const & in_c_cb) {
	_c_notification_cb = in_c_cb;
}

void CT_TASK::f_notification_set_cb(std::nullptr_t) {
	_c_notification_cb = NULL;
}

/* Describe task */
void CT_TASK::f_step_set_nb(uint32_t in_nb_step) {
	_v_steps.resize(in_nb_step);
}

uint32_t CT_TASK::f_step_get_nb(void) {
	return _v_steps.size();
}

CT_TASK_STEP & CT_TASK::f_step_get(uint32_t in_i_step) {
	if (_v_steps.size() <= in_i_step) {
		f_step_set_nb(in_i_step + 1);
	}
	return _v_steps[in_i_step];
}

/* Timeout method */
void CT_TASK::f_timeout_set(uint32_t in_i_ms) {

	CT_PORT_NODE_GUARD c_tmp(_i_notification_id);
	_pc_timeout->f_set_cb(*_pc_notification_internal_port, c_tmp);
	_pc_timeout->f_start(in_i_ms);
}

/* Status method */
void CT_TASK::f_status_init(CT_PORT_NODE& in_c_node, uint32_t in_i_step) {
	ST_TASK_STATUS s_status;
	//D("%d %d (%d %d)",	in_i_step, in_i_part, _v_steps.size(), f_step_get(in_i_step).f_part_get_nb());

	M_ASSERT(_v_steps.size() > in_i_step);

	s_status.i_luid = _c_uid.first;
	s_status.i_muid = _c_uid.second;
	s_status.i_step = in_i_step;
	s_status.__dummy = 0;
	in_c_node(E_ID_TASK_STATUS).set_data(s_status);
}

void CT_TASK::f_status_init(ST_TASK_STATUS& in_s_status, uint32_t in_i_step) {

	in_s_status.i_luid = _c_uid.first;
	in_s_status.i_muid = _c_uid.second;
	in_s_status.i_step = in_i_step;
}

ST_TASK_STATUS CT_TASK::f_status_init(uint32_t in_i_step) {
	ST_TASK_STATUS s_tmp;
	s_tmp.i_luid = _c_uid.first;
	s_tmp.i_muid = _c_uid.second;
	s_tmp.i_step = in_i_step;
	s_tmp.__dummy = 0;
	return s_tmp;
}

bool CT_TASK::f_status_is_complete(void) {
	std::vector<CT_TASK_STEP>::reverse_iterator pc_it;
	for (pc_it = _v_steps.rbegin(); pc_it != _v_steps.rend(); pc_it++) {
		if (pc_it->f_is_complete() != true) {
			return false;
		}
	}
	return true;
}

float CT_TASK::f_status_get_progress(void) {
	float f_complete = 0;
	std::vector<CT_TASK_STEP>::iterator pc_it;
	for (pc_it = _v_steps.begin(); pc_it != _v_steps.end(); pc_it++) {
		f_complete += pc_it->f_get_progress();
	}

	return f_complete / float(_v_steps.size());
}

/* Reply methods */
CT_PORT_NODE_GUARD & CT_TASK::f_reply_get_node(void) {
	return _c_reply_node;
}

void CT_TASK::f_reply_set_parent_status(ST_TASK_STATUS & in_s_status) {
	_s_reply_status = in_s_status;
	_b_reply_status = true;
}
void CT_TASK::f_reply_set_parent_status(CT_PORT_NODE & in_c_node) {
	if (in_c_node.has(E_ID_TASK_STATUS)) {
		_s_reply_status =
				in_c_node(E_ID_TASK_STATUS).get_data<ST_TASK_STATUS>();
		_b_reply_status = true;
	}
}
bool CT_TASK::f_reply_has_parent_status(void) {
	return _b_reply_status;
}

ST_TASK_STATUS & CT_TASK::f_reply_get_parent_status(void) {
	M_ASSERT(_b_reply_status);
	return _s_reply_status;
}

ST_TASK_STATUS & CT_TASK::f_reply_get_status(CT_PORT_NODE & in_c_from) {
	M_ASSERT(f_reply_has_status(in_c_from));
	return in_c_from(E_ID_TASK_STATUS).map<ST_TASK_STATUS>();
}

bool CT_TASK::f_reply_has_status(CT_PORT_NODE & in_c_from) {
	return (in_c_from.has(E_ID_TASK_STATUS));
}

/// TO DELETE START - CRADO
void CT_TASK::f_reply_set_parent_task(CT_PORT_NODE & in_c_node) {
	if (in_c_node.has(E_ID_TASK_STATUS)) {
//		D("Forward task info to reply");
		CT_PORT_NODE_GUARD & c_port_node = f_reply_get_node();
		f_reply_copy_status(*c_port_node, in_c_node);
	}
}
void CT_TASK::f_reply_copy_status(CT_PORT_NODE & in_c_to,
		CT_PORT_NODE & in_c_from) {
	in_c_to(E_ID_TASK_STATUS).copy(in_c_from(E_ID_TASK_STATUS));
}
/// TO DELETE END

/********** NOTIFICATION *************/
/* Processing methods */
void CT_TASK::f_enable() {
	if (f_status_is_complete()) {
		f_notify(E_TASK_STATUS_TASK_COMPLETE, f_status_init(0));
	}
}

void CT_TASK::f_error(void) {
	//CRIT("ERROR");
	f_error(f_status_init(0), "");
}

void CT_TASK::f_error(std::string const & in_str_msg) {

	//CRIT("ERROR");
	f_error(f_status_init(0), in_str_msg);
}

void CT_TASK::f_complete(void) {

	//CRIT("COMPLETE");
	f_complete(f_status_init(0));
}

void CT_TASK::f_abort(void) {

	//CRIT("ERROR");
	f_abort(f_status_init(0));
}

void CT_TASK::f_kill(void) {

	//CRIT("ERROR");
	f_kill(f_status_init(0));
}

void CT_TASK::f_error(ST_TASK_STATUS const & in_s_status,
		std::string const & in_str_msg) {
	_CRIT << in_str_msg;
	f_notify(E_TASK_STATUS_ERROR, in_s_status, 0, in_str_msg);
}

void CT_TASK::f_complete(uint32_t in_i_step, uint32_t in_i_part) {
	f_complete(f_status_init(in_i_step));
}

void CT_TASK::f_complete(ST_TASK_STATUS const & in_s_status) {
	//D("Update complete %lld %d %d", in_s_status.i_uuid, in_s_status.i_step, in_s_status.i_part);
	M_ASSERT(_v_steps.size() > in_s_status.i_step);

	CT_TASK_STEP & c_step = f_step_get(in_s_status.i_step);

	c_step.f_set_progress(100);

	f_notify(E_TASK_STATUS_PART_COMPLETE, in_s_status);

	if (c_step.f_is_complete()) {
		//D("Step complete");
		f_notify(E_TASK_STATUS_STEP_COMPLETE, in_s_status);

		if (f_status_is_complete()) {
			//D("Task %s (%lld) complete", _str_name.c_str(), _c_uid.first);
			f_notify(E_TASK_STATUS_TASK_COMPLETE, in_s_status);
		}
	}

}

void CT_TASK::f_abort(ST_TASK_STATUS const & in_s_status) {
	//_DBG << "Abort task: " << _c_uid;
	switch (_e_last_status) {
	case E_TASK_STATE_ABORTING:
		_DBG << "Abort task: E_TASK_STATE_ABORTING";
		break;
	case E_TASK_STATE_ERROR:
		_DBG << "Abort task: E_TASK_STATE_ERROR";
		f_notify(E_TASK_STATUS_KILLED, in_s_status);
		break;
	default:
		_DBG << "Abort task: default";
		f_notify(E_TASK_STATUS_ABORTED, in_s_status);
		break;
	}
}

void CT_TASK::f_kill(ST_TASK_STATUS const & in_s_status) {
	f_notify(E_TASK_STATUS_KILLED, in_s_status);
}


void CT_TASK::f_progress(float const in_f_progress, std::string const & in_str_msg) {
	f_progress(f_status_init(0), in_f_progress, in_str_msg);
}

void CT_TASK::f_progress(uint32_t in_i_step, uint32_t in_i_part, float const in_f_progress, std::string const & in_str_msg) {
	f_progress(f_status_init(in_i_step), in_f_progress, in_str_msg);
}

void CT_TASK::f_progress(ST_TASK_STATUS const & in_s_status,
		float const in_f_progress, std::string const & in_str_msg) {

	f_notify(E_TASK_STATUS_PROGRESS, in_s_status, in_f_progress, in_str_msg);
}

void CT_TASK::f_notify(ST_TASK_NOTIFICATION const & in_s_notif,
		std::string const & in_str_msg) {
	bool b_notify = false;
	CT_PORT_NODE_GUARD c_node(_i_notification_id);
	c_node->set_data<ST_TASK_NOTIFICATION>(in_s_notif);

	if (in_s_notif.e_status == E_TASK_STATUS_PROGRESS) {

		if (in_s_notif.f_progress < 0) {
			_i_last_progress = -1;
		} else {
			CT_TASK_STEP & c_step = f_step_get(in_s_notif.s_status.i_step);
			c_step.f_set_progress(in_s_notif.f_progress);
			_i_last_progress = f_status_get_progress();
		}
	}

	_str_last_msg = in_str_msg;

	switch (in_s_notif.e_status) {
	case E_TASK_STATUS_ABORTED:
		_e_last_status = E_TASK_STATE_ABORTING;
		break;
	case E_TASK_STATUS_KILLED:
	case E_TASK_STATUS_TIMEOUT:
	case E_TASK_STATUS_ERROR:
		_e_last_status = E_TASK_STATE_ERROR;
		break;
	case E_TASK_STATUS_TASK_COMPLETE:
		_e_last_status = E_TASK_STATE_COMPLETE;
		break;
	default:
		_e_last_status = E_TASK_STATE_RUNNING;
		break;
	}

	/* Add msg */
	if (in_str_msg.size()) {
		c_node->add(E_ID_TASK_STATUS_MSG)->set_data<std::string>(in_str_msg);
	}

	/* Send notification */
	if (_pc_notification_internal_port) {
		_pc_notification_internal_port->f_push(c_node);
		b_notify = true;
	}

	if (_pc_notification_external_port) {
		_pc_notification_external_port->f_send(c_node);
		b_notify = true;
	}

	if (_c_notification_cb) {
		_c_notification_cb(c_node);
		b_notify = true;
	}

	on_update();

	if(!b_notify) {
		switch (in_s_notif.e_status) {
		case E_TASK_STATUS_ABORTED:
		case E_TASK_STATUS_KILLED:
		case E_TASK_STATUS_TIMEOUT:
		case E_TASK_STATUS_ERROR:
		case E_TASK_STATUS_TASK_COMPLETE: {
			CT_GUARD<CT_TASK> c_tmp(&s_usage);
			CT_HOST::tasks->f_release(c_tmp);
		break;
		}
	}
	}
}

void CT_TASK::f_notify(uint32_t const in_i_id,
		ST_TASK_STATUS const & in_s_status, float in_f_percent,
		std::string const & in_str_msg) {
	//_DBG << "Task notification " << _V(in_i_id)<< _V(in_s_status.i_luid) << _V(in_s_status.i_muid);

	struct ST_TASK_NOTIFICATION s_notification;

	s_notification.s_status = in_s_status;

	s_notification.e_status = (ET_TASK_STATUS) in_i_id;

	s_notification.f_progress = in_f_percent;

	f_notify(s_notification, in_str_msg);

}

bool CT_TASK::f_is_running(void) {
	switch (_e_last_status) {
	case E_TASK_STATE_INIT:
	case E_TASK_STATE_RUNNING:
		return true;
	default:
		return false;
	}
}

bool CT_TASK::f_is_error(void) {
	switch (_e_last_status) {
	case E_TASK_STATE_ERROR:
		return true;
	default:
		return false;
	}
}

void CT_TASK::on_update(void) {

}

void CT_TASK::on_delete(void) {
	delete this;
}

#if 0
void CT_TASK_COMMON::f_set_progress(float in_f_percent) {
	_c_task->f_step_get(0).f_set_progress(in_f_percent*100);
}

void CT_TASK_COMMON::f_set_complete(void) {
	_c_task->f_complete(0);
}

bool CT_TASK_COMMON::f_is_running() {
	return _c_task ? _c_task->f_is_running() : false;
}
#endif
#if 1
/* Customize GUARD init - speed improvement due to usage allocation */
template<>
CT_GUARD<CT_TASK>::CT_GUARD(CT_TASK * in_pointer) {
	//_WARN << this;
	init(&(in_pointer->s_usage));
}

template<>
void CT_GUARD<CT_TASK>::delete_pointer(void) {
	// _WARN << this;
	_pc_pointer->on_delete();
}

#endif

CT_HOST_TASK_MANAGER::CT_HOST_TASK_MANAGER() {
	_i_uid = 0;
	_i_manager_uid = rand();
	M_ASSERT(_i_manager_uid);

}


CT_HOST_TASK_MANAGER::~CT_HOST_TASK_MANAGER() {
	M_ASSERT(!_m_tasks.size());
}

void CT_HOST_TASK_MANAGER::f_clear( void) {
	while (_m_tasks.size()) {
		_m_tasks_iterator_t pc_it = _m_tasks.begin();
		_WARN << pc_it->second->f_get_name()
				<< " task is still UP, releasing ...";
		f_release(pc_it->second);
	}
}

CT_GUARD<CT_TASK> CT_HOST_TASK_MANAGER::f_get(
		CT_TASK::task_key_t const & in_c_uuid) {
	CT_GUARD<CT_TASK> c_tmp;
	CT_GUARD_LOCK c_guard(_c_spinlock);
	_m_tasks_iterator_t pc_it = _m_tasks.find(in_c_uuid);
	if (pc_it != _m_tasks.end()) {
		c_tmp = pc_it->second;
	}
	return c_tmp;
}

CT_GUARD<CT_TASK> CT_HOST_TASK_MANAGER::f_get(
		ST_TASK_STATUS const & in_s_status) {
	return f_get(CT_TASK::task_key_t(in_s_status.i_luid, in_s_status.i_muid));
}

bool CT_HOST_TASK_MANAGER::f_has(CT_TASK::task_key_t const & in_c_uuid) {
	bool b_has;
	CT_GUARD_LOCK c_guard(_c_spinlock);
	b_has = (_m_tasks.find(in_c_uuid) != _m_tasks.end());
	return b_has;
}

bool CT_HOST_TASK_MANAGER::f_has(ST_TASK_STATUS const & in_s_status) {
	return f_has(CT_TASK::task_key_t(in_s_status.i_luid, in_s_status.i_muid));
}

void CT_HOST_TASK_MANAGER::f_release(CT_GUARD<CT_TASK> in_c_task) {
	M_ASSERT(in_c_task.get());
	CT_GUARD_LOCK c_guard(_c_spinlock);
#if 0
	_DBG << "Releasing task :" << in_c_task->f_get_uid().first << ","
			<< in_c_task->f_get_uid().second << ", Name: "
			<< in_c_task->f_get_name();
#endif
	M_ASSERT(_m_tasks.find(in_c_task->f_get_uid()) != _m_tasks.end());
	_m_tasks.erase(in_c_task->f_get_uid());
	on_release(in_c_task);

}


CT_GUARD<CT_TASK> CT_HOST_TASK_MANAGER::f_new_task(
		CT_TASK::task_key_t const & in_c_uuid) {
	return CT_GUARD<CT_TASK>(new CT_TASK(in_c_uuid));
}

void CT_HOST_TASK_MANAGER::on_new(CT_GUARD<CT_TASK> & in_pc_task) {

}
void CT_HOST_TASK_MANAGER::on_release(CT_GUARD<CT_TASK> & in_pc_task) {

}

void CT_HOST_TASK_MANAGER::f_set_manager_uid(uint64_t in_i_muid) {
	_i_manager_uid = in_i_muid;
}

uint64_t CT_HOST_TASK_MANAGER::f_get_manager_uid(void) {
	return _i_manager_uid;
}

bool CT_HOST_TASK_MANAGER::f_has_error(void) {
	CT_GUARD_LOCK c_guard(_c_spinlock);
	for (_m_tasks_iterator_t pc_it = _m_tasks.begin();
			pc_it != _m_tasks.end(); pc_it++) {
		if(pc_it->second->f_get_status() == E_TASK_STATE_ERROR) {
			return true;
		}
	}
	return false;
}
