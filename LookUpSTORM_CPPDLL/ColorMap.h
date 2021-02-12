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

#include <tuple>
#include <vector>

namespace LookUpSTORM
{ 

class ColorMap
{
public:
	ColorMap();

	uint32_t rgb(double value, double scale = 1.f) const;

	void setRange(double min, double max);

private:
	std::tuple<double, double, double> rgbFromWaveLength(double wavelength) const;
	double m_min;
	double m_max;
	static constexpr double m_f1 = 1.0 / 400;
	static constexpr double m_f2 = 1.0 / 780;
	double m_step;

};

} // namespace LookUpSTORM

#endif // !COLORMAP_H
