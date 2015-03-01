#include "PlotItem.h"

#include <QtGui>

#include <algorithm> // for min()
#include <iostream>


qreal  PlotItem::s_pointSize  = 3;
int    PlotItem::s_pivotIndex = -1;
QColor PlotItem::s_pivotColor = QColor(255,255,255);
bool   PlotItem::s_showNames=true;

void PlotItem::setPointSize( qreal size )
{
   s_pointSize = (qreal)(std::max( size, 0.0001 ));
   //std::cout << "plotItem: point size set to " << s_pointSize << std::endl;
}


PlotItem::PlotItem( int index, const QColor& color, QString name)
{
   m_index = index;
   m_color = color;
   m_pointSize = s_pointSize;
   m_name = name;
   m_normalSize=QRectF(-0.5*m_pointSize,-0.5*m_pointSize, m_pointSize,m_pointSize);
   m_selectedSize=QRectF(-m_pointSize,-m_pointSize, 2*m_pointSize,2*m_pointSize);
   setFlags( ItemIsSelectable ); // | ItemIgnoresTransformations
   setAcceptsHoverEvents( true );
   setCursor( Qt::PointingHandCursor );
   setRect( QRectF(-0.5*m_pointSize,-0.5*m_pointSize, m_pointSize,m_pointSize) );
   m_isStillSelected=false;
}

void PlotItem::setThisPointSize(int size){
   m_pointSize=size;
   m_normalSize=QRectF(-0.5*m_pointSize,-0.5*m_pointSize, m_pointSize,m_pointSize);
   m_selectedSize=QRectF(-m_pointSize,-m_pointSize, 2*m_pointSize,2*m_pointSize);
   setRect( QRectF(-0.5*m_pointSize,-0.5*m_pointSize, m_pointSize,m_pointSize) );

}
void PlotItem::paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
 Q_UNUSED(widget);

   qreal penWidth = 0; // cosmetic pen by default
   QColor color = m_color;
   QColor fillColor, penColor;

   // set colors according to the selection state of the item
   if( (option->state & QStyle::State_Selected) && index()!=-1 && m_isStillSelected )
   {
      fillColor = Qt::green;
      penColor  = Qt::green;
      m_boundingRect = m_selectedSize;
	
   }
   else
   {
      fillColor = color;
      penColor  = color.darker( 150 );
      m_boundingRect = m_normalSize;

   }

   // draw "pivot element"
   if( index() == s_pivotIndex )
   {
      penColor = s_pivotColor;
      penWidth = s_pointSize * 2.3;
   }

	// set brush
	painter->setBrush( fillColor );

	// set pen
	QPen pen = painter->pen();

	pen.setColor( penColor );
	pen.setBrush(fillColor);
	pen.setWidthF( penWidth );
	painter->setPen( pen );

	painter->drawEllipse( m_boundingRect );

	if(s_showNames)
		painter->drawText(0,-10,m_name);
}

void PlotItem::adjustSize()
{
   if( m_pointSize != s_pointSize )
   {
      prepareGeometryChange();
      m_pointSize = s_pointSize;
      setRect( QRectF(-0.5*m_pointSize,-0.5*m_pointSize, m_pointSize,m_pointSize) );
   }
}


