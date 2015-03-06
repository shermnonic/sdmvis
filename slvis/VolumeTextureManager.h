#ifndef VOLUMETEXTUREMANAGER_H	
#define VOLUMETEXTUREMANAGER_H
	
#include <cstddef> // NULL
#include <vector>
#include <GL/GLConfig.h> // GLint, GLenum

// Forwards
namespace GL
{
	class GLTexture;
}

/**
	Upload 3D image data to the GPU, handle meta data and quantization.
	Can handle scalar and vector fields. 
	Manages OpenGL textures as \a GL::GLTexture.

	@author Max Hermann (hermann@cs.uni-bonn.de), March 2015
*/
class VolumeTextureManager
{
public:
	/// Supported element types
	enum Type
	{
		Char,            ///<  8 bit integer, signed
		UChar,           ///<  8 bit integer, unsigned
		Short,           ///< 16 bit integer, signed
		UShort,          ///< 16 bit integer, unsigned
		Int,             ///< 32 bit integer, signed
		UInt,            ///< 32 bit integer, unsigned
		Float16,         ///< 16 bit floating point
		Float32          ///< 32 bit floating point
	};
	
	/// Data information and raw pointer (CPU memory)
	/// Used to adapt arbitrary (raw) data representations.
	struct Data
	{
		Data(): data(NULL) { reset(); }
		
		int    resolution[3];
		int    components;  // 1 for scalar image, 3 for vector field
		double origin[3];
		double spacing[3];		
		int    type;
		void*  data;
		
		///@{ Convenience setters, can be concatenated via '.'
		Data& setResolution( int rx, int ry, int rz )
		{
			resolution[0] = rx;
			resolution[1] = ry;
			resolution[2] = rz;
			return *this;
		}		
		Data& setComponents( int c )
		{
			components = c;
			return *this;
		}
		Data& setOrigin( double x, double y, double z )
		{
			origin[0] = x;
			origin[1] = y;
			origin[2] = z;
			return *this;
		}
		Data& setSpacing( double sx, double sy, double sz )
		{
			spacing[0] = sx;
			spacing[1] = sy;
			spacing[2] = sz;
			return *this;
		}
		Data& setType( int t )
		{
			type = t;
			return *this;
		}
		Data& setDataPtr( void* ptr )
		{
			data = ptr;
			return *this;
		}
		///@}
		
		void reset()
		{
			resolution[0] = resolution[1] = resolution[2] = 0;
			components = 1;
			origin[0] = origin[1] = origin[2] = 0.0;
			spacing[0] = spacing[1] = spacing[2] = 1.0;
			type = Float32;
			data = NULL;
		}		
	};
	
	/// Quantization options
	struct Quantization
	{
		Quantization(): quantize(false), scale(1.0), bias(0.0) {}
		bool quantize;
		double scale, bias;
	};
	
	/// Uploaded texture information
	struct VolumeTexture
	{
		Data data; ///< Original input data (header information must be valid!)
		int  type; ///< Element type in uploaded texture (one of \a Type)
		Quantization quantization; ///< Applied quantization (if any)
		GL::GLTexture* texture; ///< OpenGL texture
	};

	/// Upload data with identical type
	GL::GLTexture* upload( Data d )
	{		
		return upload( d, d.type );
	}
	
	/// Upload data with given type and quantization (optional)
	GL::GLTexture* upload( Data d, int type, Quantization q=Quantization() );
	
	void destroy();
	void erase( GL::GLTexture* tex );
	int indexOf( GL::GLTexture* tex );
	
protected:
	static GLint getInternalFormat( int numChannels, int type );
	static GLenum VolumeTextureManager::getFormat( int numChannels );
	static GLenum VolumeTextureManager::getType( int type );

	static void VolumeTextureManager::free( VolumeTexture& vt );
	
private:
	std::vector<VolumeTexture> m_textures;
};	

#endif // VOLUMETEXTUREMANAGER_H
