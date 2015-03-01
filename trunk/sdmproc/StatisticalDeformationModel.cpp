#include "StatisticalDeformationModel.h"
#include <iostream>
#include <fstream>
#include <boost/format.hpp>

StatisticalDeformationModel::StatisticalDeformationModel()
	: m_config_opts("SDM config")
{
	setupConfig();
}

///////////////////////////////////////////////////////////////////////////////
//	Set
///////////////////////////////////////////////////////////////////////////////

void StatisticalDeformationModel::
  setWarpfields( mattools::RawMatrix X )
{
	m_warpfields = X;
	m_status.hasWarpfields = true;
}

void StatisticalDeformationModel::
  setEigenmodes( mattools::RawMatrix U )
{
	m_eigenmodes = U;
	m_status.hasEigenmodes = true;
}


///////////////////////////////////////////////////////////////////////////////
//	Load
///////////////////////////////////////////////////////////////////////////////

void StatisticalDeformationModel::
 clear()
{
	clearPCA();
	clearData();
	// Reset status
	m_numSamples = 0;
	m_status = Status();	
}

void StatisticalDeformationModel::
  clearData()
{
	m_warpfields.clear(); m_status.hasWarpfields = false;
	m_eigenmodes.clear(); m_status.hasEigenmodes = false;
}

void StatisticalDeformationModel::
  clearPCA()
{
	m_scatter.clear();
	m_V      .clear();
	m_lambda .clear();

	// BUG: Assigning as follows leads to a crash!
	//	m_scatter = Matrix();
	//	m_V       = Matrix();
	//	m_lambda  = Vector();

	m_status.hasScatter = false;
	m_status.hasV       = false;
	m_status.hasLambda	= false;
}

bool StatisticalDeformationModel::
  loadRawVectorfieldsFromMultipleFiles( std::vector<std::string> filenames,
                                       mattools::RawMatrix& out )
{
	unsigned numFields = filenames.size();

	// Number of rows, i.e. size of single column representing a vectorfield
	unsigned numRows = 
		mattools::RawMatrix::getFileSize( filenames.at(0).c_str() )
	                       / sizeof(mattools::ValueType);

	// Sanity check
	if( numRows % 3 != 0 )
	{
		std::cerr << "Error: Matrix size mismatch !\n";
		return false;
	}

#if 1
	// Sanity check all file sizes
	for( unsigned i=0; i < numFields; i++ )
	{
		unsigned numRows_i = 
			mattools::RawMatrix::getFileSize( filenames.at(i).c_str() )
	                       / sizeof(mattools::ValueType);
		if( numRows_i != numRows )
		{
			std::cerr << "Error: Matrix size mismatch in \"" 
				      << filenames.at(i) << "\"!\n";
			return false;
		}	
	}
#endif

	// Allocate data matrix
	if( !out.allocate( numRows, numFields ) )
	{
		std::cerr << "Error: Not enough memory for data matrix!\n";
		return false;
	}

	mattools::ValueType* buffer = new mattools::ValueType[numRows];
	for( unsigned i=0; i < numFields; i++ )
	{
		// Read vectorfield from file i
		MATTOOLS_IFSTREAM f( filenames.at(i).c_str(), std::ios::binary );
		assert( f.is_open() );
		f.read( (char*)buffer, numRows*sizeof(mattools::ValueType) );
		f.close();

		// Store in global matrix
		out.set_col( i, buffer );
	}

	delete [] buffer;
	return true;
}

