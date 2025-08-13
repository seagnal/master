/***********************************************************************
 ** host.cc
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
 * @file host.cc
 * HOST definition.
 *
 * @author SEAGNAL (johann.baudy@seagnal.fr)
 * @date 2012
 *
 * @version 1.0 Original version
 */

/***********************************************************************
 * Includes
 ***********************************************************************/
#include <host.hh>
#include <iostream>
#include <cpp/string.hh>
#include <context.hh>
#include <task.hh>
#include <c/misc.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

std::string gstr_none("NONE");

CT_MUTEX gc_mutex_fftw;
//M_CONTEXT_CREATE_CB(f_main_cb, CT_HOST, f_run);

/* Host pointer */
CT_HOST * CT_HOST::host = NULL;
master::core::CT_HOST_TASK_MANAGER * CT_HOST::tasks = NULL;
/* Cpu resrouce pointer */

using namespace master::core;

CT_HOST::CT_HOST(struct ST_HOST_ARGS * in_ps_args) {
	_s_args = *in_ps_args;

	host = this;

	tasks = NULL;

	/* CPU RESOURCE must be created after host */
	_pc_cpures = NULL;

	/* reset init flag */
	_b_init = true;

	/* reset idle mode */
	_b_idle_context = true;

	/* Reset slave flag */
	_b_slave = false;

	_c_main.f_set_name("master");
	_c_main.f_set_full_name("master");

	{
		char * str_paths = getenv("HOME");
		M_ASSERT(str_paths);
		_str_home_path = str_paths;
	}

	{
		char * str_paths = getenv("MODULE_PATH");
    _DBG << "MODULE_PATH: "<<str_paths;
		if (!str_paths) {
			str_paths = (char*) "/usr/share/master/";
		}
		_str_plugin_path = str_paths;
	}

  {
    char * str_paths = getenv("TMP");
    _DBG << "TMP_PATH: "<<str_paths;
    if (!str_paths) {
      str_paths = (char*) "/tmp";
    }
    _str_tmp_path = str_paths;
  }

 {
  std::string str_db_path = f_looking_for_file("ids.xml","IDS_DB_PATH");
  if(str_db_path.size()) {
    CT_NODE c_node;
    c_node.from_xml_file(str_db_path);
    /* D("Dumping db node:\n %s", c_node.to_xml().c_str());*/

    /* Convert XML file to std::map */
    CT_NODE::childs_result v_tmp = c_node.get_childs("id");
    for (CT_NODE::childs_iterator pv_it = v_tmp.begin(); pv_it != v_tmp.end();
        pv_it++) {
      CT_NODE & c_node = **pv_it;
      std::string str_value = (std::string)c_node("value");
      std::string str_key = (std::string)c_node("key");
      std::istringstream converter(str_value);
      uint32_t i_value;
      converter >> std::hex >> i_value;
      /*_DBG << std::hex << _V(i_value) << " " << _V(str_key);*/
      _m_id_db[i_value] = str_key;
     }
  }
 }
}

CT_HOST::~CT_HOST() {
}

void CT_HOST::f_pre_destroy(void) {
	//D("Delete CT_HOST %d", _pc_cpures->usage());
}

int CT_HOST::f_run(CT_HOST_CONTEXT&) {
	int ec;
	//D("Run %d ", _b_init);
	if (_b_init) {
		//D("Initializing HOST");
		_b_init = false;

		if (!_s_args.str_config.size()) {
			_b_slave = true;
		}

		/* Create config port of main */
		{
			CT_NODE c_node;
			c_node("url", 0)("rx") = "func://__master";
			c_node("url", 0)("tx") = "func://__master_event";
			if (_b_slave) {
				_WARN << "Enable TCP CFG Port";
				c_node("url", 1) = "tcp://server:5700";
			}
			_c_main.f_port_cfg_get("__parent__").f_set_url(c_node);
		}

		/* Create task manager */
		tasks = f_new_task_manager();

		/* If not slave load config file */
		if (!_b_slave) {
			ec = f_config_load(_s_args.str_config);
			if (ec != EC_SUCCESS) {
				f_stop();
				CRIT("Unable to load MASTER profile");
				goto out_err;
			}
		}

		ec = EC_SUCCESS;
	} else {
		ec = f_plugin_check();
		if (ec == EC_FAILURE) {
			CRIT("Unable to process timeout");
		}
		ec = f_timeout_process();
		if (ec == EC_FAILURE) {
			CRIT("Unable to process timeout");
			goto out_err;
		}

		if (f_has_extra()) {
			ec = EC_SUCCESS;
		} else {
			ec = EC_BYPASS;
		}

	}
	/* MUST RETURN BYPASS */
	out_err: return ec;
}

int CT_HOST::f_main_init(void) {
	int ec;

	/* Allocate CPU resource */
	_pc_cpures = new CT_CPU_RESOURCE();
	CT_HOST::host->f_resource_register("cpu", _pc_cpures);
	M_ASSERT(_pc_cpures);

	/* Load plugins */
	ec = f_plugin_load();
	if (ec != EC_SUCCESS) {
		CRIT("Unable to load plugins");
		goto out_err;
	}

	/* Start application */
	ec = f_start(M_CONTEXT_BIND(CT_HOST, f_run,this));
	if (ec != EC_SUCCESS) {
		CRIT("Unable to start context");
		goto out_err;
	}

	ec = EC_SUCCESS;
	out_err: return ec;
}

int CT_HOST::f_main_loop(void) {
	return EC_FAILURE;
}

