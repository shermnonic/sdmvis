#include "VectorfieldClustering.h"
#include "GlyphInvertFilter.h"

#include <cmath> // sqrt()
#include <sstream>
#include <string>
#include <algorithm> // find()

#include <vtkActor.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkDoubleArray.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPointData.h>
#include <vtkScalarsToColors.h>
#include <vtkRenderer.h>
#include <vtkXMLPPolyDataWriter.h>
#include <vtkProperty.h>
#include <vtkArrowSource.h>
#include <vtkGlyph3D.h>
#include <vtkColorTransferFunction.h>
#include <vtkCoordinate.h>

#ifndef VTKPTR
 #include <vtkSmartPointer.h>
 #define VTKPTR vtkSmartPointer
#endif

// Disable some Visual Studio warnings:
// C4018 '<' : signed/unsigned mismatch
#pragma warning(disable:4018)

//-----------------------------------------------------------------------------
//  Random number generator helpers
//-----------------------------------------------------------------------------

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>

/// Custom random number generator for \a VectorfieldClustering
class MyRand
{
public:
	MyRand( int N ): m_dist(0,N-1) {}
	int operator() () { return m_dist(m_rng); }
private:
	boost::mt19937       m_rng;
	boost::uniform_int<> m_dist;
};

/// Draw a unique random number (of type T) which is not yet in \a alreadyDrawn
template<class T,class RNG>
T randomDrawUnique( RNG& rng, std::vector<T>& alreadyDrawn )
{
	T randomNumber;

	std::vector<T>::iterator it;
	do 	{
		// Draw a new random number ...
		randomNumber = rng();

		it = std::find( alreadyDrawn.begin(), alreadyDrawn.end(), randomNumber );

		// ... until it is not contained in alreadyDrawn array
	} while( it == alreadyDrawn.end() && !alreadyDrawn.empty() );

	// Add number to alreadyDrawn array
	alreadyDrawn.push_back( randomNumber );

	return randomNumber;
}


//-----------------------------------------------------------------------------
//  c'tor / d'tor
//-----------------------------------------------------------------------------

VectorfieldClustering::VectorfieldClustering()
  : m_ClusterVolumeData( 0 )
{
}

VectorfieldClustering::~VectorfieldClustering()
{
	for (int iA=0;iA<m_clusterPoints.size();iA++)
		m_clusterPoints.at(iA)->Delete();
	for (int iA=0;iA<m_clusterVertex.size();iA++)
		m_clusterVertex.at(iA)->Delete();
	for (int iA=0;iA<m_clusterVectors.size();iA++)
		m_clusterVectors.at(iA)->Delete();
	for (int iA=0;iA<m_clusterPolyData.size();iA++)
		m_clusterPolyData.at(iA)->Delete();

	for (int iA=0;iA<m_clusterActor.size();iA++)
	{
		m_renderer->RemoveActor(m_clusterActor.at(iA));
		m_clusterMapper.at(iA)->Delete();
		m_clusterActor.at(iA)->Delete();
	}
	clearData();
	if (m_ClusterVolumeData)
		m_ClusterVolumeData->Delete();
	m_ClusterVolumeData=0;
}

void VectorfieldClustering
  ::clearData()
{		
	m_clusterPoints.clear();
	m_clusterVertex.clear();
	m_clusterPolyData.clear();
	m_clusterMapper.clear();	
	m_clusterActor.clear();
}

void VectorfieldClustering
  ::calculateW(double resX,double resY,double resZ)
{
	double L=	resX*resX+
				resY*resY+
				resZ*resZ;

	w=1/(sqrt(L)); // scale factor 
}

//-----------------------------------------------------------------------------
//  Setters
//-----------------------------------------------------------------------------

void VectorfieldClustering
  ::setVisualisationData( vtkPolyData* data)
{
	m_pointPolyData=data;
}

void VectorfieldClustering
  ::setVisualisationMapper( vtkPolyDataMapper* mapper )
{
	m_samplerMapper=mapper;
}

void VectorfieldClustering
  ::setVisualisationLut( vtkScalarsToColors* lut)
{
	m_lut=lut;
}

//-----------------------------------------------------------------------------
//  PointList
//-----------------------------------------------------------------------------

void VectorfieldClustering
  ::setPointList( std::vector<Point2> pointList )
{
	m_pointList=pointList;
}

