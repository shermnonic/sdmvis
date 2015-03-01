#ifndef SDMVISINTERACTIVEEDITINGOPTIONSWIDGET_H
#define SDMVISINTERACTIVEEDITINGOPTIONSWIDGET_H

#include "SDMVisInteractiveEditing.h"
#include <QWidget>

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QCheckBox;

//==============================================================================
//	SDMVisInteractiveEditingOptionsWidget
//==============================================================================
class SDMVisInteractiveEditingOptionsWidget : public QWidget
{
	Q_OBJECT

	friend class SDMVisInteractiveEditing;

public:
	SDMVisInteractiveEditingOptionsWidget( QWidget* parent=0 );

//protected:
	void connectMaster( SDMVisInteractiveEditing* master );

	// Some parameters adjustable with this widget do not directly change the
	// state of the connected SDMVisInteractiveEditing instance but are queried
	// externally. This allows for instance to control rendering specifics via
	// this widget where the options are queried online in the rendering loop.
	bool getShowDebugRubberband() const;
	bool getShowRubberband() const;
	double getRubberFudgeFactor() const;

public slots:
	void setCoeffs( double c0, double c1, double c2, double c3, double c4 );
	void setError( double Etotal, double Esim=-1.0, double Ereg=-1.0 );
	void setDEdit( double x, double y, double z );
	void setPos0( double x, double y, double z );

protected slots:
	void onChangedGamma( double value );
	void onChangedEditScale( double value );
	void onChangedResultScale( double value );
	void onChangedMode( int index );

private:
	SDMVisInteractiveEditing* m_master;

	QDoubleSpinBox* m_gamma;
	QDoubleSpinBox* m_editScale;
	QDoubleSpinBox* m_resultScale;
	QComboBox*      m_mode;

	QCheckBox* m_showDebugRubberband;
	QCheckBox* m_showRubberband;
	QDoubleSpinBox* m_rubberFudgeFactor;

	QLabel* m_coeffs;
	QLabel* m_error;
	QLabel* m_dedit;
	QLabel* m_pos0;
};


#endif // SDMVISINTERACTIVEEDITINGOPTIONSWIDGET_H
