#include "GlyphVisualization.h"
#include "VectorToMeshColorFilter.h"
#include "VectorToVertexNormalFilter.h"
#include "GlyphInvertFilter.h"
#include "GlyphOffsetFilter.h"
#include "ColorMapRGB.h"

#include <vtkProbeFilter.h>
#include <vtkArrowSource.h>
#include <vtkColorTransferFunction.h>
#include <vtkLookupTable.h>
#include <vtkTextProperty.h>
#include <vtkProperty.h>
#include <vtkFieldDataToAttributeDataFilter.h>
#include <vtkPointData.h>


/// Extract maximum component of a vector
template<class T>
T maxv( T* v, int n )
{
	T _max=v[0];
	for( int i=1; i < n; ++i )
		if( v[i] > _max ) _max = v[i];
	return _max;
}

//-----------------------------------------------------------------------------
//  c'tor / d'tor
//-----------------------------------------------------------------------------
GlyphVisualization
  ::GlyphVisualization( vtkImageData*			source, 
						vtkPolyData*			mesh, 
						std::vector<vtkPolyData*>	ref_meshes, 
						vtkPolyData*			pointSamples,
						bool invertVectors )
: m_scalarBar              (VTKPTR<vtkScalarBarActor >::New()),
  m_scalarBarVector        (VTKPTR<vtkScalarBarActor >::New()),
  m_lutTangentialComponent (VTKPTR<vtkScalarsToColors>::New()),
  m_lutNormalComponent     (VTKPTR<vtkScalarsToColors>::New()),
  m_clusterData( NULL )
{
	// Set some defaults
	m_glyphSize        = 0.5;
	m_warpScale        = 0.2;
	m_autoScaleEnabled = true;
	m_autoScaleFactor  = 4.5; //was: 6.3;	
	m_invertVectors    = invertVectors;
	
	// Setup probe filter
	mesh->Update();  // Make sure the mesh is already loaded in memory
	VTKPTR<vtkProbeFilter> probe = VTKPTR<vtkProbeFilter>::New();
	probe->SetInput( mesh );
	probe->SetSource( source );
	probe->Update();

	internal_create( source, probe->GetOutput(), mesh, ref_meshes,pointSamples );
	// was: internal_create( source, probe->GetOutput() );
}

GlyphVisualization
  ::~GlyphVisualization()
{}

