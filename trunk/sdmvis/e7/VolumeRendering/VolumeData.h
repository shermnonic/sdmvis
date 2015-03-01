//=============================================================================
//
//  Volume data structures:
//
//  - VolumeDataHeader
//  - VolumeDataAllocator<T>
//  - VolumeDataLoaderRAW<T>
//  - VolumeDataWriterRAW<T>
//  - VolumeDataSetFromRAW<T>
//  - VolumeData<T>             : public VolumeDataHeader
//  - VolumeDataHeaderLoaderMHD : public VolumeDataHeader
//  - VolumeDataHeaderLoaderDAT : public VolumeDataHeader  (*untested!*)
//  - VolumeAccessor
//
//  TODO:
//  * refactor DataLoader to DataReader
//  * Factory of loaders/readers
//  * forward T* VolumeData<T>::data_ptr() to void* VolumeDataHeader::void_data_ptr()
//
//	Max Hermann, August 14, 2009
//  Last changed: March 24, 2010
//
//=============================================================================
#ifndef __VOLUMEDATA_H
#define __VOLUMEDATA_H

// use std::vector instead of char* as buffer
//#define VOLUME_STD_VECTOR

// default verbosity if none is explicitly specified by the callee 
// (0=no output, 3=debug level output)
#define VOLUMEDATA_DEFAULT_VERBOSITY 0 

#ifdef WIN32
// disable some VisualStudio warnings
#pragma warning(disable: 4244) 
  // warning C4244: '=' : conversion from 'double' to 'T', possible loss of data
  //              -> occurs in VolumeData<T>::interp
#endif

#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <cassert>
#include <cmath>

//-----------------------------------------------------------------------------
//
//	VolumeDataHeader
//
//-----------------------------------------------------------------------------

class VolumeAlignmentPCA;

/// \ingroup VolumeTools
class VolumeDataHeader
{
public:
	friend class VolumeAlignmentPCA;
	
	enum ElementTypeName 
	{ 
		UCHAR  =1,
		USHORT =2,
		FLOAT
	};
	
	VolumeDataHeader()
		: m_elementType(UCHAR),
		  m_numChannels(1),
		  m_resX(0), 
		  m_resY(0), 
		  m_resZ(0),
		  m_spacingX(1.), 
		  m_spacingY(1.), 
		  m_spacingZ(1.)
		{}

	virtual ~VolumeDataHeader() 
	{
	}

	virtual void clear() {}
	
	/// Volume element type
	ElementTypeName elementTypeName() const { return m_elementType; }
	
	///@{ Volume resolution
	unsigned int resX() const { return m_resX; }
	unsigned int resY() const { return m_resY; }
	unsigned int resZ() const { return m_resZ; }
	///@}
	
	///@{ Voxel spacing
	double spacingX() const { return m_spacingX; }
	double spacingY() const { return m_spacingY; }
	double spacingZ() const { return m_spacingZ; }
	///@}

	/// Number of channels per element
	/// For example scalar field has 1 channel, vector field has 3 channels.
	unsigned int numChannels() const { return m_numChannels; }
	
	/// Filename of volume data file
	std::string filename() const { return m_filename; }

	virtual void* rawPtr() { return NULL; }
	
protected:
	ElementTypeName m_elementType;
	unsigned        m_numChannels;
	unsigned int    m_resX, m_resY, m_resZ;
	double          m_spacingX, m_spacingY, m_spacingZ;
	std::string m_filename;
};

//-----------------------------------------------------------------------------
//
//	VolumeData
//
//-----------------------------------------------------------------------------

template<class T>
class VolumeDataLoaderRAW;

template<class T>
class VolumeDataWriterRAW;

template<class T>
class VolumeDataAllocator;

template<class T>
class VolumeDataSetFromRAW;

/// \ingroup VolumeTools
template<class T>
class VolumeData : public VolumeDataHeader
{
public:
	friend class VolumeDataLoaderRAW<T>;
	friend class VolumeDataWriterRAW<T>;
	friend class VolumeDataAllocator<T>;
	friend class VolumeDataSetFromRAW<T>;

