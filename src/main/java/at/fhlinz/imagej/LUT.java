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

package at.fhlinz.imagej;

/**
 * Interface for lookup tables used by the fitting algorithm
 * @author Fabian Hauser
 */
public interface LUT {
    /**
     * @return get the pointer to the generated lookup table array 
     */
    public double[] getLookUpTableArray();
    
    /**
     * @return get the minimum lateral position within the template in pixels
     */
    public double getMinLat();

    /**
     * @return get the maximum lateral position within the template in pixels
     */
    public double getMaxLat();

    /**
     * @return get the minimum axial position in nm
     */
    public double getMinAx();

    /**
     * @return get the maximum axial position in nm
     */
    public double getMaxAx();

    /**
     * @return get the window size of the templates in pixels
     */
    public int getWindowSize();

    /**
     * @return get the lateral step size in pixels
     */
    public double getDeltaLat();

    /**
     * @return get the axial step size in nm
     */
    public double getDeltaAx();
    
    /**
     * @return get the lateral range within the template in pixels
     */
    public double getLateralRange();
    
    /**
     * @return get the axial range in nm
     */
    public double getAxialRange();
}
