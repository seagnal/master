/***********************************************************************
 ** host.hh
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
#ifndef HOST_HH_
#define HOST_HH_
/**
 * @file host.hh
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

#include <c/common.h>
#include <cpp/debug.hh>
#include <cpp/guard.hh>
#include <node.hh>
#include <port.hh>
#include <core.hh>
#include <plugin.hh>
#include <context.hh>
#include <resource.hh>
#include <property.hh>

/***********************************************************************
 * Defines
 ***********************************************************************/

/***********************************************************************
 * Types
 ***********************************************************************/

namespace master {
namespace core {
class CT_HOST_TASK_MANAGER;
}
}
/*!
 * Host argument structure.
 *
 * Filled from command line information.
 * It contains arguments of main program.
 */
struct ST_HOST_ARGS {
  /*!
   * Flag of User interface.
   *
   * Host backend with User Interface can prevent User Interface
   * from popping up.
   */
    bool b_gui_enabled;

    /*!
     * Verbose mode.
     */
    bool b_debug_enabled;

    /*!
     *  Profile path
     */
    std::string str_config;

    /*!
     * Path of file where stdout/sterr is forwarded (optional).
     */
    std::string str_redirect;

    /*!
     * Number of program arguments
     */
    int argc;

    /*!
     * Program arguments
     */
    char ** argv;
};

/*!
 * Host Abstraction Layer of Timeout.
 *
 * This timeout send a CT_PORT_NODE on CT_PORT when timeout is reached.
 * An interval can be set, for repetitive events.
 *
 * The accuracy of timers depends on the underlying host backend.
 *
 */
class CT_HOST_TIMEOUT {
	/*! Port node & Port callback */
	struct ST_HOST_CONTEXT_CB_EXTRA _s_cb;

protected:
	/*! Start time of Timer */
	uint64_t _i_origin_trigger;
	/*! Trigger time */
	uint64_t _i_next_trigger;
	/*! Interval of timer */
	uint64_t _i_interval;
public:
  /*! Constructor */
	CT_HOST_TIMEOUT();
	/*! Destructor */
	virtual ~CT_HOST_TIMEOUT();

	/*!
	 * Starts or restarts the timer with a timeout interval of in_i_ms milliseconds.
   * if in_b_interval is set, repetitive event will be generated.
   *
   * @param in_i_ms       Number of millisecond between interval.
   * @param in_b_interval Repetitive mode of timer
   */
	virtual void f_start(int in_i_ms, int in_b_interval = 0);

  /*!
   * Stop the timer.
   */
	virtual void f_stop(void);


	/*!
	 * Set callback information of timeout.
	 *
	 * When timeout is triggered, a in_c_node CT_PORT_NODE will be sent through in_c_port CT_PORT.
	 *
   * @param in_c_port Port to be used as destination port
   * @param in_c_node Node to be sent through port
   */
	void f_set_cb(CT_PORT & in_c_port, CT_GUARD<CT_PORT_NODE> & in_c_node);

	/*!
	 * Internal trigger used by Host backend.
   */
	void f_trigger(void);


  /*!
   * Get interval between two events.
   *
   * @return time in milliseconds between two trigger
   */
	uint64_t f_get_interval() { return _i_interval; }

  /*!
   * Get next trigger time.
   *
   * Return the end time of current interval.
   * TODO Not working with Qt backend moved to BOOST only
   *
   * @return time in milliseconds
   */
  uint64_t f_get_next_time() { return _i_next_trigger; }
  /*!
   * Set next event time.
   *
   * Instead of interval, user can set absolute time of next timeout.
   * TODO Ignored in Qt backend moved to BOOST only
   *
   * @param  in_i_time time in milliseconds
   */
	void  f_set_next_time(uint64_t in_i_time) { _i_next_trigger = in_i_time; }
};

/*!
 * Lock base class. (ex: mutex, spinlock, ...)
 * Protect data from being simultaneously accessed form multiple threads.
 */
class CT_HOST_LOCK {
public:
  /*!
   * Constructor
   */
	CT_HOST_LOCK();
	/*!
	 * Destructor
	 */
	virtual ~CT_HOST_LOCK();

	/*!
	 * Locks the lock, blocks if the lock is not available
	 */
	virtual void f_lock();
  /*!
   * Tries to lock the lock, returns if the lock is not available.
   *
   * Tries to lock the lock. Returns immediately.
   * On successful lock acquisition returns true, otherwise returns false.
   *
   * @return [description]
   */
	virtual bool f_try_lock();
  /*!
   * Unlocks the lock.
   */
	virtual void f_unlock();
};

