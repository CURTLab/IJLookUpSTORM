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
#include <mutex>
#include <list>

namespace LookUpSTORM
{

class RendererPrivate;

class DLL_DEF_LUT Renderer final
{
public:
	Renderer();
	~Renderer();

	void release();

	// returns true if setSize and setSettings is called
	bool isReady() const;

	// these functions have to be called!
	void setSize(int width, int height, double scaleX, double scaleY);
	void setSettings(double minZ, double maxZ, double stepZ, float sigma);

	void setSigma(float sigma);
	int imageWidth() const;
	int imageHeight() const;

	// this method is thread safe
	void set(double x, double y, double z);

	std::pair<int, int> map(double x, double y) const;

	void setRenderImage(uint32_t* imagePtr, int width, int height, double scaleX, double scaleY);
	bool updateImage(Rect region = Rect());
	const uint32_t* renderImagePtr() const;

	void clear();

	const ImageF32 &rawImage() const;

	// render a molecule list with the possiblity of different projections
	static ImageU32 render(const std::list<Molecule>& mols, int width, int height,
		double scaleX, double scaleY, double minZ, double maxZ, double dZ, 
		double sigma = 1.0, Projection projection = Projection::TopDown);

private:
	RendererPrivate* const d;

};

} // namespace LookUpSTORM

#endif // !RENDERER_H

