#ifndef VOLUMEUTILS_H
#define VOLUMEUTILS_H

#include <GL/GLTexture.h>
#include "VolumeData.h"

VolumeDataHeader* load_volume_header( const char* filename, int verb );

VolumeDataHeader* load_volume( const char* filename, int verbosity, 
							   void** data_ptr );

bool create_volume_tex( GL::GLTexture& vtex, VolumeDataHeader* vol,
					    void* dataptr, int verbosity=0 );

#endif // VOLUMEUTILS_H
