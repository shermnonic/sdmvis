/**************************************************************************
 PlotWidget : Type Widget

 Funktion :  Layout and Data functions

**************************************************************************/


#include "plotWidget.h"
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


bool PlotWidget::s_setAllComp=false;
bool PlotWidget::s_showMovingPoint=false;
int PlotWidget::s_indexOfSelectedPoint=-1;


// Constructor
PlotWidget::PlotWidget(QWidget *parent) :   QWidget(parent)
{
	// init Values
	m_matrixLoaded=false;
	m_pcX=0;
	m_pcY=0;
	m_minRange=0;
	m_maxRange=0;
	m_bool_traitSet=false;
	m_index=0;

	// widgets
	m_view =new PlotView();
	m_scene=new QGraphicsScene();
	m_view->setScene(m_scene);

	// Selection Group
	QGroupBox *selectionGroup =new QGroupBox("Select Trait");
	QHBoxLayout *selectionLayot = new QHBoxLayout;
	QVBoxLayout *spacerLayot = new QVBoxLayout;
	QLabel selectLabel("Trait");
	m_selectComBox= new QComboBox();

	selectionLayot->addWidget(&selectLabel);
	selectionLayot->addWidget(m_selectComBox);
	selectionGroup->setLayout(selectionLayot);

#ifdef PLOTWIDGET_CONTEXT_MENU
	// arrange buttons horizontally
	QHBoxLayout* butLayout = new QHBoxLayout;
	QHBoxLayout* compLayout = new QHBoxLayout;
#else
	// Buttons Layout
	QVBoxLayout* butLayout = new QVBoxLayout;

	// Components Layout
	QVBoxLayout* compLayout = new QVBoxLayout;
#endif
	QHBoxLayout* xLayout = new QHBoxLayout;
	QHBoxLayout* yLayout = new QHBoxLayout;
	QHBoxLayout* headerLayout = new QHBoxLayout;

	QLabel *componentHeaderLabel = new QLabel("Component");
	QLabel *compenentX = new QLabel ("X");
	QLabel *compenentY = new QLabel ("Y");

	m_combX  = new QComboBox();
	m_combY  = new QComboBox();

	headerLayout->addWidget(componentHeaderLabel);
	compLayout->addLayout(headerLayout);

	xLayout->addWidget(compenentX);
	xLayout->addWidget(m_combX);
	compLayout->addLayout(xLayout);

	yLayout->addWidget(compenentY);
	yLayout->addWidget(m_combY);
	compLayout->addLayout(yLayout);

	QPushButton * crosshairButton= new QPushButton("Show Free Item");
	crosshairButton->setCheckable(true);

	QPushButton * resetCompButton= new QPushButton("Reset");
	QPushButton * showCSVExportButton= new QPushButton("CSV Export");	

	m_cb_drawSigma=new QCheckBox("Draw Circles");
	m_cb_drawAxis=new QCheckBox("Draw Axis");
	m_cb_drawAxis->setChecked(true);
	m_cb_setAllComp=new QCheckBox("Set All Components");
	m_cb_setAllComp->setChecked(false);
	m_cb_drawNames=new QCheckBox("Draw Names");
	m_cb_drawNames->setChecked(true);
	m_cb_drawTraits=new QCheckBox("Draw Traits");
	m_cb_drawTraits->setDisabled(true);

#ifdef PLOTWIDGET_CONTEXT_MENU
	m_view->setContextMenuPolicy( Qt::CustomContextMenu );

	QAction *act_crosshair,
		    *act_reset,
		    *act_exportCSV,
		    *act_drawSigma,
		    *act_drawAxis,
			*act_setAllComp,
			*act_drawNames,
			*act_drawTraits;	
	act_crosshair = new QAction( tr("Free item mode"), this );
	act_crosshair->setCheckable( true  );
	act_crosshair->setChecked  ( false );
	act_reset     = new QAction( tr("Reset"), this );
	act_exportCSV = new QAction( tr("Export as CSV..."), this );
	act_drawSigma = new QAction( tr("Show stdev circles"), this );
	act_drawSigma->setCheckable( true  );
	act_drawSigma->setChecked  ( false );
	act_drawAxis = new QAction( tr("Show axes"), this );
	act_drawAxis->setCheckable( true );
	act_drawAxis->setChecked  ( true );
	act_drawNames = new QAction( tr("Show names"), this );
	act_drawNames->setCheckable( true );
	act_drawNames->setChecked  ( true );
	act_drawTraits = new QAction( tr("Show trait"), this );
	act_drawTraits->setCheckable( true );
	act_drawTraits->setChecked  ( true );
	act_setAllComp = new QAction( tr("Set all components"), this );
	act_setAllComp->setToolTip( tr("Update all components on item selection.") );
	act_setAllComp->setCheckable( true  );
	act_setAllComp->setChecked  ( false );

	m_contextMenu = new QMenu;
	m_contextMenu->addAction( act_crosshair );
	m_contextMenu->addAction( act_reset );
	m_contextMenu->addAction( act_setAllComp );
	m_contextMenu->addSeparator();
	m_contextMenu->addAction( act_exportCSV );
	m_contextMenu->addSeparator();
	m_contextMenu->addAction( act_drawSigma );
	m_contextMenu->addAction( act_drawAxis );
	m_contextMenu->addAction( act_drawNames );
	m_contextMenu->addAction( act_drawTraits );

	connect( m_view, SIGNAL(customContextMenuRequested(const QPoint&)), 
		     this, SLOT(contextMenu(const QPoint&)) );

	connect( act_crosshair, SIGNAL(toggled(bool)),this,SLOT(do_showMovingPoint(bool)));
	connect( act_reset,     SIGNAL(triggered()),  this,SLOT(resetComp()));
	connect( act_exportCSV, SIGNAL(triggered()),  this,SLOT(do_csvExport())   );
	connect( act_drawAxis,  SIGNAL(toggled(bool)),this,SLOT(drawAxis(bool))   );
	connect( act_drawTraits,SIGNAL(toggled(bool)),this,SLOT(drawTraits(bool)) );
	connect( act_drawSigma, SIGNAL(toggled(bool)),this,SLOT(drawCircles(bool)));
	connect( act_drawNames, SIGNAL(toggled(bool)),this,SLOT(drawNames(bool))  );
	connect( act_setAllComp,SIGNAL(toggled(bool)),this,SLOT(setAllComp(bool)) );
	
	//butLayout->addWidget(crosshairButton);
#else
	butLayout->addWidget(crosshairButton);
	butLayout->addWidget(resetCompButton);
	butLayout->addWidget(showCSVExportButton);
		
	butLayout->addWidget(m_cb_drawAxis);
	butLayout->addWidget(m_cb_drawNames);
	butLayout->addWidget(m_cb_drawTraits);
	butLayout->addWidget(m_cb_drawSigma);

	butLayout->addWidget(m_cb_setAllComp);
#endif

	m_browser = new QTextBrowser();

	butLayout->addLayout(compLayout);

	QLabel *spacerLabel=new QLabel("");
	butLayout->addWidget(spacerLabel,100);

	//butLayout->addWidget( theButton );
// TEST:
	QHBoxLayout *templayout= new QHBoxLayout();
	templayout->addWidget(&m_itemLabel);
	m_itemLabel.setText("");
	connect(m_view,SIGNAL(setItemLabel(QString)),this,SLOT(setItemLabel(QString)));
	// -- Main layout
	QGridLayout* layout = new QGridLayout;
	//layout->addWidget(m_browser,1,0); // debug fenster 
#ifdef PLOTWIDGET_CONTEXT_MENU
	// buttons on top
	layout->addLayout( butLayout,  0,0 );
	layout->addWidget( m_view,     1,0 );
	layout->addLayout( templayout, 2,0 );
#else
	// buttons to the right
	layout->addWidget( m_view,     0,0 );
	layout->addLayout( butLayout,  0,1 );
#endif
	layout->setContentsMargins( 0,0,0,0 );

	QVBoxLayout *mainLayout = new QVBoxLayout;
#if 1  // Not required for PCA raycaster
	mainLayout->addWidget(selectionGroup);
#endif
	mainLayout->addLayout(layout);
	mainLayout->setContentsMargins(1,1,1,1);

	setLayout( mainLayout );	

	// connections
	connect(m_combX,SIGNAL(activated(int)),this,SLOT(setComp1(int)));
	connect(m_combY,SIGNAL(activated(int)),this,SLOT(setComp2(int)));
	connect(m_selectComBox,SIGNAL(activated(int)),this,SLOT(do_changeTrait(int)));
	// Buttons
	connect(crosshairButton,SIGNAL(toggled(bool)),this,SLOT(do_showMovingPoint(bool)));
	connect(resetCompButton,SIGNAL(clicked()),this,SLOT(resetComp()));
	connect(showCSVExportButton,SIGNAL(clicked()),this,SLOT(do_csvExport()));
	// Check Boxes
	connect(m_cb_drawAxis  ,SIGNAL(toggled(bool)),this,SLOT(drawAxis(bool)));
	connect(m_cb_drawTraits,SIGNAL(toggled(bool)),this,SLOT(drawTraits(bool)));
	connect(m_cb_drawSigma ,SIGNAL(toggled(bool)),this,SLOT(drawCircles(bool)));
	connect(m_cb_drawNames ,SIGNAL(toggled(bool)),this,SLOT(drawNames(bool)));
	connect(m_cb_setAllComp,SIGNAL(toggled(bool)),this,SLOT(setAllComp(bool)));
	// View Connections
	connect(m_view,SIGNAL(userModeSelection(double,double)),this,SLOT(do_movePointInView(double,double)));
	connect(m_view,SIGNAL(itemSelected(int)),this,SLOT(itemSelected(int)));
	connect(this,SIGNAL(drawScene()),m_view,SLOT(drawScene()));
	connect(this,SIGNAL(selectionMode(int)),m_view,SLOT(setSelectionMode(int)));
	
	
	// init werte fuer plotter
	m_view->setRange( -3, 3 );
	m_movingItem = new PlotItem(-1,Qt::black,"Free Item");
	m_view->setOffset(30);
}

