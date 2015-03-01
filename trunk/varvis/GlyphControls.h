#ifndef GLYPHCONTROLS_H
#define GLYPHCONTROLS_H

#include "VarVisRender.h"
#include "ColorMapRGB.h"

// Qt Includes
#include <QWidget>
#include <QCheckBox>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QSlider>
#include <QComboBox>
#include <QVBoxLayout>
#include <QListWidget>
#include <QGroupBox >
#include <QRadioButton>
#include <QLabel>
#include <QGroupBox>
#include <QStringList>
#include <QList>

class GlyphControls : public QWidget
{
Q_OBJECT

public:
	GlyphControls();
	
	void setExpertMode(bool expert);
	//void setEigenWarpsFiles(QString name);	

	void setEigenwarps( const QStringList& mhdFilenames );

	void setVarVisRenderer(VarVisRender *varvisRen);
	
	void setIsoValue(double value){m_isoSurfaceValue->setValue(value);}
	void setNumberOfSamplingPoints(int value){m_sampleRange->setValue(value);}
	void setSamplePointSize(double value){m_sampleRadius->setValue(value);}

	double getGaussionRadius() const;
	int    getSampleRange   () const;
	double getSampleRadius  () const;
	double getIsoValue      () const;
	double getGlyphSize     () const;
	bool getGlyphAutoScaling() const {return m_chbx_glyphAutoScaling->isChecked();}
	bool getUseGaussion() const {return m_chbx_UseGauss->isChecked();}
	bool getShowContour() const {return m_chbx_Contour->isChecked();}
	bool getShowSamples() const {return m_chbx_samples->isChecked();}
	double getWarpScaleValue() const;
	
	void addTraitSelection(QComboBox * box);
	QComboBox *getEigenWarpSelectionBox(){return m_selectionBoxEigenWarps;}
	bool getTraitsFromSDMVIS(){return m_radio_getTraits->isChecked();}

	void clearEigenwarps();

	//void clearEigenWarpList(){eigenWarpsList.clear();
	//						  m_selectionBoxEigenWarps->clear();}
	void enableTraitsFromSDMVIS(bool yes)
	{
		m_selectionBox->setEnabled(yes);
		m_selectionBox->show();
		m_radio_getTraits->setEnabled(yes);
		m_radio_getTraits->show();
	}

protected:
	void addEigenwarp( QString filename );

private:

	bool setWarp( QString mhdFilename );

	VarVisRender *m_varVis;

	QList<QWidget*> m_advancedOptions;

	QLabel* m_infoLabel;

	QCheckBox* m_chbx_Glyph;
	QCheckBox* m_chbx_Volume;
	QCheckBox* m_chbx_Contour;
	QCheckBox* m_chbx_Lut;
	QCheckBox* m_chbx_samples;
	QCheckBox* m_chbx_Cluster;
	QCheckBox* m_chbx_UseGauss;
	QRadioButton* m_radio_getTraits;
	QRadioButton* m_radio_getEigentWarps;

	QComboBox * m_selectionBox;
	QComboBox * m_selectionBoxEigenWarps;
	QDoubleSpinBox *m_clusterNumber;
	QDoubleSpinBox *m_isoSurfaceValue;
	QDoubleSpinBox *m_sampleRadius;
	QDoubleSpinBox *m_glyphSize;
	QCheckBox* m_chbx_glyphScaleByVector;
	QCheckBox* m_chbx_glyphAutoScaling;
	QDoubleSpinBox *m_warpScale;
	QDoubleSpinBox *m_gausRadius;
	QSpinBox	   *m_sampleRange;

	QPushButton * m_but_UpdateIsoValue;
	QPushButton * m_but_UpdateGlyphSize;
	QPushButton * m_but_UpdateWarpScale;
	QPushButton * m_but_UpdateSampleRange;
	QPushButton * m_but_UpdatePointRadius;
	QPushButton * m_but_UpdateGaussionRadius;
	QPushButton * m_but_ScreenShot;
	QPushButton * m_but_Cluster;
	QPushButton * m_but_ClusterVolume;
	QPushButton * m_but_loadColormap;

	QStringList m_eigenWarpsList;  // List of MHD filenames, sync'd w/ m_selectionBoxEigenWarps

	QLabel *m_scaleLabel;
	QGroupBox *clusterBox;
	QLabel *m_glyphSizeLabel;	
	QLabel *m_sampleRadiusLabel;
	QLabel *m_isoValueLabel;
	QLabel *m_sampleLabel;
	QLabel *m_centroidsLabel;

	bool getTrait_oldState;
	bool getEigenWarpd_oldState;

	ColorMapRGB m_colormap;

public slots:
	void loadEigenWarp(int num);
	void getTraits();	
	void getEigenWarps();
	void clearTraitSelection();
	void warpSelectionChanged(int id);
	void do_Glyph      ( bool b );   // refactor to showGlyph()?
	void do_GlyphUpdate();
	void do_Volume     ( bool b );   // refactor to showVolume()?
	void do_Contour    ( bool b );   // refactor to showContour()?
	void do_Lut        ( bool b );   // refactor to showLUT()?
	void do_ScreenShot();	
	void do_Samples    ( bool b );   // refactor to showSamples()?
	void do_Gaussian   ( bool b );
	void do_IsoUpdate();
	void do_SampleUpdate();
	
	void setTraitsFromSDMVIS(bool yes);
	void do_updateGaussion();
	void do_Cluster();
	void do_VolumeCluster();
	void show_cluster( bool b );
	void do_updatePointRadius();

	void loadColormap();
	void setColormap( const ColorMapRGB& colormap );

	void sceneUpdate();

	void updateInfo();

signals:
	void StatusMessage(QString);
	void setwarpId(int);
	void SIG_getTraitsfromSDMVIS(bool yes);
};

#endif // GlyphControls_h