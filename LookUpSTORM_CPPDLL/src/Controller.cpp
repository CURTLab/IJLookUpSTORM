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

#include "Controller.h"

#include <chrono>
#include <iostream>
#include <algorithm>
#include <cmath>

#include "LocalMaximumSearch.h"

#undef min
#undef max

namespace LookUpSTORM
{

class ControllerPrivate
{
public:
    inline ControllerPrivate()
        : isLocFinished(false)
        , isSMLMImageReady(false)
        , nms(1, 6)
        , imageWidth(0)
        , imageHeight(0)
        , threshold(0)
        , frameFittingTimeMS(0.0)
        , renderUpdateRate(5)
        , timeoutMS(250.0)
        , enableRendering(true)
        , verbose(false)
    {
        numberOfDetectedLocs.store(0);
    }

    std::atomic<bool> isLocFinished;
    std::atomic<bool> isSMLMImageReady;
    LocalMaximumSearch nms;
    int imageWidth;
    int imageHeight;
    std::atomic<uint16_t> threshold;
    Fitter fitter;
    std::list<Molecule> detectedMolecues;
    std::atomic<int32_t> numberOfDetectedLocs;
    std::atomic<double> frameFittingTimeMS;
    std::atomic<double> renderTimeMS;
    std::atomic<int> renderUpdateRate;
    std::atomic<bool> enableRendering;
    std::atomic<double> timeoutMS;
    std::list<Molecule> mols;
    Renderer renderer;
    std::atomic<bool> verbose;

};

} // namespace LookUpSTORM

using namespace LookUpSTORM;

#ifdef JNI_EXPORT_LUT
Controller* Controller::LOOKUPSTORM_INSTANCE = nullptr;
#endif // CONTROLLER_STATIC

Controller::Controller()
    : d(new ControllerPrivate)
{
}

#ifdef JNI_EXPORT_LUT
Controller* Controller::inst()
{
    if (LOOKUPSTORM_INSTANCE == nullptr)
        LOOKUPSTORM_INSTANCE = new Controller;
    return LOOKUPSTORM_INSTANCE;
}

void Controller::release()
{
    delete LOOKUPSTORM_INSTANCE;
    LOOKUPSTORM_INSTANCE = nullptr;
}
#endif // CONTROLLER_STATIC

LookUpSTORM::Controller::~Controller()
{
    delete d;
}

bool Controller::isReady() const
{
    return d->renderer.isReady() && d->fitter.isReady() && (d->imageWidth > 0) && (d->imageHeight > 0);
}

