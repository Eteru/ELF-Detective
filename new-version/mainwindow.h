#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <vector>
#include <addressbinding.h>

namespace Ui {
  class MainWindow;
}

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();

private slots:
  void on_addObj_clicked();

  void on_addExe_clicked();

  void on_runProj_clicked();

  void on_clearProj_clicked();

  void on_objTabs_tabCloseRequested(int index);

  void on_exeDataList_clicked(const QModelIndex &index);

  void on_exeFunctionsTree_itemClicked(QTreeWidgetItem *item, int column);

  void on_checkHex_clicked(bool checked);

private:
  void showSymbols() const;
  void initFunctionTree(ELFFile *E, QTreeWidget *parent) const;
  void addCodeLines(Function *f, QTreeWidgetItem *parent) const;
  QString errorMessage(int errCode, ELFFile *E) const;

  Ui::MainWindow *ui;

  ELFFile *exefile = nullptr;
  std::vector<ELFFile *> objfiles;

  AddressBinding *AB = nullptr;
};

#endif // MAINWINDOW_H
