#include "TensorVisRenderBase.h"

#include <vtkRenderer.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkSphereSource.h>
#include <vtkCylinderSource.h>
#include <vtkCubeSource.h>
#include <vtkSuperquadricSource.h>
#include <vtkArrowSource.h>
#include <vtkLookupTable.h>
#include <vtkTextProperty.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkMath.h>

// Visual Studio, disable the following irrelevant warnings:
// C4800: 'int' : forcing value to bool 'true' or 'false' (performance warning)
#pragma warning (disable : 4800)

//-----------------------------------------------------------------------------
//  C'tor
//-----------------------------------------------------------------------------
TensorVisRenderBase::TensorVisRenderBase()
	: 
	  m_samplePolyData(VTKPTR<vtkPolyData>     ::New()),
	  m_tensorGlyph  (VTKPTR<vtkTensorGlyph3>  ::New()),
	  m_tensorMapper (VTKPTR<vtkPolyDataMapper>::New()),
	  m_tensorActor  (VTKPTR<vtkActor>         ::New()),

	  m_vectorGlyph  (VTKPTR<VECTORGLYPH>      ::New()),
	  m_vectorGlyphSource(VTKPTR<CONESOURCE>   ::New()),
	  m_vectorMapper (VTKPTR<vtkPolyDataMapper>::New()),
	  m_vectorActor  (VTKPTR<vtkActor>         ::New()),

	  m_scalarBar    (VTKPTR<vtkScalarBarActor>::New()),

	  m_sampleTensors(NULL),
	  m_glyphType    (Sphere),
	  m_visible      (true)
{
	m_tensorGlyph->ClampScalingOn();
	m_tensorGlyph->ThreeGlyphsOff();

	m_tensorGlyph->SetScaleFactor(10.0);
	m_tensorGlyph->SetColorMode( vtkTensorGlyph3::COLOR_BY_FRACTIONAL_ANISOTROPY );
		// was: vtkTensorGlyph::COLOR_BY_EIGENVALUES
	m_tensorGlyph->SetSuperquadricGamma( 3.0 );
	m_tensorGlyph->SetExtractEigenvalues( (int)true );
	m_tensorGlyph->SetColorGlyphs( (int)true );
	m_tensorGlyph->SetVectorDisplacement( (int)true );
	m_tensorGlyph->SetVectorDisplacementFactor( 1 );

	m_vectorActor->SetVisibility( 1 );
	m_tensorActor->SetVisibility( 1 );

	VTKPTR<vtkLookupTable> lut = getDefaultLookupTable();
	createScalarBar( lut );
}

//-----------------------------------------------------------------------------
//  getDefaultLookupTable()
//-----------------------------------------------------------------------------

class AlphaRamp
{
public:
	AlphaRamp( int n, bool ramp=true ) { n_ = n; ramp_ = ramp; }

	static double interp( int i, int n )
	{
		return i / (float)(n-1);
	}

	double operator () ( int i )
	{
		return ramp_ ? interp( i, n_ ) : 1.0;
	}

private:
	int n_;
	bool ramp_;
};

VTKPTR<vtkLookupTable> TensorVisRenderBase::getDefaultLookupTable() const
{
	VTKPTR<vtkLookupTable> lut = VTKPTR<vtkLookupTable>::New();
  #if 1
	double poettkow[] = {
	 0.3137,    0.6784,    0.9020, 
	 0.4118,    0.5608,    0.8235, 
	 0.5176,    0.4314,    0.7255, 
	 0.6235,    0.3020,    0.6392, 
	 0.7294,    0.1725,    0.5490, 
	 0.8392,    0.0431,    0.4549, 
	 0.8902,    0.0902,    0.3725, 
	 0.9137,    0.2353,    0.2941, 
	 0.9373,    0.3843,    0.2118, 
	 0.9608,    0.5294,    0.1333, 
	 0.9804,    0.6745,    0.0588, 
	 1.0000,    0.7922,    0.0431, 
	 1.0000,    0.8353,    0.2392, 
	 1.0000,    0.8745,    0.4314, 
	 1.0000,    0.9176,    0.6196, 
	 1.0000,    0.9608,    0.8314, 
	 1.0000,    1.0000,    1.0000
	};

	// Poettkow probability colormap
	int n = sizeof(poettkow) / (3 * sizeof(double));
	AlphaRamp ramp( n, false );
	lut->SetNumberOfColors( n );
	for( int i=0; i < n; i++ )
	{
		lut->SetTableValue( i, 
			poettkow[3*i+0], poettkow[3*i+1], poettkow[3*i+2], ramp(i) );
	}
  #else
	// Stupid rainbow colormap
	lut->SetHueRange(4./6.,0);
  #endif

/*
	// Linear ramp between two colors, e.g. for blue-to-yellow transfer color map
	const int numColors = 255;
	double colA[3] = { 0,0,1 };
	double colB[3] = { 1,1,0 };
	lut->SetNumberOfColors( numColors );
	for( unsigned i=0; i < numColors; i++ )
	{
		double lambda = i / (double)(numColors-1);

		double rgba[4];
		for( unsigned j=0; j < 3; j++ )
			rgba[j] = (1.-lambda)*colA[j] + lambda*colB[j];
		rgba[3] = 1.0;

		lut->SetTableValue( i, rgba );
	}
*/
	return lut;
}

