#ifndef MISC_FILESYSTEM_TOOLS
#define MISC_FILESYSTEM_TOOLS

//------------------------------------------------------------------------------
//  Boost filesystem utilities
//------------------------------------------------------------------------------
//	Based on filesystem library version 3
//------------------------------------------------------------------------------

#include <boost/filesystem.hpp>
#include <vector>
#include <string>

namespace Misc {

std::string get_executable_path();

void get_files_in_directory( std::string root_s,
	                         std::vector<std::string>& fnames,
							 std::string extfilter="", bool fullpath=true );

void synchronous_renaming( std::string src, std::string dst );

std::string find_file( std::string filename, std::string path, 
	                   bool case_insensitive=true, int verbose=0 );

std::string find_file( std::string filename, std::vector<std::string> paths,
                       std::string subdir="", 
					   std::vector<std::string> relpaths=std::vector<std::string>(),
					   bool case_insensitive=true, int verbose=0 );

} // namespace Misc
	
#endif // MISC_FILESYSTEM_TOOLS