//-----------------------------------------------------------------------------
//  createGlyphVisActor2( vtkPolyData* )
//-----------------------------------------------------------------------------
VTKPTR<vtkActor> GlyphVisualization
  ::createGlyphVisActor2( vtkPolyData * polyData )
{
	VTKPTR<vtkArrowSource>    arrow  = VTKPTR<vtkArrowSource>   ::New();
	VTKPTR<vtkGlyph3D>        glyph  = VTKPTR<vtkGlyph3D>       ::New();
	VTKPTR<vtkPolyDataMapper> mapper = VTKPTR<vtkPolyDataMapper>::New();	
	
	glyph->SetInput(polyData);
    glyph->SetSourceConnection( arrow->GetOutputPort() );
    glyph->SetVectorModeToUseVector();
	glyph->SetColorModeToColorByVector();
    glyph->OrientOn();
    glyph->Update();

	arrow->SetTipLength( 1.0 );
	//arrow->SetTipRadius( 0.23 );
	arrow->SetTipResolution( 50 );
	
	// create lookUpTable for arrow Colors
#if 1
	// Use LookupTable
	VTKPTR<vtkLookupTable> hlut = VTKPTR<vtkLookupTable>::New();
	//hlut->SetHueRange(4./6.,0); // rainbow-like
	//hlut->SetHueRange( 300.0/360., 300.0/360. ); // magenta
	//hlut->SetSaturationRange( 0.0, 0.7 );

	// gray
	hlut->SetHueRange( 0., 0. ); 
	hlut->SetSaturationRange( 0.0, 0.0 );
	hlut->SetValueRange( 0.3, 0.3 );
	hlut->SetAlphaRange( 1.0, 1.0 );
	hlut->SetRange( 0.0, m_maxOrthVector );
	hlut->Build();
#else
	// Use ColorTransferFunction (incompatible with ColorMapRGB)
	VTKPTR<vtkColorTransferFunction> hlut = VTKPTR<vtkColorTransferFunction>::New();
  #if 0
	// Poettkow probability
	double step = m_maxOrthVector / 16.0;
	hlut->AddRGBPoint(  0*step, 0.3137,    0.6784,    0.9020 );
	hlut->AddRGBPoint(  1*step, 0.4118,    0.5608,    0.8235 ); 
	hlut->AddRGBPoint(  2*step, 0.5176,    0.4314,    0.7255 );
	hlut->AddRGBPoint(  3*step, 0.6235,    0.3020,    0.6392 );
	hlut->AddRGBPoint(  4*step, 0.7294,    0.1725,    0.5490 );
	hlut->AddRGBPoint(  5*step, 0.8392,    0.0431,    0.4549 );
	hlut->AddRGBPoint(  6*step, 0.8902,    0.0902,    0.3725 );
	hlut->AddRGBPoint(  7*step, 0.9137,    0.2353,    0.2941 );
	hlut->AddRGBPoint(  8*step, 0.9373,    0.3843,    0.2118 );
	hlut->AddRGBPoint(  9*step, 0.9608,    0.5294,    0.1333 );
	hlut->AddRGBPoint( 10*step, 0.9804,    0.6745,    0.0588 );
	hlut->AddRGBPoint( 11*step, 1.0000,    0.7922,    0.0431 );
	hlut->AddRGBPoint( 12*step, 1.0000,    0.8353,    0.2392 );
	hlut->AddRGBPoint( 13*step, 1.0000,    0.8745,    0.4314 );
	hlut->AddRGBPoint( 14*step, 1.0000,    0.9176,    0.6196 );
	hlut->AddRGBPoint( 15*step, 1.0000,    0.9608,    0.8314 );
	hlut->AddRGBPoint( 16*step, 1.0000,    1.0000,    1.0000 );
	hlut->SetColorSpaceToRGB();
	hlut->HSVWrapOff();
  #else
	// Paraview "erdc_red2yellow_BW" hot colormap
	double step = m_maxOrthVector / 19.0;
	hlut->AddRGBPoint(  0*step,   0./255.,   0./255.,   0./255. );
	hlut->AddRGBPoint(  1*step,  46./255.,   6./255.,   0./255. );
	hlut->AddRGBPoint(  2*step,  78./255.,   0./255.,   0./255. );
	hlut->AddRGBPoint(  3*step, 108./255.,   0./255.,   0./255. );
	hlut->AddRGBPoint(  4*step, 133./255.,   0./255.,   0./255. );
	hlut->AddRGBPoint(  5*step, 155./255.,   0./255.,   0./255. );
	hlut->AddRGBPoint(  6*step, 172./255.,  36./255.,   0./255. );
	hlut->AddRGBPoint(  7*step, 186./255.,  62./255.,   0./255. );
	hlut->AddRGBPoint(  8*step, 199./255.,  85./255.,   0./255. );
	hlut->AddRGBPoint(  9*step, 211./255., 108./255.,   0./255. );
	hlut->AddRGBPoint( 10*step, 211./255., 108./255.,   0./255. );
	hlut->AddRGBPoint( 11*step, 219./255., 132./255.,   0./255. );
	hlut->AddRGBPoint( 12*step, 225./255., 155./255.,   0./255. );
	hlut->AddRGBPoint( 13*step, 233./255., 178./255.,   0./255. );
	hlut->AddRGBPoint( 14*step, 233./255., 178./255.,   0./255. );
	hlut->AddRGBPoint( 15*step, 240./255., 199./255.,  57./255. );
	hlut->AddRGBPoint( 16*step, 247./255., 219./255., 121./255. );
	hlut->AddRGBPoint( 17*step, 252./255., 238./255., 192./255. );
	hlut->AddRGBPoint( 18*step, 252./255., 238./255., 192./255. );
	hlut->AddRGBPoint( 19*step, 255./255., 255./255., 255./255. );
	hlut->SetColorSpaceToLab();

	//// Some hot colormap
	//hlut->AddRGBPoint( 0.0              , 0,0,0 ); // was: m_minOrthVector
	//hlut->AddRGBPoint( m_maxOrthVector/3., 1,.3,.3 );
	//hlut->AddRGBPoint( m_maxOrthVector  , 1,1,1 );
  #endif
	hlut->Build();
#endif

    mapper->SetInputConnection( glyph->GetOutputPort() );
	mapper->SetScalarVisibility( 1 ); // color by scalars?
    //mapper->SetScalarRange( 0,1 ); // source->GetOutput()->GetScalarRange()
	mapper->SetScalarRange( 0.0, m_maxOrthVector );

	if( m_autoScaleEnabled && (m_maxOrthVector>0.0001) )
	{
		glyph->SetScaleFactor( m_autoScaleFactor / m_maxOrthVector );
	}
	else
	{
		glyph->SetScaleFactor( m_glyphSize );
	}

#if 1
	mapper->SetLookupTable(hlut);
#else
	// Really f**ed of it, so simply hard-coded the colormap here!
	ColorMapRGB colorMapRGB;
	colorMapRGB.read("G:\\Projects\\SPP1335\\sdmvis\\trunk\\tensorvis\\data\\colormaps\\Poettkow-Probability.map");
	colorMapRGB.applyTo( mapper->GetLookupTable() );	
	mapper->GetLookupTable()->SetRange( 0.0, m_maxOrthVector );
#endif

	// setup scalar bar
	m_scalarBarVector->SetLookupTable( mapper->GetLookupTable() );
	m_scalarBarVector->SetUseOpacity( 1 );
	
	// title
	m_scalarBarVector->SetTitle("Tangential comp. and magnitude");
	m_scalarBarVector->GetTitleTextProperty()->SetFontSize( 10 );
	m_scalarBarVector->GetTitleTextProperty()->SetBold( 0 );
	m_scalarBarVector->GetTitleTextProperty()->SetItalic( 0 );
	m_scalarBarVector->GetTitleTextProperty()->SetColor( 1,1,1 );

	// labels
	m_scalarBarVector->SetNumberOfLabels( 2 );
	m_scalarBarVector->GetLabelTextProperty()->SetFontSize( 10 );
	m_scalarBarVector->GetLabelTextProperty()->SetBold( 0 );
	m_scalarBarVector->GetLabelTextProperty()->SetItalic( 0 );
	m_scalarBarVector->GetLabelTextProperty()->SetColor( 1,1,1 );
	
	m_scalarBarVector->SetOrientationToHorizontal();

#if 0
	// absolute size
	m_scalarBarVector->SetMaximumWidthInPixels( 1024 );
	m_scalarBarVector->SetMaximumHeightInPixels( 80 );
	m_scalarBarVector->SetHeight(50);
#else
	// relative size
	m_scalarBarVector->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
	m_scalarBarVector->GetPositionCoordinate()->SetValue(0.2, 0.9); // was: 0.1, 0.9
	m_scalarBarVector->SetOrientationToHorizontal();
	m_scalarBarVector->SetWidth(0.6); // was: 0.8
	m_scalarBarVector->SetHeight(0.1); // was: 0.5
#endif
	
	// store as members to allow later adjustments e.g. SetScaleFactor()
	m_glyph = glyph;
	m_glyphMapper = mapper;
	m_lutTangentialComponent = mapper->GetLookupTable(); // was: hlut;

	VTKPTR<vtkActor> actor = VTKPTR<vtkActor>::New();
    actor->SetMapper( mapper );
#if 1
	mapper->SetScalarVisibility( 0 );
	mapper->SetColorModeToDefault();
	actor->GetProperty()->SetColor(  0.1, 0.1, 0.1 );

	//// Phong lighting for arrows
	//actor->GetProperty()->SetColor( 0.2, 0.2, 0.23 );
	//actor->GetProperty()->SetSpecular( 1.0 );
	//actor->GetProperty()->SetSpecularPower( 42.0 );
	//actor->GetProperty()->SetSpecularColor( 0.9, 0.9, 1.0 );
	//actor->GetProperty()->SetInterpolationToPhong();
#endif
	return actor;
}