void VectorfieldClustering
  ::clearPointsList()
{
	m_pointList.clear();
}

//-----------------------------------------------------------------------------
//  
//-----------------------------------------------------------------------------

void VectorfieldClustering::generateCentroids()
{	
	clusterVector.resize( m_numberOfCentroids );
	
	Point2 zeroPoint;
	zeroPoint.setX( 0.0 );
	zeroPoint.setY( 0.0 );

	m_centroidsList.clear();
	m_oldCentroidsList.push_back( zeroPoint );

	MyRand myrand( m_numberOfPoints );

	// Randomly pick centroids from point set (while avoiding duplicates!)
	m_randomVector.clear(); // Keep track of randomly drawn indices
	for( unsigned i=0; i< m_numberOfCentroids; i++ )
	{
		vtkIdType id = randomDrawUnique<vtkIdType,MyRand>( myrand, m_randomVector );

		m_oldCentroidsList.push_back( zeroPoint );
		m_centroidsList   .push_back( m_pointList[ id ] );
	}
}

void VectorfieldClustering::clusterIt()
{
	MyRand myrand( m_numberOfPoints );

	verifyDistance();
	m_centroidsList.clear();
	for (int iA=0;iA<m_numberOfCentroids;iA++)
	{
		double PositionX=0;
		double PositionY=0;
		double normOfVector=0;

		double VectorX=0;
		double VectorY=0;
		double VectorZ=0;
		double vectorNorm=0;

		double sumX=0;
		double sumY=0;
		double sumZ=0;
		int sizeOfCluster=clusterVector[iA].size();
		if (sizeOfCluster==0)
		{
			
			Point2 tempCentroid;
			int random = myrand();
			double *vector=	m_pointList[random].getVector();

			double x,y;
			x=m_pointList[iA].getX();
			y=m_pointList[iA].getY();
			
			tempCentroid.setVector(vector[0],vector[1],vector[2]);
			tempCentroid.setX(x);
			tempCentroid.setY(y);
			m_centroidsList.push_back(tempCentroid);
		}
		else
		{
  			for (int iB=0;iB<clusterVector[iA].size();iB++)
			{
				double length=clusterVector[iA][iB].getVectorLength();
				PositionX+=(length*length)*	clusterVector[iA][iB].getX();
				PositionY+=(length*length)*	clusterVector[iA][iB].getY();

				sumX+=clusterVector[iA][iB].getWorldX();
				sumY+=clusterVector[iA][iB].getWorldY();
				sumZ+=clusterVector[iA][iB].getWorldZ();

				normOfVector+=(length*length);
				double * vector=clusterVector[iA][iB].getVector();

				VectorX+=length*vector[0];
				VectorY+=length*vector[1];
				VectorZ+=length*vector[2];
			}

			vectorNorm = sqrt(VectorX*VectorX + VectorY*VectorY + VectorZ*VectorZ);
			PositionX/=normOfVector;
			PositionY/=normOfVector;
			VectorX/=vectorNorm;
			VectorY/=vectorNorm;
			VectorZ/=vectorNorm;
				
			Point2 centroid;
			// need a new centroid calculation 
			centroid.setX(PositionX);
			centroid.setY(PositionY);
			centroid.setClusterID(iA);
			centroid.setWorldCoordinates(sumX/sizeOfCluster,sumY/sizeOfCluster,sumZ/sizeOfCluster);
			centroid.setVector(VectorX,VectorY,VectorZ);
			m_centroidsList.push_back(centroid);
		}
	}

	double sumEps=0;
	for (int iA=0;iA<m_centroidsList.size();iA++)
	{
		Point2 oldTempPoint=m_oldCentroidsList[iA];
		Point2 newTempPoint=(m_centroidsList.at(iA));
		Point2 tempPoint=oldTempPoint.sub(newTempPoint);
		sumEps+=tempPoint.length();
	}
	cout<<sumEps<<"|";
	if (sumEps<m_epsilon)
		m_iterationDone=true;

	m_oldCentroidsList.clear();
	for (int iA=0;iA<m_centroidsList.size();iA++)
		m_oldCentroidsList.push_back(m_centroidsList.at(iA));
}

