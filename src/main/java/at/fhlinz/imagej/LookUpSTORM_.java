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
import ij.plugin.PlugIn;
import javax.swing.JOptionPane;

public class LookUpSTORM_ implements PlugIn {
    private final LookUpSTORM _lookUpSTORM;
    private final DialogLookUpSTORM _frame;
    
    public LookUpSTORM_() {
        _lookUpSTORM = new LookUpSTORM();
        //_lookUpSTORM.setVerbose(true);
        
        _frame = new DialogLookUpSTORM(_lookUpSTORM);
    }

    @Override
    public void run(String arg) {
        if (arg.startsWith("about")) {
            JOptionPane.showMessageDialog(null, 
                        "LookUp STORM Version " + _lookUpSTORM.getVersion(), 
                        "About LookUpSTORM", 
                        JOptionPane.INFORMATION_MESSAGE);
        } else {
            if (arg.length() > 0) {
                _frame.setFileName(arg);
            }
            _frame.setVisible(true);
        }
    }
    
   /**
    * Main method for debugging.
    *
    * For debugging, it is convenient to have a method that starts ImageJ, loads
    * an image and calls the plugin, e.g. after setting breakpoints.
    *
    * @param args unused
     * @throws java.lang.Exception
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
       
       /*String path = "C:/Users/A41316/Desktop/FuE/Papers/RTStorm/Data/Sim/";
       String cali = "sequence-as-stack-Beads-AS-Exp.yaml";
       String img = "sunburst2.SPE";
       //ij.ImagePlus image = SpeFile_.open(path, "NewPlatelets_c500_f700_50mM_TestRT_red_015.SPE");
       ij.ImagePlus image = SpeFile_.open(path, img);
        //ij.ImagePlus image = SpeFile_.open(path, "cross.SPE");
       image.show();
       
       IJ.runPlugIn(clazz.getName(), path + cali);*/
   }
}
