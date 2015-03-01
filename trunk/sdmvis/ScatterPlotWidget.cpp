/**************************************************************************
 ScatterPlotWidget : Type Widget

 Funktion : Draw the first 10 pca models

**************************************************************************/


#include "ScatterPlotWidget.h"
#include "plotView.h"
#include "plotItem.h"
#include <algorithm>
#include "numerics.h"

// Qt Includes
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QGraphicsView>
#include <QTableView>
#include <QListView>
#include <QTextBrowser>
#include <QGraphicsEllipseItem>
#include <QCheckBox>
#include <qmath.h>


bool ScatterPlotWidget::s_matrixLoaded=false;
bool ScatterPlotWidget::s_traitPresent=false;

// Constructor
ScatterPlotWidget::ScatterPlotWidget(QWidget *parent) :
    QWidget(parent),
	m_traitScaling( 1.0 )
{
	mainLayout = new QGridLayout;
	mainLayout->setVerticalSpacing(0);
	mainLayout->setHorizontalSpacing(0);

	mainLayout->setContentsMargins( 0,0,0,0 );
	createLayout();
	m_zoom=1.0;
	setLayout( mainLayout );
	this->resize(1024,860);
}

void ScatterPlotWidget::itemSelected(int index)
{
		deselect();
		for (int iA=0;iA<m_viewList.size();iA++)
		{
			m_viewList.at(iA)->m_itemList.at(index)->setSelected(true);
			m_viewList.at(iA)->m_itemList.at(index)->setStillSelected(true);
			m_viewList.at(iA)->viewport()->update();
		}
}

void ScatterPlotWidget::deselect()
{	
	
	for (int iA=0;iA<m_viewList.size();iA++)
		for (int iB=0;iB<m_viewList.at(iA)->m_itemList.size();iB++)
		{
			m_viewList.at(iA)->m_itemList.at(iB)->setSelected(false);
			m_viewList.at(iA)->m_itemList.at(iB)->setStillSelected(false);
			m_viewList.at(iA)->viewport()->update();
		}
}

void ScatterPlotWidget::createLayout()
{
	for (int iA=0;iA<6;iA++)
	{
		for (int iB=0;iB<6;iB++)
		{
			if (iA<iB)
			{
				PlotView *m_view =new PlotView();
				m_view->drawTags(false);
				m_view->setAxisColor(Qt::gray);
				m_view->setAxisSize(1);
				m_view->setTickSize(2);
				m_view->dbg_xcomp=iB;
				m_view->dbg_ycomp=iA;
				QGraphicsScene* m_scene=new QGraphicsScene();
				m_view->setSceneRect(0,0,200,200);
				// init values 
			  #if 0
				// hard coded range
				m_view->minus_rangeX=-3;  // min in compX
				m_view->plus_rangeX=3;   // max in compX
				m_view->minus_rangeY=-3;  // min in compY
				m_view->plus_rangeY=3;   // max in compY
			  #endif
				m_view->setScene(m_scene);
				mainLayout->addWidget( m_view,iA,iB);
				
				m_sceneList.append(m_scene);
				m_viewList.append(m_view);
				
				//connect(m_view,SIGNAL(itemSelected(int)),this,SLOT(itemSelected(int)));
				//connect(m_view,SIGNAL(deselect()),this,SLOT(deselect()));				
			}
			else if (iA==iB)
			{
				// Diaglonalen 
				QLabel *pcLabel = new QLabel("pc "+QString::number(iA+1));
				mainLayout->addWidget( pcLabel,iA,iB,Qt::AlignCenter);
			}
		}
	}
}


QColor ScatterPlotWidget::getClassColor(QVector<int> someVector,int index)
{
	if (someVector[index]==-1)
		return Qt::blue;
	if (someVector[index]==1)
		return Qt::red;
	else
		return Qt::black;
}

void ScatterPlotWidget::clearData()
{
	m_viewList.clear();
	m_sceneList.clear();
}

void ScatterPlotWidget::drawTraitLine(bool yes)
{
	for (int iA=0;iA<m_viewList.size();iA++)
		m_viewList.at(iA)->viewport()->update();	
}

void ScatterPlotWidget::drawSigma(bool yes)
{
	for (int iA=0;iA<m_viewList.size();iA++)
	{
		m_viewList.at(iA)->drawSigmaCircle(yes,0.2); // default Vaule TODO right sigma Value
		m_viewList.at(iA)->viewport()->update();
	}
}

