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

#include "Rect.h"

bool Rect::moveInside(const Rect& r)
{
    if (width() > r.width() && height() > r.height())
        return false;
    if (x1 < r.x1) moveLeft(r.x1);
    else if (x2 > r.x2) moveRight(r.x2);
    if (y1 < r.y1) moveTop(r.y1);
    else if (y2 > r.y2) moveBottom(r.y2);
    return true;
}

bool Rect::fullyContains(const Rect& r) const
{
    return (r.x1 >= x1) && (r.y1 >= y1) && (r.x2 <= x2) && (r.y2 <= y2);
}

void Rect::extendByPoint(int x, int y)
{
    if (isNull()) {
        x1 = x; x2 = x;
        y1 = y; y2 = y;
    }
    else {
        if (x1 > x) x1 = x;
        if (x2 < x) x2 = x;
        if (y1 > y) y1 = y;
        if (y2 < y) y2 = y;
    }
}

std::ostream& operator<<(std::ostream& os, Rect const& r)
{
    if (r.isNull()) {
        os << "rect(null)";
    }
    else {
        os << "rect(" << r.x() << "," << r.y() << " " << r.width() << "x" << r.height() << ")";
    }
    return os;
}
