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

#include "Calibration.h"

#include "LinearMath.h"
#include "brent.hpp"

#include <vector>
#include <iostream>
#include <fstream>
#include <regex>

namespace LookUpSTORM
{

struct Knot
{
    double x;
    double y;
    double z;

    inline constexpr const double &operator[](size_t dir) const 
    { return dir == 1 ? y : x; }
};



class FocusFunction : public brent::func_base {
    const Calibration* m_cali;
public:
    inline FocusFunction(const Calibration* c) : m_cali(c) {}
    inline virtual double operator() (double z)
    {
        const auto v = m_cali->value(z);
        return std::abs(v.first - v.second);
    }
};

class CalibrationPrivate
{
public:
    inline CalibrationPrivate()
        : h(0.0)
        , theta(0.0)
        , pixelSize(1.0)
        , focalPlane(0.0)
    {}

    std::vector<Knot> knots;
    double h;
    Matrix coeffs[2];
    double theta;
    double pixelSize;
    double focalPlane;
    Parameters parameters;

    inline bool parseParameters();
    inline bool generateSplines();
};

inline bool CalibrationPrivate::parseParameters()
{
    pixelSize = (parameters.count("pixelSize") > 0 ? parameters["pixelSize"] / 1000.0 : 1.0);

    int knotNr = 0;
    std::string knotName = "knot" + std::to_string(knotNr);
    while (parameters.count(knotName + 'x') > 0) {
        if (parameters.count(knotName + 'y') < 1 || parameters.count(knotName + 'z') < 1)
            break;
        knots.push_back({ parameters[knotName + 'x'] / pixelSize, 
                          parameters[knotName + 'y'] / pixelSize, 
                          parameters[knotName + 'z'] });
        ++knotNr;
        knotName = "knot" + std::to_string(knotNr);
    }
    if (knots.empty()) {
        std::cout << "Calibration: Cannot import calibration because it doesn't contains knots!" << std::endl;
        return false;
    }

    focalPlane = (knots.back().z - knots.front().z) * 0.5;

    if (parameters.count("angle") > 0) {
        theta = parameters["angle"];
    }
    else if (parameters.count("theta") > 0) {
        theta = parameters["theta"];
    }
    else {
        std::cout << "Calibration: The angle theta is not defined!" << std::endl;
        return false;
    }
    return true;
}

inline bool CalibrationPrivate::generateSplines()
{
    // solve equation for cubic b-splines from knots
    const size_t N = knots.size();
    Vector b(N - 2);
    Matrix A(N - 2, N - 2);
    std::vector<int> ipiv(N - 2);

    std::vector<double> m(N);

    h = (knots[1].z - knots[0].z);

    for (size_t dir = 0; dir < 2; ++dir) {
        A.setZero();
        for (size_t j = 0; j < b.size(); ++j) {
            b[j] = (knots[j][dir] - 2.0 * knots[j + 1][dir] + knots[j + 2][dir]) * 6.0 / (h * h);
            A(j, j) = 4.0;
            if (j > 0) A(j, j - 1) = 1.0;
            if (j < N - 3) A(j, j + 1) = 1.0;
        }
        // try to solve the equation system
        if (LAPACKE::dgesv(A, ipiv.data(), b) != 0)
            return false;

        // prepare coefficent matrix
        m.front() = 0.0;
        m.back() = 0.0;
        for (size_t i = 0; i < b.size(); i++)
            m[i + 1] = b[i];

        coeffs[dir] = Matrix(N - 1, 4);
        for (size_t i = 0; i < N - 1; ++i) {
            coeffs[dir](i, 3) = knots[i][dir];
            coeffs[dir](i, 2) = (knots[i + 1][dir] - knots[i][dir]) / h
                - (m[i + 1] + 2 * m[i]) * h / 6.0;
            coeffs[dir](i, 1) = m[i] / 2;
            coeffs[dir](i, 0) = (m[i + 1] - m[i]) / (6 * h);
        }
    }
    return true;
}

} // namespace LookUpSTORM

using namespace LookUpSTORM;

Calibration::Calibration()
    : d(new CalibrationPrivate)
{
}

LookUpSTORM::Calibration::~Calibration()
{
    delete d;
}

bool Calibration::load(const std::string& fileName)
{
    std::string line;
    std::ifstream file;
    file.open(fileName, std::ifstream::in);
    if (!file) {
        std::cerr << "Calibration: Could not open calibration file! (" << fileName << ")" << std::endl;
        return false;
    }

    std::string data;
    while (std::getline(file, line))
        data += line;

    file.close();

    return parseJAML(data);
}

