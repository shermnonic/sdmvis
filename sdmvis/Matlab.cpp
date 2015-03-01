#include "Matlab.h"
#include <direct.h>	// for _chdir and _getcwd
#include <iostream>
#include <string>
using namespace std;

Matlab::Matlab()
: ep(0),
  silent(false)
{}

Matlab::~Matlab()
{
	if( ep ) {
		cout << "Closing Matlab engine...";
		engClose( ep );
		cout << "done" << endl;
	}
}

bool Matlab::init()
{
	cout << "Opening Matlab engine...";
	ep = engOpen("");
	if( !ep )
	{
		cout << "failed!" << endl;
		return false;
	}
	engOutputBuffer( ep, outbuf, sizeof(outbuf) );
	cout << "done" << endl;

	// change per default into programs current working dir
	char cwd[_MAX_PATH];
	_getcwd( cwd, sizeof(cwd) );
	cd( cwd );

	return true;			
}

int Matlab::eval( const char* cmd ) const
{
	if( !ep ) return -666;
	int ret = engEvalString( ep, cmd );
	if( !silent )
	{
		cout << "Matlab command: " << endl << cmd << endl
		     << "Return code: " << ret << endl
		     << "Output: " << endl << outbuf << endl;
	}
	return ret;
}

int Matlab::cd( const char* dir )
{
	if( !ep ) return -666;

	string cmd = string("cd ") + string(dir);
	int ret = eval( cmd.c_str() );
	eval( "pwd" );
	return ret;
}
