#include "VarVisRender.h"
#include "GlyphVisualization.h"
#include "PointSamplerFilter.h"
#include "GlyphInvertFilter.h"

// VarVis render includes
#include <vtkPolyDataConnectivityFilter.h>
#include <vtkRenderLargeImage.h>
#include <vtkPNGWriter.h>
#include <vtkMetaImageWriter.h>
#include <vtkImageData.h>
#include <vtkImageShiftScale.h>
#include <vtkMatrix4x4.h>
#include <vtkTransform.h>
#include <vtkViewPort.h>
#include <vtkDataArray.h>
#include <vtkCoordinate.h>
#include <vtkProperty2D.h>
#include <vtkPolyDataReader.h>
#include <vtkTextActor.h>
#include <vtkScalarBarActor.h>
#include <vtkPolyDataWriter.h>
#include <vtkMatrix3x3.h>
#include <vtkXMLPPolyDataWriter.h>
#include <vtkTextProperty.h>
// CueAnimator / vtkAnimationCueObserver includes
#include <vtkAnimationCue.h>
#include <vtkAnimationScene.h>
#include <vtkCommand.h>
// VTK Volume mapper
#ifdef VREN_GPU_RAYCASTER
 #include <vtkGPUVolumeRayCastMapper.h>
 #define VREN_VOLUME_MAPPER vtkGPUVolumeRayCastMapper
#else
 #include <vtkVolumeTextureMapper3D.h>
 #define VREN_VOLUME_MAPPER vtkVolumeTextureMapper3D
#endif

#include <QFileInfo>
#include <QMessageBox>
#include <QString>
#include <QCoreApplication>

#include <limits>

// Disable some Visual Studio warnings:
// C4018 '<' : signed/unsigned mismatch
#pragma warning(disable:4018)
//=============================================================================
//	Helper classes / functions in own namespace
//=============================================================================

// dirty but avoids namespace pollution and possible conflicting class names
namespace VarVisRender_Helpers {

 VTKPTR<vtkImageData> loadImageData2( const char* filename );
 void computeImageDataMagnitude2( vtkImageData* img, float* mag_min_, float* mag_max_ );


//-----------------------------------------------------------------------------
//	CueAnimator
//-----------------------------------------------------------------------------
class CueAnimator
{
public:
	CueAnimator()
	{
		this->Mesh = 0;
		this->animationMesh=0;
		this->Mapper = 0;
		this->Actor = 0;
		this->index=0;
		this->slider=0;
	}
	~CueAnimator()
	{
		this->Cleanup();
	}
	
	void setMesh(vtkPolyData * warpMesh)
	{
		this->Mesh=warpMesh;
	}
	
	void setWarpData(std::vector<vtkDoubleArray*> *warpData)
	{
		this->warpData=warpData;
	}
	
	void setSlider (QSlider * slider)
	{
		this->slider=slider;
	}
	void setColorData(std::vector<vtkUnsignedCharArray*> *colorData)
	{
		this->colorData=colorData;
	}

	void setPaused(bool *pause){this->paused=pause;}
	void setStoped(bool *stoped){this->stoped=stoped;}
   // Create an Animation Cue.
	void setCue(vtkAnimationCue* cue){this->cue= cue;}
	
	void StartCue(vtkAnimationCue::AnimationCueInfo* info, vtkRenderer* ren)
	{
		if (cuePresent)
		{
			ren->RemoveActor(this->Actor);
			this->Cleanup();
		}
		this->index=0;
		this->runLeft=false;
		*(this->paused)=false;
		*(this->stoped)=false;
		// prepare animation Mesh 
		this->animationMesh = vtkPolyData::New();
		this->animationMesh->ShallowCopy(this->Mesh);
		this->animationMesh->GetPointData()->AddArray(this->warpData->at(0));
		this->animationMesh->GetPointData()->SetActiveVectors("warpData");
		this->animationMesh->GetPointData()->GetArray("MeshColorData");
		this->animationMesh->GetPointData()->AddArray(this->colorData->at(0));

		vtkSmartPointer<vtkWarpVector> warpVector = vtkSmartPointer<vtkWarpVector>::New();
		warpVector->SetInput(this->animationMesh);
		warpVector->Update();
		this->animationMesh->ShallowCopy(warpVector->GetPolyDataOutput());

		//set Mapper and Actor with Values
		this->Mapper = vtkPolyDataMapper::New();
		this->Mapper->SetInput(this->animationMesh);
		this->Mapper->SelectColorArray("MeshColorData");
		this->Mapper->ScalarVisibilityOn();
		this->Mapper->SetScalarModeToUsePointFieldData();
		this->Actor = vtkActor::New();
		this->Actor->SetMapper(this->Mapper);
		ren->AddActor(this->Actor);
		ren->Render(); // paint 
		cuePresent=true;
	}

	void Tick(vtkAnimationCue::AnimationCueInfo* info, vtkRenderer* ren)
	{
		// let the application be able to handel signals, like mouse 
		if((*stoped))
		{
			cue->Finalize();
			
		}
		QCoreApplication::processEvents();
		if (!(*paused) && !(*stoped))
		{
			// make slider run from left to right 
			// when right index is matched run 
			// from rigth to left
			if (index==0)
				runLeft=false;
			if (index==20)
				runLeft=true;

			if (runLeft==false)
				index++; 

			if (runLeft==true)
				index--;

			// set animation mesh Values 
			this->animationMesh->ShallowCopy(this->Mesh);
			this->animationMesh->GetPointData()->AddArray(this->warpData->at(index));
			this->animationMesh->GetPointData()->SetActiveVectors("warpData");

			// genereate the deformed meshes 
			vtkSmartPointer<vtkWarpVector> warpVector = vtkSmartPointer<vtkWarpVector>::New();
			warpVector->SetInput(this->animationMesh);
			warpVector->Update();
			this->animationMesh->ShallowCopy(warpVector->GetPolyDataOutput());
			this->animationMesh->GetPointData()->AddArray(this->colorData->at(index));
			this->slider->setValue(index);
			ren->Render();
			
		}
		else if(!(*stoped))
		{
		// set animation mesh Values 
			this->animationMesh->ShallowCopy(this->Mesh);
			this->animationMesh->GetPointData()->AddArray(this->warpData->at(this->slider->value()));
			this->animationMesh->GetPointData()->SetActiveVectors("warpData");

			// genereate the deformed meshes 
			vtkSmartPointer<vtkWarpVector> warpVector = vtkSmartPointer<vtkWarpVector>::New();
			warpVector->SetInput(this->animationMesh);
			warpVector->Update();
			this->animationMesh->ShallowCopy(warpVector->GetPolyDataOutput());
			this->animationMesh->GetPointData()->AddArray(this->colorData->at(this->slider->value()));
		
			ren->Render();
		
		}
		
	}
	void EndCue(vtkAnimationCue::AnimationCueInfo *info, vtkRenderer* ren)
	{
		// animation runned to the end of cue without user interferance
		ren->RemoveActor(this->Actor);
		this->Cleanup();
	}

protected:
	// Variables
	vtkPolyData		  *Mesh;
	vtkPolyData		  *animationMesh;
	vtkPolyDataMapper *Mapper;
	vtkActor		  *Actor;
	QSlider			  *slider;
	vtkAnimationCue   *cue;
	int index;
	bool *paused;
	bool *stoped;
	bool runLeft;	
	std::vector <vtkDoubleArray*>		*warpData;
	std::vector <vtkUnsignedCharArray*> *colorData;
    bool cuePresent;
	void Cleanup()
	{
		if (this->animationMesh)
			this->animationMesh->Delete();
		this->animationMesh = 0;
	
		if (this->Mapper)
			this->Mapper->Delete();
		this->Mapper = 0;
	
		if (this->Actor)
			this->Actor->Delete();
		this->Actor = 0;

	}
};

//-----------------------------------------------------------------------------
//	vtkAnimationCueObserver
//-----------------------------------------------------------------------------
class vtkAnimationCueObserver : public vtkCommand
{
public:
	static vtkAnimationCueObserver* New()
	{
		return new vtkAnimationCueObserver;
	}

	virtual void Execute(vtkObject* caller, unsigned long event, void* calldata)
	{
		if (this->Animator && this->Renderer)
		{
			switch(event)
			{
				
				case vtkCommand::StartAnimationCueEvent:
				Animator->StartCue((vtkAnimationCue::AnimationCueInfo*) calldata, this->Renderer);
				break;
				case vtkCommand::EndAnimationCueEvent:
				Animator->EndCue((vtkAnimationCue::AnimationCueInfo*) calldata,this->Renderer);
				break;
				case vtkCommand::AnimationCueTickEvent:
				Animator->Tick((vtkAnimationCue::AnimationCueInfo*) calldata, this->Renderer);
				break;
			}
		}
		if (this->RenWin)
			this->RenWin->Render(); 

	}

   vtkRenderer* Renderer;
   vtkRenderWindow* RenWin;
   CueAnimator* Animator;

protected:
	vtkAnimationCueObserver()
	{
		this->Animator = 0;
		this->Renderer = 0;
		this->RenWin   = 0; 
	}

};

} // namespace VarVisRender_Helpers

using namespace VarVisRender_Helpers;


//=============================================================================
//	Implementation
//=============================================================================

//-----------------------------------------------------------------------------
//	VarVisRender
//-----------------------------------------------------------------------------

VarVisRender::~VarVisRender()
{
	destroy();
}

VarVisRender::VarVisRender()
		: m_isovalue( 16 )		 
{
	m_syncObject=0;
	m_isoValueForMeshes=16;
	m_isoValueForError=16;
	m_volumeScalarBarVisibility=true;
	m_indexOfVolume=0;
	m_Cluster=0;
	m_showVolumeMeshes=false;
	m_differenceVolumePresent=false;
	m_keyPressEnabled=false;
	m_useResolutionScaling=false;  
	// Per default no scaling in mhdFiles, when a scaleFactor is set then true.
	// Helps to use varvis alone 
	m_saveLocalReferenceModel=false;
	m_meshPresent=false;
	m_samplesPresent=false;
	m_volumePresent=false;
	m_glyphAutoScaling=true;
	m_glyphSize=1.0;
	m_showSilhouette=false;
	m_showMesh=true;
	m_showReference=false;
	m_showVolume=false;
	m_showSamples=false;
	m_showRoi=true;
	m_firstLoad=false;
	m_bool_isRoiRender=false;
#if 0
	m_useGaussianSmoothing=false;
	m_gaussionRadiusValue=1.5;
#else
	m_useGaussianSmoothing=true;
	m_gaussionRadiusValue=0.5;
#endif
	m_sampleRange=2500;
	m_samlePointSize=2.5;
	m_useResolutionScaling=false;
		
	m_centroidNum=0.05;
	m_referenceLoaded=false;

	// Setup smart pointers
	m_renderer        = VTKPTR<vtkRenderer             >::New();
	m_colorFunc       = VTKPTR<vtkColorTransferFunction>::New();
	m_opacityFunc     = VTKPTR<vtkPiecewiseFunction    >::New();
	m_VolumeImageData = VTKPTR<vtkImageData            >::New();		

	// Setup dynamic pointers
	init();

	// Setup color mapping
	// a) setup transfer mapping scalar value to opacity		
	m_opacityFunc->AddPoint(    0,  0 );
	m_opacityFunc->AddPoint(  255,  1 );

	// b) setup transfer mapping scalar value to color		
	m_colorFunc->AddRGBPoint(    0, 0,0,0 );
	m_colorFunc->AddRGBPoint(  255, 1,1,1 );
	m_colorFunc->HSVWrapOff();
	m_colorFunc->SetColorSpaceToHSV();
}

void VarVisRender::init()
{
	// BEWARE!  all of the vars below are 
	// NOT SMARTPOINTER , so you have to delete them 
	// when you are done !!!!

	// make new Vars
	m_volumeMapper = VREN_VOLUME_MAPPER::New();
	m_volume       = vtkVolume::New();
	
	// marching cube mesh
	m_contour      = vtkMarchingCubes ::New();
	m_contMapper   = vtkPolyDataMapper::New();
	m_contActor    = vtkActor         ::New();
	
	// surface with contour filter
	m_isoContour = vtkContourFilter  ::New();
	m_isoMapper  = vtkPolyDataMapper ::New();
	m_isoActor   = vtkActor          ::New();
	m_normals	 = vtkPolyDataNormals::New();

	// mesh to work with
	m_meshMapper    = vtkPolyDataMapper::New();
	m_meshActor     = vtkActor         ::New();
	m_mesh		    = vtkPolyData	   ::New();
	
    // point sampler
	m_samplerActor    = vtkActor         ::New();
	m_pointPolyData   = vtkPolyData		 ::New();
	m_samplerMapper   = vtkPolyDataMapper::New();

	// silhouette for working mesh (mean estimate)
	m_silhouette	   = vtkPolyDataSilhouette::New();
	m_silhouetteActor  = vtkActor			  ::New();
	m_silhouetteMapper = vtkPolyDataMapper    ::New();

	// surface outline (i.e. bounding box)
	m_outline       = vtkOutlineFilter   ::New() ;
	m_outlineMapper = vtkPolyDataMapper  ::New();
	m_outlineActor  = vtkActor		     ::New();

	// Gaussian smoothing
	m_gaussianSmoothFilter= vtkImageGaussianSmooth::New();

	// animation stuff
	m_animationMesh   = vtkPolyData       ::New();
	m_animationActor  = vtkActor		  ::New();
	m_animationMapper = vtkPolyDataMapper ::New();
}

