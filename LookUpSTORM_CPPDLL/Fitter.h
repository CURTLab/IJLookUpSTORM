/****************************************************************************
 *
 * MIT License
 *
 * Copyright (C) 2021 Fabian Hauser
 *
 * Author: Fabian Hauser <fabian.hauser@fh-linz.at>
 * University of Applied Sciences Upper Austria - Linz - Austria
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 ****************************************************************************/

#ifndef FITTER_H
#define FITTER_H

#include "Image.h"

namespace LookUpSTORM
{

class Molecule
{
public:
	inline Molecule() : data{0.0} {}

	union {
		struct {
			double background;
			double peak;
			double x;
			double y;
			double z;
			double frame;
			double time_us;
		};
		double data[7];
	};
};

class FitterPrivate;

class DLL_DEF_LUT Fitter final
{
public:
	Fitter();
	~Fitter();

	void release();

	bool isReady() const;

	bool fitSingle(const ImageU16& roi, Molecule& mol);

	bool setLookUpTable(const double* data, size_t dataSize, bool allocated, int windowSize, double dLat, double dAx, double rangeLat, double rangeAx);

	const double* lookUpTablePtr() const;

	constexpr bool isValid(double x, double y, double z) const;

	size_t windowSize() const;

	double deltaLat() const;
	double rangeLat() const;
	double minLat() const;
	double maxLat() const;
	double deltaAx() const;
	double rangeAx() const;
	double minAx() const;
	double maxAx() const;

	// thread-safe
	void setEpsilon(double eps);
	// thread-safe
	double epsilon() const;

	// thread-safe
	void setMaxIter(size_t maxIter);
	// thread-safe
	size_t maxIter() const;

private:
	FitterPrivate * const d;
};

} // namespace LookUpSTORM

#endif // !FITTER_H
