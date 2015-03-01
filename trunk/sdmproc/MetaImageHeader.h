#ifndef METAIMAGEHEADER_H
#define METAIMAGEHEADER_H
#pragma once

#include <vector>
#include <string>
#include <sstream>

//-----------------------------------------------------------------------------
//	MetaImageHeader
//-----------------------------------------------------------------------------
/// Helper class to write MHD header files for raw scalar- or vector-fields.
struct MetaImageHeader
{
	int    ndims, 
		   num_channels,
		   width, heigth, depth;
	double spacing_x, spacing_y, spacing_z;
	std::string basename,
		        raw_filename,
				element_type_string;

	/// Total number of values stored in dataset
	int size() { return width*heigth*depth*num_channels; }

	MetaImageHeader()
		: ndims       (3), 
		  num_channels(1), 
		  spacing_x   (1.0), 
		  spacing_y   (1.0), 
		  spacing_z   (1.0),
		  element_type_string("MET_FLOAT")
		{}

	void setNumChannels( unsigned ch )
	{
		num_channels = ch;
	}

	void setResolution( unsigned* res )
	{
		width  = res[0];
		heigth = res[1];
		depth  = res[2];
	}

	void setSpacing( double* spacing )
	{
		spacing_x = spacing[0];
		spacing_y = spacing[1];
		spacing_z = spacing[2];
	}

	void setFilename( std::string filename )
	{
		raw_filename = filename;
		basename = raw_filename.substr(0,raw_filename.find_last_of('.'));
	}

	std::string getHeader()
	{
		std::stringstream ss;		
		ss<< "NDims                   = "<< ndims << "\n"
		  << "ElementType             = "<< element_type_string << "\n"
		  << "DimSize                 = "<< width <<" "<< heigth <<" "<< depth <<"\n"
		  << "ElementNumberOfChannels = "<< num_channels <<" \n"
		  << "ElementSpacing          = "<< spacing_x <<" "<< spacing_y <<" "<< spacing_z << "\n"
		  << "ElementDataFile         = "<< stripPath(raw_filename) <<" \n";
		return ss.str();
	}

	static std::string stripPath( std::string fname )
	{
		using namespace std;
	
		size_t slp  = fname.find_last_of( '/' ),
			   bslp = fname.find_last_of('\\'),
			   lp = std::max( slp, bslp );	
	
		if( bslp == string::npos && slp == string::npos )
		{
			// no path delimiters found
			return fname;
		}
	
		if( bslp == string::npos )
			return fname.substr( slp+1 );
		else
		if( slp == string::npos )
			return fname.substr( bslp+1 );
	
		return fname.substr( std::max(slp,bslp)+1 );
	}
};

#endif // METAIMAGEHEADER_H