int CT_HOST::f_main_close(void) {
	int ec;
#if 0
	/* Forward pre destroy */
	{
		int ec;
		CT_PORT_NODE_GUARD c_tmp(E_ID_CORE_PRE_DESTROY);
		ec = _c_main.f_port_cfg_get("__parent__").f_receive(c_tmp);
		if (ec != EC_SUCCESS) {
			CRIT("Unable to execute pre destroy");
		}
	}
#endif
	/* Load empty profile */
	{
		//CT_NODE c_tmp;
		//std::string str_tmp = "master";
		//c_tmp.set_id(str_tmp);
		ec = _c_main.f_config_close();
		if (ec != EC_SUCCESS) {
			CRIT("Unable to load empty profile");
		}
	}


	/* flush host */_DBG << "Wait End of configuration and context close";
	{
		int i_timeout = 0;
		do {
			ec = f_execute_single();
			if (ec == EC_FAILURE) {
				CRIT("Unable to execute single");
				break;
			}

#if 0
			D("%d %d %d %d",
					ec, _c_main.f_is_running(), i_timeout, ((ec != EC_BYPASS) || (_c_main.f_is_running())) && (i_timeout < 100));
#endif
		} while (((ec != EC_BYPASS) || (_c_main.f_is_running()))
				&& (i_timeout++ < 100));
		if (i_timeout >= 100) {
			FATAL("Timeout execute single");
		}
	}
	/* Clearing remain tasks */
	tasks->f_clear();


	_WARN << "End of configuration";


	/* deleting task manager */
	delete tasks;

	/* Close all resources and transport */
	_c_main.f_port_cfg_clear();
	_c_main.f_port_clear();

	_m_transports.clear();


#if 1

	if(_l_contexts.size()) {
		_WARN << _l_contexts.size() << "contexts still in live";

		for (std::list<CT_HOST_CONTEXT *>::iterator pc_it = _l_contexts.begin();
						pc_it != _l_contexts.end(); ++pc_it) {
					/* Ignore current context */
					if ((*pc_it) != CT_HOST_CONTEXT::context) {
						/* Looking for running context */
						_DBG << "Checking context " << (*pc_it) << " status:"
								<< (*pc_it)->f_get_status();
					}
				}
	}

			usleep(100000);
#endif

	/* unload plugins */
#if 0
	{
		std::map<std::string, ST_PLUGIN_INFO>::iterator pc_it;
		for (pc_it = _m_dict.begin(); pc_it != _m_dict.end(); pc_it++) {
			f_plugin_close(pc_it->second);
		}
	}
#endif

	/* unregister cpu */
	//CT_HOST::host->f_resource_unregister("cpu");

	/* clear resources */
	_DBG << "Clearing " << _m_resources.size() << "resources";

	{
		std::map<std::string, CT_GUARD<CT_RESOURCE> >::iterator pc_it;
		for (pc_it = _m_resources.begin(); pc_it != _m_resources.end(); pc_it++) {
			_DBG << " -" << pc_it->second->f_get_name();
		}
	}
	_m_resources.clear();

	return EC_SUCCESS;
}

int CT_HOST::f_plugin_load(void) {
	return EC_FAILURE;
}

int CT_HOST::f_plugin_get_mtime(char const * in_str_module,
		uint64_t * out_pi_mtime) {
	int ec;
	struct stat s_stats;

	/* Stats library */
	ec = stat(in_str_module, &s_stats);
	if (ec != 0) {
		CRIT("Unable to stat library %s", in_str_module);
		goto out_err;
	}

	if (s_stats.st_mode | S_IRWXU) {
		*out_pi_mtime = f_get_time_ns64_from_timespec(&s_stats.st_mtim);
		ec = EC_SUCCESS;
	} else {
		ec = EC_FAILURE;
	}
#if 0
	if (std::string(in_str_module)
			== "/home/johaahn/source/seagnal/master/__out__/test/src/plugins/test/libplugin-test.so") {
		printf("File %s type:                ", in_str_module);
		f_dump_stat(&s_stats);
	}
#endif

	out_err: return ec;
}

int CT_HOST::f_plugin_close(struct ST_PLUGIN_INFO & inout_s_info) {
	M_BUG();
	return EC_FAILURE;
}

int CT_HOST::f_plugin_reload(struct ST_PLUGIN_INFO & inout_s_info) {
	M_BUG();
	return EC_FAILURE;
}

int CT_HOST::f_plugin_update() {
	int ec;
	std::map<std::string, ST_PLUGIN_INFO>::iterator pc_it;

	for (pc_it = _m_dict.begin(); pc_it != _m_dict.end(); pc_it++) {
		if (pc_it->second.b_updated) {
			switch (pc_it->second.ps_desc->e_type) {
			case E_PLUGIN_TYPE_CORE:
				/* Remove core that needs reload */
				ec = _c_main.f_child_remove(pc_it->second.ps_desc);
				if (ec != EC_SUCCESS) {
					CRIT("Unable to remove instances of %s",
							pc_it->second.ps_desc->str_name.c_str());
					goto out_err;
				}

				/* Reloading library */
				ec = f_plugin_reload(pc_it->second);
				if (ec != EC_SUCCESS) {
					CRIT("Unable to reload instances of %s",
							pc_it->second.ps_desc->str_name.c_str());
					goto out_err;
				}
				D("%llx ", pc_it->second.i_time_modification);

				/* Remove upper case from name */
				pc_it->second.ps_desc->str_name = f_string_tolower(
						pc_it->second.ps_desc->str_name);
				break;
			default:
				break;
			}
		}
	}

	ec = EC_SUCCESS;
	out_err: return ec;
}

int CT_HOST::f_plugin_check() {
	int ec;
	std::map<std::string, ST_PLUGIN_INFO>::iterator pc_it;

	for (pc_it = _m_dict.begin(); pc_it != _m_dict.end(); pc_it++) {
		if (!pc_it->second.b_updated) {
			uint64_t i_mtime;

			ec = f_plugin_get_mtime(pc_it->second.str_library_path.c_str(),
					&i_mtime);
			if (ec != EC_SUCCESS) {
				i_mtime = pc_it->second.i_time_modification;
			}
			//D("%llx %llx", i_mtime, pc_it->second.i_time_modification);
			if (i_mtime != pc_it->second.i_time_modification) {
				CRIT("Plugin %s has changed",
						pc_it->second.ps_desc->str_name.c_str());
				pc_it->second.b_updated = true;
			}

		}
	}

	ec = EC_SUCCESS;
	//out_err:
	return ec;
}

