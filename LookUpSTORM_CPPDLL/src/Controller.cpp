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
#include "LinearMath.h"
#include "LUT.h"

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
    Calibration cali;

};

class AstigmatismLUT : public LUT
{
    const Calibration& m_cali;
    double m_sina;
    double m_cosa;
    double m_sx, m_sy, m_dsx, m_dsy;
public:
    AstigmatismLUT(const Calibration& cali)
        : m_cali(cali)
        // rotation of PSF from calibration
        , m_sina(sin(cali.theta()))
        , m_cosa(cos(cali.theta()))
        , m_sx(0.0), m_sy(0.0), m_dsx(0.0), m_dsy(0.0)
    {}

protected:
    inline virtual void preTemplates(size_t windowSize, double dLat, double dAx, double rangeLat, double rangeAx) override {}
    inline virtual void endTemplate(size_t index, double x, double y, double z) override {}

    inline
    virtual void startTemplate(size_t index, double x, double y, double z) override
    {
        std::tie(m_sx, m_sy, m_dsx, m_dsy) = m_cali.valDer(z + m_cali.focalPlane());
    }

    inline
    virtual std::tuple<double, double, double, double> templateAtPixel(size_t index, 
        double x, double y, double z, size_t pixX, size_t pixY) override
    {
        // draw template image of astigmatism PSF from calibration at x,y,z
        const double xi = (pixX - x);
        const double yi = (pixY - y);
        const double tx = xi * m_cosa + yi * m_sina, tx2 = tx * tx;
        const double ty = -xi * m_sina + yi * m_cosa, ty2 = ty * ty;
        const double sx2 = m_sx * m_sx, sx3 = sx2 * m_sx;
        const double sy2 = m_sy * m_sy, sy3 = sy2 * m_sy;
        const double e = exp(-0.5 * tx2 / sx2 - 0.5 * ty2 / sy2);
        const double dx = (tx * m_cosa / sx2 - ty * m_sina / sy2) * e;
        const double dy = (tx * m_sina / sx2 + ty * m_cosa / sy2) * e;
        const double dz = (tx2 * m_dsx / sx3 + ty2 * m_dsy / sy3) * e;
        return { e, dx, dy, dz };
    }
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

bool LookUpSTORM::Controller::generate(LUT& lut, size_t windowSize, double dLat, double dAx, 
    double rangeLat, double rangeAx, std::function<void(size_t index, size_t max)> callback)
{
    if (!lut.generate(windowSize, dLat, dAx, rangeLat, rangeAx, callback))
        return false;
    return setLUT(lut);
}

bool Controller::generateFromCalibration(const Calibration& cali, size_t windowSize, 
    double dLat, double dAx, double rangeLat, double rangeAx, 
    std::function<void(size_t index, size_t max)> callback)
{
    AstigmatismLUT lut(cali);
    return generate(lut, windowSize, dLat, dAx, rangeLat, rangeAx, callback);
}

bool LookUpSTORM::Controller::setLUT(const LUT& lut)
{
    if (!lut.isValid()) {
        if (d->verbose)
            std::cerr << "LookUpSTORM: LUT is not vaild!" << std::endl;
        return false;
    }

    if (!d->fitter.setLookUpTable(lut.ptr(), lut.dataSize(), true, lut.windowSize(), lut.dLat(), lut.dAx(), lut.rangeLat(), lut.rangeAx())) {
        if (d->verbose)
            std::cerr << "Controller: Could not set generated LUT!" << std::endl;
        return false;
    }

    d->renderer.setSettings(lut.minAx(), lut.maxAx(), lut.dAx(), 1.f);
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
            if (verbose)
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

            m.xfit = m.x;
            m.yfit = m.y;

            m.x += region.left();
            m.y += region.top();

            changedRegion.extendByPoint(d->renderer.map(m.x, m.y));
            d->renderer.set(m.x, m.y, m.z);

            d->detectedMolecues.push_back(m);
            d->mols.push_back(m);
        }