//-----------------------------------------------------------------------------
//  vectorGlyphMethod()
//-----------------------------------------------------------------------------
void forwardGlyphMethod( void* instance_of_TensorVisRenderBase )
{
	TensorVisRenderBase* ptr = static_cast<TensorVisRenderBase*>( instance_of_TensorVisRenderBase );
	if( ptr )
		ptr->vectorGlyphMethod();
	else
		std::cerr << "Warning: forwardGlyphMethod() called with unknown pointer!\n";
}

void TensorVisRenderBase::vectorGlyphMethod()
{
	// Sanity workaround
	if( !m_sampleTensors )
		return;

	// Index of current sample point
	vtkIdType ptid = getVectorGlyph()->GetPointId();

	// Average displacement
	double vector[3];
	getVectorGlyph()->GetPointData()->GetVectors()->GetTuple( ptid, vector );
	double vnorm = sqrt(vector[0]*vector[0] + vector[1]*vector[1] + vector[2]*vector[2]);
	
	// Normalize direction
	double normal[3] = { 0., 0., 0. };
	if( vnorm > (0.0 + 2.0 * std::numeric_limits<float>::epsilon()) )
	{
		normal[0] = vector[0] / vnorm;
		normal[1] = vector[1] / vnorm;
		normal[2] = vector[2] / vnorm;
	}

	double radius = 2.3;
 #if 1
	// Compute cone radius from projected tensor eigenvectors
	double* tensor = this->m_sampleTensors->GetTuple9( ptid );
	TensorSpectrum ts;
	if( tensor )
	{
		if( getExtractEigenvalues() )
			// We have tensor data, compute eigenvectors
			ts.compute( tensor );
		else
			// We have scaled eigenvector basis given
			ts.set( tensor );
		radius = ts.maxProjectedLen( normal ) * 0.1; // * m_tensorGlyph->GetScaleFactor();
	}
	else
	{
		cerr << "Warning: Missing tensor data!\n";
	}
 #endif

	// Height of cone proportional to length of displacement
	double height = vnorm * getTensorGlyph()->GetVectorDisplacementFactor();

	// Center of cone, translated s.t. tip is exactly at sample point
	double* pt = m_vectorGlyph->GetPoint();
	double center[3];
 #ifdef TENSORVIS_USE_CUSTOM_CONE
	center[0] = pt[0];
	center[1] = pt[1];
	center[2] = pt[2];
 #else
	center[0] = pt[0] + .5*height*normal[0];
	center[1] = pt[1] + .5*height*normal[1];
	center[2] = pt[2] + .5*height*normal[2];
 #endif

	// Set cone parameters
	CONESOURCE* cone = getVectorGlyphSource();
	cone->SetCenter( center );
	cone->SetRadius( radius );
	cone->SetHeight( height );
	cone->SetResolution( 8 );
	cone->SetDirection( -normal[0], -normal[1], -normal[2] );
 #ifdef TENSORVIS_USE_CUSTOM_CONE
	cone->SetRadius( getGlyphScaleFactor()/2. );
	cone->SetDirection( normal[0], normal[1], normal[2] );
	double l1 = ts.lambda[0],
		   l2 = ts.lambda[1];
	cone->SetPrincipal1( l1*ts.ev1[0], l1*ts.ev1[1], l1*ts.ev1[2] );
	cone->SetPrincipal2( l2*ts.ev2[0], l2*ts.ev2[1], l2*ts.ev2[2] );
 #endif
}