#ifdef PLOTWIDGET_CONTEXT_MENU
void PlotWidget::contextMenu( const QPoint& pos )
{
	m_contextMenu->exec( m_view->mapToGlobal(pos) );
}
#endif

void PlotWidget::setItemLabel(QString name)
{
	m_itemLabel.setText(name);
}
void PlotWidget::do_csvExport()
{
	CSVExporter * exportDialog= new CSVExporter();
	exportDialog->setPcComp(m_pcX,m_pcY);
	exportDialog->setMatrix(m_plotterMatrix);
	exportDialog->setPath(m_configPath,m_configName);
	exportDialog->show();
}

void PlotWidget::setIndex(int index)
{
	m_index=index;
	m_selectComBox->blockSignals(true);
	m_selectComBox->setCurrentIndex(m_index);
	m_selectComBox->blockSignals(false);
}

void PlotWidget::setTraitList(QList<Trait> traits)
{
	m_selectComBox->clear();
		m_cb_drawTraits->setDisabled(false);
	for (int iA=0;iA<traits.size();iA++){
		m_selectComBox->addItem(traits.at(iA).identifier);
	}
	
	// load first trait 
//	traitChanged(0);	
}
void PlotWidget::drawCircles( bool b )
{
	m_view->drawSigmaCircle( b );
	emit sig_drawSigma( b );
    m_view->viewport()->update();
}
void PlotWidget::resizeEvent(QResizeEvent *e)
{
/*	int height  =m_view->frameSize().height()-20;
	int width   =m_view->frameSize().width()-20;
	int distance;
	int numberOfItems=qMax(
		 abs(m_view->minus_rangeX)+m_view->plus_rangeX,
		 abs(m_view->minus_rangeY)+m_view->plus_rangeY); // ticks in the axis

	// bestimme wiviele werte benoetigt werden auf minaler achse
	if(width<height)
	  distance=width/numberOfItems;
	else
	  distance=height/numberOfItems;

	for (int i=0;i<m_itemList.size();i++){
	  m_itemList.at(i)->setPos(
		  m_itemList.at(i)->getLocation().x()*distance,
		  m_itemList.at(i)->getLocation().y()*-distance
		 );
	}
	if (s_showMovingPoint){
		double dtempX=m_movingPointVector[m_pcX];
		double dtempY=m_movingPointVector[m_pcY];
		int tempX=m_movingPointVector[m_pcX]*10*distance;
		int tempY=m_movingPointVector[m_pcY]*-10*distance;

		m_movingItem->setPos(
				m_movingPointVector[m_pcX]*10*distance,
				m_movingPointVector[m_pcY]*-10*distance);
		}
			m_view->viewport()->update();
			
*/
}
void PlotWidget::do_movePointInView(double x,double y)
{
	//calculate new Location
	s_indexOfSelectedPoint=-1;

	int distance = m_view->getTickDistance();

	double loc_X= (x / 10.0) / (double) distance;
	double loc_Y= (y / 10.0) / (double)-distance;

	updateVector(m_pcX,loc_X,m_pcY,loc_Y);
	
	emit changeItem(-1,m_pcX,m_pcY,loc_X,loc_Y);
	do_drawMovingPoint(distance);
}

