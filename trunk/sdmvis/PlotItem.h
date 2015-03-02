#include <QtGui/QColor>
#include <QtGui/QGraphicsEllipseItem>

/**
 *  \class plotItem
 *
 *  Graphical representation of a single item on a map represented by class MDSView.
 *  Each item has an index (which should be unique), an individual color.
 *  Further size adjustment is supported, see  setPointSize() and adjustSize().
 *  Also marking of selected items and a single pivot item is provided, see
 *  setPivotIndex().
 *
 *  \see MDSView
 *  \see MDSMapBrowser
 *
 *  \author Max Hermann <hermann[at]cs.uni-bonn.de>
 */
class PlotItem : public QGraphicsEllipseItem
{
public:
   /// Set pixel size of points.
   static void setPointSize( qreal size );
   /// Get pixel size of points.
   static qreal pointSize() { return s_pointSize; }

   /// Specify which item to draw as "pivot element", set index to -1 to deselect current one.
   static void setPivotIndex( int index ) { s_pivotIndex = index; }
   /// Get event index of item currently drawn as "pivot element".
   static int pivotIndex() { return s_pivotIndex; }

   /// Set color in which "pivot element" is drawn.
   static void setPivotColor( QColor col ) { s_pivotColor = col; }

protected:
   static qreal  s_pointSize;
   static int    s_pivotIndex;
   static QColor s_pivotColor;
  // QRectF boundingRect();

public:
   PlotItem( int index, const QColor& color, QString name );
   int index() const { return m_index; }
   void setThisPointSize(int size);
   static  bool s_showNames;

   /// Call this after changing the static point size via plotItem::setPointSize().
   void adjustSize();

   /// Change color (no update performed).
   void setColor( const QColor& color ) { m_color = color; }
   void setLocation(const QPointF& position){m_location=position;}
   QPointF getLocation(){return m_location;}
   QString getName(){return m_name;}

   void setStillSelected( bool b ) { m_isStillSelected=b; }
   bool isStillSelected() const { return m_isStillSelected; }

protected:
   void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget );

/*
protected:
   void mousePressEvent  ( QGraphicsSceneMouseEvent *event );
   void mouseMoveEvent   ( QGraphicsSceneMouseEvent *event );
   void mouseReleaseEvent( QGraphicsSceneMouseEvent *event );
*/

private:
   QString m_name;
   int    m_index;
   QColor m_color;
   qreal  m_pointSize;
   QRectF m_normalSize;
   QRectF m_selectedSize;
   QRectF m_boundingRect;
   QPointF m_location;

   bool m_isStillSelected;
};

