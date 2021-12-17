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
#include "LocalMaximumSearch.h"
#include "LinearMath.h"

#include <iostream>
#include <atomic>

#ifdef USE_AVX_LUT
#include <immintrin.h>
#endif

namespace LookUpSTORM
{

class FitterPrivate
{
public:
	inline FitterPrivate()
		: lookup(nullptr)
		, tableAllocated(false)
		, countLat(0)
		, countAx(0)
		, countIndex(0)
		, winSize(0)
		, stride(0)
		, dLat(0.0)
		, dAx(0.0)
		, minLat(0.0)
		, maxLat(0.0)
		, minAx(0.0)
		, maxAx(0)
		, x0(5, Uninitialized)
		, x1(5, Uninitialized)
		, JTJ(5, 5, Uninitialized)
		, epsilon(1E-2)
		, maxIter(5)
	{}
	inline ~FitterPrivate() 
	{
		if (tableAllocated && (lookup != nullptr))
			delete[] lookup;
	}

	size_t lookupIndex(double x, double y, double z) const;
	const double* get(double x, double y, double z) const;

	inline constexpr bool isValid(double x, double y, double z) const
	{
		return ((x >= minLat) && (x <= maxLat) && (y >= minLat) && (y <= maxLat) && (z >= minAx) && (z <= maxAx));
	}

	const double* lookup;
	bool tableAllocated;
	size_t countLat;
	size_t countAx;
	size_t countIndex;
	size_t winSize;
	size_t stride;
	double dLat;
	double dAx;
	double minLat;
	double maxLat;
	double minAx;
	double maxAx;

	// Gauss-Newton algorithm variables
	Vector x0;
	Vector x1;
	Matrix J;
	Matrix JTJ;
	std::atomic<double> epsilon;
	std::atomic<size_t> maxIter;

};

} // namespace LookUpSTORM

using namespace LookUpSTORM;

size_t FitterPrivate::lookupIndex(double x, double y, double z) const
{
	const size_t xi = static_cast<size_t>(std::round((x - minLat) / dLat));
	const size_t yi = static_cast<size_t>(std::round((y - minLat) / dLat));
	const size_t zi = static_cast<size_t>(std::round((z - minAx) / dAx));
	const size_t index = zi + yi * countAx + xi * countAx * countLat;
	return index;
}

const double* FitterPrivate::get(double x, double y, double z) const
{
	if (!isValid(x, y, z))
		return nullptr;
	const size_t index = lookupIndex(x, y, z);
	if (index > countIndex) {
		std::cout << "Index error: " << x << ", " << y << ", " << z << std::endl;
		return nullptr;
	}
	return &lookup[index * stride];
}

Fitter::Fitter()
	: d(new FitterPrivate)
{
}

Fitter::~Fitter()
{
	delete d;
}

void Fitter::release()
{
	if (d->tableAllocated && (d->lookup != nullptr)) {
		delete[] d->lookup;
		d->lookup = nullptr;
		d->tableAllocated = false;
	}
	d->x0 = Vector(5, Uninitialized);
	d->x0 = Vector(5, Uninitialized);
	d->JTJ = Matrix(5, 5, Uninitialized);
	d->J = {};
	d->countIndex = 0;
	d->winSize = 0;
}

bool Fitter::isReady() const
{
	return (d->lookup != nullptr) && (d->countIndex > 1);
}

