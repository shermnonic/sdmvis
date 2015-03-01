#include "TensorDataFileProvider.h"
#include <vtkImageData.h>

//-----------------------------------------------------------------------------
bool TensorDataFileProvider::
  setup( const char* filename, VTKPTR<vtkImageData> refvol )
{
	using namespace std;

	// Sanity check
	if( !refvol )
	{
		cerr << "Error: TensorVis::setup() requires valid vtkImageData instance!\n";
		return false;
	}

	// Assume tensor data has same dimension as reference volume
	int    dims   [3];
	double spacing[3];
	double origin [3];
	refvol->GetDimensions( dims    );
	refvol->GetSpacing   ( spacing );
	refvol->GetOrigin    ( origin  );
	
	ImageDataSpace space( dims, spacing, origin );

	// Load tensor data
	if( !load( filename, space ) )
	{
		cerr << "Failed to load tensor data!" << endl;
		return false;
	}

	// Set image mask for thresholding
	m_refvol = refvol;	
	setImageMask( m_refvol );

	// Set data space
	setImageDataSpace( space );

	return true;
}

//-----------------------------------------------------------------------------
bool TensorDataFileProvider::
  setup( const char* filename )
{
	using namespace std;

	// Load tensor data
	if( !m_tdata.load( filename ) )
	{
		cerr << "Failed to load tensor data!" << endl;
		return false;
	}

	// Overwrite data space
	setImageDataSpace( m_tdata.getImageDataSpace() );

	return true;
}

//-----------------------------------------------------------------------------
bool TensorDataFileProvider::
  load( const char* filename, ImageDataSpace space )
{	
	return m_tdata.load( filename, space );
}

//-----------------------------------------------------------------------------
void TensorDataFileProvider::
  getTensor( double x, double y, double z, float (&tensor3x3)[9] )
{
	// TensorData has its own getTensor() function operating in image space.
	// Thus we have to convert the physical (x,y,z) coordinates to (i,j,k)
	// image coordinates.
	int ijk[3];
	space().getIJK( x,y,z, ijk );
	m_tdata.getTensor( ijk[0], ijk[1], ijk[2], tensor3x3 );
}

//-----------------------------------------------------------------------------
void TensorDataFileProvider::
  getVector( double x, double y, double z, float (&vec)[3] )
{
	// See comment in getTensor() function, the same holds here.
	int ijk[3];
	space().getIJK( x,y,z, ijk );
	m_tdata.getVector( ijk[0], ijk[1], ijk[2], vec );
}

//-----------------------------------------------------------------------------
bool TensorDataFileProvider::
  isValidSamplePoint( double x, double y, double z )
{
	// If tensor file comes with a mask, use that one
	if( m_tdata.hasMask() )
	{
		int ijk[3];
		space().getIJK( x,y,z, ijk );
		float mask = m_tdata.getMask( ijk[0], ijk[1], ijk[2] );

		// FIXME: For now we apply some default threshold decision criterion.
		return mask > .5f;
	}

	return TensorDataProvider::isValidSamplePoint(x,y,z);
}
