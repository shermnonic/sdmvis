#ifndef NUMERICS_H
#define NUMERICS_H

// Numerics toolbox for SDMVis

#include "rednum.h"    // rednum (svd functions)
#include "SVMTrain.h"  // libsvm wrapper

// used vector and matrix types
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/vector.hpp>
typedef double ValueType;
namespace rednum {
	typedef double ValueType;
};
typedef boost::numeric::ublas::matrix<ValueType> Matrix;
typedef boost::numeric::ublas::vector<ValueType> Vector;


#endif