void VectorfieldClustering::verifyDistance()
{
	for (int iA=0;iA<m_numberOfCentroids;iA++)
		clusterVector[iA].clear();

	for (int iA=0;iA<m_pointList.size();iA++)
	{
		m_pointList[iA].setClusterID(distance(m_pointList[iA],m_centroidsList));
	}
}

int VectorfieldClustering::distance(Point2 p,std::vector<Point2> centroids)
{
	double dist= 5000;
	double tempDistance=0;
	int index=0;
	
	// vergleiche die diestance zu jedem punkt 
	for (int iA=0;iA<centroids.size();iA++)
	{
		Point2 tempCentroid=centroids.at(iA);
		Point2 temp= p.sub(centroids.at(iA));

		double positionDistance= temp.length()*temp.length();
		
		// get Centroid Vector 
		double *centroidVector=centroids[iA].getVector();
		double *pointVector=p.getVector();

		// calculate their length
		double centroidVectorLength=centroidVector[0]*centroidVector[0]+
									centroidVector[1]*centroidVector[1]+
									centroidVector[2]*centroidVector[2];

		double pointVectorLength =  pointVector[0]*pointVector[0]+
									pointVector[1]*pointVector[1]+
									pointVector[2]*pointVector[2];

		centroidVectorLength=sqrt(centroidVectorLength);
		pointVectorLength=sqrt(pointVectorLength);

		// calculate skalar product
		double dotProd= centroidVector[0]*pointVector[0]+
					    centroidVector[1]*pointVector[1]+
						centroidVector[2]*pointVector[2];

		// calculate angle between the Vectors
		double cosVectors= dotProd/(pointVectorLength*centroidVectorLength);

		// distance = pointVectorLength * centroidVectorLength * sqrt( 1- cos(theta) + w* positionDistance)
		tempDistance= 1- cosVectors + w*positionDistance;
		if (tempDistance>0)
			tempDistance=sqrt(fabs(tempDistance));
		else 
			tempDistance=0;

		tempDistance*=pointVectorLength*centroidVectorLength;

		if (tempDistance<dist )
		{
			index=iA;
			dist=tempDistance;
		}
	}

	// push this point to Cluster[index]
	clusterVector[index].push_back(p);
	return index;
}

void VectorfieldClustering::clusterIt3D()
{
	// calculate the cluster zugehoerigkeit
	verifyDistance3D();
	std::vector<Point2> tempList;
	MyRand myrand(m_numberOfPoints);
	m_centroidsList.clear();

	// m_randomVector is not cleared ??

	int numberOfFaildClusters=0;
	for (int iA=0;iA<m_numberOfCentroids;iA++)
	{
		
		double PositionX=0;
		double PositionY=0;
		double PositionZ=0;
		double normOfVector=0;

		double VectorX=0;
		double VectorY=0;
		double VectorZ=0;
		double vectorNorm=0;

		double sumX=0;
		double sumY=0;
		double sumZ=0;

		int sizeOfCluster=clusterVector[iA].size();

		if (sizeOfCluster==0)
		{
			cout << "Warning: Cluster ["<<iA<<"] is empty." << endl;

			vtkIdType id = randomDrawUnique<vtkIdType,MyRand>( myrand, m_randomVector );
			m_centroidsList.push_back( m_pointList.at(id) );

			Point2 tempPoint = m_pointList.at( id );
			numberOfFaildClusters++;
		}
		else
		{
			for (int iB=0;iB<clusterVector[iA].size();iB++)
			{
				double length=clusterVector[iA][iB].getVectorLength();
				PositionX+=(length*length)*	clusterVector[iA][iB].getWorldX();
				PositionY+=(length*length)*	clusterVector[iA][iB].getWorldY();
				PositionZ+=(length*length)*	clusterVector[iA][iB].getWorldZ();

				sumX+=clusterVector[iA][iB].getWorldX();
				sumY+=clusterVector[iA][iB].getWorldY();
				sumZ+=clusterVector[iA][iB].getWorldZ();

				normOfVector+=(length*length);
				double * vector=clusterVector[iA][iB].getVector();

				VectorX+=length*vector[0];
				VectorY+=length*vector[1];
				VectorZ+=length*vector[2];
				 
			}
			vectorNorm = sqrt(VectorX*VectorX + VectorY*VectorY + VectorZ*VectorZ);
			VectorX/=vectorNorm;
			VectorY/=vectorNorm;
			VectorZ/=vectorNorm;
				
			Point2 centroid;
			// need a new centroid calculation 
			centroid.setClusterID(iA);
			centroid.setWorldCoordinates(sumX/sizeOfCluster,sumY/sizeOfCluster,sumZ/sizeOfCluster);
			centroid.setVector(VectorX,VectorY,VectorZ);
			m_centroidsList.push_back(centroid);
		}
	}

	cout<<"num of Faild Clusters "<< numberOfFaildClusters<<endl;
	double sumEps=0;
	for (int iA=0;iA<m_centroidsList.size();iA++)
	{
		Point2 oldTempPoint=m_oldCentroidsList[iA];
		Point2 newTempPoint=(m_centroidsList.at(iA));

		Point2 tempPoint=oldTempPoint.sub3d(newTempPoint);
		sumEps+=tempPoint.length3d();
	}
	cout<<sumEps<<" ";
	if (sumEps<m_epsilon)
		m_iterationDone=true;

	m_oldCentroidsList.clear();
	for (int iA=0;iA<m_centroidsList.size();iA++)
		m_oldCentroidsList.push_back(m_centroidsList.at(iA));	
}

