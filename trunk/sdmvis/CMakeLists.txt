# sdmvis
# Max Hermann, November 14, 2010
# [August 19, 2011] adapted Vitalis VARVIS integration to depend on VTK
# [March 4, 2011] added VTK dependency (optional)

cmake_minimum_required(VERSION 2.4)
project(sdmvis)

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)


#---- User config  ------------------------------------------------------------
set( SDMVIS_USE_VARVIS             "TRUE"  CACHE BOOL 
		"Add VarVis functionality (requires VTK)" )
set( SDMVIS_USE_TENSORVIS          "FALSE" CACHE BOOL 
		"Add TensorVis functionality (requires VTK)" )
set( SDMVIS_USE_MATLAB             "FALSE" CACHE BOOL 
		"Add Matlab based components to sdmvis (if Matlab found)" )
set( SDMVIS_USE_BOOST_FILESYSTEM_2 "FALSE"  CACHE BOOL 
		"Set to true when using the deprecated Boost::FileSystem 2 API" )

# Boost filesystem workaround
if( SDMVIS_USE_BOOST_FILESYSTEM_2 )
	ADD_DEFINITIONS(-DSDMVIS_USE_BOOST_FILESYSTEM_2)
endif( SDMVIS_USE_BOOST_FILESYSTEM_2 )

# Enable VTK if required
if( SDMVIS_USE_VARVIS OR SDMVIS_USE_TENSORVIS )
	message( STATUS "VTK enabled since one or more components require VTK." )
	set( SDMVIS_USE_VTK "TRUE" )
endif()


#---- Compiler config ---------------------------------------------------------
if( WIN32 )
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-D_SCL_SECURE_NO_WARNINGS)
endif()


#---- Dependencies ------------------------------------------------------------
#

#---------------------------------------------------------
# Boost (e.g. filesystem dependency in included libraries)
# // unix needs filesystem AND system-package by boost 
#----------------------------------------------------------
set(Boost_USE_STATIC_LIBS      ON)
set(Boost_USE_MULTITHREADED    ON)
set(Boost_USE_STATIC_RUNTIME  OFF)
set(Boost_NO_SYSTEM_PATHS      ON)
set(Boost_DEBUG OFF)
find_package(Boost COMPONENTS system filesystem program_options REQUIRED)
# prevent boost automatic linkage on Windows
add_definitions(-DBOOST_ALL_NO_LIB)
message(STATUS "Boost_INCLUDE_DIR=${Boost_INCLUDE_DIR}")
message(STATUS "Boost_LIBRARIES=${Boost_LIBRARIES}")
message(STATUS "Boost_LIBRARY_DIRS=${Boost_LIBRARY_DIRS}")
message(STATUS "BOOST_LIBRARYDIR=${BOOST_LIBRARYDIR}")
#include_directories(${Boost_INCLUDE_DIR})

#-------------------
# OpenGL, GLEW
#-------------------
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
find_package(GLEW   REQUIRED)
find_package(OpenGL REQUIRED)

#-------------------
# Qt4
#-------------------
find_package(Qt4 COMPONENTS QtCore QtGui QtOpenGL QtHelp QtWebKit QtNetwork REQUIRED)
include(${QT_USE_FILE})

# Qt4 resource files
set( sdmvis_RCCS sdmvis.qrc )

#-------------------
# VTK (optional)
#-------------------
if( SDMVIS_USE_VTK )
	find_package( VTK )
	if( VTK_FOUND )
		message( STATUS "VTK enabled." )
		include(${VTK_USE_FILE})	
		set(VTK_LIBRARIES 
				QVTK
				vtkRendering 
				vtkVolumeRendering 
				vtkGraphics
				vtkFiltering
				vtkIO
				vtkCommon
				vtkWidgets
			)
		ADD_DEFINITIONS(-DSDMVIS_VTK_ENABLED)
	else( VTK_FOUND )
		message( STATUS "VTK not found, corresponding functionality will be disabled." )
	endif( VTK_FOUND )
endif( SDMVIS_USE_VTK )

