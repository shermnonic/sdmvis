#include "FilesystemTools.h"
#include <cctype>    // tolower()
#include <algorithm> // find()
#include <iostream>
#include <string>
#ifdef WIN32	// WINDOWS specific
#include <Windows.h>
#else			// LINUX specific
#include <stdio.h>
#endif


namespace Misc {

//------------------------------------------------------------------------------
std::string lowercase( std::string s )
{
	for( size_t i=0; i < s.length(); ++i )
		s[i] = tolower( s[i] );
	return s;
}

//------------------------------------------------------------------------------
std::string get_executable_path()
{	
	std::string exepath_s;

#ifdef WIN32	// WINDOWS

	char exepath[MAX_PATH];
	DWORD n = GetModuleFileName( NULL, exepath, sizeof(exepath) );
	if( n > 0 )
		exepath_s = std::string(exepath);

#else			// LINUX

	// taken from http://stackoverflow.com/questions/143174/c-c-how-to-obtain-the-full-path-of-current-directory
	// not tested yet
	// missing includes
	char exepath[4096];
	char tmp[32];
	sprintf( tmp, "/proc/%d/exe", getpid() );
    int n = std::min( (int)readlink(tmp, exepath, 4096), 4095 );
	if( n >= 0 )
		exepath[n] = '\0';
	exepath_s = std::string(exepath);
#endif

	return exepath;
}

//------------------------------------------------------------------------------
void get_files_in_directory( std::string root_s,
	                         std::vector<std::string>& fnames,
							 std::string extfilter, bool fullpath )
{
	namespace fs = boost::filesystem;
	using namespace std;

	fs::path root( root_s );
	if( fs::is_regular_file( root ) )
		root.remove_filename();
	fnames.clear();

	if( !fs::is_directory( root ) )
	{
		//cerr << "\"" << root.string() << "\" is not a directory!" << endl;
		return;
	}

	// get volume filenames in directory
	fs::directory_iterator end_it; // default construction yields past-the-end
	for( fs::directory_iterator it( root ); it != end_it; ++it )
	{
		if( fs::is_regular_file(it->status()) )
		{
#ifdef SDMVIS_USE_BOOST_FILESYSTEM_2
			string fname( it->path().native_file_string() );
#else
			string fname( it->path().filename().string() );
#endif
			// check extension
			if( !extfilter.empty() )
			{
				// get extension
				size_t extdot = fname.find_last_of('.');
				if( extdot == string::npos )
				{
					// no extension found, skip current file
					continue;
				}

				// lowercase
				string ext = fname.substr( extdot );
				for( size_t i=0; i < ext.length(); ++i )
					ext[i] = tolower(ext[i]);

				// valid extension?
				if( extfilter.find(ext) == string::npos )
				{
					// extension mismatch, skip current file
					continue;
				}
			}

			// store filename
			if( fullpath )
			{
				// full path
				string fullpath = (root / fname).string();
				fnames.push_back( fullpath );
			}
			else
				// plain filename
				fnames.push_back( fname );
		}
	}
}

//------------------------------------------------------------------------------
void synchronous_renaming( std::string src, std::string dst )
{
	using namespace std;
	namespace fs = boost::filesystem;

	string src_ext=".mhd", dst_ext=".jpg";

	vector<string> src_names, dst_names;

	get_files_in_directory( src, src_names, src_ext, false );
	get_files_in_directory( dst, dst_names, dst_ext, true );

	cout << "Found " << src_names.size() << " names in src" << endl;

	if( src_names.size() != dst_names.size() )
	{
		cerr << "Mismatch in number of files in source and target directory!" << endl;
		return;
	}

	for( size_t i=0; i < src_names.size(); ++i )
	{
		// strip extension from src names
		src_names[i].resize( src_names[i].size()-src_ext.size() );
		
		fs::path from_p( dst_names[i] ),
			     to_p( (fs::path(dst) / (src_names[i] + dst_ext)) );

		cout << "rename \"" << from_p.string() << "\" to \"" << to_p.string() << "\"" << endl;
		fs::rename( from_p, to_p );
	}
}

//------------------------------------------------------------------------------
std::string find_file( std::string filename, std::string path, 
	                   bool case_insensitive, int verbose )
{
	namespace fs = boost::filesystem;
	using namespace std;
	vector<string> fnames, fnames_org;
	get_files_in_directory( path, fnames, "", false ); // no full path

	if( verbose>1 )
		cout << "searching \"" << filename << "\" in \"" << path << "\"" << endl;

	// copy original filenames for return path (since fnames may be modified)
	fnames_org = fnames;

	if( case_insensitive )
	{
		// turn to lower case first
		for( size_t i=0; i < fnames.size(); ++i )
			fnames[i] = lowercase( fnames[i] );
		
		filename = lowercase( filename );
	}

	vector<string>::iterator it = find(fnames.begin(), fnames.end(), filename);
	if( it != fnames.end() )  // first exact match
	{
		// found
		size_t index = it - fnames.begin();
		fs::path fullpath = fs::path(path) / fnames_org[index];
		return fullpath.string();
	}
	
	// not found
	return "";
}

//------------------------------------------------------------------------------
std::string find_file( std::string filename, std::vector<std::string> paths,
                       std::string subdir, std::vector<std::string> relpaths,
					   bool case_insensitive, int verbose )
{
	namespace fs = boost::filesystem;
	using namespace std;

	string found_file;

	for( size_t i=0; i < paths.size(); ++i )
	{
		fs::path p_base = fs::path(paths[i]);
		if( fs::is_regular_file( p_base ) )
			p_base.remove_filename();

		fs::path p = p_base;

		if( !subdir.empty() )
		{
			// enforce specific subdirectory
			p = p / subdir;
		}

		found_file = find_file( filename, p.string(), case_insensitive, verbose );

		// found
		if( !found_file.empty() )
			return found_file;

		// search all possible relative path's
		for( size_t j=0; j < relpaths.size(); ++j )
		{
			fs::path relpath = p_base / relpaths[j];

			if( !subdir.empty() )
			{
				// enforce specific subdirectory
				relpath = relpath / subdir;
			}

			found_file = find_file( filename, relpath.string(), case_insensitive, verbose );

			// found
			if( !found_file.empty() )
				return found_file;
		}
	}

	// not found
	return "";
}

} // namespace Misc