bool Fitter::fitSingle(const ImageU16& roi, Molecule& mol)
{
	const size_t startLat = d->winSize / 2;
	d->x0[0] = mol.background;
	d->x0[1] = mol.peak; // std::max(50.0, mol.peak - mol.background);
	d->x0[2] = startLat;
	d->x0[3] = startLat;
	d->x0[4] = 0.0;

	const size_t N = d->winSize * d->winSize;

	const size_t maxIter = d->maxIter.load();
	const double eps = d->epsilon.load();

	size_t iter = 0;
	for (; iter < maxIter; ++iter) {
		const double* lookup = d->get(d->x0[2], d->x0[3], d->x0[4]);
		if (lookup == nullptr)
			break;
		double bg = d->x0[0];
		double peak = d->x0[1];

		d->x1.setZero();

#ifdef USE_AVX_LUT
		// intrinic set is reversed! (d3, d2, d1, d0)
		__m256d vpeak = _mm256_set_pd(peak, peak, peak, 1.0);
#endif

		double ssq0 = 0.0;
		for (int i = 0; i < N; i++) {
#ifdef USE_AVX_LUT
			// load 4 double (e, dx, dy, dz) from lookup table
			__m256d vpsf = _mm256_load_pd(lookup);
			lookup += 4;

			// residual
			const double rval = bg + peak * vpsf.m256d_f64[0] - roi[i];
			ssq0 += rval * rval;

			// multiply delta vector by peak
			vpsf = _mm256_mul_pd(vpeak, vpsf);

			// Jacobian 
			_mm256_store_pd(&d->J(i, 1), vpsf);

			// JTr
			__m256d vrval = _mm256_set1_pd(rval);
			d->x1[0] += rval;
			__m256d vx0 = _mm256_load_pd(&d->x1[1]);
			vx0 = _mm256_fmadd_pd(vrval, vpsf, vx0);
			_mm256_store_pd(&d->x1[1], vx0);
#else
			const double e = *lookup++;
			const double dx = peak * (*lookup++);
			const double dy = peak * (*lookup++);
			const double dz = peak * (*lookup++);
			const double h = bg + peak * e;

			// Jacobian 
			d->J(i, 0) = 1.0;
			d->J(i, 1) = e;
			d->J(i, 2) = dx;
			d->J(i, 3) = dy;
			d->J(i, 4) = dz;

			// residual or cost
			const double rval = h - roi[i];
			ssq0 += rval * rval;

			// JTr
			d->x1[0] += rval;
			d->x1[1] += rval * e;
			d->x1[2] += rval * dx;
			d->x1[3] += rval * dy;
			d->x1[4] += rval * dz;
#endif
		}

		//if (BLAS::dgemm(BLAS::CblasTrans, BLAS::CblasNoTrans, 1.0, d->J, d->J, 0.0, d->JTJ) != LIN_SUCCESS) break;
		if (BLAS::dsyrk(BLAS::CblasUpper, BLAS::CblasTrans, 1.0, d->J, 0.0, d->JTJ) != LIN_SUCCESS) break;

#ifdef NO_LAPACKE_LUT
		if (BLAS::dtrsv(BLAS::CblasUpper, BLAS::CblasTrans, BLAS::CblasNonUnit, d->JTJ, d->x1) != LIN_SUCCESS)
			break;
#else
		int ipiv[5];
		if (LAPACKE::dsysv(LAPACKE::U, d->JTJ, ipiv, d->x1) != 0)
			break;
#endif // NO_LAPACKE_LUT

		/*double delta = 0.0;
		for (int i = 0; i < 5; ++i)
			delta += std::abs(d->x1[i]);

		if (delta < 1E-3) {
			std::cout << "Too small change!" << std::endl;
			break;
		}*/

		double xNew = d->x0[2] - d->x1[2];
		double yNew = d->x0[3] - d->x1[3];
		double zNew = d->x0[4] - d->x1[4];

		lookup = d->get(xNew, yNew, zNew);
		if (lookup == nullptr)
			break;

		bg -= d->x1[0];
		peak -= d->x1[1];

		double ssq1 = 0.0;
		for (int i = 0; i < N; i++, lookup += 4) {
			const double rval = bg + peak * (*lookup) - roi[i];
			ssq1 += rval * rval;
		}

		if ((ssq1 < ssq0) && ((ssq0 - ssq1) > eps)) {
			d->x0 -= d->x1; 
		} else {
			break;
		}
	}

	if ((iter == 0) || (d->x0[0] < 0.0) || (d->x0[1] < 0.0) || (d->x0[0] > 13000.0) || (d->x0[1] > 65536.0) ||
		cmp(d->x0[2], startLat) || cmp(d->x0[3], startLat) || (d->x0[4] == 0.0)) {
		//std::cout << "Val error (iter:" << iter << ",bg:" << d->x0[0] << ",I:" << d->x0[1] << std::endl;
		return false;
	}

	d->x0[2] -= fmod(d->x0[2], d->dLat);
	d->x0[3] -= fmod(d->x0[3], d->dLat);
	d->x0[4] -= fmod(d->x0[4], d->dAx);

	if (!isValid(d->x0[2], d->x0[3], d->x0[4])) {
		//std::cout << "Invalid position error" << std::endl;
		return false;
	}

	mol.background = d->x0[0];
	mol.peak = d->x0[1];
	mol.x = d->x0[2];
	mol.y = d->x0[3];
	mol.z = d->x0[4];

	return true;
}

