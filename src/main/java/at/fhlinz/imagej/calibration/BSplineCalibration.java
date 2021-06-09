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
import org.apache.commons.math3.analysis.UnivariateFunction;
import org.apache.commons.math3.analysis.polynomials.PolynomialFunction;
import org.apache.commons.math3.analysis.polynomials.PolynomialSplineFunction;
import org.apache.commons.math3.linear.Array2DRowRealMatrix;
import org.apache.commons.math3.linear.ArrayRealVector;
import org.apache.commons.math3.linear.LUDecomposition;
import org.apache.commons.math3.linear.RealMatrix;
import org.apache.commons.math3.linear.RealVector;

/**
 * Calibration class to handle astigmatism single molecule localization 
 * microscopy calibration files using cubic b-splines
 * 
 * Input is a YAML file:
 * !!at.calibration.bspline { 
 *    theta: -0.103174,
 *    pixelSize: 166.667, 
 *    knot0x: 0.986468, knot0y: 0.402337, knot0z: 0, 
 *    ...
 *    knot9x: 0.3007, knot9y: 0.558788, knot9z: 2000
 * }
 * 
 * Where theta is the rotation angle of the elliptical Gaussian functions in 
 * RAD, pixelSize of in nm, knot0 .. knotN where N is the last spline knot,
 * knotX & knotY in pixel and knotZ in nm
 * 
 * @author Fabian Hauser
 */
public class BSplineCalibration implements Calibration {
    private double _pixelSizeUM;
    private double _theta;
    private PolynomialSplineFunction _wxSpline;
    private PolynomialSplineFunction _wySpline;
    private UnivariateFunction _dwx;
    private UnivariateFunction _dwy;
    private double _minZ;
    private double _maxZ;
    private double _focusPlane;
    
    /**
     * Constructor
     */
    public BSplineCalibration() {
        _pixelSizeUM = 1.0;
        _theta = 0.0;
        _wxSpline = null;
        _wySpline = null;
    }

    @Override
    public String name() {
        return "BSpine Calibration";
    }

    @Override
    public boolean parse(HashMap<String, Double> parameters) {
        // find important parameters
        if (parameters.containsKey("pixelSize"))
            _pixelSizeUM = parameters.get("pixelSize") / 1000;

        if (parameters.containsKey("theta"))
            _theta = parameters.get("theta");
        else if (parameters.containsKey("angle"))
            _theta = parameters.get("angle");

        generateSplines(parameters);

        // determine focal plane
        if (parameters.containsKey("focalPlane"))
            _focusPlane = parameters.get("focalPlane");
        else
            _focusPlane = CalibrationFactory.calculateFocalPlane(this, _minZ, _maxZ);
        
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
        return _theta;
    }

    /**
     * @param z axial position in nm
     * @return a double array containing sigmaX & sigmaY in pixels from the 
     * provided axial position (in nm)
     */
    @Override
    public double[] value(double z) {
        return new double[] {
            _wxSpline.value(z),
            _wySpline.value(z)
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
            _wxSpline.value(z),
            _wySpline.value(z),
            _dwx.value(z),
            _dwy.value(z)
        };
    }

    /** 
     * Calculate cubic b-spline polynomials from array values
     * @param x x-values
     * @param y y-values
     * @return PolynomialSplineFunction containing the b-spline polynomials
     */
    private PolynomialSplineFunction generateSpline(double[] x, double[] y) {
        final int N = x.length;
        final double h = x[1] - x[0];        
        double [][] dA = new double[N-2][N-2];
        
        RealMatrix A = new Array2DRowRealMatrix(dA, false);
        RealVector b = new ArrayRealVector(N-2);
        
        // calculate matrix for cubic bspline polynomal
        for (int j = 0; j < N-2; j++) {
            b.setEntry(j, (y[j] - 2*y[j+1] + y[j+2]) * 6 / (h*h));
            dA[j][j] = 4;
            if (j > 0) dA[j][j-1] = 1;
            if (j < N-3) dA[j][j+1] = 1;
        }
        LUDecomposition lu = new LUDecomposition(A);
        RealVector lux = lu.getSolver().solve(b);
        
        double[] m = new double[N];
        for (int i = 0; i < lux.getDimension(); i++)
            m[i+1] = lux.getEntry(i);
        
        PolynomialFunction[] polynomials = new PolynomialFunction[N - 1];
        for (int i = 0; i < polynomials.length; i++) {
            double[] p = new double[4];
            p[3] = (m[i+1] - m[i]) / (6.0 * h);
            p[2] = 0.5 * m[i];
            p[1] = (y[i+1] -  y[i]) / h - (m[i+1] + 2.0 * m[i]) * h / 6.0;
            p[0] = y[i];
            polynomials[i] = new PolynomialFunction(p);
        }
        return new PolynomialSplineFunction(x, polynomials);
    }
    
    private void generateSplines(HashMap<String, Double> parameters) {
        int knots = 0;
        while(parameters.containsKey("knot"+knots+"x")) {
            knots++;
        }
        double[] xKnots = new double[knots];
        double[] yKnots = new double[knots];
        double[] zKnots = new double[knots];
        for (int i = 0; i < knots; ++i) {
            xKnots[i] = parameters.get("knot"+i+"x") / _pixelSizeUM;
            yKnots[i] = parameters.get("knot"+i+"y") / _pixelSizeUM;
            zKnots[i] = parameters.get("knot"+i+"z");
        }
        
        _minZ = zKnots[0];
        _maxZ = zKnots[zKnots.length-1];
        
        _wxSpline = generateSpline(zKnots, xKnots);
        _wySpline = generateSpline(zKnots, yKnots);
        
        _dwx = _wxSpline.derivative();
        _dwy = _wySpline.derivative();
    }
    
}