int CT_HOST::f_plugin_register(ST_PLUGIN_INFO & in_ps_info) {
	int ec;

	/* Remove upper case from name */
	in_ps_info.ps_desc->str_name = f_string_tolower(
			in_ps_info.ps_desc->str_name);

	/* Check if plugin already exist */
	if (_m_dict.find(in_ps_info.ps_desc->str_name) != _m_dict.end()) {
		CRIT("Plugin %s already exist", in_ps_info.ps_desc->str_name.c_str());
		ec = EC_FAILURE;
		goto out_err;
	}

	switch (in_ps_info.ps_desc->e_type) {
	case E_PLUGIN_TYPE_CORE:
		_DBG << "Registering CORE:" << in_ps_info.ps_desc->str_name
				<< " version:" << in_ps_info.ps_desc->str_version;
		break;
	case E_PLUGIN_TYPE_TRANSPORT:
		_DBG << "Registering TRANSPORT:" << in_ps_info.ps_desc->str_name
				<< " version:" << in_ps_info.ps_desc->str_version;
		break;
	case E_PLUGIN_TYPE_RESOURCE:
		_DBG << "Registering RESOURCE:" << in_ps_info.ps_desc->str_name
				<< " version:" << in_ps_info.ps_desc->str_version;
		break;
	// unknown PLUGIN
    default:
        _DBG << "Registering UNKNOWN plugin: " << in_ps_info.ps_desc->str_name
			<< " version:" << in_ps_info.ps_desc->str_version;
		break;
	}
#if 0
	WARN("Registering plugin: %s %s %p %p",
			in_ps_info.ps_desc->str_name.c_str(), in_ps_info.ps_desc->str_version.c_str(), in_ps_info.ps_desc->cb_new, in_ps_info.ps_desc);
#endif
	_m_dict[in_ps_info.ps_desc->str_name] = in_ps_info;

	/* Execute load callback on register if exists */
	if (in_ps_info.ps_desc->cb_load) {
		(*in_ps_info.ps_desc->cb_load)();
	}

	ec = EC_SUCCESS;
	out_err: return ec;
}

int CT_HOST::f_config_load(CT_NODE & in_c_node) {
	int ec;

	ec = _c_main.f_config_load(in_c_node);
	if (ec != EC_SUCCESS) {
		CRIT("Unable to load config");
		goto out_err;
	}

	ec = EC_SUCCESS;
	out_err: return ec;
}

int CT_HOST::f_config_load(std::string const & in_str_file) {
	CT_NODE c_node;
	int ec;

	c_node.from_xml_file(f_string_replace(in_str_file,"#",""));
	//D("Dumping configuration node:\n %s", c_node.to_xml().c_str());
	_WARN << "Load config: " << in_str_file;
	ec = f_config_load(c_node);
	if (ec != EC_SUCCESS) {
		CRIT("Unable to configure master with file %s", in_str_file.c_str());
		goto out_err;
	}

	ec = EC_SUCCESS;
	out_err: return ec;
}

int CT_HOST::f_state_load(std::string const & in_str_file) {
	CT_NODE c_node;
	int ec;

	ec = _c_main.f_state_restore_start(in_str_file);
	if (ec != EC_SUCCESS) {
		CRIT("Unable to restore state of main node");
		goto out_err;
	}

	ec = EC_SUCCESS;
	out_err: return ec;
}

int CT_HOST::f_state_save(std::string const & in_str_file) {
	int ec;

	ec = _c_main.f_state_save_start(in_str_file);
	if (ec != EC_SUCCESS) {
		CRIT("Unable to save state of main node");
		goto out_err;
	}

	ec = EC_SUCCESS;
	out_err: return ec;
}
int CT_HOST::f_state_loaded(void) {
	return EC_SUCCESS;
}

int CT_HOST::f_core_new(CT_CORE* & out_pc_new,
		std::string const & in_str_name) {
	int ec;
	std::string str_name = f_string_tolower(in_str_name);

	/* Check if plugin exist */
	if (_m_dict.find(str_name) == _m_dict.end()) {
		CRIT("Plugin %s does not exist", str_name.c_str());
		ec = EC_FAILURE;
		goto out_err;
	}

	{
		ST_PLUGIN_INFO & s_info = _m_dict[str_name];
		if (s_info.ps_desc->e_type != E_PLUGIN_TYPE_CORE) {
			CRIT("Plugin %s is not a CORE", str_name.c_str());
			ec = EC_FAILURE;
			goto out_err;
		}

		/* Allocate plugin */
#if 1
		D("Allocating core %s (%s %s %p %p)",
				str_name.c_str(), s_info.ps_desc->str_name.c_str(), s_info.ps_desc->str_version.c_str(), s_info.ps_desc->cb_new, s_info.ps_desc);
#endif
		void * pv_new = s_info.ps_desc->cb_new();
		if (!pv_new) {
			CRIT("Unable to initialize new core");
			ec = EC_FAILURE;
			goto out_err;
		}

		/* Set output */
		out_pc_new = static_cast<CT_CORE*>(pv_new);
	}
	ec = EC_SUCCESS;
	out_err: return ec;
}

CT_GUARD<CT_TRANSPORT> CT_HOST::f_transport_get(CT_NODE& in_c_node) {
	int ec;
	std::map<std::string, CT_GUARD<CT_TRANSPORT> >::iterator pc_it;
	CT_GUARD<CT_TRANSPORT> pc_transport;
	std::string str_url = (std::string) in_c_node;

	pc_it = _m_transports.find(str_url);
	/* Check if tranport already exists */
	if (pc_it == _m_transports.end()) {
		std::string str_transport_name;
		std::string str_transport_config;

		/* Extract plugin name from url */
		{
			std::vector<std::string> v_url = f_string_split(str_url, "://");
			if (v_url.size() != 2) {
				CRIT("Incorrect URL Format: %s (%d)",
						str_url.c_str(), v_url.size());
				ec = EC_FAILURE;
				goto out_err;
			}
			str_transport_name = f_string_tolower(v_url[0]);
			str_transport_config = v_url[1];
		}
		//D("name:%s, config: %s",str_transport_name.c_str(), str_transport_config.c_str());

		/* Check if plugin exist */
		if (_m_dict.find(str_transport_name) == _m_dict.end()) {
			CRIT("Plugin %s does not exist !", str_url.c_str());
			ec = EC_FAILURE;
			goto out_err;
		}

		{
			ST_PLUGIN_INFO & s_info = _m_dict[str_transport_name];
			if (s_info.ps_desc->e_type != E_PLUGIN_TYPE_TRANSPORT) {
				CRIT("Plugin %s is not a TRANSPORT",
						str_transport_name.c_str());
				ec = EC_FAILURE;
				goto out_err;
			}

			/* Allocate plugin */
			void * pv_transport = s_info.ps_desc->cb_new();
			if (!pv_transport) {
				CRIT("Unable to initialize new transport");
				ec = EC_FAILURE;
				goto out_err;
			}

			pc_transport = CT_GUARD<CT_TRANSPORT>(
					static_cast<CT_TRANSPORT*>(pv_transport));

			/* Store URL inside transport */
			pc_transport->f_set_config(in_c_node);

			/* Set configuration of transport */
			ec = pc_transport->f_apply_config(str_transport_config);
			if (ec != EC_SUCCESS) {
				_CRIT << "Unable to apply config transport: "
						<< str_transport_config;
				ec = EC_FAILURE;
				goto out_err;
			}

			/* Store transport inside map */
			_m_transports[str_url] = pc_transport;

			//D("Created transport %p", pc_transport.get());
		}

	} else {
		pc_transport = pc_it->second;
		//D("Found transport %p", pc_transport.get());
	}

	ec = EC_SUCCESS;
	out_err: if (ec != EC_SUCCESS) {
		return CT_GUARD<CT_TRANSPORT>();
	} else {
		return pc_transport;
	}
}
#if 0
int CT_HOST::f_transport_unregister(CT_GUARD<CT_TRANSPORT> in_pc_transport) {
	std::string str_trans_name = (std::string) in_pc_transport->f_get_config();

	/* Remove transport from map */
	std::map<std::string, CT_GUARD<CT_TRANSPORT> >::iterator pc_it;
	pc_it = _m_transports.find(str_trans_name);
	M_ASSERT(pc_it != _m_transports.end());
	M_ASSERT(pc_it->second == in_pc_transport);
	_m_transports.erase(pc_it);
	return EC_SUCCESS;
}
#endif

