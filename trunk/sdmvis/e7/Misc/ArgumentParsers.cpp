#include <iostream>
#include <cstdlib>  // for atoi, atof

namespace Misc
{

void parseInt( const char* s, int& i, int clampLower, int clampUpper )
{
	using namespace std;
	
	i = atoi( s );
	if( i < clampLower )
	{
		i = clampLower;
		cerr << "Warning: [parseInt] Clamping given int value from " << i
		          << " to " << clampLower << "." << endl;
	}
	else
	if( (clampUpper > clampLower) && (i > clampUpper) )
	{
		i = clampUpper;
		cerr << "Warning: [parseInt] Clamping given int value from " << i
		          << " to " << clampUpper << "." << endl;
	}
}

void parseFloat( const char* s, float& f, float clampLower, float clampUpper )
{
	using namespace std;

	f = (float)atof( s );
	if( f < clampLower )
	{
		f = clampLower;
		cerr << "Warning: [parseFloat] Clamping given float value from " << f
		          << " to " << clampLower << "." << endl;
	}
	else
	if( (clampUpper > clampLower) && (f > clampUpper) )
	{
		f = clampUpper;
		cerr << "Warning: [parseFloat] Clamping given float value from " << f
		          << " to " << clampUpper << "." << endl;
	}
}

} // namespace Misc
