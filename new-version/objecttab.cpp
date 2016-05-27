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

  ui->objFunctionsTree->header()->close();
}

objecttab::~objecttab()
{
  delete ui;
}

void objecttab::addSymbol(std::string sym)
{
  ui->objDataList->addItem(QString::fromStdString(sym));
}

void objecttab::selectSymbol(QString sym) const
{
  int rows = ui->objDataList->count();

  for (int row = 0; row < rows; ++row)
    {
      QString rowName = ui->objDataList->item(row)->text();
      if (rowName.compare(sym) == 0)
        {
          ui->objDataList->item(row)->setSelected(true);
          break;
        }
    }
  ui->objToolBox->setCurrentIndex(0);
}

void objecttab::selectFunction(QString sym, int line) const
{
  int rows = ui->objFunctionsTree->topLevelItemCount();

  QList<QTreeWidgetItem *> selectedItems = ui->objFunctionsTree->selectedItems();
  for (QTreeWidgetItem *item : selectedItems)
    {
      item->setSelected(false);
      item->setExpanded(false);
    }

  for (int row = 0; row < rows; ++row)
    {
      QString rowName = ui->objFunctionsTree->topLevelItem(row)->text(0);
      if (rowName.compare(sym) == 0)
        {
          ui->objFunctionsTree->topLevelItem(row)->setSelected(true);
          ui->objFunctionsTree->topLevelItem(row)->setExpanded(true);

          if (line >= 0 && line < ui->objFunctionsTree->topLevelItem(row)->childCount())
            ui->objFunctionsTree->topLevelItem(row)->child(line)->setSelected(true);

          break;
        }
    }
  ui->objToolBox->setCurrentIndex(1);
}

void objecttab::selectFunctionLine(QString sym, int line) const
{
  this->selectFunction(sym, line);
}

void objecttab::toggleHex(bool checked) const
{
  ui->objFunctionsTree->setColumnHidden(1, checked);
  ui->objFunctionsTree->setColumnHidden(2, !checked);
}

QTreeWidget *objecttab::getTree() const
{
  return ui->objFunctionsTree;
}
