/****************************************************************************
 *
 * Copyright (C) 2020 Fabian Hauser
 *
 * Author: Fabian Hauser <fabian.hauser@fh-linz.at>
 * University of Applied Sciences Upper Austria - Linz - Austra
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************************************/

#include "Fitter.h"

#include "Common.h"

#include <iostream>

using namespace LookUpSTORM;

Fitter::Fitter()
    : m_lookup(nullptr)
	, m_tableAllocated(false)
    , m_countLat(0)
	, m_countAx(0)
	, m_countIndex(0)
	, m_winSize(0)
	, m_stride(0)
	, m_dLat(0.0)
	, m_dAx(0.0)
	, m_minLat(0.0)
	, m_maxLat(0.0)
	, m_minAx(0.0)
	, m_maxAx(0)
	, m_x0(5, Uninitialized)
	, m_x1(5, Uninitialized)
	, m_JTJ(5, 5, Uninitialized)
	, m_epsilon(1E-2)
	, m_maxIter(5)
{
}

Fitter::~Fitter()
{
	release();
}

void Fitter::release()
{
	if (m_tableAllocated) {
		delete[] m_lookup;
		m_lookup = nullptr;
		m_tableAllocated = false;
		m_x0 = {};
		m_x1 = {};
		m_JTJ = {};
		m_countIndex = 0;
		m_winSize = 0;
	}
}

bool Fitter::isReady() const
{
	return (m_lookup != nullptr) && (m_countIndex > 0) && (m_winSize > 0);
}

bool Fitter::fitSingle(const ImageU16& roi, Molecule& mol)
{
	const size_t startLat = m_winSize / 2;
	m_x0[0] = mol.background;
	m_x0[1] = mol.peak; // std::max(50.0, mol.peak - mol.background);
	m_x0[2] = startLat;
	m_x0[3] = startLat;
	m_x0[4] = 0.0;

	const size_t N = m_winSize * m_winSize;

	const size_t maxIter = m_maxIter.load();
	const double eps = m_epsilon.load();

	size_t iter = 0;
	for (; iter < maxIter; ++iter) {
		const double* lookup = get(m_x0[2], m_x0[3], m_x0[4]);
		if (lookup == nullptr)
			break;
		double bg = m_x0[0];
		double peak = m_x0[1];

		m_x1.setZero();

		double ssq0 = 0.0;
		for (int i = 0; i < N; i++) {
			const double e = *lookup++;
			const double dx = peak * (*lookup++);
			const double dy = peak * (*lookup++);
			const double dz = peak * (*lookup++);
			const double rval = bg + peak * e - roi[i];
			ssq0 += rval * rval;

			// Jacobian 
			m_J(i, 0) = 1.0;
			m_J(i, 1) = e;
			m_J(i, 2) = dx;
			m_J(i, 3) = dy;
			m_J(i, 4) = dz;

			// JTr
			m_x1[0] += rval;
			m_x1[1] += rval * e;
			m_x1[2] += rval * dx;
			m_x1[3] += rval * dy;
			m_x1[4] += rval * dz;
		}

		//if (BLAS::dgemm(BLAS::CblasTrans, BLAS::CblasNoTrans, 1.0, m_J, m_J, 0.0, m_JTJ) != LIN_SUCCESS) break;
		if (BLAS::dsyrk(BLAS::CblasUpper, BLAS::CblasTrans, 1.0, m_J, 0.0, m_JTJ) != LIN_SUCCESS) break;

#ifdef NO_LAPACKE
		if (BLAS::dtrsv(BLAS::CblasUpper, BLAS::CblasTrans, BLAS::CblasNonUnit, m_JTJ, m_x1) != LIN_SUCCESS)
			break;
#else
		int ipiv[5];
		if (LAPACKE::dsysv(LAPACKE::U, m_JTJ, ipiv, m_x1) != 0)
			break;
#endif // NO_LAPACKE

		/*double delta = 0.0;
		for (int i = 0; i < 5; ++i)
			delta += std::abs(m_x1[i]);

		if (delta < 1E-3) {
			std::cout << "Too small change!" << std::endl;
			break;
		}*/

		double xNew = m_x0[2] - m_x1[2];
		double yNew = m_x0[3] - m_x1[3];
		double zNew = m_x0[4] - m_x1[4];

		lookup = get(xNew, yNew, zNew);
		if (lookup == nullptr)
			break;

		bg -= m_x1[0];
		peak -= m_x1[1];

		double ssq1 = 0.0;
		for (int i = 0; i < N; i++, lookup += 4) {
			const double rval = bg + peak * (*lookup) - roi[i];
			ssq1 += rval * rval;
		}

		if ((ssq1 < ssq0) && ((ssq0 - ssq1) > eps)) {
			m_x0 -= m_x1; 
		} else {
			break;
		}
	}

	if ((iter == 0) || (m_x0[0] < 0.0) || (m_x0[1] < 0.0) || (m_x0[0] > 13000.0) || (m_x0[1] > 13000.0) || 
		cmp(m_x0[2], startLat) || cmp(m_x0[3], startLat) || (m_x0[4] == 0.0))
		return false;

	m_x0[2] -= fmod(m_x0[2], m_dLat);
	m_x0[3] -= fmod(m_x0[3], m_dLat);
	m_x0[4] -= fmod(m_x0[4], m_dAx);

	if (!isValid(m_x0[2], m_x0[3], m_x0[4]))
		return false;

	mol.background = m_x0[0];
	mol.peak = m_x0[1];
	mol.x = m_x0[2];
	mol.y = m_x0[3];
	mol.z = m_x0[4];

	return true;
}

