/***********************************************************************
 ** Context
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
#ifndef CONTEXT_HH_
#define CONTEXT_HH_

/**
 * @file context.hh
 *
 * @author SEAGNAL (johann.baudy@seagnal.fr)
 * @date Jan 27, 2013
 *
 * @version 1.0 Original version
 */

/***********************************************************************
 * Includes
 ***********************************************************************/
#include <c/common.h>
#include <port.hh>
#include <cpp/lock.hh>
#include <cpp/bind.hh>
/***********************************************************************
 * Defines
 ***********************************************************************/
class CT_HOST;
class CT_HOST_CONTEXT;

enum ET_CONTEXT_STATUS {
		E_CONTEXT_STATUS_RUNNING = 1,
		E_CONTEXT_STATUS_STOPPED,
		E_CONTEXT_STATUS_IDLE,
	};

struct ST_HOST_CONTEXT_CB_EXTRA {
	CT_PORT_BIND_CORE const * pc_sb;
	CT_GUARD<CT_PORT_NODE> pc_data;
};

#if 0
typedef int (*FT_HOST_CONTEXT_CB)(void * in_pv_arg);
struct ST_HOST_CONTEXT_CB {
	FT_HOST_CONTEXT_CB pf_cb;
	void * pv_arg;
};


#define M_CONTEXT_CREATE_CB(FUNCTION_NAME, CLASS_NAME, METHOD_NAME) \
extern "C" int FUNCTION_NAME(void * in_pv_arg) { \
	return static_cast<CLASS_NAME*>(in_pv_arg)->METHOD_NAME(); \
} \

#define M_CONTEXT_CREATE_CB_EXTRA(FUNCTION_NAME, CLASS_NAME, METHOD_NAME) \
extern "C" int FUNCTION_NAME(CT_CORE * in_pc_core, CT_NODE & in_c_data) { \
	return static_cast<CLASS_NAME*>(in_pc_core)->METHOD_NAME(); \
} \

#endif

#if 0
template <typename T>
using CT_CONTEXT_BIND = CT_BIND< int, T, CT_HOST_CONTEXT& >;
using CT_CONTEXT_BIND_CORE = CT_BIND_CORE< int, CT_HOST_CONTEXT& >;
using CT_CONTEXT_SLOT = CT_SLOT<int, void>;
#else
typedef CT_BIND_CORE< int, CT_HOST_CONTEXT& > CT_CONTEXT_BIND_CORE;
typedef CT_SLOT<int, CT_HOST_CONTEXT&> CT_CONTEXT_SLOT;
#define M_CONTEXT_BIND(T, F, P) CT_BIND<int, T, CT_HOST_CONTEXT& >(&T ::F , P)
#endif
/***********************************************************************
 * Types
 ***********************************************************************/
class CT_HOST_CONTEXT {


	/* Extra list */
	std::list<ST_HOST_CONTEXT_CB_EXTRA> _l_list_extra;

	/* Main context call back */
	CT_CONTEXT_SLOT _c_cb;

	/* lock extra context */
	CT_SPINLOCK _c_lock;

	/* Timeout default */
	int _i_timeout;

protected:
	/* Context status */
	enum ET_CONTEXT_STATUS _e_status;
	/* Stop command */
	bool _b_stop;

protected:
	/* Execute extra callback */
	void f_excute(void);

public:
	CT_HOST_CONTEXT();
	virtual ~CT_HOST_CONTEXT();

	/* Start context on specified cb */
	int f_start(CT_CONTEXT_BIND_CORE const & in_c_db);

	/* Custom host methods - start */
	virtual int f_init(void);

	/* Custom host methods - join */
	virtual int f_join(void);

	/* Custom host methods - set_priority */
	virtual int f_set_priority(int16_t in_i_priority);

	/* Push extra callback */
	int f_push_extra(struct ST_HOST_CONTEXT_CB_EXTRA & in_s_cb);

	/* Execute core */
	void f_execute(void);

	/* Execute core no loop */
	int f_execute_single(void);

	/* Stopping core */
	void f_stop(void);

	/* Static variable */
	static __thread CT_HOST_CONTEXT * context;

	/* Current port context */
	static __thread CT_PORT * context_port;

	/* REply id */
	static __thread uint32_t context_reply_id;

	/* Static push on current context */
	static int f_push(CT_PORT_BIND_CORE const * in_c_cb, CT_GUARD<CT_PORT_NODE> const & in_pc_data);

	/* Return status of current context */
	enum ET_CONTEXT_STATUS f_get_status();

	void f_set_timeout(int i_value);
	int f_get_timeout(void);

	/* has extra */
	bool f_has_extra(void) {
		return _l_list_extra.size();
	}
	int f_size_extra(void) {
		return _l_list_extra.size();
	}
};
#else
class CT_HOST_CONTEXT;
#endif /* CONTEXT_HH_ */
