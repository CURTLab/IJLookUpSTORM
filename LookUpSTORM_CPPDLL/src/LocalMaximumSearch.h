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

#ifndef LOCALMAXIMUMSEARCH_H
#define LOCALMAXIMUMSEARCH_H

#include <list>
#include "Image.h"

namespace LookUpSTORM
{

struct LocalMaximum {
	uint16_t val;
	uint16_t localBg;
	int x;
	int y;
};

class LocalMaximumSearch
{
public:
	LocalMaximumSearch(int border, int radius);

	std::list<LocalMaximum> find(ImageU16 image, uint16_t threshold);

	std::list<LocalMaximum> find(const ImageU16 &image, const ImageF32 &filteredImage, float filterThreshold);

	std::list<LocalMaximum> findAll(const ImageU16& image);

	int border() const;
	void setBorder(int border);

	int radius() const;
	void setRadius(int radius);

private:
	int m_border;
	int m_radius;

};

} // namespace LookUpSTORM

#endif // !LOCALMAXIMUMSEARCH_H
