#include "LookupTable.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <exception>
#include <cassert>
#include <cctype> // for tolower()

using namespace std;

// Some string helper functions
std::string lowercase( std::string const &s )
{
	std::string sl = s;
	for( int i=0; i < sl.length(); i++ )
		sl[i] = tolower( sl[i] );
	return sl;
}

bool endswith( std::string const &s, std::string const &suffix )
{
	if( suffix.length() > s.length()  )
		return false;
	return s.compare( s.length()-suffix.length(), suffix.length(), suffix )==0;
}

bool endswithi( std::string const &s, std::string const &suffix )
{
	if( suffix.length() > s.length()  )
		return false;
	std::string si = lowercase(s.substr( s.length()-suffix.length() ));
	std::string suffi = lowercase(suffix);
	return si.compare(suffi)==0;
}


#ifdef LOOKUPTABLE_XML_SUPPORT
#include "tinyxml2.h"
bool LookupTable::readParaviewColorMap( const char* filename )
{
	// Temporary data
	std::string name;
	Table table;

	using namespace tinyxml2;
	XMLDocument doc;
	if( doc.LoadFile(filename) != XML_NO_ERROR )
	{
		std::cerr << "LookupTable::loadParaviewColorMap() : "
			<< "Failed to load \"" << filename << "\"!" << std::endl;
		return false;
	}

	// Get root element "ColorMap"
	XMLElement* root = doc.FirstChildElement("ColorMap");
	if( !root || strcmp(root->Name(),"ColorMap")!=0 )
	{
		std::cerr << "LookupTable::loadParaviewColorMap() : "
			<< "Invalid Paraview XML ColorMap!" << std::endl;
		return false;
	}

	// Only subset of color maps is supported
	const char* space   = root->Attribute("space");
	const char* indexed = root->Attribute("indexedLookup");
	if( (space   && strcmp(space  ,"RGB"  ) != 0) ||
		(indexed && strcmp(indexed,"false") != 0 ) )
	{
		std::cerr << "LookupTable::loadParaviewColorMap() : "
			<< "Unsupported ColorMap type!" << std::endl;
		return false;
	}

	// Read "name"
	const char* c_name  = root->Attribute("name");
	if( c_name )
		name = std::string( c_name );

	// Iterate over all siblings beneath "ColorMap"
	for( XMLElement* el = root->FirstChildElement(); el != NULL; 
		el = el->NextSiblingElement() )
	{
		if( strcmp( el->Name(), "Point" )==0 )
		{
			Datum d;

			int err=0; // No error = 0
			err+=(int)el->QueryDoubleAttribute("x", &d.intensity); // x = value = intensity
			err+=(int)el->QueryDoubleAttribute("r", &d.rgba[0]);
			err+=(int)el->QueryDoubleAttribute("g", &d.rgba[1]);
			err+=(int)el->QueryDoubleAttribute("b", &d.rgba[2]);
			err+=(int)el->QueryDoubleAttribute("o", &d.rgba[3]);   // o = opacity = alpha
			if( err != 0 )
			{
				std::cerr << "LookupTable::loadParaviewColorMap() : "
					<< "Encountered invalid ColorMap entry!" << std::endl;
				return false;
			}

			add( table, d );
		}
		else
		if( strcmp( el->Name(), "NaN" )==0 )
		{
			// NaN not supported yet, but silently ignored
		}
		else
		{
			std::cout << "LookupTable::loadParaviewColorMap() : "
				" Unknown element type \"" << el->Name() << "\"!" << std::endl;
		}
	}

	// Set loaded table
	setName( name );
	m_data = table;
	return true;
}
#endif // LOOKUPTABLE_XML_SUPPORT

void LookupTable::rescaleIntensityRange( double min_, double max_ )
{
	Table& table = m_data;

	double 
		oldMin = table.front().intensity, // Assume a sorted table!
		oldMax = table.back().intensity;

	double factor = (max_ - min_) / (oldMax - oldMin);

	for( int i=0; i < table.size(); i++ )
		table[i].intensity = factor * (table[i].intensity - oldMin) + min_;
}

void LookupTable::add( Table& table, Datum d )
{
	table.push_back( d );	
	std::sort( table.begin(), table.end() );
	// Table must always be kept sorted correctly
}

void LookupTable::add( Datum d )
{
	add( m_data, d );
}

