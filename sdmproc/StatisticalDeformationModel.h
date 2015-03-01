#ifndef STATISTICALDEFORMATIONMODEL_H
#define STATISTICALDEFORMATIONMODEL_H

#include <boost/program_options.hpp>
#include <string>
#include <vector>
#include "mat/mattools.h"  // RawMatrix
#include "mat/numerics.h"  // Matrix, Vector, rednum::

/**
    Representation of a statistical deformation model (SDM).
   
    A SDM is technically a collection of matrices equipped with SDM semantics.
    This class allows to compute a SDM and provides synthesis functionality
    as well as access semantics. As minimum input a set of deformation fields
    providing the registration are required. These deformation fields are 
    called here warpfields.
   
    The usage of this class is configured through \a SDMConfig options class
    which can be loaded from an INI file or easily set from command line 
    parameters via boost::program_options. In either case you have to call 
    \a applyConfig() to establish a functional SDM.
   
    Basic functionalities are the access to the data matrices and performing
    synthesis and reconstruction from the SDM:
    - \a getWarpfield()
    - \a getEigenmode()
    - \a reconstructField()
    - \a synthesizeField()

	Basic preprocessing options are:
	- loadWarpfieldsFromNames
	- computePCA
	- subtractMean
*/
class StatisticalDeformationModel
{
public:
	/// The used index vector datatype to address rows in the matrix.
	typedef mattools::RawMatrix::IndexVector IndexVector;

	/// Internal value type used for vector fields
	typedef mattools::ValueType ValueType;

	/// Status of which matrices are loaded and available.
	struct Status 
	{
		bool hasWarpfields,
		     hasEigenmodes,
		     hasScatter,
		     hasV,
		     hasLambda;
		
		Status()
		   : hasWarpfields (false),
		     hasEigenmodes (false),
		     hasScatter    (false),
		     hasV          (false),
		     hasLambda     (false)
		  {}

		// REMARK: 
		// Do we really have to distinguish between hasScatter/V/Lambda ?
		// Because only if we have all three, the PCA model is complete.
			  
		bool isComplete() const
		  {
			  return 
				 hasWarpfields &&
				 hasEigenmodes &&
				 hasScatter    &&
				 hasV          &&
				 hasLambda;			  
		  }
	};
	
	/// Volume header information
	struct Header
	{
		unsigned resolution[3];
		double   spacing   [3];
		double   offset    [3];
		
		Header() {
			resolution[0]=resolution[1]=resolution[2]=0;
			spacing[0]=spacing[1]=spacing[2]=1.0;
			offset[0]=offset[1]=offset[2]=0.0;
		}
		
		unsigned getNumElements() const
		{
			return resolution[0] * resolution[1] * resolution[2];
		}
		
		void setResolution( unsigned resx, unsigned resy, unsigned resz )
		{
			resolution[0] = resx;
			resolution[1] = resy;
			resolution[2] = resz;
		}		
		
		void setSpacing( double x, double y, double z )
		{
			spacing[0] = x;
			spacing[1] = y;
			spacing[2] = z;
		}
		
		void setOffset( double x, double y, double z )
		{
			offset[0] = x;
			offset[1] = y;
			offset[2] = z;
		}

		void normalizeCoordinates( Vector& v )
		{
			assert(v.size()==3);
			double x = v(0),
				   y = v(1),
				   z = v(2);
			normalizeCoordinates(x,y,z);
			v(0) = x;
			v(1) = y;
			v(2) = z;
		}

		// Convert physical coordinates to [0,1] normalized ones
		void normalizeCoordinates( double& x, double& y, double& z )
		{
			x = (x - offset[0]) / (resolution[0]*spacing[0]);
			y = (y - offset[1]) / (resolution[1]*spacing[1]);
			z = (z - offset[2]) / (resolution[2]*spacing[2]);
		}

		// Convert normalized coordinates into physical ones
		void physicalizeCoordinates( double& x, double& y, double& z )
		{
			x = x * (resolution[0]*spacing[0]) + offset[0];
			y = y * (resolution[1]*spacing[1]) + offset[1];
			z = z * (resolution[2]*spacing[2]) + offset[2];
		}

