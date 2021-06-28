/***********************************************************************
 ** qt.cc
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
 * @file qt.cc
 * QT Host definition.
 *
 * @author SEAGNAL (johann.baudy@seagnal.fr)
 * @date 2012
 *
 * @version 1.0 Original version
 */

/***********************************************************************
 * Includes
 ***********************************************************************/

#include <qt.hh>
#include <qt_property.hh>
#include <cpp/string.hh>
#include "ui_form_task.h"
#include <QTranslator>
#include <QThreadPool>
#include <QLibraryInfo>
#include <QDir>
/* UI includes */

CT_QT_HOST::CT_QT_HOST(struct ST_HOST_ARGS * in_ps_args) :
		CT_QT_NOGUI_HOST(in_ps_args, true) {
	_DBG << "Enabling QT HOST";
	setOrganizationName("SEAGNAL");
	setApplicationName("MASTER");

	_pc_main_window = NULL;

	 /* Add dock with logo */
	 {
		 QPixmap c_pix(":/images/splash.png");

		 QSplashScreen * pc_new = new QSplashScreen(NULL,c_pix);

		 pc_new->show();
		 pc_new->showMessage(QString(M_STR_VERSION),
		 Qt::AlignRight | Qt::AlignBottom, Qt::white); //This line represents the alignment of text, color and position
		 qApp->processEvents();
		 _pc_splash = pc_new;
		 //Sleeper::sleep(5);
	 }

	// Translate QT env
	{
		_pc_translator = new QTranslator();
		QString str_name_tr = "qt_" + QLocale::system().name();
		_pc_translator->load(str_name_tr,
				QLibraryInfo::location(QLibraryInfo::TranslationsPath));
		installTranslator(_pc_translator);
	}
}

CT_QT_HOST::~CT_QT_HOST() {
	removeTranslator(_pc_translator);
	delete _pc_translator;
	_pc_translator = NULL;

//	_DBG << _V(QThreadPool::globalInstance()->activeThreadCount());
//	_DBG << _V(QThreadPool::globalInstance());
}

int CT_QT_HOST::f_state_loaded(void) {
	// _WARN << " PROFILE LOADED !!!!!!!!!!!!!!!!!!!!!!!!!!";
	if(_pc_splash) {
		_pc_splash->close();
	}
	return EC_SUCCESS;
}

int CT_QT_HOST::f_main_init(void) {
	int ec;
	ec = CT_QT_NOGUI_HOST::f_main_init();
	if (ec != EC_SUCCESS) {
		CRIT("Unable to initialize QT-NOGUI main");
		goto out_err;
	}

	ec = EC_SUCCESS;
	out_err: return ec;
}

int CT_QT_HOST::f_main_loop(void) {
	/* Running QT Main LOOP */
	//_pc_main_window = new CT_QT_MAIN_WINDOW(dynamic_cast<QWidget*>(this));
	//_pc_main_window->show();
	//D("Executing QT main Loop");
	exec();
	//D("Exiting QT main Loop");
	return EC_SUCCESS;
}

