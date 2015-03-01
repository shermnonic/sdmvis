# e7 library

# TODO:
# - split into several libs?
# - use parent directory of e7 for include path
#   (so e7 components are included as e.g. #include <e7/GL/GLTexture.h>)

# e7 paths
set( E7_ENGINES_PATH         ${E7_LIB_PATH}/Engines         )
set( E7_GL_PATH              ${E7_LIB_PATH}/GL              )
set( E7_VOLUMERENDERING_PATH ${E7_LIB_PATH}/VolumeRendering )

set(E7_SOURCES
	${E7_ENGINES_PATH}/Engine.h
#	${E7_ENGINES_PATH}/GLUT/EngineGLUT.h
#	${E7_ENGINES_PATH}/GLUT/EngineGLUT.cpp
#	${E7_ENGINES_PATH}/GLUT/EngineGLUT.h	

	${E7_ENGINES_PATH}/Trackball2.h
	${E7_ENGINES_PATH}/Trackball2.cpp
	
	${E7_GL_PATH}/GLConfig.h
	${E7_GL_PATH}/GLError.cpp
	${E7_GL_PATH}/GLError.h
	${E7_GL_PATH}/GLSLProgram.cpp
	${E7_GL_PATH}/GLSLProgram.h
	${E7_GL_PATH}/GLTexture.cpp
	${E7_GL_PATH}/GLTexture.h
	${E7_GL_PATH}/RenderToTexture.cpp
	${E7_GL_PATH}/RenderToTexture.h

	${E7_VOLUMERENDERING_PATH}/ClipCube.cpp
	${E7_VOLUMERENDERING_PATH}/ClipCube.h
	${E7_VOLUMERENDERING_PATH}/RaycastShader.cpp
	${E7_VOLUMERENDERING_PATH}/RaycastShader.h
	${E7_VOLUMERENDERING_PATH}/VolumeData.cpp
	${E7_VOLUMERENDERING_PATH}/VolumeData.h
	${E7_VOLUMERENDERING_PATH}/RayPickingInfo.cpp
	${E7_VOLUMERENDERING_PATH}/RayPickingInfo.h
	${E7_VOLUMERENDERING_PATH}/VolumeRendererRaycast.cpp
	${E7_VOLUMERENDERING_PATH}/VolumeRendererRaycast.h
	${E7_VOLUMERENDERING_PATH}/LookupTable.h
	${E7_VOLUMERENDERING_PATH}/LookupTable.cpp
	
	${E7_VOLUMERENDERING_PATH}/VolumeData.h
	${E7_VOLUMERENDERING_PATH}/VolumeData.cpp
	${E7_VOLUMERENDERING_PATH}/VolumeUtils.h
	${E7_VOLUMERENDERING_PATH}/VolumeUtils.cpp
	
	${E7_LIB_PATH}/Misc/Filename.h
	${E7_LIB_PATH}/Misc/Filename.cpp
	${E7_LIB_PATH}/Misc/FilesystemTools.h
	${E7_LIB_PATH}/Misc/FilesystemTools.cpp
	${E7_LIB_PATH}/Misc/FilesystemTools.h
	${E7_LIB_PATH}/Misc/ArgumentParsers.cpp
	${E7_LIB_PATH}/Misc/ArgumentParsers.h
	
	${E7_LIB_PATH}/Generative/PerlinNoise.h
	${E7_LIB_PATH}/Generative/PerlinNoise.cpp
	${E7_LIB_PATH}/Generative/NoiseShader.h
	${E7_LIB_PATH}/Generative/NoiseShader.cpp
)

include_directories( 
	${E7_LIB_PATH}/3rdParty 
)

# static library
add_library(e7 STATIC ${E7_SOURCES})
	# target link stuff for dependencies?
