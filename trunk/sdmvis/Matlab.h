#ifndef MATLAB_WRAPPER_H
#define MATLAB_WRAPPER_H
// 2011-04-11 bugfixed indices in uploadToMatlab and downloadFromMatlab

#include <engine.h> // Matlab engine

/// Download a matrix from Matlab
/// \param m number of rows
/// \param n number of columns
template<class T>
bool downloadFromMatlab( Engine* ep, const char* name, int m, int n, T* v )
{
	mxArray* vArr = NULL;
	vArr = engGetVariable( ep, name );
	if( vArr != NULL )
	{
		double *vPtr = mxGetPr( vArr );

		// element-wise copy 
		//(Matlab stores column-wise, we do it row-wise)
		for( int i=0; i < m; ++i )     // row i
			for( int j=0; j < n; ++j ) // column j
				v[ i*n + j ] = (T) vPtr[ j*m + i ];

		// clean-up
		mxDestroyArray( vArr );

		return true;
	}
	return false;
}

/// Upload matrix to Matlab
/// \param m number of rows
/// \param n number of columns
template<class T>
void uploadToMatlab( Engine* ep, const char* name, int m, int n, T* c )
{
	mxArray* cArr = mxCreateDoubleMatrix( m, n, mxREAL );
	double* cPtr = mxGetPr( cArr );
	
	// element-wise copy 
	//(Matlab stores column-wise, we do it row-wise)
	for( int i=0; i < m; ++i )     // row i
		for( int j=0; j < n; ++j ) // column j
			cPtr[ j*m + i ] = c[ i*n + j ];

	engPutVariable( ep, name, cArr );

	// seems to be save to destroy mxArray here already
	mxDestroyArray( cArr );
}

/**
	Matlab engine wrapper
*/
struct Matlab
{
	Engine* ep;
	char outbuf[65536];

	Matlab();
	~Matlab();

	/// Matlab engine must be initialized
	bool init();

	/// Evaluate Matlab command
	int  eval( const char* cmd ) const;

	/// Change Matlab working directory relative to programs working directory on init() call.
	int cd( const char* path );

	void setSilent( bool b ) { silent = b; }
	bool silent;
};

#endif
