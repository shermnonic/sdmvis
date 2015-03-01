#include "TensorData.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cassert>
#include <cstdio>
#include <cstring> // strcmp
#include <vector>
#include <boost/algorithm/string.hpp> // for case-insensitive compare iequals() 

#ifdef TENSORVIS_TEEM_SUPPORT
#include <teem/nrrd.h>
#endif

const char TensorData::s_magic[9] = "MXVOL001";

// Utility functions

std::string getFileExt( const char* filename )
{
	using namespace std;
	string fname( filename );
	size_t sep = fname.find_last_of('.');
	if( sep != string::npos )	
		return fname.substr(sep);
	return "";
}


//-----------------------------------------------------------------------------
//  C'tor
//-----------------------------------------------------------------------------
TensorData::TensorData()
: m_data(NULL),
  m_mask(NULL)
{
}

//-----------------------------------------------------------------------------
//  D'tor
//-----------------------------------------------------------------------------
TensorData::~TensorData()
{
	freeData();	
	if( m_mask ) delete [] m_mask;
}

//-----------------------------------------------------------------------------
//  setTensors()
//-----------------------------------------------------------------------------
void TensorData::setTensors( float* tensors, ImageDataSpace space )
{
	freeData();
	m_data  = tensors;
	m_space = space;
}

//-----------------------------------------------------------------------------
//  freeData()
//-----------------------------------------------------------------------------
void TensorData::freeData()
{	
	if(m_data) delete [] m_data; m_data=NULL;
	m_space.reset();
}

//-----------------------------------------------------------------------------
//  allocateData()
//-----------------------------------------------------------------------------
void TensorData::allocateData( ImageDataSpace space )
{
	freeData();
	m_space = space;
	m_data = new float[ getSize() ];

	// On allocation we also zero the data
	memset( (void*)m_data, 0, getSize()*sizeof(float) );
}

//-----------------------------------------------------------------------------
//  Addressing
//-----------------------------------------------------------------------------
unsigned TensorData::getSize() const
{
	return m_space.getNumberOfPoints() * (9+3);
}

size_t TensorData::getTensorOfs( int x, int y, int z ) const
{
	return m_space.getPointIndex(x,y,z) * (9+3);
}

size_t TensorData::getVectorOfs( int x, int y, int z ) const
{
	return m_space.getPointIndex(x,y,z) * (9+3) + 9;
}

//-----------------------------------------------------------------------------
//  Setters
//-----------------------------------------------------------------------------
void TensorData::setTensor( int x, int y, int z, double* tensor3x3 )
{
	size_t ofs = getTensorOfs(x,y,z);
	for( int i=0; i < 9; ++i )
		m_data[ofs+i] = (float)tensor3x3[i];
}

void TensorData::setTensor( int x, int y, int z, float* tensor3x3 )
{
	size_t ofs = getTensorOfs(x,y,z);
	for( int i=0; i < 9; ++i )
		m_data[ofs+i] = tensor3x3[i];
}

void TensorData::setVector( int x, int y, int z, float* v3 )
{
	if( !v3 ) return;
	size_t ofs = getVectorOfs(x,y,z);
	for( int i=0; i < 3; ++i )
		m_data[ofs+i] = v3[i];
}

void TensorData::setVector( int x, int y, int z, double* v3 )
{
	if( !v3 ) return;
	size_t ofs = getVectorOfs(x,y,z);
	for( int i=0; i < 3; ++i )
		m_data[ofs+i] = (float)v3[i];
}

// Set directly by index

void TensorData::setTensor( int id, double* tensor3x3 )
{
	size_t ofs = id * (9+3);
	for( int i=0; i < 9; ++i )
		m_data[ofs+i] = (float)tensor3x3[i];
}

void TensorData::setVector( int id, double* v3 )
{
	size_t ofs = id * (9+3) + 9;
	for( int i=0; i < 3; ++i )
		m_data[ofs+i] = (float)v3[i];
}


//-----------------------------------------------------------------------------
//  getTensor()
//-----------------------------------------------------------------------------
void TensorData::getTensor( int x, int y, int z, float (&tensor3x3)[9] ) const
{
	size_t ofs = getTensorOfs(x,y,z);

  #if 0
	memcpy( (char*)tensor3x3[0], (char*)&(m_data[ofs]), 9*sizeof(float) );
  #else
	for( int i=0; i < 9; ++i )
		tensor3x3[i] = m_data[ofs+i];
		//was: (*tensor3x3)[i] = m_data[ofs+i];
  #endif
}

