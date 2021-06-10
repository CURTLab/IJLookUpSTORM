# LookUpSTORM

LookUpSTORM is a 3D real-time single molecule localization microscopy (SMLM) fitting algorithm that uses lookup tables for subpixel single emitter signal fitting.

This is an example implementation for ImageJ 1.x.

![Thumbnail](https://github.com/CURTLab/IJLookUpSTORM/blob/master/thumbnail_lookupstorm.png)

# Pre-Release
Pre-Release ImageJ Plugin Version 0.1.2 for ImageJ 1.x (including dependencies) 
* Window x64 [link to download](https://filebox.fhooecloud.at/index.php/s/MtTnwezEkeFC4Y7)
* Linux x64 [link to download](https://filebox.fhooecloud.at/index.php/s/n85gL9RFtZRdtmz)

# Installation
Unpack the downloaded archive and copy the directories `lib` (containing `LookUpSTORM_CPPDLL.dll` or `libLookUpSTORM_CPPDLL.so` in their respective subdirectories `win64` and `linux64`) and `plugins` into your ImageJ installation directory (e.g. `C:/ImageJ`). Now, the plugin `LookUpSTORM_-0.1.2-jar-with-dependencies.jar` should be in the subdirectory `plugins` (e.g. `C:/ImageJ/plugins`).

# Run Plugin
The plugin can be access from ImageJ within the menu `Plugins -> LookUpSTORM -> Analyse Experiment`. Load a 16-bit grayscale experiment and generate a lookup table for analysis. Don't forget to set the correct values in the `Image Setup` tab for your loaded image and a threshold (which can be adapted during analysis) in the `General Tab`. To start the analysis select the loaded image and press the `Fit current` button in the plugin window. Further detail are described in the [wiki](https://github.com/CURTLab/IJLookUpSTORM/wiki/Usage).

# Using own experiments
LookUpSTORM support the usage of calibration files (YAML) created with [ThunderSTORM](https://github.com/zitmen/thunderstorm). See [wiki](https://github.com/CURTLab/IJLookUpSTORM/wiki/CreateCalibrationFile) for details.

# Data
Experimental data for the unpublished paper `Real-Time 3D Single-Molecule Localization Microscopy Analysis Using Lookup Tables`:
* Fig 1: [link to download](https://filebox.fhooecloud.at/index.php/s/fKwtgFXaxcf8jpp) (see [details](https://github.com/CURTLab/IJLookUpSTORM/wiki/DetailsFig1))
* Fig 2: [link to download](https://filebox.fhooecloud.at/index.php/s/6EyQ4tnWM2qZdHe) (see [details](https://github.com/CURTLab/IJLookUpSTORM/wiki/DetailsFig2))
* Fig 3: [link to download](https://filebox.fhooecloud.at/index.php/s/tKZ8n9GaKtSGW3N) (see [details](https://github.com/CURTLab/IJLookUpSTORM/wiki/DetailsFig3))

# Building Library
Library for c++/java can be build using the CMake build system. It is recommended to use the CMake GUI. 

In order to build the library for java set the CATCH variable `JNI_EXPORT` to true.

Other options for building are to use Intel MKL instead of the default minimal ATLAS that is included in the source. Furthermore, to speed up fitting, the CATCH variable `USE_AVX_INTRINSIC` can be selected (it is automatically checked if the system provides AVX2).

# Tested prerequisites for compilation
* Windows 10 and Ubuntu 20.04.1
* Visual Studio 2019
* Java 1.8 + Maven 14.0.0 in NetBeans IDE 8.2
* ImageJ 1.53h
* CMake 3.18.1
