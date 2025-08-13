/***********************************************************************
 ** qt_nogui.cc
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
 * @file qt_nogui.cc
 * QT NOGUI Host definition.
 *
 * @author SEAGNAL (johann.baudy@seagnal.fr)
 * @date 2012
 *
 * @version 1.0 Original version
 */

/***********************************************************************
 * Includes
 ***********************************************************************/
#include <qt_nogui.hh>
#include <QLibrary>
#include <QDir>
#include <QFileInfo>
#include <QMetaEnum>

using namespace std;

/* UI includes */

CT_QT_NOGUI_HOST::CT_QT_NOGUI_HOST(struct ST_HOST_ARGS * in_ps_args,
		bool in_b_gui) :
		QApplication(in_ps_args->argc, in_ps_args->argv, in_b_gui), CT_HOST(
				in_ps_args) {

}

CT_QT_NOGUI_HOST::~CT_QT_NOGUI_HOST() {
	f_pre_destroy();
}

void CT_QT_NOGUI_HOST::f_idle(void) {
	//WARN("idle");
	f_execute_single();
	//WARN("restart timeout %d",f_get_timeout());
	_c_timer.start(f_get_timeout());
}

int CT_QT_NOGUI_HOST::f_join(void) {
	this->exit(0);
	return EC_SUCCESS;
}

int CT_QT_NOGUI_HOST::f_main_init(void) {
	connect(&_c_timer, SIGNAL(timeout()), this, SLOT(f_idle()));
	_c_timer.start(0);

	return CT_HOST::f_main_init();
}

int CT_QT_NOGUI_HOST::f_main_loop(void) {

	/* Running QT Main LOOP */
	return exec();
}

int CT_QT_NOGUI_HOST::f_main_close(void) {

	return CT_HOST::f_main_close();
}

int CT_QT_NOGUI_HOST::f_timeout_process(void) {
	if (f_has_extra()) {
		return EC_SUCCESS;
	} else {
		return EC_BYPASS;
	}
}

int CT_QT_NOGUI_HOST::f_plugin_close(struct ST_PLUGIN_INFO & inout_s_info) {
	//int ec;
	//processEvents();
	D("Closing module: %s", inout_s_info.str_library_path.c_str());
	QLibrary* pc_lib = (QLibrary*) inout_s_info.pv_handle;
	pc_lib->unload();
	delete pc_lib;
	return EC_SUCCESS;
}
int CT_QT_NOGUI_HOST::f_plugin_reload(struct ST_PLUGIN_INFO & inout_s_info) {
	int ec;
	/* Close library */
	ec = f_plugin_close(inout_s_info);
	if (ec != EC_SUCCESS) {
		CRIT("Unable to close library");
		goto out_err;
	}

	/* Reload library */
	{
		QFileInfo c_module(QString(inout_s_info.str_library_path.c_str()));
		std::string str_module = c_module.fileName().toLatin1().constData();
		std::string str_abs_module =
				c_module.absoluteFilePath().toLatin1().constData();
		//D("ReLoading module: %s", str_abs_module.c_str());

		QLibrary * pc_lib = new QLibrary(str_module.c_str());
		FT_PLUGIN_INFO pf_info;

		if (!pc_lib->load()) {
			CRIT("Unable to load library : %s",
					pc_lib->errorString().toLatin1().constData());
			ec = EC_FAILURE;
			goto out_err;
		}

		pf_info = (FT_PLUGIN_INFO) pc_lib->resolve("f_plugin_info");
		if (!pf_info) {
			CRIT("Unable to find init symbol");
			ec = EC_FAILURE;
			goto out_err;
		}

		struct ST_PLUGIN_DESCRIPTION *ps_desc;
		ps_desc = pf_info();
		if (!ps_desc) {
			CRIT("Unable to load module");
			goto out_err;
		}

		{

			ec = f_plugin_get_mtime(str_abs_module.c_str(),
					&inout_s_info.i_time_modification);
			if (ec != EC_SUCCESS) {
				CRIT("Unable to get modification time of %s",
						str_abs_module.c_str());
				ec = EC_FAILURE;
				goto out_err;
			}
			D("%llx", inout_s_info.i_time_modification);
			inout_s_info.str_library_path = str_abs_module;
			inout_s_info.ps_desc = ps_desc;
			inout_s_info.b_updated = false;
			inout_s_info.pv_handle = (void*) pc_lib;

		}
	}

	ec = EC_SUCCESS;
	out_err: return ec;
}
int CT_QT_NOGUI_HOST::f_plugin_load(QFileInfo & in_c_module) {
	int ec = EC_FAILURE;
	std::string str_module = in_c_module.fileName().toLatin1().constData();
	std::string str_abs_module =
			in_c_module.absoluteFilePath().toLatin1().constData();
	//D("Loading module: %s", str_abs_module.c_str());

	QLibrary * pc_lib = new QLibrary(str_module.c_str());
	FT_PLUGIN_INFO pf_info;

	if (!pc_lib->load()) {
		CRIT("Unable to load library : %s",
				pc_lib->errorString().toLatin1().constData());
		ec = EC_FAILURE;
		goto out_err;
	}

	pf_info = (FT_PLUGIN_INFO) pc_lib->resolve("f_plugin_info");
	if (!pf_info) {
		CRIT("Unable to find init symbol");
		ec = EC_FAILURE;
		goto out_err;
	}

	struct ST_PLUGIN_DESCRIPTION *ps_desc;
	ps_desc = pf_info();
	if (!ps_desc) {
		CRIT("Unable to load module");
		goto out_err;
	}

	{
		struct ST_PLUGIN_INFO s_info;

		ec = f_plugin_get_mtime(str_abs_module.c_str(),
				&s_info.i_time_modification);
		if (ec != EC_SUCCESS) {
			CRIT("Unable to get modification time of %s",
					str_abs_module.c_str());
			ec = EC_FAILURE;
			goto out_err;
		}
		s_info.str_library_path = str_abs_module;
		s_info.ps_desc = ps_desc;
		s_info.b_updated = false;
		s_info.pv_handle = (void*) pc_lib;

		ec = this->f_plugin_register(s_info);
		if (ec != EC_SUCCESS) {
			CRIT("Unable to register plugin");
			goto out_err;
		}
	}

	out_err: return ec;
}

