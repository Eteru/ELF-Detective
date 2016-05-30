/********************************************************************************
** Form generated from reading UI file 'objecttab.ui'
**
** Created by: Qt User Interface Compiler version 5.5.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_OBJECTTAB_H
#define UI_OBJECTTAB_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QToolBox>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_objecttab
{
public:
    QVBoxLayout *verticalLayout;
    QToolBox *objToolBox;
    QWidget *objData;
    QVBoxLayout *verticalLayout_2;
    QListWidget *objDataList;
    QWidget *objFunctions;
    QVBoxLayout *verticalLayout_3;
    QTreeWidget *objFunctionsTree;

    void setupUi(QWidget *objecttab)
    {
        if (objecttab->objectName().isEmpty())
            objecttab->setObjectName(QStringLiteral("objecttab"));
        objecttab->resize(400, 300);
        verticalLayout = new QVBoxLayout(objecttab);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        objToolBox = new QToolBox(objecttab);
        objToolBox->setObjectName(QStringLiteral("objToolBox"));
        objData = new QWidget();
        objData->setObjectName(QStringLiteral("objData"));
        objData->setGeometry(QRect(0, 0, 382, 220));
        verticalLayout_2 = new QVBoxLayout(objData);
        verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
        objDataList = new QListWidget(objData);
        objDataList->setObjectName(QStringLiteral("objDataList"));
        objDataList->setStyleSheet(QLatin1String("QListView::item:selected {\n"
"    background: #add8e6;\n"
"	color: #000;\n"
"}\n"
"QListView::item:selected:!active {\n"
"	background: #add8e6;\n"
"	color: #000;\n"
"}"));

        verticalLayout_2->addWidget(objDataList);

        objToolBox->addItem(objData, QStringLiteral("Page 1"));
        objFunctions = new QWidget();
        objFunctions->setObjectName(QStringLiteral("objFunctions"));
        objFunctions->setGeometry(QRect(0, 0, 382, 220));
        verticalLayout_3 = new QVBoxLayout(objFunctions);
        verticalLayout_3->setObjectName(QStringLiteral("verticalLayout_3"));
        objFunctionsTree = new QTreeWidget(objFunctions);
        QTreeWidgetItem *__qtreewidgetitem = new QTreeWidgetItem();
        __qtreewidgetitem->setText(0, QStringLiteral("1"));
        objFunctionsTree->setHeaderItem(__qtreewidgetitem);
        objFunctionsTree->setObjectName(QStringLiteral("objFunctionsTree"));
        objFunctionsTree->setStyleSheet(QLatin1String("QTreeView::item:selected {\n"
"    background: #add8e6;\n"
"	color: #000;\n"
"}\n"
"QTreeView::item:selected:!active {\n"
"	background: #add8e6;\n"
"	color: #000;\n"
"}"));

        verticalLayout_3->addWidget(objFunctionsTree);

        objToolBox->addItem(objFunctions, QStringLiteral("Page 2"));

        verticalLayout->addWidget(objToolBox);


        retranslateUi(objecttab);

        objToolBox->setCurrentIndex(1);


        QMetaObject::connectSlotsByName(objecttab);
    } // setupUi

    void retranslateUi(QWidget *objecttab)
    {
        objecttab->setWindowTitle(QApplication::translate("objecttab", "Form", 0));
        objToolBox->setItemText(objToolBox->indexOf(objData), QApplication::translate("objecttab", "Page 1", 0));
        objToolBox->setItemText(objToolBox->indexOf(objFunctions), QApplication::translate("objecttab", "Page 2", 0));
    } // retranslateUi

};

namespace Ui {
    class objecttab: public Ui_objecttab {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_OBJECTTAB_H
