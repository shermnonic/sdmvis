#ifndef SDMVISCONFIG_H
#define SDMVISCONFIG_H
#include <QObject>
#include <QString>
#include <QList>
#include <QStringList>

#include "Warpfield.h"
#include "Trait.h"
#include "numerics.h"   // Matrix, Vector, rednum::

#ifdef SDMVIS_TENSORVIS_ENABLED
#include "SDMTensorProbe.h"
#endif

const QString APP_NAME        ( "sdmvis" );
const QString APP_ORGANIZATION( "University Bonn Computer Graphics" );
#define       APP_ICON        QIcon(QPixmap(":/data/icons/icon.png"))
const int     APP_VERSION     ( 0400 );

//------------------------------------------------------------------------------
/// The main data structure for the statistical deformation model (SDM)
//------------------------------------------------------------------------------
struct SDM
{
	QString identifier; ///< identifier should be usable for path pre-/suffixes
	QString description;///< more detailed description	
	QString basePath;

	QString sdmprocIni; ///< Associated ini file from sdmproc
	
	///@{ Warp-Dataset
	QList<Warpfield> eigenwarps; ///< eigenwarps (meanfree)
	Warpfield        meanwarp;   ///< mean warp (optional)
	
	QString          reference;  ///< filename of mean-estimate (.mhd)
	QString          warpsMatrix;///< matrix of all warpfields
	QString          eigenwarpsMatrix;///< matrix of all eigenwarps
	QStringList      names;      ///< dataset names
#ifdef SDMVIS_VARVIS_ENABLED
	QString			m_meshFilename;  
	QString			m_pointsFileName;
	int				m_numberOfSamplePoints;
	double			m_isoValue;
	double			m_samplePointSize;
#endif
	///@}
	
	///@{ V-Space
	int N, n; ///< matrix sizes: scatter_t is N x N,  V is N x n,  lambda is n x 1,  n <= N
	Matrix scatter_t; ///< warps smaller scatter matrix
	Matrix V;         ///< eigenvectors of the smaller scatter matrix
	Vector lambda;    ///< eigenvalues of the smaller scatter matrix
	///@}
};

//------------------------------------------------------------------------------
/// Spherical region of interest
//------------------------------------------------------------------------------
struct ROI
{
	double radius;
	double center[3];
};

//------------------------------------------------------------------------------
/// SDMVis configuration
//------------------------------------------------------------------------------
 class SDMVisConfig //: public QObject
{
	// ... emit signals when config changed ?
	// ... allow config changes via slots ?

//Q_OBJECT
//signals:
//  void configWritten();
/*
void namesChanged( QStringList names );
	void scaleChanged( int idx, double value );
	void warpsMatrixChanged( QString warpsMatrix );
	void warpsChanged( QList<Warpfields> warps );
public slots:
	void setNames( QStringList names );
	void setScale( int idx, double value );
	void setWarpsMatrix( QString warpsMatrix );
	void setWarps( QList<Warpfields> warps );
*/

public:
	static int appVersion;
	int version;

	SDMVisConfig():version(appVersion) {}
#ifdef SDMVIS_VARVIS_ENABLED
	void setSamplePointSize(double value){sdm.m_samplePointSize=value;}
	void setMeshFilename(QString name){sdm.m_meshFilename=name;}
	void setPointsFilename(QString name){sdm.m_pointsFileName=name;}
	void setNumberOfSamplePoints(int value){sdm.m_numberOfSamplePoints=value;}
	void setIsoValue(double value){sdm.m_isoValue=value;}
	QString getMeshFilename(){return sdm.m_meshFilename;}
	QString getPointsFilename(){return sdm.m_pointsFileName;}
	int getNumberOfSamplePoints(){return sdm.m_numberOfSamplePoints;}
	double getIsoValue(){return sdm.m_isoValue;}
	double getSamplePointSize(){return sdm.m_samplePointSize;}
#endif

	QString getAssociatedSDMProcIni() { return sdm.sdmprocIni; }

	bool readConfig ( QString iniFilename );
	void writeConfig( QString iniFilename );
	void clearWarpFieldList() { sdm.eigenwarps.clear(); }
	void clearRoi() { m_roiVector.clear(); }
	bool sanityCheck();
	QString getErrMsg() const { return errmsg; }

	void setBasePath( QString basePath ) 
	{ 
		updateRelativePaths( basePath );
		sdm.basePath = basePath;
	}

	QString getConfigName() const { QString name = m_configFilename; name.replace(".ini",""); return name; }