#-------------------
# Matlab (optional, see also cmake --help-module FindMatlab)
#-------------------
if( SDMVIS_USE_MATLAB )
	find_package(Matlab)
	if( MATLAB_FOUND )
		message( STATUS "Matlab enabled." )
		message( STATUS "Matlab include dir=${MATLAB_INCLUDE_DIR}" )
		message( STATUS "Matlab libratries =${MATLAB_LIBRARIES}" )
		ADD_DEFINITIONS(-DSDMVIS_MATLAB_ENABLED)
	else( MATLAB_FOUND )
		message( STATUS "Matlab not found, corresponding functionality will be disabled.")
	endif( MATLAB_FOUND )
else( SDMVIS_USE_MATLAB EQUAL TRUE )
	# although Matlab is not found, include and lib variables may not be empty
	set( MATLAB_INCLUDE_DIR "" )
	set( MATLAB_LIBRARIES   "" )
endif( SDMVIS_USE_MATLAB )

#-------------------
# teem (for nrrd support, optional)
#-------------------
find_package(TEEM)
if( TEEM_FOUND )
	include_directories( ${TEEM_INCLUDES} )
	add_definitions(-DTENSORVIS_TEEM_SUPPORT)
	message( STATUS "Teem enabled." )
else( TEEM_FOUND )
	set( TEEM_LIBRARIES "" )
	message( STATUS "Teem not found, disabling corresponding features." )
endif( TEEM_FOUND )

#-------------------
# VARVIS (depends on VTK)
#-------------------
if( SDMVIS_USE_VARVIS)
	if( VTK_FOUND )
		message( STATUS "VarVis enabled." )
	    set(VARVIS_BASE_PATH "..")
		set(VARVIS_VTK_SOURCES
			${VARVIS_BASE_PATH}/varvis/GlyphControls.h
			${VARVIS_BASE_PATH}/varvis/GlyphControls.cpp
			${VARVIS_BASE_PATH}/varvis/RoiControls.h
			${VARVIS_BASE_PATH}/varvis/RoiControls.cpp
			${VARVIS_BASE_PATH}/varvis/VolumeControls.h
			${VARVIS_BASE_PATH}/varvis/VolumeControls.cpp
			${VARVIS_BASE_PATH}/varvis/VarVisControls.h
			${VARVIS_BASE_PATH}/varvis/VarVisControls.cpp
			${VARVIS_BASE_PATH}/varvis/VarVisRender.h
			${VARVIS_BASE_PATH}/varvis/VarVisRender.cpp		
			${VARVIS_BASE_PATH}/varvis/VarVisWidget.h
			${VARVIS_BASE_PATH}/varvis/VarVisWidget.cpp
			${VARVIS_BASE_PATH}/varvis/VectorToVertexNormalFilter.h
			${VARVIS_BASE_PATH}/varvis/VectorToVertexNormalFilter.cpp
			${VARVIS_BASE_PATH}/varvis/VectorToMeshColorFilter.h
			${VARVIS_BASE_PATH}/varvis/VectorToMeshColorFilter.cpp
			${VARVIS_BASE_PATH}/varvis/PointSamplerFilter.cpp
			${VARVIS_BASE_PATH}/varvis/PointSamplerFilter.h
			${VARVIS_BASE_PATH}/varvis/GlyphOffsetFilter.cpp
			${VARVIS_BASE_PATH}/varvis/GlyphOffsetFilter.h		
			${VARVIS_BASE_PATH}/varvis/GlyphInvertFilter.cpp
			${VARVIS_BASE_PATH}/varvis/GlyphInvertFilter.h
			${VARVIS_BASE_PATH}/varvis/VectorfieldClustering.h
			${VARVIS_BASE_PATH}/varvis/VectorfieldClustering.cpp
			${VARVIS_BASE_PATH}/varvis/GlyphVisualization.h
			${VARVIS_BASE_PATH}/varvis/GlyphVisualization.cpp
		)
		set(VARVIS_VTK_MOC_HEADERS
			${VARVIS_BASE_PATH}/varvis/VarVisWidget.h
			${VARVIS_BASE_PATH}/varvis/VarVisControls.h
			${VARVIS_BASE_PATH}/varvis/VolumeControls.h
			${VARVIS_BASE_PATH}/varvis/RoiControls.h
			${VARVIS_BASE_PATH}/varvis/GlyphControls.h
			${VARVIS_BASE_PATH}/varvis/VarVisRender.h
		)
			set(VARVIS_INCLUDE_DIR
		
		)
		source_group("varvis" FILES ${VARVIS_VTK_SOURCES})
		ADD_DEFINITIONS(-DSDMVIS_VARVIS_ENABLED)
	else( VTK_FOUND )
		message( STATUS "VarVis disabled because VTK is not available!" )
	endif( VTK_FOUND )
