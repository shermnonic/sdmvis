#ifndef VARVISRENDERER_H
#define VARVISRENDERER_H

// -- Config ------------------------------------------------------------------
// Use marching cubes specialization to generate iso-surface (performance)
#define VREN_MARCHING_CUBES
#define VREN_GPU_RAYCASTER

#ifdef _DEBUG
#define VREN_DEFAULT_VERBOSITY 3
#else
#define VREN_DEFAULT_VERBOSITY 0
#endif
// ----------------------------------------------------------------------------

#include "VectorfieldClustering.h"
#include "GlyphVisualization.h"

// vtk volume stuff
#ifdef VREN_GPU_RAYCASTER
 #include <vtkGPUVolumeRayCastMapper.h>
 #define VREN_VOLUME_MAPPER vtkGPUVolumeRayCastMapper
#else
 #include <vtkVolumeTextureMapper3D.h>
 #define VREN_VOLUME_MAPPER vtkVolumeTextureMapper3D
#endif

#include <vector>

#include <QVector>
#include <QSlider>
#include <QComboBox>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QSettings>
#include <QFileDialog>

#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkPiecewiseFunction.h>

#include <vtkMetaImageReader.h>
#include <vtkOutlineFilter.h>
#include <vtkCubeAxesActor2D.h>
#include <vtkSphereSource.h>

#include <vtkActor.h>
#include <vtkArrowSource.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkColorTransferFunction.h>
#include <vtkContourFilter.h>
#include <vtkDoubleArray.h>
#include <vtkFieldDataToAttributeDataFilter.h>
#include <vtkGlyph3D.h>
#include <vtkGridTransform.h>
#include <vtkHedgeHog.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkPointSource.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataNormals.h>
#include <vtkProperty.h>
#include <vtkProbeFilter.h>
#include <vtkMarchingCubes.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkTransformToGrid.h>
#include <vtkTriangle.h>
#include <vtkWarpScalar.h>
#include <vtkWarpVector.h>
#include <vtkStructuredGrid.h>
#include <vtkScalarBarActor.h>
#include <vtkImageData.h>
#include <vtkTextActor.h>
#include <vtkProperty2D.h>

#undef VTK_DIRECTION_SPECIFIED_VECTOR  // WORKAROUND for duplicate VTK define
#include <vtkPolyDataSilhouette.h>
#include <vtkImageGaussianSmooth.h>

#ifndef VTKPTR
 #include <vtkSmartPointer.h>
 #define VTKPTR vtkSmartPointer
#endif

//-----------------------------------------------------------------------------
// VarVisRender
//-----------------------------------------------------------------------------
/**
	Main class for several visualizations of shape variability.

	This class provides different visualizations:
	- Glyph based vectorfield visualization (see remarks below). 
	- Isosurface visualization.
	- Volume visualization (DVR).
	- Silhouette visualization (not interactive yet).
	- Animated warps to visualize principal modes.
	- Difference volumes, i.e. to access registration and reconstruction quality.

	The vectorfield visualization, realized via \a GlyphVisualization, is done
	on a specific isosurface. To this end the vectorfield is evaluated by 
	uniform sampling of positions on triangles of the Marching Cubes mesh. The
	vectorial data is split into surface parallel and orthogonal part, where 
	the former is visualized as vector glyph and the latter by color coding the
	surface.

	Instead of using the raw vectorfield as input, a Voronoi clustering can be
	performed in advance to yield a simplified visual representation. The
	clustering can be done on the whole 3D space, what is computationally very
	expensive, or on a screen space sampling of the vectorfield, increasing the
	performance hugely. The clustering is realized via \a VectorfieldClustering.

	\sa GlyphVisualization
	\sa GlyphControls
	\sa VectorfieldClustering
	\sa PointSamplerFilter
	\sa VarVisWidget

	\author Vitalis Wiens
	\author Max Hermann	
*/
class VarVisRender: public QWidget
{
	Q_OBJECT

public:
	VarVisRender();
	~VarVisRender();

	bool initVarVis( const char* volumeName,const char* meshName,const char*pointsName,
							   const double meshIsoValue,const int numOFpoints );
	
