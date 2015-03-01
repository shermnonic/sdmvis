#ifndef VOLUMECONTROLS_H
#define VOLUMECONTROLS_H

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
#include <QListWidget>
#include <QGroupBox >
#include <QRadioButton>
class VolumeControls : public QWidget
{
Q_OBJECT

public:
	VolumeControls();
	void setVarVisRenderer(VarVisRender *varvisRen)
	{
		m_varVis=varvisRen;
	}

	void setExpertMode(bool expert);
private:

	int sizeOfItems;
	int sizeOfFinishedProcesses;
	QString m_pathToVolumeData;
	QStringList m_subsetList;
	QCheckBox * m_chbx_useDiffVol;
	QRadioButton* m_radio_showVolumen;
	QRadioButton * m_radio_showMeshes;
	QCheckBox * m_chbx_showBar;

	QRadioButton *m_radio_showAllDifferenceErrors;
	QRadioButton *m_radio_showNoneDifferenceErrors;
	
	QGroupBox *m_advancedBox;

	QCheckBox * m_show_original	;
	QCheckBox * m_show_registered;
	QCheckBox * m_show_difference;

	QCheckBox * m_chbx_AdvancedMesh0;
	QCheckBox * m_chbx_AdvancedMesh1;
	QCheckBox * m_chbx_AdvancedMesh2;
	QCheckBox * m_chbx_AdvancedMesh3;
	QCheckBox * m_chbx_AdvancedMesh4;
	QCheckBox * m_chbx_AdvancedMesh5;
	QCheckBox * m_chbx_AdvancedMesh6;
	QCheckBox * m_chbx_AdvancedMesh7;
	QCheckBox * m_chbx_AdvancedMesh8;
	QCheckBox * m_chbx_AdvancedMesh9;


	QDoubleSpinBox *m_isoValueMeshes;
	QDoubleSpinBox *m_isoValueErrorMesh;
		
	
	QDoubleSpinBox *m_opacity_Original;
	QDoubleSpinBox *m_opacity_Registered;
	QDoubleSpinBox *m_opacity_Difference;
	

	QDoubleSpinBox *m_originalOpacity;
	QDoubleSpinBox *m_registeredOpacity;
	QDoubleSpinBox *m_differenceOpacity;

	QListWidget *m_itemList;
    VarVisRender *m_varVis;
	bool m_useDifferenceVol;
	bool m_DiffVolCalculated;


	QString original;
	QString registed;
	QString difference;
	QPushButton * m_generateAdvancedDifference;
	void loadPath(QString path);

public slots:
	void loadItem(QListWidgetItem *item);
	void setUsedifferenceVol();
	void setPath();
	void setShowVolume();
	void setShowMeshes();
	void proc_finished(int exitValue);
	void setIsoValues();
	void showErrorMesh();
	void showOriginalMesh();
	void showDifferenceMesh();
	void showRegisteredMesh();
	void showBar();
	void generateAdvancedDifference();
	void calculateAllDifferenceVolumes();
	void multiProcess_Finished(int exitValue);
	void showAllError();
	void showNoneError();
	void setCheckBoxes(bool original,bool registered,bool difference);
signals:
	void StatusMessage(QString);
};

#endif // VARVISCONTROLS_H