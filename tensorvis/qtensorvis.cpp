#include <QApplication>
#include "QTensorVisApp.h"

int main( int argc, char* argv[] )
{
	Q_INIT_RESOURCE( qtensorvis );
	
	QApplication app( argc, argv );

	QTensorVisApp qtensorvis;
	qtensorvis.show();

	return app.exec();
}