// -- destroy() --
void VarVisRender::destroy()
{
	// do unload
	//a) Remove all Actors from Renderer
	if (getWarpVis())
	{
		getRenderer()->RemoveActor( getWarpVis()->getVisActor() );
		getRenderer()->RemoveActor( getWarpVis()->getScalarBar() );
		getRenderer()->RemoveActor( getWarpVis()->getScalarBarVector() );
		delete m_warpVis;
		m_warpVis=0;
	}

	m_renderer->RemoveActor(m_animationActor);
	m_renderer->RemoveActor(m_contActor);
	m_renderer->RemoveActor(m_isoActor);
	m_renderer->RemoveActor(m_meshActor);
	m_renderer->RemoveActor(m_outlineActor);
	m_renderer->RemoveActor(m_samplerActor);
    m_renderer->RemoveActor(m_silhouetteActor);
	for (int iA=0;iA<m_referenceActors.size();iA++)
		m_renderer->RemoveActor(m_referenceActors.at(iA));
	
	for (int iA=0;iA<m_silhActors.size();iA++)
		m_renderer->RemoveActor(m_silhActors.at(iA));

	for (int iA=0;iA<m_roiActors.size();iA++)
		m_renderer->RemoveActor(m_roiActors.at(iA));
	

	for (int iA=0;iA<m_referenceActors.size();iA++)
		m_renderer->RemoveActor(m_silhActors.at(iA));

	// delete Vectors 
	for (int iA=0;iA<m_referenceActors.size();iA++)
	{
		m_referenceActors.at(iA)->Delete();
		m_referenceMappers.at(iA)->Delete();
		m_referenceMeshes.at(iA)->Delete();
	}
	m_referenceActors.clear();
	m_referenceMappers.clear();
	m_referenceMeshes.clear();

	for (int iA=0;iA<m_silhActors.size();iA++)
	{
		m_silhActors.at(iA)->Delete();
		m_silhMappers.at(iA)->Delete();
	}
	m_silhActors.clear();
	m_silhMappers.clear();
	for (int iA=0;iA<m_roiActors.size();iA++)
	{
		m_roiActors.at(iA)->Delete();
		m_roiMappers.at(iA)->Delete();
		m_roiList.at(iA)->Delete();
	}
	m_roiActors.clear();
	m_roiMappers.clear();
	m_roiList.clear();

	m_volumeMapper->Delete();
	
	m_volume->Delete();  
	
	// marching cube mesh
	m_contour->Delete();  
	m_contMapper->Delete();
	m_contActor->Delete(); 
	
	// surface with contour filter
	m_isoContour->Delete();
	m_isoMapper->Delete(); 
	m_isoActor->Delete();  
	m_normals->Delete();	

	// mesh to work with
	m_meshMapper->Delete();
	m_meshActor->Delete(); 
	m_mesh->Delete();		
	
    // point sampler
	m_samplerActor->Delete();
	m_pointPolyData->Delete(); 
	m_samplerMapper->Delete(); 

	// silhouette for working mesh (mean estimate)
	m_silhouette->Delete();	
	m_silhouetteActor->Delete();
	m_silhouetteMapper->Delete();

	// surface outline (i.e. bounding box)
	m_outline->Delete();      
	m_outlineMapper->Delete(); 
	m_outlineActor->Delete();

	// gaussian smoothing
	m_gaussianSmoothFilter->Delete();

	// animation stuff
	m_animationMesh->Delete(); 
	m_animationActor->Delete();
	m_animationMapper->Delete();

	m_roiActors.clear(); 
	m_roiMappers.clear(); 
    m_roiList.clear(); 
	m_referenceMeshes.clear(); 
	m_referenceActors.clear(); 
	m_referenceMappers.clear(); 
	m_silhMappers.clear();
	m_silhActors.clear(); 
	m_warpDataIndex.clear();
	m_referenceSilhouette.clear();
	m_warpColorIndex.clear();
	
	clearRoi();

	delete m_Cluster;
	m_Cluster = NULL;
}

void VarVisRender::removeWarpVisActors()
{
	getRenderer()->RemoveActor( getWarpVis()->getVisActor() );
	getRenderer()->RemoveActor( getWarpVis()->getScalarBar() );
	//getRenderer()->RemoveActor( getWarpVis()->getScalarBarVector() );	
}

void VarVisRender::addWarpVisActors()
{
	getRenderer()->AddActor(getWarpVis()->getVisActor());
	getRenderer()->AddActor(getWarpVis()->getScalarBar());
	//getRenderer()->AddActor(getWarpVis()->getScalarBarVector() );
}

//-- Clustering --
void VarVisRender::showCluster(bool visible)
{
	m_showCluster=visible;
	if (m_Cluster)
	{
		for( int iA=0;iA<m_Cluster->GetActors().size();iA++)
			m_Cluster->GetActors()[iA]->SetVisibility(visible);

		m_renderWindow->Render();
	}
}
void VarVisRender::clusterVolumeGlyphs()
{
	
	//1] we need the volume image data with ijk components
	//   save the points in a list of points , then we 
	//   just need to create glyph visualisation on these points 
	if (!getWarpVis())
	{
		cout<<"Warning no WarpField loaded -> cancel!"<<endl;
		emit statusMessage("Warning no WarpField loaded -> cancel!");
		return;
	}
	emit statusMessage("Creating Volume Point Data...");
	cout<<"Creating Volume Point Data...";
	int *volumeResolution=m_VolumeImageData->GetDimensions();
	vtkCellArray      *volumeVertex= vtkCellArray::New();
	vtkDoubleArray    *volumeVectors = 	vtkDoubleArray  ::New();
	volumeVectors->SetNumberOfComponents(3);
	volumeVectors->SetName("volumeVectors");

	vtkPoints *volumePoints= vtkPoints::New();
	for( int z=0; z < volumeResolution[2]; ++z )
		for( int y=0; y < volumeResolution[1]; ++y )
			for( int x=0; x < volumeResolution[0]; ++x )
			{
				int ijk[3];
				ijk[0]=x;
				ijk[1]=y;
				ijk[2]=z;

				// get the point at the grid position;
				double *point=m_VolumeImageData->GetPoint(
						m_VolumeImageData->ComputePointId(ijk));

				double pointsValue= m_VolumeImageData->GetScalarComponentAsDouble (x, y, z, 0);							
				
				
				// see if number of points and vectors is to big for actor to handle it 1
				if(pointsValue> 0 ) // the weight in this point > 0 push to list 
				{			
					vtkIdType pid[1];
					pid[0]= volumePoints->InsertNextPoint ( point[0], point[1], point[2] );
					volumeVertex->InsertNextCell ( 1,pid);
				}		
			}				
	cout<<"done"<<endl;
	cout<<"Creating Volume Vector Data...";
	emit statusMessage("Creating Volume Vector Data...");
	m_ClusterVolumeData= vtkPolyData       ::New();
	m_ClusterVolumeData->SetPoints(volumePoints);
	m_ClusterVolumeData->SetVerts(volumeVertex);

	// now we have the data 
	// proceed with glyphVisualisation  on this data
	// (generate the Vectors for this data from loaded warp file)
	vtkImageData * source= getWarpVis()->getImageData();
	VTKPTR<vtkProbeFilter> probe = VTKPTR<vtkProbeFilter>::New();
	probe->SetInput(m_ClusterVolumeData);
	probe->SetSource(source);
	probe->Update();

	VTKPTR<vtkFieldDataToAttributeDataFilter> fd2ad_deformMesh = VTKPTR<vtkFieldDataToAttributeDataFilter>::New();
	fd2ad_deformMesh->SetInput( probe->GetOutput());
	fd2ad_deformMesh->SetInputFieldToPointDataField();
	fd2ad_deformMesh->SetOutputAttributeDataToPointData();
	//const char* scan = "MetaImage";
	// WARNING: Assume that vectorfield data is given in array[0] of vtkImageData!	
	fd2ad_deformMesh->SetVectorComponent(0,source->GetPointData()->GetArrayName(0),0);
	fd2ad_deformMesh->SetVectorComponent(1,source->GetPointData()->GetArrayName(0),1);
	fd2ad_deformMesh->SetVectorComponent(2,source->GetPointData()->GetArrayName(0),2);
	fd2ad_deformMesh->Update();
	
	// save the Vectors to PolyData
	m_ClusterVolumeData->GetPointData()->SetVectors(
		fd2ad_deformMesh->GetPolyDataOutput()->GetPointData()->GetVectors());

	if(!m_Cluster){
		m_Cluster = new VectorfieldClustering();
		m_Cluster->setRenderer(m_renderer);
	}
	m_Cluster->setPointData(m_ClusterVolumeData);
	m_Cluster->setNumberOfCentroids(200);
	m_Cluster->setEpsilon(0.001);
	m_Cluster->calculateW(m_resolution[0],m_resolution[1],m_resolution[2]);
	
	cout<<"done"<<endl;
	// do 3dVolume Clustering 
	cout<<"Creating Cluster Data...";
	emit statusMessage("Creating Cluster Data...");
	m_Cluster->generate3DClustering();
	cout<<"done"<<endl;
	emit statusMessage("Ready");
}

void VarVisRender::setClusterVisibility(bool visible)
{
	if(m_Cluster)
	{
		m_showCluster=visible;
		int size=m_Cluster->GetActors().size();
		for( int iA=0;iA<m_Cluster->GetActors().size();iA++)
			m_Cluster->GetActors()[iA]->SetVisibility(visible);
	}

}