endif( SDMVIS_USE_VARVIS)

#-------------------
# TENSORVIS (depends on VTK)
#-------------------
if( SDMVIS_USE_TENSORVIS )
	if( VTK_FOUND )
		message( STATUS "TensorVis enabled." )
		set(TENSORVIS_BASE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../tensorvis")
		set(TENSORVIS_SOURCES
			${TENSORVIS_BASE_PATH}/TensorVisBase.h
			${TENSORVIS_BASE_PATH}/TensorVisBase.cpp
			${TENSORVIS_BASE_PATH}/TensorVisRenderBase.h
			${TENSORVIS_BASE_PATH}/TensorVisRenderBase.cpp			
			${TENSORVIS_BASE_PATH}/TensorVis2.h
			${TENSORVIS_BASE_PATH}/TensorVis2.cpp
			${TENSORVIS_BASE_PATH}/TensorSpectrum.h
			${TENSORVIS_BASE_PATH}/TensorSpectrum.cpp
			${TENSORVIS_BASE_PATH}/TensorData.h
			${TENSORVIS_BASE_PATH}/TensorData.cpp
			${TENSORVIS_BASE_PATH}/TensorDataAdaptor.h
			${TENSORVIS_BASE_PATH}/TensorDataProvider.h
			${TENSORVIS_BASE_PATH}/TensorDataProvider.cpp
			${TENSORVIS_BASE_PATH}/TensorDataFileProvider.h
			${TENSORVIS_BASE_PATH}/TensorDataFileProvider.cpp
			${TENSORVIS_BASE_PATH}/WeightedTensorDataProvider.h
			${TENSORVIS_BASE_PATH}/WeightedTensorDataProvider.cpp			
			${TENSORVIS_BASE_PATH}/TensorDataStatistics.h
			${TENSORVIS_BASE_PATH}/TensorDataStatistics.cpp
			${TENSORVIS_BASE_PATH}/ImageDataSpace.h
			${TENSORVIS_BASE_PATH}/ImageDataSpace.cpp
			${TENSORVIS_BASE_PATH}/VolVis.h
			${TENSORVIS_BASE_PATH}/VolVis.cpp
			${TENSORVIS_BASE_PATH}/Warpfields.h
			${TENSORVIS_BASE_PATH}/Warpfields.cpp
			${TENSORVIS_BASE_PATH}/vtkTensorGlyph3.cxx
			${TENSORVIS_BASE_PATH}/vtkTensorGlyph3.h
			${TENSORVIS_BASE_PATH}/vtkGlyph3D_3.cxx
			${TENSORVIS_BASE_PATH}/vtkGlyph3D_3.h
			${TENSORVIS_BASE_PATH}/vtkConeSource2.cxx
			${TENSORVIS_BASE_PATH}/vtkConeSource2.h	
			${TENSORVIS_BASE_PATH}/QTensorVisWidget.h
			${TENSORVIS_BASE_PATH}/QTensorVisOptionsWidget.h
			${TENSORVIS_BASE_PATH}/QTensorVisWidget.cpp
			${TENSORVIS_BASE_PATH}/QTensorVisOptionsWidget.cpp
			${TENSORVIS_BASE_PATH}/ColorMapRGB.h
			${TENSORVIS_BASE_PATH}/ColorMapRGB.cpp
			${TENSORVIS_BASE_PATH}/TensorNormalDistribution.h
			${TENSORVIS_BASE_PATH}/TensorNormalDistribution.cpp
			${TENSORVIS_BASE_PATH}/TensorNormalDistributionProvider.h
			${TENSORVIS_BASE_PATH}/TensorNormalDistributionProvider.cpp
			${TENSORVIS_BASE_PATH}/ModeAnimationWidget.h
			${TENSORVIS_BASE_PATH}/ModeAnimationWidget.cpp
			${TENSORVIS_BASE_PATH}/ModeAnimationParameters.h			
			SDMTensorVisWidget.cpp
			SDMTensorVisWidget.h
			SDMTensorDataProvider.h
			SDMTensorDataProvider.cpp
			EditLocalCovariance.h
			EditLocalCovariance.cpp
			LinearLocalCovariance.h
			LinearLocalCovariance.cpp
			LocalCovarianceStatistics.h
			LocalCovarianceStatistics.cpp
			SDMTensorOverviewWidget.h
			SDMTensorOverviewWidget.cpp
			SDMTensorProbe.h
			SDMTensorProbeWidget.h
			SDMTensorProbeWidget.cpp
		)
		set(TENSORVIS_MOC_HEADERS
			${TENSORVIS_BASE_PATH}/QTensorVisWidget.h
			${TENSORVIS_BASE_PATH}/QTensorVisOptionsWidget.h		
			${TENSORVIS_BASE_PATH}/ModeAnimationWidget.h
			SDMTensorOverviewWidget.h
			SDMTensorProbeWidget.h
		)
		set(TENSORVIS_INCLUDE_DIR		
		)
		source_group("tensorvis" FILES ${TENSORVIS_SOURCES})
		include_directories(${TENSORVIS_BASE_PATH})
		ADD_DEFINITIONS(-DSDMVIS_TENSORVIS_ENABLED)
	else( VTK_FOUND )
		message( STATUS "TensorVis disabled because VTK is not available!" )
	endif( VTK_FOUND )
endif( SDMVIS_USE_TENSORVIS)

#-------------------
# Additional VTK components
#-------------------
if( VTK_FOUND )
	set(SDMVIS_VTK_SOURCES
		VTKVisWidget.h
		VTKVisWidget.cpp
		VTKVisPrimitives.h
		VTKVisPrimitives.cpp
		VTKCameraSerializer.h
		VTKCameraSerializer.cpp
		VTKRayPickerHelper.h
		VTKRayPickerHelper.cpp
	)
	set(SDMVIS_VTK_MOC_HEADERS
		VTKVisWidget.h
	)
endif( VTK_FOUND )

#-------------------
# Matlab functions
#-------------------
if( MATLAB_FOUND )
	set(SDMVIS_MATLAB_SOURCES
		Matlab.h
		Matlab.cpp
	)
	set(SDMVIS_MATLAB_MOC_HEADERS
	)	
endif( MATLAB_FOUND )

#-------------------
# Qt4 moc headers
#-------------------
set( sdmvis_MOC_HDRS
	SDMVisMainWindow.h
	SDMVISConfig.h
	SDMVisVolumeRenderer.h
	SDMVisInteractiveEditingOptionsWidget.h	
	SDMTensorVisWidget.h
	BarPlotWidget.h
	BatchProcessingDialog.h
	TraitDialog.h
	DatasetWidget.h
	plotWidget.h
	plotView.h
	CSVExporter.h
	TraitSelectionWidget.h
	ConfigGenerator.h
	TraitComb.h
	ScatterPlotWidget.h
	${SDMVIS_VTK_MOC_HEADERS}
	${SDMVIS_MATLAB_MOC_HEADERS}
	${VARVIS_VTK_MOC_HEADERS}
	${TENSORVIS_MOC_HEADERS}
)

#-------------------
# SDMPROC
#-------------------
set( sdmproc_BASEPATH "../sdmproc" )
set( sdmproc_SRC
	${sdmproc_BASEPATH}/StatisticalDeformationModel.h
	${sdmproc_BASEPATH}/StatisticalDeformationModel.cpp
#	${sdmproc_BASEPATH}/Reconstruction.h
#	${sdmproc_BASEPATH}/Reconstruction.cpp
#	${sdmproc_BASEPATH}/MetaImageHeader.h
)
include_directories( ${sdmproc_BASEPATH} )

#-------------------
# All sources
#-------------------
set( sdmvis_SRCS
	main.cpp
	SDMVisConfig.cpp
	SDMVisConfig.h
	SDMVisMainWindow.cpp
	SDMVisMainWindow.h
	SDMVisVolumeRenderer.cpp
	SDMVisVolumeRenderer.h
	SDMVisInteractiveEditing.cpp
	SDMVisInteractiveEditing.h
	SDMVisInteractiveEditingOptionsWidget.cpp
	SDMVisInteractiveEditingOptionsWidget.h
	Geometry2.h
	Geometry2.cpp
	Trackball.h
	Trackball.cpp
	VolumeManager.h
	VolumeManager.cpp
	Warpfield.h
	Trait.h
	BarPlotWidget.h
	BarPlotWidget.cpp
	SphereSelection.h
	SphereSelection.cpp
	BatchProcessingDialog.h
	BatchProcessingDialog.cpp
	PleaseWaitDialog.h
	PleaseWaitDialog.cpp
	TraitDialog.h
	TraitDialog.cpp
	TraitSelectionWidget.h
	TraitSelectionWidget.cpp
	DatasetWidget.h
	DatasetWidget.cpp
	plotWidget.cpp
	plotItem.cpp
	plotView.cpp
	plotWidget.h
	plotItem.h
	plotView.h
	TraitComb.h
	TraitComb.cpp
	CSVExporter.h
	CSVExporter.cpp
	ConfigGenerator.h	
	ConfigGenerator.cpp
	ScatterPlotWidget.h
	ScatterPlotWidget.cpp
	OverdrawInterface.h
	OverdrawQGLWidget.h
	OverdrawQGLWidget.cpp
	#OverdrawQVTKWidget2.h
	#OverdrawQVTKWidget2.cpp	
	${SDMVIS_VTK_SOURCES}
	${SDMVIS_MATLAB_SOURCES}
	${VARVIS_VTK_SOURCES}
	${TENSORVIS_SOURCES}
	${CMAKE_CURRENT_SOURCE_DIR}/../tensorvis/ColorMapRGB.h
	${CMAKE_CURRENT_SOURCE_DIR}/../tensorvis/ColorMapRGB.cpp	
	${sdmproc_SRC}
)

#---- Qt4 resources & moc -----------------------------------------------------

# generate rules for building source files from the Qt resources
qt4_add_resources( sdmvis_RCC_SRCS ${sdmvis_RCCS} )

# generate rules for building source files that moc generates
qt4_wrap_cpp( sdmvis_MOC_SRCS ${sdmvis_MOC_HDRS} )

# not sure what the advantage of "automoc" is
qt4_automoc( ${sdmvis_SRCS} )

source_group("Autogenerated Moc files" FILES ${sdmvis_MOC_SRCS})

#---- Directories -------------------------------------------------------------

# e7 paths
set( E7_LIB_PATH ${PROJECT_SOURCE_DIR}/e7 )
set( E7_GLM_PATH ${E7_LIB_PATH}/3rdParty/glm    )

# math base path (parent directory of /mat)
set( MAT_BASE_PATH ${PROJECT_SOURCE_DIR}/.. )

# include directories
include_directories(
	${Boost_INCLUDE_DIR}
	${GLEW_INCLUDE_DIR}
	${VTK_INCLUDE_DIR}
	${E7_LIB_PATH}
	${E7_LIB_PATH}/3rdParty
	${E7_GLM_PATH}
	${MAT_BASE_PATH}
	${MAT_BASE_PATH}/mat
	${MATLAB_INCLUDE_DIR}  # optional, maybe empty string
	${VARVIS_INCLUDE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../tensorvis/ # make sure we find ColorMapRGB	
)

#---- Sub projects & libraries ------------------------------------------------

# add projects to generate
add_subdirectory(e7)
add_subdirectory(${MAT_BASE_PATH}/mat ${CMAKE_CURRENT_BINARY_DIR}/mat)

#---- Executable --------------------------------------------------------------

# build sources, moc'd sources and rcc'd sources
add_executable(	sdmvis 
	${sdmvis_SRCS}
	${VARVIS_MOC_SRCS}
	${sdmvis_MOC_SRCS}   # generated Qt moc sources
	${sdmvis_RCC_SRCS}   # generated Qt resources
	sdmvis.rc            # Visual Studio resource(s), e.g. windows application icon
)

message(STATUS "OPENGL_LIBRARIES=${OPENGL_LIBRARIES}")

target_link_libraries( sdmvis 
	e7
	mat
	${QT_LIBRARIES}
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARY}
	${VTK_LIBRARIES}       # optional, maybe empty string
	${TEEM_LIBRARIES}	   # optional, maybe empty string
	${MATLAB_LIBRARIES}    # optional, maybe empty string
	${Boost_LIBRARIES}	
    #${Boost_FILESYSTEM_LIBRARY}
    #${Boost_SYSTEM_LIBRARY}
	#${Boost_PROGRAM_OPTIONS_LIBRARY}
)