CT_GUARD<CT_RESOURCE> CT_HOST::f_resource_get(
		std::string in_str_resource_name) {
	std::map<std::string, CT_GUARD<CT_RESOURCE> >::iterator pc_it;
	pc_it = _m_resources.find(in_str_resource_name);
	/* Check if tranport already exists */
	if (pc_it == _m_resources.end()) {
		throw _CRIT << "No such resource: " << in_str_resource_name;
	} else {
		return pc_it->second;
	}
}
#if 0
CT_GUARD<CT_RESOURCE> CT_HOST::f_resource_create(
		std::string in_str_resource_name, std::string in_str_resource_type) {
	//int ec;
	CT_GUARD<CT_RESOURCE> pc_res;
	std::map<std::string, CT_GUARD<CT_RESOURCE> >::iterator pc_it;
	pc_it = _m_resources.find(in_str_resource_name);
	if (pc_it != _m_resources.end()) {
		//pc_it->second->use();
		//return pc_it->second;
		CRIT("resource %s already exist", in_str_resource_name.c_str());
		return CT_GUARD<CT_RESOURCE>();
	} else {
		if (_m_dict.find(in_str_resource_type) == _m_dict.end()) {
			CRIT("No such plugin %s", in_str_resource_name.c_str());
			return CT_GUARD<CT_RESOURCE>();
		} else {

			ST_PLUGIN_INFO & s_info = _m_dict[in_str_resource_name];
			if (s_info.ps_desc->e_type != E_PLUGIN_TYPE_RESOURCE) {
				CRIT("Plugin %s is not a RESOURCE",
						in_str_resource_name.c_str());
				return CT_GUARD<CT_RESOURCE>();
			}

			/* Create new resource */
			void * pv_resource = s_info.ps_desc->cb_new();
			if (!pv_resource) {
				CRIT("Unable to initialize new resource");
				return CT_GUARD<CT_RESOURCE>();
			}

			/* Cast resource pointer */
			pc_res = CT_GUARD<CT_RESOURCE>(
					static_cast<CT_RESOURCE*>(pv_resource));
		}

		_m_resources[in_str_resource_name] = pc_res;

		return pc_res;
	}

	//ec = EC_SUCCESS;
	//out_err: return ec;
}
#endif

int CT_HOST::f_resource_register(std::string const &in_str_name,
		CT_RESOURCE * in_pc_resource) {
	std::map<std::string, CT_GUARD<CT_RESOURCE> >::iterator pc_it;
	pc_it = _m_resources.find(in_str_name);
	/* Check if tranport already exists */
	if (pc_it == _m_resources.end()) {
		_m_resources[in_str_name] = CT_GUARD<CT_RESOURCE>(in_pc_resource);
	} else {
		return EC_FAILURE;
	}

	return EC_SUCCESS;
}

int CT_HOST::f_resource_unregister(std::string const &in_str_name) {

	/* Remove resource from map */
	std::map<std::string, CT_GUARD<CT_RESOURCE> >::iterator pc_it;
	pc_it = _m_resources.find(in_str_name);
	M_ASSERT(pc_it != _m_resources.end());
	_m_resources.erase(pc_it);

	return EC_SUCCESS;
}

int CT_HOST::f_context_register(CT_HOST_CONTEXT * in_pc_context) {
	//_DBG << "Register context:" << _V(in_pc_context);
	_l_contexts.push_back(in_pc_context);
	return EC_SUCCESS;
}

int CT_HOST::f_context_unregister(CT_HOST_CONTEXT * in_pc_context) {
	//_DBG << "Unregister context:" << _V(in_pc_context);
	_l_contexts.remove(in_pc_context);
	return EC_SUCCESS;
}
bool CT_HOST::f_context_is_idling(void) {
	return _b_idle_context;
}
int CT_HOST::f_context_sleep(void) {
	//WARN("Requesting sleep");
	/* Request idle mode in all context */
	_b_idle_context = true;
#if 0
	for (std::list<CT_HOST_CONTEXT *>::iterator pc_it = _l_contexts.begin();
			pc_it != _l_contexts.end(); ++pc_it) {
		/* Set idle mode of context */
		(*pc_it)->f_set_idle_state(true);
	}
#endif
	//_WARN << "Waiting all context to be sleeping";
	/* Waiting for all context to be in idle mode */
	int i_timeout = 200;

	while (--i_timeout) {
		bool b_pass = true;
		for (std::list<CT_HOST_CONTEXT *>::iterator pc_it = _l_contexts.begin();
				pc_it != _l_contexts.end(); ++pc_it) {
			/* Ignore current context */
			if ((*pc_it) != CT_HOST_CONTEXT::context) {
				/* Looking for running context */
				_DBG << "Checking context " << (*pc_it) << " status:"
						<< (*pc_it)->f_get_status();
				switch ((*pc_it)->f_get_status()) {
				case E_CONTEXT_STATUS_IDLE:
				case E_CONTEXT_STATUS_STOPPED:
					break;
				default:
					CRIT("Found busy context");
					b_pass = false;
					usleep(50000);
					break;
				}
			}
		}

		if (b_pass) {
			break;
		}
	}

	if (i_timeout) {
		D("All context in sleep mode");
		return EC_SUCCESS;
	} else {
		_FATAL << "Failed to sleep all contexts";
		return EC_FAILURE;
	}
}

