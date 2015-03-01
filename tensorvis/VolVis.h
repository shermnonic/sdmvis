#ifndef VOLVIS_H
#define VOLVIS_H

#include <vtkMetaImageReader.h>
#include <vtkImageData.h>
#include <vtkPiecewiseFunction.h>
#include <vtkColorTransferFunction.h>
#include <vtkSmartVolumeMapper.h> // was: vtkVolumeTextureMapper3D
#include <vtkVolume.h>
#include <vtkOutlineFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#ifndef VTKPTR
#include <vtkSmartPointer.h>
#define VTKPTR vtkSmartPointer
#endif
// Use marching cubes specialization to generate iso-surface
// (should increase performance)
#define VOLVIS_MARCHING_CUBES
#include <vtkMarchingCubes.h>
#ifdef VOLVIS_MARCHING_CUBES
#else
#include <vtkContourFilter.h>
#include <vtkPolyDataNormals.h>
#endif

///	Simple volume visualization, raycasting and isosurface rendering
class VolVis
{
public:
	VolVis();
	~VolVis();

	/// Load volume data from disk and setup VTK renderer.
	/// Returns false if image could not be loaded.
	bool setup( const char* filename );
	/// Setup from existing image data
	void setup( VTKPTR<vtkImageData> volume );

	/// Return pointer to current image data (for external usage)
	VTKPTR<vtkImageData> getImageData() { return m_imageData; }

	/// Attach volume visualization to given renderer
	void setRenderer( vtkRenderer* renderer );

	/// Show/hide all components of this visualization
	void setVisibility( bool visible );
	bool getVisibility() const { return m_visible; }

	void setOutlineVisibility( bool visible );
	void setVolumeVisibility ( bool visible );

	void setContourVisibility( bool visible );
	void setContourValue( double value );	
	double getContourValue() const;

	vtkVolumeMapper* getVolumeMapper() { return m_mapper; }
	vtkVolume*       getVolumeActor() { return m_volume; }
	vtkActor*        getContourActor() { return m_contourActor; }

	bool hasValidImage();

private:
	bool m_visible;

	VTKPTR<vtkImageData>             m_imageData;

	//@{ Volume classes
	VTKPTR<vtkPiecewiseFunction>     m_opacityFunc;
	VTKPTR<vtkColorTransferFunction> m_colorFunc;
	VTKPTR<vtkSmartVolumeMapper>     m_mapper;
	VTKPTR<vtkVolume>                m_volume;
	//@}

	//@{ Isosurface classes
#ifdef VOLVIS_MARCHING_CUBES
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
};

#endif // VOLVIS_H
