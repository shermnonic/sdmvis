#include "mattools.h"
#include <cmath>

namespace mattools {

////////////////////////////////////////////////////////////////////////////////
//	Matrix Wrapper
////////////////////////////////////////////////////////////////////////////////

std::size_t RawMatrix::getFileSize( const char* filename )
{
	MATTOOLS_IFSTREAM fs( filename, std::ios::binary );	
	if( !fs.is_open() )
	{
		std::cerr << "Error: Could not open \"" << filename << "\"!\n";
		return 0;
	}

	std::size_t s;
	MATTOOLS_GET_FILE_SIZE( s, fs );
	return s;
}

void RawMatrix::checkStreamIsOpen() const
{
	// Stream is only required for out-of-core treatment
	if( isInMemory() )
		return;

	// Re-open stream if required
	if( !m_fX.get() || !m_fX.get()->is_open() )
	{
		// FIXME: Re-opening of stream is not thread-safe because we could
		//        accidentially open a file twice.

		// Re-opening the stream requires to remove the const attribute.
		// We consider this save here since the internal state of our instance
		// does not change, the matrix content in memory and on disk stays the 
		// same. Only the stream which has been lost in between is re-opened.
		const_cast<boost::shared_ptr<MATTOOLS_IFSTREAM>&>(m_fX).reset();

		// Try to re-open stream
		MATTOOLS_IFSTREAM* fs = new MATTOOLS_IFSTREAM( m_filename.c_str(), std::ios::binary );
		if( !fs->is_open() )
		{
			std::cerr << "Error: Could not re-open stream \"" << m_filename
				      << "\"!\n";
		}

		const_cast<boost::shared_ptr<MATTOOLS_IFSTREAM>&>(m_fX) 
			                       = boost::shared_ptr<MATTOOLS_IFSTREAM>(fs);
	}
}

RawMatrix::RawMatrix()
	: m_numRows(0),
	  m_numCols(0),
	  m_inMemory(false)
{	
}

RawMatrix::~RawMatrix()
{
	clear();
}

void RawMatrix::clear()
{
	// Free filestream
	m_fX.reset(); // was: if( m_fX ) delete m_fX; m_fX = NULL;

	// Free memory
	m_X.reset(); // was: if( m_X ) delete [] m_X; m_X = NULL;

	// Reset variables
	m_numRows = 0;
	m_numCols = 0;
	m_inMemory = false;
	m_filename = std::string("");
}

bool RawMatrix::allocate( std::size_t m, std::size_t n )
{
	clear();

	// Try to allocate memory for complete matrix
	ValueType* X_in_mem = NULL;
	{		
		try {
			X_in_mem = new ValueType[m*n];
		}
		catch( std::bad_alloc& )
		{
			std::cerr << "Error: Could not allocate matrix of size "
				      << ((sizeof(ValueType)*m*n) / (1024*1024)) << "MB!\n";
			X_in_mem = NULL;
		}
	}

	// Treat matrix in-core
	if( X_in_mem )
	{
		m_X = boost::shared_array<ValueType>( X_in_mem ); 

		// Set members
		m_numRows = m;
		m_numCols = n;

		m_inMemory = true;
	}
	else
		m_inMemory = false;

	return m_inMemory;
}

bool RawMatrix::load( std::size_t m, std::size_t n, const char* filename, 
                      bool tryToLoadIntoMemory )
{
	clear();

	// Open new filestream	
	MATTOOLS_IFSTREAM* fs = new MATTOOLS_IFSTREAM( filename, std::ios::binary );
	m_fX = boost::shared_ptr<MATTOOLS_IFSTREAM>( fs );
	if( !m_fX.get()->is_open() )
	{
		std::cerr << "Error: Could not open \"" << filename << "\"!\n";
		return false;
	}

	// Try to allocate memory for complete matrix
	ValueType* X_in_mem = NULL;
	if( tryToLoadIntoMemory )
	{		
		try {
			X_in_mem = new ValueType[m*n];
		}
		catch( std::bad_alloc& )
		{		
			X_in_mem = NULL;
		}
	}

	if( X_in_mem )
	{
		// Read complete matrix from disk
		std::size_t Xsize = n*m*sizeof(ValueType);

		try {
			m_fX->read( (char*)X_in_mem, Xsize );
		}
		catch( std::exception& e )
		{
			std::cerr << "Error: Reading from disk failed!\n";
			std::cerr << e.what() << "\n";
			return false;
		}

		// Treat matrix in-core
		m_X = boost::shared_array<ValueType>( X_in_mem ); //was: m_X = X_in_mem;
		m_inMemory = true;
	}
	else
	{
		// Treat matrix out-of-core		
		m_inMemory = false;
	}

	// Set members
	m_numRows = m;
	m_numCols = n;
	m_filename = std::string(filename);

	// Everything went smooth
	return true;
}

void RawMatrix::get_row( std::size_t row, ValueType* buf ) const
{
	if( isInMemory() )
	{
		mattools::get_row( row,    m_X.get() , m_numRows,m_numCols, buf );
	}
	else
	{
		checkStreamIsOpen();
		mattools::get_row( row, *(m_fX.get()), m_numRows,m_numCols, buf );
	}
}

void RawMatrix::get_rows( std::vector<std::size_t> rows, ValueType* buf ) const
{	
	for( std::size_t i=0; i < rows.size(); i++ )
	{
		get_row( rows[i], &buf[i*m_numCols] );
	}
}

void RawMatrix::get_col( std::size_t col, ValueType* buf ) const
{
	if( isInMemory() )
	{
		mattools::get_col( col,    m_X.get() , m_numRows,m_numCols, buf );
	}
	else
	{
		checkStreamIsOpen();
		mattools::get_col( col, *(m_fX.get()), m_numRows,m_numCols, buf );
	}
}

void RawMatrix::get_cols( std::vector<std::size_t> cols, ValueType* buf ) const
{
	for( std::size_t i=0; i < cols.size(); i++ )
	{
		get_col( cols[i], &buf[i*m_numRows] );
	}
}

////////////////////////////////////////////////////////////////////////////////
//	Modify Functions
////////////////////////////////////////////////////////////////////////////////

void RawMatrix::applyColumnWeights( ValueType* w )
{
	ValueType* buf = new ValueType[ m_numRows ];
	for( std::size_t i=0; i < m_numCols; i++ )
	{
		get_col( i, buf );
		mattools::multiply( w[i], buf, m_numRows, buf );
		set_col( i, buf );
	}
	delete [] buf;
	// TODO: Optimized in-core variant
}

void RawMatrix::applyRowWeights( ValueType* w )
{
	ValueType* buf = new ValueType[ m_numCols ];
	for( std::size_t i=0; i < m_numRows; i++ )
	{
		get_row( i, buf );
		mattools::multiply( w[i], buf, m_numCols, buf );
		set_col( i, buf );
	}
	delete [] buf;
	// TODO: Optimized in-core variant
}

void RawMatrix::set_col( std::size_t col, ValueType* buf )
{
	// Currently, this function is only supported for in-memory matrices!
	if( !isInMemory() )	{
		std::cerr << "Error: Out-of-core matrices are treated read-only yet!\n";
		return;
	}
	
	// Assume row-major ordering
	std::size_t ofs = col;
	for( std::size_t i=0; (i < m_numRows); i++, ofs+=m_numCols )
		m_X.get()[ ofs ] = buf[i];
}

void RawMatrix::set_row( std::size_t row, ValueType* buf )
{
	// Currently, this function is only supported for in-memory matrices!
	if( !isInMemory() )	{
		std::cerr << "Error: Out-of-core matrices are treated read-only yet!\n";
		return;
	}
	
	// Assume row-major ordering
	// Note that copying of rows can also be realized via memcpy.
	std::size_t ofs = row*m_numCols;
	for( std::size_t i=0; (i < m_numCols); i++, ofs++ )
		m_X.get()[ ofs ] = buf[i];
}

bool RawMatrix::save( const char* filename )
{
	// Currently, this function is only supported for in-memory matrices!
	if( !isInMemory() )	{
		std::cerr << "Error: Out-of-core matrices are treated read-only yet!\n";
		return false;
	}

	// Open output file
	MATTOOLS_OFSTREAM f( filename, std::ios::binary );
	if( !f.is_open() )
	{
		std::cerr << "Error: Could not open \"" << filename << "\"!\n";
		return false;
	}

	// Dump in-memory matrix to disk
	std::size_t sizeInBytes = m_numRows*m_numCols*sizeof(ValueType);
	f.write( (char*)(m_X.get()), sizeInBytes );

	f.close();
	return true;
}

////////////////////////////////////////////////////////////////////////////////
//	Compute Functions
////////////////////////////////////////////////////////////////////////////////

void RawMatrix::makeZeroColumnMean()
{
	// Compute column mean row-wise since matrices are stored row-major and thus
	// this operation should be efficient in out-of-core mode.
	
	ValueType* rowbuf = new ValueType[ m_numCols ];

	for( std::size_t i=0; i < m_numRows; i++ )
	{
		// Get row i
		get_row( i, rowbuf );
		
		// Compute mean value
		ValueType mean_i = (ValueType)0.0;
		for( std::size_t j=0; j < m_numCols; j++ )
			mean_i += rowbuf[j];		
		mean_i /= m_numCols;

		// Subtract mean value
		for( std::size_t j=0; j < m_numCols; j++ )
			rowbuf[j] -= mean_i;

		// Write back row i
		set_row( i, rowbuf );
	}

	delete [] rowbuf;
}

void RawMatrix::computeColumnMean( ValueType* colbuf )
{
	// Compute column mean row-wise since matrices are stored row-major and thus
	// this operation should be efficient in out-of-core mode.
	
	ValueType* rowbuf = new ValueType[ m_numCols ];

	for( std::size_t i=0; i < m_numRows; i++ )
	{
		get_row( i, rowbuf );
		
		colbuf[i] = (ValueType)0.0;
		for( std::size_t j=0; j < m_numCols; j++ )
		{
			colbuf[i] += rowbuf[j];		
		}
		colbuf[i] /= m_numCols;
	}

	delete [] rowbuf;
}

void RawMatrix::computeColumnNorm( ValueType* rowbuf )
{
	// For simplicity of implementation we compute individual column norms,
	// although this is *not* efficient in out-of-core mode.

	ValueType* colbuf = new ValueType[ m_numRows ];

	for( std::size_t i=0; i < m_numCols; i++ )
	{
		get_col( i, colbuf );
		rowbuf[i] = norm( colbuf, m_numRows );
	}	

	delete [] colbuf;
}

void RawMatrix::multiply( ValueType* x, ValueType* res )
{
	ValueType* rowbuf = new ValueType[ m_numCols ];

	for( std::size_t i=0; i < m_numRows; i++ )
	{	
		get_row( i, rowbuf );

		ValueType val = (ValueType)0.0;
		for( std::size_t j=0; j < m_numCols; j++ )
		{
			val += rowbuf[j] * x[i];
		}
		
		res[i] = val;
	}

	delete [] rowbuf;
}

////////////////////////////////////////////////////////////////////////////////
//	Core Functions
////////////////////////////////////////////////////////////////////////////////

void mattools_debug_info()
{
	using namespace std;
#ifdef MATTOOLS_USE_64BIT_STREAMS
	cout << "mattools built with 64bit large file support" << endl;
#else
	cout << "mattools *NOT* built with 64bit large file support" << endl;
#endif
	cout << "sizeof(std::size_t)         =" << sizeof(std::size_t)          << endl
		 << "sizeof(int)            =" << sizeof(int)             << endl
		 << "sizeof(std::streamsize)=" << sizeof(std::streamsize) << endl;
}

void get_row( std::size_t row, ValueType* M, std::size_t n, std::size_t m, ValueType* buf )
{
#if 1
	memcpy( buf, &M[ (std::size_t)row*m ], m*sizeof(ValueType) );
#else
	std::size_t ofs = row*m;
	for( std::size_t j=0; j < m; ++j )
		buf[j] = M[ ofs + j ];
#endif
}

void get_col( std::size_t col, ValueType* M, std::size_t n, std::size_t m, ValueType* buf )
{	
	for( std::size_t i=0; i < n; ++i )
		buf[i] = M[ (std::size_t) col + i*m ];
}

void get_col( std::size_t col, MATTOOLS_IFSTREAM& M, std::size_t n, std::size_t m, ValueType* buf )
{
	// could be more performant to retrieve several columns	
	for( std::size_t i=0; i < n; ++i ) {
		//M.seekg(( col + i*m )*sizeof(ValueType));
		MATTOOLS_FSEEK( M, ( col + i*m )*sizeof(ValueType) )
		M.read( (char*)(&buf[i]), sizeof(ValueType) );
	}
}

void get_row( std::size_t row, MATTOOLS_IFSTREAM& M, std::size_t n, std::size_t m, ValueType* buf )
{
	std::size_t rowsize = m * sizeof(ValueType);
	//M.seekg( row * rowsize );
	MATTOOLS_FSEEK( M, row * rowsize );
	M.read( (char*)(&buf[0]), rowsize );
}


ValueType multiply( ValueType* a, ValueType* b, std::size_t n )
{
	ValueType val=0;
	for( std::size_t i=0; i < n; ++i )
		val += a[i] * b[i];
	return val;
}

void multiply( ValueType* a, ValueType* b, std::size_t n, ValueType* result )
{
	for( std::size_t i=0; i < n; ++i )
		result[i] = a[i] * b[i];
}

void multiply( ValueType a, ValueType* b, std::size_t n, ValueType* result )
{
	for( std::size_t i=0; i < n; ++i )
		result[i] = a * b[i];
}


void multiply_and_add( ValueType* a, ValueType* b, std::size_t n, ValueType* result )
{
	for( std::size_t i=0; i < n; ++i )
		result[i] += a[i] * b[i];
}

void multiply_and_add( ValueType a, ValueType* b, std::size_t n, ValueType* result )
{
	for( std::size_t i=0; i < n; ++i )
		result[i] += a * b[i];
}

void subtract( ValueType* a, ValueType* b, std::size_t n, ValueType* result )
{
	for( std::size_t i=0; i < n; ++i )
		result[i] = a[i] - b[i];
}

void add( ValueType* a, ValueType* b, std::size_t n, ValueType* result )
{
	for( std::size_t i=0; i < n; ++i )
		result[i] = a[i] + b[i];
}

void add_outer_prod( std::size_t n, ValueType* x, ValueType* y, ValueType* S )
{
	for( std::size_t i=0; i < n; ++i )
	{
		for( std::size_t j=0; j < n; ++j )
		{
			MATTOOLS_DEBUG_OUT( x[i] << "*" << y[i] << "  " );
			S[i*n+j] += x[i]*y[j];
		}
		MATTOOLS_DEBUG_OUT( std::endl );
	}
}

void print_matrix( ValueType* M, std::size_t n, std::size_t m )
{
	for( std::size_t i=0; i < n; ++i )
	{
		for( std::size_t j=0; j < n; ++j )
		{
			std::cout << M[i*n+j] << "  ";
		}
		std::cout << std::endl;
	}
}


ValueType norm( ValueType* a, std::size_t n )
{
	ValueType sum = (ValueType)0.0;
	for( std::size_t i=0; i < n; i++ )
		sum += a[i]*a[i];
	return std::sqrt( sum );
}

} // namespace mattools
