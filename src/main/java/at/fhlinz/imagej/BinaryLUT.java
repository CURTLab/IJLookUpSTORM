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
import java.io.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 *
 * @author Fabian Hauser
 */
public class BinaryLUT implements LUT {
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
    private String fileName_;
    
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
            templates_ = new double[size];

            for (int i = 0; i < size; i++) {
                templates_[i] = readDouble64(in);
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
            countIndex_ = (int)buf.getLong(16);
            winSize_ = (int)buf.getLong(24);
            dLat_ = buf.getDouble(32);
            dAx_ = buf.getDouble(40);
            final double rangeLat = buf.getDouble(48);
            final double rangeAx = buf.getDouble(56);
            
            final double borderLat = Math.floor((winSize_ - rangeLat) / 2);
            if (borderLat < 1.0)
                return false;
            
            minLat_ = borderLat;
            maxLat_ = winSize_ - borderLat;
            
            minAx_ = -0.5 * rangeAx;
            maxAx_ = 0.5 * rangeAx;

            countLat_ = (int)Math.floor((((getMaxLat() - getMinLat()) / dLat_) + 1));
            countAx_ = (int)Math.floor(((rangeAx / dAx_) + 1));
            
            if (dataSize != (fileSize - 64)) {
                System.err.println("BinaryLUT: FileSize invalid!");
                return false;
            }
            
            fileName_ = fileName;
            
            /*final int size = (int)(dataSize / 8);
            templates_ = new double[size];
            
            for (int i = 0; i < size; i++) {
                templates_[i] = readDouble64(in);
                IJ.showProgress(i, size);
            }*/
            return true;
        } catch (FileNotFoundException ex) {
        } catch (IOException ex) {
        }
        return false;
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