void VectorfieldClustering::generate3DClustering()
{
	for( int roedel=0;roedel<1;roedel++)
	{
		for (int iA=0;iA<m_clusterPoints.size();iA++)
			m_clusterPoints.at(iA)->Delete();
		for (int iA=0;iA<m_clusterVertex.size();iA++)
			m_clusterVertex.at(iA)->Delete();
		for (int iA=0;iA<m_clusterVectors.size();iA++)
			m_clusterVectors.at(iA)->Delete();
		for (int iA=0;iA<m_clusterPolyData.size();iA++)
			m_clusterPolyData.at(iA)->Delete();

		for (int iA=0;iA<m_clusterActor.size();iA++)
		{
			m_renderer->RemoveActor(m_clusterActor.at(iA));
			m_clusterMapper.at(iA)->Delete();
			m_clusterActor.at(iA)->Delete();
		}
		
		m_clusterPoints.clear();
		m_clusterVertex.clear();
		m_clusterVectors.clear();
		m_clusterPolyData.clear();
		m_clusterMapper.clear();	
		m_clusterActor.clear();

		// get all points [their coordinates and Vectors]
		// and push them to m_pointList
		for (int iA=0;iA<m_ClusterVolumeData->GetNumberOfPoints();iA++)
		{
			double *pointX=m_ClusterVolumeData->GetPoint(iA);
		
			Point2 tempPoint;
			tempPoint.setX(0);
			tempPoint.setY(0);
			tempPoint.setWorldCoordinates(pointX[0],pointX[1],pointX[2]);
			tempPoint.setPointID(iA);
			double *vector=m_ClusterVolumeData->GetPointData()->GetVectors()->GetTuple3(iA);
			tempPoint.setVector(vector[0],vector[1],vector[2]);
			m_pointList.push_back(tempPoint);
		}
		m_numberOfPoints= m_pointList.size();
		
		// init centroids 
		generateCentroids();

		// create Visualisation 
	//--RODEL KRAM --
		
		vtkPoints		  * startPoints= vtkPoints::New();
		vtkDoubleArray    *startVectors = 	vtkDoubleArray  ::New();
		startVectors->SetNumberOfComponents(3);
		vtkCellArray	  * startVertex	 = vtkCellArray  ::New();
		vtkPolyData		  *startPolyData =vtkPolyData::New();
		vtkPolyDataMapper *startPolyDataMapper	 = vtkPolyDataMapper::New();
		vtkActor		  *startActor	 =vtkActor ::New();
		
		vtkIdType startPid[1];

		for (int iA=0;iA<m_centroidsList.size();iA++)
		{		// push values to vkt points / vectors
			double x,y,z;
			double *vector=m_centroidsList[iA].getVector();
			x=m_centroidsList[iA].getWorldX();
			y=m_centroidsList[iA].getWorldY();
			z=m_centroidsList[iA].getWorldZ();
			startPid[0]= startPoints->InsertNextPoint ( x, y, z );
			startVertex->InsertNextCell ( 1,startPid);
			startVectors->InsertNextTuple3(vector[0],vector[1],vector[2]);
		}
		startPolyData->SetPoints(startPoints);
		startPolyData->SetVerts(startVertex);
		startPolyData->GetPointData()->SetVectors(startVectors);

	#if 0  // DEBUG
		// save this data to file
		VTKPTR<vtkXMLPPolyDataWriter> startWriter=VTKPTR<vtkXMLPPolyDataWriter>::New();
		startWriter->SetInput(startPolyData);		
		std::stringstream filename;
		filename << "C:\\_vitalis\\sdmvis\\roedel\\start" << roedel << ".vtp";
		startWriter->SetFileName(filename.str().c_str());
		startWriter->Write();
	#endif

		// do the clustering
		m_iterationDone=false;
		cout<<"clustering ";
		for (int iA=0;iA<200;iA++)
		{	cout<<"|"<<iA<<"->";
		if (!m_iterationDone)
			clusterIt3D();
		else 
			break;
		}	
		cout<<"done!"<<endl;

		// create Visualisation 
		vtkPoints		  * points2	= vtkPoints::New();
		vtkDoubleArray    *vectors2 = 	vtkDoubleArray  ::New();
		vectors2->SetNumberOfComponents(3);
		vtkCellArray	  * vertex2	 = vtkCellArray  ::New();
		vtkPolyData		  *polyData2 =vtkPolyData::New();
		vtkPolyDataMapper *mapper2	 = vtkPolyDataMapper::New();
		vtkActor		  *actor2	 =vtkActor ::New();
		
		vtkIdType pid[1];
		double m_maxOrthVector=0;
		double m_minOrthVector=20;

		// generate the centroids Position and their Vectors
		// calculate the mean of the points in the cluster -> new position
		// calculate the mean of the Vectors in the cluster -> new Vector
		for (int iA=0;iA<m_numberOfCentroids;iA++)
		{
			double worldSumX=0;
			double worldSumY=0;
			double worldSumZ=0;

			for (int iB=0;iB<clusterVector[iA].size();iB++)
			{
				worldSumX+=clusterVector[iA][iB].getWorldX();
				worldSumY+=clusterVector[iA][iB].getWorldY();
				worldSumZ+=clusterVector[iA][iB].getWorldZ();
			}
		
			worldSumX/=clusterVector[iA].size();
			worldSumY/=clusterVector[iA].size();
			worldSumZ/=clusterVector[iA].size();
	 
			m_centroidsList[iA].setWorldCoordinates(worldSumX,worldSumY,worldSumZ);

			double * vector=m_centroidsList[iA].getVector();
			double x=m_centroidsList[iA].getWorldX();
			double y=m_centroidsList[iA].getWorldY();
			double z=m_centroidsList[iA].getWorldZ();
			
			// push values to vkt points / vectors
			pid[0]= points2->InsertNextPoint ( x, y, z );
			vertex2->InsertNextCell ( 1,pid);
			vectors2->InsertNextTuple3(vector[0],vector[1],vector[2]);
		}
		// set the values of vtk PolyData for point Visualisation
 		polyData2->SetPoints(points2);
		polyData2->SetVerts(vertex2);
		polyData2->GetPointData()->SetVectors(vectors2);

		vtkPoints		  * resultPoints= vtkPoints::New();
		vtkDoubleArray    *resultVectors = 	vtkDoubleArray  ::New();
		resultVectors->SetNumberOfComponents(3);
		vtkCellArray	  * resultVertex	 = vtkCellArray  ::New();
		vtkPolyData		  *resultPolyData =vtkPolyData::New();
		
		vtkIdType resultPid[1];
		for (int iA=0;iA<m_centroidsList.size();iA++)
		{		// push values to vkt points / vectors
			double resultx,resulty,resultz;
			double *resultVector=m_centroidsList[iA].getVector();
			resultx=m_centroidsList[iA].getWorldX();
			resulty=m_centroidsList[iA].getWorldY();
			resultz=m_centroidsList[iA].getWorldZ();
			resultPid[0]= resultPoints->InsertNextPoint ( resultx, resulty, resultz );
			resultVertex->InsertNextCell ( 1,resultPid);
			resultVectors->InsertNextTuple3(resultVector[0],resultVector[1],resultVector[2]);
		}
		resultPolyData->SetPoints(resultPoints);
		resultPolyData->SetVerts(resultVertex);
		resultPolyData->GetPointData()->SetVectors(resultVectors);

	#if 0  // DEBUG
		VTKPTR<vtkXMLPPolyDataWriter> resultWriter=VTKPTR<vtkXMLPPolyDataWriter>::New();
		resultWriter->SetInput(resultPolyData);
		
		std::stringstream filename;
		filename << "C:\\_vitalis\\sdmvis\\roedel\\result" << roedel << ".vtp";
		resultWriter->SetFileName(filename.str().c_str());
		resultWriter->Write();
	#endif

		mapper2->SetInput(polyData2);
		actor2->SetMapper(mapper2);
		actor2->GetProperty()->SetPointSize(3);
		
		// create Glyph Data
		VTKPTR<vtkArrowSource>    arrow2  = VTKPTR<vtkArrowSource>   ::New();
		VTKPTR<vtkGlyph3D>        glyph2  = VTKPTR<vtkGlyph3D>       ::New();
		vtkPolyDataMapper* glmapper2 = vtkPolyDataMapper::New();

		VTKPTR<GlyphInvertFilter> invertFilter = VTKPTR<GlyphInvertFilter>::New();
		invertFilter->SetInput(polyData2);
		invertFilter->Update();

		glyph2->SetInput(invertFilter->GetOutput());
		glyph2->SetSourceConnection( arrow2->GetOutputPort() );
		glyph2->SetVectorModeToUseVector();
   		glyph2->SetColorModeToColorByVector();
		glyph2->OrientOn();
		glyph2->SetScaleFactor(10);
		glyph2->Update();

		glmapper2->SetInputConnection(glyph2->GetOutputPort());
		vtkActor* glactor2 = vtkActor::New();
		glactor2->SetMapper( glmapper2 );
		m_renderer->AddActor(glactor2);


		// push values [need to be done for preventing memory leaks]
		m_clusterPoints.push_back(points2);
		m_clusterPoints.push_back(startPoints);
		m_clusterPoints.push_back(resultPoints);
		
		m_clusterVertex.push_back(vertex2);
		m_clusterVertex.push_back(resultVertex);
		m_clusterVertex.push_back(startVertex);


		m_clusterVectors.push_back(vectors2);
		m_clusterVectors.push_back(resultVectors);
		m_clusterVectors.push_back(startVectors);

		m_clusterPolyData.clear();
		m_clusterPolyData.push_back(polyData2);
		m_clusterPolyData.push_back(startPolyData);
		m_clusterPolyData.push_back(resultPolyData);

		m_clusterMapper.push_back(mapper2);	
		m_clusterActor.push_back(actor2);
		m_clusterMapper.push_back(glmapper2);	
		m_clusterActor.push_back(glactor2);
		// paint all actors
//		m_renderWindow->Render();
	}
}

