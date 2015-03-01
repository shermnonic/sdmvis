#include "Reconstruction.h"
#include "MetaImageHeader.h"
#include <iostream>
#include <fstream>

Reconstruction::Reconstruction( const StatisticalDeformationModel& sdm )
	: m_sdm   ( sdm ),
      m_buffer( NULL )
{
}

Reconstruction::~Reconstruction()
{
	if( m_buffer ) delete [] m_buffer;
}

Reconstruction::ValueType* Reconstruction::
	allocateBuffer( unsigned size )
{
	static unsigned cursize = 0;
	// Re-allocate on size change and if buffer is NULL
	if( cursize != size || !m_buffer ) 
	{		
		delete [] m_buffer;
		m_buffer = new ValueType[ size ];
		cursize = size;
	}
	return m_buffer;
}

Reconstruction::ValueType* Reconstruction::
	getBuffer()
{
	return m_buffer;
}

void Reconstruction::computeWarp( unsigned idx, int numModes )
{
	// Set default parameter for zero or negative number of modes
	if( numModes <= 0 ) 
		numModes = m_sdm.getNumSamples();

	allocateBuffer( m_sdm.getFieldSize() );

	// Compute reconstruction
	m_sdm.reconstructField( idx, numModes, getBuffer() );
}

void Reconstruction::synthesizeMode( int mode, double sigma )
{
	Vector coeffs(mode+1, 0.0);	
	coeffs(mode) = sigma;
	synthesizeWarp( coeffs, false /* Do not add in mean! */ ); 
}

void Reconstruction::synthesizeWarp( Vector coeffs, bool considerMean )
{
	allocateBuffer( m_sdm.getFieldSize() );
	m_sdm.synthesizeField( coeffs, getBuffer(), considerMean );
}

bool Reconstruction::saveWarp( std::string basename )
{
	using namespace std;

	// Sanity checks
	if( !m_buffer )
		return false;

	// Setup MHD header
	MetaImageHeader mhd;
	StatisticalDeformationModel::Header header = m_sdm.getHeader();
	mhd.setResolution ( header.resolution );
	mhd.setSpacing    ( header.spacing    );
	mhd.setNumChannels( 3 );

	// Construct filename
	string filename = basename + ".raw";

	// Open output file
	ofstream f( filename.c_str(), ios::binary );
	if( !f.is_open() )
	{
		cerr <<"Error: Could not open \""<< filename <<"\" for writing!\n";
		return false;
	}
	
	// Save raw volume to disk
	f.write( (char*)getBuffer(), m_sdm.getFieldSize() * sizeof(ValueType) );
	f.close();

	// Write MHD header
	mhd.setFilename( filename );
	ofstream hdr( (mhd.basename + ".mhd").c_str() );
	if( !hdr.is_open() )
	{
		cerr << "Error: Could not create MHD header file!\n";
		return false;
	}
	else
	{
		hdr << mhd.getHeader() << endl;
		hdr.close();
	}

	// Everything went smooth
	return true;
}