int CT_QT_HOST::f_plugin_load(QFileInfo & in_c_module) {
	int ec;

	ec = CT_QT_NOGUI_HOST::f_plugin_load(in_c_module);
	if (ec != EC_SUCCESS) {
		return ec;
	}

	/* Looking for translation of module */
	{
		QTranslator *pc_tmp = new QTranslator();
		QString str_tmp = in_c_module.baseName() + "_"
				+ QLocale::system().name() + ".qm";
		bool b_tmp = pc_tmp->load(str_tmp,
				QDir(in_c_module.dir()).absolutePath());
		if (b_tmp) {
			installTranslator(pc_tmp);
			_DBG << "Loaded translation file: " << str_tmp.toStdString();
		}

	}
#if 0
	{

		char * str_tr = getenv("MODULE_PATH");

		if (!str_tr) {
			str_tr = (char*) "/usr/share/mviz/translation/";
		}

		/* Loop on all translation path */
		{
			QString c_paths_var(str_tr);

			QStringList c_paths = c_paths_var.split(';');
			/* Parse all path split with ; */
			for (QStringList::const_iterator c_it = c_paths.constBegin();
					c_it != c_paths.constEnd(); ++c_it) {

				QDir c_dir((*c_it).toAscii().constData());
				//D("Module folder: %s", (*c_it).toAscii().constData());

				c_dir.setSorting(QDir::Name);
				c_dir.setFilter(QDir::Files);

				QStringList filters;
				filters << in_c_module.baseName() + "_" + QLocale::system().name() + ".qm";
				c_dir.setNameFilters(filters);

				QStringList qsl = c_dir.entryList();

				for (QStringList::const_iterator c_it_file = qsl.constBegin();
						c_it_file != qsl.constEnd(); ++c_it_file) {
					QFileInfo finfo(c_dir, *c_it_file);
					D("Translation found: %s %s",
							(*c_it).toAscii().constData(), (char*)finfo.canonicalFilePath().toAscii().constData());

					{
						QTranslator *pc_tmp = new QTranslator();
						pc_tmp->load(*c_it_file, QString(c_dir));
						installTranslator(pc_tmp);
					}

				}
			}
		}

	}
#endif
	return EC_SUCCESS;
}

CT_HOST_PROPERTY * CT_QT_HOST::f_new_property() {
	return new CT_QT_HOST_PROPERTY();
}

master::core::CT_HOST_TASK_MANAGER * CT_QT_HOST::f_new_task_manager(void) {
	return new CT_QT_HOST_TASK_MANAGER();
}

#if 0
int
CT_QT_HOST::f_plugin_load(void) {
	return EC_FAILURE;
}
#endif

int CT_QT_HOST_PROPERTY_ELEMENT::f_data_refresh() {
	/* Update widget according to data */
	if (_pc_item) {
		switch (_pc_item->propertyType()) {
		case QVariant::Int: {
			_c_value = (int64_t) _pc_item->value().toInt();
			break;
		}
		case QVariant::Double: {
			_c_value = (double) _pc_item->value().toDouble();
			break;
		}
		case QVariant::Bool: {
			_c_value = (int64_t) _pc_item->value().toBool();
			break;
		}
		case QVariant::String: {
			_c_value = std::string(
					_pc_item->value().toString().toUtf8().constData());
			break;
		}
		default:
			if (_pc_item->propertyType()
					== QtVariantPropertyManager::enumTypeId()) {
				_c_value = (int) _pc_item->value().toInt();
			} else if (_pc_item->propertyType()
					== CT_QT_VARIANT_MANAGER::filePathTypeId())
				_c_value = std::string(
						_pc_item->value().toString().toUtf8().constData());

			else if (_pc_item->propertyType()
					!= QtVariantPropertyManager::groupTypeId()) {
				_CRIT << "Not supported type:" << _pc_item->propertyType()
						<< " objname:" << f_get_obj_name();
			}
			break;
		}
	}
	return EC_SUCCESS;
}

int CT_QT_HOST_PROPERTY_ELEMENT::f_data_update(void) {
	/* Update widget according to data */
	if (_pc_item) {
		switch (_pc_item->propertyType()) {
		case QVariant::Int: {
			_pc_item->setValue((int) _c_value);
			break;
		}
		case QVariant::Double: {
			_pc_item->setValue(double(_c_value));
			break;
		}
		case QVariant::Bool: {
			_pc_item->setValue((bool) _c_value);
			break;
		}
		case QVariant::String: {
			_pc_item->setValue(QString(std::string(_c_value).c_str()));
			break;
		}
		default:
			if (_pc_item->propertyType()
					== QtVariantPropertyManager::enumTypeId()) {
				_pc_item->setValue((int) _c_value);
			} else if (_pc_item->propertyType()
					== CT_QT_VARIANT_MANAGER::filePathTypeId())
				_pc_item->setValue(QString(std::string(_c_value).c_str()));
			else if (_pc_item->propertyType()
					!= QtVariantPropertyManager::groupTypeId()) {
				_CRIT << "Not supported type:" << _pc_item->propertyType()
						<< " objname:" << f_get_obj_name();
			}
			break;
		}
	}
	return EC_SUCCESS;
}

