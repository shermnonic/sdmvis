#ifndef MODEANIMATIONWIDGET_H
#define MODEANIMATIONWIDGET_H

#include <QWidget>
#include "ModeAnimationParameters.h"

class QSpinBox;
class QDoubleSpinBox;
class QLineEdit;

/// Mode animation widget collects ModeAnimationParameters.
/// See also QTensorVisWidget::makeModeAnimation().
class ModeAnimationWidget : public QWidget
{
	Q_OBJECT

signals:
	void renderAnimation();
	void bakeAnimation();

public:
	ModeAnimationWidget( QWidget* parent=0 );

	ModeAnimationParameters getModeAnimationParameters() const;

protected:
	void createUI();

protected slots:
	void browseOutputPath();

private:
	QLineEdit* leNameRule;
	QLineEdit* lePath;
	QSpinBox*  sbMode;
	QDoubleSpinBox* sbRangeMin;
	QDoubleSpinBox* sbRangeMax;
	QDoubleSpinBox* sbRangeStep;
};

#endif // MODEANIMATIONWIDGET_H
