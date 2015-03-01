#ifndef TRAIT_H
#define TRAIT_H

#include "Warpfield.h"
#include "numerics.h"   // Matrix, Vector, rednum::
#include <fstream>
#include <QSettings>
#include <QString>
#include <QMessageBox>
#include <QFile>
/// A trait vector (low-dimensional) and its accompanying warpfield
struct Trait : public Warpfield
{
	Vector trait;     ///< the low-dimensional trait-vector (in "V-space")
	Vector normal;    ///< separating hyperplane normal
	double distance;  ///< separating hyperplane distance to origin
	
	Vector  labels;       ///< classification labels (2 classes, usually -1,1 or 0,1)
	QString classNames[2];///< class names (2 classes)
	
	int numOfComp;
	double weightC;
	double weight1;
	double weight2;
	bool valid;

	QString identifier;   ///< trait identifier
	QString description; 

	QString filename;
	QString matFilename;
	QString fullPathFileName;

	void computeNormal()
	{
		// assume trait vector length is distance to origin
		double length = 0.0;
		for( unsigned int i=0; i < trait.size(); ++i ){
			double traitValue=trait[i];
			length += trait[i]*trait[i];
		}
		length = sqrt( length );

		// compute normalized trait vector
		normal = trait;
		for( unsigned int i=0; i < normal.size(); ++i )
		{
			double debug0=normal[i];
			normal[i] /= length;
			double debug1=normal[i];			
		}
	}

	bool writeTrait(QString iniFilename) // iniFilename is Relativ
	{
		QString errmsg = "";
		QSettings ini( iniFilename, QSettings::IniFormat );

		if(this->valid) // write only valid traits to ini file
		{
			ini.beginGroup("SDMVis_trait_configuration_file");
			ini.setValue("filename",this->filename);
			ini.setValue("identifier"  ,this->identifier);
			ini.setValue("description" ,this->description);
			ini.setValue("normal",vectorToQByteArray(this->normal));
			ini.setValue("distance"    ,this->distance);
			ini.setValue("elementScale",(double)this->elementScale);
			ini.setValue("mhdFilename" ,this->mhdFilename);
			ini.setValue("matFilename" ,this->matFilename);
			ini.setValue("classNameA"  ,this->classNames[0]);
			ini.setValue("classNameB"  ,this->classNames[1]);
			ini.setValue("labels",vectorToQByteArray(this->labels));
			ini.setValue("trait",vectorToQByteArray(this->trait));
			ini.setValue("numOfComp",this->numOfComp);
			ini.setValue("weightC",this->weightC);
			ini.setValue("weight1",this->weight1);
			ini.setValue("weight2",this->weight2);
			ini.setValue("valid",valid);
			ini.endGroup();	
			return true;
		}
		else 
		{			
			return false;
		}	
	}

	void setInvalid()
	{
		this->valid=false;
	}

	bool readTrait (QString iniFilename)
	{
		QFile readFile(iniFilename);	
		if ( !readFile.open( QIODevice::ReadOnly ) )
			return false;
		readFile.close();
		QSettings ini( iniFilename, QSettings::IniFormat );

		QByteArray raw_trait,
			       raw_normal,
				   raw_labels;

		ini.beginGroup("SDMVis_trait_configuration_file");

		this->identifier   = ini.value("identifier"  ).toString();
		this->description  = ini.value("description" ).toString();
		raw_normal		    = ini.value("normal"     ).toByteArray();
		this->distance     = ini.value("distance"    ).toDouble();
		this->elementScale = (float)ini.value("elementScale").toDouble();
		this->mhdFilename  = ini.value("mhdFilename" ).toString();
		this->classNames[0]= ini.value("classNameA"  ).toString();
		this->classNames[1]= ini.value("classNameB"  ).toString();
		this->filename     = ini.value("filename"    ).toString();
		this->matFilename  = ini.value("matFilename" ).toString();
		raw_labels = ini.value("labels").toByteArray();
		raw_trait  = ini.value("trait").toByteArray();

		this->numOfComp=ini.value("numOfComp").toInt();
		this->weightC=ini.value("weightC").toDouble();
		this->weight1=ini.value("weight1").toDouble();
		this->weight2=ini.value("weight2").toDouble();
		this->valid  =ini.value("valid").toBool();

		ini.endGroup();

		this->trait.resize( raw_trait.size() / sizeof(double) );
		this->labels.resize( raw_labels.size()/ sizeof(double) );
		this->normal.resize( raw_normal.size()/ sizeof(double) );
		rednum::vector_from_rawbuffer<double,Vector,double>( this->trait,  (double*)raw_trait.data() );
		rednum::vector_from_rawbuffer<double,Vector,double>( this->labels, (double*)raw_labels.data() );
		rednum::vector_from_rawbuffer<double,Vector,double>( this->normal, (double*)raw_normal.data() );
		return true;
	}
	


	//------------------------------------------------------------------------------
	//	HELPER FUNCTIONS
	//------------------------------------------------------------------------------

	size_t get_filesize( const char* filename )
	{
		using namespace std;
		ifstream fs( filename );
		size_t size;
		if( fs.is_open() )
		{
			fs.seekg( 0, ios::end );
			size = (size_t)fs.tellg();
			fs.close();
			return size;
		}
		return 0;
	}

	QByteArray vectorToQByteArray( Vector v )
	{	
		double* raw_v = rednum::vector_to_rawbuffer<double>( v );
		QByteArray tmp( (char*)raw_v, v.size()*sizeof(double) );
		delete [] raw_v;
		return tmp;
	}

	QByteArray matrixToQByteArray( Matrix M )
	{
		double* raw_M = rednum::matrix_to_rawbuffer<double>( M );
		QByteArray tmp( (char*)raw_M, M.size1()*M.size2()*sizeof(double) );
		delete [] raw_M;
		return tmp;
	}
	

};


#endif // TRAIT_H
