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

#ifndef MATRIX_H
#define MATRIX_H

#include "Common.h"

class MatrixData;

/*
 * matrix class with copy-on-write mechanics inspired by the GNU Scientific Library (GSL) interface
 */
class Matrix
{
public:
	// create an empty matrix with no data at all
	Matrix() noexcept;
	// create matrix with array initialized to 0.0
	Matrix(size_t size1, size_t size2);
	// create matrix with uninitialized array
	Matrix(size_t size1, size_t size2, Initialization init) noexcept;
	// create matrix with array initialized to supplied value
	Matrix(size_t size1, size_t size2, double value) noexcept;

	// copy-on-write methods
	Matrix(const Matrix& image) noexcept;
	~Matrix() noexcept;
	Matrix& operator=(const Matrix& other) noexcept;

	// chech if MatrixData or array is null
	bool isNull() const noexcept;
	// check if size1/size2 is zero
	bool isZero() const noexcept;

	size_t size1() const noexcept;
	size_t size2() const noexcept;
	size_t tda() const noexcept;

	void fill(double value) noexcept;
	void setZero() noexcept;
	void setIdentity() noexcept;

	double sum() const;

	double* data() noexcept;
	const double* constData() const noexcept;

	double& operator()(size_t i, size_t j) noexcept;
	const double& operator()(size_t i, size_t j) const noexcept;

private:
	MatrixData* d;

};

std::ostream& operator<<(std::ostream&, Matrix const&);

#endif // MATRIX_H