bool LookUpSTORM::Controller::generateFromCalibration(const Calibration& cali, size_t windowSize, 
    double dLat, double dAx, double rangeLat, double rangeAx, 
    std::function<void(size_t index, size_t max)> callback)
{
    const size_t n = windowSize * windowSize;

    size_t i = 0;
    const double borderLat = std::floor((windowSize - rangeLat) / 2);
    if (borderLat < 1.0) {
        std::cerr << "Controller: Border during generation is smaller than 1!" << std::endl;
        return false;
    }

    // calculate spatial minimas and maximas
    const double minLat = borderLat;
    const double maxLat = windowSize - borderLat;
    const double minAx = -rangeAx * 0.5;
    const double maxAx = rangeAx * 0.5;

    // calculate amount of template images
    const size_t countLat = static_cast<size_t>(std::floor((((maxLat - minLat) / dLat) + 1)));
    const size_t countAx = static_cast<size_t>(std::floor(((rangeAx / dAx) + 1)));
    const size_t countIndex = countLat * countLat * countAx;

    const size_t stride = windowSize * windowSize * 4ull;

    // rotation of PSF from calibration
    const double sina = sin(cali.theta());
    const double cosa = cos(cali.theta());

    double sx, sy, dsx, dsy;

    const size_t dataSize = countIndex * stride;
    double *data = new double[dataSize];
    double* pixels = data;
    for (i = 0; i < countIndex; ++i) {
        const size_t zidx = i % countAx;
        const size_t yidx = (i / countAx) % countLat;
        const size_t xidx = i / (countAx * countLat);

        const double x = minLat + xidx * dLat;
        const double y = minLat + yidx * dLat;
        const double z = minAx + zidx * dAx;

        std::tie(sx, sy, dsx, dsy) = cali.valDer(z + cali.focalPlane());

        // draw template image of astigmatism PSF from calibration at x,y,z
        for (size_t j = 0; j < n; ++j) {
            const size_t yy = j / windowSize;
            const size_t xx = j - yy * windowSize;

            const double xi = (xx - x);
            const double yi = (yy - y);
            const double tx = xi * cosa + yi * sina, tx2 = tx * tx;
            const double ty = -xi * sina + yi * cosa, ty2 = ty * ty;
            const double sx2 = sx * sx, sx3 = sx2 * sx;
            const double sy2 = sy * sy, sy3 = sy2 * sy;
            const double e = exp(-0.5 * tx2 / sx2 - 0.5 * ty2 / sy2);

            *pixels++ = e;
            *pixels++ = (tx * cosa / sx2 - ty * sina / sy2) * e;
            *pixels++ = (tx * sina / sx2 + ty * cosa / sy2) * e;
            *pixels++ = (tx2 * dsx / sx3 + ty2 * dsy / sy3) * e;
        }

        callback(i, countIndex);
    }

    if (!d->fitter.setLookUpTable(data, dataSize, true, windowSize, dLat, dAx, rangeLat, rangeAx)) {
        std::cerr << "Controller: Could not set generated LUT!" << std::endl;
        return false;
    }

    d->renderer.setSettings(minAx, maxAx, dAx, 1.f);
    reset();

    return true;
}

bool Controller::isLocFinished() const
{
    return d->isLocFinished.load();
}

bool Controller::isSMLMImageReady() const
{
    return d->isSMLMImageReady.load();
}

void Controller::clearSMLMImageReady()
{
    d->isSMLMImageReady.store(false);
}

bool Controller::processImage(ImageU16 image, int frame)
{
    const bool verbose = d->verbose.load();
    if (!isReady()) {
        if (verbose)
            std::cerr << "LookUpSTORM: Image processor is not ready!" << std::endl;
        return false;
    }

    d->isLocFinished.store(false);
    const auto t0 = std::chrono::high_resolution_clock::now();

    const size_t winSize = d->fitter.windowSize();
    d->nms.setRadius(winSize * 3 / 4);
    d->nms.setBorder(winSize / 2);

    const uint16_t threshold = d->threshold.load();
    const int updateRate = d->renderUpdateRate.load();
    const double timeoutMS = d->timeoutMS.load();

    auto features = d->nms.find(image, d->threshold);
    Molecule m;
    Rect bounds = image.rect();
    Rect changedRegion;
    d->detectedMolecues.clear();
    //std::cout << "Features: " << features.size() << std::endl;
    for (const auto& f : features) {
        auto t_start = std::chrono::high_resolution_clock::now();
        m.peak = std::max<double>(0.0, double(f.val) - f.localBg);
        m.background = f.localBg;
        m.x = f.x;
        m.y = f.y;
        m.z = 0.0;
        m.frame = frame;

        Rect region(int(f.x) - winSize / 2, int(f.y) - winSize/2, winSize, winSize);
        if (!region.moveInside(bounds)) {
            std::cerr << "LookUpSTORM: Impossible ROI!" << std::endl;
            continue;
        }

        ImageU16 roi = image.subImage(region);

        const bool success = d->fitter.fitSingle(roi, m);
        auto t_end = std::chrono::high_resolution_clock::now();

        m.time_us = std::chrono::duration<double, std::micro>(t_end - t_start).count();

        if (success) {
            if (m.peak < threshold)
                continue;

            m.x += region.left();
            m.y += region.top();

            changedRegion.extendByPoint(d->renderer.map(m.x, m.y));
            d->renderer.set(m.x, m.y, m.z + 1E-6);

            d->detectedMolecues.push_back(m);
            d->mols.push_back(m);
        }

        const auto t = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration<double, std::milli>(t - t0).count() > timeoutMS) {
            std::cerr << "LookUpSTORM: Timeout!" << std::endl;
            return false;
        }
    }
    const auto t1 = std::chrono::high_resolution_clock::now();

    // update SMLM image
    if (!d->isSMLMImageReady && d->enableRendering.load() && ((updateRate <= 1) || ((changedRegion.area() > 25) && (frame > 1) && (frame % updateRate == 0)))) {
        d->renderer.updateImage();
        changedRegion = {};
        d->isSMLMImageReady = true;
    }

    const auto t2 = std::chrono::high_resolution_clock::now();

    d->frameFittingTimeMS.store(std::chrono::duration<double, std::milli>(t1 - t0).count());
    d->renderTimeMS.store(std::chrono::duration<double, std::milli>(t2 - t1).count());


    if (verbose)
        std::cout << "Fitted " << d->detectedMolecues.size() << " emitter of frame " << frame << " in " << d->frameFittingTimeMS << " ms" << std::endl;

    d->numberOfDetectedLocs.store(static_cast<uint16_t>(d->detectedMolecues.size()));
    d->isLocFinished.store(true);
    return true;
}

