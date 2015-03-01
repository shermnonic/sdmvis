#include "LookupTable.h"
#include <fstream>
#include <iostream>

using namespace std;

bool LookupTable::read( const char* filename )
{
	vector<Datum> data;

	m_filename = filename;

	ifstream f( filename );
	if( !f.good() )
	{
		cerr << "Error: Could not open " << filename << "!" << endl;
		return false;
	}

	while( !f.eof() )
	{
		Datum datum;
		f >> datum.alpha;
		f >> datum.rgba[0];
		f >> datum.rgba[1];
		f >> datum.rgba[2];
		f >> datum.rgba[3];

		data.push_back( datum );
	}
	
	f.close();
	m_data = data;
	return true;
}

bool LookupTable::reload()
{
	return read( m_filename.c_str() );
}

LookupTable::Datum LookupTable::getDatum( float alpha ) const
{
	Datum result;

	// Clamp values outside defined range
	if( m_data.front().alpha >= alpha )
	{
		result = m_data.front();
		result.alpha = alpha;
	}
	else
	if( m_data.back().alpha <= alpha )
	{
		result = m_data.back();
		result.alpha = alpha;
	}
	else
	{
		// Find first entry for a value >= alpha
		int i0=1; 
		for( ; i0 < (m_data.size()-1) && m_data[i0].alpha < alpha; i0++ );

		// Linear interpolation
		float a0 = m_data[i0-1].alpha,
			  a1 = m_data[i0  ].alpha,
			  interp = (alpha - a0) / (a1 - a0);

		result.alpha = alpha;
		for( int i=0; i < 4; i++ )
			result.rgba[i] = interp * m_data[i0].rgba[i] + 
							(1-interp) * m_data[i0-1].rgba[i];
	}

	// Exponential pre-scaling of alpha

	// Workaround, downscale emissive term
	const float globalColorScalingFactor = 5.f;
	for( int i=0; i < 3; i++ )
		result.rgba[i] *= globalColorScalingFactor * m_stepsize;
	result.rgba[3] = 1.f - exp( -result.rgba[3] * m_stepsize );

	return result;
}

void LookupTable::getTable( std::vector<float>& buffer, int numEntries ) const
{
	buffer.reserve( numEntries*4 );

	for( int i=0; i < numEntries; i++ )
	{
		float alpha = i / (float)(numEntries-1);

		// Interpolate color
		Datum datum = getDatum( alpha );

		// Put RGBA result in buffer
		buffer.push_back( datum.rgba[0] );  // R
		buffer.push_back( datum.rgba[1] );  // G
		buffer.push_back( datum.rgba[2] );  // B
		buffer.push_back( datum.rgba[3] );  // A
	}
}