        const auto t = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration<double, std::milli>(t - t0).count() > timeoutMS) {
            if (verbose)
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

double LookUpSTORM::Controller::calculatePhotons(const Molecule& mol, const double adu, const double gain) const
{
    if (!d->fitter.isReady()) {
        if (d->verbose)
            std::cerr << "LookUpSTORM: LUT is not set!" << std::endl;
        return std::numeric_limits<double>::quiet_NaN();
    }

    const size_t winSize = d->fitter.windowSize();
    const size_t pixels = winSize * winSize;
    const double photonFactor = adu / gain;

    const double* psf = d->fitter.templatePtr(mol.xfit, mol.yfit, mol.z);
    if (psf == nullptr) {
        if (d->verbose)
            std::cerr << "LookUpSTORM: Molecule at the position " << mol.xfit << "," << mol.yfit << "," << mol.z << "is invalid!" << std::endl;
        return false;
    }

    double photons = 0.0;
    for (size_t i = 0; i < pixels; ++i)
        photons += psf[4 * i] * mol.peak * photonFactor;
    return photons;
}

bool Controller::calculateCRLB(const Molecule& mol, double* crlb, const double adu, 
    const double gain, const double offset, const double pixelSize) const
{
    if (!d->fitter.isReady()) {
        if (d->verbose)
            std::cerr << "LookUpSTORM: LUT is not set!" << std::endl;
        return false;
    }
    if (crlb == nullptr) {
        if (d->verbose)
            std::cerr << "LookUpSTORM: CRLB array is null!" << std::endl;
        return false;
    }

    const double photonFactor = adu / gain;

    const size_t winSize = d->fitter.windowSize();
    const size_t pixels = winSize * winSize;

    const double* psf = d->fitter.templatePtr(mol.xfit, mol.yfit, mol.z);
    if (psf == nullptr) {
        if (d->verbose)
            std::cerr << "LookUpSTORM: Molecule at the position " << mol.xfit << "," << mol.yfit << "," << mol.z << "is invalid!" << std::endl;
        return false;
    }

    const double photons = mol.peak * photonFactor;

    // helper function for the derivative of the LUT model at (b, I, x, y, z)
    auto der = [psf, pixels, photons, pixelSize](size_t i, size_t k) {
        double scale = 1.0;
        if ((i == 2) || (i == 3)) scale = photons / pixelSize;
        else if (i == 4) scale = photons;
        //if (i > 1) scale = photons / pixelSize;
        return i == 0 ? 1.0 : psf[4 * k + (i - 1)] * scale;
    };

    Matrix fisher(5, 5, 0.0);
    for (size_t i = 0; i < pixels; ++i) {
        // intensity of the molecule at the pixel k 
        const double I = photonFactor * (mol.peak * psf[i] + mol.background) - offset * photonFactor;
        for (size_t j = 0; j < 5; ++j) {
            for (size_t k = 0; k < 5; ++k) {
                fisher(j, k) += der(j, i) * der(k, i) / I;
            }
        }
    }

    // calculate inverse of Fisher information matrix
    int ipiv[5];
    if (LAPACKE::dgetrf(fisher, ipiv) != 0) {
        if (d->verbose)
            std::cerr << "LookUpSTORM: dgetrf failed!" << std::endl;
        return false;
    }
    if (LAPACKE::dgetri(fisher, ipiv) != 0) {
        if (d->verbose)
            std::cerr << "LookUpSTORM: dgetri failed!" << std::endl;
        return false;
    }

    for (size_t i = 0; i < 5; ++i)
        crlb[i] = std::sqrt(fisher(i, i));

    return true;
}

void Controller::reset()
{
    d->isLocFinished.store(false);
    d->isSMLMImageReady.store(false);
    d->numberOfDetectedLocs.store(0);
    d->mols.clear();
    d->renderer.clear();
}
