/***********************************************************************
 ** guard.hh
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
 ** -------------------------------------------------------------------
 **
 ** As a special exception to the GNU Lesser General Public License, you
 ** may link, statically or dynamically, a "work that uses this Library"
 ** with a publicly distributed version of this Library to produce an
 ** executable file containing portions of this Library, and distribute
 ** that executable file under terms of your choice, without any of the
 ** additional requirements listed in clause 6 of the GNU Lesser General
 ** Public License.  By "a publicly distributed version of this Library",
 ** we mean either the unmodified Library as distributed by the copyright
 ** holder, or a modified version of this Library that is distributed under
 ** the conditions defined in clause 3 of the GNU Lesser General Public
 ** License.  This exception does not however invalidate any other reasons
 ** why the executable file might be covered by the GNU Lesser General
 ** Public License.
 **
 ***********************************************************************/

/* define against mutual inclusion */
#ifndef GUARD_HH_
#define GUARD_HH_

/**
 * @file guard.hh
 * GUARD pointer definition.
 *
 * @author SEAGNAL (johann.baudy@seagnal.fr)
 * @date 2013
 *
 * @version 1.0 Original version
 */

/***********************************************************************
 * Includes
 ***********************************************************************/

#include <c/common.h>
#include <cpp/debug.hh>
#include <cpp/memtrack.hh>
#include <c/atomic.h>
#include <cstddef>
#include <stdexcept>
/***********************************************************************
 * Defines
 ***********************************************************************/

/***********************************************************************
 * Types
 ***********************************************************************/

class CT_OBJECT_REF {
public:
	virtual ~CT_OBJECT_REF() {

	}
};

template<typename T>
class CT_OBJECT: public CT_OBJECT_REF {
	T _data;

public:
	operator T&() {
		return _data;
	}
};

struct ST_GUARD_USAGE {
	void * pc_pointer;
	int i_usage;
	bool b_free_usage;

	ST_GUARD_USAGE(bool in_b_free_usage, void * in_pc_pointer) {
		i_usage = 1;
		b_free_usage = in_b_free_usage;
		pc_pointer = in_pc_pointer;
	}

	virtual ~ST_GUARD_USAGE() {

	}

	virtual int unuse(void) {
		M_ASSERT(pc_pointer);
		return M_ATOMIC_DEC(i_usage);
		//D("[%p] UNUSE - %d %p %p %d",this, _ps_usage->b_free_usage, _pc_pointer, _ps_usage,i_use);
	}

	virtual int use(void) {
		return M_ATOMIC_INC(i_usage);
	}

};

template<typename T>
class CT_GUARD {
	T * _pc_pointer;
	ST_GUARD_USAGE * _ps_usage;

public:
	CT_GUARD() {
		_pc_pointer = NULL;
		_ps_usage = NULL;
		//WARN("[%p]",this);
	}

	CT_GUARD(T * in_pointer) {
		if (typeid(*in_pointer).name() == typeid(T).name()) {
			CT_MEMTRACK::f_set_name(in_pointer, typeid(T).name());
		}
		//WARN("GROA [%p]",this);
		init(new ST_GUARD_USAGE(true, (void*) in_pointer));
	}

	CT_GUARD(ST_GUARD_USAGE* in_ps_usage) {
		init(in_ps_usage);
		use();
	}

#if 0
	CT_GUARD & operator=(T * other) {
		*this = CT_GUARD<T>(other);
		return *this;
	}
#endif

	CT_GUARD(CT_GUARD<T> const & other) {
		//	D("[%p] copy constructor from %p", this, &other);
		_ps_usage = NULL;
		_pc_pointer = NULL;
		if (other._ps_usage) {
			*this = other;
		}
	}

	CT_GUARD<T> & operator=(const CT_GUARD<T> & other) {
		//D("[%p] copy start from %p", this, &other);
		unuse();

		if (other) {
			init(other._ps_usage);

			M_ASSERT(_ps_usage);

			use();
		}
		//D("[%p] copy end fron %p (link:%p)", this, &other, _pc_pointer);
		return *this;
	}

	CT_GUARD<T> & operator=(std::nullptr_t) {
		unuse();
		_ps_usage = NULL;
		return *this;
	}

	bool operator!=(std::nullptr_t) const {
		return _pc_pointer;
	}
	bool operator==(std::nullptr_t) const {
		return !_pc_pointer;
	}

