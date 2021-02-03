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
 * 
 * @author Fabian Hauser
 */
public class AstigmatismLUT implements LUT {
    private final Calibration cali_;
    private double dLat_;
    private double dAx_;
    private double minLat_;
    private double maxLat_;
    private double minAx_;
    private double maxAx_;
    private int winSize_;
    private int countLat_;
    private int countAx_;
    private int countIndex_;
    private int pixels_;
    private double[] templates_;
    
    public AstigmatismLUT(Calibration cali) 
    {
        cali_ = cali;
    }
    
    private class Generator extends Thread {
        private final AstigmatismLUT d;
        
        Generator(AstigmatismLUT lut) {
            d = lut;
        }
        
        @Override
        public void run() {
            final double sina = Math.sin(d.cali_.getTheta());
            final double cosa = Math.cos(d.cali_.getTheta());
        
            for (int i = 0; i < d.countIndex_; i++) {
                final int zidx = i % d.countAx_;
                final int yidx = (i / d.countAx_) % d.countLat_;
                final int xidx = i / (d.countAx_ * d.countLat_);

                final double x0 = d.getMinLat() + xidx * d.getDeltaLat();
                final double y0 = d.getMinLat() + yidx * d.getDeltaLat();
                final double z0 = d.getMinAx() + zidx * d.getDeltaAx();

                final int windowSize = d.getWindowSize();
                
                double[] sigma = d.cali_.valDer(z0 + d.cali_.getFocusPlane());
                int idx = 0;
                for (int py = 0; py < windowSize; py++) {
                    final double y = (py - y0);
                    for (int px = 0; px < windowSize; px++, idx += 4) {
                        final double x = (px - x0);

                        final double tx = x*cosa  + y*sina, tx2 = tx*tx;
                        final double ty = -x*sina + y*cosa, ty2 = ty*ty;
                        final double sx2 = sigma[0]*sigma[0], sx3 = sx2 * sigma[0];
                        final double sy2 = sigma[1]*sigma[1], sy3 = sy2 * sigma[1];
                        final double e = Math.exp(-0.5*tx2/sx2 - 0.5*ty2/sy2);
                        
                        final int offset = pixels_ * 4 * i;
                        
                        d.templates_[offset + idx + 0] = e;
                        d.templates_[offset + idx + 1] = (tx*cosa/sx2 - ty*sina/sy2)*e;
                        d.templates_[offset + idx + 2] = (tx*sina/sx2 + ty*cosa/sy2)*e;
                        d.templates_[offset + idx + 3] = (tx2*sigma[2]/sx3 + ty2*sigma[3]/sy3)*e;
                    }
                }
                IJ.showProgress(i, d.countIndex_);
            }
        }
    }
    
    public boolean populate(int windowSize, double dLat, double dAx, double rangeLat, double rangeAx)
    {
        final double borderLat = Math.floor((windowSize - rangeLat) / 2);
	if (borderLat < 1.0)
            return false;
        
        winSize_ = windowSize;
        
	minAx_ = -rangeAx * 0.5;
	maxAx_ = rangeAx * 0.5;
        
        dLat_ = dLat;
        dAx_ = dAx;
        
	minLat_ = borderLat;
	maxLat_ = windowSize - borderLat;
        
        countLat_ = (int)Math.floor((((getMaxLat() - getMinLat()) / dLat) + 1));
        countAx_ = (int)Math.floor(((rangeAx / dAx) + 1));
        
        countIndex_ = countLat_ * countLat_ * countAx_;
        pixels_ = windowSize*windowSize;
        
        // reset and force garbage collection
        templates_ = null;
        System.gc();
        
        // allocate new array for tempaltes
        final int size = countIndex_ * pixels_ * 4;
        try {
            templates_ = new double[size];
        }catch (OutOfMemoryError ex) {
            return false;
        }
        if (templates_ == null) {
            return false;
        }
	
        // start generator thread
        Generator gen = new Generator(this);
        gen.start();
        try {
            gen.join();
            System.out.println("Done!");
            return true;
        } catch (InterruptedException ex) {
            return false;
        }
    }

    @Override
    public double[] getLookUpTableArray() {
        return templates_;
    }

    @Override
    public double getMinLat() {
        return minLat_;
    }

    @Override
    public double getMaxLat() {
        return maxLat_;
    }

    @Override
    public double getMinAx() {
        return minAx_;
    }

    @Override
    public double getMaxAx() {
        return maxAx_;
    }

    @Override
    public int getWindowSize() {
        return winSize_;
    }

    @Override
    public double getDeltaLat() {
        return dLat_;
    }

    @Override
    public double getDeltaAx() {
        return dAx_;
    }

    @Override
    public double getLateralRange() {
        return maxLat_ - minLat_ - 1.0;
    }

    @Override
    public double getAxialRange() {
        return maxAx_ - minAx_;
    }
}
