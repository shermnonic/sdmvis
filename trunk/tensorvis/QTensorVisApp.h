//=============================================================================
//
//  Qt4 application for Tensor Visualization of a Statistical Deformation Model.
//
//=============================================================================
#ifndef _QTENSORVISAPP_H
#define _QTENSORVISAPP_H

#include <QMainWindow>
class QTensorVisWidget;

const QString APP_NAME        ( "qtensorvis" );
const QString APP_ORGANIZATION( "University of Bonn" );

// Forwards
class QDockWidget;
class QTensorVisOptionsWidget;

/// Qt4 application for Tensor Visualization of a Statistical Deformation Model
class QTensorVisApp : public QMainWindow
{
	Q_OBJECT
public:
	QTensorVisApp();
	~QTensorVisApp();

public slots:
	// Append given string to window title, reset title if string is empty
	void updateWindowTitle( QString s );
	// Show status messages in the status bar
	void showStatusMessage( QString s );

protected:
	void closeEvent( QCloseEvent* event );

private:
	/// The main visualization widget
	QTensorVisWidget* m_tensorVis;
	QDockWidget*      m_controlDock;

	///@{ Application settings
	void readSettings();
    void writeSettings();
	QString m_baseDir;
	///@}
};

#endif // _QTENSORVISAPP_H
