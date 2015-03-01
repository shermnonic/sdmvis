#ifndef LOCALCOVARIANCESTATISTICS_H
#define LOCALCOVARIANCESTATISTICS_H

#include "TensorDataStatistics.h"
#include "TensorData.h"

#include <vtkImageData.h>
#include <vtkSmartPointer.h>

#include <string>
#include <vector>

class SDMTensorDataProvider;
class TensorVisBase;

// Shared types
typedef std::vector<vtkSmartPointer<vtkImageData> > ImageCollection;

//============================================================================
//	StatisticsIntegrator
//============================================================================
/**
	Virtual integrator base class for \a LocalCovarianceStatistics.
*/
class StatisticsIntegrator
{
public:
	StatisticsIntegrator()
		: m_tdata(NULL)
	{}

	///@{ Interface
	virtual void integrate_pre()=0;
	virtual void integrate_inner_loop( TensorDataProvider* tdata, int ijk[3] )=0;
	virtual void integrate_post()=0;

	virtual void save( const char* basepath ) const =0; //{};
	virtual bool load( const char* basepath ) =0; //{};
	///@}

	// Optional
	virtual void setTensorVis( TensorVisBase* tvis ) {};

	// Default functionality
	virtual void setTensorData( SDMTensorDataProvider* tdata );
	virtual void setTensorDataStatistics( /*const*/ TensorDataStatistics& tstat );
	virtual const TensorDataStatistics& getTensorDataStatistics() const;

protected:
	TensorDataStatistics&  tstat() { return m_tstat; }
	SDMTensorDataProvider* tdata() { return m_tdata; }

private:
	TensorDataStatistics m_tstat;
	SDMTensorDataProvider* m_tdata;
};

//============================================================================
//	StatisticsMomentImagesIntegrator
//============================================================================
/**
	Average a scalar tensor measure (and its moments) for \a LocalCovarianceStatistics.
*/
class StatisticsMomentImagesIntegrator : public StatisticsIntegrator
{
public:
	StatisticsMomentImagesIntegrator();

	///@{ StatisticsIntegrator implementation
	void integrate_pre();
	void integrate_inner_loop( TensorDataProvider* tdata, int ijk[3] );
	void integrate_post();

	void setTensorVis( TensorVisBase* tvis );

	void save( const char* basepath ) const;
	bool load( const char* basepath );
	///@}

	void setup( ImageDataSpace& space );

	ImageCollection getMomentImages() { return m_momentImages; }

	void getScalarRange( int modeIdx, double (&range)[2] ) const;

protected:
	// For loaded images min/max ranges have to be computed on-the-fly
	void computeScalarRangeFromImages();

private:
	ImageCollection      m_momentImages; // sub-sampled moment images
	std::vector<double>  m_min, m_max;   // moments min/max values	
};

//============================================================================
//	StatisticsTensorIntegrator
//============================================================================
/**
	Average tensor field for \a LocalCovarianceStatistics.
	Internally TensorDataStatistics::computeAverageTensor() is called.
*/
class StatisticsTensorIntegrator : public StatisticsIntegrator
{
public:
	///@{ StatisticsIntegrator implementation
	void integrate_pre();
	void integrate_inner_loop( TensorDataProvider* tdata, int ijk[3] );
	void integrate_post();

	void save( const char* basepath ) const;
	bool load( const char* basepath );
	///@}

	void setup( ImageDataSpace& space );
	//void save( const char* basepath ) const;
	//bool load( const char* basepath );

private:
	TensorData m_integralField;
	ImageDataSpace m_space;
};

//============================================================================
//	StatisticsTensorPassThru
//============================================================================
/**
	Pass through tensor at given point for \a LocalCovarianceStatistics.
	Used for instance to compute the Tqq tensor field for weighting the plain 
	overview tensor.
*/
class StatisticsTensorPassThru : public StatisticsIntegrator
{
public:
	///@{ StatisticsIntegrator implementation
	void integrate_pre();
	void integrate_inner_loop( TensorDataProvider* tdata, int ijk[3] );
	void integrate_post();

	void save( const char* basepath ) const;
	bool load( const char* basepath );
	///@}

	void setup( ImageDataSpace& space );
	//void save( const char* basepath ) const;
	//bool load( const char* basepath );

private:
	TensorData m_passThruField;
	ImageDataSpace m_space;
};

//============================================================================
//	LocalCovarianceStatistics
//============================================================================
/**
	Iterate over all sampling points and evaluate a \a StatisticsIntegrator.
*/
class LocalCovarianceStatistics
{
public:
	LocalCovarianceStatistics();

	void setTensorData( SDMTensorDataProvider* tdata );	
	void setTensorVis( TensorVisBase* tvis );	

	void setTensorDataStatistics( /*const*/ TensorDataStatistics& tstat );
	const TensorDataStatistics& getTensorDataStatistics() const { return m_tstat; }

	void setSamplingResolution( int stepX, int stepY, int stepZ );

	void compute();

	ImageCollection getMomentImages() 
	{
		return m_integratorMomentImages.getMomentImages();
	}

	void getScalarRange( int modeIdx, double (&range)[2] ) const;

	void save( const char* basepath ) const;
	bool load( const char* basepath );

	enum Integrators
	{
		IntegrateMoments,
		IntegrateTensors,
		PassThru,
	};

	void setIntegrator( int i )
	{
		switch( i )
		{
		default:
		case 0: m_curIntegrator = &m_integratorMomentImages; break;
		case 1: m_curIntegrator = &m_integratorTensor; break;
		case 2: m_curIntegrator = &m_integratorPassThru; break;
		}
	}
	unsigned getNumIntegrators() const { return 2; }
	std::string getIntegratorName( int i ) const 
	{
		switch( i )
		{
		case 0: return std::string("Moments"); break;
		case 1: return std::string("Tensor");  break;
		case 2: return std::string("Pass-thru"); break;
		}
		return std::string("(Unknown integrator)");
	}

private:
	TensorDataStatistics   m_tstat;
	SDMTensorDataProvider* m_tdata;

	StatisticsMomentImagesIntegrator m_integratorMomentImages;
	StatisticsTensorIntegrator       m_integratorTensor;
	StatisticsTensorPassThru         m_integratorPassThru;
	StatisticsIntegrator* m_curIntegrator;

	ImageDataSpace  m_space; // coordinate space of current sub-sampling
	int             m_stepsize[3]; // stepsize of current sub-sampling
};

#endif // LOCALCOVARIANCESTATISTICS_H
