#include "PlotView.h"
#include "PlotItem.h"
#include <QtGui>

#include <cmath>
#include <iostream>

bool PlotView::s_bool_drawTraits=false;
PlotView::PlotView(QWidget *parent) :QGraphicsView(parent)
{
   setRenderHint( QPainter::Antialiasing, true );
   setViewportUpdateMode( QGraphicsView::SmartViewportUpdate ); // FullViewportUpdate SmartViewportUpdate
   setDragMode( QGraphicsView::NoDrag ); // NoDrag RubberBandDrag ScrollHandDrag
   setResizeAnchor( QGraphicsView::AnchorViewCenter );
   setMouseTracking( false ); // if set to false, mouseMoveEvents only occur while a button is pressed
   setSelectionMode( SELECT_SINGLE );
   m_tickSize=5;
   m_rsBanding = false;
   m_userModeActive = false;
   m_bool_drawAxis=true;
   m_bool_drawTags=true;
   m_bool_drawSigma=false;
   m_axisColor=Qt::gray;
   m_axisSize=1;
   m_offset=5;
   m_sigma=1.0;
   m_traitScaling=1.0;
   
   connect(this,SIGNAL(deselect()),this,SLOT(deselectItems()));
}

void PlotView::setOffset(int offset)
{
	m_offset=offset;
}
void PlotView::setAxisColor(QColor color)
{
	m_axisColor=color;
}
void PlotView::setAxisSize(int size)
{
	m_axisSize=size;
}


void PlotView::drawTags(bool yes){
	m_bool_drawTags=yes;
}

void PlotView::drawTraits(bool yes){
	s_bool_drawTraits=yes;
}
void PlotView::setTickSize(int size){
	m_tickSize=size;

}

void PlotView::setSelectionMode( int mode )
{
   m_selectionMode = mode;

   // HACK: For ActionMode SELECT_RUBBERBAND we use default rubberband
   //       implementation. See also mouseReleaseEvent().
   if( m_selectionMode == SELECT_RUBBERBAND )
      setDragMode( QGraphicsView::RubberBandDrag );
   else
      setDragMode( QGraphicsView::NoDrag );

   // HACK change mouse cursor for specific modes
   // TODO: DOESN'T WORK RELIABLY!
   if( m_selectionMode >= SELECT_USER ){
   
	   setCursor(Qt::CrossCursor);
   }

   else{
      unsetCursor();
	//  setCursor(Qt::ArrowCursor);
	  this->update();
	  viewport()->update();
	 
   }// use parent's cursor (was: setCursor(Qt::ArrowCursor))
}

