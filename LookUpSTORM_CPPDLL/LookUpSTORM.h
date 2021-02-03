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

#ifndef LOOKUPSTORM_H
#define LOOKUPSTORM_H
 
#include <atomic>
#include "Fitter.h"
#include "Renderer.h"

class LookUpSTORM
{
public:
	static LookUpSTORM* inst();
	static void release();
	static bool VERBOSE;

	bool isReady() const;

	// thread-safe
	bool isLocFinished() const;

	// thread-safe
	bool isSMLMImageReady() const;

	void clearSMLMImageReady();

	void processImage(ImageU16 image, int frame);

	void setImageSize(int width, int height);
	int imageWidth() const;
	int imageHeight() const;

	// thread-safe
	void setThreshold(uint16_t threshold);
	// thread-safe
	uint16_t threshold() const;

	Fitter& fitter();
	// get detected localization from the last processImage call
	std::list<Molecule>& detectedMolecues();
	std::list<Molecule>& allMolecues();

	// get number of detected localization from the last processImage call
	// thread-safe
	int32_t numberOfDetectedLocs();

	Renderer& renderer();
	void setRenderScale(double scale);
	void setRenderSize(int width, int height);
	ImageU32 renderSMLMImage();

	void reset();

private:
	LookUpSTORM();

	std::atomic<bool> m_isLocFinished;
	std::atomic<bool> m_isSMLMImageReady;
	LocalMaximumSearch m_nms;
	int m_imageWidth;
	int m_imageHeight;
	std::atomic<uint16_t> m_threshold;
	Fitter m_fitter;
	std::list<Molecule> m_detectedMolecues;
	std::atomic<int32_t> m_numberOfDetectedLocs;
	std::list<Molecule> m_mols;
	Renderer m_renderer;

	static LookUpSTORM* LOOKUPSTORM_INSTANCE;
};

#endif // !LOOKUPSTORM_H