/*!
 * Lock class based on Mutex.
 *
 * Mutex provides one person to access a single resource at a time,
 * others must wait in a queue. Once this person is done,
 * the guy next in the queue acquire the resource.
 */
class CT_HOST_MUTEX : public CT_HOST_LOCK {
public:
  /*!
   * Constructor
   */
	CT_HOST_MUTEX();
	/*!
	 * Destructor
	 */
	virtual ~CT_HOST_MUTEX();
};


/*!
 * Lock class based on Spinlock.
 *
 * Spinlock is an aggressive mutex.
 * In mutex, if you find that resource is locked by someone else, you
 * (the thread/process) switch the context and start to wait (non-blocking)
 */
class CT_HOST_SPINLOCK : public CT_HOST_LOCK {
public:
  /*!
   * Constructor
   */
	CT_HOST_SPINLOCK();
  /*!
	 * Destructor
	 */
	virtual ~CT_HOST_SPINLOCK();
};

/*!
 * Host file path class.
 */
class CT_HOST_FILE {
  /*!
   * Absolute file path.
   */
	std::string str_abs_path;
public:
  /*!
   * Constructor from an absolute file path in std::string format.
   */
	CT_HOST_FILE(std::string const & in_str_abs_path);
  /*!
   * Constructor from an absolute file path in const char * format.
   */
	CT_HOST_FILE(char const * in_str_abs_path);

  /*!
   * Cast absolute path to std::string.
   *
   * @return absolute path in string format.
   */
	operator std::string() const;

  /*!
   * Join path with operator <<.
   *
   * Concatenate folder name and file name.
   */
	CT_HOST_FILE & operator <<=(CT_HOST_FILE & other) {
		str_abs_path += "/" + other.str_abs_path;
		return *this;
	}

  /*!
   * Join path with operator <<.
   *
   * Concatenate folder name and file name.
   */
	friend CT_HOST_FILE operator <<(CT_HOST_FILE const & old, CT_HOST_FILE  const& other) {
		return CT_HOST_FILE(old.str_abs_path+ "/" + other.str_abs_path);
	}
};


/*!
 * Host CORE.
 *
 * Host core is the root core of all core instanciated with profile description.
 * Only one instance per Host.
 *
 */
class CT_HOST_CORE : public CT_CORE {
	/*!
	 * Status flag for close.
	 * Flag set to true until application request a close.
   */
	bool _b_running;

public:
  /*!
   * Constructor
   */
	CT_HOST_CORE();

  /*!
   * Root event of configuration node.
   *
   * On Root, Node send to "__parent__" will end in this callback.
   * This method is handling all events
   *
   * @param  in_c_data CT_PORT_NODE (BML node with integer key)
   * @return           EC_SUCCESS in case of success
   *                   EC_FAILURE in case of failure
   *
   */
	int f_event_parent(CT_GUARD<CT_PORT_NODE> const & in_c_data);

	/*!
	 * Initiate a state restore process.
	 *
	 * @param  in_str_file path file which contain the state to be restored
	 *
   * @return           EC_SUCCESS in case of success
   *                   EC_FAILURE in case of failure
	 */
	int f_state_restore_start(std::string const & in_str_file);

  /*!
   * Initiate a state store process.
   *
   * @param  in_str_file path file where state will be restored

   * @return           EC_SUCCESS in case of success
   *                   EC_FAILURE in case of failure
   */
	int f_state_save_start(std::string const & in_str_file);

  /*!
   * Close successfully the state store process.
   *
   * Once root node has returned its state in
   * using CT_NODE format (BML node with string key),
   * the state will be writed into a file in_str_file.
   *
   * @param  in_str_file Path of file to write
   * @param  in_c_state  State in CT_NODE format to be stored in a file.
   *
   * @return           EC_SUCCESS in case of success
   *                   EC_FAILURE in case of failure
   */
	int f_state_save_end(std::string const & in_str_file, CT_NODE & in_c_state);

  /*!
   * Initiate a profile loading.
   *
   * Start profile loading by:
   * - creating a task with in_c_state as data.
   * - sending a E_ID_CORE_PRE_PROFILE to root node associated
   *   to task just created.
   *
   * @param  in_c_node profile in CT_NODE format (XML like)
   *
   * @return           EC_SUCCESS in case of success
   *                   EC_FAILURE in case of failure
   */
	int f_config_load(CT_NODE & in_c_node);

