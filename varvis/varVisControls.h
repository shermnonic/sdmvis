#ifndef VARVISCONTROLS_H
#define VARVISCONTROLS_H

// Own Includes 
#include "VarVisRender.h"
// Qt Includes
#include <QWidget>
#include <QCheckBox>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QSlider>
#include <QComboBox>
#include <QVBoxLayout>
class VarVisControls : public QWidget
{
 Q_OBJECT
public:

	void setIsoValue(double value){m_isoSurfaceValue->setValue(value);}
	void setNumberOfSamplingPoints(int value){m_sampleRange->setValue(value);}
	void setSamplePointSize(double value){m_sampleRadius->setValue(value);}
	void addTraitSelection(QComboBox * box);
	void clearTraitSelection();
	void setPathToMask(QString path){m_pathToMask=path;}
	QString getFullRoiName(){return m_fullRoiName;}
	QString getPathToMask(){return m_pathToMask;}
	QString getRoiName(){return m_roiName;}
    VarVisControls(QWidget *parent = 0);
	bool   get_BackwardsTr_State(){return m_chbx_backwardsTr->isChecked();}
	double getGaussionRadius();
	int    getSampleRange();
	double getSampleRadius();
	double getIsoValue();
	double getGlyphSize();
	bool getUseGaussion(){return m_chbx_gaussion->isChecked();}
	bool getShowSilhouette(){return m_chbx_silhouette->isChecked();}
	bool getShowContour(){return m_chbx_Contour->isChecked();}
	bool getShowSamples(){return m_chbx_samples->isChecked();}
	
	bool getShowReference(){return m_chbx_isoContour->isChecked();}

	bool getTraitsFromSDMVIS(){return m_chbx_getTraits->isChecked();}
 	void setTraitsFromSDMVIS(bool yes){m_chbx_getTraits->setChecked(yes);}

	void enableTraitsFromSDMVIS(bool yes)
	{
		m_selectionBox->setEnabled(yes);
		m_selectionBox->show();
		m_chbx_getTraits->setEnabled(yes);
		m_chbx_getTraits->show();
	}

	double getWarpScaleValue();
	
	void   setVRen(VarVisRender *w){
		m_vren=w;
		m_vren->setSlider(m_frameSlider);
		m_vren->setSelectionBox(m_comb_RoiSelection);
	}
signals :
	void setwarpId(int);
	void getTraitsfromSDMVIS(bool);
private:
	QString  m_fullRoiName;
	QString  m_pathToMask;
	QString  m_roiName;
	QCheckBox * m_chbx_Glyph;
	QCheckBox * m_chbx_Volume;
	QCheckBox * m_chbx_Contour;
	QCheckBox * m_chbx_isoContour;
	QCheckBox * m_chbx_Lut;
	QCheckBox * m_chbx_backwardsTr;
	QCheckBox * m_chbx_samples;
	QCheckBox * m_chbx_silhouette;
	QCheckBox * m_chbx_gaussion;
	QCheckBox * m_chbx_Roi;
	QCheckBox * m_chbx_Cluster;
	QCheckBox * m_chbx_getTraits;
	
	QSpinBox *m_sampleRange;
	QSlider  *m_frameSlider;
	
	QDoubleSpinBox *m_clusterNumber;
	QDoubleSpinBox *m_gaussionRadius;
	QDoubleSpinBox *m_isoSurfaceValue;
	QDoubleSpinBox *m_sampleRadius;
	QCheckBox      *m_glyphAutoScaling;
	QDoubleSpinBox *m_glyphSize;
	QDoubleSpinBox *m_warpScale;

	QPushButton * m_but_StartAnimation;
	QPushButton * m_but_PauseAnimation;
	QPushButton * m_but_StopAnimation;
	QPushButton * m_but_ROI;
	QPushButton * m_but_ClearROI;
	QPushButton * m_but_Cluster;

	QPushButton * m_but_VolumeCluster;
	QPushButton * m_but_UpdateIsoValue;
	QPushButton * m_but_UpdateSampleRange;
	QPushButton * m_but_UpdateGlyphSize;
	QPushButton * m_but_UpdateWarpScale;
	QPushButton * m_but_UpdateGaussion;
	QPushButton * m_but_ScreenShot;
	QPushButton * m_but_SaveRoi;
	QPushButton * m_but_UpdatePointRadius;


	QComboBox * m_selectionBox;
	QComboBox * m_comb_RoiSelection;

	QVBoxLayout *m_warpLayout;
	VarVisRender * m_vren;

	void sceneUpdate();

private slots:
	void do_ClearROI();
	void do_Glyph();
	void do_GlyphUpdate();
	void do_Volume();
	void do_Contour();
	void do_ScreenShot();
    void do_isoContour();
	void do_Lut();
	void do_Samples();
	void do_Silhouette();
	void do_Gaussion();
	void do_IsoUpdate();
	void do_SampleUpdate();
	void do_ScaleUpdate();
	void do_Animation();
	void do_StopAnimation();
	void do_PauseAnimation();
	void do_ROI();
	void do_ShowRoi();
	void selectROI(int index);
	void do_SaveROI();
	void do_updateGaussion();
	void do_Cluster();
	void do_VolumeCluster();
	void show_cluster();
	void do_updatePointRadius();
	void warpSelectionChanged(int);
	void getTraits();
	void setGetTraitsFromSDMVIS(bool);
	void setGlyphAutoScaling(bool);
};

#endif // VARVISCONTROLS_H