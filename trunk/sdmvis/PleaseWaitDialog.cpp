#include <QtGui>
#include "PleaseWaitDialog.h"

#include "SDMVisConfig.h"  // for APP_NAME and APP_ICON

PleaseWaitDialog::PleaseWaitDialog( QString msg, QString title, QWidget* parent ) 
{
	QLabel* msgLabel = new QLabel( msg );
	QHBoxLayout* layout = new QHBoxLayout;
	layout->addWidget( msgLabel );
	setLayout( layout );
	setWindowTitle( tr("%1 - title").arg(APP_NAME) );
	setWindowIcon( APP_ICON );
}	
