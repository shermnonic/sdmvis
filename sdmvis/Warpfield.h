#ifndef WARPFIELD_H
#define WARPFIELD_H

#include <QString>

/// Config options of a single warpfield
struct Warpfield
{
	Warpfield() 
		: mhdFilename(QString()),
		  elementScale(0.1f)
	{}

	QString mhdFilename;
	float   elementScale;

	void setMHDfilename(QString name){mhdFilename=name;}
	void setElementScale(float scale){elementScale=scale;}
};

#endif // WARPFIELD_H
