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
#include "ColorMap.h"
#include <thread>
#include <future>

namespace LookUpSTORM
{

class RendererPrivate
{
public:
    inline RendererPrivate()
        : corner(0.f)
        , cross(0.f)
        , scaleX(1.)
        , scaleY(1.)
    {}

    inline RendererPrivate(int width, int height, double scaleX, double scaleY)
        : image(width, height)
        , corner(0.f)
        , cross(0.f)
        , scaleX(scaleX)
        , scaleY(scaleY)
    {}

    void render(Rect roi);
    void renderTile(const Rect& tile);
    uint32_t pixelCached(int x, int y) const;

    ImageF32 image;
    ImageU32 renderImage;

    float corner;
    float cross;
    double scaleX;
    double scaleY;
    std::mutex mutex;
    ColorMap colorLUT;
    ColorMap colorCornerLUT;
    ColorMap colorCrossLUT;

};

} // namespace LookUpSTORM

using namespace LookUpSTORM;

void RendererPrivate::render(Rect roi)
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

void RendererPrivate::renderTile(const Rect& tile)
{
    const int left = tile.left();
    for (int y = tile.top(); y <= tile.bottom(); ++y) {
        uint32_t* line = renderImage.ptr(left, y);
        for (int x = left; x <= tile.right(); ++x)
            *line++ = pixelCached(x, y);
    }
}

uint32_t RendererPrivate::pixelCached(int x, int y) const
{
    if ((x < 1) || (y < 1) || (x >= image.width() - 1) || (y >= image.height() - 1))
        return BLACK;

    float val;
    val = image(x, y);
    if (val != 0.0f) return colorLUT.cachedRgb(val);

    const int startX = x - 1;
    const float* line = image.ptr(startX, y - 1);
    if (*line != 0.0f) return colorCornerLUT.cachedRgb(*line);
    else if (*(++line) != 0.0f) return colorCrossLUT.cachedRgb(*line);
    else if (*(++line) != 0.0f) return colorCornerLUT.cachedRgb(*line);

    line = image.ptr(startX, y);
    if (*line != 0.0f) return colorCrossLUT.cachedRgb(*line);
    line += 2;
    if (*line != 0.0f) return colorCrossLUT.cachedRgb(*line);

    line = image.ptr(startX, y + 1);
    if (*line != 0.0f) return colorCornerLUT.cachedRgb(*line);
    else if (*(++line) != 0.0f) return colorCrossLUT.cachedRgb(*line);
    else if (*(++line) != 0.0f) return colorCornerLUT.cachedRgb(*line);
    return BLACK;
}


Renderer::Renderer()
    : d(new RendererPrivate)
{
}

Renderer::Renderer(int width, int height, double scaleX, double scaleY)
    : d(new RendererPrivate(width, height, scaleX, scaleY))
{
}

LookUpSTORM::Renderer::~Renderer()
{
    delete d;
}

void Renderer::release()
{
    d->image = ImageF32();
    d->renderImage = ImageU32();
}

bool Renderer::isReady() const
{
    return !d->image.isNull() && d->colorLUT.isCached();
}

void Renderer::setSize(int width, int height, double scaleX, double scaleY)
{
    if (d->image.isNull() || (width != d->image.width()) || (height != d->image.height())) {
        d->image = ImageF32(width, height);
        d->scaleX = scaleX;
        d->scaleY = scaleY;
    }
}

void LookUpSTORM::Renderer::setSettings(double minZ, double maxZ, double stepZ, float sigma)
{
    d->cross = expf(-0.5f * sqr(1.f / sigma));
    d->corner = expf(-sqr(1.f / sigma));

    d->colorLUT.generate(minZ, maxZ, stepZ);
    d->colorCrossLUT.generate(minZ, maxZ, stepZ, d->cross);
    d->colorCornerLUT.generate(minZ, maxZ, stepZ, d->corner);
}

void Renderer::setSigma(float sigma)
{
	d->cross = expf(-0.5f * sqr(1.f / sigma));
	d->corner = expf(-sqr(1.f / sigma));

    if (d->colorCrossLUT.isCached())
        d->colorCrossLUT.generate(d->colorCrossLUT.min(), d->colorCrossLUT.max(), d->colorCrossLUT.step(), d->cross);
    if (d->colorCornerLUT.isCached())
        d->colorCornerLUT.generate(d->colorCornerLUT.min(), d->colorCornerLUT.max(), d->colorCornerLUT.step(), d->cross);
}

int Renderer::imageWidth() const
{
    return d->image.width();
}

int Renderer::imageHeight() const
{
    return d->image.height();
}

void Renderer::set(double x, double y, double z)
{
    if (d->image.isNull())
        return;
    std::lock_guard<std::mutex> guard(d->mutex);
    const int dx = (int)std::round(x * d->scaleX);
    const int dy = (int)std::round(y * d->scaleY);
    if (d->image.rect().contains(dx, dy)) {
        float& pixel = d->image(dx, dy);
        pixel = (std::abs(pixel) <= 0.00001f ? z : std::max<float>(z, pixel));
        //std::cout << dx << ", " << dy << ", " << pixel << std::endl;
    }
}

std::pair<int, int> Renderer::map(double x, double y) const
{
    const int dx = (int)std::round(x * d->scaleX);
    const int dy = (int)std::round(y * d->scaleY);
    return { dx, dy };
}

void Renderer::setRenderImage(uint32_t* imagePtr, int width, int height, double scaleX, double scaleY)
{
    d->renderImage = ImageU32(width, height, imagePtr, false);
    setSize(width, height, scaleX, scaleY);
}

bool Renderer::updateImage(Rect region)
{
    if (d->renderImage.isNull())
        return false;
    
    if (region.isNull()) {
        d->render(d->image.rect());
    }
    else {
        // extend region by 1
        if (region.left() > 0) region.moveLeft(region.left() - 1);
        if (region.top() > 0) region.moveTop(region.top() - 1);
        if (region.right() < d->image.width()-1) region.moveRight(region.right() + 1);
        if (region.bottom() < d->image.height()-1) region.moveBottom(region.bottom() + 1);
        d->render(region);
    }
    return true;
}

const uint32_t* Renderer::renderImagePtr() const
{
    return d->renderImage.constData();
}

void Renderer::clear()
{
    d->image.fill(0.f);
    d->renderImage.fill(BLACK);
}

const ImageF32& Renderer::rawImage() const
{
    return d->image;
}