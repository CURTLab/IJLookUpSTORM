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

#ifndef AUTOTHRESHOLD_H
#define AUTOTHRESHOLD_H

#include <list>

#include "Common.h"

namespace LookUpSTORM
{

class AutoThreshold
{
public:
	AutoThreshold();
	~AutoThreshold();

	bool isEnabled() const;
	void setEnabled(bool enabled);

	void addMolecule(const Molecule& mol, uint16_t frameMaxIntensity = 0);

	double calculateThreshold() const;

	void reset();

	static constexpr const uint16_t MAX_PEAK = 4000;
	//static constexpr uint16_t MAX_PEAK = MAX_INTENSITY;

private:
	bool m_enabled;
	double m_minIntensity;
	double m_maxIntensity;
	double m_hBin;
	uint32_t m_histogram[MAX_PEAK];

};

} // namespace LookUpSTORM

#endif // AUTOTHRESHOLD_H
