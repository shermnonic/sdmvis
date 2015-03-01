#include "VTKRayPickerHelper.h"
#include <vtkRenderer.h>
#include <vtkCamera.h>
#include <cmath> // sqrt(), fabs()
#include <cassert>

VTKRayPickerHelper::VTKRayPickerHelper()
{
	m_unproject = vtkMatrix4x4::New();
}

VTKRayPickerHelper::~VTKRayPickerHelper()
{
	m_unproject->Delete();
}

double VTKRayPickerHelper
  ::scalarProduct( const double u[3], const double v[3] ) const
{
	double sp = 0.;
	for( unsigned i=0; i < 3; i++ )
		sp += u[i]*v[i];
	return sp;
}

double VTKRayPickerHelper
  ::normalize( double* v, unsigned n ) const
{
	double mag=0.;
	
	for( unsigned i=0; i < n; i++ )
		mag += v[i]*v[i];
	mag = sqrt(mag);
	
	for( unsigned i=0; i < n; i++ )
		v[i] /= mag;
	
	return mag;
}

void VTKRayPickerHelper
  ::getWorldPoint( int x, int y, const double refPt[3], double pt[3] )
{
	// Get world space ray through viewport pixel (x,y)
	double ray[3];
	getRayThroughScreen( x, y, ray );	
	
	// Get screen normal in world space.
	// To do this unproject the view space normal (0,0,1)'.
	double normal_viewspace[4] = { 0.,0.,1., 1. };
	double normal[4];	
	m_unproject->MultiplyPoint( normal_viewspace, normal );
	
	normalize( normal, 3 );
	normalize( ray, 3 );
	
	///////////////////////////////////////////////////////////////////////////
	//
	// Rationale:
	// ----------
	//
	// A plane through refPt with a given normal is now defined by
	//
	//		{ x | <normal,(x-refPt)> }
	//
	// where <.,.> denots the standard scalar product.
	// The ray through (x,y) is given by
	//
	//		t(alpha) = eye + alpha*rayDir
	//
	// Plugging the ray into the plane equation and solving for alpha yields
	//
	//		alpha* = <normal, (refPt - eye)> / <normal, rayDir>
	//
	// which is well defined for non-parallel normal and ray.
	// The intersection point is then simply
	//
	//		pt = t(alpha*)
	//
	///////////////////////////////////////////////////////////////////////////

	// Get intersection along ray
	
	double u[3];
	u[0] = refPt[0] - m_eye[0];
	u[1] = refPt[1] - m_eye[1];
	u[2] = refPt[2] - m_eye[2];	
	
	double denom = scalarProduct(normal,ray);
	// Sanity check
	if( fabs(denom) < 1e-10 )
		// Picking ray and tangent plane nearly parallel!
		return;
	
	double alpha = scalarProduct(normal,u) / denom;
	
	// Point of intersection
	pt[0] = m_eye[0] + alpha * ray[0];
	pt[1] = m_eye[1] + alpha * ray[1];
	pt[2] = m_eye[2] + alpha * ray[2];
}

void VTKRayPickerHelper
  ::updateCamera()
{
	if( !m_renderer ) return;
	
	vtkRenderer* renderer = m_renderer;
	vtkCamera* camera = renderer->GetActiveCamera();
	
	double position[3];
	camera->GetPosition( position );
	
	double znear, zfar;	
	camera->GetClippingRange( znear, zfar );
	
	double aspect[2];
	renderer->GetAspect( aspect );  
	
	// -- Invert modelview-projection matrix --
  
	// Do this by inverting the individual matrices, since inverse of modelview
	// is required in either case to extract the eye position.
	vtkMatrix4x4* modelview  = camera->GetModelViewTransformMatrix();
	vtkMatrix4x4* projection = 
		camera->GetProjectionTransformMatrix( aspect[1], znear, zfar );
	
	// Temporary matrices
	vtkMatrix4x4* M_inv = vtkMatrix4x4::New();
	vtkMatrix4x4* P_inv = vtkMatrix4x4::New();

	vtkMatrix4x4::Invert( modelview,  M_inv );
	vtkMatrix4x4::Invert( projection, P_inv );	
	vtkMatrix4x4::Multiply4x4( M_inv, P_inv, m_unproject );
  
	// Eye position is 4th column of inverse modelview	
	m_eye[0] = M_inv->GetElement(0,3);
	m_eye[1] = M_inv->GetElement(1,3);
	m_eye[2] = M_inv->GetElement(2,3);
	m_eye[3] = M_inv->GetElement(3,3);

	// Free temporary matrices
	M_inv->Delete();
	P_inv->Delete();
}

void VTKRayPickerHelper
  ::getRayThroughScreen( int x, int y, double rayDir[3] )
{
	assert( m_renderer );
	
	int* size = m_renderer->GetSize();
	
	// Sanity check
	if( size[0] <= 0 || size[1] <= 0 )
		return;

	// Convert screen space coordinate to normalized coordinates
	double nx =      2.*x / (double)size[0] - 1.,
	       ny = 1. - 2.*y / (double)size[1]; // Invert y
	
	// Get world point on near-plane corresponding to (x,y)	
	double nearPoint[4];
	double screenPoint[4] = { 0., 0., 0., 1. };
	screenPoint[0] = nx;
	screenPoint[1] = ny;	
	m_unproject->MultiplyPoint( screenPoint, nearPoint );
	
	// Shoot ray from eye through near-plane point corresponding to (x,y)	
	rayDir[0] = nearPoint[0] - m_eye[0];
	rayDir[1] = nearPoint[1] - m_eye[1];
	rayDir[2] = nearPoint[2] - m_eye[2];	
}
