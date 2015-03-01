#ifndef VTKRAYPICKERHELPER_H
#define VTKRAYPICKERHELPER_H

#include <vtkMatrix4x4.h>

class vtkRenderer;

class VTKRayPickerHelper
{
public:
	VTKRayPickerHelper();
	~VTKRayPickerHelper();

	void setRenderer( vtkRenderer* renderer );

	/// Call this function on a camera change, updates required matrix inverses.
	void updateCamera();

	/// Find intersection of ray under screen position (x,y) with world space
	/// plane parallel to the screen going through refPt.
	/// \param[out] pt Point of intersection
	void getWorldPoint( int x, int y, const double refPt[3], double pt[3] );

protected:
	void getRayThroughScreen( int x, int y, double rayDir[3] );
	
	double normalize( double* v, unsigned n ) const;	
	double scalarProduct( const double u[3], const double v[3] ) const;

private:
	vtkRenderer*  m_renderer;  ///< The renderer provides viewport and camera
	vtkMatrix4x4* m_unproject; ///< Inverse modelview-projection matrix
	double        m_eye[4];    ///< Eye position in world space
};

#endif // VTKRAYPICKERHELPER_H