	// - renderWindow - 
	void syncWith(VarVisRender* syncObject)
	{	
		m_syncObject = syncObject;
	}
	void setMeToRoiRender(bool doIt){m_bool_isRoiRender=doIt;}
	void setRenderWindow(vtkRenderWindow* renWin){m_renderWindow=renWin;}
	void setRenderer(vtkRenderer *render){m_renderer=render;}
	vtkRenderWindow * getRenderWindow(){return m_renderWindow;}
	
	// - WarpVis-
	void setWarpVis(GlyphVisualization * warpVis){m_warpVis=warpVis;}
	GlyphVisualization * getWarpVis(){return m_warpVis;}
	
	// - Isovalue - 
	void setIsovalue( double value );
	double getIsovalue() const { return m_isovalue; }
	
	/// Load volume data from disk and setup VTK renderer.
	/// Internally calls readVolume() and setup().
	bool load_reference( const char* filename );	  

	/// Call this after explicit call to readVolume() or setVolume()
	void setup();

	void makeScreenShot( QString outputFileName );

	/// k-means clustering of vectorfield in image plane 
	/// (considered are only sample points with normal facing the viewer)
	void clusterGlyphs();
	void clusterVolumeGlyphs();
	void setCentroidNumber(double val){m_centroidNum=val;}

	/// Animation Functions
	void startAnimation();
	void pauseAnimation();
	void stopAnimation();
	
	/// Visibility Functions
	void updateVisibility();
	void setMeshVisibility	 ( bool visible );
	void setGlyphVisible	 ( bool visible );
	void setRoiVisibility	 ( bool visible );
	void setSilhouetteVisible( bool visible );
	void setRefMeshVisibility( bool visible );
	void setSamplesVisibility(bool visible);
	void setClusterVisibility(bool visible);
	void setBarsVisibility   (bool visible);
	void showCluster(bool visible);
	void setVolumeVisibility (bool visible)	{ m_showVolume = visible; 
											  m_volume->SetVisibility( visible );
											}
	bool getSilhouetteVisible()				{return m_showSilhouette;}

	// - Getters and setters -

	QString getLoadedReference(){return m_loadedReferenceName;}
	QString getLocalReference(){return m_localReference;}

	bool getUseLocalReference(){return m_saveLocalReferenceModel;}
	void setUseLocalReference(bool use){m_saveLocalReferenceModel=use;}

	void setSlider(QSlider* slider){m_frameSlider=slider;}
	void setSelectionBox(QComboBox *box){m_selectionBox=box;}
	QComboBox* getSelectionBox(){return m_selectionBox;}

	void setWarp(QString mhdFilename, bool backwardsField);
	void setScaleValue		(double value);
	void setGlyphSize		(double value);
	void setGlyphAutoScaling( bool b );
	bool getSamplePresent(){return m_samplesPresent;}
	double getSamplePointSize(){return m_samlePointSize;}

	bool getVolumePresent(){return m_volumePresent;}
	bool getMeshPresent(){return m_meshPresent;}
	void setGaussionRadius (double radius)	{m_gaussionRadiusValue=radius;}
	void setNumberOfSamplingPoints(int num)	{m_sampleRange=num;}
	void setPointSize(double value)			{m_samlePointSize=value;}
	void generateROI(int index,double radius,double x,double y, double z);
	void setClusterSamples(vtkPolyData* data)		  {m_clusterSamples=data;}
	void resetRefMeshes();
	void useGaussion(bool yes){m_useGaussianSmoothing=yes;}
	void updateSamplePoints(int num);
	void setPointRadius(double value)
	{
		m_samlePointSize=value;
		m_samplerActor->GetProperty()->SetPointSize(value);  
	}


	/// SAVING MESH AND POINTSAMPLES POLYDATA
	/// return Value is for SDMVISCONFIG (isoValue / numOfSamplingPoints)
	double saveMesh(QString fileName);
	int savePointSamples(QString fileName);
	
	void setMesh(vtkPolyData* mesh);
	void setSample(vtkPolyData *pointsamples);

