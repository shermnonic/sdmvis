#include "VolumeTextureManager.h"
#include <GL/GLConfig.h>
#include <GL/GLTexture.h>
#include <GL/GLError.h>
#include <iostream>

using std::cerr;
using std::endl;

bool gl_adjustPixeltransfer( double comp_scale, double comp_bias )
{
	// Adjust pixel transfer to shift/scale signed field data.
	// OpenGL first scales, then adds bias before clamping to [0,1].
	GLenum
		GL_c_BIAS[3] = { GL_RED_BIAS, GL_GREEN_BIAS, GL_BLUE_BIAS },
		GL_c_SCALE[3] = { GL_RED_SCALE, GL_GREEN_SCALE, GL_BLUE_SCALE };
	for( int i=0; i < 3; ++i )
	{
		glPixelTransferf( GL_c_SCALE[i], (GLfloat)comp_scale );
		glPixelTransferf( GL_c_BIAS[i] , (GLfloat)comp_bias );
	}

	return !GL::CheckGLError("adjust_pixeltransfer()");
}

void VolumeTextureManager::free( VolumeTextureManager::VolumeTexture& vt )
{
	if( vt.texture )
	{
		vt.texture->Destroy();
		delete vt.texture;
		vt.texture = NULL;	
	}
}

void VolumeTextureManager::destroy()
{
	for( unsigned i=0; i < m_textures.size(); i++ )
	{
		free( m_textures[i] );
	}
}

void VolumeTextureManager::erase( GL::GLTexture* tex  )
{
	int idx = indexOf( tex );
	if( idx>=0 )
	{
		free( m_textures[idx] );
		m_textures.erase( m_textures.begin() + idx );
	}
}

int VolumeTextureManager::indexOf( GL::GLTexture* tex )
{
	for( unsigned i=0; i < m_textures.size(); i++ )
	{
		if( m_textures[i].texture == tex )
			return i;
	}
	return -1;
}

GLint VolumeTextureManager::getInternalFormat( int numChannels, int type )
{
	GLint internalFormat;
	if( numChannels == 4 )
	{
		switch( type )
		{
		case Float32 : internalFormat = GL_RGBA32F;  break;
		case Float16 : internalFormat = GL_RGBA16F;  break;
		case Int     : internalFormat = GL_RGBA32I;  break;
		case UInt    : internalFormat = GL_RGBA32UI; break;
		case Short   : internalFormat = GL_RGBA16I;  break;
		case UShort  : internalFormat = GL_RGBA16UI; break;
		case Char    : internalFormat = GL_RGBA8;    break;
		case UChar   : internalFormat = GL_RGBA8UI;  break;
		default:
			// Unknown format ?!
			internalFormat = -1; break;
		}
	}
	else if( numChannels == 3 )
	{
		switch( type )
		{
		case Float32 : internalFormat = GL_RGB32F;  break;
		case Float16 : internalFormat = GL_RGB16F;  break;
		case Int     : internalFormat = GL_RGB32I;  break;
		case UInt    : internalFormat = GL_RGB32UI; break;
		case Short   : internalFormat = GL_RGB16I;  break;
		case UShort  : internalFormat = GL_RGB16UI; break;
		case Char    : internalFormat = GL_RGB8;    break;
		case UChar   : internalFormat = GL_RGB8UI;  break;
		default:
			// Unknown format ?!
			internalFormat = -1; break;
		}
	}
	else if( numChannels == 2 )
	{
		switch( type )
		{
		case Float32 : internalFormat = GL_RG32F;  break;
		case Float16 : internalFormat = GL_RG16F;  break;
		case Int     : internalFormat = GL_RG32I;  break;
		case UInt    : internalFormat = GL_RG32UI; break;
		case Short   : internalFormat = GL_RG16I;  break;
		case UShort  : internalFormat = GL_RG16UI; break;
		case Char    : internalFormat = GL_RG8;    break;
		case UChar   : internalFormat = GL_RG8UI;  break;
		default:
			// Unknown format ?!
			internalFormat = -1; break;
		}
	}
	else if( numChannels == 1 )
	{
		switch( type )
		{
		case Float32 : internalFormat = GL_R32F;  break;
		case Float16 : internalFormat = GL_R16F;  break;
		case Int     : internalFormat = GL_R32I;  break;
		case UInt    : internalFormat = GL_R32UI; break;
		case Short   : internalFormat = GL_R16I;  break;
		case UShort  : internalFormat = GL_R16UI; break;
		case Char    : internalFormat = GL_R8;    break;
		case UChar   : internalFormat = GL_R8;    break; // GL_R8UI ?
		default:
			// Unknown format ?!
			internalFormat = -1; break;
		}
	}
	else
	{
		// Unsupported number of channels ?!
		internalFormat = -1;
	}
	return internalFormat;
}

