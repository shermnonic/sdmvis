#include "VolVis.h"
#include <vtkRenderer.h>
#include <vtkLookupTable.h>
#include <vtkImageMapToColors.h>
#include <vtkVolumeProperty.h>

// Mapper was: vtkVolumeTextureMapper3D

//-----------------------------------------------------------------------------
//  C'tor
//-----------------------------------------------------------------------------
VolVis::VolVis()
:	m_visible( true ),
	// volume
	m_colorFunc  (VTKPTR<vtkColorTransferFunction>::New()),
	m_opacityFunc(VTKPTR<vtkPiecewiseFunction    >::New()),
	m_mapper     (VTKPTR<vtkSmartVolumeMapper    >::New()),
	m_volume     (VTKPTR<vtkVolume               >::New()),
	// isosurface
  #ifdef VOLVIS_MARCHING_CUBES
	m_contour      (VTKPTR<vtkMarchingCubes  >::New()),
  #else
	m_contour      (VTKPTR<vtkContourFilter  >::New()),
	m_normals      (VTKPTR<vtkPolyDataNormals>::New()),
  #endif
	m_contourMapper(VTKPTR<vtkPolyDataMapper >::New()),
	m_contourActor (VTKPTR<vtkActor          >::New()),
	// outline
	m_outline      (VTKPTR<vtkOutlineFilter  >::New()),
	m_outlineMapper(VTKPTR<vtkPolyDataMapper >::New()),
	m_outlineActor (VTKPTR<vtkActor          >::New())
{
	m_mapper->SetInteractiveUpdateRate( 0.0 );
	m_mapper->SetRequestedRenderModeToRayCastAndTexture();
}

//-----------------------------------------------------------------------------
//  D'tor
//-----------------------------------------------------------------------------
VolVis::~VolVis()
{
}

//-----------------------------------------------------------------------------
//  setup()
//-----------------------------------------------------------------------------
void VolVis::setup( VTKPTR<vtkImageData> volume )
{
	int verbose = false;

	// Set member
	m_imageData = volume;

	// Sanity
	int dims[3];
	volume->GetDimensions( dims );
	if( (dims[0]+dims[1]+dims[2]) <= 0 )
	{
		std::cout << "Warning: VolVis trying to set an empty image!\n";
		return;
	}

	double range[2];
	volume->GetScalarRange( range );

	// Setup volume rendering
#if 0
	// Custom transfer function for SHORT rodent mandible CT datasets
	m_opacityFunc->AddPoint(    0.0, 0 );
	m_opacityFunc->AddPoint(   38.6, 0.201057 );
	m_opacityFunc->AddPoint(   86.5, 0.578162 );
	m_opacityFunc->AddPoint(  140.0, 1 );

	m_colorFunc->AddRGBPoint(   0.0, 0,0,0 );
	m_colorFunc->AddRGBPoint(  38.6, 0.494118, 0.494118, 0.494118 );
	m_colorFunc->AddRGBPoint(  86.5, 0.843137, 0.843137, 0.843137 );
	m_colorFunc->AddRGBPoint( 140.0, 1,1,1 );
#else
	// a) setup transfer mapping scalar value to opacity		
	m_opacityFunc->AddPoint(    0,  0 );
	m_opacityFunc->AddPoint(   10,  0 );
	m_opacityFunc->AddPoint(  200,  0.42 );

	// b) setup transfer mapping scalar value to color		
	m_colorFunc->AddRGBPoint(    0, 0,0,0 );
	m_colorFunc->AddRGBPoint(  200, 1,1,1 );
	m_colorFunc->HSVWrapOff();
	m_colorFunc->SetColorSpaceToHSV();
#endif
	// c) setup volume mapper		
	m_mapper->SetInput( volume );
	// c) setup vtkVolume (similar to vtkActor but for volume data)
	//    and set rendering properties (like color and opacity functions)
	m_volume->SetMapper( m_mapper );
	m_volume->GetProperty()->SetInterpolationTypeToLinear();
	m_volume->GetProperty()->SetScalarOpacity( m_opacityFunc );
	m_volume->GetProperty()->SetColor( m_colorFunc );
	
	// Setup isosurface
	m_contour->SetInput( volume );
	m_contour->SetValue( 0, 23 ); // Hounsfield units, bones approx.500-1500HU
#ifdef VOLVIS_MARCHING_CUBES
	m_contour->SetComputeNormals( 1 );
	m_contourMapper->SetInputConnection( m_contour->GetOutputPort() );
	m_contourMapper->ScalarVisibilityOff();
#else
	// contour normals
	m_normals->SetInputConnection( m_contour->GetOutputPort() );
	m_normals->SetFeatureAngle( 60.0 );
	// contour mapper
	m_contourMapper->SetInputConnection( m_normals->GetOutputPort() );
	m_contourMapper->ScalarVisibilityOff();
#endif
	// contour actor
	m_contourActor->SetMapper( m_contourMapper );
	m_contourActor->SetVisibility( 0 );
	
	// Get volume bounds
	double bounds[6];
	m_mapper->GetBounds( bounds );
	if( verbose )
		std::cout << "VolVis: Volume bounds = (" 
                   << bounds[0] << "-" << bounds[1] << "), " <<
               "(" << bounds[2] << "-" << bounds[3] << "), " <<
			   "(" << bounds[4] << "-" << bounds[5] << ") " << std::endl;

	// Add outlines
	m_outline      ->SetInput( volume );
	m_outlineMapper->SetInputConnection( m_outline->GetOutputPort() );
	m_outlineActor ->SetMapper( m_outlineMapper );

	// Force execution of sub-pipelines
	m_volume       ->Update(); 
	m_contourMapper->Update();
	m_outlineMapper->Update();
}

