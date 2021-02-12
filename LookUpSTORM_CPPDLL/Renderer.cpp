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
    setSigma(1.f);
    m_cmap.setRange(-500.0, 500.0);
}

Renderer::Renderer(int width, int height, double scaleX, double scaleY)
    : m_image(width, height)
    , m_corner(0.f)
    , m_cross(0.f)
    , m_scaleX(scaleX)
    , m_scaleY(scaleY)
{
    setSigma(1.f);
    m_cmap.setRange(-500.0, 500.0);
}

void Renderer::release()
{
    m_image = ImageF32();
    m_renderImage = Image<OutType>();
}

bool Renderer::isReady() const
{
    return !m_image.isNull();
}

void Renderer::setSize(int width, int height, double scaleX, double scaleY)
{
    if (m_image.isNull() || (width != m_image.width()) || (height != m_image.height())) {
        m_image = ImageF32(width, height);
        m_scaleX = scaleX;
        m_scaleY = scaleY;
    }
}

void Renderer::setAxialRange(double min, double max)
{
    m_cmap.setRange(min, max);
}

void Renderer::setSigma(float sigma)
{
	m_cross = expf(-0.5f * sqr(1.f / sigma));
	m_corner = expf(-sqr(1.f / sigma));
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

Image<Renderer::OutType> Renderer::render()
{
    if (m_image.isNull())
        return {};

    Image<OutType> ret(m_image.width(), m_image.height());
    render(m_image, ret);
    return ret;
}

void Renderer::setRenderImage(OutType* imagePtr, int width, int height, double scaleX, double scaleY)
{
    m_renderImage = Image<OutType>(width, height, imagePtr, false);
    setSize(width, height, scaleX, scaleY);
}

bool Renderer::updateImage(Rect region)
{
    if (m_renderImage.isNull())
        return false;
    render(m_image, m_renderImage);
    return true;
    
    if (region.isNull()) {
        render(m_image, m_renderImage);
    }
    else {
        // extend region by 1
        if (region.left() > 0) region.moveLeft(region.left() - 1);
        if (region.top() > 0) region.moveTop(region.top() - 1);
        if (region.right() < m_image.width()-1) region.moveRight(region.right() + 1);
        if (region.bottom() < m_image.height()-1) region.moveBottom(region.bottom() + 1);
        ImageF32 in = m_image.subImage(region);
        Image<OutType> out = m_renderImage.subImage(region);
        render(in, out);
    }
    return true;
}

const Renderer::OutType* Renderer::renderImagePtr() const
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

void Renderer::render(const ImageF32& in, Image<OutType>& out) const
{
    if (in.isNull() || out.isNull() || (in.rect() != out.rect()))
        return;

    Rect tile(0, 0, in.width(), in.height());
    renderTile(tile, in, out);
    return;

    unsigned int numThreads = std::max(1u, std::thread::hardware_concurrency());
    const int numRows = in.height() / numThreads;

    std::vector<std::future<void>> futures;
    for (unsigned int i = 0; i < numThreads; ++i) {
        Rect tile(0, i * numRows, in.width(), numRows);
        if (i == numThreads - 1) {
            tile.setHeight(in.height() - i * numRows);
            renderTile(tile, in, out);
        }
        else {
            auto fun = [this, tile, &in, &out]() { renderTile(tile, in, out); };
            futures.push_back(std::async(std::launch::async, fun));
        }
    }
    for (size_t i = 0; i < futures.size(); ++i)
        futures[i].wait();
}

void Renderer::renderTile(const Rect& tile, const ImageF32 &in, Image<OutType>& image) const
{
    for (int y = tile.top(); y <= tile.bottom(); ++y) {
        auto* line = image.scanLine(y);
        line += tile.left();

        for (int x = tile.left(); x <= tile.right(); ++x)
            *line++ = pixel(in, x, y);
    }
}

Renderer::OutType Renderer::pixel(const ImageF32& in, int x, int y) const
{
    const Rect r = m_image.rect();
    if (m_image(x, y) != 0.0f)
        return m_cmap.rgb(m_image(x, y));

    float val;
    if ((x > 0) && ((val = m_image(x - 1, y)) != 0.0f))
        return m_cmap.rgb(val, m_cross);
    if ((x + 1 < m_image.width()) && ((val = m_image(x + 1, y)) != 0.0f))
        return m_cmap.rgb(val, m_cross);
    if ((y > 0) && ((val = m_image(x, y - 1)) != 0.0f))
        return m_cmap.rgb(val, m_cross);
    if ((y + 1 < m_image.height()) && ((val = m_image(x, y + 1)) != 0.0f))
        return m_cmap.rgb(val, m_cross);
    if (m_image.pixel(x - 1, y - 1, val) && (val != 0.f))
        return m_cmap.rgb(val, m_corner);
    if (m_image.pixel(x + 1, y - 1, val) && (val != 0.f))
        return m_cmap.rgb(val, m_corner);
    if (m_image.pixel(x - 1, y + 1, val) && (val != 0.f))
        return m_cmap.rgb(val, m_corner);
    if (m_image.pixel(x + 1, y + 1, val) && (val != 0.f))
        return m_cmap.rgb(val, m_corner);
    return BLACK;
}