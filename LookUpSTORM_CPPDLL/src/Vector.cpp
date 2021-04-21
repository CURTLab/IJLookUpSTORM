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

#include "Vector.h"

#include <atomic>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <iomanip>

namespace LookUpSTORM
{

class VectorData
{
public:
    inline VectorData(size_t s) noexcept
        : ref(0), size(s), stride(1), owner(true)
        , data(new double[s])
    {
        ++ref;
    }
    inline ~VectorData() noexcept { delete[] data; }

    std::atomic_int ref;
    size_t size;
    size_t stride;
    bool owner;
    double* data;
};

} // namespace LookUpSTORM

using namespace LookUpSTORM;

Vector::Vector() noexcept
    : d(nullptr)
{
}

Vector::Vector(size_t size) noexcept
    : d(new VectorData(size))
{
    std::fill_n(d->data, size, 0.0);
}

Vector::Vector(size_t size, Initialization init) noexcept
    : d(new VectorData(size))
{
}

Vector::Vector(size_t size, double value) noexcept
    : d(new VectorData(size))
{
    std::fill(d->data, d->data + size, value);
}

Vector::Vector(const std::initializer_list<double>& values) noexcept
    : d(new VectorData(values.size()))
{
    std::copy(values.begin(), values.end(), d->data);
}

Vector::Vector(const Vector& other) noexcept
    : d(other.d)
{
    if (d != nullptr)
        ++d->ref;
}

Vector::~Vector() noexcept
{
    if (d && !(--d->ref) && d->owner)
        delete d;
    d = nullptr;
}

Vector& Vector::operator=(const Vector& other) noexcept
{
    if (other.d != nullptr)
        ++other.d->ref;
    if ((d != nullptr) && !(--d->ref))
        delete d;
    d = other.d;
    return *this;
}

bool Vector::isNull() const noexcept
{
    return (d == nullptr) || (d->data == nullptr);
}

bool Vector::isZero() const noexcept
{
    return (d == nullptr) || (d->size == 0);
}

size_t Vector::size() const noexcept
{
    return d ? d->size : 0;
}

size_t Vector::stride() const noexcept
{
    return d ? d->stride : 0;
}

void Vector::fill(double value) noexcept
{
    if (d)
        std::fill(d->data, d->data + d->size, value);
}

void Vector::setZero() noexcept
{
    if (d)
        memset(d->data, 0, d->size * sizeof(double));
}

double Vector::sum() const
{
    if (!d) return 0.0;
    double ret = 0.0;
    for (size_t i = 0; i < d->size; ++i)
        ret += d->data[i];
    return ret;
}

double* Vector::data() noexcept
{
    return d ? d->data : nullptr;
}

const double* Vector::constData() const noexcept
{
    return d ? d->data : nullptr;
}

double& Vector::operator[](size_t i) noexcept
{
    return d->data[i];
}

const double& Vector::operator[](size_t i) const noexcept
{
    return d->data[i];
}

void Vector::operator+=(const Vector& v)
{
    if (!d || !v.d)
        throw std::runtime_error("vector == nullptr");
    if (v.d->size != d->size)
        throw std::runtime_error("size vector != size vector");
    for (size_t i = 0; i < d->size; ++i)
        d->data[i] += v.d->data[i];
}

void Vector::operator-=(const Vector& v)
{
    if (!d || !v.d)
        throw std::runtime_error("vector == nullptr");
    if (v.d->size != d->size)
        throw std::runtime_error("size vector != size vector");
    for (size_t i = 0; i < d->size; ++i)
        d->data[i] -= v.d->data[i];
}

std::ostream& operator<<(std::ostream& os, Vector const& v)
{
    if (v.isNull()) {
        os << "vec(null)";
    }
    else {
        os << "vec(" << v.size() << ")[";
        if (v.size() > 1)
            os << std::setprecision(3) << v[0];
        for (size_t i = 1; i < v.size(); ++i)
            os << "," << std::setprecision(3) << v[i];
        os << "]";
    }
    return os;
}