int CT_HOST::f_context_wakeup(void) {
	//WARN("Wakeup all contexts");
	/* Request run mode in all context */
	_b_idle_context = false;
#if 0
	for (std::list<CT_HOST_CONTEXT *>::iterator pc_it = _l_contexts.begin();
			pc_it != _l_contexts.end(); ++pc_it) {
		/* Set idle mode of context */
		(*pc_it)->f_set_idle_state(false);
	}
#endif
	return EC_SUCCESS;
}

int CT_HOST::f_timeout_register(CT_HOST_TIMEOUT * in_pc_timeout) {
	_l_timeouts.push_back(in_pc_timeout);
	return EC_SUCCESS;
}

int CT_HOST::f_timeout_unregister(CT_HOST_TIMEOUT * in_pc_timeout) {
	_l_timeouts.remove(in_pc_timeout);
	return EC_SUCCESS;
}

int CT_HOST::f_timeout_clear(void) {
	for (std::list<CT_HOST_TIMEOUT *>::iterator pc_it = _l_timeouts.begin();
			pc_it != _l_timeouts.end(); ++pc_it) {
		(*pc_it)->f_stop();
	}
	return EC_SUCCESS;
}

int CT_HOST::f_timeout_process(void) {
	return EC_BYPASS;
}

/* Custom HOST CONTEXT - Start
 * Remove idle state */
int CT_HOST::f_init(void) {
	return EC_SUCCESS;
}
/* Looking for file in master environnement.
 * */
std::string CT_HOST::f_looking_for_file(std::string in_str_file, std::string in_str_var, bool in_b_debug) {
  std::string str_file;
  /* Get profile path */
  {
    std::string str_system_file = "/usr/share/master/"+in_str_file;
    if(in_b_debug) {
      _DBG << "Lookging for "<< str_system_file;
    }
    if(f_misc_file_exists(str_system_file.c_str()) == EC_SUCCESS) {
      str_file = str_system_file;
    }
  }
  {
    std::string str_system_file = std::string(getenv("HOME"))+"/.master/"+in_str_file;
    if(in_b_debug) {
      _DBG << "Lookging for "<< str_system_file;
    }
    if(f_misc_file_exists(str_system_file.c_str()) == EC_SUCCESS) {
      str_file = str_system_file;
    }
  }
  if (in_str_var.size())
  {
		char * pc_tmp = getenv(in_str_var.c_str());
		if(pc_tmp != NULL) {

			std::vector<std::string> v_paths = f_string_split(std::string(pc_tmp), ";");
			for (auto str_tmp : v_paths) {
		    std::string str_system_file = str_tmp+"/"+in_str_file;
	      if(in_b_debug) {
	        _DBG << "Lookging for "<< str_system_file;
	      }
		    if(f_misc_file_exists(str_system_file.c_str()) == EC_SUCCESS) {
		      str_file = str_system_file;
					break;
		    }
			}
		}
  }
  if(in_b_debug) {
    _DBG << "File selected: " << str_file;
  }
  return str_file;
}

std::string const & CT_HOST::f_id_name(uint32_t in_i_id) {
 auto pc_it = _m_id_db.find(in_i_id);
 if(pc_it != _m_id_db.end()) {
  return pc_it->second;
 } else {
  //_CRIT << "ID "<< std::hex << in_i_id << " does not exists";
  return gstr_none;
 }
}

#if 0
/* Custom HOST CONTEXT - Join */
int CT_HOST::f_join(void) {
	M_BUG();
	return EC_FAILURE;
}
CT_HOST_CONTEXT * CT_HOST::f_new_context() {
	M_BUG();
	return NULL;
}

CT_HOST_TIMEOUT * CT_HOST::f_new_timeout() {
	M_BUG();
	return NULL;
}
#endif
CT_HOST_PROPERTY * CT_HOST::f_new_property() {
	return new CT_HOST_PROPERTY();
}

master::core::CT_HOST_TASK_MANAGER * CT_HOST::f_new_task_manager() {
	return new master::core::CT_HOST_TASK_MANAGER();
}
/** HOST FILE **/
CT_HOST_FILE::CT_HOST_FILE(std::string const & in_str_abs_path) :
		str_abs_path(in_str_abs_path) {

}
CT_HOST_FILE::CT_HOST_FILE(char const * in_str_abs_path) :
		str_abs_path(in_str_abs_path) {

}
CT_HOST_FILE::operator std::string() const {
	return str_abs_path;
}

/*** HOST LOCK ***/
CT_HOST_LOCK::CT_HOST_LOCK() {

}
CT_HOST_LOCK::~CT_HOST_LOCK() {

}

void CT_HOST_LOCK::f_lock(void) {
	M_BUG();
}
void CT_HOST_LOCK::f_unlock(void) {
	M_BUG();
}
bool CT_HOST_LOCK::f_try_lock(void) {
	M_BUG();
	return false;
}
#if 0
void CT_HOST_LOCK::f_notify_wait() {
}
void CT_HOST_LOCK::f_notify_all() {
}
void CT_HOST_LOCK::f_notify_one() {
}
#endif

/*** HOST MUTEX ***/

CT_HOST_MUTEX::CT_HOST_MUTEX() {

}
CT_HOST_MUTEX::~CT_HOST_MUTEX() {

}

/*** HOST SPINLOCK ***/

CT_HOST_SPINLOCK::CT_HOST_SPINLOCK() {

}
CT_HOST_SPINLOCK::~CT_HOST_SPINLOCK() {

}

CT_HOST_TIMEOUT::CT_HOST_TIMEOUT() {
	_s_cb.pc_data = NULL;

	/* Registering timeout */
	{
		int ec;
		ec = CT_HOST::host->f_timeout_register(this);
		M_ASSERT(ec == EC_SUCCESS);
	}
}

CT_HOST_TIMEOUT::~CT_HOST_TIMEOUT() {

	/* UnRegistering context */
	{
		int ec;
		ec = CT_HOST::host->f_timeout_unregister(this);
		M_ASSERT(ec == EC_SUCCESS);
	}

	_s_cb.pc_data = NULL;
}

