/***********************************************************************
 ** boost.cc
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
 * @file boost.cc
 * HOST definition using BOOST Library.
 *
 * @author SEAGNAL (johann.baudy@seagnal.fr)
 * @date 2012
 *
 * @version 1.0 Original version
 */

/***********************************************************************
 * Includes
 ***********************************************************************/

#include <boost.hh>

#include <iostream>
#ifdef LF_BOOST_SHARED_LIB
#include <boost/extension/shared_library.hpp>
#else
#include <dlfcn.h>
#endif
#include <boost/function.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
//#include <boost/regex.hpp>
#include <regex>
#include <c/misc.h>
#include <sys/stat.h>

using namespace std;
using namespace boost::interprocess;

/***********************************************************************
 * Defines
 ***********************************************************************/

/***********************************************************************
 * Types
 ***********************************************************************/

/* UI includes */

struct ST_HOST_MSG {
	uint i_dummy;
};

CT_BOOST_HOST::CT_BOOST_HOST(struct ST_HOST_ARGS * in_ps_args) :
		CT_HOST(in_ps_args) {
	_b_running = false;
}

CT_BOOST_HOST::~CT_BOOST_HOST() {
	f_pre_destroy();
}

bool f_compare_timeout(CT_BOOST_TIMEOUT * first, CT_BOOST_TIMEOUT * second) {
	if (first->f_get_next_time() < second->f_get_next_time()) {
		return true;
	} else {
		return false;
	}
}

int CT_BOOST_HOST::f_timeout_start(CT_BOOST_TIMEOUT * in_pc_timeout) {
	boost::lock_guard<boost::mutex> _c_guard(_c_timeout_lock);
	bool b_push = true;
	std::list<CT_BOOST_TIMEOUT*>::iterator pc_it;
	/* Exit if already exist */
	for (pc_it = _l_timeouts.begin(); pc_it != _l_timeouts.end(); ++pc_it) {
		if (*pc_it == in_pc_timeout) {
			b_push = false;
		}
	}
	if (b_push) {
		_l_timeouts.push_front(in_pc_timeout);
	}
	_l_timeouts.sort(f_compare_timeout);
	return EC_SUCCESS;
}

int CT_BOOST_HOST::f_timeout_stop(CT_BOOST_TIMEOUT * in_pc_timeout) {
	boost::lock_guard<boost::mutex> _c_guard(_c_timeout_lock);

	std::list<CT_BOOST_TIMEOUT*>::iterator pc_it;
	for (pc_it = _l_timeouts.begin(); pc_it != _l_timeouts.end(); ++pc_it) {
		if (*pc_it == in_pc_timeout) {
			pc_it = _l_timeouts.erase(pc_it);
			if (pc_it == _l_timeouts.end())
				break;
		}
	}

	return EC_SUCCESS;
}

int CT_BOOST_HOST::f_timeout_process(void) {
	CT_HOST_TIMEOUT * pc_old = NULL;
	uint64_t i_now = f_get_time_ns64();
	int ec = EC_BYPASS;
	/* Check end of list */
	boost::lock_guard<boost::mutex> _c_guard(_c_timeout_lock);

	while (_l_timeouts.size()) {
		std::list<CT_BOOST_TIMEOUT*>::iterator pc_it = _l_timeouts.begin();
		CT_HOST_TIMEOUT * pc_tmp = *pc_it;
		M_ASSERT(pc_old != pc_tmp);
		/* Get time of timeout */
		pc_old = pc_tmp;
		uint64_t i_time = pc_tmp->f_get_next_time();

		/* Check time with now */
		if (i_time < i_now) {
			ec = EC_SUCCESS;
			//_WARN << "TIMETOUT TRIGGER";
			pc_tmp->f_trigger();
			//DBG_LINE();
			if (pc_tmp->f_get_interval()) {
				//_DBG << "Adding interval : "<< pc_tmp->f_get_interval();
				pc_tmp->f_set_next_time(i_time + pc_tmp->f_get_interval());
				//DBG_LINE();
				_l_timeouts.sort(f_compare_timeout);
				//DBG_LINE();
				/* Exit if same timeout is still in first place */
				if (*_l_timeouts.begin() == pc_tmp) {
					break;
				}
			} else {
				DBG_LINE();
				_l_timeouts.erase(pc_it);
			}
		} else {
			break;
		}
	}
	//_WARN << ec;
	return ec;
}