void VarVisRender::clusterGlyphs()
{
	if (!getWarpVis())
	{
		cout<<"Warning no WarpField loaded -> cancel!"<<endl;
		emit statusMessage("Warning no WarpField loaded -> cancel!");
		return;
	}
	emit statusMessage("Clustering ....");
	// generate a Cluster whene there is none
	if(!m_Cluster){
		m_Cluster = new VectorfieldClustering();
		m_Cluster->setRenderer(m_renderer);
	}

	for (int iA=0;iA<m_Cluster->GetClusterPoints().size();iA++)
	{
		m_Cluster->GetClusterPoints()[iA]->Delete();
		m_Cluster->GetClusterVerterx()[iA]->Delete();
		m_Cluster->GetClusterPolyData()[iA]->Delete();
	
	}
	for (int iA=0;iA<m_Cluster->GetActors().size();iA++)
	{
		m_renderer->RemoveActor(m_Cluster->GetActors()[iA]);
		m_Cluster->GetClusterMapper()[iA]->Delete();
		m_Cluster->GetActors()[iA]->Delete();
	}
	m_Cluster->clearData();
	
	m_Cluster->setPointData(getWarpVis()->getClusterData());
	m_clusterSamples=getWarpVis()->getClusterData();
	VTKPTR<vtkUnsignedCharArray> colors =VTKPTR<vtkUnsignedCharArray>::New();
	colors->SetName("sampleColors");
	colors->SetNumberOfComponents(3);
	colors->SetNumberOfTuples(m_pointPolyData->GetNumberOfPoints());

	// set the color off all points to black ! 
	m_pointPolyData->GetPointData()->RemoveArray("sampleColors");
	for (int iA=0;iA<m_pointPolyData->GetNumberOfPoints();iA++)
		colors->SetTuple3(iA,0,0,0);

	m_pointPolyData->GetPointData()->AddArray(colors);
	m_pointPolyData->Update();
	m_samplerMapper->Update();

	// cluster 

	// 1. find the points on image plane to Cluster
	std::vector<vtkIdType> pointIdList;
	pointIdList.reserve( m_clusterSamples->GetNumberOfPoints() / 3 );
	
	double cameraNormal[3];
	m_renderer->GetActiveCamera()->GetViewPlaneNormal( cameraNormal );

	for (int iA=0;iA<m_clusterSamples->GetNumberOfPoints();iA++)
	{
		double * normal = m_clusterSamples->GetPointData()->GetNormals()->GetTuple3(iA);
		double dotprod=cameraNormal[0]*normal[0]+cameraNormal[1]*normal[1]+cameraNormal[2]*normal[2];
		if( dotprod>0 )
			pointIdList.push_back(iA);
	}
	// generate 2d Coordinates
#if 0
	m_Cluster->clearPointsList();
	vtkCoordinate * myCoordinates=vtkCoordinate::New();
	myCoordinates->SetCoordinateSystemToWorld();
	std::vector<Point2> tempPointList;
	for (int iA=0;iA<pointIdList.size();iA++)
	{
		double *pointX=m_clusterSamples->GetPoint(pointIdList.at(iA));
		myCoordinates->SetValue(pointX[0],pointX[1],pointX[2]);
		int *pos=myCoordinates->GetComputedDisplayValue(m_renderer);

		Point2 tempPoint;
		tempPoint.setX(pos[0]);
		tempPoint.setY(pos[1]);
		tempPoint.setWorldCoordinates(pointX[0],pointX[1],pointX[2]);
		tempPoint.setPointID(pointIdList.at(iA));
		if (getWarpVis())
		{
			double *vector=m_clusterSamples->GetPointData()->GetVectors()->GetTuple3(pointIdList.at(iA));
			tempPoint.setVector(vector[0],vector[1],vector[2]);
		}
		tempPointList.push_back(tempPoint);
	}
	m_Cluster->setPointList(tempPointList);
	m_Cluster->setNumberOfCentroids(m_centroidNum*tempPointList.size());
	m_Cluster->setNumberOfPoints(tempPointList.size());
#else
	m_Cluster->setSamplePoints( m_clusterSamples, pointIdList );
	m_Cluster->setNumberOfCentroidsRelative( m_centroidNum );
#endif
	
	m_Cluster->generateCentroids();
	m_Cluster->setIterationDone(false);
	m_Cluster->setEpsilon(0.001);
	m_Cluster->calculateW(m_resolution[0],m_resolution[1],m_resolution[2]);
	cout<<"clustering ";
	for (int iA=0;iA<100;iA++)
	{	cout<<"|";
	if (!m_Cluster->isIterationDone())
			m_Cluster->clusterIt();
		else 
			break;
	}	
	cout<<"done!"<<endl;
	
	// generate Visualisation 
	m_Cluster->setVisualisationData(m_pointPolyData);
	m_Cluster->setVisualisationLut(getWarpVis()->getLutTangentialComponent());
	m_Cluster->setVisualisationMapper(m_samplerMapper);

	m_Cluster->Visualisate();
	m_renderWindow->Render();
	emit statusMessage("Ready");

}

#include <QInputDialog>

//
void VarVisRender::makeScreenShot(QString outputFileName)
{
	// since vtkRenderLargeImage uses tiling, a background gradient will produce
	// artifacts in the final image, so we better turn it off
	bool gradient = getRenderer()->GetGradientBackground();
	double bg[3]; double bg2[3];
	getRenderer()->GetBackground( bg );
	getRenderer()->GetBackground2( bg2 );
	getRenderer()->SetGradientBackground( false );
	getRenderer()->SetBackground( 0.0, 0.0, 0.0 );
	getRenderer()->SetBackground2( 0.0, 0.0, 0.0 );

	// Let the user choose a magnification factor
	bool ok;
	int factor = QInputDialog::getInteger( this, tr("sdmvis"),
		tr("Screenshot magnification factor"), 2, 1, 10, 1, &ok );
	if( !ok )
		return; // User cancelled

	VTKPTR<vtkRenderLargeImage> renderLarge = VTKPTR<vtkRenderLargeImage>::New();
	renderLarge->SetInput(getRenderer());
	renderLarge->Update();
	renderLarge->SetMagnification( factor );

	VTKPTR<vtkPNGWriter> writer= VTKPTR<vtkPNGWriter>::New();
	writer->SetInputConnection(renderLarge->GetOutputPort());
	writer->SetFileName(outputFileName.toAscii());
	writer->Write();

	// restore background setting
	getRenderer()->SetGradientBackground( gradient );
	getRenderer()->SetBackground( bg );
	getRenderer()->SetBackground2( bg2 );
}

void VarVisRender::setWarp(QString mhdFilename, bool backwardsField)
{
	// clear Cluster if present 
	emit statusMessage("Loading Warpfield...");
	if(m_Cluster)
		delete m_Cluster;
	m_Cluster=0;

	// generate Sample Point  Data 
	// when sample data is present do not generate new one 
	if (!m_samplesPresent) 
		generateSampleData();
	// add an empty Color to pointSample - polyData
	// it will be filled after clustering 
	VTKPTR<vtkUnsignedCharArray> colors =VTKPTR<vtkUnsignedCharArray>::New();
	colors->SetName("sampleColors");
	colors->SetNumberOfComponents(3);
	colors->SetNumberOfTuples(m_pointPolyData->GetNumberOfPoints());
	
	int numOfPOints=m_pointPolyData->GetNumberOfPoints();

	for (int iA=0;iA<m_pointPolyData->GetNumberOfPoints();iA++)
	{
		colors->SetTuple3(iA,0,0,0);
			
	}
	m_pointPolyData->GetPointData()->AddArray(colors);
	
	m_samplerMapper->SetInput(m_pointPolyData);
	m_samplerMapper->ScalarVisibilityOn();
	m_samplerMapper->SetScalarModeToUsePointFieldData();
	m_samplerMapper->SelectColorArray("sampleColors");
	m_samplerActor->SetMapper(m_samplerMapper);
	m_samplerActor->GetProperty()->SetPointSize(m_samlePointSize);
		
	m_renderer->AddActor(m_samplerActor);
	//m_renderer->ResetCamera();

	// clear warpVis if present 
	if (getWarpVis())
	{
		getRenderer()->RemoveActor( getWarpVis()->getVisActor() );
		getRenderer()->RemoveActor( getWarpVis()->getScalarBar() );
		getRenderer()->RemoveActor( getWarpVis()->getScalarBarVector() );
		delete m_warpVis;
		m_warpVis=0;
	}
	resetRefMeshes(); // reset reference mesches  generates data if not present!

	// load the warpfield with properties (backwardsfield on / off )
	VTKPTR<vtkImageData> img = loadImageData2( mhdFilename.toAscii() );
	VTKPTR<vtkImageData> img_data;
	VTKPTR<vtkGridTransform> gridTrans=VTKPTR<vtkGridTransform>::New();
	if(backwardsField)
	{
		qDebug() << "VarVisRender::setWarp() : Inverting warp in-place!";

		gridTrans->SetDisplacementGrid(img);
		gridTrans->SetInterpolationMode( VTK_GRID_CUBIC );
		gridTrans->SetInverseTolerance( 0.01 );
		gridTrans->SetInverseIterations( 2500 );
		gridTrans->Inverse();
		gridTrans->Update();
			
		// Note:
		// The following will not work:
		//		img_data=gridTrans->GetDisplacementGrid();
		// Inverse() only sets a flag and GetDisplacementGrid() will return
		// the original (and *not* inverted) transform grid. We have to use
		// vtkTransformToGrid to really compute the inverse vectorfield.
		cout << "*** Generateing Transformation Grid ...";
		VTKPTR<vtkTransformToGrid> t2g = VTKPTR<vtkTransformToGrid>::New();
		t2g->SetInput( gridTrans );
		t2g->SetGridExtent ( img->GetExtent() );
		t2g->SetGridSpacing( img->GetSpacing() );
		t2g->SetGridOrigin ( img->GetOrigin() );
		//t2g->SetGridScalarTypeToFloat(); 
		t2g->Update();
		cout << " Done ***" << endl;	
		img_data = t2g->GetOutput();
		img_data->GetPointData()->GetArray(0)->SetName("FOOBAR");
	}
	else 
		img_data=img;

	// load the data
	if( !img_data.GetPointer())
	{
		cout<<"Error loading volume"<<endl;
		cout<<"Error: Could not load specified warp field!"<<endl;
	}

	emit statusMessage("Setup vectorfield visualization...");
	delete getWarpVis();
	
	// -- tell vren about the warpfield -- 
	setWarpVis(new GlyphVisualization(
					img_data.GetPointer(),
					getMesh(),
					getReferenceMeshes(),
					getPointSamples() )
				);
	getWarpVis()->setImageData(img_data.GetPointer());
	getWarpVis()->setGlyphSize(m_glyphSize);
	getWarpVis()->setGlyphAutoScaleFactor(m_glyphAutoScaling);
	getWarpVis()->getGlyph3D()->SetScaleModeToScaleByVector();

	// add to visualization
	addWarpVisActors();
	

	setClusterSamples(getWarpVis()->getClusterData());

	//// clear cluster data if there is one
	//for (int iA=0;iA<m_Cluster->GetClusterPoints().size();iA++)
	//{
	//	m_Cluster->GetClusterPoints()[iA]->Delete() ;
	//	m_Cluster->GetClusterVerterx()[iA]->Delete() ;
	//	m_Cluster->GetClusterPolyData()[iA]->Delete();
	//	
	//}
	//for (int iA=0;iA<m_Cluster->GetActors().size();iA++)
	//{
	//	m_renderer->RemoveActor(m_Cluster->GetActors()[iA]);
	//	m_Cluster->GetClusterMapper()[iA]->Delete();
	//	m_Cluster->GetActors()[iA]->Delete();
	//	
	//}
	//m_Cluster->clearData();
	//
	// update Visibility of actors
	updateVisibility();
	m_samplesPresent=true;
	emit statusMessage("Ready");

}

void VarVisRender::pauseAnimation()
{
	m_paused=true;
}
void VarVisRender::stopAnimation()
{
	m_stoped=true;
}