void CT_HOST_TIMEOUT::f_start(int in_i_ms, int in_i_interval) {
	_i_origin_trigger = f_get_time_ns64();
	_i_next_trigger = _i_origin_trigger + in_i_ms * 1000000LL;
	_i_interval = in_i_interval * 1000000LL;

}
void CT_HOST_TIMEOUT::f_stop(void) {
	M_BUG();
}
void CT_HOST_TIMEOUT::f_trigger(void) {
	CT_HOST_CONTEXT::context->f_push_extra(_s_cb);
}

void CT_HOST_TIMEOUT::f_set_cb(CT_PORT & in_c_port,
		CT_GUARD<CT_PORT_NODE> & in_c_node) {
	_s_cb.pc_sb = in_c_port.c_receive;
	_s_cb.pc_data = in_c_node;
}

//M_PLUGIN_CREATE_EVENT_CB(f_register_parent_cb, CT_HOST_CORE, f_event_parent);

struct ST_PLUGIN_DESCRIPTION gs_desc = { "master", E_PLUGIN_TYPE_CORE,
		M_STR_VERSION, NULL, NULL, NULL, NULL };

CT_HOST_CORE::CT_HOST_CORE() :
		CT_CORE() {

	_b_running = true;

	/* Register parent loopback */
	f_port_cfg_get("__parent__").f_register_cb_tx(
			M_PORT_BIND(CT_HOST_CORE, f_event_parent, this));
	/* Set custom plugin description */
	f_set_desc(&gs_desc);
}

enum ET_TASK_CONFIG_LOAD_PART {
	E_TASK_CONFIG_LOAD_STEP_PRE_CONFIG = 0,
	E_TASK_CONFIG_LOAD_STEP_CONFIG,
	E_TASK_CONFIG_LOAD_STEP_POST_CONFIG,
	E_TASK_CONFIG_LOAD_STEP_POST_INIT,
	E_TASK_CONFIG_LOAD_STEP_NB,
};

enum ET_TASK_CONFIG_CLOSE_PART {
	E_TASK_CONFIG_CLOSE_STEP_PRE_DESTROY = 0,
	E_TASK_CONFIG_CLOSE_STEP_PRE_CONFIG,
	E_TASK_CONFIG_CLOSE_STEP_CONFIG,
	E_TASK_CONFIG_CLOSE_STEP_NB,
};

