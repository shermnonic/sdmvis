#ifndef RECONSTRUCTION_H
#define RECONSTRUCTION_H
#include "StatisticalDeformationModel.h"
#include <string>

/// PCA reconstruction of an example deformation from the statistical model
class Reconstruction
{
public:
	typedef StatisticalDeformationModel::ValueType ValueType;

	Reconstruction( const StatisticalDeformationModel& sdm );
	~Reconstruction();

	void computeWarp( unsigned idx, int numModes );

	void synthesizeMode( int mode, double sigma );
	void synthesizeWarp( Vector coeffs, bool considerMean=true );

	bool saveWarp( std::string outputDirectory );

	ValueType* getWarp() { return m_buffer; }

	ValueType* getBuffer();

protected:
	ValueType* allocateBuffer( unsigned size );

private:
	const StatisticalDeformationModel& m_sdm;
	ValueType* m_buffer;
};

#endif // RECONSTRUCTION_H