int CT_QT_HOST_PROPERTY_ELEMENT::f_data_update(CT_PORT_NODE & in_c_node) {
	int ec;
	/* Execute base class */
	ec = CT_HOST_PROPERTY_ELEMENT::f_data_update(in_c_node);
	if (ec != EC_SUCCESS) {
		CRIT("Unable to call data update (base) on element");
		goto out_err;
	}

	/* Update data */
	ec = f_data_update();
	if (ec != EC_SUCCESS) {
		CRIT("Unable to call data update ");
		goto out_err;
	}

	ec = EC_SUCCESS;
	out_err: return ec;
}

int CT_QT_HOST_PROPERTY_ELEMENT::f_data_update(CT_NODE & in_c_node) {
	int ec;
	/* Execute base class */
	ec = CT_HOST_PROPERTY_ELEMENT::f_data_update(in_c_node);
	if (ec != EC_SUCCESS) {
		CRIT("Unable to call data update (base) on element %s",
				in_c_node.to_xml().c_str());
		goto out_err;
	}

	/* Update data */
	ec = f_data_update();
	if (ec != EC_SUCCESS) {
		CRIT("Unable to call data update %s", in_c_node.to_xml().c_str());
		goto out_err;
	}

	ec = EC_SUCCESS;
	out_err: return ec;
}

