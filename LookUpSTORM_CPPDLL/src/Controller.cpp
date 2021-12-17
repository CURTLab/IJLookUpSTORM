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
#include "AutoThreshold.h"
#include "Wavelet.h"

#undef min
#undef max

namespace LookUpSTORM
{

class ControllerPrivate
{
public:
    inline ControllerPrivate()
        : isSMLMImageReady(false)
        , nms(1, 6)
        , imageWidth(0)
        , imageHeight(0)
        , threshold(0)
        , frameFittingTimeMS(0.0)
        , renderUpdateRate(5)
        , timeoutMS(250.0)
        , autoThresholdUpdateRate(10)
        , enableRendering(true)
        , waveletFactor(1.25f)
        , enableWavelet(false)
        , verbose(false)
    {
        numberOfDetectedLocs.store(0);
    }

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
    Wavelet wavelet;
    float waveletFactor;
    std::atomic<bool> enableWavelet;
    std::atomic<double> timeoutMS;
    std::list<Molecule> mols;
    AutoThreshold autoThreshold;
    std::atomic<int> autoThresholdUpdateRate;
    Renderer renderer;
    std::atomic<bool> verbose;
    Calibration cali;
    Rect changedRegion;

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

Controller::~Controller()
{
    delete d;
}

bool Controller::isReady() const
{
    return /*d->renderer.isReady() && */d->fitter.isReady() && (d->imageWidth > 0) && (d->imageHeight > 0);
}

bool Controller::generate(LUT& lut, size_t windowSize, double dLat, double dAx, 
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

bool Controller::setLUT(const LUT& lut)
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

    const auto t0 = std::chrono::high_resolution_clock::now();

    const size_t winSize = d->fitter.windowSize();
    d->nms.setRadius(winSize * 3 / 4);
    d->nms.setBorder(winSize / 2);

    const uint16_t threshold = d->threshold.load();
    const double timeoutMS = d->timeoutMS.load();

    std::list<LocalMaximum> features;
    if (d->enableWavelet.load()) {
        const ImageF32 &filtered = d->wavelet.filter(image);
        const float waveletThreshold = d->autoThreshold.isEnabled() ? 0.f : d->waveletFactor * d->wavelet.inputSTD();
        features = d->nms.find(image, filtered, waveletThreshold);
    }
    else {
        // at the moment only use find all for auto threshold
        if (d->autoThreshold.isEnabled())
            features = d->nms.findAll(image);
        else
            features = d->nms.find(image, threshold);
    }

    Molecule m;
    Rect bounds = image.rect();
    d->changedRegion = {};
    d->detectedMolecues.clear();
    //std::cout << "Features: " << features.size() << std::endl;

    int failureRetries = 25;

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

        // add all fitted candiates intensities even if they failed for auto thresholding
        d->autoThreshold.addMolecule(m);

        if (success && (m.peak >= threshold)) {
            m.xfit = m.x;
            m.yfit = m.y;

            m.x += region.left();
            m.y += region.top();

            d->changedRegion.extendByPoint(d->renderer.map(m.x, m.y));
            d->renderer.set(m.x, m.y, m.z);

            d->detectedMolecues.push_back(m);
            d->mols.push_back(m);
        }
        else {
            --failureRetries;
        }

        const auto t = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration<double, std::milli>(t - t0).count() > timeoutMS) {
            if (verbose)
                std::cerr << "LookUpSTORM: Timeout!" << std::endl;
            return false;
        }

        if (failureRetries == 0)
            break;
    }

    const auto t1 = std::chrono::high_resolution_clock::now();
    
    d->frameFittingTimeMS.store(std::chrono::duration<double, std::milli>(t1 - t0).count());

    if (verbose)
        std::cout << "Fitted " << d->detectedMolecues.size() << " emitter of frame " << frame << " in " << d->frameFittingTimeMS << " ms" << std::endl;

    d->numberOfDetectedLocs.store(static_cast<uint16_t>(d->detectedMolecues.size()));
    return true;
}

void Controller::setImageSize(int width, int height)
{
    d->imageWidth = width;
    d->imageHeight = height;
    d->wavelet.setSize(width, height);
}

int Controller::imageWidth() const
{
    return d->imageWidth;
}

int Controller::imageHeight() const
{
    return d->imageHeight;
}

bool Controller::renderToImage(ImageU32 image, int frame)
{
    d->renderer.setRenderImage(image);
    return updateRenderer(frame);
}

bool Controller::updateRenderer(int frame)
{
    const int updateRate = d->renderUpdateRate.load();

    // update SMLM image
    if (!d->isSMLMImageReady && d->enableRendering.load() &&
        ((updateRate <= 1) || ((d->changedRegion.area() > 25) && (frame > 1) && (frame % updateRate == 0)))
        ) {

        const auto t1 = std::chrono::high_resolution_clock::now();

        d->renderer.updateImage();
        d->changedRegion = {};
        d->isSMLMImageReady = true;

        const auto t2 = std::chrono::high_resolution_clock::now();

        d->renderTimeMS.store(std::chrono::duration<double, std::milli>(t2 - t1).count());

        return true;
    }
    return false;
}

