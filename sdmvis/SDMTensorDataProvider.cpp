#include "SDMTensorDataProvider.h"
#include "StatisticalDeformationModel.h"

void SDMTensorDataProvider::
  setSDM( StatisticalDeformationModel* sdm )
{
	m_sdm = sdm;	

	int dims[3];
	dims[0] = sdm->getHeader().resolution[0];
	dims[1] = sdm->getHeader().resolution[1];
	dims[2] = sdm->getHeader().resolution[2];

	ImageDataSpace space( dims, 
		sdm->getHeader().spacing, sdm->getHeader().offset );
	this->setImageDataSpace( space );
}