		// Get aspect ratio wrt x, useful for undo anisotropic scaling in 
		// normalized coordinates.
		void getAspectX( double& aspectY, double& aspectZ )
		{
			aspectY = (resolution[1]*spacing[1]) / (resolution[0]*spacing[0]);
			aspectZ = (resolution[2]*spacing[2]) / (resolution[0]*spacing[0]);
		}
	};


	/// Config for INI files
	struct SDMConfig
	{
		std::string 
			   filenameWarpfields,
			   filenameEigenmodes,
			   filenameScatter,
			   filenameV,
			   filenameLambda,
			   filenameMeanwarp;

		std::string inputWarpfieldsBasepath,
		            inputWarpfieldsExt,
					outputBasepath;

		// Actions
		bool computePCA, 
			 loadWarpfieldsFromNames,
			 subtractMean;
	
		unsigned numSamples;
	
		unsigned resx, resy, resz;
		double spacingx, spacingy, spacingz;
		double offsetx, offsety, offsetz;

		std::vector<std::string> names;

		std::string reference;
	};
	
	StatisticalDeformationModel();

//protected: // <- below direct access functions still in use by SDMVis
	void clear();
	void clearPCA();
	void clearData();

	bool loadPCA(  const char* filenameScatter,
			const char* filenameV,
			const char* filenameLambda );

	bool loadWarpfields( unsigned numFields, const char* filename );
	bool loadWarpfields( std::vector<std::string> filenames );
	bool loadEigenmodes( unsigned numFields, const char* filename );

	void setWarpfields( mattools::RawMatrix X );
	void setEigenmodes( mattools::RawMatrix U );

	bool loadWarpfields();

	bool loadMean();

public:
	/// Write all matrices for which filenames are specified to disk,
	/// including the warpfields data matrix. Note that any existing files
	/// will be silently overwritten!
	bool saveSDM( std::string basepath="" );
	
	/// Write the current config to an INI file.
	bool saveIni( const char* filename );

	/// Options for \a loadIni
	enum LoadIniOptions {
		IniApplyConfig = 1
	};

	/// Load SDM from a config INI file.
	bool loadIni( const char* filename, int opts=IniApplyConfig );

	/// Program options specific functions for CLI interfaces
	boost::program_options::options_description& getProgramOptions()
	{
		return m_config_opts;
	}

	/// Call this after the config is set to establish the model. 
	/// Loads specified matrices from disk and invokes requested computations.
	bool applyConfig();
	
	/// Print current config in an INI friendly way.
	void print( std::ostream& os=std::cout );

	/// Returns a copy of the current config
	SDMConfig getConfig() const { return m_config; }

	/// Set config
	void setConfig( const SDMConfig& config ) { m_config = config; }

	/// Return number of examples in dataset
	unsigned getNumSamples() const
	{
		return m_numSamples;
	}

	/// Return name of the i-th dataset or a number string if none present
	std::string getName( unsigned idx ) const;

	/// Returns size of vectorfield, assuming same size for warpfields and eigenmodes
	size_t getFieldSize() const
	{
		return (size_t)m_header.getNumElements() * 3;
	}
	
	//@{ Return copy of a specific vectorfield (column) of \a getFieldSize() size.
	void getWarpfield( unsigned idx, mattools::ValueType* rawdata ) const;
	void getEigenmode( unsigned idx, mattools::ValueType* rawdata ) const;
	//@}


	/// Reconstruct a specific dataset from a specific number of eigenmodes.
	/// Internally this \a synthesizeFields() is called while the coefficients
	/// are computed automatically from the loaded \a V and \a lambda matrices.
	void reconstructField( unsigned idx, int numModes, 
		                   mattools::ValueType* rawdata ) const;

	/// Compute linear combination of eigenmodes
	/// In specific circumstances one wants to not add in the mean (e.g. when
	/// extracting multiples of the eigenmodes).
	void synthesizeField( Vector coeffs, mattools::ValueType* result,
	                      bool considerMean=true ) const;

	/// Returns size of a single or multiple rows in our data matrix.
	/// Note that a single row represents one coordinate (x, y or z) of all
	/// vectorfields (datasets) at a specific location. 
	/// \see appendRowIndices()
	size_t getRowSize( IndexVector idx=IndexVector() ) const
	{
		if( !idx.empty() )
			// Return size of numbers of rows in index vector
			return (size_t)idx.size()*m_eigenmodes.getNumCols();
		// Return size of a single row
		return (size_t)m_eigenmodes.getNumCols();
	}

