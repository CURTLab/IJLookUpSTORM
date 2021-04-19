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

import ij.IJ;

/**
 * LUT implementation that generate 2D elliptical Gaussian functions templates
 * from a calibration.
 * @author Fabian Hauser
 */
public class AstigmatismLUT implements LUT {
    private final Calibration _cali;
    private double _dLat;
    private double _dAx;
    private double _minLat;
    private double _maxLat;
    private double _minAx;
    private double _maxAx;
    private int _winSize;
    private int _countLat;
    private int _countAx;
    private int _countIndex;
    private int _pixels;
    private double[] _templates;
    
    /**
     * Constructor
     * @param cali calibration curves object
     */
    public AstigmatismLUT(Calibration cali) 
    {
        _cali = cali;
    }
    
    /** 
     * Populate the templates with 2D elliptical Gaussian functions that 
     * approximate the PSF
     * @param windowSize template window size
     * @param dLat lateral steps in pixels
     * @param dAx axial steps in nm
     * @param rangeLat lateral range in both direction around the center in pixels
     * @param rangeAx axial range in nm around the focus
     * @return 
     */
    public boolean populate(int windowSize, double dLat, double dAx, double rangeLat, double rangeAx)
    {
        final double borderLat = Math.floor((windowSize - rangeLat) / 2);
	if (borderLat < 1.0)
            return false;
        
        _winSize = windowSize;
        
	_minAx = -rangeAx * 0.5;
	_maxAx = rangeAx * 0.5;
        
        _dLat = dLat;
        _dAx = dAx;
        
	_minLat = borderLat;
	_maxLat = windowSize - borderLat;
        
        // calculate number of unique lateral and axial templates
        _countLat = (int)Math.floor((((getMaxLat() - getMinLat()) / dLat) + 1));
        _countAx = (int)Math.floor(((rangeAx / dAx) + 1));
        
        // number of unique template images
        _countIndex = _countLat * _countLat * _countAx;
        _pixels = windowSize*windowSize;
        
        // reset and force garbage collection
        _templates = null;
        System.gc();
        
        // allocate new array for tempaltes
        final int size = _countIndex * _pixels * 4;
        try {
            _templates = new double[size];
        }catch (OutOfMemoryError ex) {
            System.err.println("AstigmatismLUT: OutOfMemoryError: "
                                + ex.getMessage());
            return false;
        }
        if (_templates == null) {
            return false;
        }
	
        // possible angle of elliptical Gaussian function
        final double sina = Math.sin(_cali.getTheta());
        final double cosa = Math.cos(_cali.getTheta());

        // start generation
        for (int i = 0; i < _countIndex; i++) {
            final int zidx = i % _countAx;
            final int yidx = (i / _countAx) % _countLat;
            final int xidx = i / (_countAx * _countLat);

            final double x0 = _minLat + xidx * _dLat;
            final double y0 = _minLat + yidx * _dLat;
            final double z0 = _minAx + zidx * _dAx;

            // elliptical Gaussian sigma and derevatives
            final double[] sigma = _cali.valDer(z0 + _cali.getFocusPlane());
            final double sx = sigma[0], sy = sigma[1];
            final double sx2 = sx * sx, sx3 = sx2 * sx;
            final double sy2 = sy * sy, sy3 = sy2 * sy;
            final double dsx = sigma[2], dsy = sigma[3];
            
            int idx = 0;
            for (int py = 0; py < windowSize; py++) {
                final double y = (py - y0);
                for (int px = 0; px < windowSize; px++, idx += 4) {
                    final double x = (px - x0);

                    final double tx = x*cosa  + y*sina, tx2 = tx*tx;
                    final double ty = -x*sina + y*cosa, ty2 = ty*ty;
                    final double e = Math.exp(-0.5*tx2/sx2 - 0.5*ty2/sy2);

                    final int offset = _pixels * 4 * i;

                    _templates[offset + idx + 0] = e;
                    _templates[offset + idx + 1] = (tx*cosa/sx2 - ty*sina/sy2)*e;
                    _templates[offset + idx + 2] = (tx*sina/sx2 + ty*cosa/sy2)*e;
                    _templates[offset + idx + 3] = (tx2*dsx/sx3 + ty2*dsy/sy3)*e;
                }
            }
            IJ.showProgress(i, _countIndex);
        }
        return true;
    }

    /**
     * @return pointer to the generated lookup table array 
     */
    @Override
    public double[] getLookUpTableArray() {
        return _templates;
    }

    /**
     * @return get the minimum lateral position within the template in pixels
     */
    @Override
    public double getMinLat() {
        return _minLat;
    }

    /**
     * @return get the maximum lateral position within the template in pixels
     */
    @Override
    public double getMaxLat() {
        return _maxLat;
    }

    /**
     * @return get the minimum axial position in nm
     */
    @Override
    public double getMinAx() {
        return _minAx;
    }

    /**
     * @return get the maximum axial position in nm
     */
    @Override
    public double getMaxAx() {
        return _maxAx;
    }

    /**
     * @return get the window size of the templates in pixels
     */
    @Override
    public int getWindowSize() {
        return _winSize;
    }

    /**
     * @return get the lateral step size in pixels
     */
    @Override
    public double getDeltaLat() {
        return _dLat;
    }

    /**
     * @return get the axial step size in nm
     */
    @Override
    public double getDeltaAx() {
        return _dAx;
    }

    /**
     * @return get the lateral range within the template in pixels
     */
    @Override
    public double getLateralRange() {
        return _maxLat - _minLat - 1.0;
    }

    /**
     * @return get the axial range in nm
     */
    @Override
    public double getAxialRange() {
        return _maxAx - _minAx;
    }
}
