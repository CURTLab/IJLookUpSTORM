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

#ifndef RENDERER_H
#define RENDERER_H

#include "Image.h"
#include "ColorMap.h"
#include <mutex>

class Renderer
{
public:
	Renderer();
	Renderer(int width, int height, double scaleX, double scaleY);

	void release();

	using OutType = uint32_t;

	bool isReady() const;

	void setSize(int width, int height, double scaleX, double scaleY);
	void setAxialRange(double min, double max);
	void setSigma(float sigma);
	int imageWidth() const;
	int imageHeight() const;

	// this method is thread safe
	void set(double x, double y, double z);

	std::pair<int, int> map(double x, double y) const;

	Image<OutType> render();
	void setRenderImage(OutType* imagePtr, int width, int height, double scaleX, double scaleY);
	bool updateImage(Rect region = Rect());
	const OutType* renderImagePtr() const;

	void clear();

	const ImageF32 &rawImage() const;

private:
	void render(const ImageF32 &in, Image<OutType> &out) const;
	void renderTile(const Rect& tile, const ImageF32& in, Image<OutType>& image) const;
	OutType pixel(const ImageF32& in, int x, int y) const;

	ImageF32 m_image;
	Image<OutType> m_renderImage;

	float m_corner;
	float m_cross;
	double m_scaleX;
	double m_scaleY;
	ColorMap m_cmap;
	std::mutex m_mutex;

};

#endif // !RENDERER_H

