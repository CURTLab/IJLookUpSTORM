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

import java.io.File;
import java.io.FileInputStream;
import java.util.HashMap;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.apache.commons.math3.analysis.UnivariateFunction;
import org.apache.commons.math3.optim.MaxEval;
import org.apache.commons.math3.optim.nonlinear.scalar.GoalType;
import org.apache.commons.math3.optim.univariate.BrentOptimizer;
import org.apache.commons.math3.optim.univariate.SearchInterval;
import org.apache.commons.math3.optim.univariate.UnivariateObjectiveFunction;
import org.apache.commons.math3.optim.univariate.UnivariatePointValuePair;

/**
 *
 * @author Fabian Hauser
 */
public class CalibrationFactory {    
    /**
     * Load a calibration YAML file. Supported are cubic b-spline representation 
     * (at.calibration.bspline) and DaostormCalibration/PolynomialCalibration 
     * from ThunderSTORM (https://github.com/zitmen/thunderstorm)
     * Needed parameters are 'pixelSize' in nm and 'angle'/'theta'
     * @param fileName YAML file with parameter
     * @return a calibration class or null if no valid calibration was found
     */
    public Calibration load(String fileName)
    {
        try {
            // read string data
            File file = new File(fileName);   
            FileInputStream stream = new FileInputStream(file);
            byte [] bytes = new byte[(int)file.length()];
            if (stream.read(bytes) != file.length()) {
                stream.close();
                return null;
            }
            String data = new String(bytes, "UTF8");
            stream.close();
            
            Matcher matcher = Pattern.compile("!!([\\w\\.]+)\\s*").matcher(data);
            if (!matcher.find())
                return null;
            
            Calibration cali;
            String typeString = matcher.group(1);
            switch (typeString) {
                case "at.calibration.bspline":
                    cali = new BSplineCalibration();
                    break;
                case "cz.cuni.lf1.lge.ThunderSTORM.calibration.PolynomialCalibration":
                    cali = new ThunderStormCalibration();
                    break;
                case "cz.cuni.lf1.lge.ThunderSTORM.calibration.DaostormCalibration":
                    cali = new DaoStormCalibration();
                    break;
                default:
                    return null;
            }
            
            HashMap<String, Double> parameters = new HashMap<String, Double>();
            
            // parse parameters from yaml file
            Pattern p = Pattern.compile("(\\w*):\\s*([\\-]*[0-9+][.\\w]*[-\\w]*)");
            matcher = p.matcher(data.substring(matcher.end()));
            while (matcher.find()) {
                parameters.put(matcher.group(1), Double.parseDouble(matcher.group(2)));
            }
            
            // parse parameters to generate calibration
            if (parameters.isEmpty() || !cali.parse(parameters))
                return null;
            
            return cali;
        } catch(Exception e) {
            return null;
        }
    }
    
    private static class FocusFunction implements UnivariateFunction {
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
    
    
    /**
     * Calculate the focal plane based on the provided calibration by 
     * determination of the axial position where sigmaX and sigmaY are the same
     * @param cali astigmatism calibration class
     * @param minZ minimum axial position of search
     * @param maxZ maximum axial position of search
     * @return Axial position of focal plane
     */
    static public double calculateFocalPlane(Calibration cali, double minZ, double maxZ) {
        final double relativeTolerance = 1.49e-08;
        final double absoluteTolerance = 1.49e-08;
        FocusFunction fn = new FocusFunction(cali);
        final BrentOptimizer optimizer = new BrentOptimizer(relativeTolerance, 
                absoluteTolerance);
        UnivariatePointValuePair p = optimizer.optimize(
                new UnivariateObjectiveFunction(fn), 
                new SearchInterval(minZ, maxZ, (maxZ - minZ)/2), 
                new MaxEval(100),
                GoalType.MINIMIZE);
        return p.getPoint();
    }
}
