# sdmproc
# Max Hermann, March 7, 2013

cmake_minimum_required(VERSION 2.4)
project(sdmproc)

#---- Dependencies ------------------------------------------------------------

#-------------------
#  VTK
#-------------------
find_package(VTK REQUIRED)
# configure VTK
include(${VTK_USE_FILE})
set(VTK_LIBRARIES 
		vtkImaging
		vtkGraphics
		vtkFiltering
		vtkIO
		vtkHybrid
		vtkCommon		)

#-------------------
#  Boost
#-------------------
# configure boost
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS program_options REQUIRED)
# prevent boost automatic linkage on Windows
add_definitions(-DBOOST_ALL_NO_LIB)
message( STATUS "Boost_INCLUDE_DIRS=${Boost_INCLUDE_DIRS}" )
message( STATUS "Boost_LIBRARY_DIRS=${Boost_LIBRARY_DIRS}" )


#-------------------
#  SDM project internal dependencies
#-------------------
set(SDMVIS_BASE_PATH "../sdmvis" CACHE PATH "Path to sdmvis (project internal dependencies)")
set(VOLTOOLSAPPS_BASE_PATH "../../../voltools/apps" CACHE PATH "Path to voltools/apps/")
set(TENSORVIS_BASE_PATH "../tensorvis")
#set(TENSORVIS_BASE_PATH "${VOLTOOLSAPPS_BASE_PATH}/qtensorvis")
set(SDM_TensorData_SRC
	${TENSORVIS_BASE_PATH}/TensorData.h
	${TENSORVIS_BASE_PATH}/TensorData.cpp
	${TENSORVIS_BASE_PATH}/TensorDataProvider.h
	${TENSORVIS_BASE_PATH}/TensorDataProvider.cpp
	${TENSORVIS_BASE_PATH}/ImageDataSpace.cpp
	${TENSORVIS_BASE_PATH}/ImageDataSpace.h
	${SDMVIS_BASE_PATH}/SDMTensorDataProvider.h
	${SDMVIS_BASE_PATH}/SDMTensorDataProvider.cpp
	${SDMVIS_BASE_PATH}/LinearLocalCovariance.h
	${SDMVIS_BASE_PATH}/LinearLocalCovariance.cpp
)
include_directories(${TENSORVIS_BASE_PATH})
include_directories(${SDMVIS_BASE_PATH})
message( STATUS "TENSORVIS_BASE_PATH = ${TENSORVIS_BASE_PATH}" )
message( STATUS "SDMVIS_BASE_PATH    = ${SDMVIS_BASE_PATH}" )
message( STATUS "SDM_TensorData_SRC = ${SDM_TensorData_SRC}" )


#---- Directories -------------------------------------------------------------

# math base path (parent directory of /mat)
set( MAT_BASE_PATH ${PROJECT_SOURCE_DIR}/.. )

# include directories
include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${MAT_BASE_PATH}
	${MAT_BASE_PATH}/mat
	${Boost_INCLUDE_DIRS}
)

#---- Sub projects & libraries ------------------------------------------------

add_subdirectory(${MAT_BASE_PATH}/mat ${CMAKE_CURRENT_BINARY_DIR}/mat)

#---- Executables -------------------------------------------------------------

#-------------------
#  sdmproc
#-------------------

add_executable( sdmproc
	MetaImageHeader.h
	StatisticalDeformationModel.h
	StatisticalDeformationModel.cpp	
	Reconstruction.h
	Reconstruction.cpp
	ImageTools.h
	ImageTools.cpp	
	sdmproc.cpp
)

target_link_libraries( sdmproc
	mat
	${Boost_PROGRAM_OPTIONS_LIBRARY}
	${VTK_LIBRARIES}
)

#-------------------
#  imgproc
#-------------------

add_executable( imgproc
	ImageTools.h
	ImageTools.cpp
	imgproc.cpp
)

target_link_libraries( imgproc
	${VTK_LIBRARIES}
	${Boost_PROGRAM_OPTIONS_LIBRARY}
)

#-------------------
#  crossvalidate
#-------------------

add_executable( crossvalidate
	ImageTools.h
	ImageTools.cpp
	MetaImageHeader.h
	StatisticalDeformationModel.h
	StatisticalDeformationModel.cpp
	crossvalidate.cpp
	${SDM_TensorData_SRC}
)

target_link_libraries( crossvalidate
	mat
	${VTK_LIBRARIES}
	${Boost_PROGRAM_OPTIONS_LIBRARY}
)
