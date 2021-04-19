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

import ij.IJ;
import java.io.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * LUT implementation that loads a binary file containing the PSF at discrete 3D
 * positions including their derivatives. Can also be loaded using the CPP 
 * library (much faster!).
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
 * @author Fabian Hauser
 */
public class BinaryLUT implements LUT {
    private double _dLat;
    private double _dAx;
    private double _minLat;
    private double _maxLat;
    private double _minAx;
    private double _maxAx;
    private int _winSize;
    private int _countIndices;
    private String _fileName;
    private double[] _templates;
    
    public BinaryLUT() {
        
    }
    
    private double readDouble64(FileInputStream in) throws IOException {
        byte[] bytes = new byte[] {
            (byte)(in.read() & 0xff), (byte)(in.read() & 0xff),
            (byte)(in.read() & 0xff), (byte)(in.read() & 0xff),
            (byte)(in.read() & 0xff), (byte)(in.read() & 0xff),
            (byte)(in.read() & 0xff), (byte)(in.read() & 0xff)
        };
        return ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN).getDouble();
    }
    
    public boolean load(String fileName) {
        if (!loadHeader(fileName))
            return false;
        
        try {
            File file = new File(fileName);
            FileInputStream in = new FileInputStream(file);
            final long fileSize = file.length();
            final int size = (int)((fileSize - 64) / 8);
            _templates = new double[size];

            for (int i = 0; i < size; i++) {
                _templates[i] = readDouble64(in);
                IJ.showProgress(i, size);
            }
            return true;
        } catch (FileNotFoundException ex) {
        } catch (IOException ex) {
        }
        return false;
    }
    
    public boolean loadHeader(String fileName) {
        try {
            File file = new File(fileName);
            FileInputStream in = new FileInputStream(file);
            final long fileSize = file.length();
            
            byte[] hdr = new byte[64];
            if (in.read(hdr) != 64) {
                System.err.println("BinaryLUT: Could not read header!");
                return false;
            }
            if ((hdr[0] != 'L') || (hdr[1] != 'U') || (hdr[2] != 'T') || (hdr[3] != 'D') ||
                (hdr[4] != 'S') || (hdr[5] != 'M') || (hdr[6] != 'L') || (hdr[7] != 'M')) 
            {
                System.err.println("BinaryLUT: ID is not 'LUTDSMLM'!");
                return false;
            }
            
            ByteBuffer buf = ByteBuffer.wrap(hdr).order(ByteOrder.LITTLE_ENDIAN);
            
            final long dataSize = buf.getLong(8);
            _countIndices = (int)buf.getLong(16);
            _winSize = (int)buf.getLong(24);
            _dLat = buf.getDouble(32);
            _dAx = buf.getDouble(40);
            final double rangeLat = buf.getDouble(48);
            final double rangeAx = buf.getDouble(56);
            
            final double borderLat = Math.floor((_winSize - rangeLat) / 2);
            if (borderLat < 1.0)
                return false;
            
            _minLat = borderLat;
            _maxLat = _winSize - borderLat;
            
            _minAx = -0.5 * rangeAx;
            _maxAx = 0.5 * rangeAx;
            
            if (dataSize != (fileSize - 64)) {
                System.err.println("BinaryLUT: FileSize invalid!");
                return false;
            }
            
            _fileName = fileName;
            return true;
        } catch (FileNotFoundException ex) {
        } catch (IOException ex) {
        }
        return false;
    }
    
    /** 
     * Get the fileName from the header check
     * @return file name
     */
    public String getFileName() {
        return _fileName;
    }
    
    /**
     * Returns the number of unique templates in the lookup table
     * @return number of unique templates
     */
    public int getNumberOfIndices() {
        return _countIndices;
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