void VarVisRender::startAnimation()
{
	if (m_referenceLoaded && getWarpVis())
	{
		m_paused=false;
		m_stoped=true;
		m_warpDataIndex.clear();
		m_warpColorIndex.clear();
		m_warpMesh=getWarpVis()->getVectorField();
		
		int size_of_steps=20;
		double step=(double)2 / size_of_steps;
		double strength=-1;

		vtkScalarsToColors* lut = NULL;
		vtkScalarsToColors* lut2 = NULL;

		vtkSmartPointer<vtkColorTransferFunction> hlut = vtkSmartPointer<vtkColorTransferFunction>::New();
		hlut->AddRGBPoint( 0, 0,0,1 );						// blue
		hlut->AddRGBPoint( size_of_steps/2,0.8,0.8,0.8 );	// gray
		hlut->AddRGBPoint( size_of_steps, 1,0,0 );			//red
		hlut->HSVWrapOff();
		hlut->SetColorSpaceToRGB();

		lut = hlut.GetPointer();
		// generate the deformation warps
		for (int iA=0;iA<size_of_steps+1;iA++)
		{
			double warpScale= getWarpVis()->getWarpScale();
			vtkDoubleArray* warpData =   vtkDoubleArray::New();
			warpData->SetNumberOfComponents(3);
			warpData->SetName("warpData");
			
			vtkUnsignedCharArray * colorData= vtkUnsignedCharArray::New();
			colorData->SetNumberOfComponents(3);
			colorData->SetName("MeshColorData");
			
			for (int iB=0;iB<m_warpMesh->GetNumberOfPoints();iB++)
			{
				double *color=lut->GetColor(iA);
				double r,g,b;
				r=color[0]*255;
				g=color[1]*255;
				b=color[2]*255;
				colorData->InsertTuple3(iB,r,g,b);
				
				double *warp = m_warpMesh->GetPointData()->GetVectors()->GetTuple3(iB);
				for (int iC=0;iC<3;iC++)
					warp[iC]=strength*warpScale*warp[iC];
				warpData->InsertNextTuple(warp);
			}

			m_warpDataIndex.push_back(warpData);
			m_warpColorIndex.push_back(colorData);
			strength+=step;
		}

		m_warpMesh->GetPointData()->AddArray(m_warpColorIndex[0]);

		VTKPTR<vtkAnimationCue> animationCue = VTKPTR<vtkAnimationCue>::New();
		vtkAnimationScene* scene = vtkAnimationScene::New();
		  
		scene->SetModeToSequence();
		scene->SetLoop(0);
		scene->SetFrameRate(50);
		scene->SetStartTime(0);
		scene->SetEndTime(100);

		// Create an Animation Cue.
		vtkAnimationCue* cue1 = vtkAnimationCue::New();

		cue1->SetStartTime(0);
		cue1->SetEndTime(19);
		scene->AddCue(cue1);

		// Create cue animator;
		static CueAnimator animator;  // FIXED: put static here to make CueAnimator instance persistent!
		animator.setMesh(m_warpMesh);
		animator.setWarpData(&m_warpDataIndex);
		animator.setColorData(&m_warpColorIndex);
		animator.setSlider(m_frameSlider);
		animator.setPaused(&m_paused);
		animator.setStoped(&m_stoped);
		animator.setCue(cue1);
		// Create Cue observer.
		vtkAnimationCueObserver* observer = vtkAnimationCueObserver::New();
		observer->Renderer = m_renderer;
		observer->Animator = &animator;
		observer->RenWin = getRenderWindow();

		cue1->AddObserver(vtkCommand::StartAnimationCueEvent, observer);
		cue1->AddObserver(vtkCommand::EndAnimationCueEvent, observer);
		cue1->AddObserver(vtkCommand::AnimationCueTickEvent, observer);

		scene->Play();
		scene->Stop();
	}
}
// -- setGlyphVisible --
void VarVisRender::setGlyphVisible(bool visible)
{
	m_showGlyph=visible;
	if (getWarpVis())
	getWarpVis()->getVisActor()->SetVisibility(visible);
}
// -- setSilhouetteVisible --
void VarVisRender::setSilhouetteVisible(bool visible)
{
	m_showSilhouette=visible;
	for (int iA=0;iA<m_silhActors.size();iA++)
		m_silhActors[iA]->SetVisibility(visible);
	getSilhouetteActor()->SetVisibility(visible);

}
// -- setRoiVisibility --
void VarVisRender::setRoiVisibility	 ( bool visible )
{
	m_showRoi= visible;
	for (int iA=0; iA<m_roiActors.size(); iA++)
	{
		m_roiActors.at(iA)->SetVisibility(m_showRoi);
	}
}
// -- setRefMeshVisibility --
void VarVisRender::setRefMeshVisibility(bool visible)
{
	m_showReference=visible;
	for (int iA=0;iA<m_referenceActors.size();iA++)
		m_referenceActors[iA]->SetVisibility(visible);
		
}
// -- setBarsVisibility --
void VarVisRender::setBarsVisibility(bool visible)
{
	m_showBars=visible;
	if (getWarpVis())
	{
		getWarpVis()->getScalarBar()->SetVisibility(visible);
		getWarpVis()->getScalarBarVector()->SetVisibility(visible);
	}

}
// -- setScaleValue --
void VarVisRender::setScaleValue(double value)
{
	// scale value , removes old Glyph actor,
	// set the svale Value , whitch resets the reference meshes 
	// and then creates the deformed meshes with updateWarpVis
	if (getWarpVis())
	{
		getWarpVis()->setScaleValue(value);
		m_renderer->RemoveActor(getWarpVis()->getVisActor());
		resetRefMeshes(); 
		getWarpVis()->updateWarpVis(getWarpVis()->getImageData(),m_mesh,m_referenceMeshes,m_pointPolyData);
		m_renderer->AddActor(getWarpVis()->getVisActor());

		setGlyphVisible(m_showGlyph);
		setBarsVisibility(m_showBars);
		updateVisibility();
	}
}
// -- generateRefMeshesData --
void VarVisRender::generateRefMeshesData()
{
	int x[3]={0,0,255};
	int y[3]={255,0,0};
	
	int count=0;
	//genereate multi meshes for deformation
	for (int iA=0;iA<2;iA++)
	{
		vtkPolyData * tempRefMesh = vtkPolyData::New();
		tempRefMesh->ShallowCopy(m_mesh);
		vtkUnsignedCharArray* tempReference_colors =vtkUnsignedCharArray::New();
	
		tempReference_colors->SetName("refColors");
		tempReference_colors->SetNumberOfComponents(3);
		tempReference_colors->SetNumberOfTuples(tempRefMesh->GetNumberOfPoints());
		
		for (int i = 0; i < tempRefMesh->GetNumberOfPoints() ;i++)
		{
			if (count==0)
				tempReference_colors->InsertTuple3(i,x[0],x[1],x[2]);
			if (count==1)
				tempReference_colors->InsertTuple3(i,y[0],y[1],y[2]);
		}
		tempRefMesh->GetPointData()->AddArray(tempReference_colors);
		m_referenceMeshes.push_back(tempRefMesh);

		vtkPolyDataMapper * tempMapper= vtkPolyDataMapper::New();

		tempMapper->SetInput(m_referenceMeshes[count]);
		tempMapper->ScalarVisibilityOn();
		tempMapper->SetScalarModeToUsePointFieldData();
		tempMapper->SelectColorArray("refColors");
		
		vtkActor * tempActor = vtkActor::New();
		tempActor->SetMapper(tempMapper);
		tempActor->GetProperty()->SetOpacity( 0.2 );
		m_renderer->AddActor(tempActor);

		m_referenceActors.push_back(tempActor);
		m_referenceMappers.push_back(tempMapper);

		// generate a silhoute for this one;
		vtkPolyDataSilhouette * tempSilh = vtkPolyDataSilhouette::New();
		tempSilh->SetInput(m_referenceMeshes[count]);
		tempSilh->SetCamera(m_renderer->GetActiveCamera());
		tempSilh->SetEnableFeatureAngle(60);
	
		//create mapper and actor for silouette
		vtkPolyDataMapper * silhMapper= vtkPolyDataMapper::New();
		vtkActor * silhActor = vtkActor::New();

		silhMapper->SetInputConnection(tempSilh->GetOutputPort());
		silhActor->SetMapper(silhMapper);

		if (count==0)
			silhActor->GetProperty()->SetColor(x[0], x[1], x[2]);
		if (count==1)
			silhActor->GetProperty()->SetColor(y[0], y[1], y[2]);
		
		silhActor->GetProperty()->SetLineWidth(1);
		m_renderer->AddActor(silhActor);

		m_silhActors.push_back(silhActor);
		m_silhMappers.push_back(silhMapper);
		m_referenceSilhouette.push_back(tempSilh);
		silhActor->SetVisibility(m_showSilhouette);
		count++;
	}
}
void VarVisRender::clearRoi()
{
	// remove actors  so we can delete the imtes
	for (int iA=0;iA<m_roiActors.size();iA++)
	{
		m_renderer->RemoveActor(m_roiActors.at(iA));
		m_roiActors.at(iA)->Delete();
		m_roiMappers.at(iA)->Delete();
		m_roiList.at(iA)->Delete();

	}
	m_roiActors.clear();
	m_roiMappers.clear();
	m_roiList.clear();
	m_volumePresent=false;
	m_meshPresent=false;
	m_selectionBox->clear();
}
// -- resetRefMeshes -- 
void VarVisRender::resetRefMeshes()
{
	// clear data 
	for (int iA=0;iA<m_referenceActors.size();iA++)
	{
		m_renderer->RemoveActor(m_referenceActors[iA]);
		m_renderer->RemoveActor(m_silhActors[iA]);
	}	
	m_samplesPresent=false;
	// does it delete the data inside the list or just the pointer to data ? oO 
	// we dont wanna have a memory leak ! TODO: FIND OUT
	m_referenceActors.clear(); 
	m_referenceMeshes.clear();
	m_referenceMappers.clear();
	m_referenceSilhouette.clear();
	m_silhActors.clear();
	m_silhMappers.clear();
	
	// create new data 	
	generateRefMeshesData();
}
// -- generateSampleData -- 
void VarVisRender::generateSampleData()
{
	// Generate Samples
	VTKPTR<PointSamplerFilter> sampler=VTKPTR<PointSamplerFilter>::New();
	sampler->SetInput(m_mesh);
	sampler->setNumberOfSamplingPoints(m_sampleRange);
	sampler->saveMeshNormas(true);
	sampler->Update();
	m_pointPolyData->DeepCopy(sampler->GetOutput());
	
	m_samplesPresent=true;
}
// -- setIsovalue -- 
void VarVisRender::setIsovalue( double value )
{
	if (m_referenceLoaded)
	{
		m_contour->SetValue( 0, value ); // Hounsfield units, bones approx.500-1500HU
		m_contour->Update(); // needed!

		m_mesh->ShallowCopy(m_contour->GetOutput());
		
		// add empty color Data ; 
		// data will be filled with vectorToMeshColorFilter
		VTKPTR<vtkUnsignedCharArray> colors =VTKPTR<vtkUnsignedCharArray>::New();
		colors->SetName("meshColors");
		colors->SetNumberOfComponents(3);
		colors->SetNumberOfTuples(m_mesh->GetNumberOfPoints());
		
		for (int iA=0;iA<m_mesh->GetNumberOfPoints();iA++)
		{
			colors->InsertTuple3(iA,200,200,200); // std gray color 
		}


		m_mesh->GetPointData()->AddArray(colors);
		m_isovalue=value;
		m_samplesPresent=false;
		if (getWarpVis())
		{
			removeWarpVisActors();
			
			generateSampleData();	// needed because  we have a new mesh -> new pointsampling
			resetRefMeshes();	
			// generate updateted Data
			getWarpVis()->updateWarpVis(getWarpVis()->getImageData(),m_mesh,m_referenceMeshes,m_pointPolyData);
			// add new actors

			addWarpVisActors();

			setSilhouetteVisible(m_showSilhouette);
			setBarsVisibility(m_showBars);
			setGlyphVisible(m_showGlyph);
			// also need to update cluster samples data 
			if (m_Cluster)
				m_Cluster->setPointData(getWarpVis()->getClusterData());

		}
		updateVisibility();
	}
}



