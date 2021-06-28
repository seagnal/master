/***********************************************************************
 ** quaternion.hh
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
#ifndef COMMON_PEAK_HH_
#define COMMON_PEAK_HH_

/**
 * @file quaternion.hh
 * Quaternion class definition.
 *
 * @author SEAGNAL (johann.baudy@seagnal.fr)
 * @date 2014
 *
 * @version 1.0 Original version
 */

/***********************************************************************
 * Includes
 ***********************************************************************/
#include <list>
#include <c/common.h>

//Eigen ERROR
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#include <Eigen/Core>
/***********************************************************************
 * Defines
 ***********************************************************************/
 struct ST_PEAK {
 	uint32_t i_index;
 	float f_value;
 	int32_t i_left_margin;
 	int32_t i_right_margin;
 	float f_left_acc;
 	int i_left_acc;
 	float f_right_acc;
 	int i_right_acc;
  float f_noise;
 };

 struct ST_PEAK_INTERPOLATED {
   float f_index;
   float f_value;
   float f_noise;
 };

int f_peak_find(std::list<ST_PEAK> & out_l_peak,
		Eigen::VectorXf const & in_c_vect,
		float const in_f_snr_from_max_time,
		float const in_f_level_limit,
		int const in_width_analysis,
		int const in_i_width_modulation);

int f_peak_find(std::list<ST_PEAK> & out_l_peak,
		float const * const in_pf_data,
		size_t const in_sz_data,
		float const in_f_snr_from_max_time,
		float const in_f_level_limit,
		int const in_width_analysis,
		int const in_i_width_modulation);


bool f_peak_level_sorter(ST_PEAK const& lhs, ST_PEAK const& rhs);

int f_peak_interpolate_max(ST_PEAK_INTERPOLATED & out_s_peak_interpolated, ST_PEAK const & in_s_peak, Eigen::VectorXf const & in_c_vect);

void f_peak_parabolic_interpolation(
  float & out_f_ymax,
  float & out_f_dx,
  float in_f_yleft,
  float in_f_ymid,
  float in_f_yright
);

#endif