	typedef T ElementType;

	VolumeData()
		: m_verbosity(0),
		  m_deleteBuffer(true),
		  m_buffer(0)
		{}

	void clear()
		{
			if( m_deleteBuffer )
			{
				delete [] m_buffer;
			}
			m_buffer=NULL;
			
			// Don't reset header information! clear() is intended to free up
			// allocated memory but keep the meta information.
			//*this = VolumeData();  // TEST: does this work as intended?
		}

	void zero()
		{
			if( m_buffer )
			{
				// m_size seems not to be initialized, so calculate size by hand
				memset( m_buffer, 0, resX()*resY()*resZ()*numChannels()*sizeof(T) );
			}
		}
	
	virtual ~VolumeData()
		{
			clear();
		}
	
	T operator() ( int x, int y, int z ) const
		{	
			assert( m_buffer );
			int ofs = z*m_resX*m_resY + y*m_resX + x;
			return m_buffer[ofs];
		}

	T& operator() ( int x, int y, int z )
		{	
			assert( m_buffer );
			int ofs = z*m_resX*m_resY + y*m_resX + x;
			return m_buffer[ofs];
		}

	/// Returns (typed) pointer to volume data buffer (use with care!)
	T* data_ptr()
		{
			assert( m_buffer );
			return &m_buffer[0];
		}

	void* rawPtr() 
		{ 
			return (void*)data_ptr(); 
		}

	/// Trilinear interpolation
	T interp( double x, double y, double z ) const;

protected:
	void setBufferPtr( void* ptr, bool takeOwnerShip=true )
	{
		m_buffer = (T*)ptr;
		m_deleteBuffer = takeOwnerShip;

		if( m_verbosity > 2 ) 
			std::cout << "Volume buffer address = 0x" 
				 << std::hex << (int)m_buffer << std::dec << "\n";
	}

private:
	int m_verbosity;
	bool m_deleteBuffer;
	T* m_buffer;
};


//-----------------------------------------------------------------------------
//
//	VolumeDataSetFromRAW
//
//-----------------------------------------------------------------------------
template<class T>
class VolumeDataSetFromRAW
{
public:
	/// Specialized constructor to set element type according to template type
	VolumeDataSetFromRAW();
	
	void setSpacing( double sx, double sy, double sz )
	{
		m_spacing[0] = sx;
		m_spacing[1] = sy;
		m_spacing[2] = sz;
	}
	//void setOffset( double x, double y, double z );
	//void setOrigin( double x, double y, double z );

	VolumeData<T>* setFromRaw( int resX, int resY, int resZ,
		T* rawData, bool takeOwnerShip );
protected:
	VolumeDataHeader::ElementTypeName m_elementType;
private:
	double   m_spacing[3];
};

//-----------------------------------------------------------------------------
//
//	VolumeDataAllocator
//
//-----------------------------------------------------------------------------

//template<class T1, class T2>
//class VolumeDataConverter
//{
//public:
//	VolumeData<T2>* convert( VolumeData<T1>* vol, float scale, float shift );
//};

/// \ingroup VolumeTools
template<class T>
class VolumeDataAllocator
{
public:
	/// Specialized constructor to set element type according to template type
	VolumeDataAllocator();

	/// Allocate volume of the given size.
	/// \todo Let the user specify additional header information like spacing.
	VolumeData<T>* allocate( int resX, int resY, int resZ, 
                double spacingX=1.0, double spacingY=1.0, double spacingZ=1.0 );
protected:
	VolumeDataHeader::ElementTypeName m_elementType;
};

//-----------------------------------------------------------------------------
//
//	VolumeDataLoaderRAW
//
//-----------------------------------------------------------------------------
/// \ingroup VolumeTools
template<class T>
class VolumeDataLoaderRAW
{
public:	
	static T* loadRAW( const char* filename, int& bytesRead, 
		               int verbosity=VOLUMEDATA_DEFAULT_VERBOSITY );

	VolumeDataLoaderRAW()
		: m_gcount(0)
	{}

