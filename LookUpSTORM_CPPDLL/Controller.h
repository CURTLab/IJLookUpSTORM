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

#ifndef CONTROLLER_H
#define CONTROLLER_H
 
#include <atomic>
#include "Fitter.h"
#include "Renderer.h"

namespace LookUpSTORM
{

class DLL_DEF_LUT Controller
{
public:
#ifdef JNI_EXPORT_LUT
	static Controller* inst();
	static void release();
#else
	virtual ~Controller();
#endif

	bool isReady() const;

	// thread-safe
	bool isLocFinished() const;

	// thread-safe
	bool isSMLMImageReady() const;

	void clearSMLMImageReady();

	bool processImage(ImageU16 image, int frame);

	void setImageSize(int width, int height);
	int imageWidth() const;
	int imageHeight() const;

	// thread-safe
	void setThreshold(uint16_t threshold);
	// thread-safe
	uint16_t threshold() const;

	// thread-safe
	void setVerbose(bool verbose);
	// thread-safe
	bool isVerbose() const;

	// thread-safe
	void setFrameRenderUpdateRate(int rate);
	// thread-safe
	int frameRenderUpdateRate() const;

	// thread-safe
	void setTimeoutMS(double timeoutMS);
	// thread-safe
	double timeoutMS() const;

	Fitter& fitter();
	// get detected localization from the last processImage call
	std::list<Molecule>& detectedMolecues();
	std::list<Molecule>& allMolecues();

	// get number of detected localization from the last processImage call
	// thread-safe
	int32_t numberOfDetectedLocs();

	// get the time needed to fit a frame provieded by processImage in ms
	// thread-safe
	double frameFittingTimeMS() const;

	// get the time needed to render a SMLM image provieded by processImage in ms
	// thread-safe
	double renderTimeMS() const;

	// thread-safe
	void setRenderingEnabled(bool enabled);
	// thread-safe
	bool isRenderingEnabled() const;

	Renderer& renderer();
	void setRenderScale(double scale);
	void setRenderSize(int width, int height);

	void reset();

#ifdef JNI_EXPORT_LUT
private:
#endif // CONTROLLER_STATIC
	Controller();

private:
#ifdef JNI_EXPORT_LUT
	static Controller* LOOKUPSTORM_INSTANCE;
#endif // CONTROLLER_STATIC

	std::atomic<bool> m_isLocFinished;
	std::atomic<bool> m_isSMLMImageReady;
	LocalMaximumSearch m_nms;
	int m_imageWidth;
	int m_imageHeight;
	std::atomic<uint16_t> m_threshold;
	Fitter m_fitter;
	std::list<Molecule> m_detectedMolecues;
	std::atomic<int32_t> m_numberOfDetectedLocs;
	std::atomic<double> m_frameFittingTimeMS;
	std::atomic<double> m_renderTimeMS;
	std::atomic<int> m_renderUpdateRate;
	std::atomic<bool> m_enableRendering;
	std::atomic<double> m_timeoutMS;
	std::list<Molecule> m_mols;
	Renderer m_renderer;
	std::atomic<bool> m_verbose;

};

} // namespace LookUpSTORM

#endif // !CONTROLLER_H