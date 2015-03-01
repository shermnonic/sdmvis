#ifndef TENSORDATASTATISTICS_H
#define TENSORDATASTATISTICS_H

#include "TensorVisBase.h" // contains also TensorSpectrum
#include "TensorDataProvider.h"

#include <vector>
#include <string>

#ifdef TENSORDATASTATISTICS_SUPPORT_MOMENT_IMAGES
#include <vtkImageData>
#inlcude <vtkSmartPointer>
#endif

/// Compute average and variance of tensor measures like fractional anisotropy.
class TensorDataStatistics
{
public:
	TensorDataStatistics();

	enum Measures { MaxEigenvalue, MeanDiffusivity, FractionalAnisotropy, 
		            FrobeniusNorm,
		            NumMeasures };
	enum Moments  { Average, Variance, 
		            NumMoments };

	/// Set \a TensorDataProvider (required).
	void setTensorData( TensorDataProvider* tdata );
	
	/// Associate \a TensorVisBase instance (optional).
	/// For now we use this only to synchronize the option, if eigenvalues have
	/// to be extracted from the provided data matrix. 
	/// \sa TensorVisBase::getExtractEigenvalues().
	void setTensorVisBase( TensorVisBase* tvis );
	
	/// Compute first and second order moments on given tensor data.
	/// \sa setSamplingResolution()
	void compute();

	void computeAverageTensor();

	/// Returns the specified measure and moment.
	/// \sa compute()
	double getMoment( int moment, int measure ) const;

	void getAverageTensor( double (&tensor)[9] ) const;

	std::vector<double> getMomentMeasures() const { return m_moment; }

	unsigned getNumMomentMeasures() const { return NumMeasures*NumMoments; }

	std::string getMomentMeasureName( unsigned index ) const;
	int getIndex( int moment, int measure ) const;

	TensorDataProvider* getTensorData() { return m_tdata; }
	TensorVisBase* getTensorVisBase() { return m_tvis; }

protected:
	TensorSpectrum getSpectrum( double* tensor9 );
	
private:
	TensorDataProvider* m_tdata;
	TensorVisBase*      m_tvis;
	std::vector<double> m_moment;

	double              m_avgTensor[9];

#ifdef TENSORDATASTATISTICS_SUPPORT_MOMENT_IMAGES
public:
	typedef std::vector<vtkSmartPointer<vtkImageData> > ImageCollection;
	void updateTensorData();
	/// Specify the subsampling for the computation.
	void setSamplingResolution( int stepX, int stepY, int stepZ );
protected:
	float* getScalarPointer( vtkImageData* img, const double (&point)[3] );
private:
	ImageCollection m_momentImages;
	int             m_stepsize[3];
#endif
};

#endif // TENSORDATASTATISTICS_H