//-----------------------------------------------------------------------------
//  getVector()
//-----------------------------------------------------------------------------
void TensorData::getVector( int x, int y, int z, float (&v)[3] ) const
{
	size_t ofs = getVectorOfs(x,y,z);

  #if 0
	memcpy( (char*)v[0], (char*)&(m_data[ofs]), 3*sizeof(float) );
  #else
	for( int i=0; i < 3; ++i )
		v[i] = m_data[ofs+i];
		//was: (*v)[i] = m_data[ofs+i];
  #endif
}

//-----------------------------------------------------------------------------
//  getMask()
//-----------------------------------------------------------------------------
float TensorData::getMask( int x, int y, int z ) const
{
	if( !m_mask )
		return 1.f;

	size_t ofs = m_space.getPointIndex(x,y,z);
	if( ofs >= m_space.getNumberOfPoints() )
	{
		std::cerr << "TensorData::getMask(): Invalid address!" << std::endl;
		return 0.f;
	}
	return m_mask[ofs];
}

//-----------------------------------------------------------------------------
//  load()
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/// Provided for convenience, sets default spacing and origin
bool TensorData::load( const char* filename, int* dims )
{
	return load( filename, ImageDataSpace(dims) );
}

//-----------------------------------------------------------------------------
bool TensorData::load( const char* filename, ImageDataSpace space )
{
	using namespace std;

	enum FileType {	UNKNOWN, RAW, MXVOL, NRRD };
	int filetype = UNKNOWN;

	// Guess filetype depending on format suffix (e.g. ".raw")
	string ext = getFileExt( filename );
	if( boost::iequals(	ext, ".raw" ) ) filetype = RAW;
	else
	if( boost::iequals(	ext, ".mxvol") ) filetype = MXVOL;
	else
	if( boost::iequals(	ext, ".nrrd" ) ) filetype = NRRD;
	else
		filetype = UNKNOWN;

	// Call corresponding load function
	bool success;
	switch( filetype )
	{
	default:
	case UNKNOWN:
	case RAW:
	case MXVOL:
		success = load_raw_or_mxvol( filename, space );
		break;

	case NRRD:
		success = load_nrrd( filename );
	}
	return success;
}

//-----------------------------------------------------------------------------
bool TensorData::load_nrrd( const char* filename )
{
	using namespace std;

#ifndef TENSORVIS_TEEM_SUPPORT
	cerr << "Error: TensorData compiled without NRRD support!" << endl;
	return false;
#else
	// Taken from the nrrd documentation 
	// http://teem.sourceforge.net/nrrd/lib.html

	char me[] = "TensorData::load_nrrd()";
	Nrrd* nin = nrrdNew();

	if( nrrdLoad( nin, filename, NULL ) )
	{
		char* err = biffGetDone(NRRD);
		fprintf( stderr, "%s: trouble reading \"%s\":\n%s", me, filename, err );
		free( err );
		return false;
	}
	
	/* say something about the array */
	printf("%s: \"%s\" is a %d-dimensional nrrd of type %d (%s)\n", 
		 me, filename, nin->dim, nin->type,
		 airEnumStr(nrrdType, nin->type));
	printf("%s: the array contains %d elements, each %d bytes in size\n",
		 me, (int)nrrdElementNumber(nin), (int)nrrdElementSize(nin));


	// Hardcoded handling of Thomas 28D statistics tensor.
	// We simply extract the symmetric mean tensor stored in floats 2 to 8.

	// Some sanity check
	if( nin->axis[0].size!=28 )
	{
		fprintf( stderr, "%s: file type mismatch \"%s\"", me, filename );
		nrrdNuke(nin);
		return false;
	}

	// Establish ImageDataSpace
	int dims[3];
	dims[0] = (int)nin->axis[1].size;
	dims[1] = (int)nin->axis[2].size;
	dims[2] = (int)nin->axis[3].size;

	double spacing[3] = { 1, 1, 1 };

	double origin[3];
	origin[0] = nin->spaceOrigin[0];
	origin[1] = nin->spaceOrigin[1];
	origin[2] = nin->spaceOrigin[2];

	ImageDataSpace space( dims, spacing, origin );

	if( (unsigned)(nrrdElementNumber(nin)/28) != space.getNumberOfPoints() )
	{
		fprintf( stderr, "%s: tensor field size mismatch \"%s\"", me, filename );
		nrrdNuke(nin);
		return false;
	}

	// Convert data

	// Allocate memory for m_data and set to zero
	allocateData( space );

	// We have the same fast-to-slow axes convention as nrrd, so we can
	// directly iterate over the elements.
	float* data = (float*)(nin->data);
	for( int i=0; i < space.getNumberOfPoints(); i++ )
	{
		// Convert 6D symmetric repr. to full 9D matrix
		// [ 0 1 2 ]      [ 0 1 2 ]
		// [   3 4 ] <->  [ 3 4 5 ]
		// [     5 ]      [ 6 7 8 ]

		const int ofs = 1; // start index of mean tensor

		m_data[ i*(9+3)+0 ] = data[ i*28+ofs+0 ];
		m_data[ i*(9+3)+1 ] = data[ i*28+ofs+1 ];
		m_data[ i*(9+3)+2 ] = data[ i*28+ofs+2 ];
		m_data[ i*(9+3)+4 ] = data[ i*28+ofs+3 ];
		m_data[ i*(9+3)+5 ] = data[ i*28+ofs+4 ];
		m_data[ i*(9+3)+8 ] = data[ i*28+ofs+5 ];

		m_data[ i*(9+3)+3 ] = data[ i*28+ofs+1 ];
		m_data[ i*(9+3)+6 ] = data[ i*28+ofs+2 ];
		m_data[ i*(9+3)+7 ] = data[ i*28+ofs+4 ];
	}

	// Read mask

	// Allocate and zero memory
	if( m_mask ) delete [] m_mask;
	m_mask = new float[ space.getNumberOfPoints() ];
	memset( (void*)m_mask, 0, space.getNumberOfPoints()*sizeof(float) );

	for( int i=0; i < space.getNumberOfPoints(); i++ )
	{
		m_mask[i] = data[i*28];
	}
	
	/* blow away both the Nrrd struct *and* the memory at nin->data
	 (nrrdNix() frees the struct but not the data,
	 nrrdEmpty() frees the data but not the struct) */
	nrrdNuke(nin);

	return true;
#endif
}

