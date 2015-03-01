#ifndef QTENSORVISOPTIONSWIDGET_H
#define QTENSORVISOPTIONSWIDGET_H

#include <QWidget>

// Forwards
class TensorVisBase;
class TensorVisRenderBase;
class QDoubleSpinBox;
class QSpinBox;
class QComboBox;
class QCheckBox;
class QGroupBox;
class QButtonGroup;
class QSlider;

/// Options widget to control a \a TensorVis instance
class QTensorVisOptionsWidget : public QWidget
{
	Q_OBJECT	

signals:
	void visChanged();
	void statusMessage( QString );

public:
	QTensorVisOptionsWidget( QWidget* parent=0 );

	/// Set \a TensorVis instance to control (pointer must stay valid!)
	void setTensorVis( TensorVisRenderBase* master );

public slots:
	void setModeAnimation( int mode, double val );

protected:
	void createWidgets();

	TensorVisBase*       getVisBase();
	TensorVisRenderBase* getVisRenderBase();

protected slots:
	void resample();

	// On nearly all UI changes updateVis() is invoked which updates the 
	// visualization according to the current UI settings.
	void updateVis();

	// Slicing
	void setSlicing( int enable );
	void setSliceDir( int dir );
	void setSlice( int slice );

	// Tensor distribution
	void computeSpectrum();
	void updateSpectrumModes();

private:
	TensorVisRenderBase* m_master;

	QComboBox*      m_comboStrategy;
	QDoubleSpinBox* m_sbThreshold;
	QSpinBox*       m_sbSampleSize;
	QSpinBox*       m_sbGridStepSize;
	QDoubleSpinBox* m_sbGlyphScaleFactor;
	QCheckBox*      m_cbExtractEigenvalues;
	QCheckBox*      m_cbSqrtScaling;
	QCheckBox*      m_cbColorGlyphs;
	QDoubleSpinBox* m_sbSuperquadricGamma;
	QComboBox*      m_comboGlyphType;
	QCheckBox*      m_cbShowVectorfield;
	QCheckBox*      m_cbShowTensorGlyphs;
	QCheckBox*      m_cbShowLegend;
	QComboBox*      m_comboColorBy;
	QCheckBox*      m_slicingEnabled;
	QButtonGroup*   m_sliceDir;
	QSlider*        m_sliceSlider;
	QGroupBox*      m_sliceGroup;
	QGroupBox*      m_distGroup;
	QSpinBox*       m_sbSpectMode;
	QDoubleSpinBox* m_sbSpectModeScale;
	QCheckBox*      m_cbSpectModeApply;
};

#endif // QTENSORVISOPTIONSWIDGET_H