void PlotView::mousePressEvent( QMouseEvent *event )
{
   viewport()->update();
   if( m_selectionMode == SELECT_NONE ) return;

   // when radial selection is in progress...
   if( m_rsBanding )
   {
      // ...the user can abort via right mouse button
      if( event->button() == Qt::RightButton )
      {
        #ifdef DEBUG_PLOT
         std::cout << "DEBUG: plotView::mousePressEvent(): Radial selection aborted" << std::endl;
        #endif

         m_rsBanding = false;
         resetPivotRadius();
         viewport()->update( /*m_rsBandRect*/ );
      }

      // and nothing to do here anymore.
      return;
   }

   // only left button actions (note that default QGraphicsView::mousePressEvent isn't called either!)
   if( !(event->button()==Qt::LeftButton) ) return;

   // detemine selected event
   PlotItem* m_plotItem=0;
   emit setItemLabel("");
  
   if( m_selectionMode >= SELECT_USER )
   {
      QPointF pt = this->mapToScene( event->pos() );
     #ifdef DEBUG_PLOT
      std::cout << "DEBUG: plotView::mousePressEvent(): coordinate " << pt.x() << ", " << pt.y() << std::endl;
     #endif
      m_userModeActive = true;
      emit userModeSelection( pt.x(), pt.y() );
   }
 if( QGraphicsItem *item = itemAt(event->pos()) )
   {
      m_plotItem = dynamic_cast<PlotItem*>( item );
	  m_plotItem->setStillSelected(true);
     #ifdef DEBUG_PLOT
      std::cout << "DEBUG: plotView::mousePressEvent(): Selected item " << m_plotItem->index() << std::endl;
     #endif
	  if( m_plotItem->index()==-1)
	  {
		viewport()->update();// moving item is not selectAble
		return;
	  }
	  else
	  {
		 emit itemSelected(m_plotItem->index());
	
	  }
		
		if( m_selectionMode >= SELECT_USER )
			emit userModeSelection( m_plotItem->index() );
		else
			emit selectionSingle( m_plotItem->index() );

      viewport()->update();
   }
   else {
	emit deselect();
   }// for user modes also emit signal with coordinates when no item hit
   // start radial selection
   if( (m_selectionMode==SELECT_RADIAL) )
   {
      resetPivotRadius();

      if( m_plotItem )
      {
        #ifdef DEBUG_PLOT
         std::cout << "DEBUG: plotView::mousePressEvent(): New radial selection started" << std::endl;
        #endif

         m_pivotIndex = m_plotItem->index();
         PlotItem::setPivotIndex( m_pivotIndex );

         m_rsCenter = m_plotItem->pos();
         m_rsBanding = true;

         QPoint p = mapFromScene( m_rsCenter );
         m_rsBandRect = QRect( p, p );
      }
   }
   else
      // default drag mode implementation
      QGraphicsView::mousePressEvent( event );
}
void PlotView::deselectItems()
{
	for (int iA=0;iA<m_itemList.size();iA++)
		m_itemList.at(iA)->setStillSelected(false);
		

}
void PlotView::mouseMoveEvent( QMouseEvent *event )
{
   if( m_selectionMode == SELECT_NONE ) return;

   if( m_selectionMode >= SELECT_USER && m_userModeActive )
   {
      QPointF pt = this->mapToScene( event->pos() );
    // #ifdef DEBUG_PLOT
      //std::cout << "DEBUG: plotView::mouseMoveEvent(): coordinate " << pt.x() << ", " << pt.y() << std::endl;
    // #endif
      emit userModeSelection( pt.x(), pt.y() );
	  emit deselect();
   }

   if( m_rsBanding )
   {
      // update banding rect (in viewport coordinates)
      if( !m_rsBandRect.isNull() )
         viewport()->update( m_rsBandRect );

      // calculate ellipse rect (in scene coordinates)
      QPointF r = m_rsCenter - mapToScene( event->pos() );
      m_rsRadius = std::sqrt( r.x()*r.x() + r.y()*r.y() );
      m_rsEllipseRect = QRectF( m_rsCenter, m_rsCenter );
      m_rsEllipseRect.adjust( -m_rsRadius, -m_rsRadius, m_rsRadius, m_rsRadius );

      // calculate new banding rect
      QRectF adjRect = m_rsEllipseRect.adjusted(
                           -PlotItem::pointSize()*.5, -PlotItem::pointSize()*.5,
                            PlotItem::pointSize()*.5,  PlotItem::pointSize()*.5  );
      m_rsBandRect = mapFromScene(adjRect).boundingRect();

      // update new banding rect
      viewport()->update( m_rsBandRect );

     #if 1
      ///////////////////////////////////////////////////////////////////////
      // REMARK: Selection update on move can hit performance. For many
      //         graphics items it is faster to update selection not here
      //         but in mouseReleaseEvent.
      ///////////////////////////////////////////////////////////////////////

      // update selection area
      QPainterPath selectionArea;
      selectionArea.addEllipse( m_rsEllipseRect );
      if( scene() )
         scene()->setSelectionArea( selectionArea );
     #endif
   }
   else
      QGraphicsView::mouseMoveEvent( event );
}

