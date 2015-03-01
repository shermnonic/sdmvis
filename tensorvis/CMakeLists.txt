# TensorVis (main CMakeLists)

cmake_minimum_required(VERSION 2.6)
project(TensorVis)

#---- Dependencies ------------------------------------------------------------

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../sdmvis/cmake)

#-------------------
# Boost (headers)
#-------------------
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
message(STATUS "Boost_INCLUDE_DIR=${Boost_INCLUDE_DIR}")

#-------------------
# VTK
#-------------------
find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

#-------------------
# QT4
#-------------------
find_package(Qt4 REQUIRED)
include(${QT_USE_FILE})

# Qt resource files
set( qtensorvis_RCCS qtensorvis.qrc )

# moc headers
set( qtensorvis_MOC_HDRS
	QTensorVisApp.h
	QTensorVisWidget.h
	QTensorVisOptionsWidget.h
	ModeAnimationWidget.h
)

#-------------------
# teem (for nrrd support, optional)
#-------------------
find_package(TEEM)
if( TEEM_FOUND )
	include_directories( ${TEEM_INCLUDES} )
	add_definitions(-DTENSORVIS_TEEM_SUPPORT)
else( TEEM_FOUND )
	set( TEEM_LIBRARIES "" )
endif( TEEM_FOUND )

#---- Sources -----------------------------------------------------------------

# numerical and matrix functions
set( mat_SRCS
	../mat/mattools.cpp
	../mat/mattools.h
	../mat/numerics.h
	../mat/rednum.h
)
include_directories( ../mat )

# application sources (qtensorvis)
set( qtensorvis_SRCS 
	qtensorvis.cpp
	QTensorVisApp.cpp
	QTensorVisApp.h
	QTensorVisWidget.h
	QTensorVisWidget.cpp
	QTensorVisOptionsWidget.h
	QTensorVisOptionsWidget.cpp
	TensorVisBase.h
	TensorVisBase.cpp
	TensorVisRenderBase.h
	TensorVisRenderBase.cpp
	TensorVis.h
	TensorVis.cpp
	TensorVis2.h
	TensorVis2.cpp
	TensorSpectrum.h
	TensorSpectrum.cpp
	TensorData.h
	TensorData.cpp
	TensorDataAdaptor.h
	TensorDataProvider.h
	TensorDataProvider.cpp
	TensorDataFileProvider.h
	TensorDataFileProvider.cpp
	TensorNormalDistribution.h
	TensorNormalDistribution.cpp
	TensorNormalDistributionProvider.h
	TensorNormalDistributionProvider.cpp
	WeightedTensorDataProvider.h
	WeightedTensorDataProvider.cpp
	ImageDataSpace.h
	ImageDataSpace.cpp
	VolVis.h
	VolVis.cpp
	Warpfields.h
	Warpfields.cpp
	vtkTensorGlyph3.cxx
	vtkTensorGlyph3.h
	vtkGlyph3D_3.cxx
	vtkGlyph3D_3.h
	vtkConeSource2.cxx
	vtkConeSource2.h
	ColorMapRGB.h
	ColorMapRGB.cpp	
	ModeAnimationWidget.h
	ModeAnimationWidget.cpp
	ModeAnimationParameters.h
)

#---- Qt4 resources & moc -----------------------------------------------------

# generate rules for building source files from the Qt resources
qt4_add_resources( qtensorvis_RCC_SRCS ${qtensorvis_RCCS} )

# generate rules for building source files that moc generates
qt4_wrap_cpp( qtensorvis_MOC_SRCS ${qtensorvis_MOC_HDRS} )

# not sure what the advantage of "automoc" is
qt4_automoc( ${qtensorvis_SRCS} )

#---- Executable --------------------------------------------------------------

# build sources, moc'd sources and rcc'd sources
add_executable(	qtensorvis 
	${qtensorvis_SRCS}
	${qtensorvis_MOC_SRCS}   # generated Qt moc sources
	${qtensorvis_RCC_SRCS}   # generated Qt resources
	qtensorvis.rc            # Visual Studio resource(s), e.g. application icon
)

target_link_libraries( qtensorvis 
	QVTK
	${QT_LIBRARIES}
	vtkRendering
	vtkVolumeRendering
	vtkGraphics
	vtkIO
	vtkCommon
	${VOLTOOLS_COMMON_LINK_LIBRARIES}  # required?
	${TEEM_LIBRARIES}
	#${TEEM_LIBRARY} # optional, may be empty
)

#---- Teem executable(s) ------------------------------------------------------

if( TEEM_FOUND )
	add_executable(	tensordist
		tensordist.cpp
		ImageDataSpace.h
		ImageDataSpace.cpp
		TensorNormalDistribution.h
		TensorNormalDistribution.cpp
	)

	target_link_libraries( tensordist
		${TEEM_LIBRARIES}
	)
endif( TEEM_FOUND )
