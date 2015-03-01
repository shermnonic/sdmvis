#include "BarPlotWidget.h"
#include <QtGui>
#include <limits> // std::numeric_limits

// FIXME [vitalis]: Skalierung mal als 3 angenommen 
// Ueberlegung 3 also 100 %
// ist max value dann auch 100? oO
// muss noch geklaert werden , aber sieht doch schon ganz nett aus 
const double scalingHack = 1.0;

BarPlotWidget::BarPlotWidget( QWidget* parent )
: QFrame(parent),
  m_valuesChangeable(false),
  m_valueDragActive(false),
  m_valueSelActive(false),
  m_onlyPositiveValues(false)
{
	setFrameStyle( QFrame::Panel | QFrame::Plain );
	setLineWidth( 2 );	
}

void BarPlotWidget::unload()
{
	m_valuesChangeable=true;
	m_valueDragActive=true;
	m_valueSelActive=true;	
}

void BarPlotWidget::changeItem(int index,int pcX,int pcY,double valueX, double valueY)
{
	// can we change one or both components ???	

	if (pcX>m_values.size()-1 && pcY>m_values.size()-1){
		
		return;
	}
	else if (pcX<=m_values.size()-1 && pcY>m_values.size()-1)  // only pcX
	{
		m_values[pcX] = valueX*scalingHack;
		update();                           // update myself		
		emit valueChanged( pcX, valueX*scalingHack );  // tell others about it}
	}
	else if (pcX>m_values.size()-1 && pcY<=m_values.size()-1)  // only pcY
	{
		m_values[pcY] = valueY*scalingHack;
		update();                           // update myself		
		emit valueChanged( pcY, valueY*scalingHack );  // tell others about it
	}

	else if (pcX<=m_values.size()-1 && pcY<=m_values.size()-1) // we cann change both components
	{
		m_values[pcX] = valueX*scalingHack;
		m_values[pcY] = valueY*scalingHack;
		update();                           // update myself		
		emit valueChanged( pcX, valueX*scalingHack );  // tell others about it
		emit valueChanged( pcY, valueY*scalingHack );  // tell others about it	
	}
}

void BarPlotWidget::setBarValue(int pc, double value)
{
		m_values[pc] = value;
		update();                           // update myself		
		emit valueChanged( pc, value );  // tell others about it}
}

void BarPlotWidget::changeItem(int pc, double value)
{
	if (pc>m_values.size()-1)
		return;

	else 
	{
		m_values[pc] = value*scalingHack;
		update();                           // update myself		
		emit valueChanged( pc, value*scalingHack );  // tell others about it}
	}

}

void BarPlotWidget::addHLine( int pos, double value, QColor color )
{
	if( m_hlines.empty() ) return;
	Q_ASSERT( (pos>=0) && (pos < m_hlines.size()) );

	BarPlotLine line;
	line.value = value;
	line.color = color;
	m_hlines[pos].push_back( line );
	update();
}

void BarPlotWidget::addHLines( Vector values, QColor color )
{
	Q_ASSERT( m_values.size() == values.size() );
	m_hlines.resize( m_values.size() );

	for( int i=0; i < values.size(); ++i )
	{
		BarPlotLine line;
		line.value = values.at(i);
		line.color = color;
		m_hlines[i].push_back( line );
	}

	update();
}

void BarPlotWidget::setValues( Vector values )
{
	m_values = values;

	m_minVal = std::numeric_limits<double>::max();
	m_maxVal = -m_minVal;
	for( int i=0; i < m_values.size(); ++i )
	{
		m_minVal = std::min<double>( m_values.at(i), m_minVal );
		m_maxVal = std::max<double>( m_values.at(i), m_maxVal );
	}
	m_minVal = std::min( -0.1, m_minVal );
	m_maxVal = std::max(  0.1, m_maxVal );

	m_hlines.resize( m_values.size() );

	update();
}

void BarPlotWidget::setLimits( double min, double max )
{
	m_minVal = min;
	m_maxVal = max;
	update();
}

void BarPlotWidget::setLabels( QStringList labels )
{
	m_labels = labels;
	update();
}

QSize BarPlotWidget::minimumSizeHint() const
{
	return QSize( 100, 100 );
}

QSize BarPlotWidget::sizeHint() const
{
	return QSize( 400, 100 );
}

