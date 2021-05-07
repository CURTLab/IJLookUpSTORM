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

#include "LUT.h"

#include <cmath>
#include <iostream>
#include <fstream>

using namespace LookUpSTORM;

LUT::LUT()
    : m_data(nullptr)
    , m_dataSize(0)
    , m_windowSize(0)
    , m_countLat(0)
    , m_countAx(0)
    , m_dLat(0.0)
    , m_dAx(0.0)
    , m_rangeLat(0.0)
    , m_rangeAx(0.0)
    , m_minLat(0.0)
    , m_maxLat(0.0)
    , m_minAx(0.0)
    , m_maxAx(0.0)
{
}

bool LUT::generate(size_t windowSize, double dLat, double dAx, double rangeLat, double rangeAx, std::function<void(size_t index, size_t max)> callback)
{
    const size_t n = windowSize * windowSize;

    size_t i = 0;
    const double borderLat = std::floor((windowSize - rangeLat) / 2);
    if (borderLat < 1.0) {
        std::cerr << "LUT: Border during generation is smaller than 1!" << std::endl;
        return false;
    }

    m_windowSize = windowSize;
    m_dLat = dLat;
    m_dAx = dAx;
    m_rangeLat = rangeLat;
    m_rangeAx = rangeAx;

    // calculate spatial minimas and maximas
    m_minLat = borderLat;
    m_maxLat = windowSize - borderLat;
    m_minAx = -rangeAx * 0.5;
    m_maxAx = rangeAx * 0.5;

    // calculate amount of template images
    m_countLat = static_cast<size_t>(std::floor((((m_maxLat - m_minLat) / dLat) + 1)));
    m_countAx = static_cast<size_t>(std::floor(((rangeAx / dAx) + 1)));
    const size_t countIndex = m_countLat * m_countLat * m_countAx;

    const size_t stride = windowSize * windowSize * 4ull;

    m_dataSize = countIndex * stride;
    delete m_data;
    m_data = new double[m_dataSize];
    
    preTemplates(windowSize, dLat, dAx, rangeLat, rangeAx);

    double* pixels = m_data;
    for (i = 0; i < countIndex; ++i) {
        const size_t zidx = i % m_countAx;
        const size_t yidx = (i / m_countAx) % m_countLat;
        const size_t xidx = i / (m_countAx * m_countLat);

        const double x = m_minLat + xidx * dLat;
        const double y = m_minLat + yidx * dLat;
        const double z = m_minAx + zidx * dAx;

        startTemplate(i, x, y, z);
        for (size_t j = 0; j < n; ++j, pixels += 4) {
            const size_t yy = j / windowSize;
            const size_t xx = j - yy * windowSize;
            std::tie(pixels[0], pixels[1], pixels[2], pixels[3]) = 
                templateAtPixel(i, x, y, z, xx, yy);
        }
        endTemplate(i, x, y, z);
        callback(i, countIndex);
    }

    return true;
}

void LUT::release()
{
    delete m_data;
}

bool LookUpSTORM::LUT::save(const std::string& fileName)
{
    if (!isValid())
        return false;

    std::fstream file(fileName, std::ios::out | std::ios::binary);
    if (!file)
        return false;

    struct HeaderLUT {
        char id[8] = { 'L','U','T','D','S','M','L','M' };
        size_t dataSize;
        size_t indices;
        size_t windowSize;
        double dLat;
        double dAx;
        double rangeLat;
        double rangeAx;
    } hdr;

    hdr.dataSize = m_dataSize * sizeof(double);
    hdr.indices = m_countAx * m_countLat * m_countLat;
    hdr.windowSize = m_windowSize;
    hdr.dLat = m_dLat;
    hdr.dAx = m_dAx;
    hdr.rangeLat = m_rangeLat;
    hdr.rangeAx = m_rangeAx;

    file.write((const char*)&hdr, sizeof(hdr));
    file.write((const char*)m_data, hdr.dataSize);

    file.close();
    return true;
}

size_t LUT::lookupIndex(double x, double y, double z) const
{
    const size_t xi = static_cast<size_t>(std::round((x - m_minLat) / m_dLat));
    const size_t yi = static_cast<size_t>(std::round((y - m_minLat) / m_dLat));
    const size_t zi = static_cast<size_t>(std::round((z - m_minAx) / m_dAx));
    const size_t index = zi + yi * m_countAx + xi * m_countAx * m_countLat;
    return index;
}

std::tuple<double, double, double> LookUpSTORM::LUT::lookupPosition(size_t index) const
{
    const size_t zidx = index % m_countAx;
    const size_t yidx = (index / m_countAx) % m_countLat;
    const size_t xidx = index / (m_countAx * m_countLat);

    const double x = m_minLat + xidx * m_dLat;
    const double y = m_minLat + yidx * m_dLat;
    const double z = m_minAx + zidx * m_dAx;

    return { x, y, z };
}

size_t LookUpSTORM::LUT::calculateUsageBytes(size_t windowSize, double dLat, double dAx, double rangeLat, double rangeAx)
{
    const double minLat = std::floor((windowSize - rangeLat) / 2);
    const size_t countLat = static_cast<size_t>(std::floor((((windowSize - 2.0 * minLat) / dLat) + 1)));
    const size_t countAx = static_cast<size_t>(std::floor(((rangeAx / dAx) + 1)));
    return countLat * countLat * countAx * 4ull * sizeof(double) * (windowSize * windowSize);
}
