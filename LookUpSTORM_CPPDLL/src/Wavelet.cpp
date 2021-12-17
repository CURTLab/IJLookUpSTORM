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

#include "Wavelet.h"

namespace LookUpSTORM
{

// Mainly calculates level 1 and level 2 wavelets. In addition, mean and sd is calcualted of the input image.
inline void waveletFilter(const ImageU16& input, ImageF32& padded, ImageF32& result, float &mean, float &sd)
{
	if ((input.width() != result.width()) || (input.height() != result.height()))
		return;

	const int w0 = input.width();
	const int h0 = input.height();
	const int s0 = input.stride();

	const int s1 = padded.stride();

	mean = 0.f;
	sd = 0.f;

	// pad the image with a 4 pixel reflected boarder
	const uint16_t* src = input.constData();
	float* dst = padded.scanLine(4) + 4;
	for (int y = 0; y < h0; ++y, dst += (s1 - s0)) {
		//std::copy_n(src, w0, dst);
		for (int x = 0; x < w0; ++x) {
			mean += *src;
			*dst++ = *src++;
		}
	}
	mean /= w0 * h0;

	// top pad (y-axis padding)
	src = input.scanLine(4);
	dst = padded.data() + 4;
	for (int y = 0; y < 4; ++y, dst += s1, src -= s0)
		std::copy_n(src, w0, dst);

	// bottom pad
	src = input.scanLine(h0 - 2);
	dst = padded.scanLine(h0 + 4) + 4;
	for (int y = 0; y < 4; ++y, dst += s1, src -= s0)
		std::copy_n(src, w0, dst);

	// x-axis padding
	for (int y = 0; y < h0; ++y) {
		// left pad
		int srcX = 8, dstX;
		for (int x = 0; x < 4; ++x, --srcX)
			padded(x, y) = padded(srcX, y);

		// right pad
		dstX = 4 + w0;
		srcX = 2 + w0;
		for (int x = 0; x < 4; ++x, --srcX, ++dstX)
			padded(dstX, y) = padded(srcX, y);
	}

	// Algorithm from: Izeddin et al., "Wavelet analysis for single molecule localization microscopy", 2012
	// g1 = [1/16,1/4,3/8,1/4,1/16], g2 = [1/16,0,1/4,0,3/8,0,1/4,0,1/16]
	dst = result.data();
	const uint16_t* src1 = input.constData();
	for (int y = 0; y < h0; ++y) {
		for (int x = 0; x < w0; ++x, ++src1) {
			float val1 = 0.f, val2 = 0.f;
			const auto* src = padded.constData() + (y + 2) * s1 + (x + 2);
			val1 += 0.00390625f * src[0] + 0.015625f * src[1] + 0.0234375f * src[2] + 0.015625f * src[3] + 0.00390625f * src[4]; src += s1;
			val1 += 0.015625f * src[0] + 0.0625f * src[1] + 0.09375f * src[2] + 0.0625f * src[3] + 0.015625f * src[4]; src += s1;
			val1 += 0.0234375f * src[0] + 0.09375f * src[1] + 0.140625f * src[2] + 0.09375f * src[3] + 0.0234375f * src[4]; src += s1;
			val1 += 0.015625f * src[0] + 0.0625f * src[1] + 0.09375f * src[2] + 0.0625f * src[3] + 0.015625f * src[4]; src += s1;
			val1 += 0.00390625f * src[0] + 0.015625f * src[1] + 0.0234375f * src[2] + 0.015625f * src[3] + 0.00390625f * src[4];

			src = padded.constData() + y * s1 + x;
			val2 += 0.00390625f * src[0] + 0.015625f * src[2] + 0.0234375f * src[4] + 0.015625f * src[6] + 0.00390625f * src[8]; src += 2 * s1;
			val2 += 0.015625f * src[0] + 0.0625f * src[2] + 0.09375f * src[4] + 0.0625f * src[6] + 0.015625f * src[8]; src += 2 * s1;
			val2 += 0.0234375f * src[0] + 0.09375f * src[2] + 0.140625f * src[4] + 0.09375f * src[6] + 0.0234375f * src[8]; src += 2 * s1;
			val2 += 0.015625f * src[0] + 0.0625f * src[2] + 0.09375f * src[4] + 0.0625f * src[6] + 0.015625f * src[8]; src += 2 * s1;
			val2 += 0.00390625f * src[0] + 0.015625f * src[2] + 0.0234375f * src[4] + 0.015625f * src[6] + 0.00390625f * src[8];

			*dst++ = val1 - val2;

			sd += (*src1 - mean) * (*src1 - mean);
		}
	}

	// divide sd by w0*h0 because the samples are from the complete population of the frame
	sd /= (w0 * h0);
}

}

using namespace LookUpSTORM;

Wavelet::Wavelet()
	: m_mean(0.f), m_sd(0.f)
{
}

Wavelet::Wavelet(int width, int height)
	: m_padded(width + 8, height + 8)
	, m_result(width, height)
	, m_mean(0.f), m_sd(0.f)
{
}

void Wavelet::setSize(int width, int height)
{
	if ((width == m_result.width()) && (height  == m_result.height()))
		return;
	// allocate new buffers
	m_padded = ImageF32(width + 8, height + 8);
	m_result = ImageF32(width, height);
}

const ImageF32& Wavelet::filter(const ImageU16& input)
{
	waveletFilter(input, m_padded, m_result, m_mean, m_sd);
	return m_result;
}

const float Wavelet::inputMean() const
{
	return m_mean;
}

const float Wavelet::inputSD() const
{
	return m_sd;
}

const float Wavelet::inputSTD() const
{
	return std::sqrt(m_sd);
}

ImageF32 LookUpSTORM::waveletFilter(const ImageU16& input)
{
	float mean, sd;
	ImageF32 padded(input.width() + 8, input.height() + 8);
	ImageF32 ret(input.width(), input.height());
	waveletFilter(input, padded, ret, mean, sd);
	return ret;
}
