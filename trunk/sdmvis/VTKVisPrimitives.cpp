#include "VTKVisPrimitives.h"
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkObjectFactory.h>
#include <vtkMath.h>           // for: vtkMath::RandomSeed()
#include <vtkLookupTable.h>
#include <vtkImageMapToColors.h>
#include <vtkPointData.h>
#include <vtkLookupTable.h>
#include <vtkColorTransferFunction.h>
#include <vtkTextProperty.h>
#include <time.h>              // for: time(NULL)

//-----------------------------------------------------------------------------
//	Markers
//-----------------------------------------------------------------------------

void SphereMarker::setup( double x, double y, double z, double radius )
{		
	m_sphere->SetPhiResolution( 23 );
	m_sphere->SetRadius( radius );
	m_sphere->SetThetaResolution( 23 );
	
	m_mapper->SetInputConnection( m_sphere->GetOutputPort() );

	m_actor->SetMapper( m_mapper );
	m_actor->SetPosition( x, y, z );
}

void ArrowMarker::setup( double x0, double y0, double z0, 
                         double x1, double y1, double z1 )
{
	m_source->SetPoint1( x0, y0, z0 );
	m_source->SetPoint2( x1, y1, z1 );

	m_mapper->SetInputConnection( m_source->GetOutputPort() );

	m_actor->SetMapper( m_mapper );
	//m_actor->SetPosition( x0, y0, z0 );
}

//-----------------------------------------------------------------------------
//	VRen
//-----------------------------------------------------------------------------

bool VRen::setup( const char* filename )
{
	int verbose = true;

	// Load volume data from disk
	
	VTKPTR<vtkMetaImageReader> reader = VTKPTR<vtkMetaImageReader>::New();
	reader->SetFileName( filename );
	reader->Update();

	// Setup volume rendering

	// a) setup transfer mapping scalar value to opacity		
	m_opacityFunc->AddPoint(    0,  0 );
	m_opacityFunc->AddPoint(  255,  1 );

	// b) setup transfer mapping scalar value to color		
	m_colorFunc->AddRGBPoint(    0, 0,0,1 );
	m_colorFunc->AddRGBPoint(  255, 1,0,0 );
	m_colorFunc->HSVWrapOff();
	m_colorFunc->SetColorSpaceToHSV();

	// c) setup volume mapper		
	m_mapper->SetInputConnection( reader->GetOutputPort() );
	// c) setup vtkVolume (similar to vtkActor but for volume data)
	//    and set rendering properties (like color and opacity functions)
	m_volume->SetMapper( m_mapper );
	m_volume->GetProperty()->SetInterpolationTypeToLinear();
	m_volume->GetProperty()->SetScalarOpacity( m_opacityFunc );
	m_volume->GetProperty()->SetColor( m_colorFunc );
	double foo[3];
	m_volume->GetPosition( foo );

	if( verbose )
		std::cout << "Volume position = (" << foo[0] << "," << foo[1] << "," 
		                                   << foo[2] << ")" << std::endl;

	// Setup isosurface

	// contour filter
	m_contour->SetInputConnection( reader->GetOutputPort() );
	m_contour->SetValue( 0, 23 ); // Hounsfield units, bones approx.500-1500HU
#ifdef VREN_MARCHING_CUBES
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

	// Setup renderer

	m_renderer->SetBackground(.2,.2,.2);
	m_renderer->AddVolume( m_volume );	                                    
	m_renderer->AddActor( m_contourActor );
	m_renderer->SetViewport(  0, 0, 1, 1 );
	m_renderer->ResetCamera();

	// get volume bounds
	double bounds[6];
	m_mapper->GetBounds( bounds );
	if( verbose )
		std::cout << "Volume bounds = (" 
                   << bounds[0] << "-" << bounds[1] << "), " <<
               "(" << bounds[2] << "-" << bounds[3] << "), " <<
			   "(" << bounds[4] << "-" << bounds[5] << ") " << std::endl;

	// Add oulines
	m_outline      ->SetInputConnection( reader   ->GetOutputPort() );
	m_outlineMapper->SetInputConnection( m_outline->GetOutputPort() );
	m_outlineActor ->SetMapper( m_outlineMapper );
	m_renderer     ->AddViewProp( m_outlineActor );

	// Add axes		
	//m_axes->SetInput( m_outline->GetOutput() );
	//m_axes->SetBounds( bounds );
	m_axes->SetBounds( bounds[0], bounds[1]+1,
	                   bounds[2], bounds[3]+1,
					   bounds[4], bounds[5]+1 );
	m_axes->SetCamera( m_renderer->GetActiveCamera() );
	m_axes->SetNumberOfLabels( 9 );
	m_renderer->AddViewProp( m_axes );

	// Add grid
	float xlength = (bounds[1]-bounds[0])+1;
	float ylength = (bounds[3]-bounds[2])+1;
	float zlength = (bounds[5]-bounds[4])+1;
	m_grid->SetXLength( xlength );
	m_grid->SetYLength( ylength );
	m_grid->SetZLength( zlength );
	m_grid->SetXCubes( 8 );
	m_grid->SetYCubes( 8 * (ylength/xlength) );
	m_grid->SetZCubes( 8 * (zlength/xlength) );
	m_grid->SetCenterGrid( false );
	m_gridMapper->SetInput( m_grid->GetOutput() );
	m_gridActor ->SetMapper( m_gridMapper );
	m_renderer->AddActor( m_gridActor );

	return true;
}

void VRen::setOutlineVisibility( bool visible )
{
	m_outlineActor->SetVisibility( visible );
}

