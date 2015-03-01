// crossvalidate - CLI to cross validate regularization of local covar.tensor
#include "ImageTools.h"
#include "StatisticalDeformationModel.h"
#include "LinearLocalCovariance.h"
#include "MetaImageHeader.h"
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <boost/program_options.hpp>
#include <vtkImageData.h>
#include <vtkSmartPointer.h>

namespace po = boost::program_options;

using namespace std;

//-----------------------------------------------------------------------------
//	writeField()
//-----------------------------------------------------------------------------
bool writeField( const mattools::ValueType* buffer, string name, string path,
	StatisticalDeformationModel::Header header )
{
	string filename = name + ".raw"; 
	string filepath = path + filename;

	unsigned fieldSize = header.getNumElements() * 3;

	// Open output file
	ofstream f( filepath.c_str(), ios::binary );
	if( !f.is_open() )
	{
		cerr <<"Error: Could not open \""<< filename <<"\" for writing!\n";
		return false;
	}
	
	// Save raw volume to disk
	f.write( (char*)buffer, fieldSize * sizeof(mattools::ValueType) );
	f.close();

	// Write MHD header
	MetaImageHeader mhd;
	mhd.setResolution ( header.resolution );
	mhd.setSpacing    ( header.spacing    );
	mhd.setNumChannels( 3 );
	mhd.setFilename( filename );
	ofstream hdr( (path + mhd.basename + ".mhd").c_str() );
	if( !hdr.is_open() )
	{
		cerr << "Error: Could not create MHD header file!\n";
		return false;
	}
	else
	{
		hdr << mhd.getHeader() << endl;
		hdr.close();
	}

	return true;
}

//-----------------------------------------------------------------------------
//	ImageMask
//-----------------------------------------------------------------------------
struct ImageMask
{
	ImageMask()
		: img(NULL), tLow(0.0), tHigh(100000.0)
		{}
	ImageMask( vtkSmartPointer<vtkImageData> img_, double tLow_, double tHigh_=100000.0 )
		: img(img_), tLow(tLow_), tHigh(tHigh_)
		{}
	vtkSmartPointer<vtkImageData> img;
	double tLow, tHigh;
};

//-----------------------------------------------------------------------------
//	SingleWarpfield
//-----------------------------------------------------------------------------
/// This is a small workaround for a missing warpfield class
struct SingleWarpfield
{
	bool load( const StatisticalDeformationModel::SDMConfig& refConf, unsigned r )
	{
		string name_r = refConf.names[r];
		
		StatisticalDeformationModel::SDMConfig conf_r( refConf );
		conf_r.names.clear();
		conf_r.names.push_back( name_r );
		
		sdm.setConfig( conf_r );
		if( !sdm.loadWarpfields() )
		{
			cerr << "Error: Could not load warpfield for " << name_r << "!\n";
			return false;
		}
		
		rawdata.resize( sdm.getFieldSize() );		
		sdm.getWarpfield( 0, &rawdata[0] );
		
		sdm.clearData();		
		return true;
	}
	
	void getDisplacementAt( double x, double y, double z, Vector& edit )
	{
		StatisticalDeformationModel::IndexVector idx;
		sdm.appendRowIndices( x,y,z, idx );
		
		edit.resize(3);
		edit[0] = rawdata[ idx[0] ];
		edit[1] = rawdata[ idx[1] ];
		edit[2] = rawdata[ idx[2] ];
	}

	void getDisplacementAt( double pt[3], Vector& edit )
	{
		getDisplacementAt( pt[0], pt[1], pt[2], edit );
	}
	
	vector<mattools::ValueType> rawdata;
	StatisticalDeformationModel sdm;
};

//-----------------------------------------------------------------------------
//	appendRowIndices()
//-----------------------------------------------------------------------------
void appendRowIndices( const StatisticalDeformationModel& sdm,
	vtkPoints* pts, StatisticalDeformationModel::IndexVector& idx )
{
	double point[3];
	for( unsigned i=0; i < pts->GetNumberOfPoints(); i++ )
	{
		pts->GetPoint( i, point );
	
		// Normalize (x,y,z) to conform to SDM format
		sdm.getHeader().normalizeCoordinates( point[0], point[1], point[2] );

		sdm.appendRowIndices( point[0], point[1], point[2], idx );
	}
}