	QVector<ROI> getRoiVector(){ return m_roiVector; }
	void addRoi(ROI roi)	{ m_roiVector.push_back(roi); }
	void setRoiVector( QVector<ROI> rv ) { m_roiVector = rv; }
	
	void    setIdentifier( QString ident ) { sdm.identifier = ident; }
	QString getIdentifier() const          { return sdm.identifier; }
	
	void    setDescription(QString desc) { sdm.description = desc; }
	QString getDescription() const       { return sdm.description; }
	
	void clearTraits(){traits.clear();}

	void setMeanwarp( QString filename )
	{
		QString relPath = filename;
		relPath.remove(sdm.basePath+"/");

		Warpfield tmp;
		tmp.setElementScale( 1.0 );
		tmp.setMHDfilename( relPath );
		sdm.meanwarp = tmp;
	}

	void setWarpfieldFileList(QStringList list)
	{		
		for (int iA=0;iA<list.size();iA++)
		{
			QString relPath=list.at(iA);
			relPath.remove(sdm.basePath+"/");			

			int numSamples = std::min( getNames().size(), 1 );
			float scale = getWarpfieldAutoScaling( numSamples );

			Warpfield tmp;
			tmp.setElementScale(scale);
			tmp.setMHDfilename(relPath);
			sdm.eigenwarps.push_back(tmp);			
		}	
	}

	float getWarpfieldAutoScaling( int numSamples )
	{
		// Default scaling is 1 / sqrt(numSamples)
		return (float)(1.0 / sqrt((double)numSamples));
	}

	// Provided only for convenience, some dialogs require a set of scaling values
	QList<float> getWarpfieldAutoScalings()
	{
		int numSamples = std::max( getNames().size(), 1 );
		
		// We have only a global scaling factor now
		float scale = getWarpfieldAutoScaling( numSamples );

		QList<float> scalings;
		for( int i=0; i < sdm.eigenwarps.size(); i++ )
			scalings.push_back( scale );
		return scalings;
	}

	QString getBasePath() const { return sdm.basePath; }

	QString          getReference()  const { return getAbsolutePath(sdm.reference); }
	/// Same as getEigenwarps(), provided for convenience
	QList<Warpfield> getWarpfields() const { return getEigenwarps(); }
	QList<Warpfield> getFirstTraits() const 
	{ // do some dirty cast
		QList<Warpfield> tempList;
			Warpfield temp;
			temp.mhdFilename=getAbsolutePath(traits.at(0).mhdFilename);
			temp.elementScale=traits.at(0).elementScale;
			tempList.push_back(temp);
			return tempList;
	}

	QList<Warpfield> getSpecificTraits(int index) const 
	{ // do some dirty cast
		QList<Warpfield> tempList;
			Warpfield temp;
			temp.mhdFilename=getAbsolutePath(traits.at(index).mhdFilename);
			temp.elementScale=traits.at(index).elementScale;
			tempList.push_back(temp);
			return tempList;
	}

	QList<Trait>& getTraits()  {return traits;} 
	void setTraitList(QList<Trait> newTraitList){traits=newTraitList;}

	Warpfield getMeanwarp() const
	{
		Warpfield tmp = sdm.meanwarp;
		if( sdm.meanwarp.mhdFilename.isEmpty() )
			return tmp;
		tmp.mhdFilename = getAbsolutePath( sdm.meanwarp.mhdFilename );
		return tmp;
	}

	/// Return copy of eigenwarps with absolute file paths
	QList<Warpfield> getEigenwarps() const 
		{
			QList<Warpfield> tmp = sdm.eigenwarps;
			for( int i=0; i < tmp.size(); ++i )
				tmp[i].mhdFilename = getAbsolutePath(tmp[i].mhdFilename);
			return tmp; 
		}
	QString          getWarpsMatrix()      const { return getAbsolutePath(sdm.warpsMatrix); }
	QString          getEigenwarpsMatrix() const { return getAbsolutePath(sdm.eigenwarpsMatrix); }
	QStringList      getNames()            const { return sdm.names; }

	Matrix * getPlotterMatrix()
		{	
			// Standardize score matrix by scaling with sqrt(numSamples)
			static Matrix scores;
			scores = sdm.V * getPlotScaling();
			return &scores;
		}

	//Trait getPlotterTrait( int idx )
	//	{
	//		// Scale trait with same factor as V matrix, sqrt(numSamples)
	//		static Trait trait;
	//		trait = getTraits().at(idx);
	//		trait.distance *= getPlotScaling();
	//		return trait;
	//	}

