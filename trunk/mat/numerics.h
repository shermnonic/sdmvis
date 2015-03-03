#ifndef NUMERICS_H
#define NUMERICS_H

// Numerics toolbox for SDMVis

#ifndef Q_MOC_RUN 
// Workaround for BOOST_JOIN problem: Undef critical code for moc'ing.
// There is a bug in the Qt Moc application which leads to a parse error at the 
// BOOST_JOIN macro in Boost version 1.48 or greater.
// See also:
//	http://boost.2283326.n4.nabble.com/boost-1-48-Qt-and-Parse-error-at-quot-BOOST-JOIN-quot-error-td4084964.html
//	http://cgal-discuss.949826.n4.nabble.com/PATCH-prevent-Qt-s-moc-from-choking-on-BOOST-JOIN-td4081602.html
//	http://article.gmane.org/gmane.comp.lib.boost.user/71498

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

#endif // ifndef Q_MOC_RUN

#endif
