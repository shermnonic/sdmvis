#include "VolumeUtils.h"
#include <GL/glew.h>
#include <Misc/Filename.h>

using namespace std;

namespace {
	int next_power2( int val )
	{
		for( int i=0; i < 32; ++i )
		{
			int pow2 = 1<<i;
			if( val <= pow2 )
				return pow2;
		}
		return 512;
	};
}

//------------------------------------------------------------------------------
//  create_volume_tex()
//------------------------------------------------------------------------------
bool create_volume_tex(GL::GLTexture& vtex, VolumeDataHeader* vol,void* dataptr,
					    int verbosity )
{
	using namespace std;

#if 1	// Create texture (should only be done once!)

	// since glDeleteTexture() silently ignores non existing names it is
	// save to destroy texture first
	vtex.Destroy();

	if( !vtex.Create( GL_TEXTURE_3D ) )
	{
		cerr << "Error: Coudln't create 3D texture!" << endl;
		return false;
	}
#else	// Do not create texture but assume it is already created.
	    // This allows for multiple calls to create_volume_tex().
		//
		// Q: Do we have to delete texture first to free up GPU memory?
	vtex.Bind();
#endif

	// don't forget to set HW supported parameters	
	// GL_CLAMP_TO_EDGE / GL_CLAMP / GL_REPEAT
	vtex.SetWrapMode  ( GL_CLAMP_TO_EDGE ); 
	vtex.SetFilterMode( GL_LINEAR );

	// --- allocate on GPU (no data transfer yet) ---

	// calculate power of two size 
	// FIXME: Does pow2 restriction still apply to recent GPUs?
#if 0
	GLsizei width  = next_power2( vol->resX() );
	GLsizei height = next_power2( vol->resY() );
	GLsizei depth  = next_power2( vol->resZ() );
#else
	GLsizei width  = vol->resX();
	GLsizei height = vol->resY();
	GLsizei depth  = vol->resZ();	
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1); // needed odd sized textures?
#endif
	if( verbosity > 2 ) 
		cout << "Allocate 3D texture of size " 
		     << width << "x" << height << "x" << depth << endl;

	GLint  internalFormat;
	GLenum format;

	switch( vol->numChannels() )
	{
	case 4: internalFormat=GL_RGBA8;  format=GL_RGBA;  break;
	case 3: internalFormat=GL_RGB8;   format=GL_RGB;   break;
	case 1: internalFormat=GL_ALPHA8; format=GL_ALPHA; break;
			//or GL_INTENSITY8 / GL_COLOR_INDEX8 and GL_COLOR_INDEX
	default:
		cerr << "Error: Only scalar (1 channel) and vectorfield (3 channels) volumes supported!" << endl;
		cerr << "       Read volume has " << vol->numChannels() << " channels!" << endl;
		return false;
	}
	if( !vtex.Image( 0, internalFormat, width, height, depth, 0, 
	                   format, GL_UNSIGNED_BYTE, NULL ) )
	{
		cerr << "Error: Couldn't allocate 3D texture!" << endl;
		return false;
	}

	// --- download data ---

	GLenum type;
	switch( vol->elementTypeName() )
	{
	case VolumeDataHeader::UCHAR  :  type=GL_UNSIGNED_BYTE;  break;
	case VolumeDataHeader::USHORT :  type=GL_UNSIGNED_SHORT; break;
	case VolumeDataHeader::FLOAT  :  type=GL_FLOAT;          break;
	default:
		assert(false);
	}

	if( !vtex.SubImage( 0, 0,0,0, vol->resX(),vol->resY(),vol->resZ(),
	                      format, type, dataptr ) )
	{
		cerr << "Error: Couldn't upload volume to 3D texture!" << endl;
		return false;
	}

	return true;
}


//------------------------------------------------------------------------------
//	load_volume()
//------------------------------------------------------------------------------

template<class T>
VolumeData<T>* load_volume_T( const char* filename, VolumeDataHeader* vdh, 
                              int verbosity )
{
	VolumeDataLoaderRAW<T> volumeLoader;
	VolumeData<T>* vol = NULL;
	vol =  volumeLoader.load( filename, vdh, verbosity );

	if( vol && (verbosity > 2)  )
		cout << "Volume size "
	         << vol->resX() <<"x"<< vol->resY() <<"x"<< vol->resZ() <<endl;

	return vol;
}

VolumeDataHeader* load_volume_header( const char* filename, int verb )
{
	Misc::Filename mhdfile( filename );

	// -- load MHD/DAT header --

	if( verb > 1 ) 
		cout <<"Loading volume dataset \""<< mhdfile.filename <<"\"..."<<endl;

	VolumeDataHeader* header = NULL;
	VolumeDataHeaderLoaderMHD* mhd = new VolumeDataHeaderLoaderMHD;
	VolumeDataHeaderLoaderDAT* dat = new VolumeDataHeaderLoaderDAT;

	if( mhd->load( mhdfile.filename.c_str() ) )
	{
		header = mhd;
		delete dat;
	}
	else
	if( dat->load( mhdfile.filename.c_str() ) )
	{
		header = dat;
		delete mhd;
	}
	else
	{
		cerr << "Error: Couldn't load volume header '" << mhdfile.filename
		     << "'!" << endl;
		return NULL;
	}

	return header;
}

VolumeDataHeader* load_volume( const char* filename, int verb, void** data_ptr )
{
	Misc::Filename mhdfile( filename );
	VolumeDataHeader* vol = NULL;

	// -- load MHD/DAT header --
	
	VolumeDataHeader* header = load_volume_header( filename, verb );
	if( !header )
	{
		return NULL;
	}

	// -- load RAW volume data --

	std::string raw_filename = mhdfile.path + header->filename();

#define LOAD_VOLUME_MACRO                                                  \
		VolumeData<T>* volT =NULL;                                         \
		volT = load_volume_T<T>( raw_filename.c_str(), header, verb );     \
		if( !volT ) return NULL;                                           \
		vol = volT;                                                        \
		*data_ptr = (void*)volT->data_ptr();

	if( header->elementTypeName() == VolumeDataHeader::UCHAR )
	{
		typedef unsigned char T;
		LOAD_VOLUME_MACRO
	}
	else
	if( header->elementTypeName() == VolumeDataHeader::USHORT )
	{
		typedef unsigned short T;
		LOAD_VOLUME_MACRO
	}
	else
	if( header->elementTypeName() == VolumeDataHeader::FLOAT )
	{
		typedef float T;
		LOAD_VOLUME_MACRO
	}
	else
	{
		cerr << "Unsupported volume element type!" << endl;
		return NULL;
	}

	return vol;
}
