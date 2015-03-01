#ifndef ROICONTROLS_H
#define ROICONTROLS_H

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
class RoiControls : public QWidget
{
Q_OBJECT

public:
	RoiControls();

	void setVarVisRenderer(VarVisRender *varvisRen)
	{
		m_varVis=varvisRen;
		m_varVis->setSelectionBox(m_selectionBox);
	}
	void initRoiContols();
	void setPathToMask(QString path){m_pathToMask=path;}
	void setExpertMode(bool expert);
	QString getRoiName()	{return m_roiName;}
	QString getFullRoiName(){return m_fullRoiName;}
private:
	VarVisRender *m_varVis;

	QCheckBox * m_chbx_ShowRoi;
	QComboBox * m_selectionBox;
	
	QPushButton * m_but_AddRoi;
	QPushButton * m_but_Clear;
	QPushButton * m_but_SaveMask;
	QPushButton * m_but_CalculateRoi;
	
	QString m_pathToMask;
	QString m_roiName;
	QString m_fullRoiName;

	bool maskCalculated;
	bool m_expertMode;
	void sceneUpdate();
public slots:
	void roiSelectionChanged(int id);
	void do_ClearRoi();
	void do_SaveRoi();
	void do_Calculate();
	void do_AddRoi();
	void do_ShowRoi();
	void do_Screenshot();
signals:
	void statusMessage(QString);
	void calculateROI();
};

#endif // ROICONTROLS_H