bool StatisticalDeformationModel::
  loadMean()
{
	std::string filename = getConfig().filenameMeanwarp;

	// Sanity
	if( filename.empty() )
	{
		std::cerr << "Error: Trying to load mean warp but no filename was "
			"specified in config!\n";
		return false;
	}

	// Mean warp file has to be uncompressed raw of identical size as warpfields
	if( mattools::RawMatrix::getFileSize( filename.c_str() ) !=
		sizeof(mattools::ValueType) * getFieldSize() )
	{
		std::cerr << "Error: Filesize mismatch of " << filename << "!\n";
		return false;
	}

	// Open file
	MATTOOLS_IFSTREAM f( filename.c_str(), std::ios::binary );
	if( !f.is_open() )
	{
		std::cerr << "Error: Could not open " << filename << "!\n";
		return false;
	}
	
	// Read raw data
	unsigned n = getFieldSize() * sizeof(mattools::ValueType);
	std::vector< mattools::ValueType > buffer( getFieldSize() );
	f.read( (char*)&buffer[0], n );
#ifndef MATTOOLS_USE_64BIT_STREAMS // Boost iostreams does not support .gcount()
	if( !f )  // failbit ? 
	{
		std::cerr << "Error: Only " << f.gcount() << " Bytes of expected "
			<< n << " Bytes could be read from " << filename << "!\n";
		f.close();
		return false;
	}
#endif
	f.close();

	// Success, set member variable
	m_mean = buffer;
	return true;
}

bool StatisticalDeformationModel::
 loadRawVectorfields( unsigned numFields, const char* filename,
			         mattools::RawMatrix& mat )
{	
	// Number of elements in matrix
	unsigned numElements = mattools::RawMatrix::getFileSize( filename )
	                       / sizeof(mattools::ValueType);
	// Number of rows, i.e. size of single column representing a vectorfield
	unsigned numRows = numElements / numFields;
	
	// Sanity check
	if( numElements % numFields != 0 || numRows % 3 != 0 )
	{
		std::cerr << "Error: Matrix size mismatch in \"" << filename << "\"!\n";
		return false;
	}
	
	// Load matrix	
	return mat.load( numRows, numFields, filename );
}

void StatisticalDeformationModel::
  makeFilepath( std::vector<std::string> names,
	std::string basepath, std::string ext, 
	std::vector<std::string>& filenames )
{
	filenames.resize( names.size() );
	for( unsigned i=0; i < names.size(); i++ )
		filenames.at(i) = basepath + names.at(i) + ext;
}

bool StatisticalDeformationModel::
 loadWarpfields( std::vector<std::string> filenames )
{
	m_numSamples = filenames.size();
	bool ok = loadRawVectorfieldsFromMultipleFiles( filenames, m_warpfields );
	m_status.hasWarpfields = ok;
	return ok;
}

bool StatisticalDeformationModel::
 loadWarpfields( unsigned numFields, const char* filename )
{
	m_numSamples = numFields;
	bool ok = loadRawVectorfields( numFields, filename, m_warpfields );
	m_status.hasWarpfields = ok;
	return ok;
}

bool StatisticalDeformationModel::
 loadEigenmodes( unsigned numFields, const char* filename )
{
	m_numSamples = numFields;
	bool ok = loadRawVectorfields( numFields, filename, m_eigenmodes );
	m_status.hasEigenmodes = ok;
	return ok;
}

bool StatisticalDeformationModel::
  loadPCA(  const char* filenameScatter,
			const char* filenameV,
			const char* filenameLambda )
{
	clearPCA();
	
	unsigned n = m_numSamples; //was: m_eigenmodes.getNumCols();
	
	Matrix S( n,n );	
	if( filenameScatter &&
		rednum::load_matrix<float,Matrix,ValueType>( S, filenameScatter ) )
	{
		m_scatter = S;
		m_status.hasScatter = true;
		m_fileScatter = filenameScatter;
	}
	
	Matrix V( n,n );
	if( filenameV &&
		rednum::load_matrix<float,Matrix,ValueType>( V, filenameV ) )
	{
		m_V = V;
		m_status.hasV = true;
		m_fileV = filenameV;
	}
	
	Vector lambda( n );
	if( filenameLambda && 
		rednum::load_vector<float,Vector,ValueType,Matrix>(  lambda, 
			filenameLambda ) )
	{
		m_lambda = lambda;
		m_status.hasLambda = true;
		m_fileLambda = filenameLambda;
	}

	return m_status.hasLambda && m_status.hasV && m_status.hasScatter;
}

