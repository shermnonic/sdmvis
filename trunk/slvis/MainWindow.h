#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

class Viewer;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow();
	~MainWindow();

private slots:
	void destroy();

protected:
	void createUI();
	void closeEvent( QCloseEvent* event );

	///@{ Application settings
	void readSettings();
    void writeSettings();
	QString m_baseDir; ///< directory of last successfully opened file
	///@}

private:
	Viewer* m_viewer;
};

#endif // MAINWINDOW_H