std::list<CT_HOST_FILE> CT_BOOST_HOST::f_file_list(
		CT_HOST_FILE const in_str_path, std::string const & in_str_prefix,
		std::string const & in_str_suffix) {
	std::list<CT_HOST_FILE> _l_tmp;
	std::string str_path(in_str_path);

	std::vector<std::string> c_paths;
	boost::split(c_paths, str_path, boost::is_any_of(";"));

	/* Parse all path split with ; */
	for (std::vector<std::string>::const_iterator c_it = c_paths.begin();
			c_it != c_paths.end(); ++c_it) {
		boost::filesystem::path someDir(*c_it);
		boost::filesystem::directory_iterator end_iter;

		auto str_re = f_string_format("(.*)(/)(%s)(.*)(\\.)%s",
				in_str_prefix.c_str(), in_str_suffix.c_str());
		const std::regex c_filter(str_re);

		D("Path: %s %s", someDir.generic_string().c_str(), str_re.c_str());
		if (boost::filesystem::exists(someDir)
				&& boost::filesystem::is_directory(someDir)) {

			/* Parse all file of current folder */
			for (boost::filesystem::directory_iterator dir_iter(someDir);
					dir_iter != end_iter; ++dir_iter) {
				/* Check that current file is regular */
				if (boost::filesystem::is_regular_file(dir_iter->status())) {
					std::string str_tmp = dir_iter->path().generic_string();

					D("Match: %s",str_tmp.c_str());
					/* Match plugin regexp */
					if (std::regex_match(str_tmp.begin(), str_tmp.end(),
							c_filter)) {
						_l_tmp.push_back(str_tmp);
					}
				}
			}
		}
	}

	return _l_tmp;
}

int CT_BOOST_HOST::f_main_init(void) {
	return CT_HOST::f_main_init();
}

int CT_BOOST_HOST::f_main_loop(void) {
	//struct ST_HOST_MSG s_tmp;
	//size_t i_size;
	//unsigned int i_priority = 1;
	//int ec;
	_b_running = true;
	/* Running QT Main LOOP */
	while (_b_running) {
#if 0
		boost::posix_time::ptime c_now(
				boost::posix_time::microsec_clock::universal_time());
		boost::posix_time::ptime c_timeout = c_now
		+ boost::posix_time::milliseconds(f_get_timeout());
#endif
		//D("Waiting %d ms", f_get_timeout());
#if 0
		_c_mq.timed_receive(&s_tmp, sizeof(s_tmp), i_size, i_priority,
				c_timeout);
#endif
		usleep(f_get_timeout() * 1e3);
		//_c_mq.try_receive(&s_tmp, sizeof(s_tmp), i_size, i_priority);
		//D("End waiting");
		f_execute_single();
	}
	return EC_SUCCESS;
}

int CT_BOOST_HOST::f_join(void) {
	_b_running = false;
	return EC_FAILURE;
}

int CT_BOOST_HOST::f_plugin_close(struct ST_PLUGIN_INFO & inout_s_info) {
	int ec;

	D("Closing %s", inout_s_info.str_library_path.c_str());
	/* Close library */
	ec = dlclose(inout_s_info.pv_handle);
	if (ec != 0) {
		CRIT("Unable to close library");
		ec = EC_FAILURE;
		goto out_err;
	}
	ec = EC_SUCCESS;
	out_err: return ec;
}

int CT_BOOST_HOST::f_plugin_reload(struct ST_PLUGIN_INFO & inout_s_info) {
	int ec;
	FT_PLUGIN_INFO pf_info;
	struct ST_PLUGIN_DESCRIPTION *ps_desc;
	void * pv_lib;

	/* Close library */
	ec = f_plugin_close(inout_s_info);
	if (ec != EC_SUCCESS) {
		CRIT("Unable to close library");
		goto out_err;
	}

	/* Ensure library is not in memory any more */
	pv_lib = dlopen(inout_s_info.str_library_path.c_str(), RTLD_NOLOAD);
	if (pv_lib) {
		CRIT("Library %s is still in  memory",
				inout_s_info.str_library_path.c_str());
		ec = EC_FAILURE;
		goto out_err;
	}

	/* Open library */
	pv_lib = dlopen(inout_s_info.str_library_path.c_str(), RTLD_LAZY);
	if (!pv_lib) {
		CRIT("Unable to load library : %s (%s)",
				inout_s_info.str_library_path.c_str(), dlerror());
		ec = EC_FAILURE;
		goto out_err;
	}

	/* Get plugin info symbol */
	pf_info = (FT_PLUGIN_INFO) dlsym(pv_lib, "f_plugin_info");
	if (!pf_info) {
		CRIT("Unable to find init symbol: %s", dlerror());
		ec = EC_FAILURE;
		goto out_err;
	}

	/* Execute plugin info symbol */
	ps_desc = pf_info();
	if (!ps_desc) {
		CRIT("Unable to load module");
		goto out_err;
	}

	/* Stats module */
	ec = f_plugin_get_mtime(inout_s_info.str_library_path.c_str(),
			&inout_s_info.i_time_modification);
	if (ec != EC_SUCCESS) {
		CRIT("Unable to get modification time of %s",
				inout_s_info.str_library_path.c_str());
		ec = EC_FAILURE;
		goto out_err;
	}

	/* Store info */
	inout_s_info.ps_desc = ps_desc;
	inout_s_info.b_updated = false;
	inout_s_info.pv_handle = pv_lib;

	ec = EC_SUCCESS;
	out_err: return ec;
}