bool Fitter::setLookUpTable(const double* data, size_t dataSize, bool allocated, int windowSize, double dLat, double dAx, double rangeLat, double rangeAx)
{
	const double borderLat = std::floor((windowSize - rangeLat) / 2);
	if (borderLat < 1.0) {
		std::cerr << "LookUpSTORM_CPPDLL: setLookUpTable: Lateral border is less than one! (Lateral range: " << rangeLat << ")" << std::endl;
		return false;
	}

	m_lookup = data;
	m_tableAllocated = allocated;
	m_winSize = windowSize;

	m_dLat = dLat;
	m_dAx = dAx;

	m_minLat = borderLat;
	m_maxLat = windowSize - borderLat;
	m_minAx = -rangeAx * 0.5;
	m_maxAx = rangeAx * 0.5;
	m_countLat = static_cast<size_t>(std::floor((((m_maxLat - m_minLat) / dLat) + 1)));
	m_countAx = static_cast<size_t>(std::floor(((rangeAx / dAx) + 1)));

	m_countIndex = m_countLat * m_countLat * m_countAx;

	m_stride = m_winSize * m_winSize * 4;

	const size_t expected = m_countIndex * m_stride;
	if (dataSize != expected) {
		std::cerr << "LookUpSTORM_CPPDLL: setLookUpTable: Template size does not correspond to the supplied array!" 
				  << "(expected: " << expected << ", got: " << dataSize << ")" << std::endl;
		return false;
	}

	const size_t N = size_t(windowSize) * size_t(windowSize);
	m_J = Matrix(N, 5, Uninitialized);

	return true;
}

const double* Fitter::lookUpTablePtr() const
{
	return m_lookup;
}

size_t Fitter::lookupIndex(double x, double y, double z) const
{
	const size_t xi = static_cast<size_t>(std::round((x - m_minLat) / m_dLat));
	const size_t yi = static_cast<size_t>(std::round((y - m_minLat) / m_dLat));
	const size_t zi = static_cast<size_t>(std::round((z - m_minAx) / m_dAx));
	const size_t index = zi + yi * m_countAx + xi * m_countAx * m_countLat;
	return index;
}

size_t Fitter::windowSize() const
{
	return m_winSize;
}

double Fitter::deltaLat() const
{
	return m_dLat;
}

double Fitter::rangeLat() const
{
	return m_maxLat - m_minLat - 1.0;
}

double Fitter::minLat() const
{
	return m_minLat;
}

double Fitter::maxLat() const
{
	return m_maxLat;
}

double Fitter::deltaAx() const
{
	return m_dAx;
}

double Fitter::rangeAx() const
{
	return m_maxAx - m_minAx;
}

double Fitter::minAx() const
{
	return m_minAx;
}

double Fitter::maxAx() const
{
	return m_maxAx;
}

void Fitter::setEpsilon(double eps)
{
	m_epsilon.store(eps);
}

double Fitter::epsilon() const
{
	return m_epsilon.load();
}

void Fitter::setMaxIter(size_t maxIter)
{
	m_maxIter.store(maxIter);
}

size_t Fitter::maxIter() const
{
	return m_maxIter.load();
}

constexpr bool Fitter::isValid(double x, double y, double z) const
{
	return ((x >= m_minLat) && (x <= m_maxLat) && (y >= m_minLat) && (y <= m_maxLat) && (z >= m_minAx) && (z <= m_maxAx));
}

const double* Fitter::get(double x, double y, double z) const
{
	if (!isValid(x, y, z))
		return nullptr;
	const size_t index = lookupIndex(x, y, z);
	if (index > m_countIndex) {
		std::cout << "Index error: " << x << ", " << y << ", " << z << std::endl;
		return nullptr;
	}
	return &m_lookup[index * m_stride];
}