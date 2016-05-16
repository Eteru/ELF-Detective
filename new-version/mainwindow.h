#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
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

private:
    void showSymbols() const;
    QString errorMessage(int errCode, ELFFile *E) const;

    Ui::MainWindow *ui;

    ELFFile *exefile = nullptr;
    std::vector<ELFFile *> objfiles;

    AddressBinding *AB = nullptr;
};

#endif // MAINWINDOW_H