void PlotView::mouseReleaseEvent( QMouseEvent *event )
{
   if( m_userModeActive )
      m_userModeActive = false;

   if( m_selectionMode == SELECT_NONE ) return;

   if( m_rsBanding )
   {
      // don't draw ellipse in drawForeground() anymore
      m_rsBanding = false;

      // update banding rect (in viewport coordinates)
      viewport()->update( /*m_rsBandRect*/ );

      // reset banding
      double radius = m_rsRadius;

      // check minimum radius
      if( radius > PlotItem::pointSize() )
      {
         emit selectionRadial( m_pivotIndex, radius );
      }
   }
   else
   {
      // default drag mode implementation
      QGraphicsView::mouseReleaseEvent( event );

      // HACK: For ActionMode SELECT_RUBBERBAND we use default rubberband
      //       implementation. See also setSelectionMode().
      if( (m_selectionMode == SELECT_RUBBERBAND) && scene() )
      {
         // do nothing if single item was selected
         if( scene()->selectedItems().size() > 1 )
         {
            emit selectionRubberband( scene()->selectionArea().boundingRect() );
         }
      }
   }
}

void PlotView::drawForeground ( QPainter * painter, const QRectF & /*rect*/ )
{
	// draw selection rubberband
	if( m_rsBanding && !m_rsEllipseRect.isNull() )
	{
		QBrush brush = painter->brush();
		QPen   pen   = painter->pen();

		painter->setPen( QPen(Qt::blue, 0, Qt::DashDotLine) );
		painter->setBrush( Qt::NoBrush ); //QColor(0,0,255, 63) );
		painter->drawEllipse( m_rsEllipseRect );

		painter->setPen( pen );
		painter->setBrush( brush );
	}
}

void PlotView::drawBackground(  QPainter * painter, const QRectF & /*rect*/ )
{
	int height  =frameSize().height()-m_offset;
    int width   =frameSize().width() -m_offset;
	int distance;
    int numberOfItems=qMax(abs(minus_rangeX)+plus_rangeX,abs(minus_rangeY)+plus_rangeY);
	  
    // bestimme wiviele werte benoetigt werden auf minaler achse
	if(width<height)
		distance=width/numberOfItems;
	else
		distance=height/numberOfItems;

	if (m_bool_drawSigma)
	{
		// m_sigma times ten for double arithmetik 
	    QPen pen;
	    pen.setWidth( m_axisSize-1 );
		pen.setColor( m_axisColor );
		painter->setPen(pen);		

		// Draw ellipses (from larger to smaller ones for correct z-order)
		for( int i=1; i >= 0; i-- )
		{
			// Set fill color
			int gray = i*20;
			painter->setBrush( QColor(200+gray,200+gray,200+gray) );

			// Draw ellipse
			double radius = (i+1) * m_sigma * 10.0 * distance;
			int x0 = -radius,
				y0 = x0;
			painter->drawEllipse( x0,y0, 2*radius,2*radius );
		}
   }

   if (m_bool_drawAxis)
   {
		QBrush brush;
		QPen   pen;

		brush.setColor(m_axisColor);
		brush.setStyle(Qt::SolidPattern);
		pen.setStyle(Qt::SolidLine);
		pen.setWidth(m_axisSize);
		pen.setColor(m_axisColor);
		pen.setBrush(brush);
	
		painter->setPen( pen);
		painter->setBrush( brush); //QColor(0,0,255, 63) );

		setSceneRect(minus_rangeX*distance,-plus_rangeY*distance,(abs(minus_rangeX)+plus_rangeX)*distance,(abs(minus_rangeY)+plus_rangeY)*distance);
		//x-component
		painter->drawLine(minus_rangeX*distance,0,plus_rangeX*distance,0);
		//y-component
		painter->drawLine(0,-minus_rangeY*distance,0,-plus_rangeY*distance);

		// x-Achse
		const int stepsize = 5;
		for (int iX=minus_rangeX;iX<plus_rangeX+1;iX++)
		{
			if( iX % stepsize == 0 )
			{
				painter->drawLine(iX*distance,-m_tickSize,iX*distance,m_tickSize);
				if(m_bool_drawTags)
					painter->drawText(iX*distance-5,10,50,20,0,QString::number((float)iX/10));
			}
		}
		if(m_bool_drawTags)
			painter->drawText((plus_rangeX)*distance-10,-20,(plus_rangeX+1)*distance,20,0,m_lableX);

		// y-Achse
		for (int iY=-plus_rangeY;iY<abs(minus_rangeY)+1;iY++)
		{
			if( iY % stepsize == 0 )
			{
				painter->drawLine(-m_tickSize,iY*distance,m_tickSize,iY*distance);
				if(m_bool_drawTags)
					painter->drawText(10,iY*distance-7,50,20,0,QString::number((float)(-1)*iY/10));
			}
		}
		if(m_bool_drawTags)
			painter->drawText(-m_offset,-plus_rangeY *distance-5,50,20,0,m_lableY);
   }
   else
   	  setSceneRect(minus_rangeX*distance,-plus_rangeY*distance,(abs(minus_rangeX)+plus_rangeX)*distance,(abs(minus_rangeY)+plus_rangeY)*distance);

   if (s_bool_drawTraits)
   {
	   QPen pen;
	   pen.setWidth(m_axisSize);
	   pen.setColor(QColor(0,255,0));
	   painter->setPen(pen);
  	   painter->drawLine(
		   m_traitScaling*m_traitStartX*distance,
		   m_traitScaling*m_traitStartY*distance,
		   m_traitScaling*m_traitEndX*distance,
		   m_traitScaling*m_traitEndY*distance );
	   pen.setWidth(m_axisSize+3);
	   painter->setPen(pen);
	//   painter->drawLine(distance*m_traitStartX-m_traitOffsetMinus,distance*m_traitStartY,distance*m_traitEndX-m_traitOffsetMinus,distance*m_traitEndY);
	//   painter->drawLine(distance*m_traitStartX+m_traitOffsetPlus,distance*m_traitStartY,distance*m_traitEndX+m_traitOffsetPlus,distance*m_traitEndY);
   }
}