void VectorfieldClustering::verifyDistance3D()
{
	for (int iA=0;iA<m_numberOfCentroids;iA++)
		clusterVector[iA].clear();

	for (int iA=0;iA<m_pointList.size();iA++)
	{
		m_pointList[iA].setClusterID(distance3D(m_pointList[iA],m_centroidsList));
		if (iA%1000==0)
			cout<<".";
	}
}

int  VectorfieldClustering::distance3D(Point2 p,std::vector<Point2> centroids)
{
	double dist= 500000;
	double tempDistance=0;
	int index=0;
	
	// vergleiche die diestance zu jedem punkt 
	for (int iA=0;iA<centroids.size();iA++)
	{
		Point2 tempCentroid=centroids[iA];
		Point2 temp= p.sub3d(centroids[iA]);

		double positionDistance= temp.length3d()*temp.length3d();

		// get the Vectors
		double *centroidVector=centroids[iA].getVector();
		double *pointVector=p.getVector();
		// calculate their length
		double centroidVectorLength=centroidVector[0]*centroidVector[0]+
									centroidVector[1]*centroidVector[1]+
									centroidVector[2]*centroidVector[2];

		double pointVectorLength =  pointVector[0]*pointVector[0]+
									pointVector[1]*pointVector[1]+
									pointVector[2]*pointVector[2];

		centroidVectorLength=sqrt(centroidVectorLength);
		pointVectorLength=sqrt(pointVectorLength);

		// calculate skalarProduct
		double dotProd= centroidVector[0]*pointVector[0]+
					    centroidVector[1]*pointVector[1]+
						centroidVector[2]*pointVector[2];

		// calculate Angle
		double cosVectors= dotProd/(pointVectorLength*centroidVectorLength);
		
		// distance = pointVectorLength * centroidVectorLength * sqrt( 1- cos(theta) + w* positionDistance)
		tempDistance= 1- cosVectors + w*positionDistance;

		if (tempDistance>0)
			tempDistance=sqrt(fabs(tempDistance));

		else 
			tempDistance=0;

		tempDistance*=pointVectorLength*centroidVectorLength;

		if (tempDistance<dist )
		{
			index=iA;
			dist=tempDistance;
		}
	}
	// push this point to Cluster[index]
	clusterVector[index].push_back(p);
	return index;
}

