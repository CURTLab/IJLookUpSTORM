#ifndef WAVELET_H
#define WAVELET_H

#include "Image.h"

#include <cmath>

namespace LookUpSTORM
{

// this class is not thread safe!
class DLL_DEF_LUT Wavelet
{
public:
	Wavelet();
	Wavelet(int width, int height);

	void setSize(int width, int height);

	const ImageF32 &filter(const ImageU16& input);

	const float inputMean() const;
	const float inputSD() const;
	const float inputSTD() const;

private:
	ImageF32 m_padded;
	ImageF32 m_result;
	float m_mean;
	float m_sd;

};

DLL_DEF_LUT ImageF32 waveletFilter(const ImageU16& input);

}

#endif // !WAVELET_H
