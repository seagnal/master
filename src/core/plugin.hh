/***********************************************************************
 ** plugin.hh
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
#ifndef PLUGIN_HH_
#define PLUGIN_HH_
/**
 * @file plugin.hh
 * Plugin definition.
 *
 * @author SEAGNAL (johann.baudy@seagnal.fr)
 * @date 2012
 *
 * @version 1.0 Original version
 */

/***********************************************************************
 * Includes
 ***********************************************************************/

#include <node.hh>
#include <c/common.h>

/***********************************************************************
 * Defines
 ***********************************************************************/

/***********************************************************************
 * Types
 ***********************************************************************/

class CT_HOST;
class CT_CORE;
struct ST_PLUGIN_DESCRIPTION;

typedef void* (*FT_PLUGIN_NEW)(void);
typedef struct ST_PLUGIN_DESCRIPTION * (*FT_PLUGIN_INFO)(void);
typedef int (*FT_PLUGIN_EXTRA)(void);

enum ET_PLUGIN_TYPE {
	E_PLUGIN_TYPE_CORE = 0, E_PLUGIN_TYPE_RESOURCE, E_PLUGIN_TYPE_TRANSPORT, E_PLUGIN_TYPE_OTHER,
};

struct ST_PLUGIN_INFO {
	struct ST_PLUGIN_DESCRIPTION * ps_desc;
	std::string str_library_path;
	uint64_t i_time_modification;
	bool b_updated;
	void * pv_handle;
};

struct ST_PLUGIN_DESCRIPTION {
	std::string str_name;
	enum ET_PLUGIN_TYPE e_type;
	std::string str_version;
	FT_PLUGIN_NEW cb_new;
	FT_PLUGIN_INFO cb_info;
	FT_PLUGIN_EXTRA cb_load;
	FT_PLUGIN_EXTRA cb_unload;
};

/***********************************************************************
 * Macros
 ***********************************************************************/


#define M_PLUGIN_CORE(CLASS_NAME, PLUGIN_NAME, PLUGIN_VERSION) M_PLUGIN_EXT(CLASS_NAME, CT_CORE, PLUGIN_NAME, E_PLUGIN_TYPE_CORE, PLUGIN_VERSION , NULL, NULL)
#define M_PLUGIN_RESOURCE(CLASS_NAME, PLUGIN_NAME, PLUGIN_VERSION) M_PLUGIN_EXT(CLASS_NAME, CT_RESOURCE, PLUGIN_NAME, E_PLUGIN_TYPE_RESOURCE, PLUGIN_VERSION , NULL, NULL)
#define M_PLUGIN_TRANSPORT(CLASS_NAME, PLUGIN_NAME, PLUGIN_VERSION) M_PLUGIN_EXT(CLASS_NAME, CT_TRANSPORT, PLUGIN_NAME, E_PLUGIN_TYPE_TRANSPORT, PLUGIN_VERSION , NULL, NULL)


#define M_PLUGIN_EXT(CLASS_NAME, CLASS_TYPE, PLUGIN_NAME, PLUGIN_TYPE, PLUGIN_VERSION , CB_LOAD, CB_UNLOAD) \
static struct ST_PLUGIN_DESCRIPTION gs_desc = { PLUGIN_NAME, PLUGIN_TYPE, PLUGIN_VERSION, f_plugin_new, f_plugin_info, CB_LOAD, CB_UNLOAD }; \
extern "C" { \
	void* f_plugin_new(void) { \
	CLASS_NAME * pc_plugin = new CLASS_NAME(); \
	if (!pc_plugin) { \
		CRIT("Unable to allocate plugin"); \
		return NULL; \
		} \
		pc_plugin->f_set_desc(&gs_desc); \
		return dynamic_cast<CLASS_TYPE*>(pc_plugin); \
	} \
	struct ST_PLUGIN_DESCRIPTION * f_plugin_info(void) { return &gs_desc; } \
}

	//	D("Allocating %p", pc_plugin);
#define M_PLUGIN(CLASS_NAME, CLASS_TYPE, PLUGIN_NAME, PLUGIN_TYPE, PLUGIN_VERSION) M_PLUGIN_EXT(CLASS_NAME, CLASS_TYPE, PLUGIN_NAME, PLUGIN_TYPE, PLUGIN_VERSION , NULL, NULL)

#if 1
#define M_PLUGIN_CREATE_CB(FUNCTION_NAME, CLASS_NAME, METHOD_NAME) \
extern "C" int FUNCTION_NAME(CT_CORE * in_pc_core,  CT_GUARD<CT_PORT_NODE> & in_c_data, CT_PORT * in_pc_port) { \
	return static_cast<CLASS_NAME*>(in_pc_core)->METHOD_NAME(in_c_data); \
} \

//TODO Do link as boost bind
#define M_PLUGIN_CREATE_EVENT_CB(FUNCTION_NAME, CLASS_NAME, METHOD_NAME) \
extern "C" int FUNCTION_NAME(CT_CORE * in_pc_core,  CT_GUARD<CT_PORT_NODE> & in_c_data, CT_PORT * in_pc_port) { \
	return dynamic_cast<CLASS_NAME*>(in_pc_core)->METHOD_NAME(in_c_data, in_pc_port); \
} \

#endif

/***********************************************************************
 * Functions
 ***********************************************************************/
extern "C" {
void* f_plugin_new(void);
struct ST_PLUGIN_DESCRIPTION * f_plugin_info(void);
}


/***********************************************************************
 * Types
 ***********************************************************************/

class CT_PLUGIN {

	/* plugin description */
	ST_PLUGIN_DESCRIPTION * _ps_desc;
public:
	/* Description */
	void f_set_desc(struct ST_PLUGIN_DESCRIPTION * in_ps_desc) { _ps_desc = in_ps_desc; }
	struct ST_PLUGIN_DESCRIPTION * f_get_desc(void) { return _ps_desc; }
};

#endif /* PLUGIN_HH_ */
