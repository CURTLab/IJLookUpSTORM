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
import ij.ImagePlus;
import ij.ImageStack;
import ij.WindowManager;
import ij.process.ImageProcessor;
import java.awt.Dimension;
import java.awt.GridLayout;
import java.io.File;
import javax.swing.BorderFactory;
import javax.swing.BoxLayout;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JComboBox;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JSpinner;
import javax.swing.JTabbedPane;
import javax.swing.JTextField;
import javax.swing.SpinnerNumberModel;
import javax.swing.border.EmptyBorder;
import javax.swing.event.ChangeListener;

/**
 * LookUpSTORM GUI and fitting handlers
 * @author A41316
 */
public class LookUpSTORMFrame extends JFrame {
    private final LookUpSTORM _lookUpSTORM;
    private final JTextField _outputPathField;
    private final JTabbedPane _pane;
    private final JComboBox _comboSource;
    private final JComboBox _comboWinSize;
    private final JSpinner _spinnerLatDelta;
    private final JSpinner _spinnerLatRange;
    private final JSpinner _spinnerAxDelta;
    private final JSpinner _spinnerAxRange;
    private final JTextField _fileNameField;
    private final JTextField _ramField;
    private final JButton _buttonGenerate;
    private final JSpinner _spinnerThreshold;
    private final JSpinner _spinnerScale;
    private final JSpinner _spinnerMaxIter;
    private final JSpinner _spinnerEps;
    private final JCheckBox _checkSaveRend;
    private final JCheckBox _checkSaveLocs;
    private final JCheckBox _checkSaveCurves;
    private final JButton _fitButton;
    private final RenderThread _renderThread;
    private Calibration _cali;
    