//-----------------------------------------------------------------------------
//	reconError()
//-----------------------------------------------------------------------------

/// Sum of square differences (SSD) error term
double reconError( 
	const mattools::ValueType* fieldA, const mattools::ValueType* fieldB,
	const StatisticalDeformationModel::IndexVector& idx )
{
	double err = 0.0;
	for( unsigned i=0; i < idx.size(); i++ )
	{
		double di = (double)fieldA[ idx[i] ] - (double)fieldB[ idx[i] ];
		err += di*di;
	}
	// Return average error
	return err / (double)(idx.size()); // was: sqrt(err);
}

/// Directional error term
double reconError2( 
	const mattools::ValueType* fieldA, const mattools::ValueType* fieldB,
	const StatisticalDeformationModel::IndexVector& idx )
{
	// Assume we have (x,y,z) subsequent vector indices
	assert( (idx.size() % 3) == 0 );

	unsigned n = idx.size() / 3;

	double err = 0.0;
	for( unsigned i=0; i < n; i++ )
	{
		double v[3];
		double w[3];

		v[0] = fieldA[idx[3*i+0]];
		v[1] = fieldA[idx[3*i+1]];
		v[2] = fieldA[idx[3*i+2]];

		w[0] = fieldB[idx[3*i+0]];
		w[1] = fieldB[idx[3*i+1]];
		w[2] = fieldB[idx[3*i+2]];

		// Only compare direction of vectors
		double v_norm = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
		double w_norm = sqrt(w[0]*w[0] + w[1]*w[1] + w[2]*w[2]);

		// cos(v,w) = <v,w> / (|v||w|)
		double cos_theta = (v[0]*w[0] + v[1]*w[1] + v[2]*w[2]) / (v_norm*w_norm);		

		// error in [0,2], 0 = same direction, 2 = opposite direction
		err += 1. - cos_theta;
	}
	// Return average error
	return err / (2.*n);
}

/// Norm error term
double reconError3( 
	const mattools::ValueType* fieldA, const mattools::ValueType* fieldB,
	const StatisticalDeformationModel::IndexVector& idx )
{
	// Assume we have (x,y,z) subsequent vector indices
	assert( (idx.size() % 3) == 0 );

	unsigned n = idx.size() / 3;

	double err = 0.0;
	for( unsigned i=0; i < n; i++ )
	{
		double v[3];
		double w[3];
		double d[3];

		v[0] = fieldA[idx[3*i+0]];
		v[1] = fieldA[idx[3*i+1]];
		v[2] = fieldA[idx[3*i+2]];

		w[0] = fieldB[idx[3*i+0]];
		w[1] = fieldB[idx[3*i+1]];
		w[2] = fieldB[idx[3*i+2]];

		d[0] = v[0] - w[0];
		d[1] = v[1] - w[1];
		d[2] = v[2] - w[2];

		err += sqrt( d[0]*d[0] + d[1]*d[1] + d[2]*d[2] );
	}
	// Return average error
	return err / (double)n;
}

//-----------------------------------------------------------------------------
//	performCrossValidation()
//-----------------------------------------------------------------------------
enum CrossValidationErrorTerms {
	ErrorSumOfSquares,
	ErrorDirection,
	ErrorNorm
};