QColor PlotWidget::getClassColor(QVector<int> someVector,int index)
{
	if (someVector[index]==-1)
		return Qt::blue;
	if (someVector[index]==1)
		return Qt::red;
	else
		return Qt::black;
}

void PlotWidget::do_showMovingPoint(bool toggeld)
{
	if(toggeld){
		s_showMovingPoint=true;
		emit selectionMode(0);	// set itemSelection to false 
		emit selectionMode(4);
		emit setComp1(m_pcX);
		 #ifdef DEBUG_PLOT
         std::cout << "DEBUG: Crosshair mode enabled" << std::endl;
        #endif
		 

	}
	else{
		s_showMovingPoint=false;
		emit selectionMode(1);
		emit setComp1(m_pcX);
		m_view->viewport()->update();

		#ifdef DEBUG_PLOT
         std::cout << "DEBUG: Crosshair mode disabled" << std::endl;
        #endif
	
	}
}

void PlotWidget::prepareMatrix( int m, int n, Matrix *plotMatrix,
	double traitScaling,
	QStringList names, QString configPath, QString configName )
{
	this->m_numPCs=n;
	this->m_plotterMatrix=plotMatrix;
	this->m_nameList=names;
	this->m_configName=configName;
	this->m_configPath=configPath;

	m_view->m_itemList.clear();
	m_view->scene()->clear();
	m_view->update();

	m_view->setTraitScaling( traitScaling );

	m_combX->clear();
	m_combY->clear();
	for (int iA=0;iA<m_numPCs;iA++) 
	{
		m_combX->addItem(tr("PC %1").arg(iA+1));
		m_combY->addItem(tr("PC %1").arg(iA+1));
	}

	m_pcX=0;
	m_pcY=1;
	m_combX->setCurrentIndex( m_pcX );
	m_combY->setCurrentIndex( m_pcY );

	m_matrixLoaded=true;
	m_movingPointVector.resize(m_numPCs);
	m_classVector.resize(m);

	for (int iA=0;iA<m_numPCs;iA++)
		m_classVector[iA]=0;
    
	createScene();
}