GLenum VolumeTextureManager::getFormat( int numChannels )
{
	GLenum format;
	switch( numChannels )
	{
	case 4: format = GL_RGBA; break;
	case 3: format = GL_RGB;  break;
	case 2: format = GL_RG;   break;
	case 1: format = GL_RED;  break;
	default:
		// Unsupported number of channels ?!
		assert(false);
	}
	return format;
}

GLenum VolumeTextureManager::getType( int type )
{
	GLenum internalFormat;
	switch( type )
	{
	case Float32 : internalFormat = GL_FLOAT;          break;
	case Float16 : internalFormat = GL_FLOAT;          break;
	case Int     : internalFormat = GL_INT;            break;
	case UInt    : internalFormat = GL_UNSIGNED_INT;   break;
	case Short   : internalFormat = GL_SHORT;          break;
	case UShort  : internalFormat = GL_UNSIGNED_SHORT; break;
	case Char    : internalFormat = GL_BYTE;           break;
	case UChar   : internalFormat = GL_UNSIGNED_BYTE;  break;
	default:
		// Unknown format ?!
		assert(false);
	}
	return internalFormat;
}

GL::GLTexture* VolumeTextureManager::upload( Data data, int type, Quantization quant )
{
	GL::CheckGLError("VolumeTextureManager::upload() : at beginning");

	// Create texture
	GL::GLTexture* tex;
	tex = new GL::GLTexture;
	if( !tex->Create( GL_TEXTURE_3D ) )
	{
		cerr << "Error: Couldn't create 3D texture!" << endl;
		delete tex;
		return NULL;		
	}
	
	// Set hardware supported texture parameters
	tex->SetWrapMode( GL_CLAMP_TO_EDGE );
	tex->SetFilterMode( GL_LINEAR );	

	// OpenGL formats and type
	GLint internalFormat = getInternalFormat( data.components, type );	
	GLenum format = getFormat( data.components );
	GLenum gltype = getType( type );

	// Allocate
	if( !tex->Image( 0, internalFormat, 
		data.resolution[0], data.resolution[1], data.resolution[2], 0,
		format, GL_UNSIGNED_BYTE, NULL ) ) // gltype does not matter for alloc
	{
		cerr << "Error: Couldn't allocate 3D texture!" << endl;
		tex->Destroy();
		delete tex;
		return NULL;		
	}		
	
	// Setup quantization via OpenGL pixel transfer
	if( quant.quantize && (quant.scale > 0.0 || quant.bias != 1.0) )
		gl_adjustPixeltransfer( quant.scale, quant.bias );
	
	// WORKAROUND for odd sized textures?
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
	
	// Upload
	if( !tex->SubImage( 0, 0,0,0,
		data.resolution[0], data.resolution[1], data.resolution[2],
		format, gltype, data.data ) )
	{
		cerr << "Error: Couldn't upload volume to 3D texture!" << endl;
		tex->Destroy();
		delete tex;
		return NULL;
	}
	
	// Reset OpenGL pixel transfer
	if( quant.quantize )
		gl_adjustPixeltransfer( 1.0, 0.0 );
	
	// Store
	VolumeTexture vt;
	vt.data = data;
	vt.type = type;
	vt.quantization = quant;
	vt.texture = tex;
	
	m_textures.push_back( vt );
	
	return tex;
}