void performCrossValidation( 
	const StatisticalDeformationModel::SDMConfig& refConf,
	vector<double> gammaSampling, vector<double>& errGamma,
	vector<double>& baseline,
	int errorTerm,
	int spatialSampling,
	ImageMask* mask
	)
{
	// We only support loadWarpfieldByNames yet
	assert( refConf.numSamples == refConf.names.size() );
	assert( refConf.loadWarpfieldsFromNames );

	unsigned numSamples = refConf.numSamples;

	// Selecte error term function
	double (*reconErrorFunc)( 
		const mattools::ValueType* fieldA, const mattools::ValueType* fieldB,
		const StatisticalDeformationModel::IndexVector& idx ) 
		= NULL;

	switch( errorTerm ) {
	case ErrorSumOfSquares:	reconErrorFunc = reconError;  break;
	case ErrorDirection   :	reconErrorFunc = reconError2; break;
	default:
	case ErrorNorm        :	reconErrorFunc = reconError3; break;
	}

	switch( errorTerm ) {
	case ErrorSumOfSquares:	cout << "Using SSD error\n";       break;
	case ErrorDirection   :	cout << "Using direction error\n"; break;
	default:
	case ErrorNorm        :	cout << "Using norm error\n";      break;
	}

	// Average recon.error for (r,gamma) iteration
	Matrix err( numSamples, gammaSampling.size() );
	for( unsigned i=0; i < err.size1(); i++ )
		for( unsigned j=0; j < err.size2(); j++ )
			err(i,j) = 0.0;
	
	// Buffer for reconstructed warp field
	vector<mattools::ValueType> reconData;
	// Row indices corrsponding to sampling positions	
	StatisticalDeformationModel::IndexVector idx;
	
	// Setup tensor sampling
	LinearLocalCovariance llc;	
	if( mask )
	{
		llc.setImageMask( mask->img );
		llc.setImageThreshold( mask->tLow, mask->tHigh );
	}
	
	// Iterate over all individuals, where individual r is left out
	for( unsigned r=0; r < numSamples; r++ )
	{
		cout << "Cross-validating " << r+1 << " / " << numSamples << "\n";		
		
		// Load left-out warpfield (sort of a workaround)
		SingleWarpfield leftout;
		if( !leftout.load( refConf, r ) )
			throw("Fatal Error: Could not load warpfield!");
		
		// Build leave-one-out config
		StatisticalDeformationModel::SDMConfig config( refConf );
		config.names.erase( config.names.begin() + r );
		config.numSamples = config.names.size();
		
		// Setup leave-one-out SDM
		StatisticalDeformationModel sdm;
		sdm.setConfig( config );		
		sdm.applyConfig();  // expensive
		//sdm.computeMean();
		
		// Update tensor sampling
		llc.setSDM( &sdm );
		if( r== 0 )
		{
			// Clamp sampling level to [1,4]
			spatialSampling = std::max( 1, spatialSampling );
			spatialSampling = std::min( 4, spatialSampling );

			// Sampling only required in first iteration since we have
			// the same image data space for all subsequent iterations.
			llc.generateGridPoints( spatialSampling );
			
			appendRowIndices( sdm, llc.getPoints(), idx );

			// In first run also allocate reconstruction buffer
			reconData.resize( sdm.getFieldSize() );

			cout << "Spatial sampling level " << spatialSampling << "\n";
			cout << "Number of sample points = " << llc.getNumValidPoints() << "\n";
		}
		
		// Baseline comparison against mean
		double baselineErr = reconErrorFunc
				                ( &leftout.rawdata[0], sdm.getMeanPtr(), idx );
		baseline.push_back( baselineErr );

		// Iterate over gamma values
		cout << "Sampling gamma values"; cout.flush();
		for( unsigned g=0; g < gammaSampling.size(); g++ )
		{
			cout << "."; cout.flush();
			
			llc.setGamma( gammaSampling[g] );
			
			// Iterate over valid points
			for( unsigned p=0; p < llc.getNumValidPoints(); p++ )
			{
				// Compute inner system matrix Z_p depending on gamma
				double point[3];
				llc.getPoints()->GetPoint( p, point );
				// Normalize (x,y,z) to conform to SDM format
				sdm.getHeader().normalizeCoordinates( point[0], point[1], point[2] );
				llc.setReferencePoint( point[0], point[1], point[2] ); // was: ( point );
				
				// Get displacement vector at p
				Vector edit;
				leftout.getDisplacementAt( point, edit );
					//// Normalize (x,y,z) to conform to SDM format
					//sdm.getHeader().normalizeCoordinates( edit );
				
				// Get PCA coefficients 
				Vector coeffs;
				llc.getCoefficients( edit, coeffs );
				
				// Reconstruction				
				sdm.synthesizeField( coeffs, &reconData[0] );
				
				// Compare to original field at sample positions
				mattools::add( &reconData[0], sdm.getMeanPtr(), sdm.getFieldSize(),
				               &reconData[0] );
				
				// Accumulate error term
				err(r,g) += reconErrorFunc
					               ( &reconData[0], &leftout.rawdata[0], idx );
				
			} // valid points p
			
			err(r,g) /= llc.getNumValidPoints();
			
		} // gamma values g		
		cout << endl;
		
	} // left-outs r
	
	// Integrate error over left-outs
	errGamma = vector<double>( gammaSampling.size(), 0.0 );
	for( unsigned r=0;  r < err.size1(); r++ )
		for( unsigned g=0; g < err.size2(); g++ )
			errGamma[g] += err(r,g);
	for( unsigned i=0; i < errGamma.size(); i++ ) 
		errGamma[i] /= numSamples;
}