	/// Load volume data from uncompressed raw file
	/// @param filename filename of raw data file
	/// @param header meta image header object (usually read from a MHD file)
	/// @param verbosity output verbosity (0=no output, 3=debug level output)
	VolumeData<T>* load( const char* filename, VolumeDataHeader* header,
		                 int verbosity=VOLUMEDATA_DEFAULT_VERBOSITY );

	/// Return number of bytes read on last call to load().
	int gcount() const { return m_gcount; }

private:
	int m_gcount;
};

//-----------------------------------------------------------------------------
//
//	VolumeDataWriterRAW
//
//-----------------------------------------------------------------------------
/// \ingroup VolumeTools
template<class T>
class VolumeDataWriterRAW
{
public:	
	static void writeRAW( const char* filename, const VolumeData<T>* volume,
		                  int verbosity=VOLUMEDATA_DEFAULT_VERBOSITY );

	static void writeMHD( const char* filename, const VolumeData<T>* volume,
		                  int verbosity=VOLUMEDATA_DEFAULT_VERBOSITY );

	/// Write volume data to uncompressed RAW file with accompanying MHD file.
	/// @param name filename of raw data output file (extension ignored)
	/// @param volume volume data object (with correctly set header infos)
	/// @param verbosity output verbosity (0=no output, 3=debug level output)
	void write( const char* name,
	            const VolumeData<T>* volume,
		        int verbosity=VOLUMEDATA_DEFAULT_VERBOSITY );
};

//-----------------------------------------------------------------------------
//
//	VolumeDataHeaderLoaderMHD
//
//-----------------------------------------------------------------------------
/// \ingroup VolumeTools
class VolumeDataHeaderLoaderMHD : public VolumeDataHeader
{
public:
	VolumeDataHeaderLoaderMHD() {}
	VolumeDataHeaderLoaderMHD( const VolumeDataHeader& super )
		: VolumeDataHeader(super)
		{}
	bool load( const char* filename );
	void save( const char* filename );
	void setDataFilename( std::string fname ) { m_filename = fname; }
};

//-----------------------------------------------------------------------------
//
//	VolumeDataHeaderLoaderDAT
//
//-----------------------------------------------------------------------------
/// \ingroup VolumeTools
class VolumeDataHeaderLoaderDAT : public VolumeDataHeader
{
public:
	VolumeDataHeaderLoaderDAT() {}
	VolumeDataHeaderLoaderDAT( const VolumeDataHeader& super )
		: VolumeDataHeader(super)
		{}
	bool load( const char* filename );
	//void save( const char* filename ); // TBD
	void setDataFilename( std::string fname ) { m_filename = fname; }
};

//-----------------------------------------------------------------------------
//
//	VolumeAccessor
//
//-----------------------------------------------------------------------------

/// Accessor for volume data which supports dynamic voxel operations
/// like tresholding. Dynamic means here that the volume data is not changed
/// but the specified operations/filters are applied online when a voxel
/// is accessed via operator().
/// \ingroup VolumeTools
template<class T>
class VolumeAccessor : public VolumeDataHeader
{
public:
	/// Methods to handle out-of-bounds indexing
	enum IndexMode { 
		NoIndexCheck,  ///< perform no range checking at all
		ClampIndex,    ///< clamp index range to volume resolution
		ZeroIndex      ///< return zero intensity outside index range
//		MirrorIndex,   ///< mirror index range
	};

	VolumeAccessor()
		: m_vol(0), m_threshold(false), m_indexMode(NoIndexCheck)
		{}

	void setVolumeData( VolumeData<T>* vol );

	float operator() ( int x, int y, int z ) const;

	/// Apply simple threshold excluding voxel intensities outside.
	/// Excluded voxels are set to zero.
	/// Threshold filter is automatically enabled.
	void setThreshold( T lower, T upper );
	void setThresholdEnabled( bool enable );
	void setIndexMode( IndexMode mode );

private:
	VolumeData<T>* m_vol;
	bool m_threshold;
	T m_threshLo, m_threshUp;
	IndexMode m_indexMode;
};


//=============================================================================
//
//	Template implementations
//
//=============================================================================