void VRen::setAxesVisibility( bool visible )
{
	m_axes->SetVisibility( visible );
}

void VRen::setGridVisibility( bool visible )
{
	m_gridActor->SetVisibility( visible );
}

void VRen::setVolumeVisibility( bool visible )
{	
	m_volume->SetVisibility( visible );
}

void VRen::setContourVisibility( bool visible )
{
	m_contourActor->SetVisibility( visible );
}

void VRen::setContourValue( double value )
{
	m_contour->SetValue( 0, value );
}

double VRen::getContourValue() const
{
	return m_contour->GetValue( 0 );
}

void VRen::setGridSize( int size )
{
	double bounds[6];
	m_mapper->GetBounds( bounds );

	float xlength = (bounds[1]-bounds[0])+1;
	float ylength = (bounds[3]-bounds[2])+1;
	float zlength = (bounds[5]-bounds[4])+1;
	//m_grid->SetXLength( xlength );
	//m_grid->SetYLength( ylength );
	//m_grid->SetZLength( zlength );
	m_grid->SetXCubes( size );
	m_grid->SetYCubes( size * (ylength/xlength) );
	m_grid->SetZCubes( size * (zlength/xlength) );

	m_grid->Modified();
	m_grid->Update();
}

int VRen::getGridSize() const
{
	return m_grid->GetXCubes();
}

#include <vtkOBJExporter.h>
void VRen::exportOBJ( const char* fileprefix )
{	
	VTKPTR<vtkOBJExporter> exporter = VTKPTR<vtkOBJExporter>::New();	

	exporter->SetInput( m_renderer->GetRenderWindow() );
	exporter->SetFilePrefix( fileprefix );
	exporter->Write();
}


//-----------------------------------------------------------------------------
//	VRen2 :: VolumeMetaActor
//-----------------------------------------------------------------------------

void VRen2::VolumeMetaActor::setInputConnection( vtkAlgorithmOutput* input )
{
	// a) setup transfer mapping scalar value to opacity		
	opacityFunc->AddPoint(    0,  0 );
	opacityFunc->AddPoint(  255,  1 );

	// b) setup transfer mapping scalar value to color		
	colorFunc->AddRGBPoint(    0, 0,0,1 );
	colorFunc->AddRGBPoint(  255, 1,0,0 );
	colorFunc->HSVWrapOff();
	colorFunc->SetColorSpaceToHSV();

	// c) setup volume mapper
	mapper->SetInputConnection( input );
	// d) setup vtkVolume (similar to vtkActor but for volume data)
	//    and set rendering properties (like color and opacity functions)
	volume->SetMapper( mapper );
	volume->GetProperty()->SetInterpolationTypeToLinear();
	volume->GetProperty()->SetScalarOpacity( opacityFunc );
	volume->GetProperty()->SetColor( colorFunc );
}

void VRen2::VolumeMetaActor::setInput( vtkImageData* input )
{
	    /// TODO: Avoid code duplication by adding an Update() like
		///       function (currently setInputConnection() and
		///       setInput() have nearly identical code)!

	// a) setup transfer mapping scalar value to opacity		
	opacityFunc->AddPoint(    0,  0 );
	opacityFunc->AddPoint(  255,  1 );

	// b) setup transfer mapping scalar value to color		
	colorFunc->AddRGBPoint(    0, 0,0,1 );
	colorFunc->AddRGBPoint(  255, 1,0,0 );
	colorFunc->HSVWrapOff();
	colorFunc->SetColorSpaceToHSV();

	// c) setup volume mapper
	mapper->SetInput( input );
	// d) setup vtkVolume (similar to vtkActor but for volume data)
	//    and set rendering properties (like color and opacity functions)
	volume->SetMapper( mapper );
	volume->GetProperty()->SetInterpolationTypeToLinear();
	volume->GetProperty()->SetScalarOpacity( opacityFunc );
	volume->GetProperty()->SetColor( colorFunc );
}

//-----------------------------------------------------------------------------
//	VRen2 :: IsosurfaceMetaActor
//-----------------------------------------------------------------------------

void VRen2::IsosurfaceMetaActor::setInputConnection( vtkAlgorithmOutput *input)
{
	// REMARK: 
	// Disabled depthsorting since we assume that depth-peeling is used!
	bool do_depthsort = false && (activeCamera.GetPointer()!=NULL);

	// contour filter
	contour->SetInputConnection( input );
	contour->SetValue( 0, 16 ); // Hounsfield units, bones approx.500-1500HU
#ifdef VREN_MARCHING_CUBES
	contour->SetComputeNormals( 1 );

	if( do_depthsort )
	{
		std::cout << "VRen2::IsosurfaceMetaActor (on input " << input << ")"
		          << ": Depthsorting enabled!" << std::endl;
		depthsort->SetInputConnection( contour->GetOutputPort() );
		depthsort->SetDirectionToBackToFront();
		depthsort->SetCamera( activeCamera );
		contourMapper->SetInputConnection( depthsort->GetOutputPort() );
		contourMapper->ScalarVisibilityOff();
	}
	else
	{	
		contourMapper->SetInputConnection( contour->GetOutputPort() );
		contourMapper->ScalarVisibilityOff();
	}
#else
	// contour normals
	normals->SetInputConnection( contour->GetOutputPort() );
	normals->SetFeatureAngle( 60.0 );
	// contour mapper
	contourMapper->SetInputConnection( normals->GetOutputPort() );
	contourMapper->ScalarVisibilityOff();
#endif
	// contour actor
	contourActor->SetMapper( contourMapper );
	contourActor->SetVisibility( 0 );
}