void VarVisRender::saveROI(QString mhdFilename)
{
	using namespace std;

	QFileInfo someInfo(mhdFilename);
	
	if (someInfo.exists())
	{
		if( QMessageBox::question( this,"VarVis: Mask already exist",
		"Do you want to over write the MASK?",
		QMessageBox::Yes | QMessageBox::No,  QMessageBox::Yes )
		== QMessageBox::Yes )
		{
			QFile::remove(mhdFilename);
			
			QString rawmhdFile=mhdFilename;
				rawmhdFile.remove(".mhd");
				rawmhdFile.append(".raw");
			QFile::remove(rawmhdFile);

				QString localMhdFilename=mhdFilename;
				localMhdFilename.remove("MASK_");
				localMhdFilename.remove(".mhd");
				localMhdFilename.append("_LOCAL_REFERENCE.mhd");
				QFile::remove(localMhdFilename);
		
		QString localRawFileName=localMhdFilename.remove(".mhd");
		localRawFileName.append(".raw");
		QFile::remove(localRawFileName);
	
		
		}
		else 
			return;
	
	}

	// SAVE LOCAL REFERENCE MODEL ???? 
	if (m_saveLocalReferenceModel)
	{	
		int *resolution=m_VolumeImageData->GetDimensions();
		cout<<"Saving selected ROI to "<< mhdFilename.toStdString()<<endl;;
		cout<<"please Wait..."<<endl;
		emit statusMessage("Saving local Reference ....");
		for( int z=0; z < resolution[2]; ++z )
			for( int y=0; y <resolution[1]; ++y )
				for( int x=0; x < resolution[0]; ++x )
				{
					int ijk[3];
					ijk[0]=x;
					ijk[1]=y;
					ijk[2]=z;

					// get the point at the grid position;
					double *point=m_VolumeImageData->GetPoint(
							m_VolumeImageData->ComputePointId(ijk));
					

					// calculate point zugehoerigkeit 
					bool pointInside=false;
					double pointsValue=0.0;
					for (int iA=0;iA<m_roiList.size();iA++)
					{
						// get selection parameters
		
						double radius = m_roiList.at(iA)->GetRadius();
						double center[3];
						m_roiList.at(iA)->GetCenter(center);

						double pointInside= (point[0]-center[0])*(point[0]-center[0])+
											(point[1]-center[1])*(point[1]-center[1])+
											(point[2]-center[2])*(point[2]-center[2]);
			
						if( pointInside < radius*radius )
						{
							pointsValue= m_VolumeImageData->GetScalarComponentAsDouble (x, y, z, 0);							
							pointInside=true;
						}
					}
					m_VolumeImageData->SetScalarComponentFromDouble (x, y, z, 0, pointsValue);
				}				
	
		VTKPTR<vtkMetaImageWriter> writerLocal= VTKPTR<vtkMetaImageWriter>::New();
		writerLocal->SetInput(m_VolumeImageData);
		QString localMhdFilename=mhdFilename;
		localMhdFilename.remove("MASK_");
		localMhdFilename.remove(".mhd");
		localMhdFilename.append("_LOCAL_REFERENCE.mhd");
		m_localReference=localMhdFilename;
		writerLocal->SetFileName (localMhdFilename.toAscii());
		
		writerLocal->SetCompression(false);
		
		QString localRawFileName=localMhdFilename.remove(".mhd");
		localRawFileName.append(".raw");
		writerLocal->SetRAWFileName(localRawFileName.toAscii());
		
		writerLocal->Write(); 
	
	}
	

	
	cout<<"Saving selected ROI to "<< mhdFilename.toStdString()<<endl;;
	emit statusMessage("Saving mask ....");
	
	VTKPTR<vtkImageData>dataToWrite= VTKPTR<vtkImageData>::New();
	
	if (m_useResolutionScaling) // set the resolution of data to write
	{
		dataToWrite->SetDimensions(m_resolution[0],m_resolution[1],m_resolution[2]);
		dataToWrite->SetSpacing(m_elementSpacing[0],m_elementSpacing[1],m_elementSpacing[2]);
	}
	else
	{
		int *resolution=m_VolumeImageData->GetDimensions();
		double *spacing = m_VolumeImageData->GetSpacing();
		dataToWrite->SetDimensions(resolution[0],resolution[1],resolution[2]);
		dataToWrite->SetSpacing(spacing[0],spacing[1],spacing[2]);
		
	}
	
	
	int *resolution=dataToWrite->GetDimensions();
	cout << "Converting selection into volumetric mask... "<<endl;
	emit statusMessage("Converting selection into volumetric mask...");
	for( int z=0; z < resolution[2]; ++z )
		for( int y=0; y <resolution[1]; ++y )
			for( int x=0; x < resolution[0]; ++x )
			{
				int ijk[3];
				ijk[0]=x;
				ijk[1]=y;
				ijk[2]=z;

				// get the point at the grid position;
				double *point=dataToWrite->GetPoint(
						dataToWrite->ComputePointId(ijk));
				
	

				// calculate point zugehoerigkeit 
				bool pointInside=false;
				double pointsValue=0.0;
				for (int iA=0;iA<m_roiList.size();iA++)
				{
					// get selection parameters
	
					double radius = m_roiList.at(iA)->GetRadius();
					double center[3];
					m_roiList.at(iA)->GetCenter(center);

					double pointInside= (point[0]-center[0])*(point[0]-center[0])+
										(point[1]-center[1])*(point[1]-center[1])+
										(point[2]-center[2])*(point[2]-center[2]);
		
					if( pointInside < radius*radius )
					{
						
						pointsValue=1.0;
						pointInside=true;
					}
				}
				
				dataToWrite->SetScalarComponentFromDouble (x, y, z, 0, pointsValue);
			}
	
	// now change the saving format from uChar to float 
	VTKPTR<	vtkImageShiftScale> typeChanger= VTKPTR<vtkImageShiftScale>::New();
	typeChanger->SetInput(dataToWrite);
	typeChanger->SetOutputScalarTypeToFloat();
	typeChanger->Update();



	VTKPTR<vtkMetaImageWriter> writer= VTKPTR<vtkMetaImageWriter>::New();
	writer->SetInput(typeChanger->GetOutput());
	writer->SetFileName (mhdFilename.toAscii());
	
	writer->SetCompression(false);
	
	QString rawFileName=mhdFilename.remove(".mhd");
	rawFileName.append(".raw");
	writer->SetRAWFileName(rawFileName.toAscii());
	
	writer->Write(); 


	

cout<<"... done "<<endl;
emit statusMessage("Ready!");
}
// -- generateROI() -- 
void VarVisRender::generateROI()
{
	// generate a sphere 
	vtkSphereSource *roiSphere= vtkSphereSource::New();
	roiSphere->SetRadius(30.0);
	roiSphere->SetCenter(m_center[0],m_center[1],m_center[2]);
	roiSphere->SetPhiResolution(50);
	roiSphere->SetThetaResolution(50);

	
	m_roiList.push_back(roiSphere);
  // map to graphics library
  vtkPolyDataMapper* roiMapper = vtkPolyDataMapper::New();
  roiMapper ->SetInput(roiSphere->GetOutput());

  m_roiMappers.push_back(roiMapper);

  // actor coordinates geometry, properties, transformation
  vtkActor *roiActor = vtkActor::New();
  roiActor->SetMapper(roiMapper);
  roiActor->GetProperty()->SetColor(0,0,1); // sphere color blue 
  roiActor->GetProperty()->SetOpacity(0.3); // sphere color blue 
  m_roiActors.push_back(roiActor);

  m_renderer->AddActor(roiActor);
  m_renderWindow->Render(); // paint new sphere
}

// -- generateROI() Loaded from Config with Parameters from cfg -- 
void VarVisRender::generateROI(int index, double radius,double x,double y, double z)
{
	// generate a sphere 
	vtkSphereSource *roiSphere= vtkSphereSource::New();
	roiSphere->SetRadius(radius);
	roiSphere->SetCenter(x,y,z);
	roiSphere->SetPhiResolution(50);
	roiSphere->SetThetaResolution(50);

	
	m_roiList.push_back(roiSphere);
	// map to graphics library
	vtkPolyDataMapper* roiMapper = vtkPolyDataMapper::New();
	roiMapper ->SetInput(roiSphere->GetOutput());
	m_roiMappers.push_back(roiMapper);

	// actor coordinates geometry, properties, transformation
	vtkActor *roiActor = vtkActor::New();
	roiActor->SetMapper(roiMapper);
	roiActor->GetProperty()->SetColor(0,0,1); // sphere color blue 
	roiActor->GetProperty()->SetOpacity(0.3); // sphere color blue 
	m_roiActors.push_back(roiActor);

	m_renderer->AddActor(roiActor);
	m_renderWindow->Render(); // paint new sphere

	//TODO : add control Items 
	m_selectionBox->addItem("Roi - "+QString::number(index));
}

void VarVisRender::updateVisibility()
{
	setSilhouetteVisible(m_showSilhouette);
	setVolumeVisibility(m_showVolume);
	setRefMeshVisibility(m_showReference);
	setSamplesVisibility(m_showSamples);
	setMeshVisibility(m_showMesh);
	setRoiVisibility(m_showRoi);
	if (getWarpVis())
	{
		setGlyphVisible(m_showGlyph);
		setBarsVisibility(m_showBars);
		setClusterVisibility(m_showCluster);
	}
	m_renderWindow->Render();
}

double VarVisRender::saveMesh(QString fileName)
{
	// TODO: check if writer overwrites the Files
	VTKPTR<vtkPolyDataWriter> meshWriter=VTKPTR<vtkPolyDataWriter>::New();
	meshWriter->SetInput(m_mesh);
	
	meshWriter->SetFileName(fileName.toAscii());
	meshWriter->Write();

return m_isovalue;
}

int VarVisRender::savePointSamples(QString fileName)
{
	// TODO: check if writer overwrites the Files
	VTKPTR<vtkPolyDataWriter> pointsWriter=VTKPTR<vtkPolyDataWriter>::New();
	pointsWriter->SetInput(m_pointPolyData);
	pointsWriter->SetFileName(fileName.toAscii());
	pointsWriter->Write();
	return m_sampleRange;
}

// -- updateSamplePoints -- 
void VarVisRender::updateSamplePoints(int num)
{
	if (m_referenceLoaded)
	{
		m_sampleRange=num;
		resetRefMeshes();
		generateSampleData();
		m_samplerActor->GetProperty()->SetPointSize(m_samlePointSize);
		if (getWarpVis())
		{
			removeWarpVisActors();

			// generate updateted Data
			getWarpVis()->updateWarpVis(getWarpVis()->getImageData(),m_mesh,m_referenceMeshes,m_pointPolyData);

			addWarpVisActors();
			
			// set there visibility
			setGlyphVisible(m_showGlyph);
			setBarsVisibility(m_showBars);
		}
		updateVisibility();

	}
}

// -- setGlyphSize -- 
void VarVisRender::setGlyphSize(double value)
{
	if (getWarpVis())
	getWarpVis()->setGlyphSize(value); m_glyphSize=value;
}

void VarVisRender::setGlyphAutoScaling( bool b )
{
	if( getWarpVis() )
		getWarpVis()->setGlyphAutoScaling( b ); 
	m_glyphAutoScaling = b; 
}

void VarVisRender::createContour()
{
	emit statusMessage("VarVis: Generating contour mesh...");

	// create mesh from marching cubes
	m_contour->SetValue( 0, m_isovalue ); // Hounsfield units, bones approx.500-1500HU
	m_contour->SetComputeNormals( 1 );
	m_contour->Update();
		// Note: m_contour input is set in setVolume()

	vtkPolyData* contour = m_contour->GetOutput();
#if 0
	// try connectivityFilter for  Largest Region ,
	// maybe OBSOLETE  because iso value generates a fractal geometry,
	// where you wanna see also the regions not belong to the large region
	VTKPTR<vtkPolyDataConnectivityFilter> connectFilter = VTKPTR<vtkPolyDataConnectivityFilter>::New();
	connectFilter->SetInput(m_contour->GetOutput());
	connectFilter->SetExtractionModeToLargestRegion();

	contour = connectFilter->GetOutput();
#endif

	// mapper and actor for the marching cubes mesh 
	m_contMapper->SetInput( m_contour->GetOutput() );
	m_contMapper->ScalarVisibilityOff();
	m_contActor->SetMapper(m_contMapper);
	m_contActor->SetVisibility(false); // set contActor invisible (is just dummy data that is copied to mesh)
	
	// copy marching cube mesh to m_mesh polyData 
	m_mesh->ShallowCopy( contour );

	// Set standard gray color for mesh, else it will be colorised with colorToMeshFilter!
	VTKPTR<vtkUnsignedCharArray> colors =VTKPTR<vtkUnsignedCharArray>::New();
	colors->SetName("meshColors");
	colors->SetNumberOfComponents(3);
	colors->SetNumberOfTuples(m_mesh->GetNumberOfPoints());
	for (int iA=0;iA<m_mesh->GetNumberOfPoints();iA++)
		colors->InsertTuple3(iA,200,200,200); // std gray color 
	m_mesh->GetPointData()->AddArray(colors);

	// mapper and actor for standard contour mesh
	m_meshMapper->SetInput(m_mesh);
	m_meshMapper->ScalarVisibilityOn();
	m_meshMapper->SetScalarModeToUsePointFieldData();
	m_meshMapper->SelectColorArray("meshColors");
	m_meshActor->SetMapper(m_meshMapper);
	
	// add to renderer
	m_renderer->AddActor( m_contActor );
	m_renderer->AddActor( m_meshActor );
}

void VarVisRender::createSilhouette()
{
	emit statusMessage("VarVis: Generating silhouette...");

	// generate Silhouette for std mesh 
	m_silhouette->SetInput(m_mesh);
	m_silhouette->SetCamera(m_renderer->GetActiveCamera());
	m_silhouette->SetEnableFeatureAngle(60);

	// mapper and actor for standard silhouette
	m_silhouetteMapper->SetInputConnection(m_silhouette->GetOutputPort());
	m_silhouetteActor->SetMapper(m_silhouetteMapper);
	m_silhouetteActor->GetProperty()->SetColor(0,0,0);
	m_silhouetteActor->GetProperty()->SetLineWidth(1);

	// add to renderer
	m_renderer->AddActor(m_silhouetteActor);
}

void VarVisRender::createOutline()
{
	// outline (bounding box of Volume)
	m_outline->SetInput( m_VolumeImageData );
	m_outline->Update();
	m_outlineMapper->SetInput(m_outline->GetOutput());
	m_outlineActor->SetMapper(m_outlineMapper);
	m_outlineActor->SetVisibility( 0 ); // WORKAROUND: Hide outline by default
		
	// add to renderer
	m_renderer->AddActor(m_outlineActor);

	// synchronize
	if (m_syncObject)
		m_syncObject->getRenderer()->AddActor(m_outlineActor);

	// generate CenterPosition 
	double p1[3]={0,0,0};
	double p2[3]={0,0,0};
	double p3[3]={0,0,0};
	double p4[3]={0,0,0};
	double *bounds= m_volume->GetBounds();
	m_resolution[0]=bounds[1];
	m_resolution[1]=bounds[3];
	m_resolution[2]=bounds[5];
	m_outline->GetOutput()->GetPoint(1,p1);
	m_outline->GetOutput()->GetPoint(3,p2);
	m_outline->GetOutput()->GetPoint(5,p3);
	m_outline->GetOutput()->GetPoint(0,p4);

	double v1[3]={p2[0]-p1[0],p2[1]-p1[1],p2[2]-p1[2]};
	double v2[3]={p3[0]-p1[0],p3[1]-p1[1],p3[2]-p1[2]};
	double center[3];
	center[0]=0.5*v1[0]+0.5*v2[0];
	center[1]=0.5*v1[1]+0.5*v2[1];
	center[2]=0.5*v1[2]+0.5*v2[2];

	m_aufpunkt[0]=p1[0]+0.5*(p4[0]-p1[0]);
	m_aufpunkt[1]=p1[1]+0.5*(p4[1]-p1[1]);
	m_aufpunkt[2]=p1[2]+0.5*(p4[2]-p1[2]);

	m_vectorHeight[0]=v1[0];
	m_vectorHeight[1]=v1[1];
	m_vectorHeight[2]=v1[2];

	m_vectorWidth[0]=v2[0];
	m_vectorWidth[1]=v2[1];
	m_vectorWidth[2]=v2[2];

	m_center[0]=m_aufpunkt[0]+center[0];
	m_center[1]=m_aufpunkt[1]+center[1];
	m_center[2]=m_aufpunkt[2]+center[2];
}

