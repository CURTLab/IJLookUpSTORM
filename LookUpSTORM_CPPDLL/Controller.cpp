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

using namespace LookUpSTORM;

#ifdef CONTROLLER_STATIC
Controller* Controller::LOOKUPSTORM_INSTANCE = nullptr;
bool Controller::VERBOSE = false;
#endif // CONTROLLER_STATIC

Controller::Controller()
    : m_isLocFinished(false)
    , m_isSMLMImageReady(false)
    , m_nms(1, 6)
    , m_imageWidth(0)
    , m_imageHeight(0)
    , m_threshold(0)
{
    m_numberOfDetectedLocs.store(0);
}

Controller* Controller::inst()
{
    if (LOOKUPSTORM_INSTANCE == nullptr)
        LOOKUPSTORM_INSTANCE = new Controller;
    return LOOKUPSTORM_INSTANCE;
}

#ifdef CONTROLLER_STATIC
void Controller::release()
{
    delete LOOKUPSTORM_INSTANCE;
    LOOKUPSTORM_INSTANCE = nullptr;
}
#else
LookUpSTORM::Controller::~Controller()
{
}
#endif // CONTROLLER_STATIC

bool Controller::isReady() const
{
    return m_renderer.isReady() && m_fitter.isReady() && (m_imageWidth > 0) && (m_imageHeight > 0);
}

bool Controller::isLocFinished() const
{
    return m_isLocFinished.load();
}

bool Controller::isSMLMImageReady() const
{
    return m_isSMLMImageReady.load();
}

void Controller::clearSMLMImageReady()
{
    m_isSMLMImageReady.store(false);
}

void Controller::processImage(ImageU16 image, int frame)
{
    //while (!m_isLocFinished.load()) {}
    if (!isReady()) {
        if (Controller::VERBOSE)
            std::cerr << "LookUpSTORM: Image processor is not ready!";
        return;
    }

    m_isLocFinished.store(false);
    const auto t0 = std::chrono::high_resolution_clock::now();

    const size_t winSize = m_fitter.windowSize();
    m_nms.setRadius(winSize * 3 / 4);
    m_nms.setBorder(winSize / 2);

    const uint16_t threshold = m_threshold.load();

    auto features = m_nms.find(image, m_threshold);
    Molecule m;
    Rect bounds = image.rect();
    Rect changedRegion;
    m_detectedMolecues.clear();
    //std::cout << "Features: " << features.size() << std::endl;
    for (const auto& f : features) {

        auto t_start = std::chrono::high_resolution_clock::now();
        m.peak = std::max(0.0, double(f.val) - f.localBg);
        m.background = f.localBg;
        m.x = f.x;
        m.y = f.y;
        m.z = 0.0;

        //std::cout << "Feature: " << f.x << ", " << f.y << std::endl;

        Rect region(int(f.x) - winSize / 2, int(f.y) - winSize/2, winSize, winSize);
        if (!region.moveInside(bounds)) {
            std::cout << "LookUpSTORM: Impossible ROI!";
            continue;
        }

        //std::cout << "ROI: " << region.x() << ", " << region.y() << " " << region.width() << "x" << region.height() << std::endl;

        ImageU16 roi = image.subImage(region);

        const bool success = m_fitter.fitSingle(roi, m);
        auto t_end = std::chrono::high_resolution_clock::now();

        m.time = std::chrono::duration<double, std::micro>(t_end - t_start).count();

        if (success) {
            if (m.peak < threshold)
                continue;

            m.x += region.left();
            m.y += region.top();

            changedRegion.extendByPoint(m_renderer.map(m.x, m.y));
            m_renderer.set(m.x, m.y, m.z + 1E-6);

            m_detectedMolecues.push_back(m);
            m_mols.push_back(m);
        }
    }
    const auto t1 = std::chrono::high_resolution_clock::now();

    if (Controller::VERBOSE)
        std::cout << "Fitted " << m_detectedMolecues.size() << " emitter of frame " << frame << " in " << std::chrono::duration<double, std::milli>(t1 - t0).count() << " ms" << std::endl;

    if (!m_isSMLMImageReady && (changedRegion.area() > 25) && (frame > 1) && (frame % 5 == 0)) {
        m_renderer.updateImage();
        changedRegion = {};
        m_isSMLMImageReady = true;
    }

    m_numberOfDetectedLocs.store(static_cast<uint16_t>(m_detectedMolecues.size()));
    m_isLocFinished.store(true);
}

void Controller::setImageSize(int width, int height)
{
    m_imageWidth = width;
    m_imageHeight = height;
}

int Controller::imageWidth() const
{
    return m_imageWidth;
}

int Controller::imageHeight() const
{
    return m_imageHeight;
}

void Controller::setThreshold(uint16_t threshold)
{
    m_threshold.store(threshold);
}

uint16_t Controller::threshold() const
{
    return m_threshold.load();
}

std::list<Molecule>& Controller::detectedMolecues()
{
    return m_detectedMolecues;
}

std::list<Molecule>& Controller::allMolecues()
{
    return m_mols;
}

int32_t Controller::numberOfDetectedLocs()
{
    return m_numberOfDetectedLocs.load();
}

Fitter& Controller::fitter()
{
    return m_fitter;
}

Renderer& Controller::renderer()
{
    return m_renderer;
}

void Controller::setRenderScale(double scale)
{
    m_renderer.setSize(
        (int)std::ceil(m_imageWidth * scale),
        (int)std::ceil(m_imageHeight * scale),
        scale, scale);
}

void Controller::setRenderSize(int width, int height)
{
    m_renderer.setSize(width, height,
        double(width) / m_imageWidth, 
        double(height) / m_imageHeight);
}

ImageU32 Controller::renderSMLMImage()
{
    m_isSMLMImageReady.store(false);
    auto ret = m_renderer.render();
    m_isSMLMImageReady.store(true);
    return ret;
}

void Controller::reset()
{
    m_isLocFinished.store(false);
    m_isSMLMImageReady.store(false);
    m_numberOfDetectedLocs.store(0);
    m_mols.clear();
    m_renderer.clear();
}
