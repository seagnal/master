/***********************************************************************
 ** qt_nogui.hh
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
#ifndef QT_NOGUI_HH_
#define QT_NOGUI_HH_

/**
 * @file qt_nogui.hh
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
#include <host.hh>
#include <QApplication>
#include <QMutex>
#include <QThread>
#include <QTimer>
#include <QFileInfo>

/***********************************************************************
 * Defines
 ***********************************************************************/

/***********************************************************************
 * Types
 ***********************************************************************/
class CT_QT_NOGUI_TIMEOUT: public QObject, public CT_HOST_TIMEOUT {
Q_OBJECT

	QTimer _c_timer;
public:
	CT_QT_NOGUI_TIMEOUT();
	/* virtual methods */
	virtual ~CT_QT_NOGUI_TIMEOUT();

	/* timeout methods */
	void f_start(int in_i_ms, int in_i_interval);
	void f_stop();

public Q_SLOTS:
	void f_timeout(void);
};

class CT_QT_NOGUI_MUTEX: public CT_HOST_MUTEX {
	QMutex _c_lock;
public:
	void f_lock();
	bool f_try_lock();
	void f_unlock();
};

class CT_QT_NOGUI_SPINLOCK: public CT_HOST_SPINLOCK {
	QMutex _c_lock;
public:
	void f_lock();
	bool f_try_lock();
	void f_unlock();
};

class CT_QT_NOGUI_CONTEXT: public QThread, public CT_HOST_CONTEXT {
Q_OBJECT
private:
	void run();
public:
	CT_QT_NOGUI_CONTEXT();
	~CT_QT_NOGUI_CONTEXT();

	int f_init(void);
	int f_join(void);
	int f_set_priority(int16_t in_i_priority);
};

class CT_QT_NOGUI_HOST: public QApplication, public CT_HOST {
Q_OBJECT

	QTimer _c_timer;
public:
	CT_QT_NOGUI_HOST(struct ST_HOST_ARGS * in_ps_args, bool in_b_gui = false);
	~CT_QT_NOGUI_HOST();

	/* Virtual redefined function */
	int f_main_init(void);
	int f_main_loop(void);
	int f_main_close(void);

	/* plugin methods */
	int f_plugin_load(void);
	int f_plugin_reload(struct ST_PLUGIN_INFO & inout_s_info);
	int f_plugin_close(struct ST_PLUGIN_INFO & inout_s_info);

	/* timeout method */
	int f_timeout_process(void);

	/* file methods */
	std::list<CT_HOST_FILE> f_file_list(CT_HOST_FILE const in_str_path,
			std::string const & in_str_prefix,
			std::string const & in_str_suffix);

	/* allocators */
	CT_HOST_LOCK * f_new_mutex(void);
	CT_HOST_LOCK * f_new_spinlock(void);
	CT_HOST_CONTEXT * f_new_context(void);
	CT_HOST_TIMEOUT * f_new_timeout(void);

	/* Throw support */
	bool notify(QObject * receiver, QEvent * event);

	int f_join(void);

public Q_SLOTS:
	void f_idle(void);

protected:
	virtual int f_plugin_load(QFileInfo & in_c_module);
};

CT_HOST * f_host_qt(struct ST_HOST_ARGS * in_ps_args);

#endif /* QT_NOGUI_HH_ */