bool StatisticalDeformationModel::
  saveSDM( std::string basepath )
{
	bool ok = true;
	
	if( !m_config.filenameWarpfields.empty() )
		ok &= m_warpfields.save(
			(basepath + m_config.filenameWarpfields).c_str()
		);

	if( !m_config.filenameEigenmodes.empty() )
		ok &= m_eigenmodes.save(
			(basepath + m_config.filenameEigenmodes).c_str()
		);

	if( !m_config.filenameV.empty() )
		rednum::save_matrix<mattools::ValueType,Matrix>( 
			m_V, 
			(basepath + m_config.filenameV).c_str()
		);

	if( !m_config.filenameScatter.empty() )
		rednum::save_matrix<mattools::ValueType,Matrix>(
			m_scatter,
			(basepath + m_config.filenameScatter).c_str() 
		);

	if( !m_config.filenameLambda.empty() )
		rednum::save_vector<mattools::ValueType,Vector>(
			m_lambda,
			(basepath + m_config.filenameLambda).c_str() 
		);

	if( !m_config.filenameMeanwarp.empty() )
	{
		if( !m_mean.empty() )
		{
			saveVector( 
				(basepath + m_config.filenameMeanwarp).c_str(), 
				&m_mean[0],
				m_warpfields.getNumRows() 
			);
		}
		else
		{
			std::cout << "Warning: Can not save empty mean warp!\n";
		}
	}

	return ok;
}

///////////////////////////////////////////////////////////////////////////////
//	Get
///////////////////////////////////////////////////////////////////////////////

std::string StatisticalDeformationModel::
 getName( unsigned idx ) const
{
	if( m_names.size() <= idx || m_names.empty() )
	{
		// No name present, return number as string
		return (boost::format("Dataset%02d") % idx).str();
	}
	return m_names.at(idx);
}

void StatisticalDeformationModel::
 getWarpfield( unsigned idx, mattools::ValueType* rawdata ) const
{
	m_warpfields.get_col( idx, rawdata );
}
	
void StatisticalDeformationModel::
 getEigenmode( unsigned idx, mattools::ValueType* rawdata ) const
{
	m_eigenmodes.get_col( idx, rawdata );
}

void StatisticalDeformationModel::
 appendRowIndices( double x, double y, double z, IndexVector& idx ) const
{
	int ix = (int)std::floor( x * m_header.resolution[0] ),
		iy = (int)std::floor( y * m_header.resolution[1] ),
		iz = (int)std::floor( z * m_header.resolution[2] );

	size_t ofs = iz * m_header.resolution[1]*m_header.resolution[0] 
	           + iy * m_header.resolution[0] 
			   + ix;

	idx.push_back( 3*ofs+0 );
	idx.push_back( 3*ofs+1 );
	idx.push_back( 3*ofs+2 );
}

void StatisticalDeformationModel::
 getEigenmodeRows( IndexVector rows, mattools::ValueType* rawdata ) const
{
	m_eigenmodes.get_rows( rows, rawdata );
}

///////////////////////////////////////////////////////////////////////////////
//	Helper functions
///////////////////////////////////////////////////////////////////////////////

void StatisticalDeformationModel::
  multiply( mattools::RawMatrix& X, Matrix& V, mattools::RawMatrix& XV )
{
	typedef mattools::ValueType FloatType;

	// Matrix dimensions must match
	assert( X.getNumCols() == V.size1() );

	XV.allocate( X.getNumRows(), V.size2() );

	FloatType* rowbuf = new FloatType[ X.getNumCols() ];
	FloatType* resbuf = new FloatType[ V.size2() ];

	for( unsigned i=0; i < X.getNumRows(); i++ )
	{	
		X.get_row( i, rowbuf );		

		for( unsigned resCol=0; resCol < V.size2(); resCol++ )
		{
			// Multiply X row i with V column resCol
			double val = 0.0;
			for( unsigned j=0; j < X.getNumCols(); j++ )
			{
				// Intermediate calculations in double precision
				val += (double)rowbuf[j] * (double)V(j,resCol);
			}

			resbuf[resCol] = (FloatType)val;
		}

		// Store row i of product
		XV.set_row( i, resbuf );
	}

	delete [] rowbuf;
	delete [] resbuf;
}

