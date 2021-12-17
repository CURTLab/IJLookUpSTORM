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

#include "LocalMaximumSearch.h"

#include <functional>

namespace LookUpSTORM
{

template<class T>
inline T localBackground(const T* data, int x, int y, int w, int h, int W, int H, int stride)
{
	T ret = 0;
	int n = 0;
	auto inc = [&ret, &n, &data, &W, &H, &stride](int x, int y) {
		if (x >= 0 && y >= 0 && x < W && y < H) {
			ret += data[x + y * stride];
			++n;
		}
	};

	if (w == h) {
		for (int i = 0; i < w; ++i) {
			inc(x + i, y);
			inc(x + i, y + h - 1);
			inc(x, y + i);
			inc(x + w - 1, y + i);
		}
	}
	else {
		for (int i = 0; i < w; ++i) {
			inc(x + i, y);
			inc(x + i, y + h - 1);
			ret += data[x + i + y * stride];
			ret += data[x + i + (y + h - 1) * stride];
		}
		for (int i = 1; i < (h - 1); ++i) {
			inc(x, y + i);
			inc(x + w - 1, y + i);
		}
	}

	return (n == 0 ? 0 : ret / n);
}

template<class T>
inline uint16_t centerMean(const T* data, int x, int y, int W, int H, int stride)
{
	T ret{ 0 };
	int n = 0;

	auto inc = [&ret, &n, &data, &W, &H, &stride](int x, int y) {
		if (x >= 0 && y >= 0 && x < W && y < H) {
			ret += data[x + y * stride];
			++n;
		}
	};

	inc(x, y);
	inc(x + 1, y);
	inc(x, y + 1);
	inc(x - 1, y);
	inc(x, y - 1);

	return (n == 0 ? 0 : ret / n);
}

struct greater
{
	bool operator()(const LocalMaximum& f1, const LocalMaximum& f2) { return f1.val > f2.val; }
};

template<class T>
void nms(const Image<T>& image, int r, int b, std::function<void(T, int, int)> maxima)
{
	const int bg_radius = r + 1;

	if (image.width() <= (2 * bg_radius + 2 * b + 1) || image.height() <= (2 * bg_radius + 2 * b + 1))
		return;

	const int w = image.width() - (b + 1);
	const int h = image.height() - (b + 1);

	T value, canidate;

	// A. Neubeck et.al., 'Efficient Non-MaximumSuppression', 2006, (2n+1)×(2n+1)-Block Algorithm
	// with sorted insert into a list
	for (int i = b; i < w; i += (r + 1)) {
		for (int j = b; j < h; j += (r + 1)) {
			int mi = i;
			int mj = j;
			for (int i2 = i; i2 <= i + r; ++i2) {
				for (int j2 = j; j2 <= j + r; ++j2) {
					if (!image.pixel(i2, j2, value))
						continue;
					if (value > image(mi, mj)) {
						mi = i2;
						mj = j2;
					}
				}
			}

			canidate = image(mi, mj);
			for (int i2 = mi - r; i2 <= mi + r; ++i2) {
				for (int j2 = mj - r; j2 <= mj + r; ++j2) {
					if (!image.pixel(i2, j2, value))
						continue;
					if (value > canidate)
						goto failed;
				}
			}

			maxima(canidate, mi, mj);

		failed:;
		}
	}
}

}

using namespace LookUpSTORM;

LocalMaximumSearch::LocalMaximumSearch(int border, int radius)
    : m_border(border), m_radius(radius)
{
}

std::list<LocalMaximum> LocalMaximumSearch::find(ImageU16 image, uint16_t threshold)
{
    std::list<LocalMaximum> features;
	const int bg_radius = m_radius + 1;
	nms<uint16_t>(image, m_radius, m_border, 
		[&](uint16_t canidate, int x, int y) {
			uint16_t localBg = localBackground(image.constData(), x - 1, y - 1, bg_radius, bg_radius, image.width(), image.height(), image.stride());
			uint16_t mean = centerMean(image.constData(), x, y, image.width(), image.height(), image.stride());
			if ((canidate - localBg) < threshold || (mean - localBg) < threshold)
				return;

			LocalMaximum f = { canidate, localBg, x, y };
			features.insert(std::lower_bound(features.begin(), features.end(), f, greater()), f);
		});

    return features;
}

std::list<LocalMaximum> LocalMaximumSearch::find(const ImageU16& image, const ImageF32& filteredImage, float filterThreshold)
{
	std::list<LocalMaximum> features;

	const int bg_radius = m_radius + 1;
	nms<float>(filteredImage, m_radius, m_border, 
		[&](float canidate, int x, int y) {

			if (canidate < filterThreshold)
				return;

			uint16_t found = image(x, y);
			uint16_t localBg = localBackground(image.constData(), x - 1, y - 1, bg_radius, bg_radius, image.width(), image.height(), image.stride());

			LocalMaximum f = { found, localBg, x, y };
			features.insert(std::lower_bound(features.begin(), features.end(), f, greater()), f);
		});

	return features;
}

std::list<LocalMaximum> LookUpSTORM::LocalMaximumSearch::findAll(const ImageU16& image)
{
	std::list<LocalMaximum> features;
	const int bg_radius = m_radius + 1;
	nms<uint16_t>(image, m_radius, m_border, 
		[&](uint16_t canidate, int x, int y) {
			uint16_t localBg = localBackground(image.constData(), x - 1, y - 1, bg_radius, bg_radius, image.width(), image.height(), image.stride());
			features.push_back({ canidate, localBg, x, y });
		});

	return features;
}

int LocalMaximumSearch::border() const
{
    return m_border;
}

void LocalMaximumSearch::setBorder(int border)
{
    m_border = border;
}

int LocalMaximumSearch::radius() const
{
    return m_radius;
}

void LocalMaximumSearch::setRadius(int radius)
{
    m_radius = radius;
}