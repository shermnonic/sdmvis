#ifndef BARPLOTWIDGET_H
#define BARPLOTWIDGET_H

#include <QFrame>
#include <QVector>
#include <QStringList>

/// Widget displaying a simplistic interactive bar plot
class BarPlotWidget : public QFrame
{
	Q_OBJECT

signals:
	void valueChanged( int index, double newValue );
	void valueSelected( int index, double selValue );

public:
	typedef QVector<double> Vector;

	BarPlotWidget( QWidget* parent=0 );
	void unload();

	void setValuesChangeable( bool changeable ) { m_valuesChangeable = true; }

	void setLimits( double min, double max );
	
	/// Workaround to display histogram data
	void setOnlyPositiveValues( bool b ) { m_onlyPositiveValues = b; }

	void setValues( Vector v );
	void setLabels( QStringList labels );

	Vector getValues() const { return m_values; }

	void addHLine( int pos, double value, QColor color=QColor(255,0,0) );
	void addHLines( Vector values, QColor color=QColor(255,0,0) );
	void clearHLines() { m_hlines.clear(); }

	QSize minimumSizeHint() const;
	QSize sizeHint() const;

protected slots:
	void paintEvent( QPaintEvent* /*event*/ );
    void changeItem(int index,int pcX,int pcY,double valueX,double valueY);
	void changeItem(int pc, double value);
	void setBarValue(int pc, double value);

protected:
	void mousePressEvent  ( QMouseEvent* event );
	void mouseReleaseEvent( QMouseEvent* event );
	void mouseMoveEvent   ( QMouseEvent* event );

	void getIndexValue( QPoint pos, int& index, double& value );

private:
	Vector m_values;
	double m_minVal;
	double m_maxVal;

	QStringList m_labels;

	bool m_valuesChangeable;
	bool m_valueDragActive;
	bool m_valueSelActive;
	bool m_onlyPositiveValues;

	struct BarPlotLine { double value; QColor color; };
	QVector< QVector<BarPlotLine> > m_hlines;
};


#endif // BARPLOTWIDGET_H