//-----------------------------------------------------------------------------
//  createGlyphVisActor2( vtkAlgorithm* )
//-----------------------------------------------------------------------------
VTKPTR<vtkActor> GlyphVisualization
  ::createGlyphVisActor2( vtkAlgorithm* algo )
{
	VTKPTR<vtkArrowSource>    arrow  = VTKPTR<vtkArrowSource>   ::New();
	VTKPTR<vtkGlyph3D>        glyph  = VTKPTR<vtkGlyph3D>       ::New();
	VTKPTR<vtkPolyDataMapper> mapper = VTKPTR<vtkPolyDataMapper>::New();

	glyph->SetInputConnection( algo->GetOutputPort() );
    glyph->SetSourceConnection( arrow->GetOutputPort() );
    glyph->SetVectorModeToUseVector();
    glyph->OrientOn();
	//glyph->SetScaleFactor(m_glyphSize);
    glyph->Update();
	
    mapper->SetInputConnection( glyph->GetOutputPort() );
	mapper->SetScalarVisibility( 0 ); // color by scalars?
    
	// store as members to allow later adjustments e.g. SetScaleFactor()
	m_glyph = glyph;
	m_glyphMapper = mapper;

	VTKPTR<vtkActor> actor = VTKPTR<vtkActor>::New();
    actor->SetMapper( mapper );
	actor->GetProperty()->SetColor(1,0,1);
	actor->GetProperty()->SetOpacity(0.2);

	return actor;
}