  /*!
   * Initiate profile destruction.
   *
   * Initiate a pre-destroy of all nodes by sending E_ID_CORE_PRE_DESTROY
   * to root node.
   *
   * @return           EC_SUCCESS in case of success
   *                   EC_FAILURE in case of failure
   */
	int f_config_close();

  /*!
   * Return running flag.
   *
   * Return true, until profile is destroyed from f_config_close().
   *
   * @return true if application is Running
   *         false if application
   */
	bool f_is_running();

};

/*!
 * Host base class.
 *
 * Abstration layer between Operating system and cores.
 *
 */
class CT_HOST : public CT_HOST_CONTEXT {
	/*!
	 * Slave mode of main application.
	 *
	 * Slave mode is enabled if config file is not provided through environnement
	 * variable or command line.
	 *
	 * On slave mode, root node has its configuration port mapped to a tcp server (port 5700).
	 * TODO: master core.
	 */
	bool _b_slave;

	/*!
	 *  Plugin folder.
	 *  Location of plugin database (List of shared library)
	 */
	std::string _str_plugin_path;

  /*!
	 *  Temporqry folder.
	 *  Location of temporary folder
	 */
	std::string _str_tmp_path;

	/*!
	 *  Home folder.
	 *  Absolute path of user home.
	 */
	std::string _str_home_path;

	/*!
	 *  Host arguments structure.
	 */
	struct ST_HOST_ARGS _s_args;

	/*!
	 *  CPU resource.
	 *  Node ressource allocator.
	 */
	CT_CPU_RESOURCE *_pc_cpures;

	/*!
	 * Init flag.
	 * Flag true from constructor to first run of main context.
	 */
	bool _b_init;

	/*!
	 * Idle context flag.
	 * Application is freezing all context during profile update.
	 * When true, this flag indicates that all other context than main context
	 * are sleeping.
	 */
	bool _b_idle_context;

public :
	/*!
	 * Map of plugins.
	 *
	 * Map of plugin available for CT_CORE and CT_TRANSPORT entities.
	 * Each plugin has an unique string identifier (type name).
	 * This name is used in profile xml.
	 *   - inside <node><type></type></node> for CT_CORE
	 *   - and <node><port><url></url></port></node> for CT_TRANSPORT
	 *
	 */
	std::map<std::string, ST_PLUGIN_INFO> _m_dict;

	/*!
	 *  Map of instantiated ports.
	 *
	 * Host is keeping track of all transport allocated during profile creation.
	 *
	 * Each transport has a unique string identifier (url) used as key of this map.
	 * Ex:
	 *   - func://function1
	 *   - msg://message_queue0
	 *   - tcp://192.168.0.1
	 *   - ...
	 *
	 */
	std::map<std::string, CT_GUARD<CT_TRANSPORT> > _m_transports;

	/*!
	 * Map of instantiated resources.
	 * Host is keeping track of all resource registred.
	 * Each resource has a unique string identifier (name) used as hey of this map.
	 */
	std::map<std::string, CT_GUARD<CT_RESOURCE> > _m_resources;

	/*!
	 * List of instantiated context.
	 * Host is keeping track of all context registred.
	 * This list allow host to put all context in idle.
	 */
	std::list<CT_HOST_CONTEXT*> _l_contexts;

	/*!
	 *  List of instantiated timer.
	 *  Host is keeping track of all timer registred.
	 *  TODO: Rename TIMEOUT => TIMER
	 */
	std::list<CT_HOST_TIMEOUT*> _l_timeouts;

	/*!
	 *  Root core.
	 */
	CT_HOST_CORE _c_main;

 /*!
 * BML ID database.
 * Database used for ID checking.
 */
 std::map<uint32_t, std::string> _m_id_db;

public:
  /*!
   * Constructor from Host argument structrure.
   *
   * Host class needs in_ps_args in order mainly to get:
   *  - plugin path
   *  - arguments of command line
   *  - debug mode
   *  - ...
   */
	CT_HOST(struct ST_HOST_ARGS * in_ps_args);

  /*!
   * Destructor.
   */
	virtual ~CT_HOST();

  /*!
   *  Debug purpose. TODO REMOVE
   */
	void f_pre_destroy(void);

