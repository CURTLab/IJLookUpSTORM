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

#include "AutoThreshold.h"

#include <vector>

using namespace LookUpSTORM;

AutoThreshold::AutoThreshold()
	: m_enabled(false)
	, m_minIntensity(MAX_INTENSITY)
	, m_maxIntensity(0.0)
	, m_hBin(2.0)
	, m_histogram{{0}}
{
}

AutoThreshold::~AutoThreshold()
{
}

bool AutoThreshold::isEnabled() const
{
	return m_enabled;
}

void AutoThreshold::setEnabled(bool enabled)
{
	m_enabled = enabled;
}

void AutoThreshold::addMolecule(const Molecule& mol)
{
	if (m_enabled && (mol.peak > 0) && (mol.peak < MAX_INTENSITY)) {
		m_minIntensity = std::min(m_minIntensity, mol.peak);
		m_maxIntensity = std::max(m_maxIntensity, mol.peak);
		const uint16_t bin = std::min<uint16_t>(std::floor(mol.peak / m_hBin), MAX_INTENSITY - 1);
		++m_histogram[bin];
	}
}

/****************************************************************************
 *
 * Algorithm from:
 * "Automatic Bayesian single molecule identification for localization
 * microscopy", Y. Tang et al., Scientific Reports, 2016
 *
 * BSD_License
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE FREEBSD PROJECT ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE FREEBSD PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 ****************************************************************************/
double AutoThreshold::calculateThreshold() const
{
	const uint16_t minIndex = std::min<uint16_t>(std::floor(m_minIntensity / m_hBin), MAX_INTENSITY - 1);
	const uint16_t maxIndex = std::min<uint16_t>(std::floor(m_maxIntensity / m_hBin), MAX_INTENSITY - 1);

	const uint16_t graylevel = maxIndex - minIndex + 1;

	const uint32_t* hist = m_histogram + minIndex;

	double minJ = std::numeric_limits<double>::max();

	double optimalThreshold = 0.0;
	double background_mean = 0.0;
	double object_mean = 0.0;

	// GaussianGaussianMET
	double Pb = 0.0;
	double Ps = 0.0;
	double mean_b1 = 0.0;
	double mean_s1 = 0.0;

	// Calculate accumulated histogram
	for (int T = 0; T < graylevel; ++T) {
		Ps += hist[T];
		mean_s1 += hist[T] * (T + 1);
	}

	for (int T = 0; T < graylevel; ++T) {
		// Compute the number of pixels in the two classes.
		const double h = hist[T];
		const double mean_h = hist[T] * (T + 1);
		Pb += h;
		Ps -= h;
		mean_b1 += mean_h;
		mean_s1 -= mean_h;

		// Only continue if both classes contain at least one pixel.
		if ((Pb > 0.0) && (Ps > 0.0)) {
			// Compute the mean and standard deviations of the classes.
			const double mean_b = mean_b1 / Pb;
			const double mean_s = mean_s1 / Ps;

			double variance_b = 0.0;
			for (int k = 0; k < T; ++k)
				variance_b += hist[k] * sqr((k + 1) - mean_b);
			variance_b /= Pb;

			double variance_s = 0.0;
			for (int k = T; k < graylevel; ++k)
				variance_s += hist[k] * sqr((k + 1) - mean_s);
			variance_s /= Ps;

			// Only compute the criterion function if both classes contain at
			// least two intensity values.
			if ((variance_b > 0.0) && (variance_s > 0.0)) {
				// Compute the criterion function.
				//const double J = 1.0 + log(pow(variance_b, Pb) * pow(variance_s, Ps)) - 2.0 * log(pow(Pb, Pb) * pow(Ps, Ps));
				const double J = 1.0 + (Pb * log(variance_b) + Ps * log(variance_s)) - 2.0 * (Pb * log(Pb) + Ps * log(Ps));
				if (J < minJ) {
					minJ = J;
					optimalThreshold = m_minIntensity + T * m_hBin;
					background_mean = m_minIntensity + mean_b * m_hBin;
					object_mean = m_minIntensity + mean_s * m_hBin;
				}
			}
		}
	}

	return optimalThreshold;
}

void AutoThreshold::reset()
{
	m_minIntensity = MAX_INTENSITY;
	m_maxIntensity = 0.0;
	std::fill_n(m_histogram, MAX_INTENSITY, 0u);
}
