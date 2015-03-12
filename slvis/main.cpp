#include <QApplication>
#include <QWidget>
#include <QFile>
#include <QString>
#include <QDebug>

#include <exception>

#include "MainWindow.h"
#include "Viewer.h"

/// Derived QApplication to catch exceptions via notify()
/// See http://stackoverflow.com/questions/4661883/qt-c-error-handling
class QMyApplication : public QApplication
{
public:
	QMyApplication( int & argc, char ** argv )
		: QApplication( argc, argv  )
	{}

protected:
	bool notify( QObject* receiver, QEvent* event )
	{
		try {
			return QApplication::notify( receiver, event );
		} catch (std::exception &e) {
			qFatal("Error %s sending event %s to object %s (%s)", 
				e.what(), typeid(*event).name(), qPrintable(receiver->objectName()),
				typeid(*receiver).name());
		} catch (...) {
			qFatal("Error <unknown> sending event %s to object %s (%s)", 
				typeid(*event).name(), qPrintable(receiver->objectName()),
				typeid(*receiver).name());
		}

		// qFatal aborts, so this isn't really necessary
		// but you might continue if you use a different logging lib
		return false;
	}
};

int main( int argc, char* argv[] )
{
	Q_INIT_RESOURCE( slvis );

	QMyApplication app( argc, argv );

#if 0
	QFile f(":/style.css");	
	if( f.open(QIODevice::ReadOnly | QIODevice::Text) )
	{		
		qDebug("Setting style sheet style.css");
		QString style = QTextStream(&f).readAll();
		//qDebug(style.toStdString().c_str());
		QApplication::instance().setStyleSheet( style );
	}
#endif

	MainWindow mw;
	mw.show();

	// Parse command line options
	if( argc > 1 ) mw.streamlineViewer()->loadSeedPoints ( QString::fromAscii(argv[1]) );
	if( argc > 2 ) mw.streamlineViewer()->loadDeformation( QString::fromAscii(argv[2]) );
	if( argc > 3 ) mw.streamlineViewer()->loadTemplate   ( QString::fromAscii(argv[3]) );
	if( argc > 4 ) mw.streamlineViewer()->loadMesh       ( QString::fromAscii(argv[4]) );

	return app.exec();;
}