	/*!
	 * Get transport from URL.
	 *
	 * If current URL does not exit in registred CT_TRANSPORT map,
	 *   Host will create a new CT_TRANSPORT.
	 * Otherwise, return registred CT_TRANSPORT.
	 *
	 * Method used during profile loading when mapping node ports to
	 * CT_TRANSPORT according to profile description.
	 *
	 * @param in_str_url URL under CT_NODE format
	 * @return smart pointer of CT_TRANSPORT
	 */
	CT_GUARD<CT_TRANSPORT> f_transport_get(CT_NODE& in_str_url);

	/*!
	 *  Get resource from name.
	 *
	 *  If current name does not exit in registred CT_RESOURCE map,
   *   Host will throw an exception.
   * Otherwise, return registred CT_RESOURCE.
   *
	 * @param in_str_name Name of resource to be searched for
	 * @return smart pointer of CT_RESOURCE
	 */
	CT_GUARD<CT_RESOURCE> f_resource_get(std::string in_str_name);


	//CT_GUARD<CT_RESOURCE> f_resource_create(std::string in_str_name, std::string in_str_type);

  /*!
   * Register a resource.
   *
   * Plugin can share a resource with other plugins.
   * Ex:  GPU resource
   *
   * Resource is idenfied with a name in_str_name.
   *
   * This method must be called from the plugin init function : __f_load(void)
   *
   *
   * @param  in_str_name    Name of resource to be registred
   * @param  in_pc_resource Pointer to resource class.
   * @return                EC_SUCCESS in case of success
   *                        EC_FAILURE in case of failure
   */
	int f_resource_register(std::string const &in_str_name, CT_RESOURCE * in_pc_resource);

  /*!
   * Unregister a resource.
   * TODO remove. Ressource is needed until end of application once registred. Application
   * do not verify that resource is not used somewhere in a node.
   *
   * @param  in_str_name  Name of resource to be unregistred
   * @return                EC_SUCCESS in case of success
   *                        EC_FAILURE in case of failure
   */
	int f_resource_unregister(std::string const &in_str_name);


	/*!
	 * Register a context.
	 *
	 * All context performing node communication (sending and receiving),
	 * must be registred to host. Otherwise host will not be able to make them
	 * sleeping during profile update. (->Crash)
	 *
	 * User do not have to perform this call. It is done by CT_HOST_CONTEXT
	 * constructor itself.
	 *
	 * @param  in_pc_ctx Pointer to host context to be registred
   * @return                EC_SUCCESS in case of success
   *                        EC_FAILURE in case of failure
	 */
	int f_context_register(CT_HOST_CONTEXT * in_pc_ctx);
  /*!
   * Unregistr context.
   *
   * As f_resource_unregister(), User do not have to perform this call.
   * It is done from CT_HOST_CONTEXT destructor.
   *
	 * @param  in_pc_ctx Pointer to host context to be unregistred
   * @return                EC_SUCCESS in case of success
   *                        EC_FAILURE in case of failure
   */
	int f_context_unregister(CT_HOST_CONTEXT * in_pc_ctx);

  /*!
   * Entering Idle mode.
   *
   * Force all context to idle. Meaning not performing any node transfert.
   * This is usefull while running a profile update for exemple.
   *
   * @return                EC_SUCCESS in case of success
   *                        EC_FAILURE in case of failure
   */
	int f_context_sleep(void);

  /*!
   * Leaving Idle mode.
   *
   * Force all context to wake-up (leaving idle mode) previously set with
   * f_context_sleep().
   *
   * @return                EC_SUCCESS in case of success
   *                        EC_FAILURE in case of failure
   */
	int f_context_wakeup(void);

  /*!
   * Check that all registred context are idling.
   *
   * @return  true if in idle mode
   *          false if not
   */
	bool f_context_is_idling(void);

	/*!
	 * Execute timeout process.
	 *
	 * Virtual function. May not be useful. Depending on host backend.
	 *
   * @return                EC_SUCCESS in case of success
   *                        EC_FAILURE in case of failure
	 */
	virtual int f_timeout_process(void);

  /*!
   * Register a Timer.
   *
   * @param  in_pc_timeout Timer to be registred
   *
   * @return                EC_SUCCESS in case of success
   *                        EC_FAILURE in case of failure
   */
	int f_timeout_register(CT_HOST_TIMEOUT * in_pc_timeout);

