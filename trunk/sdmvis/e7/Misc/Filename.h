#ifndef MISC_FILENAME_H
#define MISC_FILENAME_H

#include <string>

namespace Misc
{

/// Decompose filename into path, name and extension.
struct Filename
{
	Filename() {}
	Filename( const char* fname );
	std::string filename; ///< original input filename
	std::string path;     ///< stripped path (Unix style)
	std::string name;     ///< stripped name (without extension)
//	std::string ext;      ///< extension (lowercase)
	bool empty() { return filename.empty(); }
};

} // namespace Misc

#endif
