// tensordist - compute eigenmodes for a tensor normal distribution nrrd file
#include <iostream>
#include "TensorNormalDistribution.h"

const char usage_msg[] =
"tensordist - compute eigenmodes for a tensor normal distribution nrrd file \n"
"usage: \n"
"  tensordist <input.nrrd> <output-modes.nrrd> <output-spectrum.nrrd> \n";

int main( int argc, char* argv[] )
{
	using namespace std;

	// Parse command line
	if( argc != 4 )
	{
		cout << usage_msg << endl;
		return 0;
	}

	// Load input
	TensorNormalDistribution tnd;
	cout << "Loading 28D tensor normal distribution data from " << argv[1] << endl;
	if( !tnd.load( argv[1] ) )
	{
		cerr << "Error loading " << argv[1] << "!" << endl;
		return -1;
	}
	
	// Heavy computation...
	cout << "Computing eigenvectors (modes) and eigenvalues (spectrum)..." << endl;
	tnd.computeModes();

	// Save results
	cout << "Saving modes to " << argv[2] << "..." << endl;
	tnd.save_modes   ( argv[2] );
	cout << "Saving spectrum to " << argv[3] << "..." << endl;
	tnd.save_spectrum( argv[3] );

	// Wait for keypress
	cout << "Press any key to end program..." << endl;
	getchar();
	
	return 0;
}