void VectorfieldClustering::Visualisate()
{

	vtkPoints		  * points2	= vtkPoints::New();
	vtkDoubleArray    *vectors2 = 	vtkDoubleArray  ::New();
	vectors2->SetNumberOfComponents(3);
	vtkCellArray	  * vertex2	 = vtkCellArray  ::New();
	vtkPolyData		  *polyData2 =vtkPolyData::New();
	vtkPolyDataMapper *mapper2	 = vtkPolyDataMapper::New();
	vtkActor		  *actor2	 =vtkActor ::New();
		
	vtkIdType pid[1];
	double m_maxOrthVector=0;
	double m_minOrthVector=20;

	{	// generate the color map for the centroids arrows
		for (int iA=0;iA<m_numberOfCentroids;iA++)
		{
			double tempLength=0;
			for (int iB=0;iB<clusterVector[iA].size();iB++)
				tempLength+=clusterVector[iA][iB].getVectorLength();

			tempLength/=clusterVector[iA].size();

			double *tempV= m_centroidsList[iA].getVector();
			double tempVLength= m_centroidsList[iA].getVectorLength();
			m_centroidsList[iA].setVector(tempLength*tempV[0],tempLength*tempV[1],tempLength*tempV[2]);

			if (tempLength<m_minOrthVector)
				m_minOrthVector=tempLength;

			if (tempLength>m_maxOrthVector)
				m_maxOrthVector=tempLength;

		}

	}

	{	// do color the clusterd points
		// each centroid has its length 
		// so set the color of the points in this
		//polydata to its length oO 
		
		// generate a new Color look up Table for Cluster Index
	
		vtkScalarsToColors* lut = NULL;

		VTKPTR<vtkColorTransferFunction> hlut = VTKPTR<vtkColorTransferFunction>::New();
		hlut->AddRGBPoint( 0*m_numberOfCentroids/6, 1,0,0 );
		hlut->AddRGBPoint( 1*m_numberOfCentroids/6, 1,0.5,0 );
		hlut->AddRGBPoint( 2*m_numberOfCentroids/6, 1,1,0 );
		hlut->AddRGBPoint( 3*m_numberOfCentroids/6, 0,1,0 );
		hlut->AddRGBPoint( 4*m_numberOfCentroids/6, 0,0,1 );
		hlut->AddRGBPoint( 5*m_numberOfCentroids/6, 0.44,0,1 );
		hlut->AddRGBPoint( 6*m_numberOfCentroids/6, 0.56,0,1 );


		hlut->HSVWrapOff();
		hlut->SetColorSpaceToRGB();
		lut = hlut.GetPointer();

		for (int iA=0;iA<m_pointList.size();iA++)
		{
			// get the point id and cluster id
			int pointId = m_pointList[iA].getPointId();
			int clusterId= m_pointList[iA].getClusterId();

			// now we need the color -> see the length of centroidID 
			// and get the color from lut
			double * color = lut->GetColor(clusterId);
			double r=color[0]*255;
			double g=color[1]*255;
			double b=color[2]*255;

			// now set the color int the polyData 
			m_pointPolyData->GetPointData()->GetArray("sampleColors")->SetTuple3(pointId,r,g,b);
					
		
		}
		m_pointPolyData->Update();
		m_samplerMapper->Update();
	}

	for (int iA=0;iA<m_numberOfCentroids;iA++)
	{
		double worldSumX=0;
		double worldSumY=0;
		double worldSumZ=0;

		for (int iB=0;iB<clusterVector[iA].size();iB++)
		{
			worldSumX+=clusterVector[iA][iB].getWorldX();
			worldSumY+=clusterVector[iA][iB].getWorldY();
			worldSumZ+=clusterVector[iA][iB].getWorldZ();
		}
	
		worldSumX/=clusterVector[iA].size();
		worldSumY/=clusterVector[iA].size();
		worldSumZ/=clusterVector[iA].size();
 
		m_centroidsList[iA].setWorldCoordinates(worldSumX,worldSumY,worldSumZ);

	
	double * vector=m_centroidsList[iA].getVector();
	double x=m_centroidsList[iA].getWorldX();
	double y=m_centroidsList[iA].getWorldY();
	double z=m_centroidsList[iA].getWorldZ();
	pid[0]= points2->InsertNextPoint ( x, y, z );
	vertex2->InsertNextCell ( 1,pid);
	vectors2->InsertNextTuple3(vector[0],vector[1],vector[2]);
		
	}

	 polyData2->SetPoints(points2);
	 polyData2->SetVerts(vertex2);
	 polyData2->GetPointData()->SetVectors(vectors2);
	 mapper2->SetInput(polyData2);
	 actor2->SetMapper(mapper2);
	
	 actor2->GetProperty()->SetPointSize(3);
//	 m_renderer ->AddActor(actor2); // add points actor

	VTKPTR<vtkArrowSource>    arrow2  = VTKPTR<vtkArrowSource>   ::New();
	VTKPTR<vtkGlyph3D>        glyph2  = VTKPTR<vtkGlyph3D>       ::New();
	vtkPolyDataMapper* glmapper2 = vtkPolyDataMapper::New();

	glyph2->SetInput(polyData2);
	glyph2->SetSourceConnection( arrow2->GetOutputPort() );
	glyph2->SetVectorModeToUseVector();
	glyph2->SetColorModeToColorByVector();
    glyph2->OrientOn();
    glyph2->SetScaleFactor(10);
    glyph2->Update();

	glmapper2->SetInputConnection(glyph2->GetOutputPort());
	glmapper2->SetLookupTable(m_lut);

	vtkActor* glactor2 = vtkActor::New();
    glactor2->SetMapper( glmapper2 );
	m_renderer->AddActor(glactor2);

	// push values 
	m_clusterPoints.push_back(points2);
	m_clusterVertex.push_back(vertex2);
	m_clusterPolyData.clear();
	m_clusterPolyData.push_back(polyData2);
	m_clusterMapper.push_back(mapper2);	
	m_clusterMapper.push_back(glmapper2);	
	m_clusterActor.push_back(actor2);
	m_clusterActor.push_back(glactor2);
	//m_renderWindow->Render();
}