//-----------------------------------------------------------------------------
bool TensorData::load_raw_or_mxvol( const char* filename, ImageDataSpace space )
{
	using namespace std;

	// Open tensor data file
	ifstream f( filename, std::ios_base::binary );
	if( !f.is_open() )
	{
		cerr << "Error opening tensor data file \"" << filename << "\"!" <<endl;
		return false;
	}

	// Estimate file size
	f.seekg( 0, ios::end );
	size_t filesize = (size_t)f.tellg();
	f.seekg( 0, ios::beg );

	// Test for magic
	bool has_magic = false;
	char magic[sizeof(s_magic)];
	f.read( magic, sizeof(s_magic) );
	if( strcmp(magic,s_magic)==0 )
	{
		// Has magic, use dimensionality / spacing / origin given in file
		int dims_[3];
		f.read( (char*)dims_, sizeof(int)*3 );

		double spacing_[3];
		f.read( (char*)spacing_, sizeof(double)*3 );

		double origin_[3];
		f.read( (char*)origin_, sizeof(double)*3 );

		space = ImageDataSpace( dims_, spacing_, origin_ );

		has_magic = true;
	}
	else
	{
		// No magic number, rewind file position
		f.seekg( 0, ios::beg );
	}

	// Size of tensor data in number of floats
	int nfloats = space.getNumberOfPoints() * (9+3);

	// Sanity check on filesize and dimensions
	size_t estimated_filesize = nfloats*sizeof(float);
	if( has_magic )
		estimated_filesize += sizeof(s_magic) + 3*sizeof(int) + 6*sizeof(double);
	if( estimated_filesize != filesize )
	{
		cerr << "Error: Tensor data and reference volume size mismatch!" <<endl;
		f.close();
		return false;
	}

	// Allocate memory for tensor data
	float* tensors(NULL);
	try 
	{
		 tensors = new float[ nfloats ];
	} 
	catch( std::bad_alloc& ) 
	{
		cerr << "Error allocating \"" << nfloats*sizeof(float)/(1024*1024) 
			 << "MB of memory!" << endl;
		f.close();
		return false;
	}

	// Read data from disk	
	f.read( (char*)tensors, nfloats*sizeof(float) );
	if( f.fail() )
	{
		// Failed to read requested number of floats
		cerr << "Warning: Failed to read requested number of floats!\n";
	}
	f.close();
	
	// Set internal data pointer
	setTensors( tensors, space );
	return true;
}

//-----------------------------------------------------------------------------
//  save()
//-----------------------------------------------------------------------------
void TensorData::save( const char* filename ) const
{
	using namespace std;
	string ext = getFileExt( filename );
	if( boost::iequals(	ext, ".tensorfield" ) || boost::iequals( ext, ".tf" ) )
	{
		save_tensorfield( filename );
	}
	else // if( boost::iequals(	ext, ".mxvol" ) )
	{
		save_mxvol( filename );
	}
}