//-----------------------------------------------------------------------------
//	VRen2
//-----------------------------------------------------------------------------

vtkImageData* VRen2::getVolumeImageData( int volid )
{
	if( volid==1 )
		return m_volume1.mapper->GetInput();
	return m_volume0.mapper->GetInput();
}

#ifdef VREN_SUPPORT_TOM
void VRen2::setTpsTransform( tomTpsTransform* tps )
{
	// Warp grid
	if( m_verbosity > 2 )
		std::cout << "VRen2: TPS warp grid" << std::endl;
	m_grid->tomSetTpsTransform( tps );
	m_grid->Modified();
	m_grid->Update();

	if( m_verbosity > 2 )
		std::cout << "VRen2: Setting isosurface coloring to TPS magnitude (experimental!)" << std::endl;

	// Set scalar data of reference isosurface to warpfield magnitude
	// ATTENTION: This modifies the isosurface pipeline!
	vtkPolyData* isosurf = m_contour0.contour->GetOutput();
	float min, max;
	tomTpsWarpfield( *tps, (vtkPointSet*)isosurf, min, max, false );
	m_contour0.contourMapper->RemoveAllInputs(); // !!
	m_contour0.contourMapper->SetInput( isosurf );

	// Color lookup table
	vtkColorTransferFunction* colorFunc = vtkColorTransferFunction::New();
	colorFunc->AddRGBPoint(  min, 0,0,1 );
	colorFunc->AddRGBPoint(  max, 1,0,0 );
	colorFunc->HSVWrapOff();
	colorFunc->SetColorSpaceToHSV();
	m_contour0.contourMapper->SetLookupTable( colorFunc );
	m_contour0.contourMapper->SetColorModeToMapScalars();
	m_contour0.contourMapper->Update();
	m_contour0.contourMapper->ScalarVisibilityOn();
	colorFunc->Delete();

	if( m_verbosity > 2 )
		std::cout << "VRen2: Finished applying TPS" << std::endl;
}
#endif

bool VRen2::load_target( const char* filename )
{
	// Load volume data from disk	
	VTKPTR<vtkMetaImageReader> reader = VTKPTR<vtkMetaImageReader>::New();
	reader->SetFileName( filename );
	reader->Update();

	// Setup volume and isosurface
	m_volume1 .setInputConnection( reader->GetOutputPort() );
	m_contour1.setInputConnection( reader->GetOutputPort() );

	// Add to renderer
	m_renderer->AddVolume( m_volume1 .volume );
	m_renderer->AddActor ( m_contour1.contourActor );

	// get volume bounds
	double bounds[6];
	m_volume1.mapper->GetBounds( bounds );
	if( m_verbosity > 2 )
		std::cout << "VRen2: Target volume bounds = (" 
		          << bounds[0] << "-" << bounds[1] << "), " <<
		      "(" << bounds[2] << "-" << bounds[3] << "), " <<
		      "(" << bounds[4] << "-" << bounds[5] << ") " << std::endl;

	// Nice defaults visual comparison of two isosurfaces
	m_contour0.contourActor->GetProperty()->SetColor( 85./255, 1., 1.);
	m_contour0.contourActor->GetProperty()->SetOpacity( 0.7 );
	m_contour1.contourActor->GetProperty()->SetColor( 1., 115./255, 115./255);
	m_contour1.contourActor->GetProperty()->SetOpacity( 0.5 );

	return true;
}

bool VRen2::load_reference( const char* filename )
{
	// Load volume data from disk
	VTKPTR<vtkMetaImageReader> reader = VTKPTR<vtkMetaImageReader>::New();
	reader->SetFileName( filename );
	reader->Update();

	// Setup volume rendering
	m_volume0.setInputConnection( reader->GetOutputPort() );

	// Setup isosurface
	m_contour0.setInputConnection( reader->GetOutputPort() );

	// Setup renderer
	m_renderer->AddVolume( m_volume0 .volume );
	m_renderer->AddActor ( m_contour0.contourActor );
	m_renderer->SetViewport(  0, 0, 1, 1 );
	m_renderer->ResetCamera();

#if 0
	// setup transfer function (gray ramp)
	m_volume0.colorFunc->RemoveAllPoints();
	m_volume0.colorFunc->AddRGBPoint(   0.0, 0.0, 0.0, 0.0 );
	m_volume0.colorFunc->AddRGBPoint( 255.0, 1.0, 1.0, 1.0 );
		// --> FIXME: produces a crash?
#endif

	// get volume bounds
	m_volume0.mapper->GetBounds( m_bounds );
	if( m_verbosity > 2 )
		std::cout << "VRen2: Reference volume bounds = (" 
		          << m_bounds[0] << "-" << m_bounds[1] << "), " <<
		      "(" << m_bounds[2] << "-" << m_bounds[3] << "), " <<
		      "(" << m_bounds[4] << "-" << m_bounds[5] << ") " << std::endl;

	// Add oulines
	m_outline      ->SetInputConnection( reader   ->GetOutputPort() );
	m_outlineMapper->SetInputConnection( m_outline->GetOutputPort() );
	m_outlineActor ->SetMapper( m_outlineMapper );
	m_renderer     ->AddViewProp( m_outlineActor );

	// Add axes		
		//m_axes->SetInput( m_outline->GetOutput() );
		//m_axes->SetBounds( m_bounds );
	m_axes->SetBounds( m_bounds[0], m_bounds[1]+1,
	                   m_bounds[2], m_bounds[3]+1,
					   m_bounds[4], m_bounds[5]+1 );
	m_axes->SetCamera( m_renderer->GetActiveCamera() );
	m_axes->SetNumberOfLabels( 9 );
	m_renderer->AddViewProp( m_axes );

	// Add grid
	float xlength = (m_bounds[1]-m_bounds[0])+1;
	float ylength = (m_bounds[3]-m_bounds[2])+1;
	float zlength = (m_bounds[5]-m_bounds[4])+1;
	m_grid->SetXLength( xlength );
	m_grid->SetYLength( ylength );
	m_grid->SetZLength( zlength );
	m_grid->SetXCubes( 8 );
	m_grid->SetYCubes( 8 * (ylength/xlength) );
	m_grid->SetZCubes( 8 * (zlength/xlength) );
	m_grid->SetCenterGrid( false );
	m_gridMapper->SetInput( m_grid->GetOutput() );
	m_gridActor ->SetMapper( m_gridMapper );
	m_renderer->AddActor( m_gridActor );

	return true;
}