void PlotWidget::setConfigName(QString filename)
{
	m_configName=filename;
}

QString getShortName( const QString& name )
{
	// Strings up to 10 characters long are suited as labels
	if( name.length() < 10 )
		return name;

	QString shortName; 
	QStringList tokens = name.split("_", QString::SkipEmptyParts);	
	if( tokens.size()==1 )
	{
		// "MyMuchTooLongName" -> "MyM..ame"
		shortName = name.left(3) + QString("..") + name.right(3);
	}
	else
	{
		// "Multi_token_identifiert_string_Foo12345" -> "Mu.to..12345"
		// "Multi_Foo12345" -> "Mu..12345"
		for( unsigned i=0; i < std::min( tokens.size()-1, 2 ); i++ )
		{
			shortName += tokens.at(i).left(2) + QString(".");
		}
		shortName += tokens.last().right(5);		
	}

	return shortName;
}

void PlotWidget::createScene()
{
	if( !m_matrixLoaded )
		return;

	double minimumItem=0;
	double maximumItem=0;
		
		
	// find min and max values 
	for(int iA=0;iA<m_numPCs;iA++)
	{
		// find maximum
		if(maximumItem<m_plotterMatrix->at_element(iA,m_pcX))
			maximumItem=m_plotterMatrix->at_element(iA,m_pcX);
		if(maximumItem<m_plotterMatrix->at_element(iA,m_pcY))
			maximumItem=m_plotterMatrix->at_element(iA,m_pcY);
		// find minimum
		if(minimumItem>m_plotterMatrix->at_element(iA,m_pcX))
			minimumItem=m_plotterMatrix->at_element(iA,m_pcX);
		if(minimumItem>m_plotterMatrix->at_element(iA,m_pcY))
			minimumItem=m_plotterMatrix->at_element(iA,m_pcY);
	}
 
	// double arithmetik HACK
	minimumItem*=10;
	maximumItem*=10;
	m_view->setRange( minimumItem-1, maximumItem+1 );

	// moving Item
		m_movingItem = new PlotItem(-1,Qt::black,"Free Item");
		double tempX=m_movingPointVector.at(m_pcX);
		double tempY=m_movingPointVector.at(m_pcY);
		m_movingItem->setLocation(QPointF(tempX*10,tempY*10)); // y value
		 
		if(s_showMovingPoint)
		{
		m_view->m_itemList.append(m_movingItem);
		m_view->scene()->addItem(m_movingItem);
		}
	
  
	// create items
	for(int iA=0;iA<m_numPCs;iA++) 
	{
		// create Short Name
		QString shortName = getShortName( m_nameList.at(iA) );

		// create Item
		PlotItem *tempPoint=new PlotItem(iA,getClassColor(m_classVector,iA),shortName);
		// calc location for the point in the View
		double x_loc=m_plotterMatrix->at_element(iA,m_pcX);
		double y_loc=m_plotterMatrix->at_element(iA,m_pcY);
		x_loc*=10;
		y_loc*=10;
		if (iA==s_indexOfSelectedPoint)
		{// this point is selected
			tempPoint->setSelected(true);
			tempPoint->setStillSelected(true);
		}
		tempPoint->setLocation(QPointF(x_loc,y_loc));
		tempPoint->setToolTip(m_nameList.at(iA)+" Pos: ("+QString::number(m_plotterMatrix->at_element(iA,m_pcX))+
							" , "+QString::number(m_plotterMatrix->at_element(iA,m_pcY))+" )");
      
		// push Point into View->ItemList		
		tempPoint->setThisPointSize(4);
		m_view->m_itemList.append(tempPoint);
		m_view->scene()->addItem(tempPoint);
	}	
		
	emit drawScene(); // tell view to draw the new Scene
}