	double getPlotScaling() const 
		{
			// The variable sdm.N seems not be set correctly, so we get the 
			// number of samples from the length of an eigenvector.
			int numSamples = sdm.V.size1();
			return sqrt( (double)numSamples );
		}

	QString getEigenVectorMatrix(){return sdm.eigenwarpsMatrix;}
	void setNames           ( QStringList names )   { sdm.names            = names; }
	void setWarpsMatrix     ( QString warpsMatrix ) { sdm.warpsMatrix      = getRelativePath(warpsMatrix); }
	void setEigenwarpsMatrix( QString warpsMatrix ) { sdm.eigenwarpsMatrix = getRelativePath(warpsMatrix); }
	void setReference       ( QString reference )   { sdm.reference        = getRelativePath(reference  ); }
	/// Same as setEigenwarps(), provided for convenience
	void setWarpfields      ( QList<Warpfield> warpfields ) { setEigenwarps(warpfields); }
	/// Set eigenwarps (converts absolute to relative paths)
	bool isTraitListEmpty()
	{
		return traits.isEmpty();
	}
	void setEigenwarps( QList<Warpfield> eigenwarps )
		{
			for( int i=0; i < eigenwarps.size(); ++i )
				eigenwarps[i].mhdFilename = eigenwarps[i].mhdFilename;
			sdm.eigenwarps = eigenwarps;
		}

	/// Same as eigenwarps(), provided for convenience
	QList<Warpfield>& warpfields() { return eigenwarps(); }
	/// Reference to eigenwarps (note that internally relative file paths are used!)
	QList<Warpfield>& eigenwarps() { return sdm.eigenwarps; }

	bool loadPCAEigenvectors( QString fullPathFilename, int nrow, int ncols=-1 );
	bool loadPCAEigenvalues ( QString fullPathFilename, int n );
	bool loadScatterMatrix  ( QString fullPathFilename, int n );

	Matrix getScatterMatrix  () const { return sdm.scatter_t; }
	Matrix getPCAEigenvectors() const { return sdm.V;         }
	Vector getPCAEigenvalues () const { return sdm.lambda;    }

	void setScatterMatrix  ( const Matrix& scatter ) { sdm.scatter_t = scatter; }
	void setPCAEigenvectors( const Matrix& V )       { sdm.V         = V; }
	void setPCAEigenvalues ( const Vector& lambda )  { sdm.lambda    = lambda; }

	/// Returns path relative to basePath (or explicitly specified path)
	QString getRelativePath( QString path, QString relativeTo="" ) const;
	QString getAbsolutePath( QString filename ) const;

	QStringList getLookmarks() const { return m_lookmarks; }

	///@{
	void setRaycasterIsovalue   ( float v ) { m_raycasterIsovalue    = v; }
	void setRaycasterIsoStepsize( float v ) { m_raycasterIsoStepsize = v; }
	void setRaycasterDVRStepsize( float v ) { m_raycasterDVRStepsize = v; }
	void setRaycasterDVRAlpha   ( float v ) { m_raycasterDVRAlpha    = v; }

	float getRaycasterIsovalue   () const { return m_raycasterIsovalue; }
	float getRaycasterIsoStepsize() const { return m_raycasterIsoStepsize; }
	float getRaycasterDVRStepsize() const { return m_raycasterDVRStepsize; }
	float getRaycasterDVRAlpha   () const { return m_raycasterDVRAlpha; }
	///@}

protected:	
	void updateRelativePaths( QString newBasePath, QString oldBasePath="" );

private:
	QString      m_configFilename;

	SDM          sdm;
	QList<Trait> traits;  // optional (check if empty)

	//ROI          roi;     // optional (check roi.active)
	
	QVector<ROI> m_roiVector;

	// ... how to handle analysis derived from ROI, subset or trait projection ?
	
	QString errmsg;

	QStringList m_lookmarks; // optional

#ifdef SDMVIS_TENSORVIS_ENABLED
public:
	QList<SDMTensorProbe> getTensorProbes() { return m_tensorProbes; }
	void setTensorProbes( QList<SDMTensorProbe> list )
	{
		m_tensorProbes = list;
	}
private:
	QList<SDMTensorProbe> m_tensorProbes;
#endif

	float m_raycasterIsovalue;
	float m_raycasterIsoStepsize;
	float m_raycasterDVRStepsize;
	float m_raycasterDVRAlpha;
};

#endif // SDMVISCONFIG_H