	void setVolume( VTKPTR<vtkImageData> volume, QString name );
	VTKPTR<vtkImageData> getVolume() { return m_VolumeImageData; }
	
	/// Reading Volume/Mesh/Points
	bool readVolume(QString volumeName);
	bool readMesh(QString meshName);
	bool readPoints(QString pointsName);
	void finalInit();


	/// Getter for Vtk
	vtkRenderer		  * getRenderer()		 {return m_renderer; }
	vtkPolyData		  * getMesh()			 {return m_mesh;}
	vtkPolyData		  * getPointSamples()	 {return m_pointPolyData;}
	vtkActor		  * getSamplesActor()	 {return m_samplerActor;}
	vtkActor		  * getSilhouetteActor() {return m_silhouetteActor;}
	vtkPolyData		  * getRefMesh()	     {return reference_mesh;}
	vtkPolyDataMapper * getMeshMapper()		 {return m_meshMapper;}

	std::vector<vtkPolyData*>       getReferenceMeshes () {return m_referenceMeshes;}
	std::vector<vtkActor*>          getReferenceActors () {return m_referenceActors;}
	std::vector<vtkPolyDataMapper*> getReferenceMappers() {return m_referenceMappers;}
	std::vector<vtkActor*>          getSilhActors ()	  {return m_silhActors;}
	std::vector<vtkPolyDataMapper*> getSilhMappers()	  {return m_silhMappers;}
	std::vector<vtkPolyDataSilhouette*> getReferenceSilh(){return m_referenceSilhouette;}

	// REGION OF INTEREST
	void clearRoi();
	void generateROI();
	void saveROI(QString mhdFilename);
	int getRoiIndex(){return m_RoiIndex;}
	void setRoiIndex(int index){m_RoiIndex=index;}
	std::vector<vtkSphereSource*>  getROI(){return m_roiList;}
	std::vector<vtkActor*>  getROIActors(){return m_roiActors;}
	
	void specifyROI(int index)
	{
			m_RoiIndex=index;
			for (unsigned i=0;i<m_roiActors.size();i++)
			{
				if (i==index)
					m_roiActors.at(i)->GetProperty()->SetColor(1,0,0);
				else
					m_roiActors.at(i)->GetProperty()->SetColor(0,0,1);
			}
			m_renderWindow->Render();
	}

	/// HELPING FUNCTIONS

	// Getter
	double * getHeightVector(){return m_vectorHeight;}
	double * getCenter(){return m_center;}
	double * getAufpunkt(){return m_aufpunkt;}
	double * getWidthVector(){return m_vectorWidth;}

	// Setter
	void setElementSpacing(double spaceX,double spaceY,double spaceZ)
	{
		m_elementSpacing[0]=spaceX;
		m_elementSpacing[1]=spaceY;
		m_elementSpacing[2]=spaceZ;
		m_useResolutionScaling=true;
	}
	
	void setResolution(double resX,double resY,double resZ)
	{
		m_resolution[0]=resX;
		m_resolution[1]=resY;
		m_resolution[2]=resZ;
		m_useResolutionScaling=true;
	}

	void setHeightVector(double x,double y,double z)
	{
		m_vectorHeight[0]=x;
		m_vectorHeight[1]=y;
		m_vectorHeight[2]=z;
    }

	void setCenter(double x,double y,double z)
	{
		m_center[0]=x;
		m_center[1]=y;
		m_center[2]=z;
	}
	
	void setAufpunkt(double x,double y,double z)
	{
		m_aufpunkt[0]=x;
		m_aufpunkt[1]=y;
		m_aufpunkt[2]=z;
	}
    
	void setWidthVector(double x,double y,double z)
	{
		m_vectorWidth[0]=x;
		m_vectorWidth[1]=y;
		m_vectorWidth[2]=z;
    }




	/// ---- DIFFERENCE VOLUME STUFF (ERROR ANALYSIS)
	bool generateAdvancedDiffContour();
	void loadVolumen(QString original,QString registed,QString difference);
	
	void loadVolumen(QString original,QString registed);