void PlotView::setTraitScaling( double s )
{
	m_traitScaling = s;
}

void PlotView::paintTraits(QPointF start,QPointF end,int traitOffsetPlus,int traitOffsetMinus)
{
	m_traitStartX = start.x();
	m_traitStartY =-start.y();
	m_traitEndX   =   end.x();
	m_traitEndY   =  -end.y();
	m_traitOffsetMinus = traitOffsetMinus;
	m_traitOffsetPlus  = traitOffsetPlus;
};

void PlotView::drawSigmaCircle(bool yes){
	m_bool_drawSigma=yes;
}
void PlotView::drawSigmaCircle(bool yes, double sigma){
	m_bool_drawSigma=yes;
	m_sigma=sigma;
}

void PlotView::resetPivotRadius()
{
   // make invalid
   m_rsEllipseRect.setWidth(0);
   m_rsEllipseRect.setHeight(0);
   m_rsRadius = 0.0;

   // clear current selection
   if( scene() )
      scene()->clearSelection();
}

int PlotView::getNumberOfTicks() const
{
	return qMax(
		 abs(minus_rangeX)+plus_rangeX,
		 abs(minus_rangeY)+plus_rangeY); 
}

int PlotView::getTickDistance() const
{
	int height = frameSize().height()-20;
	int width  = frameSize().width ()-20;

	return (width<height)? width/getNumberOfTicks() : height/getNumberOfTicks();
}

void PlotView::drawScene()
{
	int distance = getTickDistance();

	for (int i=0;i<m_itemList.size();i++){
	  m_itemList.at(i)->setPos(
			  m_itemList.at(i)->getLocation().x()* distance,
			  m_itemList.at(i)->getLocation().y()*-distance);
	}

    viewport()->update();
}

void PlotView::do_drawScene()
{
	int distance = getTickDistance();

	scene()->clear();
	for (int i=0;i<m_itemList.size();i++){
	  m_itemList.at(i)->setPos(
			  m_itemList.at(i)->getLocation().x()*distance,
			  m_itemList.at(i)->getLocation().y()*-distance);		
	  scene()->addItem(m_itemList.at(i));
	}

    viewport()->update();
}

void PlotView::resizeEvent(QResizeEvent *e)
{
	drawScene();
}

void PlotView::setRange( int xmin, int xmax, int ymin, int ymax )
{
	minus_rangeX = xmin;
	plus_rangeX  = xmax;
	minus_rangeY = ymin;
	plus_rangeY  = ymax;
}