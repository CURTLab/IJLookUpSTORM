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

#ifndef COLORMAP_H
#define COLORMAP_H

#include "Common.h"

#include <tuple>
#include <vector>

namespace LookUpSTORM
{ 

class DLL_DEF_LUT ColorMap
{
public:
	ColorMap();
	ColorMap(double min, double max);
	virtual ~ColorMap();

	uint32_t rgb(double value, double scale = 1.0) const;

	void setRange(double min, double max);

	void generate(double min, double max, double step, double scale = 1.0);

	uint32_t cachedRgb(double value) const;

	double min() const;
	double max() const;
	double step() const;

	bool isCached() const;

private:
	static std::tuple<double, double, double> rgbFromWaveLength(double wavelength);
	double m_min;
	double m_max;
	double m_step;
	static constexpr double m_f1 = 1.0 / 400;
	static constexpr double m_f2 = 1.0 / 780;
	size_t m_entries;
	uint32_t *m_lut;

};

} // namespace LookUpSTORM

#endif // !COLORMAP_H
