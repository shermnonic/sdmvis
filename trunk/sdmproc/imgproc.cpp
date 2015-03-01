// imgproc - CLI to ImageTools
#include <boost/program_options.hpp>
#include <string>
#include "ImageTools.h"

namespace po = boost::program_options;

using namespace std;

bool performWarp( string inputImage, string deformationField, 
	              string outputImage, bool inverse, int interp )
{
	ImageLoad img( inputImage.c_str() );
	ImageLoad deform( deformationField.c_str() );
	if( !img.isLoaded() )
	{
		cerr << "Error: Could not load image \"" << inputImage << "\"!\n";
		return false;
	}
	if( !deform.isLoaded() )
	{
		cerr << "Error: Could not load vectorfield \"" << deformationField << "\"!\n";
		return false;
	}



	ImageWarp warp( img.getImageData(), deform.getImageData(), inverse, interp );

	ImageSave out( warp.getResult(), outputImage.c_str() );
	
	return true;
}

//-----------------------------------------------------------------------------
//	main()
//-----------------------------------------------------------------------------
int main( int argc, char* argv[] )
{	
	vector<string> configfiles;	

	bool doWarp;
	bool invert;

	std::string inputImage,
		        outputImage,
	            deformationField;

	int interp;

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
	("warp",
		po::bool_switch( &doWarp ),
		"Apply a deformation field to a source image")
	;

	po::options_description arguments("Arguments");
	arguments.add_options()
	("inputImage,i",
		po::value<string>( &inputImage ),
		"Input image")
	("outputImage,o",
		po::value<string>( &outputImage ),
		"Output image")
	("deformationField,d",
		po::value<string>( &deformationField ),
		"Deformation field")
	("invert",		
		po::value<bool>( &invert )
		->default_value( false ),
		"Set to 1 if given deformation should be inverted on-the-fly.")
	("interpolate",
		po::value<int>( &interp )
		->default_value( 1 ),
		"Choose between nearest neighbout (=0), linear (=1) and cubic (=2) "
		"interpolation.")
	;

	po::options_description desc;
	desc.add( general_opts )
		.add( actions )
		.add( arguments );

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

	// Check input
	if( !(vm.count("inputImage") && vm.count("outputImage")) )
	{
		cerr << "Input and output image filenames must be specified!\n";
		return -1;
	}

	// --- Perform actions / computations ---

	if( vm.count("warp") )
	{
		// Check input
		if( !vm.count("deformationField") )
		{
			cerr << "Deformation field missing!\n";
			return -2;
		}

		// Set the inverse flag to false if the user wants to explicitly invert 
		// the deformation.
		performWarp( inputImage, deformationField, outputImage, !invert, interp );
	}

	cout << "imgproc finished.\n";
	return EXIT_SUCCESS;
}