int CT_QT_HOST_PROPERTY_ELEMENT::f_configure(CT_NODE & in_c_node,
		int in_i_count) {
	int ec;

	ec = CT_HOST_PROPERTY_ELEMENT::f_configure(in_c_node, in_i_count);
	if (ec != EC_SUCCESS) {
		CRIT("Unable to configure base of element: %s",
				std::string(in_c_node("name")).c_str());
		goto out_err;
	}

	/* Set QT Name */
	//_DBG << "QT Name: " << f_get_obj_name();

	{
		std::string str_tmp =
				in_c_node.has("title") ? in_c_node("title") : in_c_node("name");
		QString str_title = QWidget::tr(
				f_string_format(str_tmp.c_str(), in_i_count).c_str());
		if (in_c_node.has("child")) {
			//D("Creating group %s", _str_name.c_str());
			if (_pc_parent) {
				_pc_item =
						dynamic_cast<CT_QT_HOST_PROPERTY*>(_pc_property)->f_get_property_manager()->addProperty(
								QtVariantPropertyManager::groupTypeId(),
								str_title);

			} else {
				_pc_item = NULL;
			}
		} else {

			M_ASSERT(in_c_node.has("name"));

			M_ASSERT(in_c_node.has("type"));

			std::string str_type = in_c_node("type");
			//D("Creating child %s", str_title.c_str(), str_type.c_str());
			if (str_type == "int") {
				_pc_item =
						dynamic_cast<CT_QT_HOST_PROPERTY*>(_pc_property)->f_get_property_manager()->addProperty(
								QVariant::Int, str_title);

				if (in_c_node.has("min")) {
					_pc_item->setAttribute("minimum", (int) in_c_node("min"));
				}
				if (in_c_node.has("max")) {
					_pc_item->setAttribute("maximum", (int) in_c_node("max"));
				}
				if (in_c_node.has("default")) {
					_pc_item->setValue((int) in_c_node("default"));
				}
			} else if (str_type == "double") {
				_pc_item =
						dynamic_cast<CT_QT_HOST_PROPERTY*>(_pc_property)->f_get_property_manager()->addProperty(
								QVariant::Double, str_title);

				if (in_c_node.has("step")) {
					_pc_item->setAttribute("singleStep",
							(double) in_c_node("step"));
				}
				if (in_c_node.has("min")) {
					_pc_item->setAttribute("minimum", (double) in_c_node("min"));
				}
				if (in_c_node.has("max")) {
					_pc_item->setAttribute("maximum", (double) in_c_node("max"));
				}
				if (in_c_node.has("decimals")) {
					_pc_item->setAttribute("decimals",
							(int) in_c_node("decimals"));
				}
				if (in_c_node.has("default")) {
					_pc_item->setValue((double) in_c_node("default"));
				}
			} else if (str_type == "bool") {
				_pc_item =
						dynamic_cast<CT_QT_HOST_PROPERTY*>(_pc_property)->f_get_property_manager()->addProperty(
								QVariant::Bool, str_title);
				if (in_c_node.has("default")) {
					_pc_item->setValue((bool) in_c_node("default"));
				}
			} else if (str_type == "string") {
				_pc_item =
						dynamic_cast<CT_QT_HOST_PROPERTY*>(_pc_property)->f_get_property_manager()->addProperty(
								QVariant::String, str_title);
				if (in_c_node.has("default")) {
					_pc_item->setValue(
							QString(
									((std::string) in_c_node("default")).c_str()));
				}
			} else if (str_type == "file") {
				_pc_item =
						dynamic_cast<CT_QT_HOST_PROPERTY*>(_pc_property)->f_get_property_manager()->addProperty(
								CT_QT_VARIANT_MANAGER::filePathTypeId(),
								str_title);
				if (in_c_node.has("default")) {
					_pc_item->setValue(
							QString(
									((std::string) in_c_node("default")).c_str()));
				}

			} else if (str_type == "enum") {
				_pc_item =
						dynamic_cast<CT_QT_HOST_PROPERTY*>(_pc_property)->f_get_property_manager()->addProperty(
								QtVariantPropertyManager::enumTypeId(),
								str_title);

				QStringList enumNames;
				CT_NODE::childs_result v_tmp = in_c_node.get_childs("value");
				for (CT_NODE::childs_iterator pv_it = v_tmp.begin();
						pv_it != v_tmp.end(); pv_it++) {
					enumNames.append(
							QWidget::tr(((std::string) **pv_it).c_str()));
				}

				if (enumNames.size()) {
					_pc_item->setAttribute(QLatin1String("enumNames"),
							enumNames);
					if (in_c_node.has("default")) {
						_pc_item->setValue((int) in_c_node("default"));
					}
				}

			} else {
				CRIT("type :%s unknown", str_type.c_str());
				ec = EC_FAILURE;
				goto out_err;
			}

			if (in_c_node.has("description")) {
				_pc_item->setToolTip(
						QWidget::tr(
								((std::string) in_c_node("description")).c_str()));
			}

			if (in_c_node.has("disabled")) {
				_pc_item->setEnabled(false);
			}
			//dynamic_cast<CT_QT_HOST_PROPERTY_ELEMENT*>(_pc_parent)->f_get_property()->setAttribute( );

		}

		// Set name

		if (_pc_parent) {
			M_ASSERT(_pc_item);
			if (_pc_property->f_get_top() == _pc_parent) {
				dynamic_cast<CT_QT_HOST_PROPERTY*>(_pc_property)->f_get_property_editor()->addProperty(
						_pc_item);
			} else {
				M_ASSERT(
						dynamic_cast<CT_QT_HOST_PROPERTY_ELEMENT*>(_pc_parent)->f_get_property());
				dynamic_cast<CT_QT_HOST_PROPERTY_ELEMENT*>(_pc_parent)->f_get_property()->addSubProperty(
						_pc_item);
			}
		}
	}
	ec = EC_SUCCESS;
	out_err: return ec;
}