void StatisticalDeformationModel::
  saveVector( const char* filename, mattools::ValueType* buffer, unsigned N )
{
	MATTOOLS_OFSTREAM f(filename,std::ios::binary);
	assert(f.is_open());
	f.write( (char*)buffer, N*sizeof(mattools::ValueType) );
	f.close();
}

///////////////////////////////////////////////////////////////////////////////
//	Compute
///////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// synthesizeField()
//----------------------------------------------------------------------------
void StatisticalDeformationModel::
 synthesizeField( Vector coeffs, mattools::ValueType* result, bool considerMean ) const
{
	unsigned n = std::min( coeffs.size(), m_eigenmodes.getNumCols() );
	
	double normalization = 1.0 / sqrt((double)getNumSamples()-1.0);

	// Linear combination of eigenmodes
	std::size_t fieldSize = getFieldSize();
	mattools::ValueType* buf = new mattools::ValueType[ fieldSize ];
	for( unsigned i=0; i < n; i++ )
	{
		getEigenmode( i, buf );

		mattools::ValueType ci = (mattools::ValueType)(normalization * coeffs(i));

		if( i==0 )
			// overwrite result entries with first term
			mattools::multiply        ( ci, buf, fieldSize, result );
		else
			// add additional terms
			mattools::multiply_and_add( ci, buf, fieldSize, result );
	}
	delete [] buf;

	// Add mean warp (if valid)
	if( considerMean )
	{
		const ValueType* mean = getMeanPtr();
		if( mean )
			mattools::add( result, const_cast<ValueType*>(mean), fieldSize, result ); 
	}
}

//----------------------------------------------------------------------------
// reconstructField()
//----------------------------------------------------------------------------
void StatisticalDeformationModel::
 reconstructField( unsigned idx, int numModes, 
                   mattools::ValueType* result ) const
{
	using namespace boost::numeric::ublas;

	// Only use first n modes in reconstruction
	unsigned n = std::min( (int)m_V.size2(), numModes );

	double normalization = sqrt((double)n-1.0);

	// Get idx-th column of V' (i.e. idx-th row of V) 
	Vector vi( n );
	for( unsigned i=0; i < n; i++ )
		// Note that although modes are computed as E=X*V the synthesizeField()
		// method normalizes coefficients (correctly) with 1/sqrt(n-1) such
		// that coefficients are Gaussian normal distributed (and can be 
		// interpreted in times of standard deviation sigma). Therefore we have
		// to manually rescale the V matrix with sqrt(n-1) to reconstruct X.
		// Note further, that synthesizeField() will add in the mean again!.
		vi(i) = normalization * m_V(idx,i);

	// Linear combination of eigenmodes (+ mean if available)
	synthesizeField( vi, result );
}

//----------------------------------------------------------------------------
// computePCA()
//----------------------------------------------------------------------------
void StatisticalDeformationModel::
  computePCA()
{
	// Warpfields must be given
	assert( m_status.hasWarpfields );

	// Note: We assume that warpfields are already centered and have zero mean.
	//       Else we could compute the mean somewhat like this:
	//
	//	// Compute mean	
	//	ValueType* mu = new ValueType[ m_warpfields.getNumRows() ];
	//	m_warpfields.computeColumnMean( mu );
	//
	//	delete [] mu;  // do not forget to free memory

	// (1) Compute scatter matrix 
	computeScatterMatrix();

	// (2) Perform PCA
	Matrix score;
	rednum::compute_pcacov<Matrix,Vector,rednum::ValueType>
		( m_scatter, m_V, score, m_lambda );

	// Update status
	m_status.hasScatter = true;
	m_status.hasLambda  = true;
	m_status.hasV       = true;
}