int CT_HOST_CORE::f_event_parent(CT_GUARD<CT_PORT_NODE> const & in_c_node) {
	int ec;

	switch (in_c_node->get_id()) {
	case E_ID_CORE_HOST_CONFIG_LOAD_STATUS: {
		//D("E_ID_CORE_HOST_CONFIG_LOAD_STATUS for node %s",f_get_name().c_str());
		ST_TASK_NOTIFICATION & c_notif = in_c_node->map<ST_TASK_NOTIFICATION>();
		//D("Received task status : %d", c_notif.e_status);
		CT_GUARD<CT_TASK> c_task = CT_HOST::tasks->f_get(c_notif.s_status);

		switch (c_notif.e_status) {
		case E_TASK_STATUS_STEP_COMPLETE: {

			switch (c_notif.s_status.i_step) {

			case E_TASK_CONFIG_LOAD_STEP_PRE_CONFIG: {

				CT_NODE & c_tmp = c_task->f_get_data<CT_NODE>();
				/* Sleep all contexts */
				ec = CT_HOST::host->f_context_sleep();
				if (ec != EC_SUCCESS) {
					CRIT("Unable to put context in sleep mode");
					c_task->f_error();
					goto out_err;
				}

				/* Configure main node */
				if (c_tmp.get_id() == "master") {

					/* Reload library that needs reload */
					ec = CT_HOST::host->f_plugin_update();
					if (ec != EC_SUCCESS) {
						CRIT("Unable to reload library during node update: %s",
								c_tmp.get_id().c_str());
						c_task->f_error();
						goto out_err;
					}
					//D("%s", c_tmp.to_xml().c_str());

					/* Triggering configure */
					{
						CT_PORT_NODE_GUARD c_node(E_ID_CORE_CONFIGURE,
								c_tmp.to_xml());

						c_task->f_status_init(*c_node,
								E_TASK_CONFIG_LOAD_STEP_CONFIG);

						ec = f_port_cfg_get("__parent__").f_receive(c_node);
						if (ec != EC_SUCCESS) {
							CRIT("Unable to execute cfg event (current:%s)",
									f_get_name().c_str());
							c_task->f_error();
							goto out_err;
						}
					}

				} else {
					c_task->f_error();
					CRIT("No master node, skipping load ...");
					ec = EC_FAILURE;
					goto out_err;
				}
				break;
			}

			case E_TASK_CONFIG_LOAD_STEP_CONFIG: {

				/* wakeup all contexts */
				ec = CT_HOST::host->f_context_wakeup();
				if (ec != EC_SUCCESS) {
					c_task->f_error();
					CRIT("Unable to wake up contexts");
					goto out_err;
				}

				// Execute post init
				{
					CT_PORT_NODE_GUARD c_tmp(E_ID_CORE_POST_PROFILE);
					c_task->f_status_init(*c_tmp,
							E_TASK_CONFIG_LOAD_STEP_POST_CONFIG);
					ec = f_port_cfg_get("__parent__").f_receive(c_tmp);
					if (ec != EC_SUCCESS) {
						c_task->f_error();
						CRIT("Unable to execute post init");
						goto out_err;
					}

				}
				break;
			}
			case E_TASK_CONFIG_LOAD_STEP_POST_CONFIG: {
				/* Send state to parent node */
				{
					CT_PORT_NODE_GUARD c_tmp(E_ID_CORE_POST_INIT);
					c_task->f_status_init(*c_tmp,
							E_TASK_CONFIG_LOAD_STEP_POST_INIT);
					ec = f_port_cfg_get("__parent__").f_receive(c_tmp);
					if (ec != EC_SUCCESS) {
						CT_HOST::host->f_stop();
						CRIT("Unable to execute post init");
						goto out_err;
					}
				}
				break;
			}
			}

			break;
		}
		case E_TASK_STATUS_TASK_COMPLETE: {
			D("END of CONFIG LOAD - Task complete");

			/* Task is not needed any more */
			CT_HOST::tasks->f_release(c_task);
			break;
		}
		case E_TASK_STATUS_TIMEOUT:
		case E_TASK_STATUS_ERROR: {
			FATAL("Task Error");
			CT_HOST::host->f_stop();
			CT_HOST::tasks->f_release(c_task);
			break;

		}

		default:
			//D("Default");
			break;
		}

		break;
	}
#if 1
	case E_ID_CORE_HOST_CONFIG_CLOSE_STATUS: {
		//D("E_ID_CORE_HOST_CONFIG_CLOSE_STATUS for node %s",f_get_name().c_str());
		ST_TASK_NOTIFICATION & c_notif = in_c_node->map<ST_TASK_NOTIFICATION>();
		//D("Received task status : %d", c_notif.e_status);
		CT_GUARD<CT_TASK> c_task = CT_HOST::tasks->f_get(c_notif.s_status);

		switch (c_notif.e_status) {
		case E_TASK_STATUS_STEP_COMPLETE: {

			switch (c_notif.s_status.i_step) {

			case E_TASK_CONFIG_CLOSE_STEP_PRE_DESTROY: {
				/* Send state to parent node */
				{
					CT_PORT_NODE_GUARD c_tmp(E_ID_CORE_PRE_PROFILE);
					c_task->f_status_init(*c_tmp,
							E_TASK_CONFIG_CLOSE_STEP_PRE_CONFIG);
					ec = f_port_cfg_get("__parent__").f_receive(c_tmp);
					if (ec != EC_SUCCESS) {
						CT_HOST::host->f_stop();
						CRIT("Unable to execute post init");
						goto out_err;
					}
				}
				break;
			}
			case E_TASK_CONFIG_CLOSE_STEP_PRE_CONFIG: {

				/* Sleep all contexts */
				ec = CT_HOST::host->f_context_sleep();
				if (ec != EC_SUCCESS) {
					CRIT("Unable to put context in sleep mode");
					c_task->f_error();
					goto out_err;
				}

				/* Configure main node */
				{
					CT_NODE c_tmp;
					c_tmp.set_id("master");

					/* Triggering configure */
					{
						CT_PORT_NODE_GUARD c_node(E_ID_CORE_CONFIGURE,
								c_tmp.to_xml());

						c_task->f_status_init(*c_node,
								E_TASK_CONFIG_CLOSE_STEP_CONFIG);

						ec = f_port_cfg_get("__parent__").f_receive(c_node);
						if (ec != EC_SUCCESS) {
							CRIT("Unable to execute cfg event (current:%s)",
									f_get_name().c_str());
							c_task->f_error();
							goto out_err;
						}
					}

				}
				break;
			}

			case E_TASK_CONFIG_CLOSE_STEP_CONFIG: {
#if 0
				/* wakeup all contexts */
				ec = CT_HOST::host->f_context_wakeup();
				if (ec != EC_SUCCESS) {
					c_task->f_error();
					CRIT("Unable to wake up contexts");
					goto out_err;
				}
#endif
				break;
			}
			}

			break;
		}
		case E_TASK_STATUS_TASK_COMPLETE: {
			D("END of CONFIG CLOSE - Task complete");

			/* Task is not needed any more */
			CT_HOST::tasks->f_release(c_task);
			_b_running = false;
			break;
		}
		case E_TASK_STATUS_TIMEOUT:
		case E_TASK_STATUS_ERROR: {
			FATAL("Task Error");
			CT_HOST::host->f_stop();
			CT_HOST::tasks->f_release(c_task);
			_b_running = false;
			break;

		}

		default:
			break;
		}

		break;
	}
#endif
	case E_ID_CORE_ACK: {
		//D("E_ID_CORE_ACK for host core");
		if (CT_TASK::f_reply_has_status(*in_c_node)) {
			ST_TASK_STATUS & s_status = CT_TASK::f_reply_get_status(*in_c_node);
			ec = in_c_node->map<ST_CORE_ACK>().i_ack;
			//D("Task %d", s_status.i_luid);
			CT_GUARD<CT_TASK> c_task = CT_HOST::tasks->f_get(s_status);

			if (!c_task) {
				WARN("Task seems to be closed");
			} else {
				if (ec == EC_SUCCESS) {
					c_task->f_complete(s_status);
				} else {
					c_task->f_error();
				}

			}
		}
		break;
	}
	case E_ID_CORE_SETTINGS_ACK:
	case E_ID_CORE_CONFIGURE_ACK: {
#if 0
		if (in_c_node->get_id() == E_ID_CORE_SETTINGS_ACK) {
			D("E_ID_CORE_SETTINGS_ACK for host core");
		} else {
			D("E_ID_CORE_CONFIGURE_ACK for host core");
		}
#endif
		if (CT_TASK::f_reply_has_status(*in_c_node)) {
			ST_TASK_STATUS & s_status = CT_TASK::f_reply_get_status(*in_c_node);
			ec = in_c_node->map<ST_CORE_ACK>().i_ack;
			//D("Task %d", s_status.i_luid);
			CT_GUARD<CT_TASK> c_task = CT_HOST::tasks->f_get(s_status);

			if (!c_task) {
				WARN("Task seems to be closed");
			} else {
				if (ec == EC_SUCCESS) {
					c_task->f_complete(s_status);
				} else {
					c_task->f_error();
				}

				/* Release task */
				if (in_c_node->get_id() == E_ID_CORE_CONFIGURE_ACK) {
					CT_HOST::tasks->f_release(c_task);
				}
			}
		}
		// Execute post init
		if (in_c_node->get_id() == E_ID_CORE_SETTINGS_ACK) {
			CT_PORT_NODE_GUARD c_tmp(E_ID_CORE_POST_SETTINGS);
			ec = f_port_cfg_get("__parent__").f_receive(c_tmp);
			if (ec != EC_SUCCESS) {
				CRIT("Unable to execute post settings");
				goto out_err;
			}
		}

		CT_HOST::host->f_state_loaded();

		break;
	}
	case E_ID_CORE_SAVE_STATE_REPLY: {
		//	D("E_ID_CORE_SAVE_STATE_REPLY");
		M_ASSERT(in_c_node->has(E_ID_TASK_STATUS));

		ST_TASK_STATUS & s_status = in_c_node->get(E_ID_TASK_STATUS)->map<
				ST_TASK_STATUS>();
		CT_GUARD<CT_TASK> c_task = CT_HOST::tasks->f_get(s_status);

		if (!c_task) {
			WARN("Task seems to be closed");
		} else {
			/* update task node */

			std::string & c_tmp = c_task->f_get_data<std::string>();
			CT_NODE c_node;

			c_node.from_xml(in_c_node->get_data<std::string>());

			/* Save file */
			ec = f_state_save_end(c_tmp, c_node);
			if (ec != EC_SUCCESS) {
				c_task->f_error();
				CRIT("Unable to save state in file");
				goto out_err;
			}
			/* inform task that this part of step is completed */
			c_task->f_complete(s_status);

			/* Release task */
			//CT_HOST::tasks->f_release(c_task);
		}
		break;
	}
	case E_ID_CORE_SAVE_STATE_ERROR: {
		//D("E_ID_CORE_SAVE_STATE_ERROR");
		ST_TASK_STATUS & s_status = in_c_node->map<ST_TASK_STATUS>();
		CT_GUARD<CT_TASK> c_task = CT_HOST::tasks->f_get(s_status);
		c_task->f_error();
		CRIT("Unable to save state in file %s",
				c_task->f_get_data<std::string>().c_str());

		/* Release task */
		//CT_HOST::tasks->f_release(c_task);
		break;
	}
	default:
		break;
	}
	ec = EC_SUCCESS;
	out_err: return ec;
}

