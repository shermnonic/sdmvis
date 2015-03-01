// sdmproc - CLI to StatisticalDeformationModel
#include <boost/program_options.hpp>
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include "StatisticalDeformationModel.h"
#include "Reconstruction.h"
#include "ImageTools.h"

#ifdef WIN32
#include <Windows.h> // for GetConsoleScreenBufferInfo()
#endif

namespace po = boost::program_options;

using namespace std;

//-----------------------------------------------------------------------------
//	performReconstruction()
//-----------------------------------------------------------------------------
bool performReconstruction( const StatisticalDeformationModel& sdm,
	     string outputDirectory, int numModes )
{
	Reconstruction recon( sdm );

	for( unsigned i=0; i < sdm.getNumSamples(); i++ )
	{
		cout << "Reconstruction " << i+1 <<"/"<< sdm.getNumSamples()
			 << ": " << sdm.getName(i) << "\n";

		recon.computeWarp( i, numModes );

		string basename = outputDirectory + string("/") + sdm.getName(i);
		recon.saveWarp( basename );
	}
	return true;
}

//-----------------------------------------------------------------------------
//	extractMode()
//-----------------------------------------------------------------------------
void extractMode( const StatisticalDeformationModel& sdm, int mode,
                  string outputDirectory )
{
	// Surpress convergence errors of VTK image warping
	int globalWarnings = vtkObject::GetGlobalWarningDisplay();
	vtkObject::GlobalWarningDisplayOff();

	// Assemble output filename
	stringstream ss;
	ss << "mode" << mode << "" << std::setfill('0') << std::setw(3);
	string basename = (outputDirectory.empty() ? "" : (outputDirectory + string("/"))) + ss.str();

	// Get eigenmode from SDM and save as MHD file
	Reconstruction recon( sdm );
	recon.synthesizeMode( mode, 1.0 );
	recon.saveWarp( basename );

	// Reset VTK warnings display
	vtkObject::SetGlobalWarningDisplay( globalWarnings );
}

//-----------------------------------------------------------------------------
//	createAnimation()
//-----------------------------------------------------------------------------

vtkImageData* wrapRawData( void* pointer, int vtktype, int size[3], int dim )
{
	vtkImageData* img = vtkImageData::New();
	img->SetExtent( 0, size[0]-1, 0, size[1]-1, 0, size[2]-1 );
	img->SetScalarType( vtktype );
	img->SetNumberOfScalarComponents( dim );
	img->AllocateScalars();

	int sizeoftype = 0;
	switch( vtktype )
	{
	case VTK_CHAR           : sizeoftype = 1; break;
	case VTK_SIGNED_CHAR    : sizeoftype = 1; break;
	case VTK_UNSIGNED_CHAR  : sizeoftype = 1; break;
	case VTK_SHORT          : sizeoftype = 2; break;
	case VTK_UNSIGNED_SHORT : sizeoftype = 2; break;
	case VTK_INT            : sizeoftype = 4; break;
	case VTK_UNSIGNED_INT   : sizeoftype = 4; break;
	case VTK_LONG           : sizeoftype = 8; break;
	case VTK_UNSIGNED_LONG  : sizeoftype = 8; break;
	case VTK_FLOAT          : sizeoftype = 4; break;
	case VTK_DOUBLE         : sizeoftype = 8; break;
	}
	
	int nelements = size[0]*size[1]*size[2]*dim;
	memcpy( img->GetScalarPointer(), pointer, nelements * sizeoftype );

	return img;
}

template<typename T>
vtkImageData* wrapRawData( T* pointer, int size[3], int dim );

template<>
vtkImageData* wrapRawData<float>( float* pointer, int size[3], int dim )
{
	return wrapRawData( (void*)pointer, VTK_FLOAT, size, dim );
}

template<>
vtkImageData* wrapRawData<double>( double* pointer, int size[3], int dim )
{
	return wrapRawData( (void*)pointer, VTK_DOUBLE, size, dim );
}

#include <vtkImageReslice.h>
#include <vtkObject.h>
#include <vtkImageChangeInformation.h>