//----------------------------------------------------------------------------
// reconstructEigenmodes()
//----------------------------------------------------------------------------
void StatisticalDeformationModel::
  reconstructEigenmodes()
{
	// Reconstruct eigenwarps U = X*V
	multiply( m_warpfields, m_V, m_eigenmodes );

	// Update status
	m_status.hasEigenmodes = true;
}

//----------------------------------------------------------------------------
// computeScatterMatrix()
//----------------------------------------------------------------------------
void StatisticalDeformationModel::
  computeScatterMatrix()
{
	typedef mattools::ValueType FloatType;
	typedef rednum::ValueType  DoubleType;

	// We compute the smaller n x n scatter matrix where n = #indivduals
	unsigned n = m_warpfields.getNumCols();
	FloatType* C = new FloatType[n*n];
	memset( (void*)C, 0, n*n*sizeof(FloatType) ); // initalize with zero matrix
	
	// Sum of outer products
	FloatType* x_i = new FloatType[n];  // temporary buffer for single row
	for( unsigned i=0; i <  m_warpfields.getNumRows(); i++ )
	{
		// C += x_i*x_i'
		m_warpfields.get_row( i, x_i );
		mattools::add_outer_prod( n, x_i, x_i, C );
	}

	// Set scatter matrix
	m_scatter.resize( n, n );
	rednum::matrix_from_rawbuffer<FloatType,Matrix,DoubleType>( m_scatter, C );

	// Free memory
	delete [] C;
	delete [] x_i;
}

//----------------------------------------------------------------------------
// computeMean()
//----------------------------------------------------------------------------
void StatisticalDeformationModel::
  computeMean()
{
	m_mean.resize(  m_warpfields.getNumRows() );	
	m_warpfields.computeColumnMean( &m_mean[0] );
}

//----------------------------------------------------------------------------
// subtractMean()
//----------------------------------------------------------------------------
void StatisticalDeformationModel::
  subtractMean()
{
	assert( m_status.hasWarpfields );

	m_warpfields.makeZeroColumnMean();	
}

///////////////////////////////////////////////////////////////////////////////
//	Config
///////////////////////////////////////////////////////////////////////////////

bool StatisticalDeformationModel::
 saveIni( const char* filename )
{
	using namespace std;
	ofstream f( filename );
	if( !f.is_open() )
	{
		cerr << "Error: Could not open \"" << filename << "\" for writing!\n";
		return false;
	}
	
	print( f );
	
	f.close();
	return true;
}

bool StatisticalDeformationModel::
 loadIni( const char* filename, int opts )
{
	namespace po = boost::program_options;
	using namespace std;

	// Clear variable map
	m_config_vmap = po::variables_map();

	// Load config file
	try {
		po::store( po::parse_config_file<char>( filename, m_config_opts ), 
		           m_config_vmap );
		po::notify( m_config_vmap );
	} 
	catch( const std::exception& e )
	{
		// Exception handling (boost::program_options already provides nice
		// error messages, so we only print e.what())
		cerr << e.what() << endl;
		return false;
	}

	if( opts & IniApplyConfig )
		// Apply new config
		return applyConfig();

	return true;
}

bool StatisticalDeformationModel::
  loadWarpfields()
{
	using namespace std;

	setResolution( m_config.resx, m_config.resy, m_config.resz );
	setSpacing( m_config.spacingx, m_config.spacingy, m_config.spacingz );
	setOffset( m_config.offsetx, m_config.offsety, m_config.offsetz );

	// Load warpfields
	if( m_config.loadWarpfieldsFromNames )
	{
		std::cout << "Loading warpfields from individual files...\n";

		// Load warpfields from individual files
		vector<string> filenames;
		this->makeFilepath( m_config.names, m_config.inputWarpfieldsBasepath,
			                ".raw", filenames );

		if( !loadWarpfields( filenames ) )
		{
			std::cerr << "Error: Could not load warpfields from \""
				      << m_config.inputWarpfieldsBasepath << "\"!\n";
			return false;
		}
	}
	else
	{
		// Load single MAT warpfields file
		std::string filename = m_config.filenameWarpfields;

		if( filename.empty() )
		{
			std::cerr << "Error: No warpfield matrix specified!\n";
			return false;
		}	

		if( !loadWarpfields( m_config.numSamples, filename.c_str() ) )
		{
			std::cerr << "Error: Could not load warpfields matrix \""
				      << filename << "\"!\n";
			return false;
		}
	}

	return true;
}