void VRen2::setVolumeVisibility( bool visible, int volid )
{	
	if( volid==1 )
		m_volume1.volume->SetVisibility( visible );
	else
		m_volume0.volume->SetVisibility( visible );
}

bool VRen2::getVolumeVisibility( int volid ) const
{	
	// REMARK
	// We use here "!!" to convert an int to bool to avoid Warning C4800 on 
	// VS Compiler. But havn't checked if produced ASM code is now really 
	// faster (most probably not), because this getters are surely not so 
	// performance critical.
	bool vis;
	if( volid==1 )
		vis = !!m_volume1.volume->GetVisibility();
	else
		vis = !!m_volume0.volume->GetVisibility();
	return vis;
}

void VRen2::setContourVisibility( bool visible, int volid )
{
	if( volid==1 )
		m_contour1.contourActor->SetVisibility( visible );
	else
		m_contour0.contourActor->SetVisibility( visible );
}

bool VRen2::getContourVisibility( int volid ) const
{
	bool vis;
	if( volid==1 )
		vis = !!m_contour1.contourActor->GetVisibility();
	else
		vis = !!m_contour0.contourActor->GetVisibility();
	return vis;
}


// Apply contour changes to both volumes:

void VRen2::setContourValue( double value )
{
	m_contour0.contour->SetValue( 0, value );
	m_contour1.contour->SetValue( 0, value );
}

double VRen2::getContourValue() const
{
	return m_contour0.contour->GetValue( 0 );
}

// Defined only with respect to reference volume:

void VRen2::setOutlineVisibility( bool visible )
{
	m_outlineActor->SetVisibility( visible );
}

bool VRen2::getOutlineVisibility() const
{
	return !!m_outlineActor->GetVisibility();
}

void VRen2::setAxesVisibility( bool visible )
{
	m_axes->SetVisibility( visible );
}

bool VRen2::getAxesVisibility() const
{
	return !!m_axes->GetVisibility();
}

void VRen2::setGridVisibility( bool visible )
{
	m_gridActor->SetVisibility( visible );
}

bool VRen2::getGridVisibility() const
{
	return !!m_gridActor->GetVisibility();
}

void VRen2::setGridSize( int size )
{
	double bounds[6];
	m_volume0.mapper->GetBounds( bounds );

	float xlength = (bounds[1]-bounds[0])+1;
	float ylength = (bounds[3]-bounds[2])+1;
	float zlength = (bounds[5]-bounds[4])+1;
	//m_grid->SetXLength( xlength );
	//m_grid->SetYLength( ylength );
	//m_grid->SetZLength( zlength );
	m_grid->SetXCubes( size );
	m_grid->SetYCubes( size * (ylength/xlength) );
	m_grid->SetZCubes( size * (zlength/xlength) );

	m_grid->Modified();
	m_grid->Update();
}

int VRen2::getGridSize() const
{
	return m_grid->GetXCubes();
}

#include <vtkOBJExporter.h>
void VRen2::exportOBJ( const char* fileprefix )
{	
	VTKPTR<vtkOBJExporter> exporter = VTKPTR<vtkOBJExporter>::New();	

	exporter->SetInput( m_renderer->GetRenderWindow() );
	exporter->SetFilePrefix( fileprefix );
	exporter->Write();
}


//-----------------------------------------------------------------------------
//	vtkext 3DGridSource
//-----------------------------------------------------------------------------

//vtkCxxRevisionMacro(vtkext3DGridSource, "$Revision: 1.3 $");
vtkStandardNewMacro(vtkext3DGridSource);

vtkext3DGridSource::vtkext3DGridSource(float xL, float yL, float zL)
#ifdef VTKEXT_3DGRIDSOURCE_SUPPORT_TOM
	: m_tomTpsTransform(0)
#endif
{
	this->XLength = fabs(xL);
	this->YLength = fabs(yL);
	this->ZLength = fabs(zL);
	
	this->Center[0] = 0.0;
	this->Center[1] = 0.0;
	this->Center[2] = 0.0;
	
	this->XCubes = 10;
	this->YCubes = 10;
	this->ZCubes = 10;

	this->CenterGrid = false;
}