int CT_QT_NOGUI_HOST::f_plugin_load(void) {
	int ec;


	QString c_paths_var(f_get_plugin_path().c_str());

	QStringList c_paths = c_paths_var.split(';');
	/* Parse all path split with ; */
	for (QStringList::const_iterator c_it = c_paths.constBegin();
			c_it != c_paths.constEnd(); ++c_it) {

		QDir c_dir((*c_it).toLatin1().constData());
		//D("Module folder: %s", (*c_it).toLatin1().constData());

		c_dir.setSorting(QDir::Name);
		c_dir.setFilter(QDir::Files);

		QStringList filters;
		filters << "libplugin*.so";
		c_dir.setNameFilters(filters);

		QStringList qsl = c_dir.entryList();

		for (QStringList::const_iterator c_it_file = qsl.constBegin();
				c_it_file != qsl.constEnd(); ++c_it_file) {
			QFileInfo finfo(c_dir, *c_it_file);
			//D("Module folder: %s %s",	(*c_it).toLatin1().constData(), (char*)finfo.canonicalFilePath().toLatin1().constData());
			ec =f_plugin_load(finfo);
			if(ec != EC_SUCCESS) {
				CRIT("Unable to load plugin %s", finfo.canonicalFilePath().toLatin1().constData());
				goto out_err;
			}
		}
	}

	ec =EC_SUCCESS;
	out_err:
	return ec;
}

std::list<CT_HOST_FILE> CT_QT_NOGUI_HOST::f_file_list(CT_HOST_FILE const in_str_path, std::string const & in_str_prefix, std::string const & in_str_suffix) {
	std::list<CT_HOST_FILE> _l_tmp;
	std::string str_path(in_str_path);

	QString c_paths_var(str_path.c_str());

	QStringList c_paths = c_paths_var.split(';');
	/* Parse all path split with ; */
	for (QStringList::const_iterator c_it = c_paths.constBegin();
			c_it != c_paths.constEnd(); ++c_it) {

		QDir c_dir((*c_it).toLatin1().constData());
		//D("Parse folder: %s", (*c_it).toLatin1().constData());

		c_dir.setSorting(QDir::Name);
		c_dir.setFilter(QDir::Files);

		QStringList filters;
		filters << QString((in_str_prefix+"*" + in_str_suffix).c_str());
		c_dir.setNameFilters(filters);

		QStringList qsl = c_dir.entryList();

		for (QStringList::const_iterator c_it_file = qsl.constBegin();
				c_it_file != qsl.constEnd(); ++c_it_file) {
			QFileInfo finfo(c_dir, *c_it_file);
			_l_tmp.push_back(CT_HOST_FILE(finfo.absoluteFilePath().toLatin1().constData()));
		}
	}

	return _l_tmp;
}

bool CT_QT_NOGUI_HOST::notify(QObject * receiver, QEvent * event)  {
  try {
    return QApplication::notify(receiver, event);
  } catch(std::exception& e) {

	  if(event) {
		  static int eventEnumIndex = QEvent::staticMetaObject
		         .indexOfEnumerator("Type");
		   if (event) {
		      QString name = QEvent::staticMetaObject
		            .enumerator(eventEnumIndex).valueToKey(event->type());
		      if (!name.isEmpty()) {
		    	  _CRIT << "QEvent:" << name.toStdString();
		      } else {
		    	  _CRIT << "QEvent:" << (int)event->type();
		      };
		   } else {
			   _CRIT << "QEvent: NULL";
		   }
	  }
	 if(receiver) {
		 _FATAL << "Exception thrown from " << receiver->metaObject()->className() << " : "  << e.what();
	 } else {
		 _FATAL << "Exception thrown" << e.what() << _V(event);
	 }
  } catch(...) {
	  _FATAL << "Unknown Exception thrown";
  }
  return false;
}