void VarVisRender::setup()
{
	m_renderer->SetViewport(  0, 0, 1, 1 );
	m_renderer->ResetCamera();

	createContour();
	//createSilhouette(); // FIXME: Commented this out since we are not using
	                      //        silhouettes right now but they are costly
	                      //        to compute.
	createOutline();

	updateVisibility();
	setRefMeshVisibility(false);
	
	// repaint everything
	m_renderWindow->Render();
}

bool VarVisRender::load_reference( const char* filename )
{
	m_samplesPresent = false;

	if( !readVolume( QString(filename) ) )
	{
		return false;
	}

	setup();

	emit statusMessage("VarVis: Setup complete");
	return true;
}

void VarVisRender::loadVolumen(QString original,QString registed)
{
	// clear data 
	VolumeNames.clear();
	
	for (int iA=0;iA<m_analyseDiffMeshActor.size();iA++)
	{
		m_renderer->RemoveActor(m_analyseDiffMeshActor.at(iA));
		// da smartpointer wird dieser auch dann aus dem speicher entfernt
		}
	m_analyseDiffMeshActor.clear();

	QFileInfo infoOriginal(original);
	VolumeNames.push_back(infoOriginal.fileName());
	
	QFileInfo infoRegisted(registed);
	VolumeNames.push_back(infoRegisted.fileName());
	
	m_renderer->RemoveActor(m_textActor);
	m_renderer->RemoveActor(m_volumeScalarBar);

	m_volumeScalarBar= VTKPTR<vtkScalarBarActor>::New();
	m_volumeScalarBar->SetLookupTable(m_colorFunc);
	m_volumeScalarBar->SetTitle("Image scalar");

	m_volumeScalarBar->SetNumberOfLabels( 4 );
	m_volumeScalarBar->GetTitleTextProperty()->SetFontSize( 6 );
	m_volumeScalarBar->GetTitleTextProperty()->ItalicOff();
	m_volumeScalarBar->GetLabelTextProperty()->SetFontSize( 2 );
	m_volumeScalarBar->GetLabelTextProperty()->ItalicOff();
	m_volumeScalarBar->SetOrientationToHorizontal();		

	m_volumeScalarBar->SetMaximumWidthInPixels( 1024 );
	m_volumeScalarBar->SetMaximumHeightInPixels( 80 );
	m_volumeScalarBar->SetHeight(50);

	m_volumeScalarBar->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
	m_volumeScalarBar->GetPositionCoordinate()->SetValue(0.1, 0.90);
	m_volumeScalarBar->SetOrientationToHorizontal();
	m_volumeScalarBar->SetWidth(0.8);
	m_volumeScalarBar->SetHeight(0.5);

	m_textActor=VTKPTR<vtkTextActor>::New();
	m_textActor->SetTextScaleModeToViewport();
	m_textActor->SetDisplayPosition(10,50); 
	m_textActor->GetTextProperty()->SetFontSize(8);
	m_textActor->SetInput(VolumeNames.at(m_indexOfVolume).toAscii());

	switch( m_indexOfVolume ) 
	{
		default:
		case 0:
			m_textActor->GetProperty()->SetColor( 
					m_opts.getDarkTextColor(m_opts.colorRegistered) );
			break;
		case 1:
			m_textActor->GetProperty()->SetColor( 
				m_opts.getDarkTextColor(m_opts.colorOriginal) );
			break;
	}

	m_renderer->AddActor(m_textActor);
	m_renderer->AddActor(m_volumeScalarBar);
	m_volumeScalarBar->SetVisibility(m_volumeScalarBarVisibility);

	// remove any present volumes
	if (m_analyseVolumen.size()>0) // we have loaded data before , clean it up
	{
		for (int iA=0;iA<m_analyseVolumen.size();iA++)
			m_renderer->RemoveVolume(m_analyseVolumen.at(iA));
	}

	// remove any present meshes
	m_analyseVolumen.clear();
	if (m_analyseMeshActor.size()>0) // we have loaded data before , clean it up
	{
		for (int iA=0;iA<m_analyseMeshActor.size();iA++)
			m_renderer->RemoveActor(m_analyseMeshActor.at(iA));
	}
	m_analyseMeshActor.clear();

	emit statusMessage("loading "+original);	
	VTKPTR<vtkMetaImageReader> originalReader = VTKPTR<vtkMetaImageReader>::New();
	originalReader->SetFileName( original.toAscii());
	originalReader->Update();

	emit statusMessage("loading "+registed);	
	VTKPTR<vtkMetaImageReader> registedReader = VTKPTR<vtkMetaImageReader>::New();
	registedReader->SetFileName( registed.toAscii());
	registedReader->Update();

	emit statusMessage("generating Visualisation Data");	
	// a) setup transfer mapping scalar value to opacity		
	m_opacityFunc->AddPoint(    0,  0 );
	m_opacityFunc->AddPoint(  255,  1 );

	// b) setup transfer mapping scalar value to color		
	m_colorFunc->AddRGBPoint(    0, 0,0,0 );
	m_colorFunc->AddRGBPoint(  255, 1,1,1 );
	m_colorFunc->HSVWrapOff();
	m_colorFunc->SetColorSpaceToHSV();

	VTKPTR<VREN_VOLUME_MAPPER> originalVolumeMapper=VTKPTR<VREN_VOLUME_MAPPER> ::New();
	VTKPTR<VREN_VOLUME_MAPPER> registedVolumeMapper=VTKPTR<VREN_VOLUME_MAPPER> ::New();
	originalVolumeMapper->SetInputConnection( originalReader->GetOutputPort() );
	registedVolumeMapper->SetInputConnection( registedReader->GetOutputPort() );

	// c) setup vtkVolume (similar to vtkActor but for volume data)
	//    and set rendering properties (like color and opacity functions)
	VTKPTR<vtkVolume> originalVolume=VTKPTR<vtkVolume> ::New();
	VTKPTR<vtkVolume> registedVolume=VTKPTR<vtkVolume> ::New();
	
	originalVolume->SetMapper( originalVolumeMapper );
	originalVolume->GetProperty()->SetInterpolationTypeToLinear();
	originalVolume->GetProperty()->SetScalarOpacity( m_opacityFunc );
	originalVolume->GetProperty()->SetColor( m_colorFunc );
		
	registedVolume->SetMapper( registedVolumeMapper );
	registedVolume->GetProperty()->SetInterpolationTypeToLinear();
	registedVolume->GetProperty()->SetScalarOpacity( m_opacityFunc );
	registedVolume->GetProperty()->SetColor( m_colorFunc );
	
	// Generate the Meshes for original
	emit statusMessage("create Original Mesh");	
	VTKPTR<vtkMarchingCubes>originalContour=VTKPTR<vtkMarchingCubes>::New();
	originalContour->SetInputConnection(originalReader->GetOutputPort());
	originalContour->SetValue( 0, m_isoValueForMeshes ); // Hounsfield units, bones approx.500-1500HU
	originalContour->SetComputeNormals( 1 );
	originalContour->Update();

	VTKPTR<vtkPolyDataMapper> originalContourMapper=VTKPTR<vtkPolyDataMapper>::New();
	originalContourMapper->ScalarVisibilityOn();
	originalContourMapper->SetScalarModeToUsePointFieldData();
	originalContourMapper->SetInputConnection(originalContour->GetOutputPort());
	
	VTKPTR<vtkActor> originalContourActor=VTKPTR<vtkActor>	::New();
	originalContourActor->SetMapper(originalContourMapper);
	m_renderer->AddActor(originalContourActor);
	
	// create Mesh 
	VTKPTR<vtkPolyData> originalMesh=VTKPTR<vtkPolyData>::New();
	VTKPTR<vtkPolyDataMapper> originalMeshMapper=VTKPTR<vtkPolyDataMapper>::New();
	VTKPTR<vtkActor> originalMeshActor=VTKPTR<vtkActor>::New();

	originalMesh->ShallowCopy(originalContour->GetOutput());
	originalMeshMapper->SetInput(originalMesh);
	originalMeshMapper->ScalarVisibilityOn();
	originalMeshMapper->SetScalarModeToUsePointFieldData();
	originalMeshActor->SetMapper(originalMeshMapper);
	originalMeshActor->GetProperty()->SetColor( m_opts.colorOriginal ); // color for original Mesh

	m_analyseMeshActor.push_back(originalMeshActor);
	m_analyseContourActor.push_back(originalContourActor);
	m_renderer->AddActor(originalMeshActor);

	// Generate the Meshes for registed
	emit statusMessage("create Registered Mesh");	
	VTKPTR<vtkMarchingCubes>registedContour=VTKPTR<vtkMarchingCubes>::New();
	registedContour->SetInputConnection(registedReader->GetOutputPort());
	registedContour->SetValue( 0, m_isoValueForMeshes ); // Hounsfield units, bones approx.500-1500HU
	registedContour->SetComputeNormals( 1 );
	registedContour->Update();

	VTKPTR<vtkPolyDataMapper> registedContourMapper=VTKPTR<vtkPolyDataMapper>::New();
	registedContourMapper->ScalarVisibilityOn();
	registedContourMapper->SetScalarModeToUsePointFieldData();
	registedContourMapper->SetInputConnection(registedContour->GetOutputPort());
	
	VTKPTR<vtkActor> registedContourActor=VTKPTR<vtkActor>	::New();
	registedContourActor->SetMapper(registedContourMapper);
	m_renderer->AddActor(registedContourActor);

	// create Mesh 
	VTKPTR<vtkPolyData> registedMesh=VTKPTR<vtkPolyData>::New();
	VTKPTR<vtkPolyDataMapper> registedMeshMapper=VTKPTR<vtkPolyDataMapper>::New();
	VTKPTR<vtkActor> registedMeshActor=VTKPTR<vtkActor>::New();

	registedMesh->ShallowCopy(registedContour->GetOutput());
	registedMeshMapper->SetInput(registedMesh);
	registedMeshMapper->ScalarVisibilityOn();
	registedMeshMapper->SetScalarModeToUsePointFieldData();
	registedMeshActor->SetMapper(registedMeshMapper);
	registedMeshActor->GetProperty()->SetColor( m_opts.colorRegistered ); // Color for registed Mesh

	m_analyseMeshActor.push_back(registedMeshActor);
	m_analyseContourActor.push_back(registedContourActor);
	m_renderer->AddActor(registedMeshActor);

	// doNot Show the Contour Cause it has not Color Mode
	originalContourActor->SetVisibility(false);
	registedContourActor->SetVisibility(false);

	// push Volumen Data to lists for Handling
	m_analyseVolumen.push_back(originalVolume);
	m_analyseVolumen.push_back(registedVolume);
	
	// add the actors
	m_renderer->AddVolume(originalVolume);
	m_renderer->AddVolume(registedVolume);
	if (!m_firstLoad)
	{	
		m_renderer->SetViewport(  0, 0, 1, 1 );
		m_renderer->ResetCamera();
		m_firstLoad=true;
	}
	m_renderWindow->Render();

	m_differenceVolumePresent=false;
	m_keyPressEnabled=true;
	setShowVolumeMeshes(m_showVolumeMeshes);
	
	emit statusMessage("Ready");	

}
void VarVisRender::loadVolumen(QString original,QString registed,QString difference)
{
	// clear data 
	for (int iA=0;iA<m_analyseDiffMeshActor.size();iA++)
	{
		m_renderer->RemoveActor(m_analyseDiffMeshActor.at(iA));
		// da smartpointer wird dieser auch dann aus dem speicher entfernt
	}
	m_analyseDiffMeshActor.clear();

	VolumeNames.clear();

	QFileInfo infoOriginal(original);
	VolumeNames.push_back(infoOriginal.fileName());
	
	QFileInfo infoRegisted(registed);
	VolumeNames.push_back(infoRegisted.fileName());
	
	QFileInfo infoDiff(difference);
	VolumeNames.push_back(infoDiff.fileName());
	
	m_renderer->RemoveActor(m_textActor);
	m_renderer->RemoveActor(m_volumeScalarBar);

	m_volumeScalarBar= VTKPTR<vtkScalarBarActor>::New();
	m_volumeScalarBar->SetLookupTable(m_colorFunc);
	m_volumeScalarBar->SetTitle("Magnitude");


	m_volumeScalarBar->SetNumberOfLabels( 4 );
	m_volumeScalarBar->GetTitleTextProperty()->SetFontSize( 6 );
	m_volumeScalarBar->GetTitleTextProperty()->ItalicOff();
	m_volumeScalarBar->GetLabelTextProperty()->SetFontSize( 2 );
	m_volumeScalarBar->GetLabelTextProperty()->ItalicOff();
	m_volumeScalarBar->SetOrientationToHorizontal();
		

	m_volumeScalarBar->SetMaximumWidthInPixels( 1024 );
	m_volumeScalarBar->SetMaximumHeightInPixels( 80 );
	m_volumeScalarBar->SetHeight(50);

	m_volumeScalarBar->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
	m_volumeScalarBar->GetPositionCoordinate()->SetValue(0.1, 0.90);
	m_volumeScalarBar->SetOrientationToHorizontal();
	m_volumeScalarBar->SetWidth(0.8);
	m_volumeScalarBar->SetHeight(0.5);



	m_textActor=VTKPTR<vtkTextActor>::New();
	m_textActor->SetTextScaleModeToViewport();
	m_textActor->SetDisplayPosition(10,50); 
	m_textActor->GetTextProperty()->SetFontSize(8);
	
	
	m_renderer->AddActor(m_textActor);
	m_renderer->AddActor(m_volumeScalarBar);
	m_volumeScalarBar->SetVisibility(m_volumeScalarBarVisibility);

	if (m_analyseVolumen.size()>0) // we have loaded data befor , clean it up
	{
		for (int iA=0;iA<m_analyseVolumen.size();iA++)
		{
			m_renderer->RemoveVolume(m_analyseVolumen.at(iA));
		}
	}
	m_analyseVolumen.clear();

	if (m_analyseMeshActor.size()>0) // we have loaded data befor , clean it up
	{
		for (int iA=0;iA<m_analyseMeshActor.size();iA++)
		{
			m_renderer->RemoveActor(m_analyseMeshActor.at(iA));
		}
	}
	m_analyseMeshActor.clear();
	//a read all 3 Volumes;
	emit statusMessage("loading "+original);	
	VTKPTR<vtkMetaImageReader> originalReader = VTKPTR<vtkMetaImageReader>::New();
	originalReader->SetFileName( original.toAscii());
	originalReader->Update();

	emit statusMessage("loading "+registed);	
	VTKPTR<vtkMetaImageReader> registedReader = VTKPTR<vtkMetaImageReader>::New();
	registedReader->SetFileName( registed.toAscii());
	registedReader->Update();

	emit statusMessage("loading "+difference);	
	VTKPTR<vtkMetaImageReader> diffReader = VTKPTR<vtkMetaImageReader>::New();
	diffReader->SetFileName( difference.toAscii());
	diffReader->Update();

	m_diffImage=diffReader->GetOutput();
	emit statusMessage("generating Visualisation Data");	
	// a) setup transfer mapping scalar value to opacity		
	m_opacityFunc->AddPoint(    0,  0 );
	m_opacityFunc->AddPoint(  255,  1 );

	// b) setup transfer mapping scalar value to color		
	m_colorFunc->AddRGBPoint(    0, 0,0,1 );
	m_colorFunc->AddRGBPoint(  255, 1,0,0 );
	m_colorFunc->HSVWrapOff();
	m_colorFunc->SetColorSpaceToHSV();

	VTKPTR<VREN_VOLUME_MAPPER> originalVolumeMapper=VTKPTR<VREN_VOLUME_MAPPER> ::New();
	VTKPTR<VREN_VOLUME_MAPPER> registedVolumeMapper=VTKPTR<VREN_VOLUME_MAPPER> ::New();
	VTKPTR<VREN_VOLUME_MAPPER>     diffVolumeMapper=VTKPTR<VREN_VOLUME_MAPPER> ::New();

	originalVolumeMapper->SetInputConnection( originalReader->GetOutputPort() );
	registedVolumeMapper->SetInputConnection( registedReader->GetOutputPort() );
	    diffVolumeMapper->SetInputConnection(     diffReader->GetOutputPort() );

	VTKPTR<vtkVolume> originalVolume=VTKPTR<vtkVolume> ::New();
	VTKPTR<vtkVolume> registedVolume=VTKPTR<vtkVolume> ::New();
	VTKPTR<vtkVolume> diffVolume    =VTKPTR<vtkVolume> ::New();
	
	originalVolume->SetMapper( originalVolumeMapper );
	originalVolume->GetProperty()->SetInterpolationTypeToLinear();
	originalVolume->GetProperty()->SetScalarOpacity( m_opacityFunc );
	originalVolume->GetProperty()->SetColor( m_colorFunc );
		
	registedVolume->SetMapper( registedVolumeMapper );
	registedVolume->GetProperty()->SetInterpolationTypeToLinear();
	registedVolume->GetProperty()->SetScalarOpacity( m_opacityFunc );
	registedVolume->GetProperty()->SetColor( m_colorFunc );
	
	diffVolume->SetMapper( diffVolumeMapper );
	diffVolume->GetProperty()->SetInterpolationTypeToLinear();
	diffVolume->GetProperty()->SetScalarOpacity( m_opacityFunc );
	diffVolume->GetProperty()->SetColor( m_colorFunc );

	emit statusMessage("create Original Mesh");	

	// Generate the Meshes for original
	VTKPTR<vtkMarchingCubes>originalContour=VTKPTR<vtkMarchingCubes>::New();
	originalContour->SetInputConnection(originalReader->GetOutputPort());
	originalContour->SetValue( 0, m_isoValueForMeshes ); // Hounsfield units, bones approx.500-1500HU
	originalContour->SetComputeNormals( 1 );
	originalContour->Update();

	VTKPTR<vtkPolyDataMapper> originalContourMapper=VTKPTR<vtkPolyDataMapper>::New();
	originalContourMapper->ScalarVisibilityOn();
	originalContourMapper->SetScalarModeToUsePointFieldData();
	originalContourMapper->SetInputConnection(originalContour->GetOutputPort());
	
	VTKPTR<vtkActor> originalContourActor=VTKPTR<vtkActor>	::New();
	originalContourActor->SetMapper(originalContourMapper);
	m_renderer->AddActor(originalContourActor);

	// create Mesh 
	VTKPTR<vtkPolyData> originalMesh=VTKPTR<vtkPolyData>::New();
	VTKPTR<vtkPolyDataMapper> originalMeshMapper=VTKPTR<vtkPolyDataMapper>::New();
	VTKPTR<vtkActor> originalMeshActor=VTKPTR<vtkActor>::New();

	originalMesh->ShallowCopy(originalContour->GetOutput());
	originalMeshMapper->SetInput(originalMesh);
	originalMeshMapper->ScalarVisibilityOn();
	originalMeshMapper->SetScalarModeToUsePointFieldData();
	originalMeshActor->SetMapper(originalMeshMapper);
	originalMeshActor->GetProperty()->SetColor( m_opts.colorOriginal ); // was: 0,1,0

	m_analyseMeshActor.push_back(originalMeshActor);
	m_analyseContourActor.push_back(originalContourActor);
	m_renderer->AddActor(originalMeshActor);
	
	// Generate the Meshes for registed
	emit statusMessage("create Registered Mesh");	
	VTKPTR<vtkMarchingCubes>registedContour=VTKPTR<vtkMarchingCubes>::New();
	registedContour->SetInputConnection(registedReader->GetOutputPort());
	registedContour->SetValue( 0, m_isoValueForMeshes ); // Hounsfield units, bones approx.500-1500HU
	registedContour->SetComputeNormals( 1 );
	registedContour->Update();

	VTKPTR<vtkPolyDataMapper> registedContourMapper=VTKPTR<vtkPolyDataMapper>::New();
	registedContourMapper->ScalarVisibilityOn();
	registedContourMapper->SetScalarModeToUsePointFieldData();
	registedContourMapper->SetInputConnection(registedContour->GetOutputPort());
	
	VTKPTR<vtkActor> registedContourActor=VTKPTR<vtkActor>	::New();
	registedContourActor->SetMapper(registedContourMapper);
	m_renderer->AddActor(registedContourActor);

	// create Mesh 
	VTKPTR<vtkPolyData> registedMesh=VTKPTR<vtkPolyData>::New();
	VTKPTR<vtkPolyDataMapper> registedMeshMapper=VTKPTR<vtkPolyDataMapper>::New();
	VTKPTR<vtkActor> registedMeshActor=VTKPTR<vtkActor>::New();

	registedMesh->ShallowCopy(registedContour->GetOutput());
	registedMeshMapper->SetInput(registedMesh);
	registedMeshMapper->ScalarVisibilityOn();
	registedMeshMapper->SetScalarModeToUsePointFieldData();
	registedMeshActor->SetMapper(registedMeshMapper);
	registedMeshActor->GetProperty()->SetColor( m_opts.colorRegistered ); // was: 0,0,1

	m_analyseMeshActor.push_back(registedMeshActor);
	m_analyseContourActor.push_back(registedContourActor);
	m_renderer->AddActor(registedMeshActor);

	// Generate the Meshes for difference
	emit statusMessage("create Difference Mesh");	
	VTKPTR<vtkMarchingCubes>differenceContour=VTKPTR<vtkMarchingCubes>::New();
	differenceContour->SetInputConnection(diffReader->GetOutputPort());
	differenceContour->SetValue( 0, m_isoValueForError ); // Hounsfield units, bones approx.500-1500HU
	differenceContour->SetComputeNormals( 1 );
	differenceContour->Update();

	VTKPTR<vtkPolyDataMapper> differenceContourMapper=VTKPTR<vtkPolyDataMapper>::New();
	differenceContourMapper->ScalarVisibilityOn();
	differenceContourMapper->SetScalarModeToUsePointFieldData();
	differenceContourMapper->SetInputConnection(differenceContour->GetOutputPort());
	
	VTKPTR<vtkActor> differenceContourActor=VTKPTR<vtkActor>	::New();
	differenceContourActor->SetMapper(differenceContourMapper);
	m_renderer->AddActor(differenceContourActor);

	// create Mesh 
	VTKPTR<vtkPolyData> differenceMesh=VTKPTR<vtkPolyData>::New();
	VTKPTR<vtkPolyDataMapper> differenceMeshMapper=VTKPTR<vtkPolyDataMapper>::New();
	VTKPTR<vtkActor> differenceMeshActor=VTKPTR<vtkActor>::New();

	differenceMesh->ShallowCopy(differenceContour->GetOutput());
	differenceMeshMapper->SetInput(differenceMesh);
	differenceMeshMapper->ScalarVisibilityOn();
	differenceMeshMapper->SetScalarModeToUsePointFieldData();
	differenceMeshActor->SetMapper(differenceMeshMapper);
	differenceMeshActor->GetProperty()->SetColor( m_opts.colorDifference ); // was: 1,0,0

	m_analyseMeshActor.push_back(differenceMeshActor);
	m_analyseContourActor.push_back(differenceContourActor);
	m_renderer->AddActor(differenceMeshActor);

	// doNot Show the Contour Cause it has not Color Mode
	originalContourActor->SetVisibility(false);
	registedContourActor->SetVisibility(false);
	differenceContourActor->SetVisibility(false);

	// push Volumen Data to lists for Handling
	m_analyseVolumen.push_back(originalVolume);
	m_analyseVolumen.push_back(registedVolume);
	m_analyseVolumen.push_back(diffVolume);
	
	// add the actors
	m_renderer->AddVolume(originalVolume);
	m_renderer->AddVolume(registedVolume);
	m_renderer->AddVolume(diffVolume);

	if (!m_firstLoad)
	{
		m_renderer->SetViewport(  0, 0, 1, 1 );
		m_renderer->ResetCamera();
		m_firstLoad=true;
	}
	m_differenceVolumePresent=true;
	m_keyPressEnabled=true;
	setShowVolumeMeshes(m_showVolumeMeshes);

	// show original Volume Mesh with opacity 0.3
	setIndexOfVolume(m_indexOfVolume);
	m_renderWindow->Render();
	emit statusMessage("Ready");	
}
void VarVisRender::showMagnitudeBar(bool visible)
{
	m_volumeScalarBarVisibility=visible;

	if( m_keyPressEnabled) // we know we have the magnitude bar 
		m_volumeScalarBar->SetVisibility(m_volumeScalarBarVisibility);

}
bool VarVisRender::generateAdvancedDiffContour()
{
	if (m_differenceVolumePresent)
	{
		// clear data 
		for (int iA=0;iA<m_analyseDiffMeshActor.size();iA++)
		{
			m_renderer->RemoveActor(m_analyseDiffMeshActor.at(iA));
			// da smartpointer wird dieser auch dann aus dem speicher entfernt

		}
		m_analyseDiffMeshActor.clear();
		int sizeOfCountours=10;
		emit statusMessage("create Advanced Difference Meshes");	
		for (int iA=0;iA<sizeOfCountours;iA++)
		{
		
			VTKPTR<vtkMarchingCubes>differenceContour=VTKPTR<vtkMarchingCubes>::New();
			differenceContour->SetInput(m_diffImage);

			if (iA==9)
				differenceContour->SetValue( 0, 255); // Hounsfield units, bones approx.500-1500HU
			else	
				differenceContour->SetValue( 0, (iA+1)*25 ); // Hounsfield units, bones approx.500-1500HU
	
			emit statusMessage("create Advanced Difference Meshes "+QString::number(iA+1)+"/10");	
			differenceContour->SetComputeNormals( 1 );
			differenceContour->Update();

			VTKPTR<vtkPolyDataMapper> differenceContourMapper=VTKPTR<vtkPolyDataMapper>::New();
			differenceContourMapper->SetInputConnection(differenceContour->GetOutputPort());
			differenceContourMapper->SetLookupTable(m_colorFunc);
			
			VTKPTR<vtkActor> differenceContourActor=VTKPTR<vtkActor>	::New();
			differenceContourActor->SetMapper(differenceContourMapper);
			
			m_renderer->AddActor(differenceContourActor); 
			m_analyseDiffMeshActor.push_back(differenceContourActor);
			differenceContourActor->SetVisibility(false);
			differenceContourActor->GetProperty()->SetOpacity((double)iA/10+0.1);

		} // mesh generating done 

		m_renderWindow->Render();
		emit statusMessage("Ready!");	
		return true;
	}
return false;
}