int CT_BOOST_HOST::f_plugin_load(char const * in_str_module) {
	D("Loading module: %s", in_str_module);
	int ec = EC_FAILURE;

#ifdef LF_BOOST_SHARED_LIB
	using namespace boost::extensions;

	shared_library c_lib(in_str_module);

	if (!c_lib.open()) {
		CRIT("Unable to load library : %s", in_str_module);
		ec = EC_FAILURE;
		goto out_err;
	}

	boost::function<int (CT_HOST*)>
	pf_init(lib.get<int, CT_HOST*>("f_plugin_init"));
	if(!pf_init) {
		CRIT("Unable to find init symbol");
		ec = EC_FAILURE;
		goto out_err;
	}

#else

	FT_PLUGIN_INFO pf_info;

	void * pv_lib = dlopen(in_str_module, RTLD_LAZY);
	if (!pv_lib) {
		CRIT("Unable to load library : %s (%s)", in_str_module, dlerror());
		ec = EC_FAILURE;
		goto out_err;
	}

	pf_info = (FT_PLUGIN_INFO) dlsym(pv_lib, "f_plugin_info");

	if (!pf_info) {
		CRIT("Unable to find init symbol: %s", dlerror());
		ec = EC_FAILURE;
		goto out_err;
	}
#endif

	struct ST_PLUGIN_DESCRIPTION *ps_desc;
	ps_desc = pf_info();
	if (!ps_desc) {
		CRIT("Unable to load module");
		goto out_err;
	}

	{
		struct ST_PLUGIN_INFO s_info;

		ec = f_plugin_get_mtime(in_str_module, &s_info.i_time_modification);
		if (ec != EC_SUCCESS) {
			CRIT("Unable to get modification time of %s", in_str_module);
			ec = EC_FAILURE;
			goto out_err;
		}
		s_info.str_library_path = in_str_module;
		s_info.ps_desc = ps_desc;
		s_info.b_updated = false;
		s_info.pv_handle = pv_lib;

		ec = this->f_plugin_register(s_info);
		if (ec != EC_SUCCESS) {
			CRIT("Unable to register plugin");
			goto out_err;
		}
	}

	ec = EC_SUCCESS;
	out_err: if (ec != EC_SUCCESS) {
		if (pv_lib) {
			dlclose(pv_lib);
		}
	}
	return ec;
}

int CT_BOOST_HOST::f_plugin_load(void) {
	int ec;

	std::vector<std::string> c_paths;
	boost::split(c_paths, f_get_plugin_path(), boost::is_any_of(";"));

//D("Paths: %s",str_paths);

	/* Parse all path split with ; */
	for (std::vector<std::string>::const_iterator c_it = c_paths.begin();
			c_it != c_paths.end(); ++c_it) {
		boost::filesystem::path someDir(*c_it);
		boost::filesystem::directory_iterator end_iter;
		//.*[\\][-](\\.)so(\\\\)
		const std::regex c_filter("(.*)(/)(libplugin-)(.*)(\\.)so");

		D("Path: %s", someDir.generic_string().c_str());

		if (boost::filesystem::exists(someDir)
				&& boost::filesystem::is_directory(someDir)) {

			/* Parse all file of current folder */
			for (boost::filesystem::directory_iterator dir_iter(someDir);
					dir_iter != end_iter; ++dir_iter) {
				/* Check that current file is regular */
				if (boost::filesystem::is_regular_file(dir_iter->status())) {
					std::string str_tmp = dir_iter->path().generic_string();
					//boost::smatch what;
					//D("MAtch: %s",str_tmp.c_str());
					/* Match plugin regexp */
					if (std::regex_match(str_tmp.begin(), str_tmp.end(),
							c_filter)) {

						//D("MAtch: %s",str_tmp.c_str());
						ec = f_plugin_load(str_tmp.c_str());
						if (ec != EC_SUCCESS) {
							CRIT("Unable to load library: %s", str_tmp.c_str());
							goto out_err;
						}
					}
				}
			}
		}
	}
	ec = EC_SUCCESS;
	out_err: return ec;
}

