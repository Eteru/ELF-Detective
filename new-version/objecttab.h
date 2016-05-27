#ifndef OBJECTTAB_H
#define OBJECTTAB_H

#include <QWidget>
#include <QTreeWidget>

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
  void selectSymbol(QString) const;
  void selectFunction(QString, int = -1) const;
  void selectFunctionLine(QString, int) const;
  void toggleHex(bool) const;
  QTreeWidget *getTree() const;

private:
  Ui::objecttab *ui;
};

#endif // OBJECTTAB_H