void ScatterPlotWidget::createData( double min, double max )
{
	for (int iA=0;iA<m_viewList.size();iA++)
	{
		PlotView* plot = m_viewList[iA];
		int xComp=plot->dbg_xcomp;
		int yComp=plot->dbg_ycomp;

		plot->setTraitScaling( m_traitScaling );

		plot->minus_rangeX=min;
	    plot->minus_rangeY=min;
		plot->plus_rangeX=max;
		plot->plus_rangeY=max;

		int normalSize=m_trait.normal.size();
		if (s_traitPresent  && xComp< normalSize &&  yComp< normalSize)
		{
			QPointF a;
			QPointF d;	
			QPointF normalized_a;			
			
			a.setX(m_trait.normal[xComp]);
			a.setY(m_trait.normal[yComp]);
			double c= a.x()*a.x() + a.y()*a.y();
			c=sqrt(c);
		
			normalized_a.setX(a.x()/c);
			normalized_a.setY(a.y()/c);

			d.setX(-normalized_a.y());
			d.setY(normalized_a.x());

			QPointF start;
			QPointF end;

			double alpha1=3.0;
			double alpha2=3.0;
			double distance= -m_trait.distance;  // WORKAROUND: negative distance

			start=normalized_a*distance+alpha1*d;
			end = normalized_a*distance-alpha2*d;
		
			plot->paintTraits(start*10,end*10,10,20);
		}
		plot->m_itemList.clear();

		for (int iB=0;iB<m_sizeX;iB++)
		{
			// create Item
			PlotItem *tempPoint=new PlotItem(iB,getClassColor(m_classVector,iB),"");// no name for ScatterPlot Item
			// calc location for the point in the View
			double x_loc=m_plotterMatrix->at_element(iB,xComp);
			double y_loc=m_plotterMatrix->at_element(iB,yComp);
			x_loc*=10;
			y_loc*=10;

			tempPoint->setLocation(QPointF(x_loc,y_loc));
			tempPoint->setToolTip(m_nameList.at(iB)+" Pos: ("+QString::number(m_plotterMatrix->at_element(iB,xComp))+
							 " , "+QString::number(m_plotterMatrix->at_element(iB,yComp))+" )");
      
			tempPoint->setThisPointSize(2);
			// push Point into View->ItemList		
			plot->m_itemList.push_back(tempPoint);
		}
		plot->do_drawScene();

	}
}

void ScatterPlotWidget::wheelEvent(QWheelEvent *event)
{
/*
	int my_delta=event->delta();
	if (event->delta()<0)
		m_zoom=0.9;
	else
		m_zoom=1.1;
	for (int iA=0;iA<m_viewList.size();iA++){
		m_viewList.at(iA)->scale(m_zoom,m_zoom);
		m_viewList.at(iA)->viewport()->update();
		
	}
*/
}

void ScatterPlotWidget::prepareMatrix( int m, int n, Matrix *plotMatrix, 
	double traitScaling, QStringList names )
{
	this->m_sizeX=m;
    this->m_sizeY=n;
	this->m_plotterMatrix=plotMatrix;
	this->m_nameList=names;

	s_matrixLoaded=true;
	
	// reset classVector
	m_classVector.resize(m);
	for (int iA=0;iA<m_sizeX;iA++)
		m_classVector[iA]=0;

	// calculate minimum Item 
	{ 
		double minimumItem=0.0;
		double maximumItem=0.0;

		for (int iA=0;iA<m_sizeX;iA++)
		{
			for (int iB=0;iB<m_sizeY;iB++)	
			{
				if(maximumItem<m_plotterMatrix->at_element(iB,iA))
					maximumItem=m_plotterMatrix->at_element(iB,iA);
				if(minimumItem>m_plotterMatrix->at_element(iB,iA))
					minimumItem=m_plotterMatrix->at_element(iB,iA);
			}
	 
		}		
		// double arithmetik HACK
		minimumItem*=10;
		maximumItem*=10;  
		// set member vars
		m_min=minimumItem-1;
		m_max=maximumItem+1;
	}	
	// clear item lists
	for (int iA=0;iA<m_viewList.size();iA++)
		m_viewList.at(iA)->m_itemList.clear();

	// create plot data	
	m_traitScaling = traitScaling;
	createData( m_min, m_max );	
}

void ScatterPlotWidget::setTrait(Trait trait){
	m_trait=trait;
	s_traitPresent=true;
	setClassVector(m_trait.labels);

	// create plot data
	createData(m_min,m_max);
}

void ScatterPlotWidget::setEmptyClassVector()
{
	for (int iA=0;iA<m_classVector.size();iA++){
		m_classVector[iA]=0;
	}
	// create plot data
	createData(m_min,m_max);
}

void ScatterPlotWidget::setClassVector(Vector labels)
{
	m_classVector.resize(labels.size());
	for (int iA=0;iA<m_classVector.size();iA++)
		m_classVector[iA]=labels[iA];
}