CT_QT_HOST_PROPERTY_ELEMENT::CT_QT_HOST_PROPERTY_ELEMENT(
		CT_HOST_PROPERTY * in_pc_property,
		CT_HOST_PROPERTY_ELEMENT * in_pc_parent) :
		CT_HOST_PROPERTY_ELEMENT(in_pc_property, in_pc_parent) {
#if 0
	D("Create QT Propertyy element %p %p (%p %p)",
			in_pc_property, in_pc_parent, _pc_property, _pc_parent);
#endif
#if 0
	if (in_pc_parent) {
		_pc_parent = dynamic_cast<CT_QT_HOST_PROPERTY_ELEMENT*>(in_pc_parent);
	} else {
		_pc_parent = NULL;
	}
	if (in_pc_new) {
		_pc_property = dynamic_cast<CT_QT_HOST_PROPERTY*>(in_pc_new);
	} else {
		_pc_property = NULL;
	}
#endif
	_pc_item = NULL;
}
void CT_QT_HOST_PROPERTY_ELEMENT::f_remove(void) {

	if (_pc_parent) {
		M_ASSERT(_pc_item);
		if (_pc_property->f_get_top() == _pc_parent) {
			dynamic_cast<CT_QT_HOST_PROPERTY*>(_pc_property)->f_get_property_editor()->removeProperty(
					_pc_item);
		} else {
			M_ASSERT(
					dynamic_cast<CT_QT_HOST_PROPERTY_ELEMENT*>(_pc_parent)->f_get_property());
			dynamic_cast<CT_QT_HOST_PROPERTY_ELEMENT*>(_pc_parent)->f_get_property()->removeSubProperty(
					_pc_item);
		}
	}
	setParent(NULL);

	/* Call base class remove */
	CT_HOST_PROPERTY_ELEMENT::f_remove();
}

CT_QT_HOST_PROPERTY_ELEMENT::~CT_QT_HOST_PROPERTY_ELEMENT() {
#if 0
	if (_pc_item) {
		delete _pc_item;
	}
#endif
}

CT_QT_HOST_PROPERTY::CT_QT_HOST_PROPERTY() :
		CT_HOST_PROPERTY() {
	_pc_variantmanager = new CT_QT_VARIANT_MANAGER(this);
	_pc_variantfactory = new CT_QT_VARIANT_FACTORY(this);
	_pc_varianteditor = new QtTreePropertyBrowser(this);
  _b_updated = true ;
	M_ASSERT(
			connect(_pc_variantmanager, SIGNAL(valueChanged(QtProperty*, const QVariant &)), this, SLOT(on_property_updated(QtProperty*,QVariant))));

	_pc_layout = new QVBoxLayout(this);
#if 0

#endif
}

CT_QT_HOST_PROPERTY::~CT_QT_HOST_PROPERTY() {
	//delete via QT
	//delete _pc_variantmanager;
	//delete _pc_variantfactory;
	//delete _pc_varianteditor;
}

int CT_QT_HOST_PROPERTY::f_set_from(CT_NODE & in_c_node) {
  _b_updated = false;
	int ec = _pc_top->f_set_from(in_c_node);
  _b_updated = true;
  emit updated();
  return ec;

}

int CT_QT_HOST_PROPERTY::f_set_from(CT_PORT_NODE & in_c_node) {
  _b_updated = false;
  int ec = _pc_top->f_set_from(in_c_node);
  _b_updated = true;
  emit updated();
  return ec;
}

void CT_QT_HOST_PROPERTY::on_property_updated(QtProperty* in_pc_property,
		QVariant) {
	CT_QT_HOST_PROPERTY_ELEMENT* pc_prop =
			dynamic_cast<CT_QT_HOST_PROPERTY_ELEMENT*>(in_pc_property);
	if (pc_prop) {

	}
  if(_b_updated){
    emit updated();
  }
}

int CT_QT_HOST_PROPERTY::f_set_dict(CT_NODE & in_c_node) {
	int ec;

	_pc_varianteditor->setFactoryForManager(_pc_variantmanager,
			_pc_variantfactory);

	ec = CT_HOST_PROPERTY::f_set_dict(in_c_node);
	if (ec != EC_SUCCESS) {
		CRIT("unable to set dict");
		goto out_err;
	}

	//M_ASSERT(	dynamic_cast<CT_QT_HOST_PROPERTY_ELEMENT*>(_pc_top)->f_get_property());
	//dynamic_cast<CT_QT_HOST_PROPERTY_ELEMENT*>(_pc_top)->f_get_property()->setEnabled(false);

	_pc_varianteditor->setPropertiesWithoutValueMarked(true);
	_pc_varianteditor->setRootIsDecorated(false);
	/* Reset Margin */
	_pc_layout->setContentsMargins(0, 0, 0, 0);
	_pc_varianteditor->setSizePolicy(QSizePolicy::Expanding,
			QSizePolicy::Expanding);
	_pc_layout->addWidget(_pc_varianteditor);
	_pc_varianteditor->show();

	ec = EC_SUCCESS;
	out_err: return ec;
}