int CT_HOST_CORE::f_state_save_end(std::string const & in_str_file,
		CT_NODE & in_c_state) {
	int ec;
	//D("Save %s to %s", in_c_state.to_xml().c_str(), in_str_file.c_str());
	try {
		in_c_state.to_xml_file(in_str_file);
	} catch (...) {
		CRIT("Unable to configure master with file %s", in_str_file.c_str());
		ec = EC_FAILURE;
		goto out_err;
	}
	ec = EC_SUCCESS;
	out_err: return ec;
}

int CT_HOST_CORE::f_state_restore_start(std::string const & in_str_file) {
	CT_NODE c_config;
	int ec;

	c_config.from_xml_file(in_str_file);
	//std::cout << c_config << std::endl;
	//fflush(0);
	//fflush(stderr);
	//_WARN << "Dumping configuration node:" << c_config.to_xml();
	//fflush(0);
	//fflush(stderr);
	_WARN << "Initiate state restore:" << in_str_file;
	CT_GUARD<CT_TASK> c_task = CT_HOST::tasks->f_new<std::string>(
			"HOST_CORE_RESTORE_STATE");

	{
		std::string & c_tmp = c_task->f_get_data<std::string>();
		c_tmp = in_str_file;
	}
	{
		CT_PORT_NODE_GUARD c_node(E_ID_CORE_SETTINGS, c_config.to_xml());

		c_task->f_status_init(*c_node, 0);

		//D("Receive state save on main node: %s",	c_task->f_get_data<std::string>().c_str());

		ec = f_port_cfg_get("__parent__").f_receive(c_node);
		if (ec != EC_SUCCESS) {
			CRIT("Unable to execute cfg event (current:%s)",
					f_get_name().c_str());
			c_task->f_error();
			goto out_err;
		}
	}

	ec = EC_SUCCESS;
	out_err: return ec;
}

int CT_HOST_CORE::f_state_save_start(std::string const & in_str_file) {
	int ec;
	//D("Initiate state save: %s", in_str_file.c_str());
	CT_GUARD<CT_TASK> c_task;
	c_task = CT_HOST::tasks->f_new<std::string>("STATE SAVE LOAD");

	{
		std::string & c_tmp = c_task->f_get_data<std::string>();
		c_tmp = in_str_file;
	}

	{
		CT_PORT_NODE_GUARD c_node(E_ID_CORE_SAVE_STATE);

		c_task->f_status_init(*c_node, 0);

		//D("Receive state save on main node: %s",c_task->f_get_data<std::string>().c_str());

		ec = f_port_cfg_get("__parent__").f_receive(c_node);
		if (ec != EC_SUCCESS) {
			CRIT("Unable to execute cfg event (current:%s)",
					f_get_name().c_str());
			c_task->f_error();
			goto out_err;
		}
	}

	ec = EC_SUCCESS;
	out_err: return ec;
}

int CT_HOST_CORE::f_config_load(CT_NODE & in_c_node) {
	int ec;

	{
		//D("CONFIG: %s", in_c_node.to_xml().c_str());
		CT_GUARD<CT_TASK> c_task = CT_HOST::tasks->f_new<CT_NODE>(
				"CONFIG LOAD");
		c_task->f_notification_set_id(E_ID_CORE_HOST_CONFIG_LOAD_STATUS);
		c_task->f_notification_set_external_port(f_port_cfg_get("__parent__"));
		c_task->f_step_set_nb(E_TASK_CONFIG_LOAD_STEP_NB);
		c_task->f_enable();
		/* Fill reply node with task status if present */
		c_task->f_get_data<CT_NODE>().copy(in_c_node);

		//DBG_LINE();
		//D("%s", c_task->f_get_data<CT_NODE>().to_xml().c_str());
		// Execute post init
		{
			int ec;
			CT_PORT_NODE_GUARD c_tmp(E_ID_CORE_PRE_PROFILE);
			c_task->f_status_init(*c_tmp, E_TASK_CONFIG_LOAD_STEP_PRE_CONFIG);
			ec = f_port_cfg_get("__parent__").f_receive(c_tmp);
			if (ec != EC_SUCCESS) {

				CRIT("Unable to execute pre profile");
				goto out_err;
			}
		}
	}

	ec = EC_SUCCESS;
	out_err: return ec;
}

int CT_HOST_CORE::f_config_close() {
	int ec;

	{
		//D("CONFIG CLOSE");
		CT_GUARD<CT_TASK> c_task = CT_HOST::tasks->f_new<CT_NODE>(
				"CONFIG CLOSE");
		c_task->f_notification_set_id(E_ID_CORE_HOST_CONFIG_CLOSE_STATUS);
		c_task->f_notification_set_external_port(f_port_cfg_get("__parent__"));
		c_task->f_step_set_nb(E_TASK_CONFIG_CLOSE_STEP_NB);
		c_task->f_enable();

		{
			int ec;
			CT_PORT_NODE_GUARD c_tmp(E_ID_CORE_PRE_DESTROY);
			c_task->f_status_init(*c_tmp, E_TASK_CONFIG_CLOSE_STEP_PRE_DESTROY);
			ec = f_port_cfg_get("__parent__").f_receive(c_tmp);
			if (ec != EC_SUCCESS) {
				CRIT("Unable to execute pre destroy");
				goto out_err;
			}
		}
	}

	ec = EC_SUCCESS;
	out_err: return ec;
}

bool CT_HOST_CORE::f_is_running() {
	return _b_running;
}
