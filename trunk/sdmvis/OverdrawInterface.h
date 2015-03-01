#ifndef OVERDRAWINTERFACE_H
#define OVERDRAWINTERFACE_H

#include <QPainter>

/// Basic interface for QPainter overdraw widgets
class OverdrawInterface
{
protected:
	virtual void overdraw( QPainter& painter )=0;
};

#endif // OVERDRAWINTERFACE_H
