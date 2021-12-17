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
        , dZ(1.)
        , minZ(0.)
    {}

    void render(Rect roi);
    void renderTile(const Rect& tile);
    uint32_t pixelCached(int x, int y) const;

    ImageU32 histogramImage;
    ImageU32 renderImage;

    float corner;
    float cross;
    double scaleX;
    double scaleY;
    double dZ;
    double minZ;
    std::mutex mutex;
    ColorMap colorLUT;
    ColorMap colorCornerLUT;
    ColorMap colorCrossLUT;

    inline void setTD(double x, double y, double z)
    {
        const int dx = (int)std::round(x * scaleX), dy = (int)std::round(y * scaleY);
        if (histogramImage.rect().contains(dx, dy)) {
            const uint32_t zi = uint32_t((z - minZ) / dZ) + 1;
            auto& pixel = histogramImage(dx, dy);
            pixel = std::max(zi, pixel);
        }
    }

    inline void setBU(double x, double y, double z)
    {
        const int dx = (int)std::round(x * scaleX), dy = (int)std::round(y * scaleY);
        if (histogramImage.rect().contains(dx, dy)) {
            const uint32_t zi = uint32_t((z - minZ) / dZ) + 1;
            auto& pixel = histogramImage(dx, dy);
            pixel = std::min(zi, pixel);
        }
    }

    inline void setXZ(double x, double y, double z)
    {
        const int dx = (int)std::round(x * scaleX);
        const int dz = (int)std::round(z / dZ * scaleY) + histogramImage.height() / 2;
        if (histogramImage.rect().contains(dx, dz)) {
            const uint32_t zi = uint32_t((z - minZ) / dZ) + 1;
            auto& pixel = histogramImage(dx, dz);
            pixel = std::max(zi, pixel);
        }
    }

    inline void setYZ(double x, double y, double z)
    {
        const int dy = (int)std::round(y * scaleX);
        const int dz = (int)std::round(z / dZ * scaleY) + histogramImage.height() / 2;
        if (histogramImage.rect().contains(dy, dz)) {
            const uint32_t zi = uint32_t((z - minZ) / dZ) + 1;
            auto& pixel = histogramImage(dy, dz);
            pixel = std::max(zi, pixel);
        }
    }
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
    if ((x < 1) || (y < 1) || (x >= histogramImage.width() - 1) || (y >= histogramImage.height() - 1))
        return BLACK;

    const uint32_t* line = histogramImage.ptr(x, y);
    if (*line) return colorLUT.cachedRgbByIndex((*line) - 1);

    const int startX = x - 1;
    line = histogramImage.ptr(startX, y - 1);
    if (*line) return colorCornerLUT.cachedRgbByIndex((*line) - 1);
    else if (*(++line)) return colorCrossLUT.cachedRgbByIndex((*line) - 1);
    else if (*(++line)) return colorCornerLUT.cachedRgbByIndex((*line) - 1);

    line = histogramImage.ptr(startX, y);
    if (*line) return colorCrossLUT.cachedRgbByIndex((*line) - 1);
    line += 2;
    if (*line) return colorCrossLUT.cachedRgbByIndex((*line) - 1);

    line = histogramImage.ptr(startX, y + 1);
    if (*line) return colorCornerLUT.cachedRgbByIndex((*line) - 1);
    else if (*(++line)) return colorCrossLUT.cachedRgbByIndex((*line) - 1);
    else if (*(++line)) return colorCornerLUT.cachedRgbByIndex((*line) - 1);

    return BLACK;
}


Renderer::Renderer()
    : d(new RendererPrivate)
{
}

LookUpSTORM::Renderer::~Renderer()
{
    delete d;
}

void Renderer::release()
{
    d->histogramImage = ImageU32();
    d->renderImage = ImageU32();
}

bool Renderer::isReady(bool verbose) const
{
    if (!verbose)
        return !d->renderImage.isNull() && !d->histogramImage.isNull() && d->colorLUT.isCached();

    // verbose check
    if (d->renderImage.isNull()) {
        std::cerr << "Renderer: Render image is null!";
        return false;
    } 
    else if (d->histogramImage.isNull()) {
        std::cerr << "Renderer: Histogram image is null!";
        return false;
    }
    else if (!d->colorLUT.isCached()) {
        std::cerr << "Renderer: Colormap not generated!";
        return false;
    }
    return true;
}

void Renderer::setSize(int width, int height, double scaleX, double scaleY)
{
    if (d->histogramImage.isNull() || (width != d->histogramImage.width()) || (height != d->histogramImage.height())) {
        d->histogramImage = ImageU32(width, height, 0);
        d->scaleX = scaleX;
        d->scaleY = scaleY;
    }
}

void LookUpSTORM::Renderer::setSettings(double minZ, double maxZ, double stepZ, float sigma)
{
    d->cross = expf(-0.5f * sqr(1.f / sigma));
    d->corner = expf(-sqr(1.f / sigma));

    d->minZ = minZ;
    d->dZ = stepZ;

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
    return d->histogramImage.width();
}

int Renderer::imageHeight() const
{
    return d->histogramImage.height();
}

void Renderer::set(double x, double y, double z)
{
    if (d->histogramImage.isNull())
        return;
    std::lock_guard<std::mutex> guard(d->mutex);

    const int dx = (int)std::round(x * d->scaleX);
    const int dy = (int)std::round(y * d->scaleY);
    if (d->histogramImage.rect().contains(dx, dy)) {
        const uint32_t zi = uint32_t((z - d->minZ) / d->dZ) + 1;
        auto& pixel = d->histogramImage(dx, dy);
        pixel = std::max(zi, pixel);
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

void Renderer::setRenderImage(ImageU32 image)
{
    d->renderImage = image;
}

bool Renderer::updateImage(Rect region)
{
    if (d->renderImage.isNull())
        return false;
    
    if (region.isNull()) {
        d->render(d->histogramImage.rect());
    }
    else {
        // extend region by 1
        if (region.left() > 0) region.moveLeft(region.left() - 1);
        if (region.top() > 0) region.moveTop(region.top() - 1);
        if (region.right() < d->histogramImage.width()-1) region.moveRight(region.right() + 1);
        if (region.bottom() < d->histogramImage.height()-1) region.moveBottom(region.bottom() + 1);
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
    d->histogramImage.fill(0);
    d->renderImage.fill(BLACK);
}

const ImageU32 Renderer::rawImageHistogram() const
{
    return d->histogramImage;
}

ImageU32 Renderer::render(const std::list<Molecule>& mols, int width, int height, double scaleX, 
    double scaleY, double minZ, double maxZ, double dZ, double sigma, Projection projection)
{
    ImageU32 image(width, height);
    Renderer r;
    r.setRenderImage(image.data(), width, height, scaleX, scaleY);
    r.setSettings(minZ, maxZ, dZ, static_cast<float>(sigma));

    switch (projection)
    {
    case Projection::TopDown: 
        for (const auto& m : mols) 
            r.d->setTD(m.x, m.y, m.z); 
        break;
    case Projection::BottomUp: 
        for (const auto& m : mols) 
            r.d->setBU(m.x, m.y, m.z); 
        break;
    case Projection::SideXZ: 
        for (const auto& m : mols) 
            r.d->setXZ(m.x, m.y, m.z); 
        break;
    case Projection::SideYZ: 
        for (const auto& m : mols) 
            r.d->setYZ(m.x, m.y, m.z); 
        break;
    }
    r.updateImage();

    return image;
}