void BarPlotWidget::paintEvent( QPaintEvent* event )
{
	QFrame::paintEvent( event );

	if( m_values.empty() )
		return;

	QPainter painter( this );
	painter.setBackground( QBrush(Qt::white) );

	// no values, nothing to do
	if( m_values.empty() )
		return;

	// draw bars (and lines)
	painter.save();
	{
		// value range [-dy,+dy]
		double dy = std::max( fabs(m_maxVal), fabs(m_minVal) );

		int h = height()-1;
		if( m_onlyPositiveValues )
		{
			painter.translate( 0, h );
			painter.scale( width(), h / dy );
		}
		else
		{
			painter.translate( 0, h / 2 );
			painter.scale( width(), h / (2*dy) );
		}
		
		// draw bars
		painter.setPen( Qt::darkGreen );
		painter.setBrush( Qt::cyan );
		for( int i=0; i < m_values.size(); ++i )
		{
			double left = i / (double)m_values.size();
			double w = 1.0 / (double)m_values.size();			
			QRectF bar( left, 0.0, w, -m_values.at(i) );  		// flip y
			painter.drawRect( bar );

			// draw lines
			if( m_hlines.size() == m_values.size() )
			{
				for( int j=0; j < m_hlines[i].size(); ++j )
				{					
					double val = -m_hlines[i][j].value; 		// flip y
					painter.setPen( m_hlines[i][j].color );
					painter.drawLine( QPointF( left, val ), QPointF( left+w, val ) );
				}
			}
		}

		// draw zero-crossing
		painter.setPen( Qt::black );
		painter.drawLine( QPointF( 0.0, 0.0 ),  QPointF( m_values.size()-1, 0.0 ) );
	}	
	painter.restore();

	// label style
	double labelHeight = 15;
	int fontSize = 10;

	QFont font = this->font();
	font.setPixelSize( fontSize );
	painter.setFont( font );

	// draw labels
	painter.save();
	{
		int w = width() / m_values.size();

		painter.setPen( Qt::black );
		for( int i=0; i < std::min<int>(m_values.size(),m_labels.size()); ++i )
		{
			int left = i * w;
			QRect labelRect( left, 0, w, labelHeight );
			painter.drawText( labelRect, Qt::AlignCenter, m_labels.at(i) );
		}
	}
	painter.restore();

	// draw value labels
	painter.save();
	{
		int w = width() / m_values.size();

		painter.setPen( Qt::black );
		for( int i=0; i < std::min<int>(m_values.size(),m_labels.size()); ++i )
		{
			QString valueLabel = QString::number( m_values.at(i), 'f', 2 );
			int left = i * w;
			QRect labelRect( left, height()/2, w, labelHeight );
			painter.drawText( labelRect, Qt::AlignCenter, valueLabel );
		}
	}
	painter.restore();
}

void BarPlotWidget::getIndexValue( QPoint pos, int& index, double& value )
{
	int numBars = m_values.size(); // == m_labels.size()
	double barWidth = width() / (double)m_values.size();
	index = (int)( pos.x() / barWidth );

	// clamp index to valid range
	if( index < 0 )  index = 0;
	if( index >= numBars ) index = numBars-1;

	// value range [-dy,+dy]
	double dy = std::max( fabs(m_maxVal), fabs(m_minVal) );
	int y = pos.y();
	int h = (height()-1)/2;
	double normalized_y = -(y - h) / (double)h; // flip y
	value = dy * normalized_y;

	// clamp values to range
	if( value < m_minVal )  value = m_minVal;
	if( value > m_maxVal )  value = m_maxVal;
}

void BarPlotWidget::mousePressEvent( QMouseEvent* event )
{
	if( m_values.empty() )
		return;

	int index;
	double value;
	getIndexValue( event->pos(), index, value );

	bool updateNeeded = false;

	if( event->button() == Qt::LeftButton )
	{
		// start dragging value
		m_valueDragActive = true;
		updateNeeded = true;
	}
	else
	if( event->button() == Qt::RightButton )
	{
		// reset value
		value = 0.0;
		updateNeeded = true;
	}
	else
	if( event->button() == Qt::MidButton )
	{
		// user specific value change
		m_valueSelActive = true;
		emit valueSelected( index, value );
	}

	if( updateNeeded )
	{
		m_values[index] = value;
		update();                           // update myself		
		emit valueChanged( index, value );  // tell others about it
	}
}

void BarPlotWidget::mouseMoveEvent( QMouseEvent* event )
{
	if( !m_valuesChangeable || m_values.empty() )
		return;

	int index;
	double value;
	getIndexValue( event->pos(), index, value );

	if( m_valueDragActive )
	{
		// update myself
		m_values[index] = value;
		update();

		// tell others about it
		emit valueChanged( index, value );
	}

	if( m_valueSelActive )
	{
		// user specific value change
		emit valueSelected( index, value );
	}
}

void BarPlotWidget::mouseReleaseEvent( QMouseEvent* event )
{
	m_valueDragActive = false;
	m_valueSelActive = false;
}