void VectorfieldClustering::setSamplePoints( vtkPolyData* samplePts, std::vector<vtkIdType> pointIdList )
{
	clearPointsList();

	vtkCoordinate * myCoordinates=vtkCoordinate::New();
	myCoordinates->SetCoordinateSystemToWorld();
	std::vector<Point2> tempPointList;
	tempPointList.reserve( pointIdList.size() );
	for (int iA=0;iA<pointIdList.size();iA++)
	{
		double *pointX = samplePts->GetPoint(pointIdList.at(iA));
		myCoordinates->SetValue(pointX[0],pointX[1],pointX[2]);
		int *pos=myCoordinates->GetComputedDisplayValue(m_renderer);

		Point2 tempPoint;
		tempPoint.setX(pos[0]);
		tempPoint.setY(pos[1]);
		tempPoint.setWorldCoordinates(pointX[0],pointX[1],pointX[2]);
		tempPoint.setPointID(pointIdList.at(iA));
		//if (getWarpVis())
		{
			double *vector = samplePts->GetPointData()->GetVectors()->GetTuple3(pointIdList.at(iA));
			tempPoint.setVector(vector[0],vector[1],vector[2]);
		}
		tempPointList.push_back(tempPoint);
	}
	int someSize=tempPointList.size();

	setPointList(tempPointList);	
	setNumberOfPoints( tempPointList.size() );
}

void VectorfieldClustering::setNumberOfCentroidsRelative( double fraction )
{
	unsigned numPoints = m_numberOfPoints; // == m_pointList.size() ?
	setNumberOfCentroids( fraction * numPoints );
}

