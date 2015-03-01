#ifndef TRAITCOMB_H
#define TRAITCOMB_H

#include <QComboBox>

class TraitComb : public QComboBox
{
   Q_OBJECT
public:
    TraitComb();
    TraitComb(int index);
    int getIndex(){return m_index;}

 private :
       int m_index;

};

#endif // TRAITCOMB_H