	std::vector<vtkVolume*>		getAnalyseVolumen(){return m_analyseVolumen;}
	std::vector<vtkActor*>		getAnalyseMeshActor(){return m_analyseMeshActor;}
	std::vector<vtkActor*>		getAnalyseContourActor(){return m_analyseContourActor;}

	bool getDifferenceVolumePresense(){return m_differenceVolumePresent;}
	bool getKeyPressEventsEnabled(){return m_keyPressEnabled;}
	bool getShowVolumeMeshes(){return m_showVolumeMeshes;}
	void setShowVolumeMeshes(bool show);
	int * getIndexOfVolume(){return &m_indexOfVolume;}
	void setIndexOfVolume(int index)
	{
		m_indexOfVolume=index;
		m_textActor->SetInput(VolumeNames.at(m_indexOfVolume).toAscii());

		if (m_indexOfVolume==0)
			m_textActor->GetProperty()->SetColor(0,0,0);

		if (m_indexOfVolume==1)
			m_textActor->GetProperty()->SetColor( m_opts.colorRegistered ); // was: 0,0,1
	
		if (m_indexOfVolume==2)
			m_textActor->GetProperty()->SetColor( m_opts.colorOriginal ); // was: 1,0,0
		m_renderWindow->Render();
	}

	void SetErrorIsoValue(double value){m_isoValueForError=value;}
	void SetMeshsIsoValue(double value){m_isoValueForMeshes=value;}

	vtkTextActor* getText(){return m_textActor;}
	void setErrorMeshVisibility(int index,bool visible);
			void showMagnitudeBar(bool visible);

protected:
	void createContour();
	void createSilhouette();
	void createOutline();

	void removeWarpVisActors();
	void addWarpVisActors();

private:

	/// Color options of visualization
	struct Options
	{
		Options()
			{
				// Green Color for original Mesh
				colorOriginal[0] = .7;
				colorOriginal[1] = 1;
				colorOriginal[2] = .7;

				// Blue Color for registered Mesh
				colorRegistered[0] = .7;
				colorRegistered[1] = .7;
				colorRegistered[2] = 1;

				// Red Color for difference Mesh
				colorDifference[0] = 1;
				colorDifference[1] = 0;
				colorDifference[2] = 0;
			}

		double* getDarkTextColor( double* color, float dampeningFactor=0.2 )
			{
				static double dampenedColor[3];
				dampenedColor[0] = dampeningFactor * color[0];
				dampenedColor[1] = dampeningFactor * color[1];
				dampenedColor[2] = dampeningFactor * color[2];
				return dampenedColor;
			}

		double colorOriginal  [3],
			   colorRegistered[3],
			   colorDifference[3];
	} m_opts;

	/// ---- sync

	VarVisRender * m_syncObject;


	/// ---- DIFFERENCE VOLUME STUFF (ERROR ANALYSIS)

	// meshes of this analyse
	std::vector<vtkActor*>		     m_analyseMeshActor; 
	std::vector<vtkActor*>		     m_analyseContourActor; 
	std::vector<vtkActor*>		     m_analyseDiffMeshActor;

	QStringList VolumeNames;
	// need the same stuff vor volume;
	std::vector<vtkVolume*>				m_analyseVolumen;
	
	int m_indexOfVolume;
	double m_isoValueForMeshes;
	double m_isoValueForError;
	bool m_volumeScalarBarVisibility;
	vtkImageData *m_diffImage;

	
	VTKPTR<vtkTextActor> m_textActor;
    VTKPTR<vtkScalarBarActor> m_volumeScalarBar;	
	
	bool m_firstLoad;

	// Functions
	void generateRefMeshesData();
	void generateSampleData();
	
	void destroy();
	void init();

	// Std Variables
	int m_resolution[3];
	double m_elementSpacing[3];
	double m_center[3];
	double m_aufpunkt[3];
	double m_vectorWidth[3];
	double m_vectorHeight[3];