//-----------------------------------------------------------------------------
//  internal_create( vtkAlgorithm* )
//-----------------------------------------------------------------------------
void GlyphVisualization
  ::internal_create( vtkImageData* source,
					 vtkDataSet*   samples,
					 vtkPolyData*  mesh,
					 std::vector<vtkPolyData*> ref_meshes,
					 vtkPolyData*  pointSamples)
{	
	// set input scalars as a vector attribute
	VTKPTR<vtkFieldDataToAttributeDataFilter> fd2ad_deformMesh = VTKPTR<vtkFieldDataToAttributeDataFilter>::New();
	fd2ad_deformMesh->SetInput( samples );
	fd2ad_deformMesh->SetInputFieldToPointDataField();
	fd2ad_deformMesh->SetOutputAttributeDataToPointData();	
	// WARNING: Assume that vectorfield data is given in array[0] of vtkImageData!	
	fd2ad_deformMesh->SetVectorComponent(0,source->GetPointData()->GetArrayName(0),0);
	fd2ad_deformMesh->SetVectorComponent(1,source->GetPointData()->GetArrayName(0),1);
	fd2ad_deformMesh->SetVectorComponent(2,source->GetPointData()->GetArrayName(0),2);
	fd2ad_deformMesh->Update();

	m_vectorField = fd2ad_deformMesh->GetPolyDataOutput();
	
	/*
	// generate warp Data 
	int count=0;
	for (int iB=0;iB<ref_meshes.size();iB++)
	{
		VTKPTR<vtkDoubleArray> warpData =   VTKPTR<vtkDoubleArray>::New();
		warpData->SetNumberOfComponents(3);
		warpData->SetName("warpData");
		for (int iA=0;iA<fd2ad_deformMesh->GetPolyDataOutput()->GetNumberOfPoints();iA++)
		{
			double strength=0;
			if (count==0)
				strength=-1;
			if (count==1)
				strength=1;
			
			double *warp = fd2ad_deformMesh->GetPolyDataOutput()->GetPointData()->GetVectors()->GetTuple3(iA);
			for (int iA=0;iA<3;iA++)
				warp[iA]=strength*m_warpScale*warp[iA];
			warpData->InsertNextTuple(warp);
		}
		// this generated warpdata have to be appendet to each mesh
		ref_meshes[count]->GetPointData()->AddArray(warpData);
		ref_meshes[count]->GetPointData()->SetActiveVectors("warpData");

		// genereate the deformed meshes 
		VTKPTR<vtkWarpVector> warpVector = VTKPTR<vtkWarpVector>::New();
		warpVector->SetInput(ref_meshes[count]);
		warpVector->Update();
		ref_meshes[count]->ShallowCopy(warpVector->GetPolyDataOutput());
		
		count++;
	}
	*/

	// generate colors for mesh with warpfield data 
	VTKPTR<VectorToMeshColorFilter> v2mc = VTKPTR<VectorToMeshColorFilter>::New();
	v2mc->SetInput( 0, fd2ad_deformMesh->GetPolyDataOutput() );
	v2mc->SetInput( 1, mesh );
	v2mc->Update(); // execute once to get min/max range for lookup table
	mesh->ShallowCopy( v2mc->GetOutput() );

	// Setup lookup table
	VTKPTR<vtkColorTransferFunction> hlut = VTKPTR<vtkColorTransferFunction>::New();
#if 0
	// Classic blue-to-yellow
	hlut->AddRGBPoint( mmin,  10./255.,  10./255., 242./255. );
	hlut->AddRGBPoint( mmin, 242./255., 242./255.,  10./255. );
	hlut->HSVWrapOff();
	hlut->SetColorSpaceToRGB();
#endif
#if 0
	// Custom blue-white-green
	hlut->AddRGBPoint( mmax, 0,1,0 );
	hlut->AddRGBPoint( mmax/2, 0.72,1,0.28 );
	hlut->AddRGBPoint( 0.1, .6,.6,.6 ); 
	hlut->AddRGBPoint( -0.1, .6,.6,.6 ); 
	hlut->AddRGBPoint( mmin/2, 0.28,0.87,1);
	hlut->AddRGBPoint( mmin, 0,0,1 );	
	hlut->HSVWrapOff();
	hlut->SetColorSpaceToRGB();
#endif
#if 1
	// Custom cool-to-warm shading (ParaView-like)
	double min = v2mc->GetMin(),
		   max = v2mc->GetMax();

	//min = -15.0;
	//max = +15.0;

	hlut->AddRGBPoint( min, 58./255., 76./255., 193./255. );	
	hlut->AddRGBPoint( -0.1, .95,.95,.95 );  // grey at center
	hlut->AddRGBPoint(  0.1, .95,.95,.95 ); 
	hlut->AddRGBPoint( max, 180./255., 4./255., 38./255 );
	hlut->HSVWrapOff();
	hlut->SetColorSpaceToDiverging();
#endif
	m_lutNormalComponent = hlut;

	v2mc->SetLookupTable( hlut );
	v2mc->Update();  // update needed because of changed lookup table
		// FIXME: Avoid a second call to v2mc->Update(), this is expensive!

	// generate new samples for the the sampling points 
	VTKPTR<vtkProbeFilter> probe = VTKPTR<vtkProbeFilter>::New();
	probe->SetInput(pointSamples );
	probe->SetSource( source );
	probe->Update();

	// sample the field
	VTKPTR<vtkFieldDataToAttributeDataFilter> fd2ad = VTKPTR<vtkFieldDataToAttributeDataFilter>::New();
	fd2ad->SetInput( probe->GetOutput());
	fd2ad->SetInputFieldToPointDataField();
	fd2ad->SetOutputAttributeDataToPointData();
	// WARNING: Assume that vectorfield data is given in array[0] of vtkImageData!	
	fd2ad->SetVectorComponent(0,source->GetPointData()->GetArrayName(0),0);
	fd2ad->SetVectorComponent(1,source->GetPointData()->GetArrayName(0),1);
	fd2ad->SetVectorComponent(2,source->GetPointData()->GetArrayName(0),2);
	fd2ad->Update();

	// generate vertexNormal stuff on reduced Data 
	VTKPTR<VectorToVertexNormalFilter> v2vn = VTKPTR<VectorToVertexNormalFilter>::New();
	v2vn->SetInput( 0, fd2ad->GetPolyDataOutput() );
	v2vn->SetInput( 1, pointSamples );
	v2vn->Update();

	fd2ad->GetPolyDataOutput()->ShallowCopy( v2vn->GetOutput() );
	m_minOrthVector = v2vn->getMin();
	m_maxOrthVector = v2vn->getMax();
	
	// set Vectors to pointsSample Data 
	pointSamples->GetPointData()->SetVectors( v2vn->GetOutput()->GetPointData()->GetVectors() );
	
	// Invert direction of vectors in vectorfield
	vtkPolyData* myData = pointSamples;	
	if( m_invertVectors )
	{
		VTKPTR<GlyphInvertFilter> invertFilter = VTKPTR<GlyphInvertFilter>::New();
		invertFilter->SetInput(pointSamples);
		invertFilter->Update();
		myData = invertFilter->GetOutput();
	}

	// Offset glyph position +2 from the surface in direction of the normal
	VTKPTR<GlyphOffsetFilter> offsetFilter = VTKPTR<GlyphOffsetFilter>::New();
	offsetFilter->SetOffset( 2 );
	offsetFilter->SetInput( myData );
	offsetFilter->Update();	

	// create the actor for Visualization
	m_visActor = createGlyphVisActor2( offsetFilter->GetOutput() );

	if( m_clusterData )
		m_clusterData->Delete();
	m_clusterData  = vtkPolyData::New();
	m_clusterData->ShallowCopy(	offsetFilter->GetOutput() );

	// adjust mapper
	//m_glyphMapper->SetScalarRange( v2mc->getMin(),v2mc->getMax() );
	//m_glyphMapper->SetLookupTable( lut );
	
	// setup scalar bar
	m_scalarBar->SetLookupTable( hlut );
	
	// title
	m_scalarBar->SetTitle("Surface orthogonal component");
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

	m_scalarBar->SetOrientationToHorizontal();
#if 0
	// absolute size
	m_scalarBar->SetMaximumWidthInPixels( 1024 );
	m_scalarBar->SetMaximumHeightInPixels( 80 );
	m_scalarBar->SetHeight(50);
#else
	// relative size
	m_scalarBar->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
	m_scalarBar->SetOrientationToHorizontal();

  #if 1 // see also TensorVisBase.cpp
	m_scalarBar->GetPositionCoordinate()->SetValue(0.2, 0.02);
	m_scalarBar->SetWidth(0.6);
	m_scalarBar->SetHeight(0.1);
  #else
	m_scalarBar->SetPosition(0.32, 0.02);
	m_scalarBar->SetWidth(0.36); 
	m_scalarBar->SetHeight(0.063);
  #endif
#endif

	// deformation at last because we need the rigth deformation values from 
	// original mesh for coloring and parallel vectors !	
	// Do Deformation with warpData on Reference Mesh
	//WarpVector will use the array marked as active vector in polydata
	//it has to be a 3 component array
	//with the same number of tuples as points in polydata
}