template<class T>
void VolumeDataWriterRAW<T>::write( const char* fname,
                                    const VolumeData<T>* volume,
                                    int verbosity                )
{
	if( !volume )
		return;

	using namespace std;
	string filename( fname ), path, name;

	// replace backslashes (indicating directory structure) by slashes
	for( unsigned int i=0; i < filename.length(); ++i )
		if( filename[i] == '\\' )  filename[i] = '/';					
	
	// extract path and name
	path = "";
	size_t seploc;
	seploc = filename.find_last_of("/");
	if( seploc != string::npos )
		path = filename.substr(0,seploc+1);
	else
		seploc = -1;
	size_t ptloc = filename.find_last_of(".");
	name = filename.substr( seploc+1, ptloc-(seploc+1) );

	string filename_mhd = path + name + ".mhd";
	string filename_raw = path + name + ".raw";

	// write MHD
	if( verbosity > 2 ) cout << "Saving MHD file '" << filename_mhd << "'..." << endl;
	VolumeDataHeader mhd = (VolumeDataHeader)*volume;
	VolumeDataHeaderLoaderMHD ml( mhd );
	ml.setDataFilename( name + string(".raw") );
	ml.save( filename_mhd.c_str() );

	// write RAW
	if( verbosity > 2 ) cout << "Saving RAW file '" << filename_raw << "'..." << endl;
	writeRAW( filename_raw.c_str(), volume, verbosity );
}

template<class T>
void VolumeDataWriterRAW<T>::writeRAW( const char* filename,
                                       const VolumeData<T>* volume,
                                       int verbosity                )
{
	using namespace std;

	ofstream f;
	if( verbosity > 2 ) cout << "Opening file " << filename 
		                     << " for writing " << endl;

	// open file
	f.open( filename, ios::out | ios::binary );
	if( !f.is_open() )
	{
		cerr << "Error: Couldn't open file " << filename << "!" << endl;
	}
	
	// write data as block
	long size = volume->resX()*volume->resY()*volume->resZ() * sizeof(T);
	f.write( (char*)volume->m_buffer, size );

	f.close();
}

template<class T>
T* VolumeDataLoaderRAW<T>::loadRAW( const char* filename, 
                                    int& bytesRead,
									int verbosity         )
{
	using namespace std;
	ifstream f;

	if( verbosity > 2 ) cout << "Opening file " << filename << endl;

	// open file
	f.open( filename, ios::in | ios::binary );	
	if( !f.is_open() )
	{
		cerr << "Error: Couldn't open file " << filename << "!" << endl;
		return 0;
	}

	// get file size
	f.seekg( 0, ios::end );
	int size = (int)f.tellg();
	f.seekg( 0, ios::beg );

	if( verbosity > 2 ) cout << "Allocate " << size/(1024*1024) << " MB..." << endl;
	
	// allocate buffer
	T* buffer = 0;
	try
	{
		buffer = new T[ size / sizeof(T) ];	
	}
	catch( bad_alloc& )
	{
		cerr << "Error allocating " << size/(1024*1024) << "MB of memory!" << endl;
		return 0;
	}
	
	if( verbosity > 2 ) cout << "Buffer address=0x" << std::hex << (int)buffer << std::dec << endl;	
	if( verbosity > 2 ) cout << "Read data as one block..."; cout.flush();

	// read data as block
	f.read( (char*)buffer, size );
	bytesRead = (int)f.gcount();

	if( verbosity > 2 ) cout << "done, " << bytesRead << " bytes read." << endl;
	if( verbosity > 2 ) cout << "Closing file" << endl;

	// close file
	f.close();
	return buffer;
}

template<class T>
VolumeData<T>* VolumeDataLoaderRAW<T>::load( const char* filename, 
											 VolumeDataHeader* header,
											 int verbosity             )
{
	if( verbosity > 2 ) std::cout << "Creating new VolumeData object" << std::endl;
	VolumeData<T>* volume = new VolumeData<T>();
	
	// set header part (resolution, spacing, etc.)
	*(dynamic_cast<VolumeDataHeader*>( volume )) = *header;
	
	// load raw volume data
	T* buffer = loadRAW( filename, m_gcount, verbosity );
	if( !buffer )
	{			
		delete volume; volume = 0;
		return 0;
	}

	// check size
	int size = volume->resX()*volume->resY()*volume->resZ() * sizeof(T) * volume->numChannels();
	if( size != m_gcount )
	{
		std::cerr << "Error: Size mismatch between volume dimensions described "
			      << "in header file and RAW volume data size!" << std::endl;
		delete volume; volume = 0;
		return 0;
	}

	volume->setBufferPtr( buffer );
	return volume;
}	


