#ifndef BATCHPROCESSINGDIALOG_H
#define BATCHPROCESSINGDIALOG_H

#include <QDialog>
#include <QList>
#include <QProcess>
#include <QProgressBar>

class QTextEdit;
class QTreeWidget;

/// Execute a batch of commands in a \a QProcess (non-blocking).
/// Provides console widget for standard output.
///
/// TODO: Add status and corresponding indicator
/// TODO: Add time measurement
/// TODO: Add log file support
class BatchProcessingDialog : public QDialog
{
	Q_OBJECT

signals:
	void crashed();
	void finished();
	void mhdFinished();
public:
	BatchProcessingDialog( QWidget* parent=0 );

	struct BatchCommand
	{
		QString     prog; ///< command line program call
		QStringList args; ///< command line arguments
		QString     desc; ///< description (optional)
		
		BatchCommand(): run(true) {}
		BatchCommand( QString prog_, QStringList args_, QString desc_="" )
			: run(true) {
				prog = prog_;	args = args_;	desc = desc_; 
			}
		
		QString toString( QString separator=" " );
		bool run;
	};

	typedef QList<BatchCommand> CommandList;
	
	void initBatch( CommandList batch );
	QProcess* process();

	void printConsole( QString msg );

	bool hasCrashed() const { return m_crashed; }

public slots:
	void start();
	void reset();

protected slots:
	void updateConsole();
	void updateRunState();
	void procFinished( int exitCode, QProcess::ExitStatus exitStatus );
	void procStarted();
	void procError( QProcess::ProcessError err );
	void saveLog();

protected:
	bool getNextCommand( BatchCommand& cmd, int& index );
	void updateCmdStatus( int i, QString status );

	QProcess* m_proc;
	int m_curProc; ///< currently running proc, -1 if none is running
	CommandList m_batch;

private:
	QTextEdit *m_info, *m_con;  // m_info is OBSOLETE!
	QTreeWidget *m_cmdTree;
	bool m_crashed;
	QProgressBar * m_progressBar;

};

#endif // BATCHPROCESSINGDIALOG_H
