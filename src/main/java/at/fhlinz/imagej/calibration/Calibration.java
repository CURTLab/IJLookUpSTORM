/****************************************************************************
 *
 * MIT License
 *
 * Copyright (C) 2020 Fabian Hauser
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

package at.fhlinz.imagej.calibration;

import java.util.HashMap;

/**
 * Interface for astigmatism calibrations
 * @author Fabian Hauser
 */
public interface Calibration {
    /*
     * @return calibration type name
     */
    public String name();
    
    /*
     * Parse the parameter of a yaml file
     * @return true if the parameters could be parsed
     */
    public boolean parse(HashMap<String, Double> parameters);
    
    /**
     * @return the focus plane in nm (axial position)
     */
    public double getFocusPlane();
    
    /**
     * @return the rotation theta in radians of the 2D elliptical Gaussian 
     * function
     */
    public double getTheta();
    
    /**
     * @param z axial position in nm
     * @return a double array containing sigmaX & sigmaY in pixels from the 
     * provided axial position (in nm)
     */
    public double[] value(double z);
    
    /**
     * Returns an array with the values (sigmaX, sigmaY) and their derivative
     * in an array
     * @param z axial position in nm
     * @return array of [sigmaX,sigmaY,dSigmaY, dSigmaY]
     */
    public double[] valDer(double z);
    
}
