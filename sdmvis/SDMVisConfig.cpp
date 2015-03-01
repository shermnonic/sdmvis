#include "SDMVisConfig.h"
#include <QObject>
#include <QSettings>
#include <QDir>
#include <QString>
#include <fstream>
#include <iostream>
#include <string>

// Versions Number 
int SDMVisConfig::appVersion = 5;

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

//------------------------------------------------------------------------------
//	sanityCheck()
//------------------------------------------------------------------------------
bool SDMVisConfig::sanityCheck()
{
	// TODO: additional sanity checks, eventually fill error message?

	// sanity checks
	if( (sdm.scatter_t.size1() != sdm.V.size1()) ||
		(sdm.V.size2() != sdm.lambda.size()) )
	{
		errmsg = QObject::tr("Config error, mismatching matrix dimensions!");
		return false;
	}	
	return true;
}

//------------------------------------------------------------------------------
//	getRelativePath()
//------------------------------------------------------------------------------
QString SDMVisConfig::getRelativePath( QString path, QString relativeTo ) const
{
	if( relativeTo.isEmpty() )
		relativeTo = sdm.basePath;
	QDir baseDir( relativeTo );
	QString returnValue=baseDir.relativeFilePath( path );
	return baseDir.relativeFilePath( path );
}

//------------------------------------------------------------------------------
//	getAbsolutePath()
//------------------------------------------------------------------------------
QString SDMVisConfig::getAbsolutePath( QString filename ) const
{
	QDir baseDir( sdm.basePath );
	QString some=	baseDir.absoluteFilePath( filename );
	return some;
}

//------------------------------------------------------------------------------
//	updateRelativePaths()
//------------------------------------------------------------------------------
void SDMVisConfig::updateRelativePaths( QString newBasePath, QString oldBasePath )
{
	if( oldBasePath.isEmpty() )
		oldBasePath = sdm.basePath;

	QDir oldBaseDir( oldBasePath );
	// TODO : find ERROR 

	QString oldRef=sdm.reference;


	sdm.reference   = getRelativePath( oldBaseDir.filePath( sdm.reference   ), newBasePath );
	sdm.warpsMatrix = getRelativePath( oldBaseDir.filePath( sdm.warpsMatrix ), newBasePath );
	sdm.eigenwarpsMatrix   = getRelativePath( oldBaseDir.filePath( sdm.eigenwarpsMatrix   ), newBasePath );
	for( int i=0; i < sdm.eigenwarps.size(); ++i )
		sdm.eigenwarps[i].mhdFilename = getRelativePath( oldBaseDir.filePath( sdm.eigenwarps[i].mhdFilename ), newBasePath );
	if( !sdm.meanwarp.mhdFilename.isEmpty() )
		sdm.meanwarp.mhdFilename = getRelativePath( oldBaseDir.filePath(sdm.meanwarp.mhdFilename), newBasePath );

	// TODO: update traits, update ROI
}

