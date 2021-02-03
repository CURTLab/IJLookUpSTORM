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
import ij.ImageJ;
import ij.ImagePlus;
import ij.ImageStack;
import ij.gui.GenericDialog;
import ij.plugin.filter.PlugInFilter;
import ij.process.ImageProcessor;

public class LookUpSTORM_ implements PlugInFilter {
    private LookUpSTORM _lookUpSTORM;
    private ImagePlus _imagePlus;
    private RenderThread _renderThread;

    @Override
    public int setup(String string, ImagePlus ip) {
        _lookUpSTORM = new LookUpSTORM();
        _renderThread = new RenderThread(_lookUpSTORM);
        _imagePlus = ip;
        return DOES_16;
    }
    
    @Override
    public void run(ImageProcessor ip) {
        ImageStack stack = _imagePlus.getStack();
        
        if (stack.getBitDepth() != 16) {
            IJ.showMessage("LookUpSTORM error", "Image type is not 16 bit grayscale!");
            return;
        }
        
        //String path = "C:/Users/A41316/Desktop/FuE/Papers/RTStorm/Data/Platelets/";
        String path = "C:/Users/A41316/Desktop/FuE/Papers/RTStorm/Data/Sim/";
        Calibration cali = new Calibration();
        //cali.load(path + "Calib_f700_c500_closeToFar_red_002.yaml");
        cali.load(path + "sequence-as-stack-Beads-AS-Exp.yaml");
        cali.plot();
       
        AstigmatismLUT lut = new AstigmatismLUT(cali);
        if (!lut.populate(9, 0.1, 25.0, 4.0, 1000.0)) {
            IJ.showMessage("LookUpSTORM error", "Could not populate LUT!");
            return;
        }
        _lookUpSTORM.setLUT(lut);
        
        _lookUpSTORM.setImagePara(ip.getWidth(), ip.getHeight());
        _lookUpSTORM.setVerbose(true);
        
        /*final int threshold = (int)spinnerThreshold_.getValue();
        final int maxIter = (int)spinnerMaxIter_.getValue();
        final int scale = (int)spinnerScale_.getValue();
        final double eps = (double)spinnerEps_.getValue();
        _lookUpSTORM.setThreshold(threshold);
        _lookUpSTORM.setEpsilon(eps);
        _lookUpSTORM.setMaxIter(maxIter);**/
        final int scale = 10;
        _lookUpSTORM.setThreshold(5);
        _renderThread.setSize(stack.getWidth() * scale, stack.getHeight() * scale);
        
        _renderThread.startRendering();
        Runnable fitter = () -> {
            //startAnalysisGUI();
            //startButton_.setEnabled(false);
            final int frames = stack.getSize();
            final long t0 = System.nanoTime();
            _lookUpSTORM.reset();
            for (int frame = 0; frame < frames; frame++) {
                ImageProcessor img = stack.getProcessor(frame + 1);
                if (!_lookUpSTORM.feedImage(img, frame)) 
                    continue;
                _lookUpSTORM.waitForLocFinished(1, 100);
                IJ.showProgress(frame, frames);
            }
            _renderThread.stopRendering();
            final long t1 = System.nanoTime();
            //stopAnalysisGUI();
            //startButton_.setEnabled(true);
            System.out.println("Found " + _lookUpSTORM.numberOfAllLocs() + " emitters in " + (t1 - t0) / 1E9 + " s!");
            IJ.showStatus("Found " + _lookUpSTORM.numberOfAllLocs() + " emitters in " + (t1 - t0) / 1E9 + " s!");
        };
        (new Thread(fitter)).start();
    }

    private boolean showDialog() {
        GenericDialog gd = new GenericDialog("Process pixels");

        // default value is 0.00, 2 digits right of the decimal point
        gd.addNumericField("value", 0.00, 2);
        gd.addStringField("name", "John");

        gd.showDialog();
        if (gd.wasCanceled())
            return false;

        // get entered values
        //value = gd.getNextNumber();
        //name = gd.getNextString();

        return true;
    }
    
   /**
    * Main method for debugging.
    *
    * For debugging, it is convenient to have a method that starts ImageJ, loads
    * an image and calls the plugin, e.g. after setting breakpoints.
    *
    * @param args unused
    */
   public static void main(String[] args) throws Exception {
       // set the plugins.dir property to make the plugin appear in the Plugins menu
       // see: https://stackoverflow.com/a/7060464/1207769
       Class<?> clazz = LookUpSTORM_.class;
       java.net.URL url = clazz.getProtectionDomain().getCodeSource().getLocation();
       java.io.File file = new java.io.File(url.toURI());
       System.setProperty("plugins.dir", file.getAbsolutePath());

       // start ImageJ
       new ImageJ();
       
       String path = "C:/Users/A41316/Desktop/FuE/Papers/RTStorm/Data/Sim/";
       //ij.ImagePlus image = SpeFile.open(path, "NewPlatelets_c500_f700_50mM_TestRT_red_015.SPE");
       ij.ImagePlus image = SpeFile.open(path, "sunburst2.SPE");
        //ij.ImagePlus image = SpeFile.open(path, "cross.SPE");
       image.show();
       
       IJ.runPlugIn(clazz.getName(), "");
       
       /*LUT lut = new LUT(cali);
       lut.populate(9, 0.1, 25.0, 4.0, 1000.0);

       //*/


       // open the Clown sample
       //ImagePlus image = IJ.openImage("http://imagej.net/images/clown.jpg");
       //image.show();

       // run the plugin
       //IJ.runPlugIn(clazz.getName(), "");
   }
}
