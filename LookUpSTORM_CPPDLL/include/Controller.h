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
#include <list>
#include <functional>
#include "Fitter.h"
#include "Renderer.h"
#include "Calibration.h"

namespace LookUpSTORM
{

class ControllerPrivate;
class LUT;

struct Canidate {
	uint16_t val;
	uint16_t localBg;
	Rect roi;
};

class DLL_DEF_LUT Controller final
{
public:
#ifdef JNI_EXPORT_LUT
	static Controller* inst();
	static void release();
#endif
	~Controller();

	bool isReady() const;

	// generate LUT from an object derived from the LUT class
	// the callback function can be used to show the progress (current index, max index)
	bool generate(LUT& lut, size_t windowSize,
		double dLat, double dAx, double rangeLat, double rangeAx,
		std::function<void(size_t index, size_t max)> callback = [](size_t,size_t){}
	);

	// generate astigmatism LUT from calibration
	// the callback function can be used to show the progress (current index, max index)
	bool generateFromCalibration(const Calibration& cali, size_t windowSize,
		double dLat, double dAx, double rangeLat, double rangeAx,
		std::function<void(size_t index, size_t max)> callback = [](size_t, size_t) {}
	);

	// set the internal lookup table from the generated table of the LUT class 
	bool setLUT(const LUT& lut);

	// thread-safe
	bool isSMLMImageReady() const;

	void clearSMLMImageReady();

	// fit the image provided
	bool processImage(ImageU16 image, int frame);

	void setImageSize(int width, int height);
	int imageWidth() const;
	int imageHeight() const;

	// thread-safe, renders the localiation imaged each nth frame (see setFrameRenderUpdateRate)
	// and if rendering is enabled (see setRenderingEnabled, default is true)
	// returns true if image was updated
	bool updateRenderer(int frame);

	// thread-safe, renders the localiation imaged each nth frame (see setFrameRenderUpdateRate)
	// and if rendering is enabled (see setRenderingEnabled, default is true)
	// returns true if image was updated
	bool renderToImage(ImageU32 image, int frame);

	// thread-safe, updates the threshold each nth frame (see setAutoThresholdUpdateRate)
	// and if auto threshold is enabled (see setAutoThresholdEnabled, default is false)
	// returns true if threshold was updated
	bool updateAutoThreshold(int frame);

	// thread-safe
	void setThreshold(uint16_t threshold);
	// thread-safe
	uint16_t threshold() const;

	bool isAutoThresholdEnabled() const;
	void setAutoThresholdEnabled(bool enabled);

	// thread-safe
	void setAutoThresholdUpdateRate(int rate);
	// thread-safe
	int autoThresholdUpdateRate() const;

	// thread-safe
	void setWaveletFilterEnabled(bool enabled);
	// thread-safe
	bool isWaveletFilterEnabled() const;

	// wavelet threshold factor (typical values 0-2, default is 1.25) for standard derivation of input image
	void setWaveletFactor(float factor);
	float waveletFactor() const;

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
	const Fitter& fitter() const;
	// get detected localization from the last processImage call
	std::list<Molecule>& detectedMolecues();
	std::list<Molecule>& allMolecues();

	// get number of detected localization from the last processImage call
	// thread-safe
	int32_t numberOfDetectedLocs();

	// get the time needed to fit a frame provieded by processImage in ms
	// thread-safe
	double frameFittingTimeMS() const;

	// get the time needed to fit a frame provieded by processImage in ms
	// thread-safe
	Milliseconds frameFittingTime() const;

	// get the time needed to render a SMLM image provieded by processImage in ms
	// thread-safe
	double renderTimeMS() const;

	// thread-safe
	void setRenderingEnabled(bool enabled);
	// thread-safe
	bool isRenderingEnabled() const;

	// renderer helper
	Renderer& renderer();
	const Renderer& renderer() const;
	void setRenderScale(double scale);
	void setRenderSize(int width, int height);

	// calculate the number of photons of a fitted molecule base on the EM CCD parameters 
	// ADU (Camera ADC count to photons) and EM-Gain
	double calculatePhotons(const Molecule& mol, const double adu, const double gain) const;

	// calculate the Cram�r�Rao lower bound of a fitted molecule base on the EM CCD 
	// parameters ADU (Camera ADC count to photons) and EM-Gain and the final pixel size in nm
	bool calculateCRLB(const Molecule &mol, double *crlb, const double adu, 
		const double gain, const double offset, const double pixelSize) const;

	// restore intial conditions to fit a new image
	void reset();

	// find canidates in an image using non-maximum suppression
	static std::vector<Canidate> findCanidates(ImageU16 image, size_t windowSize, uint16_t threshold);

#ifdef JNI_EXPORT_LUT
private:
#endif // CONTROLLER_STATIC
	Controller();

private:
	ControllerPrivate* const d;
#ifdef JNI_EXPORT_LUT
	static Controller* LOOKUPSTORM_INSTANCE;
#endif // CONTROLLER_STATIC

};

} // namespace LookUpSTORM

#endif // !CONTROLLER_H