void Controller::setImageSize(int width, int height)
{
    d->imageWidth = width;
    d->imageHeight = height;
}

int Controller::imageWidth() const
{
    return d->imageWidth;
}

int Controller::imageHeight() const
{
    return d->imageHeight;
}

void Controller::setThreshold(uint16_t threshold)
{
    d->threshold.store(threshold);
}

uint16_t Controller::threshold() const
{
    return d->threshold.load();
}

void LookUpSTORM::Controller::setVerbose(bool verbose)
{
    d->verbose.store(verbose);
}

bool LookUpSTORM::Controller::isVerbose() const
{
    return d->verbose.load();
}

void LookUpSTORM::Controller::setFrameRenderUpdateRate(int rate)
{
    d->renderUpdateRate.store(rate);
}

int LookUpSTORM::Controller::frameRenderUpdateRate() const
{
    return d->renderUpdateRate.load();
}

void LookUpSTORM::Controller::setTimeoutMS(double timeoutMS)
{
    d->timeoutMS.store(timeoutMS);
}

double LookUpSTORM::Controller::timeoutMS() const
{
    return d->timeoutMS.load();
}

std::list<Molecule>& Controller::detectedMolecues()
{
    return d->detectedMolecues;
}

std::list<Molecule>& Controller::allMolecues()
{
    return d->mols;
}

int32_t Controller::numberOfDetectedLocs()
{
    return d->numberOfDetectedLocs.load();
}

Fitter& Controller::fitter()
{
    return d->fitter;
}

double LookUpSTORM::Controller::frameFittingTimeMS() const
{
    return d->frameFittingTimeMS.load();
}

double LookUpSTORM::Controller::renderTimeMS() const
{
    return d->renderTimeMS.load();
}

void LookUpSTORM::Controller::setRenderingEnabled(bool enabled)
{
    d->enableRendering.store(enabled);
}

bool LookUpSTORM::Controller::isRenderingEnabled() const
{
    return d->enableRendering.load();
}

Renderer& Controller::renderer()
{
    return d->renderer;
}

void Controller::setRenderScale(double scale)
{
    d->renderer.setSize(
        (int)std::ceil(d->imageWidth * scale),
        (int)std::ceil(d->imageHeight * scale),
        scale, scale);
}

void Controller::setRenderSize(int width, int height)
{
    d->renderer.setSize(width, height,
        double(width) / d->imageWidth, 
        double(height) / d->imageHeight);
}

void Controller::reset()
{
    d->isLocFinished.store(false);
    d->isSMLMImageReady.store(false);
    d->numberOfDetectedLocs.store(0);
    d->mols.clear();
    d->renderer.clear();
}
