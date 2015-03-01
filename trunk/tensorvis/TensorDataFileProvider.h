#ifndef TENSORDATAFILEPROVIDER_H
#define TENSORDATAFILEPROVIDER_H

#include "TensorDataProvider.h"
#include "TensorData.h"

#ifndef VTKPTR
#include <vtkSmartPointer.h>
#define VTKPTR vtkSmartPointer
#endif

// Forwards
class vtkImageData;

/// Thin wrapper around \a TensorData to load our custom raw tensor fileformat.
/// The old version of our custom fileformat had no notion of physical space.
/// As a workaround to give legacy support the ImageDataSpace can be explicitly
/// specified from the outside when calling \a setup().
class TensorDataFileProvider : public TensorDataProvider
{
public:
	/// Load raw tensor data for reference volume. \deprecated
	bool setup( const char* filename, VTKPTR<vtkImageData> refvol );
	
	/// Load tensor data (in the new mxvol format)
	bool setup( const char* filename );

protected:
	/// Load raw tensor data of given dimensionality from disk.
	/// (Note that the dimensionality may be overridden if one is given in 
	///  the tensor data file, i.e. if it is a MXVOL file.)
	bool load( const char* filename, ImageDataSpace space );
	
	///@{ Implementation of TensorDataProvider
	virtual void getTensor( double x, double y, double z, 
	                        float (&tensor3x3)[9] );
	virtual void getVector( double x, double y, double z,
	                        float (&vec)[3] );
	virtual bool isValidSamplePoint( double x, double y, double z );
	///@}

private:
	TensorData m_tdata;
	VTKPTR<vtkImageData> m_refvol; // store reference image for thresholding
};

#endif // TENSORDATAFILEPROVIDER_H