template<class T>
void VolumeAccessor<T>::setVolumeData( VolumeData<T>* vol )
{
	// copy header information (like resolution and spacing)
	if( vol )
		*((VolumeDataHeader*)this) = *(VolumeDataHeader*)vol;
	m_vol = vol;
}

template<class T>
void VolumeAccessor<T>::setThreshold( T lower, T upper )
{
	m_threshLo = lower;
	m_threshUp = upper;
	m_threshold = true;
}

template<class T>
void VolumeAccessor<T>::setThresholdEnabled( bool enable )
{
	m_threshold = enable;
}

template<class T>
void VolumeAccessor<T>::setIndexMode( IndexMode mode )
{
	m_indexMode = mode;
}

template<class T>
float VolumeAccessor<T>::operator() ( int x, int y, int z ) const
{
	if( !m_vol ) 
	{
		std::cerr << "Warning: Trying to access NULL volume!" << std::endl;
		return 0.;
	}

	// adjust indices
	switch( m_indexMode )
	{
		default:
		case NoIndexCheck:
			break;
		case ClampIndex:
			// clamp to volume borders
			if( x < 0 ) x = 0;  else  if( x > (int)resX()-1 )  x = resX()-1;
			if( y < 0 ) y = 0;  else  if( y > (int)resY()-1 )  y = resY()-1;
			if( z < 0 ) z = 0;  else  if( z > (int)resZ()-1 )  z = resZ()-1;
			break;
		case ZeroIndex:
			// return zero if any coordinate is outside volume
			if( (x < 0) || (x >= (int)resX()) ||
				(y < 0) || (y >= (int)resY()) ||
				(z < 0) || (z >= (int)resZ())    )  return 0.f;
			break;
	}

	// get voxel intensity
	T v = (*m_vol)(x,y,z);

	// apply threshold
	if( m_threshold )
	{
		v = ((v >= m_threshLo) && (v <= m_threshUp)) ? v : T(0);
	}

	return (float)v;
}

template<class T>
VolumeData<T>* VolumeDataAllocator<T>::allocate( int resX, int resY, int resZ,
							double spacingX, double spacingY, double spacingZ )
{
	using namespace std;

	int verbosity = 0;

	if( verbosity > 2 ) std::cout << "Creating new VolumeData object" << std::endl;
	VolumeData<T>* volume = new VolumeData<T>();
	
	// set header part (resolution, spacing, etc.)
	volume->m_resX = resX;
	volume->m_resY = resY;
	volume->m_resZ = resZ;
	volume->m_spacingX = spacingX;
	volume->m_spacingY = spacingY;
	volume->m_spacingZ = spacingZ;
	volume->m_filename = "";
	volume->m_elementType = m_elementType;
	// m_elementType is set in specialized constructor
	
	// allocate buffer
	int size = resX*resY*resZ * sizeof(T);
	T* buffer = 0;
	try
	{
		buffer = new T[ size / sizeof(T) ];	
	}
	catch( std::bad_alloc& )
	{
		cerr << "Error allocating " << size/(1024*1024) << "MB of memory!" << endl;
		return 0;
	}

	volume->setBufferPtr( (void*)buffer, true );
	return volume;
}