void PlotWidget::drawAxis( bool b )
{
   m_view->m_bool_drawAxis = b;
   m_view->viewport()->update();
}

void PlotWidget::setAllComp( bool b )
{
	s_setAllComp = b;
}

void PlotWidget::drawTraits( bool b )
{
	m_view->s_bool_drawTraits = b;
	emit sig_drawTraits( b );
	m_view->viewport()->update();
}

void PlotWidget::updateMovingPointPosition( int pcValue, double value )
{
	// update vector for mp, view changes when pc is selected
	updateVector(pcValue,value);
	do_drawMovingPoint( m_view->getTickDistance() );	
}

void PlotWidget::do_drawMovingPoint( int distance )
{
	double tempX=m_movingPointVector[m_pcX]*10;
	double tempY=m_movingPointVector[m_pcY]*10;
	
	m_movingItem->setLocation(
		QPointF(tempX,tempY));
	
	emit drawScene();
}

void PlotWidget::reset()
{
	for (int iA=0;iA<m_classVector.size();iA++)
		m_classVector[iA]=0;
	m_selectComBox->clear();
	createScene();
	m_cb_drawTraits->setDisabled(true);
	m_cb_drawTraits->setChecked(false);
	m_view->s_bool_drawTraits=m_cb_drawTraits->isChecked();
	emit sig_drawTraits(m_cb_drawTraits->isChecked());
	m_view->viewport()->update();

}

void PlotWidget::do_changeTrait(int index)
{
emit traitChanged(index);
}

void PlotWidget::drawNames( bool b )
{
	PlotItem::s_showNames = b;
    m_view->viewport()->update();
}