//-----------------------------------------------------------------------------
//  updateGlyphData()
//-----------------------------------------------------------------------------
void TensorVisRenderBase::updateGlyphData( vtkPoints* pts, vtkDataArray* tensors, 
	                                 vtkDataArray* vectors )
{
	// Let's hope that the tensor data pointer doesn't go out of scope ;-)
	m_sampleTensors = tensors;

	// Update PolyData instance (only required first time?)
	m_samplePolyData->SetPoints( pts );
	m_samplePolyData->GetPointData()->SetTensors( tensors );
	
	if( vectors )
	{
		m_samplePolyData->GetPointData()->SetVectors( vectors );
		m_hasVectors = true;
	}
	else
	{
		m_hasVectors = false;
	}
	
	// --- Tensor glyph visualization ---
	
	// Glyph geometry
	m_tensorGlyph->SetInput( m_samplePolyData );
	setGlyphType( getGlyphType() );


	// --- Vectorfield visualization ---	
		
	if( m_hasVectors )
	{
		// Glyph geometry
	#ifdef TENSORVIS_USE_PROGRAMMABLE_VECTORGLYPH
		m_vectorGlyph->SetInput          (0, m_samplePolyData );
		m_vectorGlyph->SetInputConnection(1, m_vectorGlyphSource->GetOutputPort());
		m_vectorGlyph->SetColorModeToColorBySource(); // ?

		m_vectorGlyph->SetGlyphMethod( forwardGlyphMethod, this );
	#else
		VTKPTR<vtkArrowSource> arrow = VTKPTR<vtkArrowSource>::New();
		m_vectorGlyph->SetInput( m_samplePolyData );
		m_vectorGlyph->SetSourceConnection( arrow->GetOutputPort() );
		m_vectorGlyph->SetColorMode( VTK_COLOR_BY_SCALAR );
		m_vectorGlyph->SetScaleMode( VTK_SCALE_BY_VECTOR );
		m_vectorGlyph->ScalingOn();
		m_vectorGlyph->SetScaleFactor( 1e7 );
	#endif

		// Mapper and actor
		m_vectorMapper->SetInputConnection( m_vectorGlyph->GetOutputPort() );
		m_vectorActor->SetMapper( m_vectorMapper );	
	}

	// Color lookup table
	updateColorMap();
}

//-----------------------------------------------------------------------------
//  updateColorMap()
//-----------------------------------------------------------------------------
void TensorVisRenderBase::updateColorMap()
{
	// Create scalar bar (with default values)
	VTKPTR<vtkLookupTable> lut = getDefaultLookupTable();
	createScalarBar( lut );

	// Get min/max scalar range
	double scalarRange[2];	
	m_tensorGlyph->Update();
	m_tensorGlyph->GetOutput()->GetScalarRange( scalarRange );

	// Customize scalar bar and lookup table according to color mode
	switch( m_tensorGlyph->GetColorMode() )
	{
	case vtkTensorGlyph::COLOR_BY_SCALARS: 
		m_scalarBar->SetTitle("Scalar"); 
		break;

	case vtkTensorGlyph::COLOR_BY_EIGENVALUES:
		m_scalarBar->SetTitle("Max. eigenvalue");
		// Range lower bound is 0
		scalarRange[0] = 0.0;
		break;

	case vtkTensorGlyph3::COLOR_BY_FRACTIONAL_ANISOTROPY:
		m_scalarBar->SetTitle("Fractional anisotropy");		
		// Range [0,1] for FA
		scalarRange[0] = 0.0;
		scalarRange[1] = 1.0;
		break;
	};	

	// Mapper and actor
	m_tensorMapper->SetInputConnection( m_tensorGlyph->GetOutputPort() );
	m_tensorMapper->SetLookupTable( lut );
	m_tensorMapper->SetScalarRange( scalarRange ); // was: ( 0., 10. );
	m_tensorActor->SetMapper( m_tensorMapper );

	if( m_hasVectors )
	{
		m_vectorMapper->SetLookupTable( lut );
		m_vectorMapper->SetScalarRange( scalarRange ); // was: ( 0., 10. );	
	}
}

//-----------------------------------------------------------------------------
//  setRenderer()
//-----------------------------------------------------------------------------
void TensorVisRenderBase::setRenderer( vtkRenderer* renderer )
{
	if( !renderer ) return;
	
	renderer->AddActor( m_tensorActor );
	renderer->AddActor( m_vectorActor );
	renderer->AddActor( m_scalarBar );
}