CT_HOST_LOCK * CT_QT_NOGUI_HOST::f_new_mutex() {
	return new CT_QT_NOGUI_MUTEX();
}
CT_HOST_LOCK * CT_QT_NOGUI_HOST::f_new_spinlock() {
	return new CT_QT_NOGUI_SPINLOCK();
}

CT_HOST_CONTEXT * CT_QT_NOGUI_HOST::f_new_context(void) {
	return new CT_QT_NOGUI_CONTEXT();
}
CT_HOST_TIMEOUT * CT_QT_NOGUI_HOST::f_new_timeout(void) {
	return new CT_QT_NOGUI_TIMEOUT();
}
/**** SPINLOCK ****/

void CT_QT_NOGUI_SPINLOCK::f_lock(void) {
	_c_lock.lock();
}

void CT_QT_NOGUI_SPINLOCK::f_unlock(void) {
	_c_lock.unlock();
}

bool CT_QT_NOGUI_SPINLOCK::f_try_lock(void) {
	return _c_lock.tryLock();
}

/**** MUTEX ****/

void CT_QT_NOGUI_MUTEX::f_lock(void) {
	_c_lock.lock();
}

void CT_QT_NOGUI_MUTEX::f_unlock(void) {
	_c_lock.unlock();
}

bool CT_QT_NOGUI_MUTEX::f_try_lock(void) {
	return _c_lock.tryLock();
}

/**** CONTEXT ****/
CT_QT_NOGUI_CONTEXT::CT_QT_NOGUI_CONTEXT() : CT_HOST_CONTEXT() {
}

CT_QT_NOGUI_CONTEXT::~CT_QT_NOGUI_CONTEXT() {
	if (isRunning()) {
		f_stop();
	}
}

int CT_QT_NOGUI_CONTEXT::f_init(void) {
	start();
	return EC_SUCCESS;
}

void CT_QT_NOGUI_CONTEXT::run(void) {
	f_execute();
}

int CT_QT_NOGUI_CONTEXT::f_join(void) {
	wait();
	return EC_SUCCESS;
}


int CT_QT_NOGUI_CONTEXT::f_set_priority(int16_t in_i_priority){
	QThread::Priority QPrio;
	switch (in_i_priority) {
		case 0:{QPrio = QThread::IdlePriority;break;}
		case 1:{QPrio = QThread::LowestPriority;break;}
		case 2:{QPrio = QThread::LowPriority;break;}
		case 3:{QPrio = QThread::NormalPriority;break;}
		case 4:{QPrio = QThread::HighPriority;break;}
		case 5:{QPrio = QThread::HighestPriority;break;}
		case 6:{QPrio = QThread::TimeCriticalPriority;break;}
		case 7:{QPrio = QThread::InheritPriority;break;}
		default:{QPrio = QThread::NormalPriority;break;}

	}
	this->setPriority(QPrio);
	return EC_SUCCESS;
}

/**** TIMEOUT ****/
CT_QT_NOGUI_TIMEOUT::CT_QT_NOGUI_TIMEOUT() :
		CT_HOST_TIMEOUT() {
	// _pc_timer = new QTimer(this);
	//M_ASSERT(_pc_timer);
}
CT_QT_NOGUI_TIMEOUT::~CT_QT_NOGUI_TIMEOUT() {
	f_stop();
	//delete _pc_timer;
}

void CT_QT_NOGUI_TIMEOUT::f_timeout(void) {
	if (!_i_interval) {
		_c_timer.stop();
	}

	f_trigger();
}

void CT_QT_NOGUI_TIMEOUT::f_start(int in_i_ms, int in_i_interval) {
	CT_HOST_TIMEOUT::f_start(in_i_ms, in_i_interval);

	connect(&_c_timer, SIGNAL(timeout()), this, SLOT(f_timeout()));
	_c_timer.stop();
	//D("%d %d", in_i_ms, in_i_interval);
	_c_timer.start(in_i_ms);
	_i_interval = in_i_interval;
}
void CT_QT_NOGUI_TIMEOUT::f_stop() {
	_c_timer.stop();
}

CT_HOST * f_host_nogui_qt(struct ST_HOST_ARGS * in_ps_args) {
	CT_QT_NOGUI_HOST * pc_host = new CT_QT_NOGUI_HOST(in_ps_args);
	return dynamic_cast<CT_HOST*>(pc_host);
}