void PlotWidget::setComp1(int component)
{   
   m_scene->clear();
   m_view->m_itemList.clear();
   m_maxRange=0;
   m_minRange=0;
   m_pcX=component;
   m_view->m_lableX="PC"+QString::number(m_pcX+1);
    if(m_view->s_bool_drawTraits && m_bool_traitSet){
	  calcTrait();
   }
   createScene();
}

void PlotWidget::setComp2(int component)
{
   
   m_scene->clear();
   m_view->m_itemList.clear();
   m_minRange=0;
   m_maxRange=0;
   m_pcY=component;
   m_view->m_lableY="PC"+QString::number(m_pcY+1);
   
    if(m_view->s_bool_drawTraits && m_bool_traitSet){
	  calcTrait();
   }
   createScene();
}
void PlotWidget::setClassVector(Vector labels)
{
	m_classVector.resize(labels.size());
	for (int iA=0;iA<m_classVector.size();iA++){
		m_classVector[iA]=labels[iA];
	}
}
void PlotWidget::setTrait( Trait trait )
{	m_browser->clear();
	m_trait = trait;
	m_trait.computeNormal();
	m_bool_traitSet=true;
	setClassVector(m_trait.labels);
	calcTrait();
	emit setComp1(m_pcX);
}

void PlotWidget::calcTrait()
{	
	int normalSize=m_trait.normal.size();
	if (m_pcX< normalSize &&  m_pcY< normalSize)
	{
		QPointF a;
		a.setX(m_trait.normal[m_pcX]);
		a.setY(m_trait.normal[m_pcY]);

		QPointF normalized_a;
		
		double c= a.x()*a.x() + a.y()*a.y();
		c=sqrt(c);
		
		// richtige berechnung
		
		normalized_a.setX(a.x()/c);
		normalized_a.setY(a.y()/c);

		QPointF d;
		d.setX(-normalized_a.y());
		d.setY(normalized_a.x());
		

		QPointF start;
		QPointF end;

		double alpha1=3.0;
		double alpha2=3.0;
		double distance= -m_trait.distance;
		// WORKAROUNDS: 
		//	- negative distance
		//  - scaling

		start=normalized_a*distance+alpha1*d;
		end = normalized_a*distance-alpha2*d;
		
		m_view->paintTraits(start*10,end*10,10,20);
	}
	else
	{
		m_view->paintTraits(QPointF(0,0),QPointF(10,0),0,0);	 
	}	
}

void PlotWidget::itemSelected(int index)
{
	m_itemLabel.setText(m_nameList.at(index));
	s_indexOfSelectedPoint=index; // save the selected point 	

	int distance = m_view->getTickDistance();

	if (!s_setAllComp)
	{
		double valueX=m_plotterMatrix->at_element(index,m_pcX);
		double valueY=m_plotterMatrix->at_element(index,m_pcY);
		updateVector(m_pcX,valueX,m_pcY,valueY);
		do_drawMovingPoint(distance);
		emit changeItem(index,m_pcX,m_pcY,valueX,valueY);
	}
	else // jetzt wirds dirty ....och nööö ;-)
	{
		for (int iA=0;iA<m_numPCs;iA++){
			double value=m_plotterMatrix->at_element(index,iA);
			emit changeItem(iA,value);
		
		// un petit hack mack  fuer mp
		// da nur 5 modele da sind wird ab dem sechsten der wert nicht geaendert , also gehen wir 
		//das um und schreiben es direct in den mp vector
		m_movingPointVector[iA]=value;
	    
		}
		do_drawMovingPoint(distance);
	}
}

void PlotWidget::updateVector(int pc, double value){
	m_movingPointVector[pc]=value;
}

void PlotWidget::updateVector(int pc1, double value1,int pc2,double value2){
	m_movingPointVector[pc1]=value1;
	m_movingPointVector[pc2]=value2;

}

void PlotWidget::resetComp()
{
	for (int iA=0;iA<m_numPCs;iA++)
	{
		emit changeItem(iA,0.0);
	    m_movingPointVector[iA]=0.0;
	}
	
    s_indexOfSelectedPoint=-1;
}