//------------------------------------------------------------------------------
//	readConfig()
//------------------------------------------------------------------------------
bool SDMVisConfig::readConfig( QString iniFilename )
{
	errmsg = QString();
	QSettings ini( iniFilename, QSettings::IniFormat );

	// extract absolute path and filename
	QFileInfo info( iniFilename );

	m_configFilename = info.fileName();

	// all paths stored in config are relative to config file path
	sdm.basePath = info.absolutePath();

	// read header
	ini.beginGroup("SDMVis_configuration_file");
	version = ini.value("Version",0).toInt();
	sdm.identifier  = ini.value("Identifier","NOID").toString();
	sdm.description = ini.value("Description","(No description)").toString();
	sdm.sdmprocIni = ini.value("sdmprocIni","").toString();
	ini.endGroup();	

	// --- SDM Warp Dataset ---------------------------------------------------
	
	// read reference
	QString reference;
	ini.beginGroup("reference");
	if( ini.contains("mhdFilename") )
		reference = ini.value("mhdFilename").toString();
	ini.endGroup();
	sdm.reference = reference;

	// read meanwarp (optional)
	Warpfield meanwarp;
	ini.beginGroup("meanwarp");
	meanwarp.mhdFilename  = ini.value("mhdFilename" ,QString()     ).toString();
	meanwarp.elementScale = ini.value("elementScale",QString("1.0")).toString().toFloat();
	ini.endGroup();
	sdm.meanwarp = meanwarp;
	
	// read eigenwarps
	QList<Warpfield> warpfields;
	int numWarpfields = ini.beginReadArray("eigenwarps");
	for( int i=0; i < numWarpfields; ++i )
	{
		ini.setArrayIndex(i);
		Warpfield wf;
		wf.mhdFilename  = ini.value("mhdFilename" ).toString();
		wf.elementScale = ini.value("elementScale").toString().toFloat(); 
		  // directly toFloat() only for Qt specific hex representation of value

		warpfields.push_back( wf );
	}
	ini.endArray();
	sdm.eigenwarps = warpfields;

	// read names (optional)
	QStringList names;
	int numNames = ini.beginReadArray("dataset");
	for( int i=0; i < numNames; ++i )
	{
		ini.setArrayIndex(i);
		names.push_back( ini.value("name").toString() );
	}
	ini.endArray();
	sdm.names = names;

	// read warps matrix filename (optional)
	QString warpsMatrix;
	ini.beginGroup("warpsMatrix");
	if( ini.contains("matFilename") )
		warpsMatrix = ini.value("matFilename").toString();
	ini.endGroup();
	sdm.warpsMatrix = warpsMatrix;

	// read eigenwarps matrix filename (optional)
	QString eigenwarpsMatrix;
	ini.beginGroup("eigenwarpsMatrix");
	if( ini.contains("matFilename") )
		eigenwarpsMatrix = ini.value("matFilename").toString();
	ini.endGroup();
	sdm.eigenwarpsMatrix = eigenwarpsMatrix;

	// sanity checks
	if( version != appVersion )
	{
		errmsg = QObject::tr("Config version mismatch!\n"
			"Probably this config was created with a different version of SDMVis.");
		return false;
	}
	if( reference.isEmpty() )
	{
		errmsg = QObject::tr("Config fileformat mismatch!");
		return false;
	}

	
	// --- SDM PCA Model ------------------------------------------------------
	
	// read PCA model
	QByteArray raw_scatter_t, raw_V, raw_lambda;
	int N, n; // scatter_t is N x N,  V is N x n,  lambda is n x 1,  n <= N
	ini.beginGroup("yapca");
	N = ini.value("N").toInt();
	n = ini.value("n").toInt();	
	raw_scatter_t = ini.value("scatter_t").toByteArray();
	raw_V         = ini.value("V")        .toByteArray();
	raw_lambda    = ini.value("lambda")   .toByteArray();
	ini.endGroup();
	
	// sanity checks
	if( n<0 || N <0 || n>N )
	{
		errmsg = QObject::tr("Config error in group yapca, invalid values for n and N!");
		return false;
	}
	if( (raw_scatter_t.size() / sizeof(double) != N*N) ||
		(raw_V        .size() / sizeof(double) != N*n) ||
	    (raw_lambda   .size() / sizeof(double) != n) )
	{
		errmsg = QObject::tr("Config error in group yapca, matrix size mismatch!");
		return false;
	}
	
	sdm.scatter_t.resize(N,N);
	sdm.V        .resize(N,n);
	sdm.lambda   .resize(n);
	rednum::matrix_from_rawbuffer<double,Matrix,double>( sdm.scatter_t, (double*)raw_scatter_t.data() );
	rednum::matrix_from_rawbuffer<double,Matrix,double>( sdm.V,         (double*)raw_V        .data() );
	rednum::vector_from_rawbuffer<double,Vector,double>( sdm.lambda,    (double*)raw_lambda   .data() );


	// --- Traits -------------------------------------------------------------
	QStringList failedTraitList;

	int ntraits = ini.beginReadArray("traits");
	traits.clear();
	for( int i=0; i < ntraits; ++i )
	{
		ini.setArrayIndex( i );
		Trait t;
		QString relName=ini.value("filename"  ).toString();
		QString absName=getAbsolutePath(relName);
		if (t.readTrait(absName))
			traits.push_back( t );
		else
			failedTraitList.push_back(relName);
	}
	ini.endArray();
	if (!failedTraitList.isEmpty())
	{
		QMessageBox msgBox;
		QString message=QObject::tr("Warning: \n");
		for (int iA=0;iA<failedTraitList.size();iA++)
			message.append(QObject::tr("File Not Found ")+ failedTraitList.at(iA)+"\n");
		msgBox.setText(message);	
		msgBox.exec();
	}

	// --- ROI ----------------------------------------------------------------

	int numRoi= ini.beginReadArray("ROI");
	for( int i=0; i < numRoi; ++i )
	{
		ini.setArrayIndex(i);
		ROI tempRoi;
		tempRoi.radius= ini.value("radius").toDouble();
		tempRoi.center[0]=ini.value("centerX").toDouble();
		tempRoi.center[1]=ini.value("centerY").toDouble();
		tempRoi.center[2]=ini.value("centerZ").toDouble();
	
		addRoi(tempRoi);

	}
	ini.endArray();

	// --- VarVis -------------------------------------------------------------

	#ifdef SDMVIS_VARVIS_ENABLED
	// VarVis stuff
	ini.beginGroup("mesh");
	sdm.m_meshFilename=ini.value("meshFilename").toString();
	sdm.m_isoValue=ini.value("meshIsoValue").toDouble();
	ini.endGroup();

	ini.beginGroup("SamplePoints");
	sdm.m_pointsFileName=ini.value("sampleFilename").toString();
	sdm.m_numberOfSamplePoints=ini.value("numberOfSamplePoints").toInt();
	sdm.m_samplePointSize=ini.value("samplePointSize").toDouble();
	ini.endGroup();
	#endif

	// --- Lookmarks ----------------------------------------------------------

	QStringList lookmarks;
	int numLookmarks = ini.beginReadArray("lookmarks");
	for( int i=0; i < numLookmarks; ++i )
	{
		ini.setArrayIndex(i);
		lookmarks.push_back( ini.value("filename").toString() );
	}
	ini.endArray();
	m_lookmarks = lookmarks;

#ifdef SDMVIS_TENSORVIS_ENABLED
	// --- Tensor Probes ------------------------------------------------------

	QList<SDMTensorProbe> probes;
	int numProbes = ini.beginReadArray("tensorProbes");
	for( int i=0; i < numProbes; i++ )
	{
		ini.setArrayIndex(i);

		SDMTensorProbe probe;

		probe.name = ini.value("name", "(unnamed)").toString().toStdString();

		probe.point[0] = ini.value("point_x").toDouble();
		probe.point[1] = ini.value("point_y").toDouble();
		probe.point[2] = ini.value("point_z").toDouble();
		probe.gamma    = ini.value("gamma"  ).toDouble();
		probe.radius   = ini.value("radius" ).toDouble();

		probe.samplingStrategy = ini.value("samplingStrategy" ).toInt();
		probe.samplingThreshold= ini.value("samplingThreshold").toDouble();
		probe.samplingSize     = ini.value("samplingSize"     ).toInt();
		probe.samplingStepsize = ini.value("samplingStepSize" ).toInt();

		probe.glyphType              = ini.value("glyphType"       ).toInt();
		probe.glyphScaleFactor       = ini.value("glyphScaleFactor").toDouble();
		probe.glyphColorBy           = ini.value("glyphColorBy"    ).toInt();
		probe.glyphExtractEigenvalues= ini.value("glyphExtractEigenvalues").toBool();
		probe.glyphSuperquadricGamma = ini.value("glyphSuperquadricGamma" ).toDouble();

		probes.push_back( probe );
	}
	ini.endArray();
	m_tensorProbes = probes;

/*
	double point[3]; ///< Position (in physical coordinates)
	double gamma;
	double radius;
	
	// TensorvisBase sampling
	int    samplingStrategy;
	double samplingThreshold;
	int    samplingSize;
	int    samplingStepsize;
	
	// TensorvisBase glyph
	int    glyphType;
	double glyphScaleFactor;
	int    glyphColorBy;
	bool   glyphExtractEigenvalues;
	double glyphSuperquadricGamma;	
*/
#endif

	// --- Raycaster settings -------------------------------------------------
	ini.beginGroup("raycaster");
	m_raycasterIsovalue    = ini.value("raycasterIsoValue"   ).toDouble();
	m_raycasterIsoStepsize = ini.value("raycasterIsoStepsize").toDouble();
	m_raycasterDVRStepsize = ini.value("raycasterDVRStepsize").toDouble();
	m_raycasterDVRAlpha    = ini.value("raycasterDVRAlpha"   ).toDouble();
	ini.endGroup();

	return true;	
}

