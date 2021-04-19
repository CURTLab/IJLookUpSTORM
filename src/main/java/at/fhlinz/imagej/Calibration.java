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

package at.fhlinz.imagej;

import ij.gui.Plot;
import ij.gui.PlotWindow;
import java.awt.Color;
import java.io.File;
import java.io.FileInputStream;
import java.util.HashMap;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.apache.commons.math3.analysis.UnivariateFunction;
import org.apache.commons.math3.analysis.polynomials.PolynomialFunction;
import org.apache.commons.math3.analysis.polynomials.PolynomialSplineFunction;
import org.apache.commons.math3.linear.Array2DRowRealMatrix;
import org.apache.commons.math3.linear.ArrayRealVector;
import org.apache.commons.math3.linear.LUDecomposition;
import org.apache.commons.math3.linear.RealMatrix;
import org.apache.commons.math3.linear.RealVector;
import org.apache.commons.math3.optim.MaxEval;
import org.apache.commons.math3.optim.nonlinear.scalar.GoalType;
import org.apache.commons.math3.optim.univariate.BrentOptimizer;
import org.apache.commons.math3.optim.univariate.SearchInterval;
import org.apache.commons.math3.optim.univariate.UnivariateObjectiveFunction;
import org.apache.commons.math3.optim.univariate.UnivariatePointValuePair;

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
public class Calibration {
    private final HashMap<String, Double> _parameters;
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
    public Calibration() {
        _parameters = new HashMap<String, Double>();
        _pixelSizeUM = 1.0;
        _theta = 0.0;
        _wxSpline = null;
        _wySpline = null;
    }
            
    /**
     * Load a calibration yaml file. Supported are cubic bspline representation 
     * (at.calibration.bspline)
     * Needed parameters are 'pixelSize' in nm and 'angle'/'theta'
     * @param fileName YAML file with parameter
     * @return returns true if the YAML file could be loaded
     */
    public boolean load(String fileName)
    {
        try {
            File file = new File(fileName);   
            FileInputStream stream = new FileInputStream(file);
            byte [] bytes = new byte[(int)file.length()];
            if (stream.read(bytes) != file.length())
                return false;
            String data = new String(bytes, "UTF8");
            
            Matcher matcher = Pattern.compile("!!([\\w\\.]+)\\s*").matcher(data);
            if (!matcher.find())
                return false;
            
            String typeString = matcher.group(1);
            if (!typeString.equals("at.calibration.bspline")) {
                System.err.printf("LookUpSTORM: Calibration: Unknown type '%s'!\n", typeString);
                return false;
            }
            
            _parameters.clear();
            
            // parse parameters
            Pattern p = Pattern.compile("(\\w*):\\s*([\\-]*[0-9+][.\\w]*[-\\w]*)");
            matcher = p.matcher(data.substring(matcher.end()));
            while (matcher.find()) {
                _parameters.put(matcher.group(1), Double.parseDouble(matcher.group(2)));
            }
            if (_parameters.isEmpty())
                return false;
            
            // find important parameters
            if (_parameters.containsKey("pixelSize"))
                _pixelSizeUM = _parameters.get("pixelSize") / 1000;
            
            if (_parameters.containsKey("theta"))
                _theta = _parameters.get("theta");
            else if (_parameters.containsKey("angle"))
                _theta = _parameters.get("angle");
            
            generateSplines();
            
            // determine focal plane
            if (_parameters.containsKey("focalPlane"))
                _focusPlane = _parameters.get("focalPlane");
            else
                _focusPlane = calculateFocalPlane();
            
            return true;
        } catch(Exception e) {
            return false;
        }
    }
    
    /**
     * @return the focus plane in nm (axial position)
     */
    public double getFocusPlane() {
        return _focusPlane;
    }
    
    /**
     * @return get axial range in nm
     */
    public double getAxialRange() {
        return (_maxZ - _minZ);
    }
    
    /**
     * @return the minimum of the z position in nm
     */
    public double getMinZ() {
        return (_maxZ - _minZ);
    }
    
    /**
     * @return the maximum of the z position in nm
     */
    public double getMaxZ() {
        return (_maxZ - _minZ);
    }
    
    /**
     * @return the rotation theta in radians of the 2D elliptical Gaussian 
     * function
     */
    public double getTheta() {
        return _theta;
    }
    
    /**
     * @param z axial position in nm
     * @return a double array containing sigmaX & sigmaY in pixels from the 
     * provided axial position (in nm)
     */
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
    public double[] valDer(double z) {
        return new double[] {
            _wxSpline.value(z),
            _wySpline.value(z),
            _dwx.value(z),
            _dwy.value(z)
        };
    }
    
    /**
     * Show the sigmaX & sigmaY curves of the loaded calibration
     */
    public void plot() {
        int n = 100;
        double[] wx = new double[n];
        double[] wy = new double[n];
        double[] x = new double[n];
        for (int i = 0; i < n; i++) {
            final Double z = _minZ + i * (_maxZ-_minZ) / (n-1);
            x[i] = z;
            wx[i] = _wxSpline.value(z);
            wy[i] = _wySpline.value(z);
        }
        Plot p = new Plot("Calibration", "z position / nm", "width / pixels");
        p.setLineWidth(1);

        p.setColor(new Color(255, 0, 0));
        p.addPoints(x, wx, PlotWindow.LINE);

        p.setColor(new Color(0, 0, 255));
        p.addPoints(x, wy, PlotWindow.LINE);

        p.addLegend("SigmaX\nSigmaY");
        p.show();
    }
    
    /** 
     * Calculate cubic bspline polynomials from array values
     * @param x x-values
     * @param y y-values
     * @return PolynomialSplineFunction containing the bspline polynomials
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
    
    private void generateSplines() {
        int knots = 0;
        while(_parameters.containsKey("knot"+knots+"x")) {
            knots++;
        }
        double[] xKnots = new double[knots];
        double[] yKnots = new double[knots];
        double[] zKnots = new double[knots];
        for (int i = 0; i < knots; ++i) {
            xKnots[i] = _parameters.get("knot"+i+"x") / _pixelSizeUM;
            yKnots[i] = _parameters.get("knot"+i+"y") / _pixelSizeUM;
            zKnots[i] = _parameters.get("knot"+i+"z");
        }
        
        _minZ = zKnots[0];
        _maxZ = zKnots[zKnots.length-1];
        
        _wxSpline = generateSpline(zKnots, xKnots);
        _wySpline = generateSpline(zKnots, yKnots);
        
        _dwx = _wxSpline.derivative();
        _dwy = _wySpline.derivative();
    }
    
    private class FocusFunction implements UnivariateFunction {
        private final Calibration _cali;
        
        public FocusFunction(Calibration cali)
        {
            _cali = cali;
        }
        
        @Override
        public double value(double d) {
            double[] v = _cali.value(d);
            return Math.abs(v[0] - v[1]);
        }
    };
    
    private double calculateFocalPlane() {
        final double relativeTolerance = 1.49e-08;
        final double absoluteTolerance = 1.49e-08;
        FocusFunction fn = new FocusFunction(this);
        final BrentOptimizer optimizer = new BrentOptimizer(relativeTolerance, absoluteTolerance);
        UnivariatePointValuePair p = optimizer.optimize(
                new UnivariateObjectiveFunction(fn), 
                new SearchInterval(_minZ, _maxZ, (_maxZ-_minZ)/2), 
                new MaxEval(100),
                GoalType.MINIMIZE);
        return p.getPoint();
    }
    
}

