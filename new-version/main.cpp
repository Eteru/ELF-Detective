#include "mainwindow.h"
#include <QDesktopWidget>
#include <QApplication>

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  QDesktopWidget dw;
  MainWindow w;

  QRect mainScreenSize = dw.availableGeometry(dw.primaryScreen());

  int x = mainScreenSize.width() *0.7;
  int y = mainScreenSize.height()*0.7;
  w.setFixedSize(x,y);

  w.show();

  return a.exec();
}