//------------------------------------------------------------------------------
// writeConfig
//------------------------------------------------------------------------------
void SDMVisConfig::writeConfig( QString iniFilename )
{
	errmsg = "";
	QSettings ini( iniFilename, QSettings::IniFormat );

	// write header
	ini.beginGroup("SDMVis_configuration_file");
	ini.setValue("Version"    ,appVersion);
	ini.setValue("Identifier" ,sdm.identifier);
	ini.setValue("Description",sdm.description);
	ini.setValue("basePath"   ,sdm.basePath);
	ini.setValue("sdmprocIni" ,sdm.sdmprocIni);
	ini.endGroup();
	
	// --- SDM Warp Dataset ---------------------------------------------------

	// write reference
	ini.beginGroup("reference");
	ini.setValue("mhdFilename",sdm.reference);
	ini.endGroup();

	// write meanwarp (optional)
	if( !sdm.meanwarp.mhdFilename.isEmpty() )
	{
		QString relPath=sdm.meanwarp.mhdFilename;
		relPath.remove(sdm.basePath+"/");

		ini.beginGroup("meanwarp");
		ini.setValue( "mhdFilename", relPath );
		ini.setValue( "elementScale", QString::number((double)sdm.meanwarp.elementScale) );
		ini.endGroup();
	}

	// write warpfields
	ini.beginWriteArray("eigenwarps");
	for( int i=0; i < sdm.eigenwarps.size(); ++i )
	{
		ini.setArrayIndex(i);
		QString relPath=sdm.eigenwarps.at(i).mhdFilename  ;
		relPath.remove(sdm.basePath+"/");
		ini.setValue( "mhdFilename" , relPath  );
		ini.setValue( "elementScale", 
			QString::number((double)sdm.eigenwarps.at(i).elementScale) );
		  // direct setValue(.,float) produces Qt specific hex representation of value
	}
	ini.endArray();

	// write warpsMatrix filename (optional)
	if( !sdm.warpsMatrix.isEmpty() )
	{
		ini.beginGroup("warpsMatrix");
			ini.setValue("matFilename",sdm.warpsMatrix);
		ini.endGroup();
	}

	// write eigenwarpsMatrix filename (optional)
	if( !sdm.eigenwarpsMatrix.isEmpty() )
	{
		ini.beginGroup("eigenwarpsMatrix");
			ini.setValue("matFilename",sdm.eigenwarpsMatrix);
		ini.endGroup();
	}

	// write names (optional)
	if( !sdm.names.isEmpty() )
	{
		ini.beginWriteArray("dataset");
			for( int i=0; i < sdm.names.size(); ++i )
			{
				ini.setArrayIndex(i);
				ini.setValue( "name", sdm.names.at(i) );
			}
		ini.endArray();
	}
	
	// --- SDM PCA Model ------------------------------------------------------
	
	// read PCA model
	QByteArray raw_scatter_t, raw_V, raw_lambda;

	int N, n; // scatter_t is N x N,  V is N x n,  lambda is n x 1,  n <= N
	N = sdm.scatter_t.size1();
	n = sdm.V.size2();
	
	if( N>0 && n>0 )
	{	
		ini.beginGroup("yapca");
			ini.setValue("N",N);
			ini.setValue("n",n);
	#if 1
			ini.setValue("scatter_t",matrixToQByteArray(sdm.scatter_t));
			ini.setValue("V"        ,matrixToQByteArray(sdm.V));
			ini.setValue("lambda"   ,vectorToQByteArray(sdm.lambda));
	#else
			double *raw_scatter_t = rednum::matrix_to_rawbuffer<double>( sdm.scatter_t ),
				   *raw_V         = rednum::matrix_to_rawbuffer<double>( sdm.V ),
				   *raw_lambda    = rednum::vector_to_rawbuffer<double>( sdm.lambda );
			
			int sizeScatterT = sdm.scatter_t.size1() * sdm.scatter_t.size2() * sizeof(double),
				sizeV        = sdm.V.size1() * sdm.V.size2() * sizeof(double),
				sizeLambda   = sdm.lambda.size() * sizeof(double);
			
			ini.setValue("scatter_t", QByteArray((char*)raw_scatter_t, sizeScatterT));
			ini.setValue("V",         QByteArray((char*)raw_V,         sizeV));
			ini.setValue("lambda",    QByteArray((char*)raw_lambda,    sizeLambda));
			
			delete [] raw_scatter_t;
			delete [] raw_V;
			delete [] raw_lambda;
	#endif	
		ini.endGroup();
	}
	
	// --- ROI ----------------------------------------------------------------
	if (!m_roiVector.isEmpty())
	{
		ini.beginWriteArray("ROI",m_roiVector.size());
		for( int i=0; i < m_roiVector.size(); ++i )
		{
			ini.setArrayIndex(i);
			ini.setValue( "radius" , m_roiVector.at(i).radius );
			ini.setArrayIndex(i);
			ini.setValue( "centerX" ,m_roiVector.at(i).center[0]);
			ini.setArrayIndex(i);
			ini.setValue( "centerY" , m_roiVector.at(i).center[1]);
			ini.setArrayIndex(i);
			ini.setValue( "centerZ" , m_roiVector.at(i).center[2]);
		}
		ini.endArray();
	}

	// --- Traits -------------------------------------------------------------	
	QStringList failedTraitsList;
	int traitsize=traits.size();
	int validTraits=0;
	if (!traits.isEmpty())
	{
		ini.beginWriteArray("traits");
		for( int i=0; i < traits.size(); ++i )
		{
			bool valid=traits.at(i).valid;
			if (!valid){
				failedTraitsList.push_back(traits.at(i).filename);
				continue;
			}
			ini.setArrayIndex( validTraits );
			ini.setValue("filename",traits.at(i).filename);
			// overwrite Description
			traits[i].writeTrait(getBasePath()+"/"+traits.at(i).filename);
			validTraits++;
		}
		ini.endArray();
	}
	if (traits.size()>0 && failedTraitsList.size()>0){
		QMessageBox msgBox;
		QString message=QObject::tr("Warning: \n");
		for (int iA=0;iA<failedTraitsList.size();iA++)
			message.append(failedTraitsList.at(iA)+QObject::tr(", skipped : Trait is INVALID, calculate trait and warpfield!\n"));
		msgBox.setText(message);
		msgBox.exec();
	}

#ifdef SDMVIS_VARVIS_ENABLED
	// --- VarVis stuff -------------------------------------------------------

	ini.beginGroup("mesh");
	ini.setValue("meshFilename",sdm.m_meshFilename);
	ini.setValue("meshIsoValue",sdm.m_isoValue);
	ini.endGroup();
	if (!sdm.m_pointsFileName.isEmpty())
	{
		ini.beginGroup("SamplePoints");
		ini.setValue("sampleFilename",sdm.m_pointsFileName);
		ini.setValue("numberOfSamplePoints",sdm.m_numberOfSamplePoints);
		ini.setValue("samplePointSize",sdm.m_samplePointSize);
		ini.endGroup();
	}
#endif

	// --- Lookmarks ----------------------------------------------------------

	if( !m_lookmarks.isEmpty() )
	{
		ini.beginWriteArray("lookmarks");
		for( int i=0; i < m_lookmarks.size(); ++i )
		{
			ini.setArrayIndex(i);
			ini.setValue( "filename", m_lookmarks.at(i) );
		}
		ini.endArray();
	}

#ifdef SDMVIS_TENSORVIS_ENABLED
	// --- Tensor Probes ------------------------------------------------------

	if( !m_tensorProbes.isEmpty() )
	{
		ini.beginWriteArray("tensorProbes");
		for( int i=0; i < m_tensorProbes.size(); i++ )
		{
			SDMTensorProbe probe = m_tensorProbes[i];

			ini.setArrayIndex(i);

			ini.setValue("name", QString::fromStdString(probe.name));

			ini.setValue("point_x", probe.point[0]);
			ini.setValue("point_y", probe.point[1]);
			ini.setValue("point_z", probe.point[2]);
			ini.setValue("gamma", probe.gamma );
			ini.setValue("radius",probe.radius);

			ini.setValue("samplingStrategy" ,probe.samplingStrategy );
			ini.setValue("samplingThreshold",probe.samplingThreshold);
			ini.setValue("samplingSize"     ,probe.samplingSize     );
			ini.setValue("samplingStepSize" ,probe.samplingStepsize );

			ini.setValue("glyphType"       ,probe.glyphType       );
			ini.setValue("glyphScaleFactor",probe.glyphScaleFactor);
			ini.setValue("glyphColorBy"    ,probe.glyphColorBy    );
			ini.setValue("glyphExtractEigenvalues",probe.glyphExtractEigenvalues);
			ini.setValue("glyphSuperquadricGamma" ,probe.glyphSuperquadricGamma );
		}
		ini.endArray();
	}
#endif

	//emit configWritten();
}