void vtkext3DGridSource::Execute()
{
	vtkPolyData *output = this->GetOutput();
	
	vtkDebugMacro(<<"Creating 3D grid");

	vtkPoints *pts = vtkPoints::New();
	vtkCellArray *lines = vtkCellArray::New();

	// number of verts in grid
	const int ntx = this->XCubes+1;
	const int nty = this->YCubes+1;
	const int ntz = this->ZCubes+1;
	
	int nxa, nxb,
		nya, nyb,
		nza, nzb;
	if( CenterGrid )
	{
		// align grid center at origin
		const int nx2 = this->XCubes/2;
		const int ny2 = this->YCubes/2;
		const int nz2 = this->ZCubes/2;

		nxa = -nx2;  nxb = nx2+1;
		nya = -ny2;  nyb = ny2+1;
		nza = -nz2;  nzb = nz2+1;
	}
	else
	{
		// align grid lower left front with origin
		nxa = 0;  nxb = ntx;
		nya = 0;  nyb = nty;
		nza = 0;  nzb = ntz;
	}
	
	// side length of cube
	const double sx = (double)this->XLength/(double)this->XCubes;
	const double sy = (double)this->YLength/(double)this->YCubes;
	const double sz = (double)this->ZLength/(double)this->ZCubes;
	
	pts->SetNumberOfPoints(ntx*nty*ntz);
	// construct grid points
	for (int tx = nxa; tx < nxb; tx++)
		for (int ty = nya; ty < nyb; ty++)
			for (int tz = nza; tz < nzb; tz++)
			{
				// pointID of center point
				int pIDc = (tx-nxa) + (ty-nya) * ntx + (tz-nza) * ntx * nty;
				
				// start of cube
				double x = tx * sx + this->Center[0];
				double y = ty * sy + this->Center[1];
				double z = tz * sz + this->Center[2];

#ifdef VTKEXT_3DGRIDSOURCE_SUPPORT_TOM
				// HACK:
				// If a TPS transform is specified, apply it on grid vertices.
				if( m_tomTpsTransform )
				{
					double tmp[3], warped[3];
					tmp[0] = x;
					tmp[1] = y;
					tmp[2] = z;
					m_tomTpsTransform->warp( tmp, warped );
					x = warped[0];
					y = warped[1];
					z = warped[2];
				}
#endif				
				pts->SetPoint(pIDc, x, y, z);
			}

	// construct grid cells / lines
	for (int tx = nxa; tx < nxb; tx++)
		for (int ty = nya; ty < nyb; ty++)
			for (int tz = nza; tz < nzb; tz++)
			{
				// pointID of center point
				int pIDc = (tx-nxa) + (ty-nya) * ntx + (tz-nza) *ntx*nty;
				
				if (tx != nxb-1)
				{
					int pID2 = (tx-nxa+1) + (ty-nya) * ntx + (tz-nza) *ntx*nty;
					lines->InsertNextCell(2);
					lines->InsertCellPoint(pIDc);
					lines->InsertCellPoint(pID2);
				}
				if (ty != nyb-1)
				{
					int pID2 = (tx-nxa) + (ty+1-nya) * ntx + (tz-nza) *ntx*nty;
					lines->InsertNextCell(2);
					lines->InsertCellPoint(pIDc);
					lines->InsertCellPoint(pID2);
				}
				if (tz != nzb-1)
				{
					int pID2 = (tx-nxa) + (ty-nya) * ntx + (tz+1-nza) *ntx*nty;
					lines->InsertNextCell(2);
					lines->InsertCellPoint(pIDc);
					lines->InsertCellPoint(pID2);
				}
			}

	output->SetPoints(pts);
	pts->Delete();
	output->SetLines(lines);
	lines->Delete();
}

void vtkext3DGridSource::PrintSelf(ostream& os, vtkIndent indent)
{
	vtkPolyDataSource::PrintSelf(os,indent);
	
	os << indent << "X Length: " << this->XLength << "\n";
	os << indent << "Y Length: " << this->YLength << "\n";
	os << indent << "Z Length: " << this->ZLength << "\n";
	os << indent << "Center: (" << this->Center[0] << ", " 
		<< this->Center[1] << ", " << this->Center[2] << ")\n";
	os << indent << "X cubes: " << this->XCubes << "\n";
	os << indent << "Y cubes: " << this->YCubes << "\n";
	os << indent << "Z cubes: " << this->ZCubes << "\n";
}



//-----------------------------------------------------------------------------
//	WarpVis  ( StructuredGrid )
//-----------------------------------------------------------------------------


#ifdef VREN_SUPPORT_TOM
void WarpVis::setup( const tomTpsTransform& tps, int dimi, int dimj, int dimk )
{
	// We assume here our default voxel grid coordinate system
	// with origin (0,0,0), spacing (1,1,1) and dimensionality of voxel grid size.
	m_warp->SetDimensions( dimi, dimj, dimk );

	// Compute dense displacement field
	if( m_verbosity > 1 )
		std::cout << "WarpVis: Computing dense displacement field" << std::endl;
	tomTpsWarpfield( tps, m_warp, m_spacing, m_origin, m_stratify );

	setup( m_warp );
}
#endif