	/// Append 3 matrix row indices for the normalized coordinates (x,y,z) in [0,1].
	void appendRowIndices( double x, double y, double z, IndexVector& idx ) const;

	/// Return a copy of matrix rows.
	/// Note that a single row represents one coordinate (x, y or z) of all
	/// vectorfields (datasets) at a specific location. 
	/// \see appendRowIndices(), getRowSize()
	void getEigenmodeRows( IndexVector rows, mattools::ValueType* rawdata ) const;
	

	mattools::RawMatrix getEigenmodesRawMatrix() const
	{
		return m_eigenmodes;
	}


	//@{ Volume header information
	void setResolution( unsigned resx, unsigned resy, unsigned resz )
	{
		m_header.setResolution( resx, resy, resz );
	}
	void setSpacing( double x, double y, double z )
	{
		m_header.setSpacing( x, y, z );
	}
	void setOffset( double x, double y, double z )
	{
		m_header.setOffset( x, y, z );
	}
	Header getHeader() const
	{
		return m_header;
	}
	//@}

	void setScatterMatrix( const Matrix& scatter )
	{
		m_scatter = scatter;
		m_status.hasScatter = true;
	}

	void setEigenvectors( const Matrix& V )
	{
		m_V = V;
		m_status.hasV = true;
	}

	void setEigenvalues( const Vector& lambda )
	{
		m_lambda = lambda;
		m_status.hasLambda = true;
	}

	Matrix getScatterMatrix() const { return m_scatter; }
	Matrix getEigenvectors () const { return m_V; }
	Vector getEigenvalues  () const { return m_lambda; }

	/// Compute the sample average of the dataset.
	void computeMean();
	/// Raw pointer access to sample average of deformation fields.
	mattools::ValueType* getMeanPtr() { return &m_mean[0]; }
	const mattools::ValueType* getMeanPtr() const { return &m_mean[0]; }

	/// Make warpfields datamatrix to have zero mean.
	/// Note that this function uses a specialized routine and does not call
	/// \a computeMean() internally.
	void subtractMean();

	/// Compute scatter matrix, eigenvalues and -vectors on the fly.
	/// Overwrites existing matrices, including eigenmodes.
	/// Assumes that warpfields datamatrix has already zero mean.
	/// See also \a subtractMean().
	void computePCA();

	/// Reconstruct eigenwarps U = X*V.
	/// Note that this is expensive for large data matrices X.
	void reconstructEigenmodes();

	
protected:
	/// Load raw data matrix with vectorfields stored in columns
	static bool loadRawVectorfields( 
		unsigned numFields, const char* filename, 
		mattools::RawMatrix& mat );

	/// Assemble data matrix from single files, each storing one vectorfield
	static bool loadRawVectorfieldsFromMultipleFiles( 
		std::vector<std::string> filenames,
        mattools::RawMatrix& out );

	static void makeFilepath( std::vector<std::string> names,
		std::string basepath, std::string ext, 
		std::vector<std::string>& filenames );

	void setupConfig();

	/// Internally called by computePCA()
	void computeScatterMatrix();

	/// Helper function for reconstruction of eigenmodes, used in computePCA()
	static void multiply( /*const*/ mattools::RawMatrix& X, Matrix& V, 
		           mattools::RawMatrix& XV );

	/// Helper function to write a raw vector with \a N values to disk
	static void saveVector( const char* filename, mattools::ValueType* buffer, 
					unsigned N );
	
private:
	std::vector<std::string> m_names;
	std::string              m_reference;
	
	mattools::RawMatrix m_warpfields;
	mattools::RawMatrix m_eigenmodes;

	std::vector<mattools::ValueType> m_mean;

	unsigned m_numSamples;

	Matrix m_scatter;
	Matrix m_V;
	Vector m_lambda;

	std::string m_fileScatter,
	            m_fileV,
	            m_fileLambda;

	Status m_status;
	Header m_header;

	SDMConfig m_config;
	boost::program_options::options_description m_config_opts;
	boost::program_options::variables_map       m_config_vmap;
};

#endif // STATISTICALDEFORMATIONMODEL_H