/* Maybe some coordinate normalization is required ?

	// Normalize (x,y,z) to conform to SDM format
	getSDM()->getHeader().normalizeCoordinates( x, y, z );
*/
/*
		for( unsigned z=0; z < config.resz; z++ )
			for( unsigned y=0; y < config.resy; y++ )
				for( unsigned x=0; x < config.resx; x++ )
				{
					
				}
*/

//-----------------------------------------------------------------------------
//	recreateDatasets()
//-----------------------------------------------------------------------------
bool recreateDatasets( /*const*/ StatisticalDeformationModel& sdm,
	string basepath )
{
	unsigned m = sdm.getFieldSize();
	vector<mattools::ValueType> buffer( m );

	// Load mean warp 
	// Important to *not* use computeMean() since we assume zero-mean dataset!
	if( !sdm.loadMean() )
		return false;

	for( unsigned i=0; i < sdm.getNumSamples(); i++ )
	{
		sdm.getWarpfield( i, &buffer[0] );

		// Assume that we have zero mean, so add mean to gain original dataset
		mattools::add( &buffer[0], sdm.getMeanPtr(), m, &buffer[0] );

		// Save to disk
		string name = sdm.getConfig().names.at(i);
		if( !writeField( &buffer[0], name, basepath, sdm.getHeader() ) )
			return false;

		cout << "Created " << (i+1) << "/" << sdm.getNumSamples() << ": "
			 << (basepath + name + ".mhd") << "\n";
	}

	return true;
}