//-----------------------------------------------------------------------------
//  updateColorBars
//-----------------------------------------------------------------------------
void GlyphVisualization
  ::updateColorBars()
{
	m_scalarBarVector->SetLookupTable( m_lutTangentialComponent );
	m_glyphMapper->SetLookupTable( m_lutTangentialComponent );
	m_glyphMapper->Update();
}

//-----------------------------------------------------------------------------
//  setGlyphSize
//-----------------------------------------------------------------------------
void GlyphVisualization
  ::setGlyphSize(double value)
{
	m_glyphSize=value;
	if( !m_autoScaleEnabled )
	{
		m_glyph->SetScaleFactor(m_glyphSize);
		m_glyph->Update();
	}
}

//-----------------------------------------------------------------------------
//  updateWarpVis
//-----------------------------------------------------------------------------
void GlyphVisualization
  ::updateWarpVis( vtkImageData* source,
				   vtkPolyData* mesh,
				   std::vector<vtkPolyData*> ref_meshes,
				   vtkPolyData* pointSamples )
{
	mesh->Update();  // just to be sure the model is already loaded in memory

	VTKPTR<vtkProbeFilter> probe = VTKPTR<vtkProbeFilter>::New();
	probe->SetInput(mesh );
	probe->SetSource( source );
	probe->Update();

	internal_create( source, probe->GetOutput(),mesh,ref_meshes,pointSamples );
}

//-----------------------------------------------------------------------------
//  setScaleValue
//-----------------------------------------------------------------------------
void GlyphVisualization
  ::setScaleValue( double value )
{
	m_warpScale = value;
}