//-----------------------------------------------------------------------------
//  set/get Visibility()
//-----------------------------------------------------------------------------
void TensorVisRenderBase::setVisibility( bool visible )
{
	if( m_visible == visible ) return;

	// Canonical access to all Actors, etc. as vtkProp*
	const int NumProps = 3;
	vtkProp* props[NumProps];
	props[0] = (vtkProp*)m_tensorActor;
	props[1] = (vtkProp*)m_scalarBar;
	props[2] = (vtkProp*)m_vectorActor;

	// Remember previous states when turning invisible
	static int state[NumProps];

	if( m_visible )
	{		
		for( int i=0; i < NumProps; ++i )
		{
			// Turning from visible to invisible, save visibility states
			state[i] = props[i]->GetVisibility();
			// Hide actor
			props[i]->SetVisibility( 0 );
		}
		m_visible = false;
	}
	else
	{
		// Turning visible again, restore previous states
		for( int i=0; i < NumProps; ++i )
			props[i]->SetVisibility( state[i] );
		
		m_visible = true;
	}
}

bool TensorVisRenderBase::getVisibility() const
{
	return m_visible;
}

//-----------------------------------------------------------------------------
//  set/get VectorfieldVisibility()
//-----------------------------------------------------------------------------
void TensorVisRenderBase::setVectorfieldVisibility( bool b )
{
	m_vectorActor->SetVisibility( b );
}

bool TensorVisRenderBase::getVectorfieldVisibility() const
{
	return (bool)m_vectorActor->GetVisibility();
}

//-----------------------------------------------------------------------------
//  set/get TensorVisibility()
//-----------------------------------------------------------------------------
void TensorVisRenderBase::setTensorVisibility( bool b )
{
	m_tensorActor->SetVisibility( b );
}

bool TensorVisRenderBase::getTensorVisibility() const
{
	return (bool)m_tensorActor->GetVisibility();
}

//-----------------------------------------------------------------------------
//  set/get LegendVisibility()
//-----------------------------------------------------------------------------
void TensorVisRenderBase::setLegendVisibility( bool b )
{
	m_scalarBar->SetVisibility( b );
}

bool TensorVisRenderBase::getLegendVisibility() const
{
	return (bool)m_scalarBar->GetVisibility();
}

//-----------------------------------------------------------------------------
//  getLookupTable()
//-----------------------------------------------------------------------------
vtkScalarsToColors* TensorVisRenderBase::getLookupTable()
{
	return m_tensorMapper->GetLookupTable();
}

//-----------------------------------------------------------------------------
//  createScalarBar()
//-----------------------------------------------------------------------------
void TensorVisRenderBase::createScalarBar( vtkScalarsToColors* lut )
{	
	m_scalarBar->SetLookupTable( lut );

	// title
	m_scalarBar->SetTitle("(No title)"); // create dummy title for text props.
	m_scalarBar->GetTitleTextProperty()->SetFontSize( 10 );
	m_scalarBar->GetTitleTextProperty()->SetBold( 0 );
	m_scalarBar->GetTitleTextProperty()->SetItalic( 0 );
	m_scalarBar->GetTitleTextProperty()->SetColor( 1,1,1 );

	// labels
	m_scalarBar->SetNumberOfLabels( 2 );
	m_scalarBar->GetLabelTextProperty()->SetFontSize( 10 );
	m_scalarBar->GetLabelTextProperty()->SetBold( 0 );
	m_scalarBar->GetLabelTextProperty()->SetItalic( 0 );
	m_scalarBar->GetLabelTextProperty()->SetColor( 1,1,1 );
	
#if 0
	// absolute size
	m_scalarBar->SetOrientationToVertical();
	m_scalarBar->SetMaximumWidthInPixels( 80 );
	m_scalarBar->SetMaximumHeightInPixels( 800 );
	m_scalarBar->SetHeight(400);
#else
	// relative size
	m_scalarBar->SetOrientationToHorizontal();
	m_scalarBar->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();

  #if 1 // see also GlyphVisualization.cpp
	m_scalarBar->SetPosition(0.2, 0.02);
	m_scalarBar->SetWidth(0.6);
	m_scalarBar->SetHeight(0.1); 
  #else
	m_scalarBar->SetPosition(0.32, 0.02);
	m_scalarBar->SetWidth(0.36); 
	m_scalarBar->SetHeight(0.063);
  #endif
#endif
	
	// Visibility is now controlled via user interface
	//m_scalarBar->SetVisibility( (lut?1:0) );
}