//-----------------------------------------------------------------------------
//	main()
//-----------------------------------------------------------------------------
int main( int argc, char* argv[] )
{
	StatisticalDeformationModel sdm;
	vector<string> configfiles;	
	
	// Cross-validation parameters
	vector<double> gammaSampling;
	string maskImage;
	double threshLow, threshHigh;
	int samplingStep;

	// Recreation parameters
	string recreateOutdir;
	

	// --- Program options ---
	
	po::options_description general_opts("General options");
    general_opts.add_options()
    ("help,h", "produce help message")
	
	("config",
		po::value<vector<string>>(&configfiles)->multitoken(),
		"Read options from config file(s)")
	;

	po::options_description actions("Actions");
	actions.add_options()
	("crossval",		
		"Perform cross-validation, see also specific options.")

	("recreateDatasets",
		"Re-create individual MHD warpfields from a warpfield matrix. Note "
		"that your SDM config should provide an existing meanwarp since the "
		"eigenwarps are usually zero-mean in this situation.")
	;

	po::options_description recreate_opts("Re-creation options (--recreateDatasets)");
	recreate_opts.add_options()
	("recreateOutdir",
		po::value< string >( &recreateOutdir ),
		"Output directory for re-created MHD warpfields.")
	;

	po::options_description crossval_opts("Cross-validation options (--crossval)");
	crossval_opts.add_options()
	("gamma",
		po::value< vector<double> >(&gammaSampling)->multitoken(),
		"List of gamma values to sample average reconstruction error at.")
	
	("maskImage",
		po::value< string >( &maskImage ),
		"Filename of scalar image mask for thresholding sample positions."
		"See also --maskThreshLow and --maskThreshHigh. All voxel positions "
		"ouside of given threshold range [low,high] will be ignored."
		)
	
	("maskThreshLow",
		po::value< double >( &threshLow )
		->default_value(0.0),
		"Lower threshold for image mask; use with --maskImage.")
	
	("maskThreshHigh",
		po::value< double >( &threshHigh )
		->default_value(100000.0),
		"Higher threshold for image mask; use with --maskImage.")

	("gridSamplingStep",
		po::value<int>( &samplingStep )
		->default_value(2),
		"Spatial sampling, consider only each i-th grid point.")
	;
	
	po::options_description desc;
	desc.add( general_opts )
		.add( actions )
		.add( crossval_opts )
		.add( recreate_opts )
		.add( sdm.getProgramOptions() );	

	po::variables_map vm;        
	try {
		// Parse command line arguments
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);
	
		// Parse config file(s)
		if( vm.count("config") )
		{
			for( unsigned i=0; i < configfiles.size(); i++ )
			{
				string configfile = configfiles.at(i);
				cout << "Reading options from \"" << configfile << "\"...\n";
				po::store(
					po::parse_config_file<char>(configfile.c_str(), desc), vm);
				po::notify(vm);
			}
		}
	} 
	catch( const std::exception& e )
	{
		// Exception handling (boost::program_options already provides nice
		// error messages, so we only print e.what())
		cerr << "Error on parsing comand line arguments: " << e.what() << " "
		     << desc << endl;
		return -1;
	}

	if( vm.count("help") ) 
	{
        cout << desc << "\n";
		return 1;
	}
	
	// --- Perform actions / computations ---

	// Re-create MHD warpfields
	if( vm.count("recreateDatasets") )
	{
		// SDM apply() is too much here since we do not require a PCA model
		// but only the warpfields. The meanwarp is loaded in recreateDatasets().
		if( !sdm.loadWarpfields() )
		{
			return -2;
		}
		if( !recreateDatasets( sdm, recreateOutdir ) )
		{
			return -3;
		}
	}

	// Cross validation
	if( vm.count("crossval") )
	{
		// Set gamma sampling
		if( gammaSampling.empty() )
		{
			cout << "Warning: No gamma sampling values provided, falling back to "
					"some (arbitrary?) default values.\n";
			gammaSampling.push_back( 0.00001 );
			gammaSampling.push_back( 0.0001 );
			gammaSampling.push_back( 0.001 );
			gammaSampling.push_back( 0.01 );
			gammaSampling.push_back( 0.1 );
			gammaSampling.push_back( 1.0 );
			gammaSampling.push_back( 2.0 );
			gammaSampling.push_back( 5.0 );
			gammaSampling.push_back( 10.0 );
			gammaSampling.push_back( 20.0 );
			gammaSampling.push_back( 50.0 );
			gammaSampling.push_back( 100.0 );
			gammaSampling.push_back( 200.0 );
			gammaSampling.push_back( 500.0 );
			gammaSampling.push_back( 1000.0 );
		}	
	
		// Load image mask
		ImageMask mask;
		ImageMask* useMask=NULL;	
		if( vm.count("maskImage") )
		{
			ImageLoad img( maskImage.c_str() );
			if( !img.isLoaded() )
			{
				cerr << "Error: Could not load image \"" << maskImage << "\"!\n";
				return -1;
			}	
			mask = ImageMask( img.getImageData(), threshLow, threshHigh );
			useMask = &mask;
		
			cout << "Using image mask \"" << maskImage << "\" with threshold range "
					"[" << threshLow << ", " << threshHigh << "].\n";
		}

		// Compute ordinary cross validation error
		vector<double> crossValError;
		vector<double> baseline;
		performCrossValidation( sdm.getConfig(), gammaSampling, crossValError, 
		                        baseline, ErrorNorm, samplingStep, useMask );

		printf("Results:\n");

		// Average basline
		double avgBaseline = 0.0;
		for( unsigned i=0; i < baseline.size(); i++ )
		{
			printf("Baseline #%d = %12g\n",i+1,baseline.at(i));
			avgBaseline += baseline.at(i);
		}
		avgBaseline /= (double)baseline.size();
		printf("Average baseline error = %12g\n",avgBaseline);
	
		// Print result	
		printf("%12s     %12s\n","Gamma","Error");
		for( unsigned i=0; i < crossValError.size(); i++ )
		{
			printf("%12g     %12g\n", gammaSampling[i], crossValError[i] );
		}

		cout << "done.\n";
	}
	
	cout << "crossvalidate finished.\n";
	return EXIT_SUCCESS;	
}