bool StatisticalDeformationModel::
 applyConfig()
{
	using namespace std;

	if( !loadWarpfields() )
		return false;

	// Subtract mean estimate
	if( m_config.subtractMean )
	{
		// Compute mean *before* making the dataset zero-mean!
		computeMean();

		std::cout << "Subtracting mean from warpfields...\n";
		subtractMean();
	}
	else
	{
		std::cout << "Note: No need to compute mean, since dataset is zero-mean "
			         "already!\n";
	}

	// PCA model
	if( m_config.computePCA )
	{
		// Compute PCA anew
		std::cout << "Computing PCA model...\n";
		computePCA();
	}
	else	
	{
		// Load from disk
		if( !loadPCA( m_config.filenameScatter    .c_str(),
	                  m_config.filenameV          .c_str(),
	                  m_config.filenameLambda     .c_str() ) )
		{
			std::cerr << "Error: Could not load PCA model!\n";
			return false;
		}
	}

	// Eigenwarps
	if( m_config.computePCA || m_config.filenameEigenmodes.empty() )
	{
		std::cout << "Reconstructing eigenmodes...\n";
		reconstructEigenmodes();
	}
	else
	{
		// Assume that eigenmodes are also precomputed
		std::string filename = m_config.filenameEigenmodes;
		if( !loadEigenmodes( m_config.numSamples, filename.c_str() ) )
		{
			std::cerr << "Error: Could not load eigenmodes matrix \""
						<< filename << "\"!\n";
			return false;
		}
	}

	// Copy names (if any given)
	m_names = m_config.names;

	// Everything went fine
	return true;
}

void StatisticalDeformationModel::
 print( std::ostream& os )
{
	using namespace std;
	//os << "warpfields  = " << m_warpfields.getFilename() << endl
	//   << "eigenmodes  = " << m_eigenmodes.getFilename() << endl
	//   << "fileScatter = " << m_fileScatter              << endl
	//   << "fileV       = " << m_fileV                    << endl
	//   << "fileLambda  = " << m_fileLambda               << endl
	os << "warpfields  = " << m_config.filenameWarpfields<< endl
	   << "eigenmodes  = " << m_config.filenameEigenmodes<< endl
	   << "fileScatter = " << m_config.filenameScatter   << endl
	   << "fileV       = " << m_config.filenameV         << endl
	   << "fileLambda  = " << m_config.filenameLambda    << endl
	   << "fileMeanwarp= " << m_config.filenameMeanwarp  << endl
	   << "numSamples  = " << m_numSamples               << endl
	   << "resx = "        << m_header.resolution[0]     << endl
	   << "resy = "        << m_header.resolution[1]     << endl
	   << "resz = "        << m_header.resolution[2]     << endl
	   << "spacingx = "    << m_header.spacing[0]        << endl
	   << "spacingy = "    << m_header.spacing[1]        << endl
	   << "spacingz = "    << m_header.spacing[2]        << endl
	   << "offsetx  = "    << m_header.offset[0]         << endl
	   << "offsety  = "    << m_header.offset[1]         << endl
	   << "offsetz  = "    << m_header.offset[2]         << endl
	   << "reference = "   << m_reference                << endl;

	for( unsigned i=0; i < m_numSamples; i++ )
	{
		os << "names = " << getName(i) << endl;
	}

	/* Do not save options used in initial creation of a SDM config file.
	   This silently assumes that the config.ini is written in the same 
	   directory as all the data matrices, namely outputBasepath. 
	os 
	<< "inputWarpfieldsBasepath = " << m_config.inputWarpfieldsBasepath << endl
	<< "inputWarpfieldsExt      = " << m_config.inputWarpfieldsExt << endl
	<< "outputBasepath          = " << m_config.outputBasepath << endl;
	*/
}