//-----------------------------------------------------------------------------
//  get/set GlyphType()
//-----------------------------------------------------------------------------
void TensorVisRenderBase::setGlyphType( int glyphType )
{
	m_glyphType = glyphType;

	// Sphere geometry
	VTKPTR<vtkSphereSource> sphereGlyph = VTKPTR<vtkSphereSource>::New();
	sphereGlyph->SetThetaResolution( 8 );
	sphereGlyph->SetPhiResolution  ( 8 );

	// Cylinder geometry
	VTKPTR<vtkCylinderSource> cylinderGlyph = VTKPTR<vtkCylinderSource>::New();
	cylinderGlyph->SetResolution( 8 );
	// Rotate cylinder such that its principal axis matches largest eigenvector.
	// (The principal axis is initially aligned with the global Y-axis and
	//  vtkTensorGlyph will align the largest eigenvector to global X-axis.)
	VTKPTR<vtkTransform> rotz90 = VTKPTR<vtkTransform>::New();
	rotz90->RotateZ( 90 ); // Rotate Y- onto X-axis
	VTKPTR<vtkTransformPolyDataFilter> rotatedCylinder = 
	                                 VTKPTR<vtkTransformPolyDataFilter>::New();
	rotatedCylinder->SetTransform( rotz90 );
	rotatedCylinder->SetInputConnection( cylinderGlyph->GetOutputPort() );

	// Cube geometry
	VTKPTR<vtkCubeSource> cubeGlyph = VTKPTR<vtkCubeSource>::New();

	// Superquadric
	VTKPTR<vtkSuperquadricSource> quadric =VTKPTR<vtkSuperquadricSource>::New();
	quadric->SetPhiResolution( 16 );
	quadric->SetThetaResolution( 16 );
	quadric->SetPhiRoundness( 0.7 );
	quadric->SetThetaRoundness( 0.5 );

	// Tensor glyph filter	
	m_tensorGlyph->SetSuperquadricSource( NULL );
	switch( m_glyphType )
	{
	default:
	case Sphere:
		m_tensorGlyph->SetSourceConnection( sphereGlyph->GetOutputPort() );
		break;
	case Cylinder:
		m_tensorGlyph->SetSourceConnection( rotatedCylinder->GetOutputPort() );
		break;
	case Cube:
		m_tensorGlyph->SetSourceConnection( cubeGlyph->GetOutputPort() );
		break;
	case SuperQuadric:
		m_tensorGlyph->SetSourceConnection( quadric->GetOutputPort() );
		m_tensorGlyph->SetSuperquadricSource( quadric );
	}	
}
int TensorVisRenderBase::getGlyphType() const
{
	return m_glyphType;
}

//-----------------------------------------------------------------------------
//  getTensorGlyph()
//-----------------------------------------------------------------------------
vtkTensorGlyph3* TensorVisRenderBase::getTensorGlyph()
{
	return m_tensorGlyph;
}

//-----------------------------------------------------------------------------
//  get/set GlyphScaleFactor()
//-----------------------------------------------------------------------------
void TensorVisRenderBase::setGlyphScaleFactor( double scale )
{
	m_tensorGlyph->SetScaleFactor( scale );
}
double TensorVisRenderBase::getGlyphScaleFactor() const
{
	return m_tensorGlyph->GetScaleFactor();
}

//-----------------------------------------------------------------------------
//  get/set ExtractEigenvalues()
//-----------------------------------------------------------------------------
void TensorVisRenderBase::setExtractEigenvalues( bool b )
{
	m_tensorGlyph->SetExtractEigenvalues( (int)b );
}
bool TensorVisRenderBase::getExtractEigenvalues() const
{
	return (bool)m_tensorGlyph->GetExtractEigenvalues();
}

//-----------------------------------------------------------------------------
//  get/set ColorGlyphs()
//-----------------------------------------------------------------------------
void TensorVisRenderBase::setColorGlyphs( bool b )
{
	m_tensorGlyph->SetColorGlyphs( (int)b );
}
bool TensorVisRenderBase::getColorGlyphs() const
{
	return (bool)m_tensorGlyph->GetColorGlyphs();
}

//-----------------------------------------------------------------------------
//  get/set ColorMode()
//-----------------------------------------------------------------------------
void TensorVisRenderBase::setColorMode( int mode )
{
	m_tensorGlyph->SetColorMode( mode );
	//updateColorMap();
}
int TensorVisRenderBase::getColorMode() const
{
	return m_tensorGlyph->GetColorMode();
}

//-----------------------------------------------------------------------------
//  get/set SuperquadricGamma()
//-----------------------------------------------------------------------------
void TensorVisRenderBase::setSuperquadricGamma( double gamma )
{
	m_tensorGlyph->SetSuperquadricGamma( gamma );
}
double TensorVisRenderBase::getSuperquadricGamma() const
{
	return m_tensorGlyph->GetSuperquadricGamma();
}
