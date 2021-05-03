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

#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <cmath>
#include <algorithm>

#if !defined(JNI_EXPORT_LUT) && defined(DLL_EXPORT_LUT)
#define DLL_DEF_LUT __declspec(dllexport)
#elif !defined(JNI_EXPORT_LUT) && defined(DLL_IMPORT_LUT) 
#define DLL_DEF_LUT __declspec(dllimport)
#else
#define DLL_DEF_LUT
#endif // DLL_EXPORT

namespace LookUpSTORM
{

template<typename T>
static inline T constexpr sqr(const T& x) { return x * x; }

template <class T>
static inline T constexpr bound(const T& v, const T& lo, const T& hi) { return (v < lo ? lo : (v > hi ? hi: v)); }

static inline bool cmp(double v1, double v2) { return std::abs(v1 - v2) * 1E12 <= std::min(std::abs(v1), std::abs(v2)); }

static constexpr uint32_t BLACK = 0xff000000;

enum class Initialization {
    Uninitialized
};
static constexpr Initialization Uninitialized = Initialization::Uninitialized;

enum class Projection {
    TopDown,
    BottomUp,
    SideXZ,
    SideYZ
};


class Molecule
{
public:
	inline Molecule() : data{ 0.0 } {}

	union {
		struct {
			double background;
			double peak;
			double x;
			double y;
			double z;
			double frame;
			double xfit;
			double yfit;
			double time_us;
		};
		double data[7];
	};
};

} // namespace LookUpSTORM

#endif // !COMMON_H