  /*!
   * Unregister a Timer.
   *
   * @param  in_pc_timeout Timer to be registred
   *
   * @return                EC_SUCCESS in case of success
   *                        EC_FAILURE in case of failure
   */
	int f_timeout_unregister(CT_HOST_TIMEOUT * in_pc_timeout);

  /*!
   * Clearing all timeout.
   * TODO Remove.
   *
   * @return                EC_SUCCESS in case of success
   *                        EC_FAILURE in case of failure
   */
	int f_timeout_clear(void);

	/*!
	 * Return file list of a directory.
	 *
	 * Parse a directory and return a list that contains each file.
	 *
	 * in_str_prefix and in_str_suffix can be used to filter files pushed
	 * into the returned list.
	 *
	 * @param in_str_path path of directory
	 * @param in_str_prefix prefix of file to be matched
	 * @param in_str_suffix suffix of file to be matched
	 * @return list of files contained in in_str_path
	 */
	virtual std::list<CT_HOST_FILE> f_file_list(CT_HOST_FILE const in_str_path,
				std::string const & in_str_prefix,
				std::string const & in_str_suffix) = 0;

/*!
 * Reload a profile.
 *
 * Plugin that matches in_ps_info will be dynamicaly unloaded then reloaded.
 *
 * @param  inout_s_info Plugin description of plugin to be reloaded
 *
 * @return                EC_SUCCESS in case of success
 *                        EC_FAILURE in case of failure
 */
	virtual int f_plugin_reload(struct ST_PLUGIN_INFO & inout_s_info);

  /*!
   * Close a plugin.
   *
   * Plugin that matches in_ps_info will be unloaded.
   *
   * @param  inout_s_info plugin description structure
   *
   * @return                EC_SUCCESS in case of success
   *                        EC_FAILURE in case of failure
   */
	virtual int f_plugin_close(struct ST_PLUGIN_INFO & inout_s_info);

  /*!
   * Register a plugin.
   *
   * Add plugin to database. Make the plugin available for profile loading.
   *
   * @param  inout_s_info plugin description structure
   *
   * @return                EC_SUCCESS in case of success
   *                        EC_FAILURE in case of failure
   */
	int f_plugin_register(ST_PLUGIN_INFO & in_pc_plugin);

  /*!
   * Check for plugin update.
   *
   * Verify of shared library of plugin has changed.
   * Modify b_updated with result.
   *
   * @return                EC_SUCCESS in case of success
   *                        EC_FAILURE in case of failure
   */
	int f_plugin_check(void);

  /*!
   * Reload all plugins with updated flag.
   *
   * @return                EC_SUCCESS in case of success
   *                        EC_FAILURE in case of failure
   */
	int f_plugin_update(void);

  /*!
   *  Get modification time of a plugin from filename.
   *  @param in_str_module file path of module
   *  @param out_pi_mtime time of last modification
   *
   *
   * @return                EC_SUCCESS in case of success
   *                        EC_FAILURE in case of failure
   */
	int f_plugin_get_mtime(char const * in_str_module,
			uint64_t * out_pi_mtime);

  /*!
   * Load all plugin from database (shared library directory).
   *
   * @return                EC_SUCCESS in case of success
   *                        EC_FAILURE in case of failure
   */
	virtual int f_plugin_load(void);

	/*!
	 * Load a profile in CT_NODE format.
	 *
	 * @param  in_c_node CT_NODE that contain profile
	 *
   * @return                EC_SUCCESS in case of success
   *                        EC_FAILURE in case of failure
	 */
	int f_config_load(CT_NODE & in_c_node);

  /*!
   * Load a profile from a file in XML format.
   *
   * @param  in_str_file absolute path of profile
   *
   * @return EC_SUCCESS in case of success
   *         EC_FAILURE in case of failure
   */
	int f_config_load(std::string const & in_str_file);

	/*!
	 * Restore state from a file in XML format.
	 *
	 * @param  in_str_file absolute path of file to be used for writing
	 *
   * @return EC_SUCCESS in case of success
   *         EC_FAILURE in case of failure
	 */
	int f_state_load(std::string const & in_str_file);

  /*!
   * Save state in a file in XML format.
   *
   * @param  in_str_file absolute path of file where state is stored
   *
   * @return EC_SUCCESS in case of success
   *         EC_FAILURE in case of failure
   */
	int f_state_save(std::string const & in_str_file);

  /*!
   * Event of state loaded.
   *
   * This method is called when state has been loaded.
   *
   * @return EC_SUCCESS in case of success
   *         EC_FAILURE in case of failure
   */
	virtual int f_state_loaded(void);

