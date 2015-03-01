#ifndef VECTORFIELDCLUSTERING_H
#define VECTORFIELDCLUSTERING_H

#include <vector>
#include <vtkType.h>

// VTK forwards
class vtkActor;
class vtkPoints;
class vtkCellArray;
class vtkDoubleArray;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkScalarsToColors;
class vtkRenderer;

//-----------------------------------------------------------------------------
//  Point2 helper class for VectorfieldClustering
//-----------------------------------------------------------------------------
/// 2D point (helper class for \a VectorfieldClustering)
class Point2
{
public :
	int getPointId(){return pointId;}
	int getClusterId(){return clusterId;}
	double getX(){return x;}
	double getY(){return y;}
	double getWorldX(){return worldX;}
	double getWorldY(){return worldY;}
	double getWorldZ(){return worldZ;}
	double getVectorLength(){return vectorLength;}
	double * getVector(){return (double*)vector;}
	
	double calcVectorLenght(double x,double y, double z)
	{
		double weight=x*x+y*y+z*z;
		if (weight>0)
			return sqrt(weight);
		else
			return 0;	
	}
	
	void setPointID(int value){pointId=value;}
	void setClusterID(int value){clusterId=value;}
	void setX(double value){x=value;}
	void setY(double value){y=value;}
	void setVector(double x,double y, double z)
	{
		vectorX=x;
		vectorY=y;
		vectorZ=z;
		vector[0]=x;
		vector[1]=y;
		vector[2]=z;

		vectorLength=calcVectorLenght(x,y,z);
	}
	
	void setWorldCoordinates(double x,double y, double z)
	{
		worldX=x;
		worldY=y;
		worldZ=z;
	}

	Point2 sub(Point2 p)
	{
		Point2 tempPoint;
		tempPoint.setX(x-p.getX());
		tempPoint.setY(y-p.getY());
		return tempPoint;
	}

	Point2 sub3d(Point2 p)
	{
		Point2 tempPoint;
		double lenX=worldX-p.getWorldX();
		double lenY=worldY-p.getWorldY();
		double lenZ=worldZ-p.getWorldZ();
		
		tempPoint.setX(x-p.getX());
		tempPoint.setY(y-p.getY());
		tempPoint.setWorldCoordinates(lenX,lenY,lenZ);

		return tempPoint;
	}

	Point2 add(Point2 p)
	{
		Point2 tempPoint;
		tempPoint.setX(x+p.getX());
		tempPoint.setY(y+p.getY());
		return tempPoint;
	}

	double length()
	{
		double distance=x*x+y*y;
		if (distance>0)
			return sqrt(distance);
		else 
			return 0;
	}
	
	double length3d()
	{
		double distance=worldX*worldX+worldY*worldY+worldZ+worldZ;
		if (distance>0)
			return sqrt(distance);
		else 
			return 0;
	}
	
private:
	double x;
	double y;

	int pointId;
	int clusterId;

	double worldX,worldY,worldZ;
	double vector[3];
	double vectorX,vectorY,vectorZ;
	double vectorLength;
};


//-----------------------------------------------------------------------------
//  VectorfieldClustering
//-----------------------------------------------------------------------------
/**
	Voronoi clustering of a vectorfield.

	[Du2004] Du, Qiang, and Xiaoqiang Wang. "Centroidal Voronoi tessellation 
	based algorithms for vector fields visualization and segmentation." 
	Proceedings of the conference on Visualization'04., 2004.

	\author Vitalis Wiens
*/
class VectorfieldClustering
{
public :
	VectorfieldClustering();
	~VectorfieldClustering();

	void setSamplePoints( vtkPolyData* samplePts, std::vector<vtkIdType> pointIdList );

	void calculateW(double resX,double resY,double resZ);

	void Visualisate();
	void generateCentroids();
	void clearData();

	void generate3DClustering();
	void setVisualisationData  ( vtkPolyData* data );
	void setVisualisationMapper( vtkPolyDataMapper* mapper );
	void setVisualisationLut   ( vtkScalarsToColors* lut );

protected:
	// Use setSamplePoints() instead
	void setPointList( std::vector<Point2> pointList );
	void clearPointsList();

public:
	void clusterIt();
	void verifyDistance();
	int distance(Point2 p,std::vector<Point2> centroids);
	void clusterIt3D();
	void verifyDistance3D();
	int distance3D(Point2 p,std::vector<Point2> centroids);

	std::vector<vtkActor*>		   GetActors()		   {return m_clusterActor;}
	std::vector<vtkPoints*>		   GetClusterPoints()  {return m_clusterPoints;}
	std::vector<vtkCellArray*>     GetClusterVerterx() {return m_clusterVertex;}
	std::vector<vtkDoubleArray*>   GetClusterVectors() {return m_clusterVectors;}
	std::vector<vtkPolyData*>      GetClusterPolyData(){return m_clusterPolyData;}
	std::vector<vtkPolyDataMapper*>GetClusterMapper()  {return m_clusterMapper;}
	
	void setNumberOfPoints   ( int num ) { m_numberOfPoints    = num; }
	void setNumberOfCentroids( int num ) { m_numberOfCentroids = num; }
	void setEpsilon          ( double e) { m_epsilon = e; }
	
	void setNumberOfCentroidsRelative( double fraction );

	bool isIterationDone() const          { return m_iterationDone; }
	void setIterationDone( bool done )    { m_iterationDone=done; }
	void setRenderer( vtkRenderer* ren )  { m_renderer = ren; }

	void setPointData( vtkPolyData* pointData ) { m_ClusterVolumeData=pointData; }

private:
	std::vector<std::vector<Point2>> clusterVector;
	std::vector<Point2> m_pointList;
	std::vector<Point2> m_pointListVolume;
	std::vector<Point2> m_centroidsList;
	std::vector<Point2> m_oldCentroidsList;
	std::vector<vtkIdType> m_randomVector;
	
	// Visualization
	std::vector<vtkPoints*>	        m_clusterPoints ;
	std::vector<vtkCellArray*>      m_clusterVertex ;
	std::vector<vtkDoubleArray*>    m_clusterVectors ;
	std::vector<vtkPolyData*>       m_clusterPolyData;
    std::vector<vtkPolyDataMapper*> m_clusterMapper ;
    std::vector<vtkActor*>          m_clusterActor ;
	
	double w;
	bool m_iterationDone;
	double m_epsilon;
	int m_numberOfPoints;
	int m_numberOfCentroids;

	vtkRenderer			*m_renderer;
	vtkPolyData			*m_ClusterVolumeData;
	vtkPolyData			*m_pointPolyData;
	vtkPolyDataMapper	*m_samplerMapper;
	vtkScalarsToColors  *m_lut;
};

#endif // VECTORFIELDCLUSTERING_H