CT_GUARD<CT_HOST_PROPERTY_ELEMENT> CT_QT_HOST_PROPERTY::on_new_element(
		CT_HOST_PROPERTY * in_pc_new, CT_HOST_PROPERTY_ELEMENT * in_pc_parent) {
	//D("New  property element %p %p", in_pc_new, in_pc_parent);
	CT_GUARD<CT_HOST_PROPERTY_ELEMENT> c_tmp = CT_GUARD<CT_HOST_PROPERTY_ELEMENT>(new CT_QT_HOST_PROPERTY_ELEMENT(in_pc_new, in_pc_parent));
	//_DBG << c_tmp.get();
	return c_tmp;
}

CT_QT_HOST_TASK::CT_QT_HOST_TASK(
		master::core::CT_TASK::task_key_t const & in_c_uuid) :
		master::core::CT_TASK(in_c_uuid) {
	_pc_widget = NULL;
}

void CT_QT_HOST_TASK::on_destroy(void) {
	if (_pc_widget) {
		delete _pc_widget;
		_pc_widget = NULL;
		delete _pc_ui;
		_pc_ui = NULL;
	}
}

void CT_QT_HOST_TASK::on_init(void) {

	_pc_widget = new QWidget();
	_pc_ui = new Ui::form_task();
	_pc_ui->setupUi(_pc_widget);

	M_ASSERT(
			connect(_pc_ui->toolButton, SIGNAL(clicked()), this, SLOT(on_button_clicked())));
	M_ASSERT(
			connect(this, SIGNAL(signal_update()), this, SLOT(on_signal_update())));
	on_update();
}
void CT_QT_HOST_TASK::on_signal_update(void) {
	if (!_pc_ui) {
		return;
	}
	/* Set Name */
	_pc_ui->label_name->setText(QString(f_get_name().c_str()));

	/* Set progress */
	if (f_get_progress() != 0) {
		switch (f_get_status()) {
		case master::core::E_TASK_STATE_ERROR:
			_pc_ui->progressBar->setVisible(false);
			break;
		default:
			_pc_ui->progressBar->setVisible(true);
			break;
		}

		if (f_get_progress() < 0) {
			_pc_ui->progressBar->setMaximum(0);
		} else {
			_pc_ui->progressBar->setMaximum(100);
			_pc_ui->progressBar->setValue(f_get_progress());
		}

	} else {
		_pc_ui->progressBar->setVisible(false);

	}

	/* Set msg */
	if (f_get_msg().size()) {
		_pc_ui->label_status->setVisible(true);
		_pc_ui->label_status->setText(QString(f_get_msg().c_str()));
	} else {
		_pc_ui->label_status->setVisible(false);

	}

	/* Set background color */
	switch (f_get_status()) {
	case master::core::E_TASK_STATE_RUNNING:
		if (f_get_progress() != 0) {
			//_pc_widget->setStyleSheet("QLabel { background-color : #0066CC; color : #FFFFFF }");
			//_pc_widget->setStyleSheet("QLabel { background-color : #99FFCC; }");
			_pc_widget->setStyleSheet("QLabel { }");
		} else {
			//_pc_widget->setStyleSheet("QLabel { background-color : #FFFFCC; }");
			_pc_widget->setStyleSheet("QLabel { }");
		}
		break;
	case master::core::E_TASK_STATE_COMPLETE:
		_pc_widget->setStyleSheet("QLabel { }");
		//_pc_widget->setStyleSheet("QLabel { background-color : #006666; }");
		break;
	case master::core::E_TASK_STATE_ERROR:
		_pc_widget->setStyleSheet(
				"QLabel { background-color : #800000; color : #FFFFFF }");
		break;
	default:
		_pc_widget->setStyleSheet("QLabel { }");
		break;
	}

}
void CT_QT_HOST_TASK::on_update(void) {
	//_pc_ui->progressBar->setValue(in_i_value);
	//_pc_ui->label_status->setText(QString(in_str_status.c_str()));
	emit signal_update();
}
void CT_QT_HOST_TASK::on_delete(void) {
	// _DBG<< "Delete CT_QT_HOST_TASK";
	deleteLater();
}