bool LookUpSTORM::Calibration::parseJAML(const std::string& data)
{
    if (data.empty()) {
        std::cerr << "Calibration: Calibration file is empty!" << std::endl;
        return false;
    }

    d->knots.clear();
    d->parameters.clear();

    try {
        std::regex re("!!([\\w\\.]+)\\s*");
        std::smatch match;
        if (std::regex_search(data, match, re) && match.size() > 1) {
            const std::string typeName = match.str(1);

        }
        else {
            std::cerr << "Calibration: Invalid header!" << std::endl;
            return false;
        }

        std::regex param("(\\w*):\\s*([\\-]*[0-9+][.\\w]*[-\\w]*)");
        std::sregex_iterator next(data.begin(), data.end(), param);
        std::sregex_iterator end;
        while (next != end) {
            std::smatch match = *next;
            next++;
            if (match.size() != 3) continue;
            std::string key = match.str(1);
            const double value = std::stod(match.str(2));
            d->parameters.insert({ key, value });
        }
    }
    catch (std::regex_error& e) {
        // Syntax error in the regular expression
        std::cerr << "Calibration: Regex error: " << e.what() << std::endl;
        return false;
    }

    if (!d->parseParameters() || !d->generateSplines())
        return false;

    if (d->parameters.count("focalPlane") > 0) {
        d->focalPlane = d->parameters["focalPlane"];
    }
    else {
        // calculate focal plane
        FocusFunction func(this);
        const double e = std::sqrt(brent::r8_epsilon());
        const double t = std::sqrt(brent::r8_epsilon());

        const double min = d->knots.front().z;
        const double max = d->knots.back().z;
        brent::glomin(min, max, (max - min) / 2, 10, e, t, func, d->focalPlane);
    }

    return true;
}

bool Calibration::generateSpline()
{
    return false;
}

std::pair<double, double> Calibration::value(double z) const
{
    std::pair<double, double> ret{ 0.0, 0.0 };
    const size_t i = std::min(std::max(0ull, static_cast<size_t>(std::floor((z - d->knots[0].z) / d->h))), d->knots.size() - 2);
    const double dx = (z - d->knots[i].z);
    for (size_t term = 0; term < 4; ++term) {
        ret.first = ret.first * dx + d->coeffs[0](i, term);
        ret.second = ret.second * dx + d->coeffs[1]( i, term);
    }
    return ret;
}

std::pair<double, double> Calibration::dvalue(double z) const
{
    std::pair<double, double> ret{ 0.0, 0.0 };
    const size_t i = std::min(std::max(0ull, static_cast<size_t>(std::floor((z - d->knots[0].z) / d->h))), d->knots.size() - 2);
    const double dx = (z - d->knots[i].z);
    for (size_t term = 0; term < 3; ++term) {
        ret.first = ret.first * dx + (3 - term) * d->coeffs[0](i, term);
        ret.second = ret.second * dx + (3 - term) * d->coeffs[1](i, term);
    }
    return ret;
}

std::tuple<double, double, double, double> Calibration::valDer(double z) const
{
    std::tuple<double, double, double, double> ret{ 0.0, 0.0, 0.0, 0.0 };
    const size_t i = std::min(std::max(0ull, static_cast<size_t>(std::floor((z - d->knots[0].z) / d->h))), d->knots.size() - 2);
    const double dx = (z - d->knots[i].z);
    for (size_t term = 0; term < 3; ++term) {
        std::get<0>(ret) = std::get<0>(ret) * dx + d->coeffs[0](i, term);
        std::get<1>(ret) = std::get<1>(ret) * dx + d->coeffs[1](i, term);
        std::get<2>(ret) = std::get<2>(ret) * dx + (3 - term) * d->coeffs[0](i, term);
        std::get<3>(ret) = std::get<3>(ret) * dx + (3 - term) * d->coeffs[1](i, term);
    }
    std::get<0>(ret) = std::get<0>(ret) * dx + d->coeffs[0](i, 3);
    std::get<1>(ret) = std::get<1>(ret) * dx + d->coeffs[1](i, 3);
    return ret;
}

size_t LookUpSTORM::Calibration::knots() const
{
    return d->knots.size();
}

std::tuple<double, double, double> LookUpSTORM::Calibration::knot(size_t i) const
{
    return { d->knots[i].x, d->knots[i].y, d->knots[i].z };
}

double Calibration::theta() const
{
    return d->theta;
}

double LookUpSTORM::Calibration::pixelSize() const
{
    return d->pixelSize;
}

double Calibration::focalPlane() const
{
    return d->focalPlane;
}

double Calibration::minZ() const
{
    return d->knots.empty() ? 0.0 : d->knots.front().z;
}

double Calibration::maxZ() const
{
    return d->knots.empty() ? 0.0 : d->knots.back().z;
}

const Parameters& LookUpSTORM::Calibration::parameters() const
{
    return d->parameters;
}
