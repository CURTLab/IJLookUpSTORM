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

#include "Renderer.h"

#include "Common.h"
#include <thread>
#include <future>

using namespace LookUpSTORM;

Renderer::Renderer()
    : m_corner(0.f)
    , m_cross(0.f)
    , m_scaleX(1.)
    , m_scaleY(1.)
{
}

Renderer::Renderer(int width, int height, double scaleX, double scaleY)
    : m_image(width, height)
    , m_corner(0.f)
    , m_cross(0.f)
    , m_scaleX(scaleX)
    , m_scaleY(scaleY)
{
}

void Renderer::release()
{
    m_image = ImageF32();
    m_renderImage = ImageU32();
}

bool Renderer::isReady() const
{
    return !m_image.isNull() && m_colorLUT.isCached();
}

void Renderer::setSize(int width, int height, double scaleX, double scaleY)
{
    if (m_image.isNull() || (width != m_image.width()) || (height != m_image.height())) {
        m_image = ImageF32(width, height);
        m_scaleX = scaleX;
        m_scaleY = scaleY;
    }
}

void LookUpSTORM::Renderer::setSettings(double minZ, double maxZ, double stepZ, float sigma)
{
    m_cross = expf(-0.5f * sqr(1.f / sigma));
    m_corner = expf(-sqr(1.f / sigma));

    m_colorLUT.generate(minZ, maxZ, stepZ);
    m_colorCrossLUT.generate(minZ, maxZ, stepZ, m_cross);
    m_colorCornerLUT.generate(minZ, maxZ, stepZ, m_corner);
}

void Renderer::setSigma(float sigma)
{
	m_cross = expf(-0.5f * sqr(1.f / sigma));
	m_corner = expf(-sqr(1.f / sigma));

    if (m_colorCrossLUT.isCached())
        m_colorCrossLUT.generate(m_colorCrossLUT.min(), m_colorCrossLUT.max(), m_colorCrossLUT.step(), m_cross);
    if (m_colorCornerLUT.isCached())
        m_colorCornerLUT.generate(m_colorCornerLUT.min(), m_colorCornerLUT.max(), m_colorCornerLUT.step(), m_cross);
}

int Renderer::imageWidth() const
{
    return m_image.width();
}

int Renderer::imageHeight() const
{
    return m_image.height();
}

void Renderer::set(double x, double y, double z)
{
    if (m_image.isNull())
        return;
    std::lock_guard<std::mutex> guard(m_mutex);
    const int dx = (int)std::round(x * m_scaleX);
    const int dy = (int)std::round(y * m_scaleY);
    if (m_image.rect().contains(dx, dy)) {
        float& pixel = m_image(dx, dy);
        pixel = (std::abs(pixel) <= 0.00001f ? z : std::max<float>(z, pixel));
        //std::cout << dx << ", " << dy << ", " << pixel << std::endl;
    }
}

std::pair<int, int> Renderer::map(double x, double y) const
{
    const int dx = (int)std::round(x * m_scaleX);
    const int dy = (int)std::round(y * m_scaleY);
    return { dx, dy };
}

void Renderer::setRenderImage(uint32_t* imagePtr, int width, int height, double scaleX, double scaleY)
{
    m_renderImage = ImageU32(width, height, imagePtr, false);
    setSize(width, height, scaleX, scaleY);
}

bool Renderer::updateImage(Rect region)
{
    if (m_renderImage.isNull())
        return false;
    
    if (region.isNull()) {
        render(m_image.rect());
    }
    else {
        // extend region by 1
        if (region.left() > 0) region.moveLeft(region.left() - 1);
        if (region.top() > 0) region.moveTop(region.top() - 1);
        if (region.right() < m_image.width()-1) region.moveRight(region.right() + 1);
        if (region.bottom() < m_image.height()-1) region.moveBottom(region.bottom() + 1);
        render(region);
    }
    return true;
}

const uint32_t* Renderer::renderImagePtr() const
{
    return m_renderImage.constData();
}

void Renderer::clear()
{
    m_image.fill(0.f);
    m_renderImage.fill(BLACK);
}

const ImageF32& Renderer::rawImage() const
{
    return m_image;
}

void Renderer::render(Rect roi)
{
    unsigned int numThreads = std::max(1u, std::thread::hardware_concurrency());
    const int numRows = roi.height() / numThreads;

    std::vector<std::future<void>> futures;
    futures.reserve(1ull + numThreads);
    for (unsigned int i = 0; i < numThreads; ++i) {
        Rect tile(0, i * numRows, roi.width(), numRows);
        if (i == numThreads - 1)
            tile.setHeight(roi.height() - i * numRows);
        auto fun = [this, tile]() { renderTile(tile); };
        futures.push_back(std::async(std::launch::async, fun));
    }
    for (size_t i = 0; i < futures.size(); ++i)
        futures[i].wait();
}

void Renderer::renderTile(const Rect& tile)
{
    const int left = tile.left();
    for (int y = tile.top(); y <= tile.bottom(); ++y) {
        uint32_t* line = m_renderImage.ptr(left, y);
        for (int x = left; x <= tile.right(); ++x)
            *line++ = pixelCached(x, y);
    }
}

uint32_t Renderer::pixelCached(int x, int y) const
{
    if ((x < 1) || (y < 1) || (x >= m_image.width() - 1) || (y >= m_image.height() - 1))
        return BLACK;

    float val;
    val = m_image(x, y);
    if (val != 0.0f) return m_colorLUT.cachedRgb(val);

    const int startX = x - 1;
    const float* line = m_image.ptr(startX, y - 1);
    if (*line != 0.0f) return m_colorCornerLUT.cachedRgb(*line);
    else if (*(++line) != 0.0f) return m_colorCrossLUT.cachedRgb(*line);
    else if (*(++line) != 0.0f) return m_colorCornerLUT.cachedRgb(*line);
    
    line = m_image.ptr(startX, y);
    if (*line != 0.0f) return m_colorCrossLUT.cachedRgb(*line);
    line += 2;
    if (*line != 0.0f) return m_colorCrossLUT.cachedRgb(*line);

    line = m_image.ptr(startX, y + 1);
    if (*line != 0.0f) return m_colorCornerLUT.cachedRgb(*line);
    else if (*(++line) != 0.0f) return m_colorCrossLUT.cachedRgb(*line);
    else if (*(++line) != 0.0f) return m_colorCornerLUT.cachedRgb(*line);
    return BLACK;
}