void LookupTable::add( double intensity, double r, double g, double b, double a )
{
	add( Datum(intensity, r,g,b,a) );
}

bool LookupTable::read( const char* c_filename )
{
	m_filename = c_filename;

#ifdef LOOKUPTABLE_XML_SUPPORT
	std::string filename( c_filename );
	if( endswithi(filename,".xml") )
		return readParaviewColorMap( c_filename );
	else
#endif
		return readTable( c_filename );
}

bool LookupTable::readTable( const char* filename )
{
	vector<Datum> data;

	try {
		ifstream f( filename );
		if( !f.good() )
		{
			cerr << "Error: Could not open " << filename << "!" << endl;
			return false;
		}

		while( !f.eof() )
		{
			Datum datum;
			f >> datum.intensity;
			f >> datum.rgba[0];
			f >> datum.rgba[1];
			f >> datum.rgba[2];
			f >> datum.rgba[3];

			add( data, datum );
		}
	
		f.close();
	}
	catch( std::exception& e )
	{
		std::cerr << "LookupTable::read() caught an exception: " << e.what() << "\n";
		return false;
	}

	// Only replace current data if file loaded & parsed succesfully
	m_data = data;
	return true;
}

bool LookupTable::reload()
{
	return read( m_filename.c_str() );
}

LookupTable::Datum LookupTable::getDatum( double intensity, bool applyCorrection ) const
{
	Datum result;

	// Sanity: Treat empty data
	if( m_data.empty() )
		return Datum();

	// Clamp values outside defined range
	if( m_data.front().intensity >= intensity )
	{
		result = m_data.front();
		result.intensity = intensity;
	}
	else
	if( m_data.back().intensity <= intensity )
	{
		result = m_data.back();
		result.intensity = intensity;
	}
	else
	{
		// Find first entry for a value >= alpha
		int i0=1; 
		for( ; i0 < (m_data.size()-1) && m_data[i0].intensity < intensity; i0++ );

		// Linear interpolation
		double a0 = m_data[i0-1].intensity,
			  a1 = m_data[i0  ].intensity,
			  interp = (intensity - a0) / (a1 - a0);

		result.intensity = intensity;
		for( int i=0; i < 4; i++ )
			result.rgba[i] = interp * m_data[i0].rgba[i] + 
							(1-interp) * m_data[i0-1].rgba[i];
	}

	// Apply correction
	if( applyCorrection )
	{
		if( m_correctionStrategy == SampleRateCorrection )
		{
			// Opacity correction according to sampling rate (taken from GPUGems)
			double s0 = m_referenceSampleRate;
			double s  = m_sampleRate;
			result.rgba[3] = 1.f - (float)pow(1.0 - (double)result.rgba[3], s0/s);

			// Opacity weighted color
			for( int i=0; i < 3; i++ )
				result.rgba[i] *= result.rgba[3];
		}
		else
		if( m_correctionStrategy == SampleRateCorrectionVTK )
		{
			// Sampling step size
			double d = 1.0 / m_sampleRate;

			// Opacity correction
			result.rgba[3] = 1.f - (float)pow(1.0 - (double)result.rgba[3], d);

			// No premultiplied alpha!
		}
		else
		if( m_correctionStrategy == FixedExponentialCorrection )
		{
			// Exponential pre-scaling of alpha

			// Workaround, downscale emissive term
			const float globalColorScalingFactor = 5.f;
			float stepsize = (float)(1.0 / m_sampleRate);
			for( int i=0; i < 3; i++ )
				result.rgba[i] *= globalColorScalingFactor * stepsize;
			result.rgba[3] = 1.f - exp( -result.rgba[3] * stepsize );
		}
	}

	return result;
}

void LookupTable::getTable( std::vector<float>& buffer, int numEntries, bool applyCorrection ) const
{
	buffer.reserve( numEntries*4 );

	for( int i=0; i < numEntries; i++ )
	{
		float alpha = i / (float)(numEntries-1);

		// Interpolate color
		Datum datum = getDatum( alpha );

		// Put RGBA result in buffer
		buffer.push_back( (float)datum.rgba[0] );  // R
		buffer.push_back( (float)datum.rgba[1] );  // G
		buffer.push_back( (float)datum.rgba[2] );  // B
		buffer.push_back( (float)datum.rgba[3] );  // A
	}
}