CT_HOST * f_host_boost(struct ST_HOST_ARGS * in_ps_args) {
	CT_BOOST_HOST * pc_host = new CT_BOOST_HOST(in_ps_args);
	return dynamic_cast<CT_HOST*>(pc_host);
}

CT_HOST_LOCK * CT_BOOST_HOST::f_new_mutex() {
	return new CT_BOOST_MUTEX();
}

CT_HOST_CONTEXT * CT_BOOST_HOST::f_new_context(void) {
//	int ec;
	CT_HOST_CONTEXT * pc_tmp = new CT_BOOST_CONTEXT();
	return pc_tmp;
}

CT_HOST_LOCK * CT_BOOST_HOST::f_new_spinlock(void) {
	return new CT_BOOST_SPINLOCK();
}

CT_HOST_TIMEOUT * CT_BOOST_HOST::f_new_timeout(void) {
	return new CT_BOOST_TIMEOUT();
}

/**** SPINLOCK ****/
CT_BOOST_SPINLOCK::CT_BOOST_SPINLOCK() {
}

void CT_BOOST_SPINLOCK::f_lock(void) {
	_c_lock.lock();
}

void CT_BOOST_SPINLOCK::f_unlock(void) {
	_c_lock.unlock();
}

bool CT_BOOST_SPINLOCK::f_try_lock(void) {
	return _c_lock.try_lock();
}

/**** MUTEX ****/

void CT_BOOST_MUTEX::f_lock(void) {
	_c_lock.lock();
}

void CT_BOOST_MUTEX::f_unlock(void) {
	_c_lock.unlock();
}

bool CT_BOOST_MUTEX::f_try_lock(void) {
	return _c_lock.try_lock();
}

#if 0
void CT_BOOST_CONTI::f_notify_wait() {
	_c_cond.wait(boost::unique_lock(_c_lock, boost::defer_lock_t));
}
void CT_BOOST_MUTEX::f_notify_all() {
	_c_cond.notify_all();
}
void CT_BOOST_MUTEX::f_notify_one() {
	_c_cond.notify_one();
}
#endif

/**** CONTEXT ****/
CT_BOOST_CONTEXT::CT_BOOST_CONTEXT() :
		CT_HOST_CONTEXT() {
	_pc_thread = NULL;
}

CT_BOOST_CONTEXT::~CT_BOOST_CONTEXT() {
	WARN("Deleting context %p", _pc_thread);

	if (_pc_thread) {
		f_stop();
		delete _pc_thread;
		_pc_thread = NULL;
	}
}

int CT_BOOST_CONTEXT::f_init(void) {
	int ec;
	//WARN("Initializing context");
	try {
		_e_status = E_CONTEXT_STATUS_RUNNING;
		_pc_thread = new boost::thread(
				boost::bind(&CT_HOST_CONTEXT::f_execute, this));
	} catch (...) {
		_e_status = E_CONTEXT_STATUS_STOPPED;
		_b_stop = false;
		CRIT("Unable to create new thread");
		ec = EC_FAILURE;
	}
	ec = EC_SUCCESS;
	return ec;
}

int CT_BOOST_CONTEXT::f_join(void) {
	_pc_thread->join();
	return EC_SUCCESS;
}

/**** TIMEOUT ****/
CT_BOOST_TIMEOUT::CT_BOOST_TIMEOUT() {

}
CT_BOOST_TIMEOUT::~CT_BOOST_TIMEOUT() {
	f_stop();
}

void CT_BOOST_TIMEOUT::f_start(int in_i_ms, int in_i_interval) {
	CT_HOST_TIMEOUT::f_start(in_i_ms, in_i_interval);
	static_cast<CT_BOOST_HOST*>(CT_HOST::host)->f_timeout_start(this);
}
void CT_BOOST_TIMEOUT::f_stop() {
	static_cast<CT_BOOST_HOST*>(CT_HOST::host)->f_timeout_stop(this);
}
