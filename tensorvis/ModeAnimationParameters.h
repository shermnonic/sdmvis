#ifndef MODEANIMATIONPARAMETERS_H
#define MODEANIMATIONPARAMETERS_H

#include <string>
#include <sstream>

/// Parameters for tensor mode animation. See also ModeAnimationWidget.
struct ModeAnimationParameters 
{
	std::string filename_pattern;
	std::string output_path;
	
	int mode;

	double range_min,
	       range_max,
	       range_stepsize;

	static std::string default_filename_pattern()
	{
		return std::string("anim-mode<MODE>-<FRAME>.<ALPHA>.png");
	}

	std::string build_filename( int frame, double val )
	{
		std::string fname = filename_pattern;		
		replace( fname, "<MODE>", to_string<int>(mode) );
		replace( fname, "<FRAME>", to_string<int>(frame) );
		replace( fname, "<ALPHA>", to_string<double>(val) );
		return output_path + "/" + fname;
	}

	/// Replace first occurence of [marker] in [s] with [text].
	static int replace( std::string& s, std::string marker, std::string text )
	{
		int count = 0;
		size_t p = s.find(marker);
		if( p != std::string::npos )
		{
			s.replace( p, marker.length(), text );
			count++;
		}
		return count;
	}
	
	template<class T>
	static std::string to_string( T x )
	{
		std::stringstream ss;
		ss << x;
		return ss.str();
	}
};

#endif // MODEANIMATIONPARAMETERS_H
