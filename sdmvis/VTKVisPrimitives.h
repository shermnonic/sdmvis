//=============================================================================
//
//  VTK Visualization Primitives
//
//	- SphereMarker  : Render simple sphere
//	- ArrowMarker   : Render simple arrow (currently only a line is drawn)
//	- VRen          : Render a volume dataset
//  - VRen2         : Render comparison of two volume datasets
//  - vtkext3DGridSource : 3D grid represented with lines
//  - WarpVis       : Visualization of TPS displacement field (aka warp)
//                    using glyphs or hedgehog.
//  - WarpVolumeVis : Visualization of TPS displacement field (aka warp)
//                    using volume rendering of magnitude of disp.field.
//  TODO:
//  - split in several source files
//  - better VTK integration (apply DataProcessing input/output concept)
//
//=============================================================================

// -- Config ------------------------------------------------------------------
// Use marching cubes specialization to generate iso-surface (performance)
#define VREN_MARCHING_CUBES

// Support tomTpsTransform class to apply thin-plate spline warp of grid
//#define VREN_SUPPORT_TOM
// ----------------------------------------------------------------------------

#ifndef VREN_DEFAULT_VERBOSITY
 #ifdef _DEBUG
  #define VREN_DEFAULT_VERBOSITY 3
 #else
  #define VREN_DEFAULT_VERBOSITY 0
 #endif
#endif

#ifdef VREN_SUPPORT_TOM
	#define VTKEXT_3DGRIDSOURCE_SUPPORT_TOM
	#ifdef VTKEXT_3DGRIDSOURCE_SUPPORT_TOM
	#include "tomTpsTransform.h"
	#endif
#endif

// vtk volume stuff
#include <vtkVolumeTextureMapper3D.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkPiecewiseFunction.h>
#include <vtkColorTransferFunction.h>

// vtk volume loader
#include <vtkMetaImageReader.h>

//#include <vtkAxesActor.h>
#include <vtkOutlineFilter.h>
#include <vtkCubeAxesActor2D.h>

#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkMatrix4x4.h>
#include <vtkPolyDataMapper.h>
#include <vtkSphereSource.h>
#include <vtkArrowSource.h>
#include <vtkLineSource.h>
#include <vtkPolyDataSource.h>
#include <vtkDepthSortPolyData.h>

#define VREN_MARCHING_CUBES
#include <vtkMarchingCubes.h>
#ifdef VREN_MARCHING_CUBES
#else
#include <vtkContourFilter.h>
#include <vtkPolyDataNormals.h>
#endif

#include <vtkRenderer.h>
#include <vtkCamera.h>

#include <vtkHedgeHog.h>
#include <vtkImageData.h>
#include <vtkStructuredGrid.h>
#include <vtkArrowSource.h>
#include <vtkGlyph3D.h>
#include <vtkScalarBarActor.h>

#ifndef VTKPTR
//---
#include <vtkSmartPointer.h>

// to make the code easier readable:
#define VTKPTR vtkSmartPointer
// Note: The usage of this smart pointer is optional; the advantage is that 
//	the object is automatically deleted,
//  i.e.             { VTKPTR<vtkClass> var = VTKPTR<vtkClass>::New();   ...; }
//  is equivalent to { vtkClass* var = vtkClass::New();   ...; var->Delete(); }
//---
#endif


//-----------------------------------------------------------------------------
//	vtkext 3DGridSource
//-----------------------------------------------------------------------------

// .NAME vtk3DGridSource - Create a 3D polygonal grid
// .SECTION Description
//
// vtk3DGridSource creates a 3D grid centered at origin. The grid is represented
// with lines. It is possible to specify the length, width, 
// and height of the grid independently. Also the number of cubes in the grid
// can be specified.