bool Fitter::setLookUpTable(const double* data, size_t dataSize, bool allocated, int windowSize, double dLat, double dAx, double rangeLat, double rangeAx)
{
	const double borderLat = std::floor((windowSize - rangeLat) / 2);
	if (borderLat < 1.0) {
		std::cerr << "LookUpSTORd->CPPDLL: setLookUpTable: Lateral border is less than one! (Lateral range: " << rangeLat << ")" << std::endl;
		return false;
	}

	if ((d->lookup != nullptr) && d->tableAllocated)
		release();

	d->lookup = data;
	d->tableAllocated = allocated;
	d->winSize = windowSize;

	d->dLat = dLat;
	d->dAx = dAx;

	d->minLat = borderLat;
	d->maxLat = windowSize - borderLat;
	d->minAx = -rangeAx * 0.5;
	d->maxAx = rangeAx * 0.5;
	d->countLat = static_cast<size_t>(std::floor((((d->maxLat - d->minLat) / dLat) + 1)));
	d->countAx = static_cast<size_t>(std::floor(((rangeAx / dAx) + 1)));

	d->countIndex = d->countLat * d->countLat * d->countAx;

	d->stride = d->winSize * d->winSize * 4;

	const size_t expected = d->countIndex * d->stride;
	if (dataSize != expected) {
		std::cerr << "LookUpSTORd->CPPDLL: setLookUpTable: Template size does not correspond to the supplied array!" 
				  << "(expected: " << expected << ", got: " << dataSize << ")" << std::endl;
		return false;
	}

	const size_t N = size_t(windowSize) * size_t(windowSize);
	d->J = Matrix(N, 5, 1.0);

	return true;
}

bool LookUpSTORM::Fitter::setLookUpTable(const LUT& lut)
{
	if (!lut.isValid())
		return false;
	return setLookUpTable(lut.ptr(), lut.dataSize(), true, lut.windowSize(), lut.dLat(), lut.dAx(), lut.rangeLat(), lut.rangeAx());
}

const double* Fitter::lookUpTablePtr() const
{
	return d->lookup;
}

const double* LookUpSTORM::Fitter::templatePtr(double x, double y, double z) const
{
	return d->get(x, y, z);
}

size_t Fitter::windowSize() const
{
	return d->winSize;
}

double Fitter::deltaLat() const
{
	return d->dLat;
}

double Fitter::rangeLat() const
{
	return d->maxLat - d->minLat - 1.0;
}

double Fitter::minLat() const
{
	return d->minLat;
}

double Fitter::maxLat() const
{
	return d->maxLat;
}

double Fitter::deltaAx() const
{
	return d->dAx;
}

double Fitter::rangeAx() const
{
	return d->maxAx - d->minAx;
}

double Fitter::minAx() const
{
	return d->minAx;
}

double Fitter::maxAx() const
{
	return d->maxAx;
}

void Fitter::setEpsilon(double eps)
{
	d->epsilon.store(eps);
}

double Fitter::epsilon() const
{
	return d->epsilon.load();
}

void Fitter::setMaxIter(size_t maxIter)
{
	d->maxIter.store(maxIter);
}

size_t Fitter::maxIter() const
{
	return d->maxIter.load();
}

constexpr bool Fitter::isValid(double x, double y, double z) const
{
	return d->isValid(x, y, z);
}
