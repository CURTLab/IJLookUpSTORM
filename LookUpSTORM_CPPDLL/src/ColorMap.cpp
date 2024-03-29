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

#include "ColorMap.h"

#include <assert.h>

#include "Common.h"

using namespace LookUpSTORM;

inline constexpr uint8_t red(uint32_t color) { return (color >> 16) & 0xff; }
inline constexpr uint8_t green(uint32_t color) { return (color >> 8) & 0xff; }
inline constexpr uint8_t blue(uint32_t color) { return color & 0xff; }

inline constexpr uint32_t boundRGB(double red, double green, double blue)
{
    const int r = bound<int>(red * 255, 0, 255);
    const int g = bound<int>(green * 255, 0, 255);
    const int b = bound<int>(blue * 255, 0, 255);
    return (0xff000000u | ((r & 0xffu) << 16) | ((g & 0xffu) << 8) | (b & 0xffu));
}

ColorMap::ColorMap()
    : m_min(0.0), m_max(1.0), m_step(0.0), m_entries(0), m_lut(nullptr)
{
}

ColorMap::ColorMap(double min, double max)
    : m_min(min), m_max(max), m_step(0.0), m_entries(0), m_lut(nullptr)
{
}

LookUpSTORM::ColorMap::~ColorMap()
{
    delete[] m_lut;
}

uint32_t ColorMap::rgb(double value, double scale) const
{
    const double mapped = (value - m_min) / (m_max - m_min);
    const double lambda = 1.0 / (m_f1 - mapped * (m_f1 - m_f2));
    const auto tup = rgbFromWaveLength(lambda);
    return boundRGB(std::get<0>(tup) * scale, std::get<1>(tup) * scale, std::get<2>(tup) * scale);
}

void ColorMap::setRange(double min, double max)
{
    m_min = min;
    m_max = max;
}

void ColorMap::generate(double min, double max, double step, double scale)
{
    m_min = min;
    m_max = max;
    m_step = step;
    const double range = max - min;
    const size_t number = static_cast<size_t>(std::floor(((range / step) + 1)));
    delete[] m_lut;
    m_entries = number;
    m_lut = new uint32_t[number];
    for (size_t i = 0; i < number; ++i) {
        const double z = min + i * step;
        m_lut[i] = rgb(z, scale);
        //std::cerr << i << ", " << z << ": r:" << (uint32_t)red(m_lut[i]) << " g:" << (uint32_t)green(m_lut[i]) << " b:" << (uint32_t)blue(m_lut[i]) << std::endl;
    }
}

uint32_t ColorMap::cachedRgb(double value) const
{
    const size_t i = static_cast<size_t>(std::round((value - m_min) / m_step));
    //std::cerr << i << ", " << value << ": r:" << (uint32_t)red(m_lut[i]) << " g:" << (uint32_t)green(m_lut[i]) << " b:" << (uint32_t)blue(m_lut[i]) << std::endl;
#ifndef NDEBUG
    if (i >= m_entries)
        return BLACK;
#endif
    return m_lut[i];
}

uint32_t ColorMap::cachedRgbByIndex(uint32_t index) const
{
    return m_lut[index];
}

double ColorMap::min() const
{
    return m_min;
}

double ColorMap::max() const
{
    return m_max;
}

double ColorMap::step() const
{
    return m_step;
}

bool ColorMap::isCached() const
{
    return (m_step > 0.0) && (m_entries > 0);
}

std::tuple<double, double, double> ColorMap::rgbFromWaveLength(double wavelength)
{
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;

    if (wavelength >= 380.0 && wavelength <= 440.0) {
        r = -1.0 * (wavelength - 440.0) / (440.0 - 380.0);
        b = 1.0;
    }
    else if (wavelength >= 440.0 && wavelength <= 490.0) {
        g = (wavelength - 440.0) / (490.0 - 440.0);
        b = 1.0;
    }
    else if (wavelength >= 490.0 && wavelength <= 510.0) {
        g = 1.0;
        b = -1.0 * (wavelength - 510.0) / (510.0 - 490.0);
    }
    else if (wavelength >= 510.0 && wavelength <= 580.0) {
        r = (wavelength - 510.0) / (580.0 - 510.0);
        g = 1.0;
    }
    else if (wavelength >= 580.0 && wavelength <= 645.0) {
        r = 1.0;
        g = -1.0 * (wavelength - 645.0) / (645.0 - 580.0);
    }
    else if (wavelength >= 645.0 && wavelength <= 780.0) {
        r = 1.0;
    }

    double s = 1.0;
    if (wavelength > 700.0)
        s = 0.3 + 0.7 * (780.0 - wavelength) / (780.0 - 700.0);
    else if (wavelength < 420.0)
        s = 0.3 + 0.7 * (wavelength - 380.0) / (420.0 - 380.0);

    r = std::pow(r * s, 0.8);
    g = std::pow(g * s, 0.8);
    b = std::pow(b * s, 0.8);
    return { r, g, b };
}