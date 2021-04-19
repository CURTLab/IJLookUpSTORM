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

#ifndef RECT_H
#define RECT_H

#include "Common.h"

namespace LookUpSTORM
{

/*
 * rectange class inspired by the QT QRect interface
 */
class DLL_DEF_LUT Rect
{
public:
    constexpr inline Rect() : x1(0), y1(0), x2(-1), y2(-1) {}
    constexpr inline Rect(int left, int top, int width, int height)
        : x1(left), y1(top), x2(left + width - 1), y2(top + height - 1) {}

    // if possible move the supplied rectange inside this rectange
    bool moveInside(const Rect& r);

    bool fullyContains(const Rect& r) const;

    constexpr inline bool contains(int x, int y) const 
    { return x >= x1 && y >= y1 && x <= x2 && y <= y2; }
    constexpr inline bool contains(std::pair<int, int> point) const
    { return contains(point.first, point.second); }

    void extendByPoint(int x, int y);
    inline void extendByPoint(std::pair<int, int> point)
    { extendByPoint(point.first, point.second); }

    constexpr inline bool isNull() const
    { return (x2 == (x1 - 1)) && (y2 == (y1 - 1)); }

    constexpr inline int x() const { return x1; }
    constexpr inline int y() const { return y1; }
    constexpr inline int width() const { return x2 - x1 + 1;  }
    constexpr inline int height() const { return y2 - y1 + 1;  }

    constexpr inline int area() const 
    { return (x2 - x1 + 1) * (y2 - y1 + 1); }

    constexpr inline void setWidth(int width) { x2 = x1 + width - 1; }
    constexpr inline void setHeight(int height) { y2 = y1 + height - 1; }

    constexpr inline int left() const { return x1; }
    constexpr inline int top() const { return y1; }
    constexpr inline int right() const { return x2; }
    constexpr inline int bottom() const { return y2; }

    constexpr inline void moveLeft(int pos) { x2 += (pos - x1); x1 = pos; }
    constexpr inline void moveTop(int pos) { y2 += (pos - y1); y1 = pos; }
    constexpr inline void moveRight(int pos) { x1 += (pos - x2); x2 = pos; }
    constexpr inline void moveBottom(int pos) { y1 += (pos - y2); y2 = pos; }

    constexpr inline bool operator==(const Rect& r) const 
    { return x1 == r.x1 && x2 == r.x2 && y1 == r.y1 && y2 == r.y2; }
    constexpr inline bool operator!=(const Rect & r) const
    { return x1 != r.x1 || x2 != r.x2 || y1 != r.y1 || y2 != r.y2; }

private:
    int x1;
    int y1;
    int x2;
    int y2;

};

} // namespace LookUpSTORM

std::ostream& operator<<(std::ostream&, LookUpSTORM::Rect const&);

#endif // !RECT_H