void WarpVis::setup( vtkStructuredGrid* warp )
{
	vtkMath::RandomSeed(time(NULL));

	m_warp = warp; // TODO: copy OK?

	if( m_type == Hedgehog )
	{
		// --- Hedgehog ---

		if( m_verbosity > 1 )
			std::cout << "WarpVis: Setting up Hedgehog visualization" << std::endl;

		m_hedgehog->SetInput( m_warp );
		m_hedgehog->Update();	
		m_hedgehog->SetScaleFactor( 2 );  // Difficulty: Choose approp. scaling factor
		
		// Color by scalar data (where displacement magnitude is stored)
		//m_mapper->SetLookuptTable( ?? )	

		// Create simple rendering pipeline
		m_mapper->SetInputConnection( m_hedgehog->GetOutputPort() );
	}
	else
	if( m_type == Glyphs )
	{
		// --- Oriented Glyphs ---

		if( m_verbosity > 1 )
			std::cout << "WarpVis: Setting up Oriented Glyphs visualization" << std::endl;

		m_arrow->SetTipResolution( 6 );
		m_arrow->SetTipRadius( 0.1  );
		m_arrow->SetTipLength( 0.35 );
		m_arrow->SetShaftResolution( 6 );
		m_arrow->SetShaftRadius( 0.03 );

		m_arrow->Update();

		m_glyph->SetInput( m_warp );
		m_glyph->SetSource( m_arrow->GetOutput() );
		m_glyph->SetVectorModeToUseVector();
		m_glyph->SetColorModeToColorByScalar();
		m_glyph->SetScaleModeToScaleByScalar();
		m_glyph->OrientOn();
		m_glyph->SetScaleFactor( 2 );  // TODO: make this user adjustable!
		
		m_glyph->Update();

		m_mapper->SetInput( m_glyph->GetOutput() );
	}		

	m_actor->SetMapper( m_mapper );

	// Override scalar coloring
	m_mapper->ScalarVisibilityOff();
	m_actor->GetProperty()->SetColor( 1,1,1 );

	if( m_verbosity > 1 )
		std::cout << "WarpVis: Setup finished." << std::endl;
}

//-----------------------------------------------------------------------------
//	WarpVis  ( PolyData )
//-----------------------------------------------------------------------------

#ifdef VREN_SUPPORT_TOM
void WarpVis::setup( const tomTpsTransform& tps, vtkPolyData* polydata )
{
	if( m_verbosity > 1 )
		std::cout << "WarpVis: Setting up on PolyData domain (experimental!)" << std::endl;

	// Put TPS displacements and magnitudes into polydata attributes
	tomTpsWarpfield( tps, (vtkPointSet*)polydata );

	if( m_type == Hedgehog )
	{
		// --- Hedgehog ---

		if( m_verbosity > 1 )
			std::cout << "WarpVis: Setting up Hedgehog visualization" << std::endl;

		m_hedgehog->SetInput( polydata );
		m_hedgehog->Update();	
		m_hedgehog->SetScaleFactor( 2 );  // Difficulty: Choose approp. scaling factor
		
		// Color by scalar data (where displacement magnitude is stored)
		//m_mapper->SetLookuptTable( ?? )	

		// Create simple rendering pipeline
		m_mapper->SetInputConnection( m_hedgehog->GetOutputPort() );
	}
	else
	if( m_type == Glyphs )
	{
		// --- Oriented Glyphs ---

		if( m_verbosity > 1 )
			std::cout << "WarpVis: Setting up Oriented Glyphs visualization" << std::endl;

		m_arrow->SetTipResolution( 6 );
		m_arrow->SetTipRadius( 0.1  );
		m_arrow->SetTipLength( 0.35 );
		m_arrow->SetShaftResolution( 6 );
		m_arrow->SetShaftRadius( 0.03 );

		m_arrow->Update();

		m_glyph->SetInput( polydata );
		m_glyph->SetSource( m_arrow->GetOutput() );
		m_glyph->SetVectorModeToUseVector();
		m_glyph->SetColorModeToColorByScalar();
		m_glyph->SetScaleModeToScaleByScalar();
		m_glyph->OrientOn();
		m_glyph->SetScaleFactor( 2 );  // TODO: make this user adjustable!
		
		m_glyph->Update();

		m_mapper->SetInput( m_glyph->GetOutput() );
	}		

	m_actor->SetMapper( m_mapper );

	// Override scalar coloring
	m_mapper->ScalarVisibilityOff();
	m_actor->GetProperty()->SetColor( 1,1,1 );

	if( m_verbosity > 1 )
		std::cout << "WarpVis: Setup finished." << std::endl;
}
#endif


/**
	\code
	// BUGBUG:
	// This is currently not working, since i tried to put the scalar data of 
	// the volume directly into the scalar data of the grid. Obvious bogus!
	// What we have to do is to iterate again over all points in the grid and
	// evaluate the image data scalar value at the corresponding coordinate
	// (with appropriate interpolation) and insert *that* value as scalar
	// attribute.
	\endcode
*/
void WarpVis::setOpacityToVolumeIntensity( vtkImageData* vol )
{

	if( m_verbosity > 1 )
		std::cout << "WarpVis: Setting opacity to volume intensity (experimental!)" << std::endl;

	// Overwrite previous point data (TPS field magnitude)
	m_warp->GetPointData()->SetScalars( (vtkDataArray*)(vol->GetPointData()->GetScalars()) );

	// Opacity lookup
	vtkLookupTable* color = vtkLookupTable::New();
	color->SetRange( 0, 255 );  // TODO: adjust opacity range to real data
	color->SetAlphaRange( 1.0, 1.0 );
	color->SetSaturationRange(1,1);
	color->SetHueRange(0,1);
	//color->SetValueRange(1,1); // ??
	color->Build();

	//vtkImageMapToColors* map = vtkImageMapToColors::New();
	//map->SetInput( ... ? ... )

	// Individual adjustments
	if( m_type == Glyphs )
	{
		// Scale by magnitude of vectors (should be the same result as before)
		m_glyph->SetScaleModeToScaleByVector();
	}
	else
	if( m_type == Hedgehog )
	{
		// no adjustments here yet :-)
	}

	m_mapper->SetLookupTable( color );
	m_mapper->ScalarVisibilityOn();
	m_mapper->Update();

	color->Delete();
	//map->Delete();
}


