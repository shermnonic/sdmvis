#include "Filename.h"

namespace Misc
{

Filename::Filename( const char* fname )
: filename( fname )
{	
	// replace backslashes (indicating directory structure) by slashes
	for( unsigned int i=0; i < filename.length(); ++i )
		if( filename[i] == '\\' )  filename[i] = '/';					
	
	// extract path and name
	path = "";
	size_t seploc;
	seploc = filename.find_last_of("/");
	if( seploc != std::string::npos )
		path = filename.substr(0,seploc+1);
	else
		seploc = -1;
	size_t ptloc = filename.find_last_of(".");
	name = filename.substr( seploc+1, ptloc-(seploc+1) );
}

} // namespace Misc