void CT_QT_HOST_TASK::on_button_clicked(void) {
	_WARN << "Manual Abort of task : " << f_get_name();
	f_abort();
}

CT_QT_HOST_TASK::~CT_QT_HOST_TASK() {
	//M_ASSERT(!_pc_widget->parent());
	//_DBG << "Deleting widget: "<< this;
	//M_ASSERT(_pc_widget == NULL);
	if (_pc_widget && !_pc_widget->parent()) {
		delete _pc_widget;
	}
}


CT_QT_HOST_TASK_MANAGER::CT_QT_HOST_TASK_MANAGER() :
		master::core::CT_HOST_TASK_MANAGER() {

	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

	/* Ui timeout */
	_pc_timer = new QTimer(this);
	M_ASSERT(connect(_pc_timer, SIGNAL(timeout()), this, SLOT(on_timeout()), Qt::QueuedConnection));
	_pc_timer->start(500);

	/* Layout scroll area */
	_pc_layout_top = new QVBoxLayout(this);
	_pc_layout_top->setContentsMargins(0, 0, 0, 0);

	/* Scroll stuff */
	_pc_scroll = new QScrollArea(this);
	_pc_scroll->setObjectName(QString::fromUtf8("scrollArea"));
	_pc_scroll->setWidgetResizable(true);
	//_pc_scroll->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

	_pc_scroll_contents = new QWidget();
	_pc_scroll_contents->setObjectName(
			QString::fromUtf8("scrollAreaWidgetContents"));
	//_pc_scroll_contents->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

	/* Layout task */
	_pc_layout = new QVBoxLayout();
	_pc_layout->setContentsMargins(0, 0, 0, 0);
	_pc_layout->setSpacing(0);
	_pc_scroll_contents->setLayout(_pc_layout);
	QSpacerItem *item = new QSpacerItem(1, 1, QSizePolicy::Minimum,
			QSizePolicy::Expanding);
	_pc_layout->addSpacerItem(item);

	_pc_scroll->setWidget(_pc_scroll_contents);
	_pc_layout_top->addWidget(_pc_scroll);
#if 0
	M_ASSERT(
			connect(this, SIGNAL(signal_task_release()), this, SLOT(on_release()), Qt::QueuedConnection));
	M_ASSERT(connect(this, SIGNAL(signal_task_new()), this, SLOT(on_new()), Qt::QueuedConnection));
#endif
}
CT_QT_HOST_TASK_MANAGER::~CT_QT_HOST_TASK_MANAGER() {
	on_timeout();

	for (_m_tasks_ui_iterator_t pc_it = _m_tasks_ui.begin();
			pc_it != _m_tasks_ui.end(); pc_it++) {
		_WARN << pc_it->second->f_get_name() << " still UP";
	}

	//M_ASSERT(!_m_tasks_ui.size());
}

void CT_QT_HOST_TASK_MANAGER::on_timeout(void) {

	CT_GUARD_LOCK c_guard_lock(_c_manager_lock);

	on_new();
	on_release();

}

