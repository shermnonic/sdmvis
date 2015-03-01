#ifndef SCATTERPLOTWIDGET_H
#define SCATTERPLOTWIDGET_H
#include <QWidget>
#include <QGenericMatrix>
#include <QStandardItemModel>
#include <QTextBrowser>
#include <QGraphicsScene>
#include <QList>
#include "PlotView.h"
#include "numerics.h"
#include "Trait.h"
#include <QColor>
class PlotItem;
class PlotView;
class ScatterPlotWidget : public QWidget
{
    Q_OBJECT
public:
    ScatterPlotWidget(QWidget *parent = 0);
	void prepareMatrix(int m,int n,Matrix *plotMatrix,double traitScaling, QStringList names);
	void setTrait(Trait trait);
	void setEmptyClassVector();

private:
	// variablen
	QGridLayout *mainLayout;
	
	QList<PlotItem*>		m_itemList;
	QList<PlotView*>		m_viewList;
	QList<QGraphicsScene*>  m_sceneList;
    QStringList				m_nameList;
    Trait					m_trait;
	QVector<double>			m_movingPointVector; 
	QVector<int>			m_classVector;
	
	QGraphicsScene			*m_scene;   
	Matrix					*m_plotterMatrix;
	PlotView				*m_view;
	PlotItem				*m_movingItem;
	
	int			m_sizeX;
	int			m_sizeY;
	double		m_zoom;
	int			m_minRange;
	int			m_maxRange;  
	int			m_min;
	int			m_max;

	double	m_traitScaling;

	static bool s_matrixLoaded;
	static bool s_traitPresent;
	static int  s_indexOfSelectedPoint;
	
	// Functions
    void	createScene();
	void	createLayout();
	void	clearData();
	void	createData(double min, double max);
	void	setClassVector(Vector labels);
	QColor	getClassColor(QVector<int> someVector,int index);

signals:
	 void selectionMode(int);

public slots:
		void itemSelected(int index);
		void deselect();
		void wheelEvent(QWheelEvent *event);
		void drawTraitLine(bool yes);
		void drawSigma(bool yes);
};

#endif // SCATTERplotWidget_H