	T & operator [](int in_i_index) const {
		M_ASSERT(_ps_usage);
		M_ASSERT(_pc_pointer);
		return _pc_pointer[in_i_index];
	}

	T * operator ->() const {
		//D("[%p] link to %p",this, _pc_pointer);
		if(!_pc_pointer) {
			_CRIT << "EMPTY GUARD FIXME FIXME FIXME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<< typeid(T).name();
			CT_DEBUG::f_print_backtrace_cxx();
		}
		M_ASSERT(_pc_pointer);
		return _pc_pointer;
	}

	T & operator *() const {
		M_ASSERT(_pc_pointer);
		return *_pc_pointer;
	}

	explicit operator T&() const {
		M_ASSERT(_pc_pointer);
		return *_pc_pointer;
	}

	T * get() const {
		return _pc_pointer;
	}

	explicit operator bool() const {
		return _pc_pointer != NULL;
	}

	~CT_GUARD() {
		//WARN("DELETE [%p]",this);
		unuse();
	}

	ST_GUARD_USAGE * get_usage() const {
		return _ps_usage;
	}

private:
	void delete_pointer(void) {
		delete _pc_pointer;
	}

	void init(ST_GUARD_USAGE * in_ps_usage) {
		_pc_pointer = static_cast<T*>(in_ps_usage->pc_pointer);
		_ps_usage = in_ps_usage;

		if (!_pc_pointer) {
			throw std::bad_alloc();
		}
		M_ASSERT(_pc_pointer);
		M_ASSERT(_ps_usage);
	}

	void unuse(void) {
		if (_ps_usage) {
			if (f_guard_dbg_in(_pc_pointer)) {
				_WARN<< "--UNUSE " << _pc_pointer << "usage: "
				<< _ps_usage->i_usage;
			}
			if (_ps_usage->unuse() == 1) {
				M_ASSERT(_pc_pointer == _ps_usage->pc_pointer);

				if (f_guard_dbg_in(_pc_pointer)) {
					_WARN << "DELETE " << _pc_pointer << " " << _ps_usage;
					f_guard_dbg_remove(_pc_pointer);
				}

				if (_ps_usage->b_free_usage) {
					delete_pointer();
					delete _ps_usage;
				} else {
					delete_pointer();
				}
			}
			_pc_pointer = NULL;
			_ps_usage = NULL;
		}
		M_ASSERT(_pc_pointer == NULL);
		M_ASSERT(_ps_usage == NULL);
	}

	void use(void) {
		if (f_guard_dbg_in(_pc_pointer)) {
			_WARN << "++USE " << _pc_pointer << "usage: " << _ps_usage->i_usage;
      if(_ps_usage->i_usage >= 5) {
   // CT_DEBUG::f_print_backtrace_cxx();
}
		}
		M_ASSERT(_ps_usage);
		M_ASSERT(_pc_pointer);
#if 1
		_ps_usage->use();
#else
		int i_use = _ps_usage->use();
		D("[%p] USE - %d %p %p %d",this, _ps_usage->b_free_usage, _pc_pointer, _ps_usage, i_use);
#endif
	}
};

template<typename T>
class CT_GUARD<T[]> {
	T * _pc_pointer;
	ST_GUARD_USAGE * _ps_usage;

public:
	CT_GUARD() {
		_pc_pointer = NULL;
		_ps_usage = NULL;
		//WARN("[%p]",this);
	}

	CT_GUARD(T * in_pointer) {
		if (typeid(*in_pointer).name() == typeid(T).name()) {
			CT_MEMTRACK::f_set_name(in_pointer, typeid(T).name());
		}
		//WARN("GROA [%p]",this);
		init(new ST_GUARD_USAGE(true, (void*) in_pointer));
	}

	CT_GUARD(ST_GUARD_USAGE* in_ps_usage) {
		init(in_ps_usage);
		use();
	}

#if 0
	CT_GUARD & operator=(T * other) {
		*this = CT_GUARD<T>(other);
		return *this;
	}
#endif

	CT_GUARD(CT_GUARD<T[]> const & other) {
		//	D("[%p] copy constructor from %p", this, &other);
		_ps_usage = NULL;
		_pc_pointer = NULL;
		if (other._ps_usage) {
			*this = other;
		}
	}

