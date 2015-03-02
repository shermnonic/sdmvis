#ifndef PLOTWIDGET_H
#define PLOTWIDGET_H

#include <QWidget>
#include <QGenericMatrix>
#include <QStandardItemModel>
#include <QTextBrowser>
#include <QGraphicsScene>
#include <QList>
#include "PlotView.h"
#include "numerics.h"
#include "Trait.h"
#include "CSVExporter.h"
#include <QVector>
#include <QLabel>

#define PLOTWIDGET_CONTEXT_MENU

class PlotItem;
class PlotView;


class PlotWidget : public QWidget
{
    Q_OBJECT

public:
    PlotWidget(QWidget *parent = 0);
	void setIndex(int index);
	int m_index;
	void clearTraitSeletion(){m_selectComBox->clear();}
	QComboBox*  getSelectionBox(){return m_selectComBox;}

private:
	// variablen
	QLabel			m_itemLabel;
    QStringList		 m_nameList;
	QTextBrowser	*m_browser ;
    QCheckBox		*m_cb_drawAxis;
	QCheckBox		*m_cb_drawNames;
	QCheckBox		*m_cb_setAllComp;
	QCheckBox		*m_cb_drawSigma;
	QCheckBox		*m_cb_drawTraits;

    QGraphicsScene	*m_scene;
    QComboBox		*m_combX;
	QComboBox		*m_combY;
	QComboBox		*m_selectComBox;
	QString			 m_configName;
	QString			 m_configPath;
	CSVExporter		*m_csvExporter;	
    
	Matrix *m_plotterMatrix;
	Trait m_trait;
	bool m_bool_traitSet;
	QVector<double> m_movingPointVector; 
	QVector<int> m_classVector;
	PlotView *m_view;
	PlotItem *m_movingItem;
	int  m_numPCs;
	int  m_pcX;
	int  m_pcY;
	int  m_minRange;
	int  m_maxRange;  
	bool m_matrixLoaded;
	static bool s_showMovingPoint;
	static bool s_setAllComp;
	static int s_indexOfSelectedPoint;

public:
	void prepareMatrix( int m,int n, Matrix *plotMatrix, double traitScaling,
		QStringList names, QString configPath, QString configName );
	void setTrait( Trait trait );
	void setConfigName(QString filename);
	void reset();
	void setTraitList(QList<Trait> traits);

protected:
    void do_drawMovingPoint(int distance);
	void updateVector(int pc,double value);
	void updateVector(int pc1,double value1,int pc2,double value2);
    void createScene();
	void calcTrait();
	void setClassVector(Vector labels);

	QColor getClassColor(QVector<int> someVector,int index);

#ifdef PLOTWIDGET_CONTEXT_MENU
protected slots:
	void contextMenu( const QPoint& );
private:
	QMenu* m_contextMenu;
#endif

signals:
	 void changeItem(int,int,int,double,double);
     void changeItem(int,double);
	 void selectionMode(int);
	 void drawScene();
	 void traitChanged(int);
	 void sig_drawTraits(bool);
	 void sig_drawSigma(bool);

public slots:
	 void setItemLabel(QString);
	 void setComp1(int);
	 void setComp2(int);
	 void resizeEvent(QResizeEvent * e);
	 void do_showMovingPoint(bool);
	 void drawAxis   ( bool );
	 void drawNames  ( bool );
	 void drawTraits ( bool );
	 void drawCircles( bool );
	 void setAllComp ( bool );
	 void itemSelected(int index);
	 void do_movePointInView(double,double);
	 void updateMovingPointPosition(int,double);
	 void resetComp();
	 void do_csvExport();
	 void do_changeTrait(int index);
};

#endif // plotWidget_H
