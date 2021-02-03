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

import ij.IJ;
import ij.ImageJ;
import ij.ImagePlus;
import ij.gui.GenericDialog;
import ij.plugin.filter.PlugInFilter;
import ij.process.ImageProcessor;

public class LookUpSTORM_ implements PlugInFilter {
    private ImagePlus _imagePlus;
    private int width;
    private int height;

    @Override
    public int setup(String string, ImagePlus ip) {
        _imagePlus = ip;
        return DOES_16;
    }

    @Override
    public void run(ImageProcessor ip) {
        // get width and height
        width = ip.getWidth();
        height = ip.getHeight();

        if (showDialog()) {
        }
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

       // open the Clown sample
       ImagePlus image = IJ.openImage("http://imagej.net/images/clown.jpg");
       image.show();

       // run the plugin
       IJ.runPlugIn(clazz.getName(), "");
   }
}
