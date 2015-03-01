#ifndef TENSORDATAPROVIDER_H
#define TENSORDATAPROVIDER_H

#include "ImageDataSpace.h"

#include <vtkPoints.h>
#include <vtkDoubleArray.h>
#ifndef VTKPTR
#include <vtkSmartPointer.h>
#define VTKPTR vtkSmartPointer
#endif

// Forwards
class vtkPoints;
class vtkDataArray;
class vtkImageData;

/// Abstract base class to feed tensor and vector data to a TensorVisBase class.
/// Implement the following functions to interface to your data:
/// - getTensor() [required]
/// - getVector() [optional]
/// - isValidSamplePoint() [optional]
/// For more customization one can overwrite updateTensorData().
///
/// To prepare tensor data for visualization the following functions should be 
/// called in this order:
/// 1. setImageDataSpace()
/// 2. generate[Grid|Random]Points()
/// 3. updateTensorData()
class TensorDataProvider
{
public:
	enum Sampling { NoSampling, GridSampling, RandomSampling };

	// Subregion (currently only used for axis-aligned slicing)
	struct Region
	{
		Region()
			: active(false)
		{
			min_[0]=min_[1]=min_[2]=-1;
			max_[0]=max_[1]=max_[2]=-1;
		}
		int min_[3], max_[3];
		bool active;

		void reset( const ImageDataSpace& space )
		{
			min_[0] = min_[1] = min_[2] = 0;
			space.getDimensions( max_ );
		}

		void adjust( int (&min__)[3], int (&max__)[3] )
		{
			for( int i=0; i < 3; i++ )
			{
				if( min_[i] >= 0 ) min__[i]=min_[i];
				if( max_[i] >= 0 ) max__[i]=max_[i];
			}
		}

		void get_range( const ImageDataSpace& space, int (&min__)[3], int (&max__)[3] )
		{
			min__[0] = min__[1] = min__[2] = 0;
			space.getDimensions( max__ );
			if( active )
				adjust( min__, max__ );
		}
	};

	TensorDataProvider();

	/// All sampling operations are performed in physical coordinates.
	/// Thus it is of crucial importance to call this function first!
	void setImageDataSpace( ImageDataSpace ids );
	const ImageDataSpace& getImageDataSpace() const { return m_space; }
	
	/// Perform a regular grid sampling of a specific stepsize in image space.
	void generateGridPoints  ( int gridStepSizeX, 
	                           int gridStepSizeY=-1, int gridStepSizeZ=-1 );
	/// Perform a random sampling. Note that the number of created samples
	/// can be smaller than \a numSamples if too many random samples have been
	/// discarded by \a isValidSamplePoint().
	void generateRandomPoints( int numSamples );

	/// Returns the actual number of sampling points generated.
	unsigned getNumValidPoints() const { return m_numPoints; }

	/// Call this function after sampling points are generated.
	/// Internally the user functions \a getTensor() and \a getVector() are
	/// invoked to setup the required tensor data at the sample points.
	virtual void updateTensorData();

	bool writeTensorData( const char* filename );

	///@{ Direct access to sample data
	vtkPoints*    getPoints()  { return m_samplePoints; }
	vtkDataArray* getTensors() { return m_sampleTensors; }
	virtual vtkDataArray* getVectors() { return m_sampleVectors; }
	///@}

	/// Use with care!
	void setPoints( vtkPoints* pts, bool threshold=false );

	/// The default implementation of \a isValidSamplePoint() does thresholding
	/// based on a given image. Sample points will only be created at positions
	/// where the image intensity (first channel) is above \a threshLow and
	/// below \a threshHigh. Note that since \a isValidSamplePoint() may be 
	/// re-implemented in a subclass, this functionality can be overridden.
	/// Set \a img to NULL to deactivate thresholding.
	void setImageMask( vtkImageData* img );
	void setImageThreshold( double threshLow, double threshHigh=30000.0 );

	vtkImageData* getImageMask() { return m_thresholdImage; }
	double getImageThresholdLow () const { return m_thresholdLow;  }
	double getImageThresholdHigh() const { return m_thresholdHigh; }

	int getLastSamplingMethod() const { return m_lastSampling; }
	void getLastGridStep( int (&stepsize)[3] ) const
	{
		stepsize[0] = m_lastGridStep[0];
		stepsize[1] = m_lastGridStep[1];
		stepsize[2] = m_lastGridStep[2];
	}

	///@{ Slicing
	void setSlicing( bool enable );
	bool getSlicing() const;
	void setSliceDirection( int dim );
	int  getSliceDirection() const { return m_sliceDir; }
	int  getNumSlices( int dim=-1 );
	void setSlice( int slice );
	int  getSlice() const { return m_slice; }
	///@}

//protected:
	/// Allow easy access to the ImageDataSpace instance
	ImageDataSpace& space() { return m_space; }

	/// This function is used internally to compute the tensor data at the 
	/// sample points. This is the main function the data provider has to 
	/// implement.
	virtual void getTensor( double x, double y, double z, 
	                        float (&tensor3x3)[9] )         = 0;

	/// Associated vector data (optional).
	/// By default a zero vector is returned.
	virtual void getVector( double x, double y, double z,
	                        float (&vec)[3] )
	{
		vec[0] = 0.0;
		vec[1] = 0.0;
		vec[2] = 0.0;
	}

	/// This function is called on generating the sample points and only points
	/// where this function returns true are generated. 
	/// By default all point positions are valid if no threshold image was 
	/// specified via \a setImageThreshold().
	virtual bool isValidSamplePoint( double x, double y, double z );

public:	
	void reserveTensorData( unsigned numPoints );
	void reservePoints( unsigned maxNumPoints );

	///@{ Insert point if \a isValidSamplePoint() returns true for the position.
	///   Returns true if point was inserted, false otherwise.
	bool insertPoint( int id, int i, int j, int k );
	bool insertPoint( int id, double x, double y, double z );
	///@}

	Region& subRegion() { return m_subregion; }

private:
	VTKPTR<vtkPoints>         m_samplePoints;
	VTKPTR<vtkDoubleArray>    m_sampleTensors;
	VTKPTR<vtkDoubleArray>    m_sampleVectors;

	ImageDataSpace m_space;

	unsigned m_numPoints;

	// The default implementation of \a isValidSamplePoint() does thresholding
	// based on a given image.
	vtkImageData*  m_thresholdImage;
	ImageDataSpace m_thresholdSpace;
	double m_thresholdLow;
	double m_thresholdHigh;

	// Remember settings on last update()
	Sampling m_lastSampling;
	int      m_lastGridStep[3];

	// Subregion (e.g. for slicing)
	Region m_subregion;

	// Slicing
	int m_sliceDir;
	int m_slice;
};

#endif // TENSORDATAPROVIDER_H
