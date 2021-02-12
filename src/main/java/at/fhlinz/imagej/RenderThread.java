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

import ij.ImagePlus;
import ij.io.FileSaver;
import ij.process.ColorProcessor;

/**
 * Rendering thread for LookUpSTORM that handles the image data for rendering
 * @author Fabian Hauser
 */
public class RenderThread extends Thread {
    private final LookUpSTORM _lookUp;
    private ImagePlus _imagePlus;
    private boolean _running;
    private boolean _rendering;
    
    /** 
     * Constructor
     * @param lookup LookUpSTORM object
     */
    public RenderThread(LookUpSTORM lookup) {
        _lookUp = lookup;
        _imagePlus = null;
        _running = false;
        _rendering = false;
    }
    
    /** 
     * Set the rendering image size 
     * @param width
     * @param height 
     */
    public void setSize(int width, int height) {
        _lookUp.releaseRenderImage();
        ColorProcessor image = new ColorProcessor(width, height);
        _imagePlus = new ImagePlus("LookUpSTORM Rendering", image);
        _lookUp.setRenderImage((int[])image.getPixels(), 
                             width, height);
    }
    
    /** 
     * Set the ImagePlus window title to the image name
     * @param imageName image name
     */
    public void setImageName(String imageName) {
        _imagePlus.setTitle("LookUpSTORM Rendering - " + imageName);
    }
    
    /**
     * Save the current rendering as PNG
     * @param fileName output filename
     */
    public void save(String fileName) {
        new FileSaver(_imagePlus).saveAsPng(fileName);
    }
    
    /**
     * Starts the rendering, if the thread is not running it is started
     */
    public void startRendering() {
        _imagePlus.show();
        _rendering = true;
        if (!isAlive())
            start();
    }
    
    /**
     * Stops the rendering but does not stop the thread
     */
    public void stopRendering() {
        _rendering = false;
    }
    
    /**
     * Stops the thread, waits for thread termination and releases the rendered
     * image
     */
    public void release() {
        if (!_running) {
            return;
        }
        _running = false;
        while(isAlive()) {
            try {
                join();
            } catch (InterruptedException ex) {
            }
        }
        _lookUp.releaseRenderImage();
        if (_imagePlus != null) {
            _imagePlus.close();
        }
    }
    
    /**
     * Show the current rendered image
     */
    public void show() {
        _imagePlus.show();
    }
    
    /**
     * Internal thread loop
     */
    @Override
    public void run()
    {
        _running = true;
        while(_running)
        {
            if (_rendering && _lookUp.isRenderingReady() && (_imagePlus != null)) {
                //System.out.println("Render now!");
                
                _imagePlus.draw();  
                _imagePlus.updateImage();
                
                _lookUp.clearRenderingReady();
            }
            
            try {
                sleep(100); // sleep in ms
            } catch (InterruptedException ex) {

            }
        }
    }
}