void VarVisRender::setErrorMeshVisibility(int index,bool visible)
{
	if (m_analyseDiffMeshActor.size()>index)
		m_analyseDiffMeshActor.at(index)->SetVisibility(visible);
	
}

void VarVisRender::setVolume( VTKPTR<vtkImageData> volume, QString name )
{
	// Set members
	m_VolumeImageData     = volume;
	m_loadedReferenceName = name;
	m_volumePresent       = true;
	
	// Clear previous data (if applicable)
	if( m_referenceLoaded )
	{
		emit statusMessage("VarVis: Clearing previous data ...");
		destroy();
		init();
		m_referenceLoaded = false;
		m_volumePresent   = false;
	}

	// Smooth image data  (Changed this code to use output directly to get
	//                     rid of network dependencies on a reader instance.)
	if( m_useGaussianSmoothing )
	{
		emit statusMessage(tr("VarVis: Applying Gaussian smoothing with sigma=%1...").arg(m_gaussionRadiusValue));
		m_gaussianSmoothFilter->SetInput( m_VolumeImageData );
		m_gaussianSmoothFilter->SetRadiusFactor( m_gaussionRadiusValue );
		m_gaussianSmoothFilter->Update();
	
		// Replace image data with smoothed version
		m_VolumeImageData = m_gaussianSmoothFilter->GetOutput();
	}

	// Set volume mapper and contour
	m_volumeMapper->SetInput( m_VolumeImageData );
	m_contour     ->SetInput( m_VolumeImageData );
	
	// c) setup vtkVolume (similar to vtkActor but for volume data)
	//    and set rendering properties (like color and opacity functions)
	m_volume->SetMapper( m_volumeMapper );
	m_volume->GetProperty()->SetInterpolationTypeToLinear();
	m_volume->GetProperty()->SetScalarOpacity( m_opacityFunc );
	m_volume->GetProperty()->SetColor( m_colorFunc );
	m_renderer->AddVolume( m_volume );	

	// Volume is present 
	m_referenceLoaded = true;
	m_volumePresent   = true;
	emit statusMessage(tr("VarVis: Succesfully set volume %1").arg(m_loadedReferenceName));
}

