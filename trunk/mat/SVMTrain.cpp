#include "SVMTrain.h"
#include <iostream>
#include <cstdlib>   // malloc()

SVMTrain::SVMTrain()
{
	m_status = Uninitialized;
	m_prob.y  = NULL;
	m_prob.x  = NULL;
	m_model   = NULL;
	m_x_space = NULL;
	default_params( m_param );
	m_errmsg = "";
	m_ncols   = 0;
	m_sv      = NULL;
	m_sv_coef = NULL;
}

SVMTrain::~SVMTrain()
{
	clear();
}

void SVMTrain::clear()
{
	// if( m_status == Trained ) {
	svm_free_and_destroy_model( &m_model ); m_model=NULL;
	// }
	free(m_param.weight_label); m_param.weight_label=NULL;
	free(m_param.weight);       m_param.weight=NULL;

	free( m_prob.y );   m_prob.y = NULL;
	free( m_prob.x );   m_prob.x = NULL;
	free( m_x_space );  m_x_space = NULL;

	default_params( m_param );

	free( m_sv );      m_sv = NULL;
	free( m_sv_coef ); m_sv_coef = NULL;
	m_ncols = 0;

	m_errmsg = "";
	m_status = Uninitialized;
}

void SVMTrain::default_params( svm_parameter& param )
{
	// copied default parameters from svm-train.c example program
	param.svm_type		= C_SVC;
	param.kernel_type	= RBF;
	param.degree		= 3;
	param.gamma			= 0;	// 1/num_features
	param.coef0			= 0;
	param.nu			= 0.5;
	param.cache_size	= 100;
	param.C				= 1;
	param.eps			= 1e-3;
	param.p				= 0.1;
	param.shrinking		= 1;
	param.probability	= 0;
	param.nr_weight		= 0;
	param.weight_label	= NULL;
	param.weight		= NULL;
}

void SVMTrain::setup_problem( int m, int n, int* labels, double* data )
{
	if( m_status != Uninitialized )
		clear();

	// copy labels for cross_validation
	m_labels.resize( m );
	for( int r=0; r < m; ++r )
		m_labels[r] = (double)labels[r];

	// total number of elements
	int elements = m*n + m+1; // plus additional "end-of-row" elements

	// number of rows
	m_prob.l = m;
	
	// HACK: store number of columns
	m_ncols = n;

	m_prob.y = (double*)   malloc( m_prob.l * sizeof(double) );
	m_prob.x = (svm_node**)malloc( m_prob.l * sizeof(svm_node*) );

	// data elements
	m_x_space = (svm_node*)malloc( elements * sizeof(svm_node) );
	// loop over x_space (xi), data (di) and its rows (r) and columns (c)
	for( int xi=0, di=0, r=0; r < m; ++r ) {
		m_prob.x[r] = &m_x_space[xi];
		m_prob.y[r] = (double)labels[r]; // why is y of type double* (libsvm only allows int labels?)
		for( int c=0; c < n; ++c, ++xi, ++di ) {
			// consistently label the elements with their column index (starting from 1)
			m_x_space[xi].index = c + 1;
			m_x_space[xi].value = data[di];
		}
		// additional "end-of-row" element
		m_x_space[xi].index = -1;
		m_x_space[xi].value = 0.0f; // just for debugging purposes
		xi++;
	}

	// set default gamma parameter
	if( m_param.gamma == 0 )
		m_param.gamma = 1./m;

	m_status = Setup;
}

void SVMTrain::add_weight( int label, double weight )
{
	++m_param.nr_weight;
	m_param.weight_label = (int *)   realloc( m_param.weight_label,sizeof(int)   *m_param.nr_weight );
	m_param.weight       = (double *)realloc( m_param.weight,      sizeof(double)*m_param.nr_weight );
	m_param.weight_label[m_param.nr_weight-1] = label;
	m_param.weight      [m_param.nr_weight-1] = weight;
}

bool SVMTrain::sanity_check()
{
	// sanity check
	if( m_status != Setup )
	{
		m_errmsg = "You have to call SDMTrain::setup_problem() before you can train or cross-validate!\n";
		std::cerr << "Error (SVMTrain): " << m_errmsg << std::endl;
		return false;
	}
	
	// check parameters
	const char* error_msg = svm_check_parameter( &m_prob, &m_param );
	if( error_msg )
	{
		m_errmsg = std::string(error_msg);
		std::cerr << "Error (SVMTrain): " << m_errmsg << std::endl;
		return false;
	}

	return true;
}

double SVMTrain::cross_validation( int n )
{
	if( !sanity_check() )
		return -1.0;

	// n-fold cross validation
	int m = m_prob.l; // number of samples
	//double* target = new double[m];
	std::vector<double> target( m, -666.0 );

	// from svm.h:
	// void svm_cross_validation(const struct svm_problem *prob, 
	//          const struct svm_parameter *param, int nr_fold, double *target);
	svm_cross_validation( &m_prob, &m_param, n, &target[0] );

	using namespace std;
	for( int i=0; i < m; ++i )
		cout << "target[" << i << "] = " << target[i] << endl;

	// evaluate score
	double score=0.0;
	for( int i=0; i < m; ++i )
		score += (target[i]-m_labels[i])*(target[i]-m_labels[i]);
	return score / (double)m;
}

bool SVMTrain::train()
{
	if( !sanity_check() )
		return false;

	// svm training
	m_model = svm_train( &m_prob, &m_param );
	assert( m_model );

	////////////////////////////////////////////////////////////////////
	// HACK: Copy support vectors and coefficients to full matrices,
	//       assumes two-class SVM and that result is not sparse.
	////////////////////////////////////////////////////////////////////
	assert(m_sv==NULL);
	assert(m_sv_coef==NULL);

	// copy sparse model.SV to full matrix m_sv
	m_sv = (double*)malloc( m_model->l * m_ncols * sizeof(double) );
	for( int r=0, di=0; r < m_model->l; ++r ) {
		for( int c=0; c < m_ncols; ++c, ++di ) {
			// HACK: Looping over number of columns and ignoring possible
			//       sparse representation (should not be the case for our
			//       application). Correct would be to iterate until svm_node
			//       with special index -1 is met.
			m_sv[di] = (m_model->SV[r])[c].value;
		}
	}

	m_sv_coef = (double*)malloc( m_model->l * sizeof(double) );
	for( int r=0; r < m_model->l; ++r )
		m_sv_coef[r] = m_model->sv_coef[ 0 ][r];

	m_status = Trained;
	return true;
}

bool SVMTrain::save_model( const char* filename )
{
	if( m_status != Trained )
	{
		m_errmsg = "You have to train the model first with SDMTrain::train() before saving it!\n";
		std::cerr << "Error (SVMTrain): " << m_errmsg << std::endl;
		return false;
	}

	if( svm_save_model( filename, m_model ) )
	{
		m_errmsg = "Can't save model to file!";
		std::cerr << "Error (SVMTrain): " << m_errmsg << std::endl;
		return false;
	}
	
	return true;
}
