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

using namespace LookUpSTORM;

LUT::LUT()
    : m_data(nullptr)
    , m_dataSize(0)
    , m_windowSize(0)
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
        std::cerr << "Controller: Border during generation is smaller than 1!" << std::endl;
        return false;
    }

    // calculate spatial minimas and maximas
    m_minLat = borderLat;
    m_maxLat = windowSize - borderLat;
    m_minAx = -rangeAx * 0.5;
    m_maxAx = rangeAx * 0.5;

    // calculate amount of template images
    const size_t countLat = static_cast<size_t>(std::floor((((m_maxLat - m_minLat) / dLat) + 1)));
    const size_t countAx = static_cast<size_t>(std::floor(((rangeAx / dAx) + 1)));
    const size_t countIndex = countLat * countLat * countAx;

    const size_t stride = windowSize * windowSize * 4ull;

    m_dataSize = countIndex * stride;
    delete m_data;
    m_data = new double[m_dataSize];
    
    preTemplates(windowSize, dLat, dAx, rangeLat, rangeAx);

    double* pixels = m_data;
    for (i = 0; i < countIndex; ++i) {
        const size_t zidx = i % countAx;
        const size_t yidx = (i / countAx) % countLat;
        const size_t xidx = i / (countAx * countLat);

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
