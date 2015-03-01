#include <QApplication>
#include "SDMVisMainWindow.h"
#include <iostream>

void info_libraryPaths( QApplication& app )
{
	QStringList libpaths = app.libraryPaths();
	std::cout << "QApplication library paths:\n";
	for( int i=0; i < libpaths.size(); ++i )
		std::cout << libpaths[i].toStdString() << std::endl;
}

int main( int argc, char* argv[] )
{
	Q_INIT_RESOURCE( sdmvis );

	QApplication app( argc, argv );

	SDMVisMainWindow sdmvis;
	sdmvis.show();

	// WORKAROUND: Activate QGLWidget tabs manually to force initialization.
	QApplication::processEvents();
	sdmvis.workaround(1);
	QApplication::processEvents();
	sdmvis.workaround(0);
	QApplication::processEvents();

#if 0 // should not be required
	// WORKAROUND: We deploy all needed libraries in the application binary folder.
	app.setLibraryPaths( QStringList() << app.applicationDirPath() );
	info_libraryPaths( app );
#endif

#ifdef DEBUG
	std::cout << "Entering Qt application loop: app.exec()" << std::endl;
#endif
	int ret = app.exec();
	QApplication::closeAllWindows();
	return ret;
}
