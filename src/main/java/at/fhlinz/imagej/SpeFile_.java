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

/*
 * Orignal code from:
 *   Akira Goto (goto@w3bio.phys.tohoku.ac.jp)
 *   Holger Doerr (holger.doerr@gmx.de) 
 * Modified for our SPE fromat with meta information in the comments
 * Link: https://imagej.nih.gov/ij/plugins/spe.html
 */

package at.fhlinz.imagej;

import ij.IJ;
import ij.ImagePlus;
import ij.WindowManager;
import ij.io.FileInfo;
import ij.io.FileOpener;
import ij.io.ImageWriter;
import ij.io.OpenDialog;
import ij.io.SaveDialog;
import ij.plugin.PlugIn;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

public class SpeFile_ implements PlugIn {
    @Override
    public void run(String arg) {
        if (arg.equals("open")) {
            OpenDialog od = new OpenDialog("Open SPE...", arg);
            String file = od.getFileName();
            if (file == null) return;
            String directory = od.getDirectory();
            ImagePlus imp = open(directory, file);
            if (imp != null) {
                imp.show();
            } else {
                IJ.showMessage("Open SPE...", "Failed.");
            }
        } else if (arg.equals("save")) {
            ImagePlus imp = WindowManager.getCurrentImage();
            if (imp == null) {
                IJ.showMessage("Save as SPE...", "No images are open.");
                return;
            }
            if (speType(imp.getFileInfo().fileType) < 0) {
                IJ.showMessage("Save as SPE...",
                        "Supported types:\n" +
                        "\n" +
                        "32-bit Grayscale float : FLOAT\n" +
                        "(32-bit Grayscale integer) : LONG\n" +
                        "16-bit Grayscale integer: INT\n" +
                        "(16-bit Grayscale unsigned integer) : UNINT\n");
                return;
            }
            String name = imp.getTitle();
            SaveDialog sd = new SaveDialog("Save as SPE...", name, ".spe");
            String file = sd.getFileName();
            if (file == null) return;
            String directory = sd.getDirectory();
            save(imp, directory, file);
        }
    }
    
    public static ImagePlus open(String directory, String file) {
        File f = new File(directory, file);
        try {
            FileInputStream in = new FileInputStream(f);
            byte[] h = new byte[SpeHeader.HDRSIZE];
            in.read(h, 0, h.length);
            in.close();
            SpeHeader header = new SpeHeader(h);
            int speType = header.getDatatype();
            int fiType = fileInfoType(speType);
            if (fiType < 0) {
                IJ.showMessage("Open SPE...",
                        "Invalid data type.");
                return null;
            }
            FileInfo fi = new FileInfo();
            fi.directory = directory;
            fi.fileFormat = FileInfo.RAW;
            fi.fileName = file;
            fi.fileType = fiType;
            fi.gapBetweenImages = 0;
            fi.height = header.getHeight();
            fi.intelByteOrder = true;
            fi.nImages = header.getStackSize();
            fi.offset = SpeHeader.RAW_OFFSET;
            fi.width = header.getWidth();
            FileOpener fo = new FileOpener(fi);
            ImagePlus imp = fo.open(false);
            ij.measure.Calibration cal = new ij.measure.Calibration();
            cal.setUnit("um");
            cal.pixelWidth = cal.pixelHeight = header.getPixelSizeNM() / 1000.0;
            imp.setCalibration(cal);
            
            imp.setProperty("cameraPixelSize", header.getCameraPixelSize());
            imp.setProperty("magnification", header.getMagnification());
            imp.setProperty("EMGain", header.getEMGain());
            imp.setProperty("sensitivity", header.getSensitivity());
            
            IJ.showStatus("");
            return imp;
        } catch (IOException e) {
            IJ.error("An error occured reading the file.\n \n" + e);
            IJ.showStatus("");
            return null;
        }
    }
    
    public static void save(ImagePlus imp, String directory, String file) {
        if (imp == null) {
            IJ.showMessage("Save as SPE...", "No image selected.");
            return;
        }
        FileInfo fi = imp.getFileInfo();
        fi.intelByteOrder = true;
        int datatype = speType(fi.fileType);
        if (datatype < 0) {
            IJ.showMessage("Save as SPE...",
                    "Supported types:\n" +
                    "\n" +
                    "32-bit Grayscale float : FLOAT\n" +
                    "(32-bit Grayscale integer) : LONG\n" +
                    "16-bit Grayscale integer: INT\n" +
                    "(16-bit Grayscale unsigned integer) : UNINT\n");
            return;
        }
        SpeHeader header = new SpeHeader(datatype, 
                                         imp.getWidth(), 
                                         imp.getHeight(), 
                                         imp.getStackSize());
        File f = new File(directory, file);
        try {
            FileOutputStream out = new FileOutputStream(f);
            
            header.setDefaultComments();
            
            /*String prop1, prop2;
            prop1 = imp.getProp("cameraPixelSize");
            prop2 = imp.getProp("magnification");
            
            if (prop1.length() > 0 && prop2.length() > 0) {
                header.setPixelSizeNM(Double.parseDouble(prop1), 
                                      Integer.parseInt(prop2));
            }
            
            prop1 = (int)imp.getProperty("EMGain");
            if (prop1.length() > 0)
                header.setEMGain(Integer.parseInt(prop1));
            prop1 = imp.getProperty("sensitivity");
            if (prop1.length() > 0)
                header.setSensitivity(Double.parseDouble(prop1));*/
            
            byte[] h = header.getHeader();
            out.write(h, 0, h.length);
            ImageWriter writer = new ImageWriter(fi);
            writer.write(out);
            IJ.showStatus("");
        } catch (IOException e) {
            IJ.error("An error occured writing the file.\n \n" + e);
            IJ.showStatus("");
        }
    }
    
    public static int fileInfoType(int speType) {
        switch (speType) {
            case SpeHeader.FLOAT:
                    return FileInfo.GRAY32_FLOAT;
            case SpeHeader.LONG:
                    return FileInfo.GRAY32_INT;
            case SpeHeader.INT:
                    return FileInfo.GRAY16_SIGNED;
            case SpeHeader.UNINT:
                    return FileInfo.GRAY16_UNSIGNED;
            default:
                    return -1;
        }
    }
    
    public static int speType(int fiType) {
        switch (fiType) {
            case FileInfo.GRAY32_FLOAT:
                    return SpeHeader.FLOAT;
            case FileInfo.GRAY32_INT:
                    return SpeHeader.LONG;
            case FileInfo.GRAY16_SIGNED:
                    return SpeHeader.INT;
            case FileInfo.GRAY16_UNSIGNED:
                    return SpeHeader.UNINT;
            default:
                    return -1;
         }
    } 
}