bool VarVisRender::readVolume( QString volumeName )
{
	// Load image from disk
	emit statusMessage(tr("VarVis: Loading volume %1 ...").arg(volumeName));
	VTKPTR<vtkMetaImageReader> reader = VTKPTR<vtkMetaImageReader>::New();
	reader->SetFileName( volumeName.toAscii() );
	reader->Update();

	// Check if image was loaded successfully
	int dims[3];
	reader->GetOutput()->GetDimensions( dims );
	if( dims[0]+dims[1]+dims[2] <= 0 )
	{
		emit statusMessage(tr("VarVis: Failed to load %1").arg(volumeName));
		return false;
	}

	// Set volume
	setVolume( reader->GetOutput(), volumeName );

	return m_volumePresent;
}

bool VarVisRender::readMesh(QString meshName)
{
	cout<<"VarVis::Reading Mesh ...";
	emit statusMessage("VarVis::Read Mesh...");
	VTKPTR<vtkPolyDataReader> meshReader=VTKPTR<vtkPolyDataReader>::New();
	meshReader->SetFileName(meshName.toAscii());
	meshReader->Update();
	setMesh(meshReader->GetOutput());
	cout<<"...done"<<endl;
	emit statusMessage("Ready");
	return m_meshPresent;
	
}

bool VarVisRender::readPoints(QString pointsName)
{
	cout<<"VarVis::Reading Points ...";
	emit statusMessage("VarVis::Reading Points ...");
	VTKPTR<vtkPolyDataReader> pointsReader=VTKPTR<vtkPolyDataReader>::New();
	pointsReader->SetFileName(pointsName.toAscii());
	pointsReader->Update();
	setSample(pointsReader->GetOutput());
	m_samplesPresent=true;
	cout<<"...done"<<endl;
	emit statusMessage("Ready");
	return m_samplesPresent;
}

void VarVisRender::setShowVolumeMeshes(bool show)
{

	m_showVolumeMeshes=show;

	for (int iA=0;iA<m_analyseVolumen.size();iA++)
	{
			m_analyseVolumen.at(iA)->SetVisibility(false);
	}
	
	for (int iA=0;iA<m_analyseMeshActor.size();iA++)
	{
		m_analyseMeshActor.at(iA)->SetVisibility(false);
	}

	int numberOfVolumes=m_analyseVolumen.size();
	int numberOfMeshs=m_analyseMeshActor.size();
	if (numberOfMeshs==0 || numberOfVolumes==0)
	{
	 return ;
	}
	if (m_showVolumeMeshes)
		m_analyseMeshActor.at(m_indexOfVolume)->SetVisibility(true);
	else
		m_analyseVolumen.at(m_indexOfVolume)->SetVisibility(true);

	m_renderWindow->Render();

}
void VarVisRender::finalInit()
{
	// 5] do other stuff as in load_reference()
	if (!m_firstLoad)
	{
		m_renderer->SetViewport(  0, 0, 1, 1 );
		m_renderer->ResetCamera();
		m_firstLoad=true;
	}

	// generate Silhouette for std mesh 
	m_silhouette->SetInput(m_mesh);
	m_silhouette->SetCamera(m_renderer->GetActiveCamera());
	m_silhouette->SetEnableFeatureAngle(60);

	// Mapper and Actor for std Silhouette
	m_silhouetteMapper->SetInputConnection(m_silhouette->GetOutputPort());
	m_silhouetteActor->SetMapper(m_silhouetteMapper);
	m_silhouetteActor->GetProperty()->SetColor(0,0,0);
	m_silhouetteActor->GetProperty()->SetLineWidth(1);
	m_renderer->AddActor(m_silhouetteActor);

	m_outline->SetInput(m_VolumeImageData);
	m_outline->Update();
	m_outlineMapper->SetInput(m_outline->GetOutput());
	m_outlineActor->SetMapper(m_outlineMapper);
	m_renderer->AddActor(m_outlineActor);

	double p1[3]={0,0,0};
	double p2[3]={0,0,0};
	double p3[3]={0,0,0};
	double p4[3]={0,0,0};
	double *bounds= m_volume->GetBounds();
	m_resolution[0]=bounds[1];
	m_resolution[1]=bounds[3];
	m_resolution[2]=bounds[5];

	m_outline->GetOutput()->GetPoint(1,p1);
	m_outline->GetOutput()->GetPoint(3,p2);
	m_outline->GetOutput()->GetPoint(5,p3);
	m_outline->GetOutput()->GetPoint(0,p4);

	double v1[3]={p2[0]-p1[0],p2[1]-p1[1],p2[2]-p1[2]};
	double v2[3]={p3[0]-p1[0],p3[1]-p1[1],p3[2]-p1[2]};
	double center[3];

	center[0]=0.5*v1[0]+0.5*v2[0];
	center[1]=0.5*v1[1]+0.5*v2[1];
	center[2]=0.5*v1[2]+0.5*v2[2];

	m_aufpunkt[0]=p1[0]+0.5*(p4[0]-p1[0]);
	m_aufpunkt[1]=p1[1]+0.5*(p4[1]-p1[1]);
	m_aufpunkt[2]=p1[2]+0.5*(p4[2]-p1[2]);

	m_vectorHeight[0]=v1[0];
	m_vectorHeight[1]=v1[1];
	m_vectorHeight[2]=v1[2];

	m_vectorWidth[0]=v2[0];
	m_vectorWidth[1]=v2[1];
	m_vectorWidth[2]=v2[2];

	m_center[0]=m_aufpunkt[0]+center[0];
	m_center[1]=m_aufpunkt[1]+center[1];
	m_center[2]=m_aufpunkt[2]+center[2];

	m_referenceLoaded=true;

	// set the Visibility Values 
	updateVisibility();
	// repaint everything
	m_renderWindow->Render();
	//cout<<"...done"<<std::endl;
}

bool VarVisRender::initVarVis( const char* volumeName,const char* meshName,const char*pointsName,
							   const double meshIsoValue,const int numOfPoints )
{

	//	1] we need to load the new Volume		
		this->load_reference( volumeName ); // was: readVolume(volumeName);
	
	//	2] we need to load the new Mesh (already
		readMesh(meshName);
	
	//	3] we need to load the PointSampling Data
		readPoints(pointsName);

	//	4] set the VarvisControls Values 	
		m_sampleRange=numOfPoints ;
		m_isovalue=meshIsoValue;

	// 5] do other stuff as in load_reference()
		finalInit();		
		return true;
}

void VarVisRender::setMesh(vtkPolyData *mesh)
{
	m_mesh->DeepCopy(mesh);
	m_meshMapper->SetInput(m_mesh);
	m_meshMapper->ScalarVisibilityOn();
	m_meshMapper->SetScalarModeToUsePointFieldData();
	if (!m_bool_isRoiRender)
		m_meshMapper->SelectColorArray("meshColors");
	m_meshActor->SetMapper(m_meshMapper);
	
	//m_renderer->AddActor(m_meshActor);
	//m_renderer->ResetCamera();
	
	m_renderWindow->Render();
	m_meshPresent=true;
}

void VarVisRender::setSample(vtkPolyData *pointsamples)
{
	m_pointPolyData->DeepCopy(pointsamples);
	m_samplerMapper->SetInput(m_pointPolyData);
	m_samplerMapper->ScalarVisibilityOn();
	m_samplerMapper->SetScalarModeToUsePointFieldData();
	m_samplerMapper->SelectColorArray("sampleColors");
	m_samplerActor->SetMapper(m_samplerMapper);

	m_renderer->AddActor(m_samplerActor);
	//m_renderer->ResetCamera();
	m_renderWindow->Render();
	m_samplesPresent=true;
}

void VarVisRender::setSamplesVisibility(bool visible)
{
	m_showSamples=visible;
	if (getWarpVis() || m_samplesPresent)
		getSamplesActor()->SetVisibility(visible);
}

// -- setMeshVisibility --
void VarVisRender::setMeshVisibility( bool visible )
{
	m_meshActor->SetVisibility(visible);
	m_showMesh=visible;
}

//==============================================================================
namespace VarVisRender_Helpers {
//==============================================================================

//-----------------------------------------------------------------------------
// loadImageData2
//-----------------------------------------------------------------------------
VTKPTR<vtkImageData> loadImageData2( const char* filename )
{
	// Load volume data from disk
	VTKPTR<vtkMetaImageReader> reader = VTKPTR<vtkMetaImageReader>::New();
	reader->SetFileName( filename );
	reader->Update();

	// Setup volume rendering
	VTKPTR<vtkImageData> img = reader->GetOutput();
	return img;
}


//-----------------------------------------------------------------------------
// computeImageDataMagnitude2
//-----------------------------------------------------------------------------
void computeImageDataMagnitude2( vtkImageData* img, float* mag_min_, float* mag_max_ )
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

//==============================================================================
} // namespace VarVisRender_Helpers
//==============================================================================
