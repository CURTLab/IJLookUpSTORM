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

#ifndef LUT_H
#define LUT_H

#include <functional>
#include "Common.h"

namespace LookUpSTORM
{

class DLL_DEF_LUT LUT
{
public:
	LUT();

	// generate a LUT table
	// parameters:
	// * windowSize: size of the template image in pixels
	// * dLat: lateral step in pixel
	// * dAx: axial step in nm
	// * rangeLat: fraction of the windowSize in pixels in which the dLat moves in x- and y-direction
	// * rangeAx: axial range in nm over which the template images are generated
	bool generate(size_t windowSize, double dLat, double dAx, double rangeLat, double rangeAx,
		std::function<void(size_t index, size_t max)> callback = [](size_t, size_t) {}
	);

	// releases the memory allocated for the LUT
	void release();

	// saves the generated LUT as binary
	bool save(const std::string& fileName);

	// checks if a LUT was generated by calling the method 'generate'
	inline constexpr bool isValid() const;

	// returns the window size of the templates in pixels
	inline constexpr size_t windowSize() const;

	// returns the number of lateral templates needed to fullfil the condition rangeLat/dLat
	inline constexpr size_t countLat() const;

	// returns the number of axial templates needed to fullfil the condition rangeAx/dAx
	inline constexpr size_t countAx() const;

	// returns the lateral step size in pixels
	inline constexpr double dLat() const;

	// returns the axial step size in nm
	inline constexpr double dAx() const;

	// returns the lateral range within the template in pixels
	inline constexpr double rangeLat() const;

	// returns the axial range in nm
	inline constexpr double rangeAx() const;

	// returns the minimum lateral position within the template in pixels
	inline constexpr double minLat() const;

	// returns the maximum lateral position within the template in pixels
	inline constexpr double maxLat() const;

	// returns the minimum axial position in nm
	inline constexpr double minAx() const;

	// returns the maximum axial position in nm
	inline constexpr double maxAx() const;

	// returns the pointer to the generated lookup table array 
	inline constexpr const double* ptr() const;

	// returns the array size for the generated LUT
	inline constexpr const size_t dataSize() const;

	// calculate the index of a generated LUT by the given xyz-position (xy in pixels, z in nm)
	size_t lookupIndex(double x, double y, double z) const;

	// calculate the xyz-position as tuple (xy in pixels, z in nm) by the given index
	std::tuple<double, double, double> lookupPosition(size_t index) const;

	// calculates the bytes needed to generate a LUT with the parameters given
	static size_t calculateUsageBytes(size_t windowSize, double dLat, double dAx, double rangeLat, double rangeAx);

protected:
	// called before the template loop starts
	virtual void preTemplates(size_t windowSize, double dLat, double dAx, double rangeLat, double rangeAx) = 0;

	// called before a new template image starts
	virtual void startTemplate(size_t index, double x, double y, double z) = 0;
	// called during the pixel loop and expects a tuple in form of {PSF, dPSF/dx, dPSF/dy, dPSF/dz}
	virtual std::tuple<double,double,double,double> templateAtPixel(size_t index, double x, double y, double z, size_t pixX, size_t pixY) = 0;
	// called after all pixel of the current template image are finshed
	virtual void endTemplate(size_t index, double x, double y, double z) = 0;

private:
	double* m_data;
	size_t m_dataSize;
	size_t m_windowSize;
	size_t m_countLat;
	size_t m_countAx;
	double m_dLat;
	double m_dAx;
	double m_rangeLat;
	double m_rangeAx;
	double m_minLat;
	double m_maxLat;
	double m_minAx;
	double m_maxAx;

};

inline
constexpr bool LUT::isValid() const
{
	return (m_data != nullptr) && (m_dataSize > 0) && (m_windowSize > 0);
}

inline
constexpr size_t LUT::windowSize() const
{
	return m_windowSize;
}

inline 
constexpr size_t LUT::countLat() const
{
	return m_countLat;
}

inline 
constexpr size_t LUT::countAx() const
{
	return m_countAx;
}

inline
constexpr double LUT::dLat() const
{
	return m_dLat;
}

inline
constexpr double LUT::dAx() const
{
	return m_dAx;
}

inline
constexpr double LUT::rangeLat() const
{
	return m_rangeLat;
}

inline
constexpr double LUT::rangeAx() const
{
	return m_rangeAx;
}

inline
constexpr double LUT::minLat() const
{
	return m_minLat;
}

inline
constexpr double LUT::maxLat() const
{
	return m_maxLat;
}

inline
constexpr double LUT::minAx() const
{
	return m_minAx;
}

inline
constexpr double LUT::maxAx() const
{
	return m_maxAx;
}

inline
constexpr const double* LUT::ptr() const
{
	return m_data;
}

inline
constexpr const size_t LUT::dataSize() const
{
	return m_dataSize;
}

} // namespace LookUpSTORM

#endif // LUT_H