void CT_QT_HOST_TASK_MANAGER::on_new(void) {


	while (_l_new_task.size()) {
		CT_GUARD<master::core::CT_TASK> & pc_task = _l_new_task.front();

		CT_QT_HOST_TASK * pc_tmp = static_cast<CT_QT_HOST_TASK*>(pc_task.get());
		pc_tmp->on_init();

		_pc_layout->insertWidget(_pc_layout->count()-1, pc_tmp->f_get_widget());

		M_ASSERT(_m_tasks_ui.find(pc_task->f_get_uid()) == _m_tasks_ui.end());
		_m_tasks_ui[pc_task->f_get_uid()] = pc_task;
#if 0
		pc_tmp->f_set_name(pc_task->f_get_name());
		pc_tmp->f_set_status(pc_task->f_get_status());
		pc_tmp->f_set_value(pc_task->f_get_progress());
#endif
#if 0
		_DBG << "New task: " << pc_task->f_get_name()
				<< _V(pc_task->f_get_uid());
#endif
		_l_new_task.pop_front();
	}
}

void CT_QT_HOST_TASK_MANAGER::on_release(void) {


	while (_l_release_task.size()) {
		//_DBG << "on release:" << _l_release_task.size();
		CT_GUARD<master::core::CT_TASK> & pc_task = _l_release_task.front();

		if (_m_tasks_ui.find(pc_task->f_get_uid()) != _m_tasks_ui.end()) {
			if (f_guard_dbg_in(pc_task.get())) {
				_DBG << " Releasing " << pc_task.get();
			}

			CT_QT_HOST_TASK * pc_tmp =
					static_cast<CT_QT_HOST_TASK*>(pc_task.get());
			//_m_tasks_ui_iterator_t pc_it = _m_tasks_ui.find(pc_task->f_get_uid());
			//M_ASSERT(pc_it != _m_tasks_ui.end());
			/* Remove from widget */
			_pc_layout->removeWidget(pc_tmp->f_get_widget());
			//_DBG << "Delete task: " << pc_task->f_get_name()<< _V(pc_task->f_get_uid());
			pc_tmp->on_destroy();

			/* Remove from map */

			_m_tasks_ui.erase(pc_task->f_get_uid());
		} else {
			_WARN << "task already released: " << pc_task->f_get_name()
					<< _V(pc_task->f_get_uid());
		}
		_l_release_task.pop_front();
	}
}

CT_GUARD<master::core::CT_TASK> CT_QT_HOST_TASK_MANAGER::f_new_task(
		master::core::CT_TASK::task_key_t const & in_c_uuid) {
	CT_QT_HOST_TASK* pc_tmp = new CT_QT_HOST_TASK(in_c_uuid);
	pc_tmp->moveToThread(thread());

	return CT_GUARD<master::core::CT_TASK>(pc_tmp);
}

void CT_QT_HOST_TASK_MANAGER::on_new(
		CT_GUARD<master::core::CT_TASK> & in_pc_task) {
	{
		CT_GUARD_LOCK c_guard_lock(_c_manager_lock);
		_l_new_task.push_back(in_pc_task);
	}
	// emit signal_task_new();
}

void CT_QT_HOST_TASK_MANAGER::on_release(
		CT_GUARD<master::core::CT_TASK> & in_pc_task) {
	M_ASSERT(in_pc_task.get());

	{
		CT_GUARD_LOCK c_guard_lock(_c_manager_lock);
#if 0
		/* Remove from new list if still not processed */
		if(_l_new_task.find(in_pc_task) != _l_new_task.end()) {
			_l_new_task.remove(in_pc_task);
		} else
		// {
#else
		if (f_guard_dbg_in(in_pc_task.get())) {
			_DBG << "Release " << in_pc_task.get();
		}
		//_DBG << "Release " << in_pc_task.get();
		_l_release_task.push_back(in_pc_task);
#endif
//	}

	}
#if 0
	{
		/* Keep task at least 1 sec */
		_pc_timer->start(500);
	}
#endif
}

QVBoxLayout * CT_QT_HOST_TASK_MANAGER::f_get_layout() {
	return _pc_layout;
}

CT_HOST * f_host_qt(struct ST_HOST_ARGS * in_ps_args) {
  QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
	CT_QT_HOST * pc_host = new CT_QT_HOST(in_ps_args);
	return dynamic_cast<CT_HOST*>(pc_host);
}
