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

import java.io.FileNotFoundException;
import java.io.PrintWriter;
import static java.lang.Thread.sleep;

public class LookUpSTORM {
    
    /** 
     * Constructor. Loads the CPPDLL
     */
    public LookUpSTORM() {
        System.loadLibrary("LookUpSTORM_CPPDLL");
    }
    
    /**
     * Sets the lookup table parameters for fitting. The lookupTable array has 
     * to contains the template images and derivations for the supplied 
     * axial and lateral steps. 
     * @param lookupTable
     * @param windowSize
     * @param dLat
     * @param dAx
     * @param rangeLat
     * @param rangeAx
     * @return Returns false if the LUT array size does not match the supplied
     * parameter
     */
    public native boolean setLookUpTable(double[] lookupTable, int windowSize, double dLat, double dAx, double rangeLat, double rangeAx);
    
    /** 
     * Sets the LUT table from a binary file containing the parameters and image
     * templates. All parameters in the header are in little endian. 
     * 
     * struct HeaderLUT {
     *    char id[8];
     *    uint64 dataSize;
     *    uint64 indices;
     *    uint64 windowSize;
     *    float64 dLat;
     *    float64 dAx;
     *    float64 rangeLat;
     *    float64 rangeAx;
     * }
     * 
     * @param fileName file name including the path to the LUT binary file.
     * @return returns true if the LUT file could be loaded
     */
    public native boolean setLookUpTable(String fileName);
    
    /** 
     * Releases the memory used for the internal LUT
     * @return returns true if the LUT memory could be released
     */
    public native boolean releaseLookUpTable();
    
    /** 
     * Residual squared difference stop value used by the Gauss-Newton 
     * algorithm.
     * @param eps Epsilon value (typical 1E-2) 
     */
    public native void setEpsilon(double eps);
    
    /** 
     * Sets the maximum iterations for the Gauss-Newton algorithm 
     * used to fit single molecules. (Thread-Safe)
     * @param maxIter Maximum iteration (typical 5)
     */
    public native void setMaxIter(int maxIter);
    
    /** 
     * Sets the threshold in counts of the gray scale image without the 
     * background. Should be interactivly (Thread-Safe)
     * @param threshold Threshold is a unsigned short value (0-65535)
     */
    public native void setThreshold(int threshold);
    
    /** 
     * Get the detected localizations of the currently fitted frame.
     * @return Array of localizations as double[7] array 
     * [bg,peak,x,y,z,frame,time]. x & y is returned as pixels, z in nm and 
     * time in µs
     * @see LookUpSTORM#reset()
     */
    public native double[][] getLocs();
    
    /** 
     * Get the number of detected localizations of the currently fitted frame.
     * @return Number of of detected localizations.
     */
    public native int numberOfDetectedLocs();
    
    /** 
     * Get the all detected localizations. This array is reseted by the reset 
     * method or setLookUpTable method.
     * @return Array of localizations as double[7] array 
     * [bg,peak,x,y,z,frame,time]. x & y is returned as pixels, z in nm and 
     * time in µs
     * @see LookUpSTORM#reset()
     */
    public native double[][] getAllLocs();
    
    /** 
     * Get the number of all detected localizations. This number is reseted by 
     * the reset method or setLookUpTable method.
     * @return Number of all detected localizations since reset
     * @see LookUpSTORM#reset()
     */
    public native int numberOfAllLocs();
    
    /** 
     * 
     * @param imageWidth
     * @param imageHeight 
     */
    public native void setImagePara(int imageWidth, int imageHeight);
    
    /** 
     * 
     * @return 
     */
    public native int getImageWidth();
    
    /** 
     * 
     * @return 
     */
    public native int getImageHeight();
    
    /** 
     * 
     * @param data
     * @param frame
     * @return 
     */
    public native boolean feedImageData(short data[], int frame);
    
    /** 
     * Flag to indicate when the feeded image is fitted and processed
     * @return Returns true if the feeded image is fitted and processed
     * @see LookUpSTORM#feedImage(ij.process.ImageProcessor, int) 
     * @see LookUpSTORM#feedImageData(short[], int) 
     */
    public native boolean isLocFinish();
    
    /**
     * Checks if all needed parameters are set
     * @return True if LUT, image parameter and render image are set
     * @see LookUpSTORM#setLUT(at.fhlinz.imagej.LUT) 
     * @see LookUpSTORM#setImagePara(int, int) 
     * @see LookUpSTORM#setRenderImage(int[], int, int) 
     */
    public native boolean isReady();
    
    /** 
     * Releases all the allocated memory of the CPPDLL
     */
    public native void release();
    
    /** 
     * Resets the isLocFinish and isRenderingReady flag to false, clears the 
     * array of detected localizations and fills rendered image with zeros
     */
    public native void reset();
    
    /** 
     * Enable or disable CPPDLL output on std::cout
     * @param verbose If true CPPDLL will output information on std::cout
     */
    public native void setVerbose(boolean verbose);
    