/** 
  Creates a 3D grid represented with lines.

  Code adpated from Rasmus Paulsen, http://www2.imm.dtu.dk/~rrp/VTK/.

  vtk3DGridSource creates a 3D grid optionally centered at origin, or 
  with lower left corner at origin. The grid is represented with lines. 
  It is possible to specify the length, width, and height of the grid 
  independently. Also the number of cubes in the grid can be specified.

  \ingroup VolumeTools
*/
class vtkext3DGridSource : public vtkPolyDataSource
{
public:
  static vtkext3DGridSource *New();
//  vtkTypeRevisionMacro(vtkext3DGridSource,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the length of the grid in the x-direction.
  vtkSetClampMacro(XLength,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(XLength,float);

  // Description:
  // Set the length of the grid in the y-direction.
  vtkSetClampMacro(YLength,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(YLength,float);

  // Description:
  // Set the length of the grid in the z-direction.
  vtkSetClampMacro(ZLength,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(ZLength,float);

  // Description:
  // Set the center of the grid.
  vtkSetVector3Macro(Center,float);
  vtkGetVectorMacro(Center,float,3);

  // Description:
  // Set the number of cubes in the x-direction.
  vtkSetMacro(XCubes, int);
  vtkGetMacro(XCubes, int);
  
  // Description:
  // Set the number of cubes in the y-direction.
  vtkSetMacro(YCubes, int);
  vtkGetMacro(YCubes, int);

  // Description:
  // Set the number of cubes in the z-direction.
  vtkSetMacro(ZCubes, int);
  vtkGetMacro(ZCubes, int);

  // Description:
  vtkSetMacro(CenterGrid, bool);
  vtkGetMacro(CenterGrid, bool);

#ifdef VTKEXT_3DGRIDSOURCE_SUPPORT_TOM
public:
  void tomSetTpsTransform( tomTpsTransform* ttt )
  {
	  m_tomTpsTransform = ttt;
  }
protected:
  tomTpsTransform* m_tomTpsTransform;
#endif

protected:
  vtkext3DGridSource(float xL=1.0, float yL=1.0, float zL=1.0);
  ~vtkext3DGridSource() {};

  void Execute();
  float XLength;
  float YLength;
  float ZLength;
  float Center[3];
  int XCubes;
  int YCubes;
  int ZCubes;
  bool CenterGrid;

private:
  vtkext3DGridSource(const vtkext3DGridSource&); // Not implemented.
  void operator=(const vtkext3DGridSource&); // Not implemented.
};


//-----------------------------------------------------------------------------
//	Markers
//-----------------------------------------------------------------------------

/** 
	Render simple sphere

	\ingroup VolumeTools
*/
class SphereMarker
{
public:
	SphereMarker()
	{
		m_sphere = VTKPTR<vtkSphereSource>  ::New();
		m_mapper = VTKPTR<vtkPolyDataMapper>::New();
		m_actor  = VTKPTR<vtkActor>         ::New();
	}

	void setup( double x, double y, double z, double radius=1. );

	vtkActor*        getActor()        { return m_actor; }
	vtkSphereSource* getSphereSource() { return m_sphere; }

private:
	VTKPTR<vtkSphereSource>   m_sphere;
	VTKPTR<vtkPolyDataMapper> m_mapper;
	VTKPTR<vtkActor>          m_actor;
};

/**
	Render simple arrow (currently only a line is drawn)

	\ingroup VolumeTools
*/
class ArrowMarker
{
public:
	ArrowMarker()
	{
		m_source = VTKPTR<vtkLineSource>    ::New();
		m_mapper = VTKPTR<vtkPolyDataMapper>::New();
		m_actor  = VTKPTR<vtkActor>         ::New();
	}

	void setup( double x0, double y0, double z0, 
		        double x1, double y1, double z1 );

	vtkActor*       getActor()  { return m_actor; }
	vtkLineSource*  getSource() { return m_source; }

private:
	VTKPTR<vtkLineSource>     m_source;
	VTKPTR<vtkPolyDataMapper> m_mapper;
	VTKPTR<vtkActor>          m_actor;
};


//-----------------------------------------------------------------------------
//	VRen
//-----------------------------------------------------------------------------

/**
	Render a volume dataset

	\ingroup VolumeTools
*/
class VRen
{
public:
	VRen()
	{
		// volume
		m_colorFunc   = VTKPTR<vtkColorTransferFunction>::New();
		m_opacityFunc = VTKPTR<vtkPiecewiseFunction    >::New();
		m_mapper      = VTKPTR<vtkVolumeTextureMapper3D>::New();
		m_volume      = VTKPTR<vtkVolume               >::New();
		m_renderer    = VTKPTR<vtkRenderer             >::New();

		// isosurface
	  #ifdef VREN_MARCHING_CUBES
		m_contour       = VTKPTR<vtkMarchingCubes  >::New();
	  #else
		m_contour       = VTKPTR<vtkContourFilter  >::New();
		m_normals       = VTKPTR<vtkPolyDataNormals>::New();
	  #endif
		m_contourMapper = VTKPTR<vtkPolyDataMapper >::New();
		m_contourActor  = VTKPTR<vtkActor          >::New();

		// outline
		m_outline       = VTKPTR<vtkOutlineFilter  >::New();
		m_outlineMapper = VTKPTR<vtkPolyDataMapper >::New();
		m_outlineActor  = VTKPTR<vtkActor          >::New();

		// axes
		m_axes = VTKPTR<vtkCubeAxesActor2D>::New();

		// grid
		m_grid          = VTKPTR<vtkext3DGridSource>::New();
		m_gridMapper    = VTKPTR<vtkPolyDataMapper >::New();
		m_gridActor     = VTKPTR<vtkActor          >::New();
	}

	/// Load volume data from disk and setup VTK renderer
	bool setup( const char* filename );

	/// Returns the volume renderer initialized by calling \a setup()
	vtkRenderer* getRenderer() { return m_renderer; }

	void setOutlineVisibility( bool visible );
	void setAxesVisibility   ( bool visible );
	void setVolumeVisibility ( bool visible );

	void setContourVisibility( bool visible );
	void setContourValue( double value );	
	double getContourValue() const;

	void setGridVisibility   ( bool visible );
	void setGridSize( int i );
	int  getGridSize() const;

	/// Export current isosurface as Wavefront OBJ
	void exportOBJ( const char* fileprefix );

#ifdef VREN_SUPPORT_TOM
	void setTpsTransform( tomTpsTransform* tps )
	{
		m_grid->tomSetTpsTransform( tps );
		m_grid->Modified();
		m_grid->Update();
	}
#endif

private:
	//@{ Volume classes
	VTKPTR<vtkPiecewiseFunction>     m_opacityFunc;
	VTKPTR<vtkColorTransferFunction> m_colorFunc;
	VTKPTR<vtkVolumeTextureMapper3D> m_mapper;
	VTKPTR<vtkVolume>                m_volume;
	VTKPTR<vtkRenderer>              m_renderer;
	//@}

	//@{ Isosurface classes
#ifdef VREN_MARCHING_CUBES
	VTKPTR<vtkMarchingCubes>   m_contour;
#else
	VTKPTR<vtkContourFilter>   m_contour;
	VTKPTR<vtkPolyDataNormals> m_normals;
#endif
	VTKPTR<vtkPolyDataMapper>  m_contourMapper;
	VTKPTR<vtkActor>           m_contourActor;
	//@}

	//@{ Outline classes
	VTKPTR<vtkOutlineFilter>  m_outline;
	VTKPTR<vtkPolyDataMapper> m_outlineMapper;
	VTKPTR<vtkActor>          m_outlineActor;
	//@}

	/// Axes
	VTKPTR<vtkCubeAxesActor2D> m_axes;

	//@{ Grid
	VTKPTR<vtkext3DGridSource> m_grid;
	VTKPTR<vtkPolyDataMapper>  m_gridMapper;
	VTKPTR<vtkActor>           m_gridActor;
	//@}
};



//-----------------------------------------------------------------------------
//	VRen2
//-----------------------------------------------------------------------------

/**
	Render two volume datasets, typically reference and target

	\ingroup VolumeTools
*/
class VRen2
{
	int m_verbosity;

public:
	VRen2()
		: m_verbosity( VREN_DEFAULT_VERBOSITY )
	{
		m_renderer      = VTKPTR<vtkRenderer       >::New();

		// outline
		m_outline       = VTKPTR<vtkOutlineFilter  >::New();
		m_outlineMapper = VTKPTR<vtkPolyDataMapper >::New();
		m_outlineActor  = VTKPTR<vtkActor          >::New();

		// axes
		m_axes          = VTKPTR<vtkCubeAxesActor2D>::New();

		// grid
		m_grid          = VTKPTR<vtkext3DGridSource>::New();
		m_gridMapper    = VTKPTR<vtkPolyDataMapper >::New();
		m_gridActor     = VTKPTR<vtkActor          >::New();

		// volumes and isosurfaces are initialized in their respective c'tors
		m_contour0.activeCamera = m_renderer->GetActiveCamera();
		m_contour1.activeCamera = m_renderer->GetActiveCamera();

		// set user transform matrix
		m_transform0 = VTKPTR<vtkMatrix4x4>::New();
		m_transform1 = VTKPTR<vtkMatrix4x4>::New();
		m_volume0 .getProp3D()->SetUserMatrix( m_transform0 );
		m_contour0.getProp3D()->SetUserMatrix( m_transform0 );
		m_volume1 .getProp3D()->SetUserMatrix( m_transform1 );
		m_contour1.getProp3D()->SetUserMatrix( m_transform1 );
	}

	//@{ Load volume data from disk and setup VTK renderer
	bool load_reference( const char* filename );
	bool load_target   ( const char* filename );
	//@}

	/// Provided for convenience to be syntactically compatible to VRen
	bool setup( const char* filename )
		{ 
			return load_reference( filename );
		}

	/// Returns the volume renderer initialized by calling \a setup()
	vtkRenderer* getRenderer() { return m_renderer; }

	/// Return isosurface of specified volume (as PolyData)
	vtkPolyData* getContour( int volid=0 )
	{
		if( volid==1 )
			return m_contour1.contour->GetOutput();
		return m_contour0.contour->GetOutput();
	}

	//@{ 
	/** Adjust Outline, Axes and Grid of reference volume respectively */
	void setOutlineVisibility( bool visible );
	bool getOutlineVisibility() const;
	void setAxesVisibility   ( bool visible );
	bool getAxesVisibility() const;
	void setGridVisibility   ( bool visible );
	bool getGridVisibility() const;
	void setGridSize( int i );
	int  getGridSize() const;
	//@}

	//@{ 
	/** Toggle display of specified volume/isosurface 
	    \arg volid Volume ID: 0=reference (default), 1=target
	*/
	void setVolumeVisibility ( bool visible, int volid=0 );
	void setContourVisibility( bool visible, int volid=0 );
	bool getVolumeVisibility ( int volid=0 ) const;
	bool getContourVisibility( int volid=0 ) const;
	//@}

	//@{ Adjust isosurface parameters for both volumes simultaneously
	void setContourValue( double value );	
	double getContourValue() const;
	//@}

	/// Export currently visible isosurface as Wavefront OBJ
	void exportOBJ( const char* fileprefix );

	/// Get bounds of reference volume as (xmin,xmax,ymin,ymax,zmin,zmax)
	void getBounds( double* bounds ) { for(int i=0;i<6;++i) bounds[i]=m_bounds[i]; }

	vtkImageData* getVolumeImageData( int volid=0 );

	/// Get user matrix (homogenous transform applied to volume and contour)
	vtkMatrix4x4* getUserMatrix( int volid=0 )
	{
		if( volid==1 ) return m_transform1;
		               return m_transform0;
	}

	/// Return pointer to volume actor (as its base class \a vtkProp3D)
	vtkProp3D* getVolumeProp3D( int volid=0 )
	{
		if( volid==1 ) return m_volume1.getProp3D();
		               return m_volume0.getProp3D();
	}

	/// Return pointer to contour actor (as its base class \a vtkProp3D)
	vtkProp3D* getContourProp3D( int volid=0 )
	{
		if( volid==1 ) return m_contour1.getProp3D();
		               return m_contour0.getProp3D();
	}

#ifdef VREN_SUPPORT_TOM
	void setTpsTransform( tomTpsTransform* tps );
#endif

	//---------------------------------------------------------------
	/** 
		Combine specific DataSet-Mapper-Actor vis.pipeline

	    The rationale here is that between DataSet, Mapper and Actor
	    a 1:1 relationship holds and thus we can encapsulate that part
	    of the pipeline for a specific visualization in a single class.
	    Since we want to support vtkActor as well as the volume pendant
	    vtkVolume, we make the shared super class vtkProp3D accessible
	    by \a getProp3D().

	    The DataSet is set as usual via \a setInputConnection(). 
	*/
	struct MetaActor
	{
		virtual void setInputConnection( vtkAlgorithmOutput* input )=0;
		virtual vtkProp3D* getProp3D()=0;

		// Probably this would be a convenient way to add own actor
		// to given renderer, either vtkVolume or vtkActor.
		// void addToRenderer( vtkRenderer* ren )
		// {
		//     if( volume.GetPointer() != NULL )
		//         ren->addVolume( volume );
		//     if( actor .GetPointer() != NULL )
		//         ren->addActor ( actor  );
		// }
		// VTKPTR<vtkVolume> volume;
		// VTKPTR<vtkActor > actor;
	};

	//---------------------------------------------------------------
	/// Volume vis. (includes transfer function)
	struct VolumeMetaActor : public MetaActor
	{
		void setInputConnection( vtkAlgorithmOutput* input );
		vtkProp3D* getProp3D() { return (vtkProp3D*)volume; }

		/** \todo Avoid code duplication by adding an Update() like
		          function (currently setInputConnection() and
		          setInput() have nearly identical code)! */
		void setInput( vtkImageData* );

		VolumeMetaActor() {
			colorFunc   = VTKPTR<vtkColorTransferFunction>::New();
			opacityFunc = VTKPTR<vtkPiecewiseFunction    >::New();
			mapper      = VTKPTR<vtkVolumeTextureMapper3D>::New();
			volume      = VTKPTR<vtkVolume               >::New();
		}
		VTKPTR<vtkPiecewiseFunction>     opacityFunc;
		VTKPTR<vtkColorTransferFunction> colorFunc;
		VTKPTR<vtkVolumeTextureMapper3D> mapper;
		VTKPTR<vtkVolume>                volume;
	};

	//---------------------------------------------------------------
	/// Isosurface vis. (includes optional depth sorting)
	struct IsosurfaceMetaActor : public MetaActor
	{
		void setInputConnection( vtkAlgorithmOutput* input );
		vtkProp3D* getProp3D() { return (vtkProp3D*)contourActor; }

		/** Active camera of associated Renderer, needed for depth-sorting
		    If not set prior to setInputConnection() depthsorting will not
		    be enabled! */
		VTKPTR<vtkCamera> activeCamera;

		IsosurfaceMetaActor() {
			// isosurface
		  #ifdef VREN_MARCHING_CUBES
			contour       = VTKPTR<vtkMarchingCubes  >::New();
		  #else
			contour       = VTKPTR<vtkContourFilter  >::New();
			normals       = VTKPTR<vtkPolyDataNormals>::New();
		  #endif
			contourMapper = VTKPTR<vtkPolyDataMapper >::New();
			contourActor  = VTKPTR<vtkActor          >::New();
			depthsort   = VTKPTR<vtkDepthSortPolyData>::New();
		}
	#ifdef VREN_MARCHING_CUBES
		VTKPTR<vtkMarchingCubes>   contour;
	#else
		VTKPTR<vtkContourFilter>   contour;
		VTKPTR<vtkPolyDataNormals> normals;
	#endif
		VTKPTR<vtkPolyDataMapper>  contourMapper;
		VTKPTR<vtkActor>           contourActor;
		VTKPTR<vtkDepthSortPolyData> depthsort;
	};

private:
	VTKPTR<vtkRenderer>        m_renderer;

	VTKPTR<vtkMatrix4x4>       m_transform0,
	                           m_transform1;

	VolumeMetaActor            m_volume0,   ///< reference volume
	                           m_volume1;   ///< target volume

	double                     m_bounds[6]; ///< bounds of reference volume

	IsosurfaceMetaActor        m_contour0,
	                           m_contour1;

	//@{ Outline classes
	VTKPTR<vtkOutlineFilter>   m_outline;
	VTKPTR<vtkPolyDataMapper>  m_outlineMapper;
	VTKPTR<vtkActor>           m_outlineActor;
	//@}

	/// Axes
	VTKPTR<vtkCubeAxesActor2D> m_axes;

	//@{ Grid
	VTKPTR<vtkext3DGridSource> m_grid;
	VTKPTR<vtkPolyDataMapper>  m_gridMapper;
	VTKPTR<vtkActor>           m_gridActor;
	//@}
};


//-----------------------------------------------------------------------------
//	WarpVis
//-----------------------------------------------------------------------------

/**
  Displacement field visualization.

  Not adapted to VRen2::MetaActor concept yet.

  \ingroup VolumeTools
*/
class WarpVis
{
	int m_verbosity;
public:
	enum WarpVisType 
	{
		Hedgehog,
		Glyphs
	};

	WarpVis()
		: m_verbosity( VREN_DEFAULT_VERBOSITY ),
		  m_type(Glyphs),
		  m_stratify(true)
	{
		m_warp     = VTKPTR<vtkStructuredGrid>::New();
		m_actor    = VTKPTR<vtkActor>         ::New();
		m_mapper   = VTKPTR<vtkPolyDataMapper>::New();

		m_hedgehog = VTKPTR<vtkHedgeHog>      ::New();

		m_arrow    = VTKPTR<vtkArrowSource>   ::New();
		m_glyph    = VTKPTR<vtkGlyph3D>       ::New();

		setSpacing( 1,1,1 );
		setOrigin( 0,0,0 );
	}

	void setSpacing( double sx, double sy, double sz ) 
	{
		m_spacing[0]=sx; m_spacing[1]=sy; m_spacing[2]=sz;
	}
	void setOrigin ( double ox, double oy, double oz )
	{
		m_origin[0]=ox; m_origin[1]=oy; m_origin[2]=oz; 
	}

	/** Set visualization type (either Hedgehog or Glyph)
	    Must be called *before* setup(). */
	void setType( WarpVisType type ) { m_type = type; }

	void setup( vtkStructuredGrid* warp );

#ifdef VREN_SUPPORT_TOM
	/** Setup visualization.
	    Computation of dense displacement field can take some time!
	    We assume here our default voxel grid coordinate system with origin at
	    (0,0,0), of spacing (1,1,1) and dimensionality of voxel grid size. */
	void setup( const tomTpsTransform& tps, int dimi, int dimj, int dimk );

	/** Use PolyData as alternative domain to StructuredGrid (experimental!)
	    Attributes (scalar and vector) will be changed of input polydata! */
	void setup( const tomTpsTransform& tps, vtkPolyData* polydata );
#endif

	/** Apply volume scalar value as opacity
	    Must be called *after* setup() */
	void setOpacityToVolumeIntensity( vtkImageData* vol );

	void addToRenderer( vtkRenderer* ren )
	{
		ren->AddActor( m_actor );
	}

	/// \deprecated Use \a addToRenderer() instead.
	vtkActor* getActor() { return m_actor; }
	
private:
	VTKPTR<vtkStructuredGrid> m_warp;
	VTKPTR<vtkPolyDataMapper> m_mapper;
	VTKPTR<vtkActor>          m_actor;

	// Hedgehog
	VTKPTR<vtkHedgeHog>       m_hedgehog;

	// Glyphs
	VTKPTR<vtkArrowSource>    m_arrow;
	VTKPTR<vtkGlyph3D>        m_glyph;

	double m_spacing[3];
	double m_origin [3];

	WarpVisType m_type;
	bool        m_stratify;  ///< stratified random sampling (applies only to StructuredGrid)
};


//-----------------------------------------------------------------------------
//	WarpVolumeVis
//-----------------------------------------------------------------------------

/**
  Volume visualization of displacement field magnitude.

  \ingroup VolumeTools
*/
class WarpVolumeVis
{
	int m_verbosity;
public:
	WarpVolumeVis()
		: m_verbosity( VREN_DEFAULT_VERBOSITY )
	{
		m_mag = VTKPTR<vtkImageData>::New();
	}

	void setup( vtkImageData* warp );

#ifdef VREN_SUPPORT_TOM
	/// Setup visualization.
	/// Computation of dense displacement field can take some time!
	/// We assume here our default voxel grid coordinate system with origin at
	/// (0,0,0), of spacing (1,1,1) and dimensionality of voxel grid size.
	void setup( const tomTpsTransform& tps, int dimi, int dimj, int dimk );
#endif

	void addToRenderer( vtkRenderer* ren )
	{
		ren->AddVolume( m_volvis.volume );
	}

	/// \deprecated Use \a addToRenderer() instead.
	vtkVolume* getVolume() { return m_volvis.volume; }

private:
	VRen2::VolumeMetaActor m_volvis;

	VTKPTR<vtkImageData> m_mag; ///< warpfield magnitude, input for volume vis.
};



//-----------------------------------------------------------------------------
// 	tomSetupStratifiedGrid
//-----------------------------------------------------------------------------

/**
  Setup a regular StructuredGrid with stratified grid vertex positions.

  Intended usage:
	\code
	grid = vtkStructuredGrid::New();
	setupGrid( grid, ... );
	tomTpsWarpfield( grid );
	\endcode

  \param[out] grid     This is overwritten with the grid constructed in this
                       function (nevertheless, the input pointer must be valid!)
  \param[in]  dim      Dimensions grid as int[3]
  \param[in]  spacing  Spacing of grid as double[3]
  \param[in]  origin   Origin of grid as double[3]
  \param      stratify Apply stratification, i.e. randomly displace grid vertices
                       by a small amount (but without leaving grid cell).
*/
void tomSetupStratifiedGrid( vtkStructuredGrid* grid, int* dim,
                             const double* spacing, const double* origin, 
							 bool stratify );







//-----------------------------------------------------------------------------
// VectorfieldGlyphVisualization2 (translated from myvis.py python class)
//-----------------------------------------------------------------------------

class VectorfieldGlyphVisualization2
{
public:
	VectorfieldGlyphVisualization2( vtkImageData* source, int numPts=50000 );
	vtkActor* getVisActor() { return m_visActor; }
	
	vtkGlyph3D*        getGlyph3D()     { return m_glyph; }
	vtkPolyDataMapper* getGlyphMapper() { return m_glyphMapper; }
	vtkScalarBarActor* getScalarBar()   { return m_scalarBar; }

protected:
	VTKPTR<vtkActor> createGlyphVisActor( vtkAlgorithm* algo );
	vtkActor* createPolyActor    ( vtkAlgorithm* algo );
	vtkActor* createOutlineActor2( vtkImageData* data );

private:
	VTKPTR<vtkActor> m_visActor;       ///< glyph visualization
	VTKPTR<vtkActor> m_polyActor;      ///< sampling points
	VTKPTR<vtkActor> m_outlineActor;   ///< vectorfield outline
	VTKPTR<vtkGlyph3D>        m_glyph;
	VTKPTR<vtkPolyDataMapper> m_glyphMapper;
	VTKPTR<vtkScalarBarActor> m_scalarBar;
};



//-----------------------------------------------------------------------------
//	some VTK helper functions
//-----------------------------------------------------------------------------
VTKPTR<vtkImageData> loadImageData( const char* filename );
void computeImageDataMagnitude( vtkImageData* img, float* mag_min_, float* mag_max_ );


