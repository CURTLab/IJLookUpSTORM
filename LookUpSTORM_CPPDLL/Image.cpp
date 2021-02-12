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

#include "Image.h"

#include <atomic>

namespace LookUpSTORM
{

template<class T>
class ImageData
{
public:
    inline ImageData(int w, int h, Initialization init)
        : ref(0), width(w), height(h), stride(w)
        , owner(true)
        , data(nullptr)
    {
        ++ref;
    }
    inline ImageData(int w, int h)
        : ref(0), width(w), height(h), stride(w)
        , owner(true)
        , data(new T[size_t(w) * size_t(h)])
    {
        ++ref;
    }
    inline ImageData(int w, int h, T *data, bool copy)
        : ref(0), width(w), height(h), stride(w)
        , owner(copy)
        , data(copy ? new T[size_t(w) * size_t(h)] : data)
    {
        if (copy)
            std::copy(data, data + (size_t(w) * h), this->data);
        ++ref;
    }
    inline ~ImageData() { 
        if (owner)
            delete[] data; 
    }
    inline constexpr size_t pixels() const { return size_t(width) * height; }
    inline constexpr size_t indexWithStride(size_t i) {
        const size_t y = i / width;
        const size_t x = i - y * width;
        return stride * y + x;
    }

    std::atomic_int ref;
    int width;
    int height;
    size_t stride;
    bool owner;
    T* data;

    static auto constexpr bytes_pixel = sizeof(T);
};

} // namespace LookUpSTORM

using namespace LookUpSTORM;

template<class T>
Image<T>::Image() : d(nullptr)
{
}

template<class T>
Image<T>::Image(int width, int height)
    : d(new ImageData<T>(width, height))
{
}

template<class T>
Image<T>::Image(int width, int height, T* data, bool copy)
    : d(new ImageData<T>(width, height, data, copy))
{
}

template<class T>
Image<T>::Image(const Image<T>& image)
    : d(image.d)
{
    if (d != nullptr)
        ++d->ref;
}

template<class T>
Image<T>::~Image()
{
    if (d && !(--d->ref))
        delete d;
    d = nullptr;
}

template<class T>
void Image<T>::fill(const T& val)
{
    if (!d)
        return;

    if (d->width == d->stride) {
        // fill full image
        std::fill_n(d->data, d->pixels(), val);
    } else {
        // fill sub image
        T* line = d->data;
        for (int y = 0; y < d->height; ++y, line += d->stride)
            std::fill_n(line, d->width, val);
    }
}

template<class T>
Image<T> Image<T>::subImage(Rect region) const
{
    if (!d || region.isNull() || !rect().fullyContains(region))
        return {};
    Image<T> ret;
    ret.d = new ImageData<T>(region.width(), region.height(), Uninitialized);
    ret.d->data = d->data + (d->stride * region.y() + region.x());
    ret.d->stride = d->stride;
    ret.d->owner = false;
    return ret;
}

template<class T>
Image<T>& Image<T>::operator=(const Image<T>& image)
{
    if (image.d != nullptr)
        ++image.d->ref;
    if ((d != nullptr) && !(--d->ref))
        delete d;
    d = image.d;
    return *this;
}

template<class T>
bool Image<T>::isNull() const
{
    return !d || (d->width == 0) || (d->height == 0);
}

template<class T>
int Image<T>::width() const
{
    return d ? d->width : 0;
}

template<class T>
int Image<T>::height() const
{
    return d ? d->height : 0;
}

template<class T>
int Image<T>::stride() const
{
    return d ? d->stride : 0;
}

template<class T>
Rect Image<T>::rect() const
{
    return d ? Rect(0, 0, d->width, d->height) : Rect();
}

template<class T>
const T& Image<T>::operator[](size_t i) const
{
    return (d->width == d->stride) ? d->data[i] : d->data[d->indexWithStride(i)];
}

template<class T>
const T& Image<T>::operator()(int x, int y) const
{
    return d->data[d->stride * y + x];
}

template<class T>
T& Image<T>::operator[](size_t i)
{
    return (d->width == d->stride) ? d->data[i] : d->data[d->indexWithStride(i)];
}

template<class T>
T& Image<T>::operator()(int x, int y)
{
    return d->data[d->stride * y + x];
}

template<class T>
bool Image<T>::pixel(size_t i, T& val) const
{
    if (!d || (i > (size_t(d->width) * d->height)))
        return false;
    val = (d->width == d->stride) ? d->data[i] : d->data[d->indexWithStride(i)];
    return true;
}

template<class T>
bool Image<T>::pixel(int x, int y, T& val) const
{
    if (!d || (x < 0) || (y < 0) || (x >= d->width) || (y >= d->height))
        return false;
    val = d->data[d->stride * y + x];
    return true;
}

template<class T>
size_t Image<T>::allocatedBytes() const
{
    return (d && d->owner) ? (d->bytes_pixel * d->width * d->height) : 0;
}

template<class T>
T* Image<T>::data()
{
    return d ? d->data : nullptr;
}

template<class T>
const T* Image<T>::constData() const
{
    return d ? d->data : nullptr;
}

template<class T>
T* Image<T>::scanLine(int line)
{
    return d->data + (d->stride * line);
}

template<class T>
const T* Image<T>::scanLine(int line) const
{
    return d->data + (d->stride * line);
}

template<class T>
T* LookUpSTORM::Image<T>::ptr(int x, int y)
{
    return d->data + (d->stride * y + x);
}

template<class T>
const T* Image<T>::ptr(int x, int y) const
{
    return d->data + (d->stride * y + x);
}

#define DECLARE(T) \
template class Image<T>;

DECLARE(uint16_t)
DECLARE(uint32_t)
DECLARE(float)
DECLARE(double)