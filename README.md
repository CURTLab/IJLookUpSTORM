# LookUpSTORM

LookUpSTORM is an 3D real-time single molecule localization microscopy (SMLM) fitting algorithm that uses lookup tables for subpixel emitter fitting.

This is an example implementation for ImageJ 1.x.

# Pre-Release
Pre-Pelease Image 1.x Plugin for Windows [link to download](https://filebox.fhooecloud.at/index.php/s/c9Xe85ybzMrLBdM)

# Installation
Unpack the archive of `LookUpSTORM_-0.1.0-SNAPSHOT.zip` and copy `LookUpSTORM_CPPDLL.dll` into your ImageJ installation directory (e.g. `C:/ImageJ`) and `commons-math3-3.6.1.jar` & `LookUpSTORM_-0.1.0-SNAPSHOT.jar` into the subdirectory `plugins` (e.g. `C:/ImageJ/plugins`).

# Run Plugin
The plugin can be access from ImageJ within the menu `Plugins -> LookUpSTORM -> Analyse Experiment`. Load a 16-bit grayscale experiment and generate a lookup table for analysis. Don't forget to set the correct values in the `Image Setup` tab for your loaded image and a threshold (which can be adapted during analysis) in the `General Tab`. To start the analysis select the loaded image and press the `Fit current` button in the plugin window.

# Data
Experimental data for the unpublished paper `Real-Time 3D Single-Molecule Localization Microscopy Analysis Using Lookup Tables`:
* Fig 1: [link to download](https://filebox.fhooecloud.at/index.php/s/fKwtgFXaxcf8jpp) (see [details](https://github.com/CURTLab/IJLookUpSTORM/wiki/DetailsFig1))
* Fig 2: [link to download](https://filebox.fhooecloud.at/index.php/s/6EyQ4tnWM2qZdHe) (see [details](https://github.com/CURTLab/IJLookUpSTORM/wiki/DetailsFig2))
* Fig 3: [link to download](https://filebox.fhooecloud.at/index.php/s/tKZ8n9GaKtSGW3N) (see [details](https://github.com/CURTLab/IJLookUpSTORM/wiki/DetailsFig3))

# Tested prerequisites for compilation
* Windows 10
* Visual Studio 2019
* Java 1.8 + Maven 14.0.0 in NetBeans IDE 8.2
* ImageJ 1.53h
