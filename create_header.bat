cd src\main\java
javac at\fhlinz\imagej\LookUpSTORM.java
javah at.fhlinz.imagej.LookUpSTORM
copy at_fhlinz_imagej_LookUpSTORM.h "..\..\..\LookUpSTORM_CPPDLL\LookUpSTORM_CPPDLL.h"
del at\fhlinz\imagej\LookUpSTORM.class
del at_fhlinz_imagej_LookUpSTORM.h
cd ..\..