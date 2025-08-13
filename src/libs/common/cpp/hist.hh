/***********************************************************************
 ** hist.hh
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

#ifndef HIST_HH_
#define HIST_HH_

/**
 * @file hist.hh
 * History management template class.
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
#include <map>
#include <utility>

template<typename Tt, typename Td>
class CT_HIST {
public:
	typedef typename std::map<Tt, Td>::iterator iterator;
	typedef typename std::map<Tt, Td>::reverse_iterator reverse_iterator;
	typedef typename std::pair<iterator, iterator> result_range;
	typedef typename std::pair<Td, Td> result_nearest;
private:
	std::map<Tt, Td> _m_values;
	iterator _c_last_inserted;
public:
	CT_HIST() {
		_c_last_inserted = _m_values.begin();
	}
	~CT_HIST() {

	}

	void f_register(Tt const & in_time, Td const & in_data) {
		//M_ASSERT(in_time);
		// Optimize insertion
    //_DBG << "insertion " <<_V(in_time) << _V(in_data.get()) ;
    auto c_tmp = std::pair<Tt, Td>(in_time,in_data);
    _c_last_inserted = _m_values.insert(_c_last_inserted, c_tmp);
    //_DBG << "AFTER";
	}

	iterator f_end() {
		return _m_values.end();
	}
	iterator f_begin() {
		return _m_values.begin();
	}
	reverse_iterator f_rbegin() {
		return _m_values.rbegin();
	}
	Td& f_last() {
		return _m_values.rbegin()->second;
	}
	size_t f_size() {
		return _m_values.size();
	}
	size_t f_mem_size() {
		return _m_values.size()*sizeof(Td);
	}
	Tt f_time_max() {
		return _m_values.rbegin() != _m_values.rend() ? _m_values.rbegin()->first : 0;
	}
	Tt f_time_min() {
			return _m_values.begin() != _m_values.end() ? _m_values.begin()->first : 0;
		}

	int f_get_range(result_range & out_res, Tt const & in_time_start,
			Tt const & in_time_end) {
		int ec;
		result_range c_res;

		iterator c_lower = _m_values.lower_bound(in_time_start);
		//_DBG << _V(in_time_start) << _V(in_time_end);
		if (c_lower == _m_values.end()) {
			//_DBG << "No data for such date";
			ec = EC_FAILURE;
			goto out_err;
		}

    if(c_lower->first > in_time_start) {
      c_lower--;
      if(c_lower == _m_values.end()) {
        ec = EC_FAILURE;
        goto out_err;
      }
    }

		out_res.first = c_lower;
		out_res.second = _m_values.upper_bound(in_time_end);

		if ((out_res.second == _m_values.end())
				&& (in_time_start != in_time_end)) {
			ec = EC_BYPASS;
		} else {
			ec = EC_SUCCESS;
		}
#if 0
		if ((in_time_start != in_time_end) || (c_lower.first != in_time_start )) {
			if (c_upper == _m_values.end()) {
				_CRIT << "No data for such time";
				ec = EC_BYPASS;
			}
			out_res.second = c_upper;
		}
#endif

		out_err: return ec;
	}

	int f_get_segment(result_range & out_res, Tt const & in_time_start) {
		return f_get_range(out_res, in_time_start, in_time_start);
	}
#if 0
	int f_get_nearest(Td *& out_res, Tt const & in_time) {
		result_range c_res;
		int ec;
		ec = f_get_range(c_res, in_time, in_time);
		//_DBG << _V(in_time) <<_V(ec);
		if (ec != EC_SUCCESS) {
			return ec;
		}


		if (c_res.second == _m_values.end()) {
			out_res = &c_res.first->second;
		} else {
			Tt i_time_first = c_res.first->first;
			Tt i_time_second = c_res.second->first;
			//_DBG << _V(i_time_first) << _V(in_time) << _V(i_time_second) << _V(_m_values.begin()->first);
			//M_ASSERT(i_time_first <= in_time);
			//M_ASSERT(i_time_second >= in_time);
			Tt i_diff_first = M_ABS(in_time - i_time_first);
			Tt i_diff_second = M_ABS(i_time_second - in_time);
			if (i_diff_second > i_diff_first) {
				out_res = &c_res.first->second;
			} else {
				out_res = &c_res.second->second;
			}
		}
		return EC_SUCCESS;
	}
	int f_get_nearest(Td & out_res, Tt const & in_time) {
		int ec;
		Td * pc_res = NULL;
		ec = f_get_nearest(pc_res, in_time);
		if (ec == EC_SUCCESS) {
			out_res = *pc_res;
		}
		return ec;
	}
#endif
	int f_get_nearest(Td & out_res, Tt const & in_time, bool in_b_discard_first = false) {
		int ec;
		result_range c_res;

		ec = f_get_range(c_res, in_time, in_time);
		if (ec != EC_SUCCESS) {
			goto out_err;
		}
		/* At least lower bound is found */
		if(c_res.first->first == in_time) {
			out_res = c_res.first->second;
		} else {
			if(in_b_discard_first) {
				/* Discard if first element match range */
				if(c_res.first == f_begin()) {
					ec = EC_FAILURE;
					goto out_err;
				}
			}

			out_res = c_res.first->second;
		}

		ec = EC_SUCCESS;
		out_err:
		return ec;
	}


	Td & f_get_nearest(Tt const & in_time) {
		int ec;
		Td * pc_res = NULL;
		ec = f_get_nearest(pc_res, in_time);
		if (ec == EC_SUCCESS) {
			M_ASSERT(pc_res);
			return *pc_res;
		} else {
			throw std::runtime_error("No data found");
		}
	}

	void f_remove(iterator in_it) {
		_m_values.erase(in_it);
		if(in_it == _c_last_inserted) {
			_c_last_inserted = _m_values.begin();
		}
	}

	void f_remove_until(Tt in_time) {
		iterator c_it = _m_values.begin();
		while (c_it != _m_values.end()) {
			if (c_it->first <= in_time) {
				//_DBG << _V(c_it->first);
				_m_values.erase(c_it);
				c_it = _m_values.begin();
			} else {
				return;
			}
		}
	}

	void f_remove_begin() {
		f_remove(_m_values.begin());
	}

	void f_clear() {
		_m_values.clear();
		_c_last_inserted = _m_values.begin();
	}
};


#endif
