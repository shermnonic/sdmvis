#ifndef GLYPHVISUALIZATION_H
#define GLYPHVISUALIZATION_H

#include <vector>

#include <vtkActor.h>
#include <vtkGlyph3D.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkScalarBarActor.h>
#include <vtkScalarsToColors.h>
#include <vtkImageData.h>

#ifndef VTKPTR
 #include <vtkSmartPointer.h>
 #define VTKPTR vtkSmartPointer
#endif

//-----------------------------------------------------------------------------
// GlyphVisualization (translated from myvis.py python class)
//-----------------------------------------------------------------------------
/**
	Visualize a displacement field on a mesh. Tangential component is shown
	via a glyph-based vectorfield visualization while the normal component of
	the displacement field is color coded on the mesh surface.

	De-factor standard visualization for surface deformation fields as used
	extensively for instance by Zollikofer et al.

	\author Max Hermann
	\author Vitalis Wiens
*/
class GlyphVisualization
{
public:
	GlyphVisualization( vtkImageData* source, vtkPolyData* mesh, 
               std::vector<vtkPolyData*> ref_meshes, vtkPolyData* pointSamples,
			   bool invertVectors=false );
	~GlyphVisualization();
	vtkActor*			getVisActor()        { return m_visActor; }
	vtkPolyData*		getVectorField()     { return m_vectorField;}
	vtkGlyph3D*			getGlyph3D()         { return m_glyph; }
	vtkPolyDataMapper*	getGlyphMapper()     { return m_glyphMapper; }
	vtkScalarBarActor*	getScalarBar()       { return m_scalarBar; }
	vtkScalarBarActor*	getScalarBarVector() { return m_scalarBarVector; }
	vtkScalarsToColors* getLutTangentialComponent() { return m_lutTangentialComponent;}
	vtkScalarsToColors* getLutNormalComponent()     { return m_lutNormalComponent; }
	double				getWarpScale() const { return m_warpScale; }	
	
	void updateWarpVis( vtkImageData* source, vtkPolyData* mesh, 
	          std::vector<vtkPolyData*> ref_meshes, vtkPolyData* pointSamples);

	void setImageData( vtkImageData* source ) { m_imageData=source; }
	vtkImageData* getImageData() { return m_imageData; }

	vtkPolyData* getClusterData(){return m_clusterData;}

	void setGlyphSize( double value );
	void setScaleValue( double value );	

	void setGlyphAutoScaling( bool b) { m_autoScaleEnabled = b; }
	void setGlyphAutoScaleFactor( double f ) { m_autoScaleFactor = f; };
	bool getGlyphAutoScaling() const { return m_autoScaleEnabled; }
	double getGlyphAutoScaleFactor() const { return m_autoScaleFactor; };

	double getMaxOrthVector() const { return m_maxOrthVector; }


	void updateColorBars();

protected:
	VTKPTR<vtkActor> createGlyphVisActor2( vtkPolyData* polyData);
	VTKPTR<vtkActor> createGlyphVisActor2( vtkAlgorithm* algo );
	void internal_create( vtkImageData* source, vtkDataSet* samples,
             vtkPolyData* mesh,std::vector<vtkPolyData*> ref_meshes,
             vtkPolyData* pointSamples );

private:
	float m_minOrthVector;
	float m_maxOrthVector;

	double m_glyphSize;
	double m_warpScale;

	bool   m_autoScaleEnabled;
	double m_autoScaleFactor;

	bool   m_invertVectors;

	VTKPTR<vtkActor>		  m_visActor;
	VTKPTR<vtkGlyph3D>        m_glyph;
	VTKPTR<vtkPolyDataMapper> m_glyphMapper;
	VTKPTR<vtkScalarBarActor> m_scalarBar;
	VTKPTR<vtkScalarBarActor> m_scalarBarVector;
	VTKPTR<vtkPolyData>		  m_vectorField;
	VTKPTR<vtkImageData>	  m_imageData;
	VTKPTR<vtkScalarsToColors> m_lutTangentialComponent;
	VTKPTR<vtkScalarsToColors> m_lutNormalComponent;
	vtkPolyData*			  m_clusterData;
};

#endif // GLYPHVISUALIZATION_H
