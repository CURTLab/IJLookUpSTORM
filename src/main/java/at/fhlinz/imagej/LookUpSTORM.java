/****************************************************************************
 *
 * MIT License
 *
 * Copyright (C) 2020 Fabian Hauser
 *
 * Author: Fabian Hauser <fabian.hauser@fh-linz.at>
 * University of Applied Sciences Upper Austria - Linz - Austra
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

import static java.lang.Thread.sleep;

public class LookUpSTORM {
    
    public LookUpSTORM() {
        System.loadLibrary("LookUpSTORM_CPPDLL");
    }
    
    public native boolean setLocPara(double[][] lookupTable, int windowSize, double dLat, double dAx, double rangeLat, double rangeAx);
    public native boolean setLocParaArray(double[] lookupTable, int windowSize, double dLat, double dAx, double rangeLat, double rangeAx);
    public native boolean setLUT(String fileName);
    public native void setEpsilon(double eps);
    public native void setMaxIter(int maxIter);
    public native boolean isLocFinish();
    public native void setThreshold(int threshold);
    public native double[][] getLocs();
    public native int numberOfDetectedLocs();
    public native double[][] getAllLocs();
    public native int numberOfAllLocs();
    
    public native void setImagePara(int imageWidth, int imageHeight);
    public native int getImageWidth();
    public native int getImageHeight();
    private native boolean feedImageData(short data[], int frame);
    
    public native boolean isReady();
    public native void release();
    public native void reset();
    public native void setVerbose(boolean verbose);
    
    public native boolean setRenderImage(int[] pixels, int SMLMimageWidth, int SMLMimageHeigt);
    public native boolean releaseRenderImage();
    public native void setRenderSigma(float sigma);
    public native boolean isRenderingReady();
    public native void clearRenderingReady();
    public native int getRenderImageWidth();
    public native int getRenderImageHeight();
    public native float[] getRawRenderImage();
    
    public native int[] renderSMLMImage(int w, int h, double[][] xyz, int idxX, int idxY, int idxZ);
    
    public boolean feedImage(ij.process.ImageProcessor ip, int frame) {
        return feedImageData((short [])ip.getPixels(), frame);
    }
    
    public boolean waitForLocFinished(long sleepms, long timeoutms) {
        final long startTime = System.nanoTime();
        while (!isLocFinish()) {
            if (((System.nanoTime() - startTime) / 1E6) > timeoutms)
                return false;
            if (sleepms > 0) {
                try {
                    sleep(1); // sleep in ms
                } catch (InterruptedException ex) {
                    return false;
                }
            }
        }
        return true;
    }
    
    public boolean setLUT(LUT lut) {
        return setLocParaArray(lut.getLookUpTableArray(), lut.getWindowSize(), 
                               lut.getDeltaLat(), lut.getDeltaAx(), 
                               lut.getLateralRange(), lut.getAxialRange());
    }
}
