#include "objecttab.h"
#include "ui_objecttab.h"

objecttab::objecttab(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::objecttab)
{
  ui->setupUi(this);
  ui->objToolBox->setItemText(0, "Data");
  ui->objToolBox->setItemText(1, "Functions");

  ui->objFunctions->activateWindow();
}

objecttab::~objecttab()
{
  delete ui;
}

void objecttab::addSymbol(std::string sym)
{
  ui->objDataList->addItem(QString::fromStdString(sym));
}
