# CMakeLists for LookUpSTORM JNI and CPP library
#
# MIT License
#
# Copyright (C) 2021 Fabian Hauser
#
# Author: Fabian Hauser <fabian.hauser@fh-linz.at>
# University of Applied Sciences Upper Austria - Linz - Austria

cmake_minimum_required (VERSION 3.9)

project (LookUpSTORM_CPPDLL VERSION 0.1 DESCRIPTION "Real-Time LookUp STORM Analysis")

set(SOURCE_FILES 
    LookUpSTORM_CPPDLL/src/Calibration.cpp
    LookUpSTORM_CPPDLL/src/ColorMap.cpp
	LookUpSTORM_CPPDLL/src/Controller.cpp
	LookUpSTORM_CPPDLL/src/Fitter.cpp
	LookUpSTORM_CPPDLL/src/Image.cpp
	LookUpSTORM_CPPDLL/src/LinearMath.cpp
	LookUpSTORM_CPPDLL/src/LocalMaximumSearch.cpp
	LookUpSTORM_CPPDLL/src/Matrix.cpp
	LookUpSTORM_CPPDLL/src/Rect.cpp
	LookUpSTORM_CPPDLL/src/Renderer.cpp
	LookUpSTORM_CPPDLL/src/Vector.cpp
)

set(PUBLIC_LIB_HEADERS 
	LookUpSTORM_CPPDLL/include/Common.h
	LookUpSTORM_CPPDLL/include/Calibration.h
	LookUpSTORM_CPPDLL/include/Controller.h
	LookUpSTORM_CPPDLL/include/Fitter.h
	LookUpSTORM_CPPDLL/include/Image.h
	LookUpSTORM_CPPDLL/include/LookUpSTORM.h
	LookUpSTORM_CPPDLL/include/Rect.h
	LookUpSTORM_CPPDLL/include/Renderer.h
)

add_definitions(-DNO_LAPACKE_LUT)

option(USE_MKL "Use MKL" False)
option(JNI_EXPORT "Export Symbols for JNI" False)

set(CMAKE_DEBUG_POSTFIX d)

if(JNI_EXPORT)
	list(APPEND SOURCE_FILES LookUpSTORM_CPPDLL/src/LookUpSTORM_CPPDLL.cpp)
endif(JNI_EXPORT)

add_library(LookUpSTORM_CPPDLL SHARED ${SOURCE_FILES})

set_target_properties(LookUpSTORM_CPPDLL PROPERTIES VERSION ${PROJECT_VERSION})	

target_include_directories(LookUpSTORM_CPPDLL PRIVATE LookUpSTORM_CPPDLL/src)
target_include_directories(LookUpSTORM_CPPDLL PUBLIC LookUpSTORM_CPPDLL/include)

add_library(LookUpSTORM_BRENT STATIC LookUpSTORM_CPPDLL/src/brent.cpp)
target_include_directories(LookUpSTORM_BRENT PRIVATE LookUpSTORM_CPPDLL/src)
target_link_libraries(LookUpSTORM_CPPDLL LookUpSTORM_BRENT)

if(JNI_EXPORT)
	find_package(JNI REQUIRED)
	include_directories(LookUpSTORM_CPPDLL ${JNI_INCLUDE_DIRS})
	target_link_libraries(LookUpSTORM_CPPDLL ${JNI_LIBRARIES})
	add_definitions(-DJNI_EXPORT_LUT)
else(JNI_EXPORT)
	add_definitions(-DDLL_EXPORT_LUT)
	set_target_properties(LookUpSTORM_CPPDLL PROPERTIES PUBLIC_HEADER "${PUBLIC_LIB_HEADERS}")
endif(JNI_EXPORT)

install(TARGETS LookUpSTORM_CPPDLL
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if(USE_MKL)
	find_package(MKL REQUIRED)
	include_directories(LookUpSTORM_CPPDLL ${MKL_INCLUDE_DIR})
	target_link_libraries(LookUpSTORM_CPPDLL ${MKL_LIBRARIES})
	add_definitions(-DUSE_MKL_LUT)
else(USE_MKL)
	message(STATUS "Use inbuild minimal ATLAS for BLAS")
	set(ATLAS_SOURCE_FILES
		LookUpSTORM_CPPDLL/src/atlas/ATL_drefsyrk.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_drefsyrkLN.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_drefsyrkLT.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_drefsyrkUN.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_drefsyrkUT.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_dreftrsv.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_dreftrsvLNN.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_dreftrsvLNU.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_dreftrsvLTN.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_dreftrsvLTU.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_dreftrsvUNN.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_dreftrsvUNU.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_dreftrsvUTN.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_dreftrsvUTU.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_drefgemm.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_drefgemmNN.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_drefgemmNT.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_drefgemmTN.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_drefgemmTT.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_drefgemv.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_drefgemvN.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_drefgemvT.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_drefger.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_drefscal.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_drefswap.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_dreftrsm.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_dreftrsmLLNN.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_dreftrsmLLNU.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_dreftrsmLLTN.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_dreftrsmLLTU.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_dreftrsmLUNN.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_dreftrsmLUNU.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_dreftrsmLUTN.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_dreftrsmLUTU.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_dreftrsmRLNN.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_dreftrsmRLNU.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_dreftrsmRLTN.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_dreftrsmRLTU.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_dreftrsmRUNN.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_dreftrsmRUNU.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_dreftrsmRUTN.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_dreftrsmRUTU.c
		LookUpSTORM_CPPDLL/src/atlas/ATL_idrefamax.c
		LookUpSTORM_CPPDLL/src/atlas/cblas.cpp
		LookUpSTORM_CPPDLL/src/atlas/lapack.c
	)
	
	add_library(LookUpSTORM_ATLAS STATIC ${ATLAS_SOURCE_FILES})
	target_include_directories(LookUpSTORM_ATLAS PRIVATE LookUpSTORM_CPPDLL/src/atlas)
	
	target_link_libraries(LookUpSTORM_CPPDLL LookUpSTORM_ATLAS)
endif(USE_MKL)
