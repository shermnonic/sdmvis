#include "VTKCameraSerializer.h"
#include <vtkCamera.h>
#include <iostream>
#include <string>
#include <sstream>
#include <map>

using namespace std;

typedef map<string,string> PropertyStringMap;

PropertyStringMap getPropertyStringMapFromFile( const char* filename )
{
	using namespace std;

	ifstream f(filename);
	if( !f.is_open() )
	{
		cerr << "Error: Could not open " << filename << "!" << endl;
		return PropertyStringMap();
	}

	PropertyStringMap pmap;	

	string line;
	while( getline(f,line) )
	{
		if( line.empty() )
			continue;

		stringstream lstream(line);
		string token;
		lstream >> token;
		
		pmap[token] = line.substr( line.find(token)+token.length()+1 );
		// length + 1 to eliminate one space character between name and data
	}

	f.close();
	return pmap;
}

void writeVec( ostream& os, double* v, int n, string sep=" " )
{
	for( int i=0; i < n; i++ )
		os << v[i] << sep;
	//return os;
}

void writeData( ostream& os, string name, double* v, int n )
{
	//os << name << writeVec(os,v,n) << endl;
	os << name;
	writeVec( os, v, n );
	os << endl;
}

bool readData( PropertyStringMap pmap, string name, double* v, int n )
{
	using namespace std;

	if( pmap.count(name) <= 0 )
		return false;
	
	stringstream ss(pmap[name]);
	for( int i=0; i < n; i++ )
		ss >> v[i];

	return true;
}

bool saveVTKCamera( vtkCamera* camera, const char* filename )
{
	using namespace std;
	ofstream f(filename);
	if( !f.is_open() )
	{
		cerr << "Error: Could not open " << filename << " for writing!" << endl;
		return false;
	}

	double pos[3], focalPoint[3], viewUp[3],
		   clippingRange[2], viewAngle;

	camera->GetPosition( pos );
	camera->GetFocalPoint( focalPoint );
	camera->GetViewUp( viewUp );
	camera->GetClippingRange( clippingRange );
	viewAngle = camera->GetViewAngle();

	writeData(f,"Position "      , pos          ,3);
	writeData(f,"FocalPoint "    , focalPoint   ,3);
	writeData(f,"ViewUp "        , viewUp       ,3);
	writeData(f,"ViewAngle "     , &viewAngle   ,1);
	writeData(f,"ClippingRange " , clippingRange,2);

	f.close();
	return true;
}

bool loadVTKCamera( vtkCamera* camera, const char* filename )
{
	using namespace std;

	PropertyStringMap pmap = getPropertyStringMapFromFile(filename);

	double pos[3], focalPoint[3], viewUp[3],
		   clippingRange[2], viewAngle;

	bool ok=true;
	ok &= readData( pmap, "Position"     , pos          ,3 );
	ok &= readData( pmap, "FocalPoint"   , focalPoint   ,3 );
	ok &= readData( pmap, "ViewUp"       , viewUp       ,3 );
	ok &= readData( pmap, "ViewAngle"    , &viewAngle   ,1 );
	ok &= readData( pmap, "ClippingRange", clippingRange,2 );

	if( !ok )
	{
		cerr << "Error parsing " << filename << "!" << endl;
		return false;
	}

	camera->SetPosition( pos );
	camera->SetFocalPoint( focalPoint );
	camera->SetViewUp( viewUp );
	camera->SetClippingRange( clippingRange );
	camera->SetViewAngle( viewAngle );

	return true;
}