void StatisticalDeformationModel::
 setupConfig()
{
	namespace po = boost::program_options;
	using namespace std;
	
	m_config_opts.add_options()
	("numSamples",
		po::value<unsigned>	( &m_config.numSamples ),
		"Number of datasets")
	
	("warpfields", 
		po::value<string>	( &m_config.filenameWarpfields ), 
		"Warpfields MAT file")
	("eigenmodes", 
		po::value<string>	( &m_config.filenameEigenmodes ), 
		"Eigenmodes MAT file")
	("fileScatter", 
		po::value<string>	( &m_config.filenameScatter ), 
		"Scatter matrix MAT file")
	("fileV", 
		po::value<string>	( &m_config.filenameV ), 
		"Right eigenvectors V MAT file")
	("fileLambda", 
		po::value<string>	( &m_config.filenameLambda ), 
		"Eigenvalues / lambda MAT file")
	
	("resx",
		po::value<unsigned>( &m_config.resx )
		->default_value(0),
		"Resolution in X")
	("resy",
		po::value<unsigned>( &m_config.resy )
		->default_value(0),
		"Resolution in Y")
	("resz",
		po::value<unsigned>( &m_config.resz )
		->default_value(0),
		"Resolution in Z")
	
	("spacingx",
		po::value<double>( &m_config.spacingx )
		->default_value(1.0),
		"Spacing in X")
	("spacingy",
		po::value<double>( &m_config.spacingy )
		->default_value(1.0),
		"Spacing in Y")
	("spacingz",
		po::value<double>( &m_config.spacingz )
		->default_value(1.0),
		"Spacing in Z")

	("offsetx",
		po::value<double>( &m_config.offsetx )
		->default_value(0.0),
		"Offset of origin in X")
	("offsety",
		po::value<double>( &m_config.offsety )
		->default_value(0.0),
		"Offset of origin in Y")
	("offsetz",
		po::value<double>( &m_config.offsetz )
		->default_value(0.0),
		"Offset of origin in Z")

	("names",
		po::value< vector<string> >( &m_config.names )->multitoken(),
		"List of dataset names")

	("reference",
		po::value<string>( &m_config.reference ),
		"Filename of reference volume")

	// This is an "action" switch, which will not be saved in the INI file	
	("computePCA",
		po::bool_switch( &m_config.computePCA ),
		"Compute PCA on smaller scatter matrix and reconstruct eigenmodes."
		"Filenames given for these matrices in the config will be used as "
		"output filenames.")

	("outputBasepath",
		po::value<string>( &m_config.outputBasepath ),		
		"Base directory for output matrices. Note that existing matrices will "
		"be silently overwritten.")

	// This is an "action" switch, which will not be saved in the INI file
	("loadWarpfieldsFromNames",
		po::bool_switch( &m_config.loadWarpfieldsFromNames ),
		"Load the warpfields from individual raw files, where the filenames "
		"are generated from the dataset names specified in the config. "
		"See also --inputWarpfieldsBasepath and --inputWarpfieldsExt.")

	("inputWarpfieldsBasepath",
		po::value<string>( &m_config.inputWarpfieldsBasepath ),
		"Base directory for individual raw warpfield files. "
		"Use with --loadWarpfieldsFromNames.")

	("inputWarpfieldsExt",
		po::value<string>( &m_config.inputWarpfieldsExt )
		->default_value(".raw"),
		"Filename extension of individual raw warpfield files."
		"Use with --loadWarpfieldsFromNames.")

	// This is an "action" switch, which will not be saved in the INI file
	("subtractMean",
		po::bool_switch( &m_config.subtractMean ),
		"Subtract meanwarp as column mean of warpfields data matrix.")

	("fileMeanwarp",
		po::value<string>	( &m_config.filenameMeanwarp ),		
		"Meanwarp MAT file")
    ;
}