	CT_GUARD<T[]> & operator=(const CT_GUARD<T[]> & other) {
		//D("[%p] copy start from %p", this, &other);
		unuse();

		if (other) {
			init(other._ps_usage);

			M_ASSERT(_ps_usage);

			use();
		}
		//D("[%p] copy end fron %p (link:%p)", this, &other, _pc_pointer);
		return *this;
	}

	CT_GUARD<T[]> & operator=(std::nullptr_t) {
		unuse();
		_ps_usage = NULL;
		return *this;
	}

	bool operator!=(std::nullptr_t) const {
		return _pc_pointer;
	}
	bool operator==(std::nullptr_t) const {
		return !_pc_pointer;
	}

	T & operator [](int in_i_index) const {
		M_ASSERT(_ps_usage);
		M_ASSERT(_pc_pointer);
		return _pc_pointer[in_i_index];
	}

	T * operator ->() const {
		//D("[%p] link to %p",this, _pc_pointer);
		M_ASSERT(_pc_pointer);
		return _pc_pointer;
	}

	T & operator *() const {
		M_ASSERT(_pc_pointer);
		return *_pc_pointer;
	}

	explicit operator T&() const {
		M_ASSERT(_pc_pointer);
		return *_pc_pointer;
	}

	T * get() const {
		return _pc_pointer;
	}

	explicit operator bool() const {
		return _pc_pointer != NULL;
	}

	~CT_GUARD() {
		//WARN("DELETE [%p]",this);
		unuse();
	}

	ST_GUARD_USAGE * get_usage() const {
		return _ps_usage;
	}
private:
	void delete_pointer(void) {
		delete[] _pc_pointer;
	}

	void init(ST_GUARD_USAGE * in_ps_usage) {
		_pc_pointer = static_cast<T*>(in_ps_usage->pc_pointer);
		_ps_usage = in_ps_usage;
		if (!_pc_pointer) {
			throw std::bad_alloc();
		}
		M_ASSERT(_pc_pointer);
		M_ASSERT(_ps_usage);
	}

	void unuse(void) {
		if (_ps_usage) {
			if (f_guard_dbg_in(_pc_pointer)) {
				_WARN<< "--UNUSE " << _pc_pointer << "usage: "
				<< _ps_usage->i_usage;
			}
			if (_ps_usage->unuse() == 1) {
				M_ASSERT(_pc_pointer == _ps_usage->pc_pointer);

				if (f_guard_dbg_in(_pc_pointer)) {
					_WARN << "DELETE " << _pc_pointer;
					f_guard_dbg_remove(_pc_pointer);
				}

				if (_ps_usage->b_free_usage) {
					delete_pointer();
					delete _ps_usage;
				} else {
					delete_pointer();
				}
			}
			_pc_pointer = NULL;
			_ps_usage = NULL;
		}
		M_ASSERT(_pc_pointer == NULL);
		M_ASSERT(_ps_usage == NULL);
	}

	void use(void) {
		if (f_guard_dbg_in(_pc_pointer)) {
			_WARN << "++USE " << _pc_pointer << "usage: " << _ps_usage->i_usage;
		}
		M_ASSERT(_ps_usage);
		M_ASSERT(_pc_pointer);
#if 1
		_ps_usage->use();
#else
		int i_use = _ps_usage->use();
		D("[%p] USE - %d %p %p %d",this, _ps_usage->b_free_usage, _pc_pointer, _ps_usage, i_use);
#endif
	}
};

template<class T1, class T2> inline bool operator==(CT_GUARD<T1> const & n1,
		CT_GUARD<T2> const & n2) {
	return n1.get() == n2.get();
}

template<class T1, class T2> inline bool operator!=(CT_GUARD<T1> const & n1,
		CT_GUARD<T2> const & n2) {
	return n1.get() != n2.get();
}

template<class T1, class T2> CT_GUARD<T1> dynamic_pointer_cast(
		CT_GUARD<T2> const & in) {
	M_ASSERT(sizeof(CT_GUARD<T1> ) == sizeof(CT_GUARD<T2> ));
	M_ASSERT(dynamic_cast<T1*>(in.get()));
	CT_GUARD<T1> c_tmp(in.get_usage());
	//CT_GUARD<T1> * pc_tmp = (CT_GUARD<T1> *) &in;
	return c_tmp;
}

#endif /* GUARD_HH_ */