bool Controller::updateAutoThreshold(int frame)
{
    const int updateRate = d->autoThresholdUpdateRate.load();

    // recalculate threshold
    if (d->autoThreshold.isEnabled() &&
        ((updateRate <= 1) || ((frame > 1) && (frame % updateRate == 0)))
        ) {
        d->threshold.store(std::min(static_cast<uint16_t>(std::ceil(d->autoThreshold.calculateThreshold())), MAX_INTENSITY));
        return true;
    }
    return false;
}

void Controller::setThreshold(uint16_t threshold)
{
    d->threshold.store(threshold);
}

uint16_t Controller::threshold() const
{
    return d->threshold.load();
}

bool Controller::isAutoThresholdEnabled() const
{
    return d->autoThreshold.isEnabled();
}

void Controller::setAutoThresholdEnabled(bool enabled)
{
    d->autoThreshold.setEnabled(enabled);
}

void Controller::setAutoThresholdUpdateRate(int rate)
{
    d->autoThresholdUpdateRate.store(rate);
}

int Controller::autoThresholdUpdateRate() const
{
    return d->autoThresholdUpdateRate.load();
}

void Controller::setWaveletFilterEnabled(bool enabled)
{
    d->enableWavelet.store(enabled);
}

bool Controller::isWaveletFilterEnabled() const
{
    return d->enableWavelet.load();
}

void Controller::setWaveletFactor(float factor)
{
    d->waveletFactor = factor;
}

float Controller::waveletFactor() const
{
    return d->waveletFactor;
}

void Controller::setVerbose(bool verbose)
{
    d->verbose.store(verbose);
}

bool Controller::isVerbose() const
{
    return d->verbose.load();
}

void Controller::setFrameRenderUpdateRate(int rate)
{
    d->renderUpdateRate.store(rate);
}

int Controller::frameRenderUpdateRate() const
{
    return d->renderUpdateRate.load();
}

void Controller::setTimeoutMS(double timeoutMS)
{
    d->timeoutMS.store(timeoutMS);
}

double Controller::timeoutMS() const
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

const Fitter& Controller::fitter() const
{
    return d->fitter;
}

double Controller::frameFittingTimeMS() const
{
    return d->frameFittingTimeMS.load();
}

Milliseconds Controller::frameFittingTime() const
{
    return Milliseconds(d->renderTimeMS.load());
}

double Controller::renderTimeMS() const
{
    return d->renderTimeMS.load();
}

void Controller::setRenderingEnabled(bool enabled)
{
    d->enableRendering.store(enabled);
}

bool Controller::isRenderingEnabled() const
{
    return d->enableRendering.load();
}

Renderer& Controller::renderer()
{
    return d->renderer;
}

const Renderer& LookUpSTORM::Controller::renderer() const
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

double Controller::calculatePhotons(const Molecule& mol, const double adu, const double gain) const
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
    // parameters: i-th pixel index and p-th parameter index
    auto der = [psf, pixels, photons, pixelSize](size_t p, size_t i) {
        double scale = 1.0;
        if (p == 0) return 1.0;
        else if ((p == 2) || (p == 3)) scale = photons / pixelSize;
        else if (p == 4) scale = photons;
        return psf[4 * i + (p - 1)] * scale;
    };

    Matrix fisher(5, 5, 0.0);
    for (size_t i = 0; i < pixels; ++i) {
        // intensity of the molecule at the pixel k 
        const double I = photonFactor * (mol.peak * psf[4 * i] + mol.background) - offset * photonFactor;
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
    d->autoThreshold.reset();
    d->isSMLMImageReady.store(false);
    d->numberOfDetectedLocs.store(0);
    d->mols.clear();
    d->renderer.clear();
    d->imageWidth = 0;
    d->imageHeight = 0;
    d->frameFittingTimeMS = 0;
    d->timeoutMS = 250;
}

std::vector<Canidate> Controller::findCanidates(ImageU16 image, size_t windowSize, uint16_t threshold)
{
    LocalMaximumSearch nms(windowSize / 2, windowSize * 3 / 4);
    auto features = nms.find(image, threshold);
    std::vector<Canidate> result(features.size());
    auto it = result.begin();
    Rect bounds = image.rect();
    for (const auto &f : features) {
        Rect region(int(f.x) - windowSize / 2, int(f.y) - windowSize / 2, windowSize, windowSize);
        region.moveInside(bounds);
        it->localBg = f.localBg;
        it->val = f.val;
        it->roi = region;
        ++it;
    }
    return result;
}
