#ifndef OBJECTTAB_H
#define OBJECTTAB_H

#include <QWidget>

namespace Ui {
  class objecttab;
}

class objecttab : public QWidget
{
  Q_OBJECT

public:
  explicit objecttab(QWidget *parent = 0);
  ~objecttab();

  void addSymbol(std::string);

private:
  Ui::objecttab *ui;
};

#endif // OBJECTTAB_H
