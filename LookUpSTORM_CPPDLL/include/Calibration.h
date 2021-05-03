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

#ifndef CALIBRATION_H
#define CALIBRATION_H

#include "Common.h"

#include <unordered_map>

namespace LookUpSTORM
{
using Parameters = std::unordered_map<std::string, double>;

class CalibrationPrivate;

class DLL_DEF_LUT Calibration
{
public:
	// constructor
	Calibration();
	// destructor
	~Calibration();

	bool load(const std::string& fileName);
	bool parseJAML(const std::string& data);

	bool generateSpline();

	// return sigma or the derivs in pixesl
	std::pair<double, double> value(double z) const;
	std::pair<double, double> dvalue(double z) const;
	std::tuple<double, double, double, double> valDer(double z) const;

	// return number of loaded knots
	size_t knots() const;
	// return knot at position i in the format in {x (pixels), y (pixels), z (nm)}
	std::tuple<double, double, double> knot(size_t i) const;

	// rotation of psf
	double theta() const;
	// pixels size in µm
	double pixelSize() const;
	// focal plane on z-axis where the curves crosses in nm
	double focalPlane() const;

	double minZ() const;
	double maxZ() const;

	const Parameters& parameters() const;

private:
	CalibrationPrivate* const d;

};

} // namespace LookUpSTORM

#endif // !CALIBRATION_H
