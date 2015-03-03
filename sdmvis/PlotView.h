#ifndef PLOTVIEW_H
#define PLOTVIEW_H

#include <QtGui/QGraphicsView>
#include <QRectF>
#include <QtGui>
#include <QtGui/QGraphicsItem>
#include <QtGui/QStyleOptionGraphicsItem>
#include <QString>

#include "PlotWidget.h"
#include "ScatterPlotWidget.h"
#include "numerics.h"

class QGraphicsView;
class QMouseEvent;
class QPainter;
class QRubberBand;
class PlotItem;

class PlotView : public QGraphicsView
{
   Q_OBJECT

   // FIXME: Friend declaration only a workaround to enable private member
   //        access. Better introduce getter/setter functions to access members.
   friend class PlotWidget;
   friend class ScatterPlotWidget;

public:
    PlotView( QWidget* parent=0 );

	/// Return distance between ticks (on minor axis?)
	int getTickDistance() const;

	/// Set x/y axis range
	void setRange( int xmin, int xmax, int ymin, int ymax );
	/// Set same range for x/y axis
	void setRange( int xymin, int xymax ) 
	{
		setRange( xymin, xymax, xymin, xymax );
	}
	
	void setTraitScaling( double s );

private:
    Matrix traitMatrix;
	QList<PlotItem*> m_itemList;
    int minus_rangeX;  // min in compX
    int plus_rangeX;   // max in compX
    int minus_rangeY;  // min in compY
    int plus_rangeY;   // max in compX
	int m_tickSize;
	double m_traitScaling;
	double m_traitStartX;
	double m_traitStartY;
	double m_traitEndX;
	double m_traitEndY;
	int m_traitOffsetPlus;
	int m_traitOffsetMinus;
	int dbg_xcomp;
	int dbg_ycomp;
	int m_offset;
    bool m_bool_drawAxis;
	bool m_bool_drawTags;
	bool m_bool_drawSigma;
	double m_sigma;
	static bool s_bool_drawTraits;
    QString m_lableX;
    QString m_lableY;

	QColor m_axisColor;
	int m_axisSize;

	void setTickSize(int size);
	void drawTags(bool yes);
	void drawTraits(bool yes);
	void drawSigmaCircle(bool yes);
	void drawSigmaCircle(bool yes, double sigma);
	void setAxisColor(QColor color);
	void setAxisSize(int size);
	
	void setOffset(int offset);
	void paintTraits(QPointF start,	QPointF end,int traitOffsetPlus, int traitOffsetMinus);
	void do_drawScene();
	

 signals:
    void selectionSingle( int index );
    void selectionRadial( int pivotIndex, double radius );
    void selectionRubberband( QRectF region );
	void deselect();
    
	int itemSelected(int);
	void setItemLabel(QString);
    void userModeSelection( int index );
    void userModeSelection( double x, double y );

 public:
    enum SelectionMode {
       SELECT_NONE,
       SELECT_SINGLE,
       SELECT_RUBBERBAND,
       SELECT_RADIAL,
       SELECT_USER        ///> Every mode above this is reserved for additional user modes (e.g. in a derived class).
    };

 public slots:
    void setSelectionMode( int mode );
    void resizeEvent(QResizeEvent * e);
	void drawScene();
	void deselectItems();
	
 protected:
    virtual void mousePressEvent  ( QMouseEvent *event );
    virtual void mouseMoveEvent   ( QMouseEvent *event );
    virtual void mouseReleaseEvent( QMouseEvent *event );

    virtual void drawForeground ( QPainter * painter, const QRectF & rect );
    virtual void drawBackground ( QPainter * painter, const QRectF & rect );
	
    void resetPivotRadius();

	/// Return maximum number of tick marks along any plot axis
	int getNumberOfTicks() const;	

 private:
    // variables for radial selection ("rs")
    bool    m_rsBanding;
    QRect   m_rsBandRect;
    qreal   m_rsRadius;
    QPointF m_rsCenter;
    QRectF  m_rsEllipseRect;
    bool    m_userModeActive;
    int     m_pivotIndex;
    int		m_selectionMode;
 };


#endif // plotView_H