//-----------------------------------------------------------------------------
//	WarpVolumeVis
//-----------------------------------------------------------------------------

#ifdef VREN_SUPPORT_TOM
void WarpVolumeVis::setup( const tomTpsTransform& tps, int dimi, int dimj, int dimk )
{
	// Setup image
	if( m_verbosity > 1 )
		std::cout << "WarpVolumeVis: Setup image volume" << std::endl;
	m_mag->SetDimensions( dimi, dimj, dimk );
	m_mag->SetSpacing( 1,1,1 );
	m_mag->SetOrigin( 0,0,0 );
	
	// Compute dense displacement field
	if( m_verbosity > 1 )
		std::cout << "WarpVolumeVis: Computing dense displacement field" << std::endl;
	float min, max;
	tomTpsWarpfield( tps, m_mag, min, max, false );  // no vector data, just set scalars
}
#endif

void WarpVolumeVis::setup( vtkImageData* warp_magnitude )
{
	m_mag = warp_magnitude;

	// HACK: min/max hardcoded
	float min =   0.0,
		  max = 100.0;

	// Adjust transfer function
	m_volvis.opacityFunc->RemoveAllPoints();
	m_volvis.opacityFunc->AddPoint( min, 0 );
	m_volvis.opacityFunc->AddPoint( max, 0.05 );
	m_volvis.colorFunc->RemoveAllPoints();
	m_volvis.colorFunc->AddRGBPoint( min, 0,0,1 );
	m_volvis.colorFunc->AddRGBPoint( max, 1,0,0 );

	if( m_verbosity > 1 )
		std::cout << "WarpVolumeVis: Setting up volume visualization" << std::endl;
	m_volvis.setInput( m_mag );

	if( m_verbosity > 1 )
		std::cout << "WarpVolumeVis: Setup finished." << std::endl;
}


//-----------------------------------------------------------------------------
// 	tomSetupStratifiedGrid
//-----------------------------------------------------------------------------

void tomSetupStratifiedGrid( vtkStructuredGrid* grid, int* dim, 
                             const double* spacing, const double* origin, 
							 bool stratify )
{
	grid->SetDimensions( dim );
	
	// allocate points
	vtkPoints* points = vtkPoints::New();
	points->Allocate( dim[0]*dim[1]*dim[2] );	

	double u[3], // voxel coordinate
	       v[3]; // stratified coordinate
	
	for( int k=0, index=0; k < dim[2]; ++k )
	{
		u[2] = origin[2] + k*spacing[2];
		
		for( int j=0; j < dim[1]; ++j )
		{
			u[1] = origin[1] + j*spacing[1];
			
			for( int i=0; i < dim[0]; ++i, ++index )
			{				
				u[0] = origin[0] + i*spacing[0];
				
				if( stratify )
				{
					v[0] = vtkMath::Gaussian( u[0], .5*spacing[0] );
					v[1] = vtkMath::Gaussian( u[1], .5*spacing[1] );
					v[2] = vtkMath::Gaussian( u[2], .5*spacing[2] );
				}
				else
				{
					v[0] = u[0];
					v[1] = u[1];
					v[2] = u[2];
				}
				
				points->InsertPoint( index, v );	
			}
		}
	}

	grid->SetPoints( points );
	points->Delete();
}



//-----------------------------------------------------------------------------
// VectorfieldGlyphVisualization2 (translated from myvis.py python class)
//-----------------------------------------------------------------------------

#include <vtkFieldDataToAttributeDataFilter.h>
#include <vtkOutlineFilter.h>
#include <vtkPointSource.h>
#include <vtkProbeFilter.h>

template<class T>
T maxv( T* v, int n )
{
	T _max=v[0];
	for( int i=1; i < n; ++i )
		if( v[i] > _max ) _max = v[i];
	return _max;
}

VTKPTR<vtkActor> VectorfieldGlyphVisualization2::createGlyphVisActor( vtkAlgorithm* algo )
{
	VTKPTR<vtkArrowSource>    arrow  = VTKPTR<vtkArrowSource>   ::New();
	VTKPTR<vtkGlyph3D>        glyph  = VTKPTR<vtkGlyph3D>       ::New();
	VTKPTR<vtkPolyDataMapper> mapper = VTKPTR<vtkPolyDataMapper>::New();

    glyph->SetInputConnection( algo->GetOutputPort() );
    glyph->SetSourceConnection( arrow->GetOutputPort() );
    glyph->SetVectorModeToUseVector();
    glyph->SetColorModeToColorByVector();
    glyph->SetScaleModeToScaleByVector();
    glyph->OrientOn();
    glyph->SetScaleFactor(0.16);
    glyph->Update();
    
    mapper->SetInputConnection( glyph->GetOutputPort() );
    mapper->ScalarVisibilityOn();
    mapper->SetScalarRange( 0,1 ); // source->GetOutput()->GetScalarRange()
    
	// store as members to allow later adjustments e.g. SetScaleFactor()
	m_glyph = glyph;
	m_glyphMapper = mapper;

	VTKPTR<vtkActor> actor = VTKPTR<vtkActor>::New();
    actor->SetMapper( mapper );
    return actor;
}

vtkActor* VectorfieldGlyphVisualization2::createPolyActor( vtkAlgorithm* algo )
{
	VTKPTR<vtkPolyDataMapper> mapper = VTKPTR<vtkPolyDataMapper>::New();
	VTKPTR<vtkActor>          actor  = VTKPTR<vtkActor>         ::New();
	mapper->SetInputConnection( algo->GetOutputPort() );
	actor->SetMapper( mapper );
	return actor;
}

