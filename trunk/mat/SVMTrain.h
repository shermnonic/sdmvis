#ifndef SVMTRAIN_H
#define SVMTRAIN_H

#include "svm.h"     // libsvm
#include <string>
#include <cassert>
#include <vector>

/// Support Vector Machine training (wrapper for libsvm)
class SVMTrain
{
public:
	SVMTrain();
	~SVMTrain();

	void clear();

	/// Setup problem with m labels and m*n data entries
	void setup_problem( int m, int n, int* labels, double* data );
	/// Set problem label weight (do this after setup_problem() & before train())
	void add_weight( int labels, double weight );
	/// SVM training, if false is returned check error message with getErrmsg()
	bool train();
	/// n-fold cross validation
	double cross_validation( int n );

	/// Get SVM parameter reference
	svm_parameter& params() { return m_param; }
	/// Set new SVM parameters
	void set_params( svm_parameter& param ) { m_param = param; }
	/// Set SVM parameters to default
	static void default_params( svm_parameter& param );	

	std::string getErrmsg() const { return m_errmsg; }

	/// Save model to file in svmlight format
	bool save_model( const char* filename );

	/// Get support vector matrix (row-major) (size of nrows() x ncols())
	double* sv() /*const*/ { return m_sv; }
	/// Get support vector coefficients (size of nrows())
	double* sv_coef() /*const*/ { return m_sv_coef; }

	/// Number of rows of support vector matrix (= total number of SV's)
	int nrows() const { assert(m_model); return m_model->l; }
	/// Number of columns of support vector matrix (= ncols of input matrix)
	int ncols() const { return m_ncols; }

	svm_model* model() /*const*/ { return m_model; }

protected:
	enum Status { Uninitialized, Setup, Trained };
	Status m_status;
	bool sanity_check();

private:
	svm_problem   m_prob;
	svm_parameter m_param;
	svm_model*    m_model;
	svm_node*     m_x_space;
	std::string   m_errmsg;

	std::vector<double> m_labels; // used only by cross_validation

	double* m_sv;
	double* m_sv_coef;
	int     m_ncols; // HACK (see train())
};

#endif // SVMTRAIN_H