    LookUpSTORMFrame(LookUpSTORM lookUpSTORM) {
        _lookUpSTORM = lookUpSTORM;
        _renderThread = new RenderThread(_lookUpSTORM);
        
        super.setTitle("LookUpSTORM");
        super.getContentPane().setLayout(new BoxLayout(super.getContentPane(), BoxLayout.Y_AXIS));
        
        JPanel topPanel = new JPanel();
        topPanel.setLayout(new BoxLayout(topPanel, BoxLayout.LINE_AXIS));
        
        topPanel.add(label("Output Path:"));
        
        _outputPathField = new JTextField();
        _outputPathField.setMaximumSize(new Dimension(2147483647, 25));
        topPanel.add(_outputPathField);
        
        JButton buttonSelOutput = new JButton("...");
        topPanel.add(buttonSelOutput);
        super.add(topPanel);
        
        buttonSelOutput.addActionListener((ActionEvent) -> {
            doSelectOutputDir();
        });
        
        _pane = new JTabbedPane
            (JTabbedPane.BOTTOM,JTabbedPane.SCROLL_TAB_LAYOUT );
        
        // lookup table parameters
        JPanel panelLUT = new JPanel();
        {
            panelLUT.setLayout(new GridLayout(9, 2, 5, 5));
            
            ChangeListener listener = (ChangeEvent) -> {
                updateRAMUsage();
            };

            panelLUT.add(label("Source:"));
            _comboSource = new JComboBox(new String[]{"Astigmatism", "From Binary File"});
            panelLUT.add(_comboSource);

            panelLUT.add(label("Window Size:"));
            _comboWinSize = new JComboBox(new String[]{"9", "11"});
            panelLUT.add(_comboWinSize);

            panelLUT.add(label("Lateral Delta:"));
            _spinnerLatDelta = new JSpinner();
            _spinnerLatDelta.setModel(new SpinnerNumberModel(0.1d, 0.01d, 2.0d, 0.1d));
            panelLUT.add(_spinnerLatDelta);
            _spinnerLatDelta.addChangeListener(listener);

            panelLUT.add(label("Lateral Range:"));
            _spinnerLatRange = new JSpinner();
            _spinnerLatRange.setModel(new SpinnerNumberModel(4.0d, 1.0d, 9.0d, 1.0d));
            panelLUT.add(_spinnerLatRange);
            _spinnerLatRange.addChangeListener(listener);

            panelLUT.add(label("Axial Delta:"));
            _spinnerAxDelta = new JSpinner();
            _spinnerAxDelta.setModel(new SpinnerNumberModel(25.0d, 1.0d, 500.0d, 10.0d));
            panelLUT.add(_spinnerAxDelta);
            _spinnerAxDelta.addChangeListener(listener);

            panelLUT.add(label("Axial Range:"));
            _spinnerAxRange = new JSpinner();
            _spinnerAxRange.setModel(new SpinnerNumberModel(1000.0d, 0.0d, 50000.d, 100.d));
            panelLUT.add(_spinnerAxRange);
            _spinnerAxRange.addChangeListener(listener);
            
            panelLUT.add(label("FileName:"));
            JPanel fpanel = new JPanel();
            {
                fpanel.setLayout(new BoxLayout(fpanel, BoxLayout.LINE_AXIS));

                _fileNameField = new JTextField();
                _fileNameField.setMaximumSize(new Dimension(2147483647, 25));
                fpanel.add(_fileNameField);

                JButton buttonSelFileName = new JButton("...");
                fpanel.add(buttonSelFileName);
                
                buttonSelFileName.addActionListener((ActionEvent) -> {
                    doSelectFileName();
                });
            }
            
            panelLUT.add(fpanel);

            panelLUT.add(label("RAM Usage:"));
            _ramField = new JTextField();
            panelLUT.add(_ramField);
            
            panelLUT.add(new JPanel());
            _buttonGenerate = new JButton("Generate");
            _buttonGenerate.setEnabled(false);
            panelLUT.add(_buttonGenerate);
            
            _buttonGenerate.addActionListener((ActionEvent) -> {
                doGenerate();
            });
        }
        _pane.addTab("LUT", panelLUT);
        
        // panel general, includes fitting parameters
        JPanel panelGeneral = new JPanel();
        {
            GridLayout layout = new GridLayout(9, 2, 5, 5);
            panelGeneral.setLayout(layout);

            panelGeneral.add(label("Threshold:"));
            _spinnerThreshold = new JSpinner();
            _spinnerThreshold.setModel(new SpinnerNumberModel(40, 1, 13000, 1));
            panelGeneral.add(_spinnerThreshold);
            
            _spinnerThreshold.addChangeListener((ChangeEvent) -> {
                _lookUpSTORM.setThreshold((int)_spinnerThreshold.getValue());
            });

            panelGeneral.add(label("Render Scale:"));
            _spinnerScale = new JSpinner();
            _spinnerScale.setModel(new SpinnerNumberModel(10, 1, 100, 1));
            panelGeneral.add(_spinnerScale);
            
            panelGeneral.add(label("Max Iterations:"));
            _spinnerMaxIter = new JSpinner();
            _spinnerMaxIter.setModel(new SpinnerNumberModel(5, 1, 50, 1));
            panelGeneral.add(_spinnerMaxIter);
            
            _spinnerMaxIter.addChangeListener((ChangeEvent) -> {
                _lookUpSTORM.setMaxIter((int)_spinnerMaxIter.getValue());
            });

            panelGeneral.add(label("Delta Epsilon:"));
            _spinnerEps = new JSpinner();
            _spinnerEps.setModel(new SpinnerNumberModel(1E-2d, 1E-10d, 1E3d, 1E-1d));
            panelGeneral.add(_spinnerEps);
            
            _spinnerEps.addChangeListener((ChangeEvent) -> {
                _lookUpSTORM.setEpsilon((double)_spinnerEps.getValue());
            });
            
            _checkSaveRend = new JCheckBox("Save rendered image");
            _checkSaveRend.setSelected(true);
            panelGeneral.add(_checkSaveRend);
            
            _checkSaveLocs = new JCheckBox("Save localizations");
            _checkSaveLocs.setSelected(true);
            panelGeneral.add(_checkSaveLocs);
            
            _checkSaveCurves = new JCheckBox("Save curves");
            panelGeneral.add(_checkSaveCurves);
        }
        _pane.addTab("General", panelGeneral);
        
        super.add(_pane);
        
        JPanel buttonPanel = new JPanel();
        buttonPanel.setLayout(new BoxLayout(buttonPanel, BoxLayout.LINE_AXIS));
        
        // main button
        _fitButton = new JButton("Fit current");
        _fitButton.setEnabled(false);
        buttonPanel.add(_fitButton);
        
        _fitButton.addActionListener((ActionEvent) -> {
            doFitCurrent();
        });
        
        updateRAMUsage();
        
        buttonPanel.setBorder(BorderFactory.createEmptyBorder(5, 5, 5, 5));
        
        super.add(buttonPanel);
        
        super.setSize(300, 450);
        
        super.pack();
        
        // close window listener
        super.addWindowListener(new java.awt.event.WindowAdapter() {
            @Override
            public void windowClosing(java.awt.event.WindowEvent e) {
                doRelease();
            }
            @Override
            public void windowClosed(java.awt.event.WindowEvent e) {
            }
        });
    }
    