template<class T>
VolumeData<T>* VolumeDataSetFromRAW<T>::
	setFromRaw( int resX, int resY, int resZ, T* rawData, bool takeOwnerShip )
{
	using namespace std;
	int verbosity = 0;
	if( verbosity > 2 ) cout << "Creating new VolumeData object" << endl;
	VolumeData<T>* volume = new VolumeData<T>();

	// set header part (resolution, spacing, etc.)
	volume->m_resX = resX;
	volume->m_resY = resY;
	volume->m_resZ = resZ;
	volume->m_spacingX = m_spacing[0];
	volume->m_spacingY = m_spacing[1];
	volume->m_spacingZ = m_spacing[2];
	volume->m_filename = "";
	volume->m_elementType = m_elementType;
	// m_elementType is set in specialized constructor

	volume->setBufferPtr( (void*)rawData, takeOwnerShip );	
	return volume;
}

template<class T>
T VolumeData<T>::interp( double x, double y, double z ) const
{
	using std::floor;

	// trilinear interpolation
	// slightly simplified since voxel size is canonical (1,1,1)
	T f;

	// voxel position
	int x0 = (int)floor(x),
		y0 = (int)floor(y),
		z0 = (int)floor(z);

	// boundary conditionss
	int x1 = x0 + 1,
		y1 = y0 + 1,
		z1 = z0 + 1;
	int  condx = (int)(x1 >= (int)m_resX),
		 condy = (int)(y1 >= (int)m_resY),
		 condz = (int)(z1 >= (int)m_resZ);
	if( (condx + condy + condz) == 0 )
	{
		// A) trilinear interpolation

		// weights 
		double a = x - x0,
			   b = y - y0,
			   c = z - z0;

		// evaluate corners
		T f000 = (*this)(x0,y0,z0), f100 = (*this)(x1,y0,z0),
		  f010 = (*this)(x0,y1,z0), f110 = (*this)(x1,y1,z0),
		  f001 = (*this)(x0,y0,z1), f101 = (*this)(x0,y1,z0),
		  f011 = (*this)(x0,y1,z1), f111 = (*this)(x1,y1,z1);

		// interpolate
		f = f000*((1-a)*(1-b)*(1-c)) +   f100*(a *(1-b)*(1-c))  +
		    f010*((1-a)*   b *(1-c)) +   f110*(a *   b *(1-c))  +
		    f001*((1-a)*(1-b)*   c ) +   f101*(a *(1-b)*   c )  +
		    f011*((1-a)*   b *   c ) +   f111*(a *   b *   c );
	}
	else
	if( (condx + condy + condz) == 1 )
	{
		// B) bilinear interpolation

		T f00, f10, f01, f11;
		double a,b;

		f00 = (*this)(x0,y0,z0);

		if( condx ) //  along y-z
		{
			                         f10 = (*this)(x0,y1,z0);
			f11 = (*this)(x0,y1,z1); f01 = (*this)(x0,y0,z1); 
			a = y - y0;
			b = z - z0;
		}
		else
		if( condy ) //  along x-z
		{
			                         f10 = (*this)(x1,y0,z0);
			f11 = (*this)(x1,y0,z1); f01 = (*this)(x0,y0,z1); 
			a = x - x0;
			b = z - z0;
		}
		else
		if( condz ) //  along x-y
		{
			                         f10 = (*this)(x1,y0,z0);
			f11 = (*this)(x1,y1,z0); f01 = (*this)(x0,y1,z0); 
			a = x - x0;
			b = y - y0;
		}
		else
			assert(false);	// should never be reached

		// interpolate
		f = f00*((1-a)*(1-b)) + f10*(a*(1-b)) + f01*((1-a)*b) + f11*(a*b);
	}
	else
	if( (condx + condy + condz) == 2 )
	{
		// C) linear interpolation
		T f0, f1;
		double a;
		f0 = (*this)(x0,y0,z0);

		if( !condx )
		{
			f1 = (*this)(x1,y0,z0);
			a = x - x0;
		}
		else 
		if( !condy )
		{
			f1 = (*this)(x0,y1,z0);
			a = y - y0;
		}
		else
		if( !condz )
		{
			f1 = (*this)(x0,y0,z1);
			a = z - z0;
		}
		else
			assert(false);  // should never be reached

		// interpolate
		f = f0*(1-a) + f1*a;
	}
	else
		// D) corner voxel
		f = (*this)(x0,y0,z0);


	return f;
}



#endif // __VOLUMEDATA_H