vtkActor* VectorfieldGlyphVisualization2::createOutlineActor2( vtkImageData* data )
{
	VTKPTR<vtkOutlineFilter> outline = VTKPTR<vtkOutlineFilter>::New();
	outline->SetInput( data );
	return createPolyActor( outline );
}

VectorfieldGlyphVisualization2::VectorfieldGlyphVisualization2
	( vtkImageData* source, int numPts )
{
	int    extent[6];
	double spacing[3];
	source->GetExtent ( extent  );
	source->GetSpacing( spacing );

	// derive sampling domain from source extent
	extent[1] *= spacing[0];  // HACK: assumes origin at (0,0,0)
	extent[3] *= spacing[1];
	extent[5] *= spacing[2];

	// random sampling (similar to "mask points" option in ParaView glyph filter)
	// note that spherical sampling is not optimal
	VTKPTR<vtkPointSource> samples = VTKPTR<vtkPointSource>::New();
	samples->SetNumberOfPoints( numPts );
	samples->SetCenter( extent[1]/2, extent[3]/2, extent[5]/2 );
	samples->SetRadius( maxv(extent,6)/2. );
	samples->Update();

	VTKPTR<vtkProbeFilter> probe = VTKPTR<vtkProbeFilter>::New();
	probe->SetInputConnection( samples->GetOutputPort() );
	probe->SetSource( source );
	probe->Update();

	// set input scalars as a vector attribute
	VTKPTR<vtkFieldDataToAttributeDataFilter> fd2ad = VTKPTR<vtkFieldDataToAttributeDataFilter>::New();
	fd2ad->SetInput( probe->GetOutput() );
	fd2ad->SetInputFieldToPointDataField();
	fd2ad->SetOutputAttributeDataToPointData();
	const char* scan = "MetaImage";
	fd2ad->SetVectorComponent(0,scan,0);
	fd2ad->SetVectorComponent(1,scan,1);
	fd2ad->SetVectorComponent(2,scan,2);
	fd2ad->Update();

	m_visActor     = createGlyphVisActor( fd2ad.GetPointer() );
	//m_polyActor    = createPolyActor    ( probe.GetPointer() );
	//m_outlineActor = createOutlineActor2( source);

	// vectorfield min/max magnitude
	float mmin,mmax;
	computeImageDataMagnitude( source, &mmin, &mmax );

	// color lookup table
	vtkScalarsToColors* lut = NULL;
#if 0
	llut = VTKPTR<vtkLookupTable>::New();
	llut->SetTableRange( mmin, mmax );
	llut->SetHueRange( 1./6., 1. );
	llut->SetSaturationRange( 1, 1 );
	llut->SetValueRange( 1, 1 );
	llut->Build();
	lut = llut.GetPointer();
#else
	VTKPTR<vtkColorTransferFunction> hlut = VTKPTR<vtkColorTransferFunction>::New();
	hlut->AddRGBPoint( mmin, 0,0,1 );
	hlut->AddRGBPoint( mmax, 1,0,0 );
	hlut->HSVWrapOff();
	hlut->SetColorSpaceToHSV();
	lut = hlut.GetPointer();
#endif

	// adjust mapper
	m_glyphMapper->SetScalarRange( mmin,mmax );
	m_glyphMapper->SetLookupTable( lut );

	// setup scalar bar
	m_scalarBar = VTKPTR<vtkScalarBarActor>::New();
	m_scalarBar->SetLookupTable( lut );
	m_scalarBar->SetTitle("Magnitude");
	m_scalarBar->SetNumberOfLabels( 4 );
	m_scalarBar->GetTitleTextProperty()->SetFontSize( 6 );
	m_scalarBar->GetTitleTextProperty()->ItalicOff();
	m_scalarBar->GetLabelTextProperty()->SetFontSize( 2 );
	m_scalarBar->GetLabelTextProperty()->ItalicOff();
	//m_scalarBar->SetMaximumWidthInPixels( 120 );
	//m_scalarBar->SetMaximumHeightInPixels( 600 );
}


VTKPTR<vtkImageData> loadImageData( const char* filename )
{
	// Load volume data from disk
	VTKPTR<vtkMetaImageReader> reader = VTKPTR<vtkMetaImageReader>::New();
	reader->SetFileName( filename );
	reader->Update();

	// Setup volume rendering
	VTKPTR<vtkImageData> img = reader->GetOutput();
	return img;
}

#include <limits>

void computeImageDataMagnitude( vtkImageData* img, float* mag_min_, float* mag_max_ )
{
	assert(img);

	int dims[3];
	int ncomp = img->GetNumberOfScalarComponents();
	img->GetDimensions( dims );

	float mag_max = -std::numeric_limits<float>::max(),
		  mag_min = +std::numeric_limits<float>::max();

	for( int z=0; z < dims[2]; ++z )
		for( int y=0; y < dims[1]; ++y )
			for( int x=0; x < dims[0]; ++x )
			{
				float sum=0.f;
				for( int c=0; c < ncomp; ++c )
				{
					float s = img->GetScalarComponentAsFloat( x,y,z, c );
					sum += s*s;
				}
				float mag = sqrt(sum);
				if( mag > mag_max ) mag_max = mag;
				if( mag < mag_min ) mag_min = mag;
			}

	std::cout << "computeImageDataMagnitude\n" 
		      << "min=" << mag_min << ", max=" << mag_max << "\n";

	*mag_min_ = mag_min;
	*mag_max_ = mag_max;
}
