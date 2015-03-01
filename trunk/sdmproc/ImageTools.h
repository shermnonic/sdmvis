#ifndef IMAGETOOLS_H
#define IMAGETOOLS_H

#include <vtkSmartPointer.h>
#include <vtkImageData.h>


//-----------------------------------------------------------------------------
//	ImageWarp
//-----------------------------------------------------------------------------
class ImageWarp
{
public:
	// Enum matches VTK defines
	//	VTK_NEAREST_INTERPOLATION       0
	//	VTK_LINEAR_INTERPOLATION        1
	//	VTK_CUBIC_INTERPOLATION         2
	enum Interpolation {
		InterpolateNearest,
		InterpolateLinear,
		InterpolateCubic 
	};		

	/// Warp an image via the displacement fields. By default the displacement
	/// is assumed to be given in the result coordinate system (of the "fixed"
	/// image) and thus can directly be used for "backward" image warping.
	ImageWarp( vtkImageData* source, vtkImageData* displacement, 
		       bool inverse=true, int interp=InterpolateLinear );

	vtkSmartPointer<vtkImageData> getResult();
private:
	vtkSmartPointer<vtkImageData> m_result;
};


//-----------------------------------------------------------------------------
//	ImageLoad
//-----------------------------------------------------------------------------
class ImageLoad
{
public:
	ImageLoad( const char* filename );
	bool isLoaded() const { return m_isLoaded; }
	vtkSmartPointer<vtkImageData> getImageData() { return m_result; }
private:
	vtkSmartPointer<vtkImageData> m_result;
	bool m_isLoaded;
};


//-----------------------------------------------------------------------------
//	ImageSave
//-----------------------------------------------------------------------------
class ImageSave
{
public:
	ImageSave( vtkImageData* img, const char* filename, 
		       bool compress=false );
	bool isSaved() const { return m_isSaved; }
private:
	bool m_isSaved;
};


#endif // IMAGETOOLS_H