	/*!
	 * Instanciate a new core.
	 *
	 * Based on provided type name, looking in the database to find a match.
	 * If a match is found, allocate the new core, and update out_pc_new with
	 * the pointer.
	 *
	 * @param  out_pc_new  pointer to be updated with new core
	 * @param  in_str_type type name of core to be searched for
	 *
   * @return EC_SUCCESS in case of success
   *         EC_FAILURE in case of failure
	 */
	int f_core_new(CT_CORE* & out_pc_new, std::string const & in_str_type);

	/*!
	 * Host init function.
	 *
	 * Virtual fonction.
	 * Function called during program init.
	 *
	 * Initialization of Host must be done in this function.
	 *
   * @return EC_SUCCESS in case of success
   *         EC_FAILURE in case of failure
	 */
	virtual int f_main_init(void);


	/*!
	 * Host loop function.
	 *
	 * Virtual fonction. Function called in loop during main.
	 *
	 * This function must never return until end of program is requested.
	 *
   * @return EC_SUCCESS in case of success
   *         EC_FAILURE in case of failure
	 */
	virtual int f_main_loop(void);

  /*!
   * Host close.
   *
	 * Virtual fonction. Function called to destroy host.
   *
   * @return EC_SUCCESS in case of success
   *         EC_FAILURE in case of failure
   */
	virtual int f_main_close(void);


	/*!
	 * Create a host context.
	 * TODO Move to CT_GUARD.
	 *
	 * @return  pointer to created context.
	 */
	virtual CT_HOST_CONTEXT * f_new_context(void) = 0;


	/*!
	 * Create a host timer.
	 * TODO Move to CT_GUARD + Rename timeout.
	 *
	 * @return  pointer to created timer.
	 */
	virtual CT_HOST_TIMEOUT * f_new_timeout(void) = 0;


	/*!
	 * Create a host property.
	 * TODO Move to CT_GUARD.
	 *
	 * @return  pointer to created property.
	 */
	virtual CT_HOST_PROPERTY * f_new_property(void);

  /*!
   * Create a task manager.
	 * TODO Move to CT_GUARD.
   *
   * @return  pointer to new task manager
   */
	virtual master::core::CT_HOST_TASK_MANAGER * f_new_task_manager(void);

	/*!
	 * Virtual method of inherited CT_HOST_CONTEXT, Initialize event.
	 */
	int f_init(void);

	/*!
	 * Virtual method of inherited CT_HOST_CONTEXT, Run event.
	 *
	 * During Init :
	 *  - Loading profile
	 * During Run:
	 *  - Checking plugin update
	 *  - Running Timers process
	 */
	int f_run(CT_HOST_CONTEXT&);

 /*!
  * Create a task manager.
 * TODO Move to CT_GUARD.
  *
  * @return  pointer to new task manager
  */
 std::string const & f_id_name(uint32_t in_i_id);

	/*!
	 * Get plugin location.
	 *
	 * @return absolute file path of plugin directory
	 */
	std::string const f_get_plugin_path() { return _str_plugin_path; }

  /*!
	 * Get tmp location.
	 *
	 * @return absolute file path of plugin directory
	 */
	std::string const f_get_tmp_path() { return _str_tmp_path; }

  /*!
   * Get host structure arguments.
   *
   * @return reference to host arguments structure
   */
	struct ST_HOST_ARGS & f_get_args() { return _s_args; }

  /*!
   * Get common cpu ressource.
   *
   * @return reference of cpu resource.
   */
	CT_CPU_RESOURCE & f_get_cpu_resource( void) { return *_pc_cpures; }

  /*!
   * Global HOST pointer.
   */

	static CT_HOST * host;
  /*!
   * Global Task manager.
   */
	static master::core::CT_HOST_TASK_MANAGER * tasks;

  /*!
   * Return file path of a file.
   * Looking for file inside /usr/share/data
   * Looking for file inside ~/.master
   * Looking for file inside $VAR defined by in_str_var (default EMPTY)
   *
   * Return true, until profile is destroyed from f_config_close().
   *
   * @return true if application is Running
   *         false if application
   */
  static std::string f_looking_for_file(std::string in_str_file, std::string in_str_var = "", bool in_b_debug = false);
};
#else
class CT_HOST;
class CT_HOST_MUTEX;
#endif /* HOST_HH_ */