    /** 
     * Sets a pointer (should be uint32) to a allocated image in which the
     * results of the real time analysis are rendered(render target)
     * @param pixels pointer to a allocated image (uint32)
     * @param SMLMimageWidth supplied image width
     * @param SMLMimageHeigt supplied image height
     * @return Returns false if the image size does not match the array size
     * @see LookUpSTORM#releaseRenderImage() 
     */
    public native boolean setRenderImage(int[] pixels, int SMLMimageWidth, int SMLMimageHeigt);
    
    /** 
     * Release the memory of the set render image target
     * @return Returns false if no memory was in used
     * @see LookUpSTORM#setRenderImage(int[], int, int) 
     */
    public native boolean releaseRenderImage();
    
    /** 
     * Set the sigma of the Gaussian rendering in pixels of the render image.
     * Caution: Only 3x3 pixels are used for Gaussian histogram rendering!
     * @param sigma Sigma in pixels
     */
    public native void setRenderSigma(float sigma);
    
    /** 
     * Return true if a the rendered image has changed and needs to be redrawn.
     * Rendering is automated and happens if enough new localizations are 
     * detected.
     * @return Return true if render image needs to be redrawn.
     * @see LookUpSTORM#feedImage(ij.process.ImageProcessor, int) 
     * @see LookUpSTORM#feedImageData(short[], int) 
     */
    public native boolean isRenderingReady();
    
    /** 
     * Sets all pixels of the rendering histogram to zero
     */
    public native void clearRenderingReady();
    
    /** 
     * @return Render image width
     * @see LookUpSTORM#setRenderImage(int[], int, int) 
     */
    public native int getRenderImageWidth();
    
    /** 
     * @return Render image height
     * @see LookUpSTORM#setRenderImage(int[], int, int) 
     */
    public native int getRenderImageHeight();
    
    /**
     * @return Returns the version of LookUpSTORM CPPDLL
     */
    public native String getVersion();
    
    /**
     * Calculate the bytes needed for the LUT template array with the supplied
     * parameters.
     * @param windowSize template window size
     * @param dLat lateral steps in pixels
     * @param dAx axial steps in nm
     * @param rangeLat lateral range in both direction around the center in pixels
     * @param rangeAx axial range in nm around the focus
     * @return Number of bytes used for LUT array.
     */
    public static long calculateBytesForLUT(int windowSize, double dLat, 
            double dAx, double rangeLat, double rangeAx) 
    {
        final long countLat = (long)Math.floor(((rangeLat + 1.0) / dLat) + 1);
        final long countAx = (long)Math.floor(((rangeAx / dAx) + 1));
        return countLat * countLat * countAx * windowSize * windowSize * 4 * 8;
    }
    
    /**
     * Feed a imagej image to the CPPDLL for fitting
     * @param ip 16-bit unsigned short image (0-65535)
     * @param frame Index of supplied image (zero starting index)
     * @return true if image could be processed (size is the same as set)
     * @see LookUpSTORM#setImagePara(int, int) 
     * @see LookUpSTORM#feedImageData(short[], int) 
     */
    public boolean feedImage(ij.process.ImageProcessor ip, int frame) {
        return feedImageData((short [])ip.getPixels(), frame);
    }
    
    /**
     * Helper function to set the LUT
     * @param lut Java LUT interface
     * @return Returns true if LUT is valid and could be set
     */
    public boolean setLUT(LUT lut) {
        return setLookUpTable(lut.getLookUpTableArray(), lut.getWindowSize(), 
                              lut.getDeltaLat(), lut.getDeltaAx(), 
                              lut.getLateralRange(), lut.getAxialRange());
    }
    
    /**
     * Helper function to wait until feeded image is fitted and processes
     * @param sleepms loop wait time in ms (default 1 ms)
     * @param timeoutms timeout in ms to stop the loop
     * @return Return false if the timeout is triggered
     * @see LookUpSTORM#feedImage(ij.process.ImageProcessor, int) 
     * @see LookUpSTORM#feedImageData(short[], int) 
     */
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
    
    /** 
     * Save all detected localizations since the last reset to a CSV file.
     * Header: index,frame,x_nm,y_nm,z_nm,intensity,background,time
     * @param fileName Output CSV file name.
     * @param pixelSize Pixels size of the input image in nm
     * @return 
     * @see LookUpSTORM#getAllLocs() 
     */
    public boolean saveLocsCSV(String fileName, double pixelSize) {
        double[][] locs = getAllLocs();
        try {
            PrintWriter stream = new PrintWriter(fileName);
            stream.write("index,frame,x_nm,y_nm,z_nm,intensity,background,time\n");
            int idx = 1;
            for (double[] l:locs) {
                // [bg,peak,x,y,z,frame,time]
                StringBuilder builder = new StringBuilder();
                builder.append(idx++);
                builder.append(',');
                builder.append(l[6] + 1);
                builder.append(',');
                builder.append(l[2] * pixelSize);
                builder.append(',');
                builder.append(l[3] * pixelSize);
                builder.append(',');
                builder.append(l[4]);
                builder.append(',');
                builder.append(l[1]);
                builder.append(',');
                builder.append(l[0]);
                builder.append(',');
                builder.append(l[6]);
                builder.append('\n');
                stream.write(builder.toString());
            }
            stream.close();
            return true;
        } catch (FileNotFoundException ex) {
            System.out.println("LookUpSTORM: Could not save file " + fileName);
            return false;
        }
    }
}
