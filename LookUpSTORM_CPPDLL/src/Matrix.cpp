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

#include "Matrix.h"

#include <atomic>
#include <algorithm>
#include <functional>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <stdexcept>
#include <cassert>

namespace LookUpSTORM
{

class MatrixData
{
public:
    inline MatrixData(size_t s1, size_t s2) noexcept
        : ref(0), size1(s1), size2(s2), tda(s2), owner(true)
        , data(new double[s1*s2])
    {
        ++ref;
    }
    inline ~MatrixData() noexcept { delete[] data; }

    inline void setElements(std::function<double(size_t, size_t)> fun) {
        for (size_t i = 0; i < size1; ++i) {
            for (size_t j = 0; j < size2; ++j)
                data[i * tda + j] = fun(i, j);
        }
    }

    inline void getElements(std::function<void(size_t, size_t, double)> fun) {
        for (size_t i = 0; i < size1; ++i) {
            for (size_t j = 0; j < size2; ++j)
                fun(i, j, data[i * tda + j]);
        }
    }

    inline constexpr size_t size() const { return size1 * size2; }

    std::atomic_int ref;
    size_t size1;
    size_t size2;
    size_t tda;
    bool owner;
    double* data;
};

} //  namespace LookUpSTORM

using namespace LookUpSTORM;

Matrix::Matrix() noexcept
    : d(nullptr)
{
}

Matrix::Matrix(size_t size1, size_t size2)
    : d(new MatrixData(size1, size2))
{
    std::fill_n(d->data, d->size(), 0.0);
}

Matrix::Matrix(size_t size1, size_t size2, Initialization init) noexcept
    : d(new MatrixData(size1, size2))
{
}

Matrix::Matrix(size_t size1, size_t size2, double value) noexcept
    : d(new MatrixData(size1, size2))
{
    std::fill_n(d->data, d->size(), value);
}

LookUpSTORM::Matrix::Matrix(size_t size1, size_t size2, const std::initializer_list<double>& values)
    : d(new MatrixData(size1, size2))
{
    if (size1 * size2 != values.size())
        throw std::runtime_error("number of value list items is not equal to the supplied matrix size!");
    std::copy_n(values.begin(), values.size(), d->data);
}

Matrix::Matrix(const Matrix& other) noexcept
    : d(other.d)
{
    if (d != nullptr)
        ++d->ref;
}

Matrix::~Matrix() noexcept
{
    if (d && !(--d->ref) && d->owner)
        delete d;
    d = nullptr;
}

Matrix& Matrix::operator=(const Matrix& other) noexcept
{
    if (other.d != nullptr)
        ++other.d->ref;
    if ((d != nullptr) && !(--d->ref))
        delete d;
    d = other.d;
    return *this;
}

bool Matrix::isNull() const noexcept
{
    return (d == nullptr) || (d->data == nullptr);
}

bool Matrix::isZero() const noexcept
{
    return (d == nullptr) || (d->size1 == 0) || (d->size2 == 0);
}

size_t Matrix::size1() const noexcept
{
    return d ? d->size1 : 0;
}

size_t Matrix::size2() const noexcept
{
    return d ? d->size2 : 0;
}

size_t Matrix::tda() const noexcept
{
    return d ? d->tda : 0;
}

void Matrix::fill(double value) noexcept
{
    if (!d)
        return;
    if (d->tda == d->size2)
        std::fill_n(d->data, d->size(), value);
    else
        d->setElements([value](size_t, size_t) { return value; });
}

void Matrix::setZero() noexcept
{
    if (!d)
        return;
    if (d->tda == d->size2)
        memset(d->data, 0, d->size() * sizeof(double));
    else
        d->setElements([](size_t, size_t) { return 0.0; });
}

void Matrix::setIdentity() noexcept
{
    if (!d)
        return;
    d->setElements([](size_t i, size_t j) { return (i == j) ? 1.0 : 0.0; });
}

double Matrix::sum() const
{
    if (!d) 
        return 0.0;
    double sum = 0.0;
    if (d->tda == d->size2)
        sum = std::accumulate(d->data, d->data + d->size(), sum);
    else
        d->getElements([&sum](size_t, size_t, double v) { sum += v; });
    return sum;
}

double* Matrix::data() noexcept
{
    return d ? d->data : nullptr;
}

const double* Matrix::constData() const noexcept
{
    return d ? d->data : nullptr;
}

double& Matrix::operator()(size_t i, size_t j) noexcept
{
    assert((d != nullptr) && (i < d->size1) && (j < d->size2));
    return d->data[i * d->tda + j];
}

const double& Matrix::operator()(size_t i, size_t j) const noexcept
{
    assert((d != nullptr) && (i < d->size1) && (j < d->size2));
    return d->data[i * d->tda + j];
}

std::ostream& operator<<(std::ostream& os, Matrix const&m)
{
    if (m.isNull() || m.size1() == 0 || m.size2() == 0) {
        os << "mat(null)";
    }
    else {
        const size_t n1 = m.size1();
        const size_t n2 = m.size2();
        //const char sep = '\t';
        const char sep = ',';
        os << "mat(" << n1 << "," << n2 << ")" << std::endl;

        for (size_t i = 0; i < n1; ++i) {
            for (size_t j = 0; j < n2; ++j) {
                os << std::setprecision(3) << m(i, j);
                if (j < (n2 - 1))
                    os << sep;
            }
            if (i < (n1 - 1))
                os << std::endl;
        }
    }
    return os;
}
