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

#ifndef IMAGE_H
#define IMAGE_H

#include <cstdint>
#include "Rect.h"

namespace LookUpSTORM
{

template<class T>
class ImageData;

template<class T>
class Image;

/*
 * class Image<T>
 * Template based image class with copy-on-write mechanics
 */
template<class T>
class DLL_DEF_LUT Image
{
public:
	Image();
	Image(int width, int height);
	Image(int width, int height, const T value);
	Image(int width, int height, T *data, bool copy);
	Image(const Image& image);

	~Image();

	void fill(const T& val);

	Image subImage(Rect region) const;

	Image& operator=(const Image& image);

	bool isNull() const;
	int width() const;
	int height() const;
	int stride() const;
	Rect rect() const;

	const T& operator[](size_t i) const;
	const T& operator()(int x, int y) const;
	T& operator[](size_t i);
	T& operator()(int x, int y);

	bool pixel(size_t i, T& val) const;
	bool pixel(int x, int y, T& val) const;

	bool setPixel(size_t i, const T& val);
	bool setPixel(int x, int y, const T& val);

	size_t allocatedBytes() const;

	T* data();
	const T* constData() const;

	// return pointer without null check!
	T* scanLine(int line);
	const T* scanLine(int line) const;
	T* ptr(int x, int y);
	const T* ptr(int x, int y) const;

private:
	ImageData<T>* d;

};

#define HANDLE(T,ext) \
using Image##ext = Image<T>;

HANDLE(uint16_t,U16)
HANDLE(uint32_t, U32)
HANDLE(float, F32)
HANDLE(double, F64)

} // namespace LookUpSTORM

#endif // !IMAGE_H
