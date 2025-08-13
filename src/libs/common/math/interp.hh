/***********************************************************************
 ** interp.hh
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
#ifndef COMMON_INTERP_HH_
#define COMMON_INTERP_HH_

/**
 * @file interp.hh
 * Interpolation methods.
 *
 * @author SEAGNAL (johann.baudy@seagnal.fr)
 * @date 2025
 *
 * @version 1.0 Original version
 */

/***********************************************************************
 * Includes
 ***********************************************************************/

/***********************************************************************
 * Defines
 ***********************************************************************/

/* Math stuff */
#include <vector>

template <typename Tx>
inline int f_findLowerBoundIndex(Tx value, const std::vector<Tx>& x)
{
    for (size_t i = 0; i < x.size() - 1; i++) {
        if (value >= x[i] && value < x[i + 1]) {
            return i;
        }
    }

    if (value >= x.back()) return x.size() - 2; // Dernière valeur
    if (value <= x.front()) return 0; // Première valeur

    return -1; // Erreur
}

template <typename Tx, typename Ty>
int f_interp1(std::vector<Tx> &x, std::vector<Ty> &y, std::vector<Tx> &x_new, std::vector<Ty> &y_new)
{
    y_new.reserve(x_new.size());

    std::vector<Tx> dx;
    std::vector<Ty> dy;
    std::vector<Ty> slope, intercept;

    dx.reserve(x.size());
    dy.reserve(x.size());
    slope.reserve(x.size());
    intercept.reserve(x.size());

    for (size_t i = 0; i < x.size(); i++) {
        if (i < x.size() - 1)
        {
            dx.push_back(x[i + 1] - x[i]);
            dy.push_back(y[i + 1] - y[i]);
            slope.push_back(dy[i] / dx[i]);
            intercept.push_back(y[i] - (slope[i] * x[i]));
        }
        else
        {
            dx.push_back(dx[i - 1]);
            dy.push_back(dy[i - 1]);
            slope.push_back(slope[i - 1]);
            intercept.push_back(intercept[i - 1]);
        }
    }

    for (size_t i = 0; i < x_new.size(); i++)
    {
        int idx = f_findLowerBoundIndex<Tx>(x_new[i], x);
        y_new.push_back(slope[idx] * x_new[i] + intercept[idx]);
    }

    return EC_SUCCESS;
}

#endif
