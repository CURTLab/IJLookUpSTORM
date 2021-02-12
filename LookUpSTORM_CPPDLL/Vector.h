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

#ifndef VECTOR_H
#define VECTOR_H

#include "Common.h"

namespace LookUpSTORM
{

class VectorData;

/*
 * vector class with copy-on-write mechanics inspired by the GNU Scientific Library (GSL) interface
 */
class Vector
{
public:
	// create an empty vector with no data at all
	Vector() noexcept;
	// create vector with array initialized to 0.0
	Vector(size_t size) noexcept;
	// create vector with uninitialized array
	Vector(size_t size, Initialization init) noexcept;
	// create vector with array initialized to supplied value
	Vector(size_t size, double value) noexcept;

	// copy-on-write methods
	Vector(const Vector& image) noexcept;
	~Vector() noexcept;
	Vector& operator=(const Vector& other) noexcept;

	// chech if MatrixData or array is null
	bool isNull() const noexcept;
	// check if size1/size2 is zero
	bool isZero() const noexcept;

	size_t size() const noexcept;
	size_t stride() const noexcept;

	void fill(double value) noexcept;
	void setZero() noexcept;

	double sum() const;

	double* data() noexcept;
	const double* constData() const noexcept;

	double &operator[](size_t i) noexcept;
	const double& operator[](size_t i) const noexcept;
	void operator+=(const Vector& v);
	void operator-=(const Vector& v);

private:
	VectorData* d;

};

} // namespace LookUpSTORM

std::ostream& operator<<(std::ostream&, LookUpSTORM::Vector const&);

#endif // !VECTOR_H

