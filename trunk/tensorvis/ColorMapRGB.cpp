#include "ColorMapRGB.h"
#include <iostream>
#include <fstream>
#include <vtkScalarsToColors.h>
#include <vtkLookupTable.h>

using namespace std;

bool ColorMapRGB::read( const char* filename )
{
	ifstream f( filename );
	if( !f.good() )
	{
		cerr << "Error: Could not open " << filename << "!" << endl;
		return false;
	}
	
	Table table;
	while( !f.eof() )
	{
		Datum datum;
		f >> datum.rgb[0];
		f >> datum.rgb[1];
		f >> datum.rgb[2];

		table.push_back( datum );
	}	
	f.close();	
	
	m_table = table;	
	return true;
}

void ColorMapRGB::applyTo( vtkScalarsToColors* colormap )
{
	if( m_table.empty() )
	{
		cerr << "Warning: Trying to apply empty color map!" << endl;
		return;
	}
	
	// Create a lookup table (for easy creation)
	vtkLookupTable* lt = vtkLookupTable::New();

	lt->SetNumberOfColors( m_table.size() );	
	for( unsigned i=0; i < m_table.size(); i++ )
		lt->SetTableValue( i, 
			m_table[i].rgb[0],
			m_table[i].rgb[1],
			m_table[i].rgb[2] );

	// Apply lookup table to given colormap
	colormap->DeepCopy( lt );

	lt->Delete();
}