    public void setFileName(String fileName) {
        if (fileName.toLowerCase().endsWith(".yaml"))
            _comboSource.setSelectedIndex(0);
        else if (fileName.toLowerCase().endsWith(".lut"))
            _comboSource.setSelectedIndex(1);
        final String selectSource = (String)_comboSource.getSelectedItem();
        if (checkLUTData(fileName, selectSource))
            _fileNameField.setText(fileName);
    }
    
    public void setOutputPath(String path) {
        _outputPathField.setText(path);
    }
    
    private static JLabel label(String text) {
        JLabel l = new JLabel(text);
        l.setBorder(new EmptyBorder(0, 5, 0, 5));
        return l;
    }
    
    private void doSelectOutputDir() {
        JFileChooser f = new JFileChooser();
        f.setCurrentDirectory(new java.io.File(_outputPathField.getText()));
        f.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY); 
        f.setAcceptAllFileFilterUsed(false);
        if (f.showSaveDialog(this) == JFileChooser.APPROVE_OPTION) {
            _outputPathField.setText(f.getSelectedFile().toString());
        }
    }
    
    private void doSelectFileName() {
        JFileChooser f = new JFileChooser();
        final String selectSource = (String)_comboSource.getSelectedItem();
        f.setAcceptAllFileFilterUsed(false);

        switch (selectSource) {
            case "From Binary File":
                f.addChoosableFileFilter(new FileFilterEx(".lut", "Binary LUT File (.lut)"));
                break;
            case "Astigmatism":
                f.addChoosableFileFilter(new FileFilterEx(".yaml", "YAML Calibration (.yaml)"));
                break;
            default:
                f.setAcceptAllFileFilterUsed(true);
                break;
        }

        f.setCurrentDirectory(new java.io.File(_outputPathField.getText()));
        if (f.showSaveDialog(this) == JFileChooser.APPROVE_OPTION) 
        {
            final String fileName = f.getSelectedFile().toString();
            if (checkLUTData(fileName, selectSource)) { 
                _fileNameField.setText(fileName);
            } else {
                JOptionPane.showMessageDialog(this, 
                        "Check for file " + fileName + " failed!", "Error LookUpSTORM", 
                        JOptionPane.ERROR_MESSAGE);
            }
        }
    }
    
    private void doGenerate() {
        final String selectedSource = (String)_comboSource.getSelectedItem();
        _fitButton.setEnabled(false);
        if (selectedSource.equals("Astigmatism")) {
            if (_cali == null) {
                JOptionPane.showMessageDialog(this, 
                        "Calibration for AstigmatismLUT is not loaded!", "Error LookUpSTORM", 
                        JOptionPane.ERROR_MESSAGE);
                return;
            }
            final double dLat = (double)_spinnerLatDelta.getValue();
            final double rangeLat = (double)_spinnerLatRange.getValue();
            final double dAx = (double)_spinnerAxDelta.getValue();
            final double rangeAx = (double)_spinnerAxRange.getValue();
            final int winSize = _comboWinSize.getSelectedIndex() == 1 ? 11 : 9;
            AstigmatismLUT lut = new AstigmatismLUT(_cali);
            
            // check memory
            final long usage = LookUpSTORM.calculateBytesForLUT(winSize, dLat, dAx, rangeLat, rangeAx);
            final long heap = Runtime.getRuntime().maxMemory();
            if (usage > Integer.MAX_VALUE) {
                JOptionPane.showMessageDialog(this, 
                        "LUT size exceeds maximum array size!", "Error LookUpSTORM", 
                        JOptionPane.ERROR_MESSAGE);
                return;
            } else if (usage > heap) {
                JOptionPane.showMessageDialog(this, 
                        "LUT size exceeds free heap size!", "Error LookUpSTORM", 
                        JOptionPane.ERROR_MESSAGE);
                return;
            } 
            // finally populate the LUT in a thread
            Runnable generator = () -> {
                final long t0 = System.nanoTime();
                if (!lut.populate(winSize, dLat, dAx, rangeLat, rangeAx)) {
                    JOptionPane.showMessageDialog(this, 
                            "Could not generate AstigmatismLUT!", "Error LookUpSTORM", 
                            JOptionPane.ERROR_MESSAGE);
                    return;
                }
                if (!_lookUpSTORM.setLUT(lut)) {
                    JOptionPane.showMessageDialog(this, 
                            "Failed to set AstigmatismLUT for DLL!", "Error LookUpSTORM", 
                            JOptionPane.ERROR_MESSAGE);
                    return;
                }
                final long t1 = System.nanoTime();
                IJ.showStatus("Generated LUT in " + (t1 - t0) / 1E9 + " s!");
                System.out.println("Generated LUT in " + (t1 - t0) / 1E9 + " s!");
                _fitButton.setEnabled(true);
            };
            (new Thread(generator)).start();
        } else if (selectedSource.equals("From Binary File")) {
            final String fileName = _fileNameField.getText();
            if (!(new File(fileName)).exists()) {
                JOptionPane.showMessageDialog(this, 
                        "No binary file selected!", "Error LookUpSTORM", 
                        JOptionPane.ERROR_MESSAGE);
                return;
            }  
            if (!_lookUpSTORM.setLookUpTable(fileName)) {
                JOptionPane.showMessageDialog(this, 
                        "Failed to set BinaryLUT for DLL!", "Error LookUpSTORM", 
                        JOptionPane.ERROR_MESSAGE);
                return;
            }
            _fitButton.setEnabled(true);
        }
    }
    
    private void doFitCurrent() {
        ImagePlus imp = WindowManager.getCurrentImage();
        if (imp == null) {
            JOptionPane.showMessageDialog(this, 
                    "There are no images opened!", "Error LookUpSTORM", 
                    JOptionPane.ERROR_MESSAGE);
            return;
        }
        if (imp.getType() != ImagePlus.GRAY16) {
            JOptionPane.showMessageDialog(this, 
                    "Image type is not 16 bit grayscale!", "Error LookUpSTORM", 
                    JOptionPane.ERROR_MESSAGE);
            return;
        }
        ImageStack stack = imp.getStack();
        _lookUpSTORM.setImagePara(stack.getWidth(), stack.getHeight());
        
        final double dLat = (double)_spinnerLatDelta.getValue();
        final int threshold = (int)_spinnerThreshold.getValue();
        final int maxIter = (int)_spinnerMaxIter.getValue();
        final int scale = (int)_spinnerScale.getValue();
        final double eps = (double)_spinnerEps.getValue();
        _lookUpSTORM.setThreshold(threshold);
        _lookUpSTORM.setEpsilon(eps);
        _lookUpSTORM.setMaxIter(maxIter);
        _renderThread.setSize(stack.getWidth() * scale, stack.getHeight() * scale);
        _renderThread.setImageName(imp.getTitle());
        
        if (dLat > (1.0/scale)) {
            JOptionPane.showMessageDialog(this, 
                    "Render image resolution is bigger than the lateral steps. "
                        + "This can result in artefacts!", "Error LookUpSTORM", 
                    JOptionPane.WARNING_MESSAGE);
        }
        
        _renderThread.startRendering();
        Runnable fitter = () -> {
            startAnalysisGUI();
            final int frames = stack.getSize();
            final long t0 = System.nanoTime();
            _lookUpSTORM.reset();
            for (int frame = 0; frame < frames; frame++) {
                ImageProcessor ip = stack.getProcessor(frame + 1);
                if (!_lookUpSTORM.feedImage(ip, frame)) 
                    continue;
                _lookUpSTORM.waitForLocFinished(1, 100);
                IJ.showProgress(frame, frames);
            }
            _renderThread.stopRendering();
            final long t1 = System.nanoTime();
            stopAnalysisGUI();
            final String out = "Found " + _lookUpSTORM.numberOfAllLocs() + " emitters in " + (t1 - t0) / 1E9 + " s!";
            IJ.showStatus(out);
            System.out.println(out);
        };
        (new Thread(fitter)).start();
    }
    
    private void doRelease() {
        _renderThread.release();
        _lookUpSTORM.release();
    }
    
    private void startAnalysisGUI() {
        _pane.setEnabledAt(0, false);
        _pane.setSelectedIndex(1);
        _spinnerScale.setEnabled(false);
        _fitButton.setEnabled(false);
    }
    
    private void stopAnalysisGUI() {
        _pane.setEnabledAt(0, true);
        _spinnerScale.setEnabled(true);
        _fitButton.setEnabled(true);
    }
    
    private boolean checkLUTData(String fileName, String selectedSource) {
        if (selectedSource.equals("From Binary File")) {
            BinaryLUT lut = new BinaryLUT();
            if (lut.loadHeader(fileName)) {
                _comboWinSize.setSelectedIndex(lut.getWindowSize() == 11 ? 1 : 0);
                _spinnerLatDelta.setValue(lut.getDeltaLat());
                _spinnerLatRange.setValue(lut.getLateralRange());
                _spinnerAxDelta.setValue(lut.getDeltaAx());
                _spinnerAxRange.setValue(lut.getAxialRange());
                _buttonGenerate.setEnabled(true);
                updateRAMUsage();
                return true;
            }
        } else if (selectedSource.equals("Astigmatism")) {
            _cali = new Calibration();
            if (_cali.load(fileName)) {
                _cali.plot();
                _buttonGenerate.setEnabled(true);
                updateRAMUsage();
                return true;
            }
        }
        return false;
    }
    
    private void updateRAMUsage() {
        final double dLat = (double)_spinnerLatDelta.getValue();
        final double rangeLat = (double)_spinnerLatRange.getValue();
        final double dAx = (double)_spinnerAxDelta.getValue();
        final double rangeAx = (double)_spinnerAxRange.getValue();
        final int winSize = _comboWinSize.getSelectedIndex() == 1 ? 11 : 9;
        final long bytes = LookUpSTORM.calculateBytesForLUT(winSize, dLat, dAx, rangeLat, rangeAx);
        _ramField.setText(getRAMUsage(bytes));
    }
    
    private static String getRAMUsage(long bytes) {
        if (bytes > 1073741824l) {
            return (bytes / 1073741824l) + "," + (bytes % 1073741824l) + " GB";
        } else if (bytes > 1048576l) {
            return (bytes / 1048576l) + " MB";
        } else if (bytes > 1024) {
            return (bytes / 1024) + " kB";
        }
        return bytes + " Bytes";
    }
}