	//Status Vars
	bool m_showVolumeMeshes;
	bool m_differenceVolumePresent;
	bool m_keyPressEnabled;
	bool m_meshPresent;
	bool m_samplesPresent;
	bool m_volumePresent;
	bool m_paused;
	bool m_referenceLoaded;
	bool m_stoped;
	bool m_useResolutionScaling;
	bool m_saveLocalReferenceModel;
	bool m_useGaussianSmoothing;
	bool m_showVolume;
	bool m_showReference;
	bool m_showMesh;
	bool m_showSamples;
	bool m_showSilhouette;
	bool m_showRoi;
	bool m_showCluster;
	bool m_showGlyph;
	bool m_showBars;
	int m_RoiIndex;
	bool m_bool_isRoiRender;
	// Visualisation Property Vars
	double m_glyphAutoScaling;
	double m_glyphSize;
	double m_gaussionRadiusValue;
	double m_samlePointSize;
	double m_isovalue;	///< current isovalue
	QString m_loadedReferenceName;
	int m_sampleRange;
	double m_centroidNum;

	// Helper Vars
	GlyphVisualization *m_warpVis;
	VectorfieldClustering *m_Cluster;
	
	//  REGION OF INTEREST
	std::vector<vtkActor*>		     m_roiActors; 
	std::vector<vtkPolyDataMapper*>  m_roiMappers; 
    std::vector<vtkSphereSource*>	 m_roiList; 

	// List for reference meshes / deformed meshes
	std::vector<vtkPolyData*>		    m_referenceMeshes; 
	std::vector<vtkActor*>			    m_referenceActors; 
	std::vector<vtkPolyDataMapper*>     m_referenceMappers; 
	std::vector<vtkPolyDataMapper*>     m_silhMappers;
	std::vector<vtkActor*>		    	m_silhActors; 
	std::vector <vtkDoubleArray*>       m_warpDataIndex;
	std::vector<vtkPolyDataSilhouette*> m_referenceSilhouette;
	std::vector<vtkUnsignedCharArray*>  m_warpColorIndex;
	QSlider							*m_frameSlider;
	QComboBox						*m_selectionBox;
	QString							m_localReference;

	// Volume Vars
	VTKPTR<vtkColorTransferFunction>	m_colorFunc;
	VTKPTR<vtkPiecewiseFunction    >	m_opacityFunc;
	VTKPTR<vtkRenderer             >	m_renderer;
	VREN_VOLUME_MAPPER	  *m_volumeMapper;
	vtkVolume             *m_volume;
	VTKPTR<vtkImageData>   m_VolumeImageData;
	
	//Cluster
	vtkPolyData			    *m_ClusterVolumeData;
	vtkActor                *m_ClusterVolumeActor;

	// mesh with marchingCubes
	vtkMarchingCubes		*m_contour;
	vtkPolyData				*m_mesh;
	vtkPolyDataMapper		*m_meshMapper;
	vtkActor				*m_meshActor;
	vtkPolyData             *m_warpMesh;
	vtkPolyDataMapper		*m_contMapper;
	vtkActor				*m_contActor;
	vtkPolyDataNormals		*m_normals;
	vtkPolyData			    *reference_mesh;

	// surface with contour filter (OBSOLETE?)
	vtkContourFilter   *m_isoContour;
	vtkPolyDataMapper  *m_isoMapper;
	vtkActor           *m_isoActor;

	// surface outline (i.e. bounding box)
	vtkOutlineFilter  *m_outline;
	vtkPolyDataMapper *m_outlineMapper;
	vtkActor          *m_outlineActor;

	// Point Data Sample
	vtkPolyData				*m_clusterSamples;
	vtkPolyData			    *m_pointPolyData;
	vtkActor                *m_samplerActor;
	vtkPolyDataMapper		*m_samplerMapper;

	//Silhoutte
	vtkPolyDataSilhouette  *m_silhouette;
	vtkActor			   *m_silhouetteActor;
	vtkPolyDataMapper	   *m_silhouetteMapper;

	// Gaussion Volume Filter 
	vtkImageGaussianSmooth *m_gaussianSmoothFilter;
	
	// animation stuff
	vtkActor                *m_animationActor;
	vtkPolyDataMapper		*m_animationMapper;
	vtkPolyData				*m_animationMesh;
	vtkRenderWindow			*m_renderWindow;

signals:
	void statusMessage(QString);

};


#endif  // VARVISRENDERER_H
