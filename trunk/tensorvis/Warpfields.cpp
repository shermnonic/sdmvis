#include "Warpfields.h"

#include <iostream>
#include <stdexcept>

//-----------------------------------------------------------------------------
// File IO helper macros / functions, copied from matttools.h
//-----------------------------------------------------------------------------

// File IO macros
#define GET_FILE_SIZE( size, fs ) \
				fs.seekg( 0, ios::end );      \
				size = (size_t)fs.tellg();    \
				fs.seekg( 0, ios::beg );
#define FSEEK( fs, ofs ) fs.seekg( ofs );
#define IFSTREAM std::ifstream
#define OFSTREAM std::ofstream

typedef Warpfields::ValueType ValueType;

/// read matrix row from disk (fast)
void get_row( size_t row, IFSTREAM& M, size_t n, size_t m, ValueType** buf )
{
	size_t rowsize = m * sizeof(ValueType);
	//M.seekg( row * rowsize );
	FSEEK( M, row * rowsize );
	M.read( (char*)(&(*buf)[0]), rowsize );
}

//-----------------------------------------------------------------------------
//  reset()
//-----------------------------------------------------------------------------
void Warpfields::reset()
{
	m_fs.close();
	m_curField.free();
	m_dim[0] = m_dim[1] = 0;
	m_filename = "";
}

//-----------------------------------------------------------------------------
//  setFile()
//-----------------------------------------------------------------------------
bool Warpfields::setFile( const char* filename, int rows, int cols )
{
	using namespace std;
	
	// set filename
	m_filename = string(filename);
	
	// try to open file
	try 	
	{
		prepareFileAccess();
	} 
	catch (exception& e )
	{
		// failed to open file
		cerr << "Error: " << e.what() << endl;
		return false;
	}
	
	// check filesize
	size_t size;	
	GET_FILE_SIZE( size, m_fs );
	
	// estimate #rows automatically ?
	if( rows==-1 )
		rows = ((int)size / cols) / sizeof(ValueType);
	
	// number of voxels, given that each column stores a 3-vectorfield
	size_t numVoxels = rows / 3;
	if( (rows % 3) != 0 )
	{
		cerr << "Error: Mismatching matrix size!\n";
		return false;
	}
	
	// assign matrix dimensionality
	setMatrixDim( rows, cols );
	
	// allocate cache
	m_curField.allocate( cols );
	
	return true;
}

//-----------------------------------------------------------------------------
//  setMatrixDim()
//-----------------------------------------------------------------------------
void Warpfields::setMatrixDim( int rows, int cols )
{
	m_dim[0] = rows;
	m_dim[1] = cols;
}

//-----------------------------------------------------------------------------
//  prepareAccess()
//-----------------------------------------------------------------------------
void Warpfields::prepareFileAccess()
{	
	if( !m_fs.is_open() )
		m_fs.open( m_filename.data(), std::ios_base::binary );

	if( !m_fs.is_open() )
		throw std::runtime_error("Cannot access matrix file!");
}

//-----------------------------------------------------------------------------
//  readVoxelDisplacements()
//-----------------------------------------------------------------------------
void Warpfields::readVoxelDisplacements( int voxelIdx )
{
	prepareFileAccess();
	
	get_row( voxelIdx*3  , m_fs, m_dim[0],m_dim[1], &(m_curField.data_x) );
	get_row( voxelIdx*3+1, m_fs, m_dim[0],m_dim[1], &(m_curField.data_y) );
	get_row( voxelIdx*3+2, m_fs, m_dim[0],m_dim[1], &(m_curField.data_z) );
}