void TensorData::save_mxvol( const char* filename ) const
{
	using namespace std;

	int dims[3];
	double spacing[3];
	double origin[3];

	m_space.getDimensions( dims );
	m_space.getSpacing( spacing );
	m_space.getOrigin( origin );

	// Sanity check
	if( !m_data || (dims[0]+dims[1]+dims[2]<=0) )
	{
		cerr << "Warning: Tried to write an empty tensor file!\n";
		return;
	}

	// Size of tensor data in number of floats
	int nfloats = m_space.getNumberOfPoints() * (9+3);

	ofstream f( filename, std::ios_base::binary );

	// Write magic
	f.write( (char*)s_magic, sizeof(s_magic) );

	// Write dimensions / spacing / origin
	f.write( (char*)dims   , sizeof(int   )*3 );
	f.write( (char*)spacing, sizeof(double)*3 );
	f.write( (char*)origin , sizeof(double)*3 );

	// Write raw data
	f.write( (char*)m_data, nfloats*sizeof(float) );
	if( f.fail() )
	{
		// Failed to read requested number of floats
		cerr << "Warning: Failed to write tensor file " << filename << "!\n";
	}
	f.close();

	return;
}

void TensorData::save_tensorfield( const char* filename ) const
{
	using namespace std;

	int dims[3];
	double spacing[3];
	double origin[3];

	m_space.getDimensions( dims );
	m_space.getSpacing( spacing );
	m_space.getOrigin( origin );

	// Sanity check
	if( !m_data || (dims[0]+dims[1]+dims[2]<=0) )
	{
		cerr << "Warning: Tried to write an empty tensor file!\n";
		return;
	}

	// Threshold for image mask (if one is present)
	float threshold=23.0; // ?  FIXME: Hardcoded threshold!

	// Collect samples
	unsigned numPoints(0);
	if( hasMask() )
	{
		// Only save sample points according to image mask
		
		// Count number of points
		for( int z=0; z < dims[2]; z++ )
			for( int y=0; y < dims[1]; y++ )
				for( int x=0; x < dims[0]; x++ )
					if( getMask(x,y,z) > threshold ) numPoints++;
	}
	else
	{
		// Save full grid
		numPoints = m_space.getNumberOfPoints();
	}

	// Fill data arrays
	vector<double> tensorData;
	vector<double> pointData;
	tensorData.resize( 6*numPoints );
	pointData.resize( 3*numPoints );
	unsigned curPoint=0;
	for( int z=0; z < dims[2]; z++ )
		for( int y=0; y < dims[1]; y++ )
			for( int x=0; x < dims[0]; x++ )
			{
				if( hasMask() && (getMask(x,y,z) < threshold) )
					continue;

				// [ 0 1 2 ]      [ 0 1 2 ]
				// [   3 4 ] <->  [ 3 4 5 ]
				// [     5 ]      [ 6 7 8 ]
				float T[9];
				getTensor( x,y,z, T );
				tensorData[curPoint*6+0] = (double)T[0];
				tensorData[curPoint*6+1] = (double)T[1];
				tensorData[curPoint*6+2] = (double)T[2];
				tensorData[curPoint*6+3] = (double)T[4];
				tensorData[curPoint*6+4] = (double)T[5];
				tensorData[curPoint*6+5] = (double)T[8];

				double pt[3];
				m_space.getPoint( x,y,z, pt );
				pointData[curPoint*3+0] = pt[0];
				pointData[curPoint*3+1] = pt[1];
				pointData[curPoint*3+2] = pt[2];

				curPoint++;
			}

	// Write 'tensorfield' file (see implementation in meshspace project).
	// Code duplication from TensorfieldObject::saveTensorfield().
	ofstream of( filename, std::ios_base::binary );
	const char magic[] = "TENSORFIELD";	
	unsigned nrows = 6,
		     ncols = numPoints,
			 npts  = numPoints;

	of.write( magic, sizeof(magic) );
	of.write( (char*)&nrows, sizeof(unsigned) );
	of.write( (char*)&ncols, sizeof(unsigned) );
	of.write( (char*)&npts, sizeof(unsigned) );
	of.write( (char*)&tensorData[0], sizeof(double)*nrows*ncols );
	of.write( (char*)&pointData[0], sizeof(double)*3*npts );
	of.close();

	return;
}