bool VolVis::setup( const char* filename )
{
	// Load volume data from disk	
	VTKPTR<vtkMetaImageReader> reader = VTKPTR<vtkMetaImageReader>::New();
	reader->SetFileName( filename );
	reader->Update();

	// Check if image was loaded successfully
	int dims[3];
	reader->GetOutput()->GetDimensions( dims );
	if( (reader->GetOutput() == NULL) || (dims[0]+dims[1]+dims[2] <= 0) )
	{
		std::cerr << "Error: vtkMetaImageReader could not load MHD file!\n";
		return false;
	}

	setup( reader->GetOutput() );
	return true;
}

//-----------------------------------------------------------------------------
//  setRenderer()
//-----------------------------------------------------------------------------
void VolVis::setRenderer( vtkRenderer* renderer )
{
	if( !renderer ) return;
	renderer->AddVolume  ( m_volume );
	renderer->AddActor   ( m_contourActor );
	renderer->AddViewProp( m_outlineActor );
}

//-----------------------------------------------------------------------------
//  setVisibility()
//-----------------------------------------------------------------------------
void VolVis::setVisibility( bool visible )
{
	if( m_visible == visible ) return;

	// Canonical access to all Actors, etc. as vtkProp*
	const int NumProps = 3;
	vtkProp* props[NumProps];
	props[0] = (vtkProp*)m_volume;
	props[1] = (vtkProp*)m_contourActor;
	props[2] = (vtkProp*)m_outlineActor;

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

//-----------------------------------------------------------------------------
//  Getter / Setter
//-----------------------------------------------------------------------------
void VolVis::setOutlineVisibility( bool visible )
{
	m_outlineActor->SetVisibility( visible );
}

void VolVis::setVolumeVisibility( bool visible )
{	
	m_volume->SetVisibility( visible );
}

void VolVis::setContourVisibility( bool visible )
{
	m_contourActor->SetVisibility( visible );
}

void VolVis::setContourValue( double value )
{
	m_contour->SetValue( 0, value );
	if( hasValidImage() )
		m_contour->Update();
}

double VolVis::getContourValue() const
{
	return m_contour->GetValue( 0 );
}


//-----------------------------------------------------------------------------
//  Getter / Setter
//-----------------------------------------------------------------------------
bool VolVis::hasValidImage()
{
	vtkImageData* img = getImageData();
	
	// Check pointer
	if( !img )
		return false;

	// Check dimensionality
	int dims[3];
	img->GetDimensions( dims );
	if( dims[0]+dims[1]+dims[2] <= 0 )
		return false;

	// Everything seems fine
	return true;
}
