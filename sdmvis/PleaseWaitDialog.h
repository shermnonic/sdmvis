#ifndef PLEASEWAITDIALOG_H
#define PLEASEWAITDIALOG_H

#include <QDialog>
#include <QString>

#define SHOW_PLEASEWAIT_DIALOG( title, msg )           \
	QApplication::setOverrideCursor( Qt::WaitCursor ); \
	PleaseWaitDialog pwd( msg, title, this );   \
	pwd.show();                                 \
	pwd.raise();                                \
	pwd.activateWindow();                       \
	QApplication::processEvents();

#define HIDE_PLEASEWAIT_DIALOG()                \
	QApplication::restoreOverrideCursor();      \
	pwd.hide();


// Everywhere, where a progress indicator is still missing, one can
// at least show a "Please wait..." message using this dialog.
class PleaseWaitDialog : public QDialog
{
public:
	PleaseWaitDialog( QString msg, QString title, QWidget* parent=0 );
};

#endif // PLEASEWAITDIALOG_H
