// mattools config and common functions - version from March 2013
// Max Hermann, October 27, 2010
#ifndef MATTOOLS_H
#define MATTOOLS_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>

// Uncomment USE_64_BIT_STREAMS to enable the use of boost::iostreams which
// provide large file support also for 32Bit targets.
#define MATTOOLS_USE_64BIT_STREAMS

#define MATTOOLS_DEBUG_OUT(m) 
//#define MATTOOLS_DEBUG_OUT(m) std::cout << m 

////////////////////////////////////////////////////////////////////////////////
//	Macros
////////////////////////////////////////////////////////////////////////////////

// File size macro
// Parameter size should be a std::size_t variable which should be sufficient also
// for large file support on 64bit systems.
// REMARK: boost::iostreams doesn't provide default constructors!
#ifdef MATTOOLS_USE_64BIT_STREAMS
	#include <boost/iostreams/device/file.hpp>
	#include <boost/iostreams/positioning.hpp>
	using boost::iostreams::file_sink;
	using boost::iostreams::file_source;
	using boost::iostreams::position_to_offset;
	  #define MATTOOLS_GET_FILE_SIZE( size, fs ) \
		size = (long long)position_to_offset(fs.seek( 0, BOOST_IOS::end ));  \
		fs.seek( 0, BOOST_IOS::beg );

	  #define MATTOOLS_IFSTREAM file_source
	  #define MATTOOLS_OFSTREAM file_sink
	  #define MATTOOLS_FSEEK( fs, ofs ) fs.seek( ofs, BOOST_IOS::beg );
#else
	  #define MATTOOLS_GET_FILE_SIZE( size, fs ) \
		fs.seekg( 0, std::ios::end );      \
		size = (std::size_t)fs.tellg();    \
		fs.seekg( 0, std::ios::beg );
	  #define MATTOOLS_IFSTREAM std::ifstream
	  #define MATTOOLS_OFSTREAM std::ofstream
	  #define MATTOOLS_FSEEK( fs, ofs ) fs.seekg( ofs );
#endif


////////////////////////////////////////////////////////////////////////////////
//	Namespace mattools
////////////////////////////////////////////////////////////////////////////////

namespace mattools {

////////////////////////////////////////////////////////////////////////////////
//	Configuration
////////////////////////////////////////////////////////////////////////////////

/// Fixed value type
typedef float ValueType;

// Epsilon is not used yet internally, so I commented it out
//const double eps = 1e-19;

////////////////////////////////////////////////////////////////////////////////
//	Matrix Wrapper
////////////////////////////////////////////////////////////////////////////////

/// Facade for \a mattools allowing a transparent use of in- and out-of-core 
/// matrices. Functionality is added here as it is required.
/// Note that out-of-core operations are stupidly simple realized by explicitly
/// reading the requested row or column from disk into a memory buffer.
/// Modifying operations like \a set_col() and \a save() are only supported
/// for in-memory matrices yet!
class RawMatrix
{
public:
	static std::size_t getFileSize( const char* filename );

public:
	RawMatrix();
	~RawMatrix();

/*
	/// Open a matrix file read only
	RawMatrix( inFilename, ReadOnly );
	/// Open a matrix file read/write, i.e. changes are written to same file
	RawMatrix( inoutFilename, ReadWrite );
	/// Open a matrix file and write changes to different file
	RawMatrix( inFilename, outFilename );
*/

	void clear();

	bool load( std::size_t m, std::size_t n, const char* filename, 
		       bool tryToLoadIntoMemory=true );

	bool allocate( std::size_t m, std::size_t n );

	typedef std::vector<std::size_t> IndexVector;

	void get_row( std::size_t row, ValueType* buf )        const;
	void get_rows( IndexVector rows, ValueType* buf ) const;

	void get_col( std::size_t col, ValueType* buf )        const;
	void get_cols( IndexVector cols, ValueType* buf ) const;

	bool isInMemory() const { return m_inMemory; /* && m_X;*/ }

	std::string getFilename() const { return m_filename; }

	std::size_t getNumRows() const { return m_numRows; }
	std::size_t getNumCols() const { return m_numCols; }

	/// Compute mean over all columns, result is a column vector
	void computeColumnMean( ValueType* colbuf );
	/// Compute norm of each column, result is a row vector
	void computeColumnNorm( ValueType* rowbuf );

	void makeZeroColumnMean();

	/// Multiply matrix with given (column) vector (from right)
	/// @param[in] x vector to multiply with, of size of number of columns of A
	/// @param[out] res solution A*x, of size of number of rows of A
	void multiply( ValueType* x, ValueType* res );

	//@{ Modify operations currently only allowed for in-memory matrices!
	void set_row( std::size_t row, ValueType* buf );
	void set_col( std::size_t col, ValueType* buf );
	void applyColumnWeights( ValueType* w );
	void applyRowWeights   ( ValueType* w );
	bool save( const char* filename );
	//@}

protected:
	void checkStreamIsOpen() const;

private:
	boost::shared_ptr  <MATTOOLS_IFSTREAM>  m_fX;
	boost::shared_array<ValueType> m_X;
	std::size_t m_numRows, m_numCols;
	bool m_inMemory;
	std::string m_filename;
};


////////////////////////////////////////////////////////////////////////////////
//	Core
////////////////////////////////////////////////////////////////////////////////

/// print debug information to standard output
void mattools_debug_info();

/// copy row from memory (untested?);
void get_row( std::size_t row, ValueType* M, std::size_t n, std::size_t m, ValueType* buf );

/// copy column from memory
void get_col( std::size_t col, ValueType* M, std::size_t n, std::size_t m, ValueType* buf );

/// read column from disk (slow);
void get_col( std::size_t col, MATTOOLS_IFSTREAM& M, std::size_t n, std::size_t m, ValueType* buf );

/// read row from disk (fast);
void get_row( std::size_t row, MATTOOLS_IFSTREAM& M, std::size_t n, std::size_t m, ValueType* buf );

/// scalar product
ValueType multiply( ValueType* a, ValueType* b, std::size_t n );

/// component wise vector multiplication
void multiply( ValueType* a, ValueType* b, std::size_t n, ValueType* result );

/// scalar/vector multiplication
void multiply( ValueType a, ValueType* b, std::size_t n, ValueType* result );

/// subtraction
void subtract( ValueType* a, ValueType* b, std::size_t n, ValueType* result );

/// addition
void add( ValueType* a, ValueType* b, std::size_t n, ValueType* result );

/// addition of outer products
void add_outer_prod( std::size_t n, ValueType* x, ValueType* y, ValueType* S );

/// print matrix to standard output
void print_matrix( ValueType* M, std::size_t n, std::size_t m );


// New functions 2013:

/// L2 vector norm
ValueType norm( ValueType* a, std::size_t n );

/// scalar multiplication where result is added to the given buffer
void multiply_and_add( ValueType a, ValueType* b, std::size_t n, ValueType* result );

/// component wise vector multiplication where result is added to given buffer
void multiply_and_add( ValueType* a, ValueType* b, std::size_t n, ValueType* result );

} // namespace mattools

#endif // MATTOOLS_H
