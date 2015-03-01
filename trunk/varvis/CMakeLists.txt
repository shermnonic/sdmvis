# varvis (standalone)
# Max Hermann, November 14, 2010
# [March 4, 2011] added VTK dependency (optional)

cmake_minimum_required(VERSION 2.4)
project(varVis)

#---- Dependencies ------------------------------------------------------------

#---------------------------------------------------------
# Boost
#----------------------------------------------------------
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED)
	# currently we only use boost header libraries
	# COMPONENTS system filesystem program_options )
add_definitions(-DBOOST_ALL_NO_LIB) # prevent boost automatic linkage on Windows

#-------------------
# Qt4
#-------------------
find_package(Qt4 COMPONENTS QtCore QtGui QtOpenGL REQUIRED)
include(${QT_USE_FILE})

#-------------------
# VTK
#-------------------
find_package(VTK REQUIRED)
include(${VTK_USE_FILE})
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
endif( VTK_FOUND )

#-------------------
# VTK functions
#-------------------
if( VTK_FOUND )
	set(VARVIS_VTK_SOURCES
	)
	set(VARVIS_VTK_MOC_HEADERS
		VarVisWidget.h
		VarVisControls.h
	)
endif( VTK_FOUND )

#-------------------
# Qt4 moc headers
#-------------------
set( VARVIS_MOC_HDRS
	${VARVIS_VTK_MOC_HEADERS}
	MainWindow.h
	VarVisControls.h
	VolumeControls.h
	GlyphControls.h
	RoiControls.h
	VarVisRender.h
)

#-------------------
# All sources
#-------------------
set( VARVIS_SRCS
	main.cpp
	MainWindow.cpp
	MainWindow.h
	VolumeControls.h
	VolumeControls.cpp
	GlyphControls.h
	GlyphControls.cpp
	RoiControls.cpp
	RoiControls.h	
	VarVisControls.h
	VarVisControls.cpp
	VarVisRender.h
	VarVisRender.cpp	
	VarVisWidget.h
	VarVisWidget.cpp
	VectorToVertexNormalFilter.h
	VectorToVertexNormalFilter.cpp
	VectorToMeshColorFilter.h
	VectorToMeshColorFilter.cpp
	PointSamplerFilter.cpp
	PointSamplerFilter.h
	GlyphOffsetFilter.cpp
	GlyphOffsetFilter.h
	GlyphInvertFilter.cpp
	GlyphInvertFilter.h
	VectorfieldClustering.h
	VectorfieldClustering.cpp
	GlyphVisualization.h
	GlyphVisualization.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../tensorvis/ColorMapRGB.h
	${CMAKE_CURRENT_SOURCE_DIR}/../tensorvis/ColorMapRGB.cpp
)

include_directories()

#---- Qt4 resources & moc -----------------------------------------------------

# generate rules for building source files that moc generates
qt4_wrap_cpp( VARVIS_MOC_SRCS ${VARVIS_MOC_HDRS} )

# not sure what the advantage of "automoc" is
qt4_automoc( ${VARVIS_SRCS} )

#---- Sub projects & libraries ------------------------------------------------

#---- Executable --------------------------------------------------------------

# include directories
include_directories(
	${Boost_INCLUDE_DIR}
	${VTK_INCLUDE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../tensorvis/ # make sure we find ColorMapRGB
)

# build sources, moc'd sources and rcc'd sources
add_executable(	varVis 
	${VARVIS_SRCS}
	${VARVIS_MOC_SRCS}   # generated Qt moc sources
	${VARVIS_RCC_SRCS}   # generated Qt resources
)

target_link_libraries( varVis 
	${QT_LIBRARIES}
	${VTK_LIBRARIES}
)