void createAnimation( const StatisticalDeformationModel& sdm, int mode,
		string filenameReference, string outputDirectory )
{
	// Load reference image
	ImageLoad ref( filenameReference.c_str() );
	if( !ref.isLoaded() )
	{
		cerr << "Error: Could not load image \"" << filenameReference << "\"!\n";
		return;
	}

	// WORKAROUND: Mis-aligned origins??!!
	bool zeroOrigins = true;	

	vtkSmartPointer<vtkImageChangeInformation> referenceImage = vtkSmartPointer<vtkImageChangeInformation>::New();
	referenceImage->SetInput( ref.getImageData() );

	if( zeroOrigins )
	{
		// One cannot simply call SetOrigin() but has to use vtkImageChangeInformation
		// instead! No, ref.getImageData()->SetOrigin( 0.0, 0.0, 0.0 ) will not work!
		referenceImage->SetOutputOrigin( 0.0, 0.0, 0.0 );
		referenceImage->Update();
	}

	ImageSave out2( referenceImage->GetOutput(), 
		((outputDirectory.empty() ? "" : (outputDirectory + string("/"))) + "reference.mhd").c_str() );

	// Surpress convergence errors of VTK image warping
	int globalWarnings = vtkObject::GetGlobalWarningDisplay();
	vtkObject::GlobalWarningDisplayOff();

	Reconstruction recon( sdm );

	double sigma_range[] = 
	{ -3.0, -2.5, -2.0, -1.5, -1.0, -0.5, 0.0, +0.5, +1.0, +1.5, +2.0, +2.5, +3.0 };
	//{ 0.0 };
	vector<double> sigma( sigma_range, sigma_range+sizeof(sigma_range)/sizeof(double) );

	for( unsigned i=0; i < sigma.size(); i++ )
	{
		// Assemble output filename
		stringstream ss;
		ss << "mode" << mode << "-" 
		   << std::setfill('0') << std::setw(3) << i << "-sigma" << sigma[i];		   
		string basename = (outputDirectory.empty() ? "" : (outputDirectory + string("/"))) + ss.str();
		string filename = basename + ".mhd";

		/// NOTE: It is somewhat stupid to synthesize plain eigenmodes! It
		///       would be sufficient to get the eigenmode once as a 
		///       deformation field and apply the different scalings on-line.
		recon.synthesizeMode( mode, sigma[i] );
		//recon.saveWarp( basename + ".warpfield" ); // We don't really need the warp itself

		// Copy data into vtkImageData
		int dim = 3; // Vectorfield with 3 components per cell
		int size[3];
		size[0] = sdm.getHeader().resolution[0];
		size[1] = sdm.getHeader().resolution[1];
		size[2] = sdm.getHeader().resolution[2];
		vtkImageData* deform = 
			wrapRawData<Reconstruction::ValueType>( recon.getBuffer(), size, dim );
		deform->SetSpacing( sdm.getHeader().spacing );
		deform->SetOrigin ( sdm.getHeader().offset );

		vtkImageData* actualDeform = deform;

		// Subsample
		vtkImageReslice* reslice = vtkImageReslice::New();
		bool subsample = true;
		double scaleFactor = 0.5;
		if( subsample )
		{
			reslice->SetInput( deform );
			reslice->SetOutputSpacing( 
				sdm.getHeader().spacing[0]*(1./scaleFactor),
				sdm.getHeader().spacing[1]*(1./scaleFactor),
				sdm.getHeader().spacing[2]*(1./scaleFactor) );
			reslice->Update();

			actualDeform = reslice->GetOutput();
		}

		// WORKAROUND: Mis-aligned origins??!!
		vtkSmartPointer<vtkImageChangeInformation> displacementField = vtkSmartPointer<vtkImageChangeInformation>::New();
		displacementField->SetInput( actualDeform );
		if( zeroOrigins )
		{
			displacementField->SetOutputOrigin( 0.0, 0.0, 0.0 );
			displacementField->Update();
		}

		// Warp image
		bool inverse = false;
		ImageWarp warp( referenceImage->GetOutput(), displacementField->GetOutput(), inverse );

		// Save warped image
		if( zeroOrigins )
		{
			// Re-set origin to the one of the input reference image
			// ... TBD ...
		}
		ImageSave out( warp.getResult(), filename.c_str() );
		//ImageSave out3( displacementField->GetOutput(), (basename + ".displacement.mhd").c_str() );

		// Free temporary vtkImageData and vtk instances
		deform ->Delete();
		reslice->Delete();
	}

	// Reset VTK warnings display
	vtkObject::SetGlobalWarningDisplay( globalWarnings );
}

