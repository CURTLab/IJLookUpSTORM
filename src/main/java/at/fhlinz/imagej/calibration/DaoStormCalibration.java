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
 * Calibration class to handle astigmatism single molecule localization 
 * microscopy calibration files using 3D DAO STORM calibration format produced
 * by ThunderSTORM
 * (https://github.com/zitmen/thunderstorm)
 * 
 * equation:
 *  sigma(z) = w0*sqrt(1 + ((z-c)/d)^2 + a*((z-c)/d)^3 + b*((z-c)/d)^4)
 * 
 * @author Fabian Hauser
 */
public class DaoStormCalibration implements Calibration {
    private double _w01;
    private double _a1;
    private double _b1;
    private double _c1;
    private double _d1;
    private double _w02;
    private double _a2;
    private double _b2;
    private double _c2;
    private double _d2;
    private double _angle;
    private double _focusPlane;
    
    /**
     * Constructor
     */
    public DaoStormCalibration() {
        _w01 = 1.0;
        _w02 = 1.0;
        _angle = 0.0;
        _focusPlane = 0.0;
    }
    
    @Override
    public String name() {
        return "3D DAO STORM Calibration";
    }

    @Override
    public boolean parse(HashMap<String, Double> parameters) {
        if (parameters.containsKey("angle"))
            _angle = parameters.get("angle");
        _w01 = parameters.getOrDefault("w01", 1.0);
        _w02 = parameters.getOrDefault("w02", 1.0);
        _a1 = parameters.getOrDefault("a1", 0.0);
        _a2 = parameters.getOrDefault("a2", 0.0);
        _b1 = parameters.getOrDefault("b1", 0.0);
        _b2 = parameters.getOrDefault("b2", 0.0);
        _c1 = parameters.getOrDefault("c1", 0.0);
        _c2 = parameters.getOrDefault("c2", 0.0);
        _d1 = parameters.getOrDefault("d1", 1.0);
        _d2 = parameters.getOrDefault("d2", 1.0);
        
        _focusPlane = CalibrationFactory.calculateFocalPlane(this, 0, 3000);
        
        return true;
    }

    /**
     * @return the focus plane in nm (axial position)
     */
    @Override
    public double getFocusPlane() {
        return _focusPlane;
    }

    /**
     * @return the rotation theta in radians of the 2D elliptical Gaussian 
     * function
     */
    @Override
    public double getTheta() {
        return _angle;
    }

    /**
     * @param z axial position in nm
     * @return a double array containing sigmaX & sigmaY in pixels from the 
     * provided axial position (in nm)
     */
    @Override
    public double[] value(double z) {
        return new double[] {
            sigma1(z),
            sigma2(z)
        };
    }

    /**
     * Returns an array with the values (sigmaX, sigmaY) and their derivative
     * in an array
     * @param z axial position in nm
     * @return array of [sigmaX,sigmaY,dSigmaY, dSigmaY]
     */
    @Override
    public double[] valDer(double z) {
        return new double[] {
            sigma1(z),
            sigma2(z),
            dsigma1(z),
            dsigma2(z)
        };
    }
    
    private double sigma1(double z) {
        final double d = (z - _c1)/_d1;
        return 0.5 * _w01 * Math.sqrt(1.0 + d * d + _a1 * d * d * d + _b1 * d * d * d * d);
    }
    
    private double sigma2(double z) {
        final double d = (z - _c2)/_d2;
        return 0.5 * _w02 * Math.sqrt(1.0 + d * d + _a2 * d * d * d + _b2 * d * d * d * d);
    }
    
     private double dsigma1(double z) {
        final double d = (z - _c1)/_d1;
        return _w01 * (d / _d1) * (2.0 + 3.0 * _a1 * d + 4.0 * _b1 * d * d) /
                (2.0 * Math.sqrt(1.0 + d * d + _a1 * d * d * d + _b1 * d * d * d * d));
    }
    
    private double dsigma2(double z) {
        final double d = (z - _c2)/_d2;
        return _w02 * (d / _d2) * (2.0 + 3.0 * _a2 * d + 4.0 * _b2 * d * d) /
                (2.0 * Math.sqrt(1.0 + d * d + _a2 * d * d * d + _b2 * d * d * d * d));
    }
    
}
