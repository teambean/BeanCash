#ifndef BEANCONTROLTREEWIDGET_H
#define BEANCONTROLTREEWIDGET_H

#include <QKeyEvent>
#include <QTreeWidget>

class BeanControlTreeWidget : public QTreeWidget {
Q_OBJECT

public:
    explicit BeanControlTreeWidget(QWidget *parent = 0);
    
protected:
  virtual void  keyPressEvent(QKeyEvent *event);
};

#endif // BEANCONTROLTREEWIDGET_H