//-----------------------------------------------------------------------------
//	main()
//-----------------------------------------------------------------------------
int main( int argc, char* argv[] )
{	
	StatisticalDeformationModel sdm;

	vector<string> configfiles;	
	string saveSDMConfig;

	bool   computePCA = false;
	string basepath;

	string reconstructDir;
	int    reconstructNumModes;
	
	string animateDir;
	vector<int> animateModes;

	string extractModesDir;
	int extractModes;

	// --- Program options ---

	// Adjust linewidth of program_options description to terminal width
	unsigned linewidth = 80;
#ifdef WIN32
	CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
	if( GetConsoleScreenBufferInfo( GetStdHandle(STD_OUTPUT_HANDLE), &csbiInfo ) )
	{
		linewidth = csbiInfo.dwSize.X;
	}
#else
	// For possible Linux implementation see here:
	// http://stackoverflow.com/questions/1022957/getting-terminal-width-in-c
#endif

	po::options_description general_opts("General options",linewidth,linewidth/2);
    general_opts.add_options()
    ("help,h", "produce help message")
	
	("config",
		po::value<vector<string>>(&configfiles)->multitoken(),
		"Read options from config file(s)")
	;

	po::options_description actions("Actions",linewidth,linewidth/2);
	actions.add_options()
	("saveSDMConfig",
		po::value<string>(&saveSDMConfig),
		"Save SDM config to this file")

	("reconstructDir",
		po::value<string>(&reconstructDir),
		"Reconstruct all individual warps and save in the given directory."
		"Assumes that directory already exists. Use with --reconstructNumModes")

	("reconstructNumModes",
		po::value<int>(&reconstructNumModes)
		->default_value(-1),
		"Number of modes to use for PCA reconstruction, specify -1 to use all "
		"available modes. To be used in combination with --reconstructDir.")

	("animateDir",
		po::value<string>(&animateDir),
		"Animate individual modes and save in the given directory."
		"Assumes that directory already exists. Use with --animateMode.")

	("animateMode",
		po::value< vector<int> >(&animateModes)->multitoken(),
		"Create animation of specific mode(s). Requires that a reference image "
		"is set via --reference. See also --animateDir.")

	("extractModes",
		po::value<int>(&extractModes),
		"Extract first n eigenmode displacement fields and save as MHD."
		"To be used in combination with --extractModesDir")

	("extractModesDir",
		po::value<string>(&extractModesDir),
		"Output directory for --extractModes.")
	;	

	po::options_description desc(linewidth,linewidth/2);
	desc.add( general_opts )
		.add( actions )
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
	
	
	// --- Setup SDM ---
	
	cout << "Setting up a statistical deformation model (SDM)...\n";

	if( sdm.applyConfig() )
	{
		cout << "Succesfully set up SDM\n";
	}
	else
		return -1;

	// --- Perform actions / computations ---

	// Save computed SDM matrices
	if( !sdm.getConfig().outputBasepath.empty() )
	{
		cout << "Saving SDM matrices to =\"" << sdm.getConfig().outputBasepath
			<< "\"...\n";

		sdm.saveSDM( sdm.getConfig().outputBasepath );
	}

	// Model reconstruction
	if( vm.count("reconstructDir") )
	{
		cout << "Performing reconstruction...\n";

		performReconstruction( sdm, reconstructDir, reconstructNumModes );
	}

	// Mode animation
	if( vm.count("animateMode") )
	{
		// Check dependencies
		if( sdm.getConfig().reference.empty() )
		{
			cerr << "Error: --animateMode requires --reference to be set!\n";
			return -1;
		}

		// Load mean warp 
		// Important to *not* use computeMean() since we assume zero-mean dataset!
		if( !sdm.loadMean() )
		{
			cerr << "Error: SDM does not specify a mean deformation file!\n";
			return -1;
		}

		// Generate mode animations
		for( int i=0; i < animateModes.size(); i++ )
		{
			cout << "Generating animation " << i+1 << "/" << animateModes.size()
				<< " of mode " << animateModes[i] << "\n";

			createAnimation( sdm, animateModes[i], sdm.getConfig().reference, animateDir );
		}
	}

	// Extract modes
	if( vm.count("extractModes") )
	{
		for( int i=0; i < extractModes; i++ )
		{
			cout << "Extracting mode " << i+1 << "/" << extractModes << "\n";

			extractMode( sdm, i, extractModesDir );
		}
	}

	// --- Save config (optional) ---
	
	if( vm.count("saveSDMConfig") )
	{
		string filename = sdm.getConfig().outputBasepath + saveSDMConfig;
		cout << "Saving SDM config to " << filename << "...\n";
		sdm.saveIni( filename.c_str() );
	}
	
	cout << "sdmproc finished.\n";
	return EXIT_SUCCESS;
};