//------------------------------------------------------------------------------
//	loadPCAEigenvectors()
//------------------------------------------------------------------------------
bool SDMVisConfig::loadPCAEigenvectors( QString fullPathFilename, int nrows, int ncols )
{
	using namespace std;
	cout << "Loading PCA eigenvector matrix " << nrows << " x " << ncols << endl;

	if( ncols==-1 )
	{
		// compute number of columns from file size
		size_t fsize = get_filesize( fullPathFilename.toAscii() );
		ncols = (fsize/sizeof(float)) / nrows;
		
		// TODO: sanity checks here
	}

	Matrix M(nrows,ncols);
	if( !rednum::load_matrix<float,Matrix,double>( M, fullPathFilename.toAscii() ) )
	{
		errmsg = QObject::tr("Failed to load eigenvectors from ") + fullPathFilename;
		return false;
	}

	sdm.V = M;
	sdm.n = M.size2();
	return true;
}

//------------------------------------------------------------------------------
//	loadPCAEigenvalues
//------------------------------------------------------------------------------
bool SDMVisConfig::loadPCAEigenvalues( QString fullPathFilename, int n )
{
	Vector lambda(n);
	if( !rednum::load_vector<float,Vector,double,Matrix>( lambda, fullPathFilename.toAscii() ) )
	{
		errmsg = QObject::tr("Failed to load vector from ") + fullPathFilename;
		return false;
	}

	sdm.lambda = lambda;
	return true;
}

//------------------------------------------------------------------------------
//	loadScatterMatrix()
//------------------------------------------------------------------------------
bool SDMVisConfig::loadScatterMatrix( QString fullPathFilename, int n )
{
	Matrix S(n,n);
	if( !rednum::load_matrix<float,Matrix,double>( S, fullPathFilename.toAscii() ) )
	{
		errmsg = QObject::tr("Failed to load scatter matrix from ") + fullPathFilename;
		return false;
	}

	sdm.scatter_t = S;
	sdm.N = S.size1();
	return true;
}
