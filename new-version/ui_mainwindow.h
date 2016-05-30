/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.5.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QToolBox>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralWidget;
    QVBoxLayout *verticalLayout_2;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_2;
    QHBoxLayout *horizontalLayout;
    QCheckBox *checkHex;
    QSpacerItem *horizontalSpacer;
    QPushButton *addExe;
    QPushButton *addObj;
    QPushButton *runProj;
    QPushButton *clearProj;
    QHBoxLayout *horizontalLayout_9;
    QToolBox *exeToolBox;
    QWidget *exeDataPage;
    QHBoxLayout *horizontalLayout_6;
    QListWidget *exeDataList;
    QWidget *exeFunctionPage;
    QHBoxLayout *horizontalLayout_7;
    QTreeWidget *exeFunctionsTree;
    QTabWidget *objTabs;
    QWidget *tab;
    QHBoxLayout *horizontalLayout_5;
    QWidget *tab_2;
    QVBoxLayout *verticalLayout_3;
    QTextBrowser *infoOutput;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        MainWindow->resize(976, 640);
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        centralWidget->setMinimumSize(QSize(0, 580));
        verticalLayout_2 = new QVBoxLayout(centralWidget);
        verticalLayout_2->setSpacing(6);
        verticalLayout_2->setContentsMargins(11, 11, 11, 11);
        verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
        verticalLayout = new QVBoxLayout();
        verticalLayout->setSpacing(6);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(6);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        checkHex = new QCheckBox(centralWidget);
        checkHex->setObjectName(QStringLiteral("checkHex"));

        horizontalLayout->addWidget(checkHex);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        addExe = new QPushButton(centralWidget);
        addExe->setObjectName(QStringLiteral("addExe"));
        addExe->setStyleSheet(QLatin1String("QPushButton {\n"
"    border-width: 0;\n"
"    border-radius: 0;\n"
"    padding: 0;\n"
"}"));

        horizontalLayout->addWidget(addExe);

        addObj = new QPushButton(centralWidget);
        addObj->setObjectName(QStringLiteral("addObj"));
        addObj->setStyleSheet(QLatin1String("QPushButton {\n"
"    border-width: 0;\n"
"    border-radius: 0;\n"
"    padding: 0;\n"
"}"));

        horizontalLayout->addWidget(addObj, 0, Qt::AlignRight);

        runProj = new QPushButton(centralWidget);
        runProj->setObjectName(QStringLiteral("runProj"));
        runProj->setStyleSheet(QLatin1String("QPushButton {\n"
"    border-width: 0;\n"
"    border-radius: 0;\n"
"    padding: 0;\n"
"}"));

        horizontalLayout->addWidget(runProj);

        clearProj = new QPushButton(centralWidget);
        clearProj->setObjectName(QStringLiteral("clearProj"));
        clearProj->setStyleSheet(QLatin1String("QPushButton {\n"
"    border-width: 0;\n"
"    border-radius: 0;\n"
"    padding: 0;\n"
"}"));

        horizontalLayout->addWidget(clearProj, 0, Qt::AlignRight);


        horizontalLayout_2->addLayout(horizontalLayout);


        verticalLayout->addLayout(horizontalLayout_2);

        horizontalLayout_9 = new QHBoxLayout();
        horizontalLayout_9->setSpacing(6);
        horizontalLayout_9->setObjectName(QStringLiteral("horizontalLayout_9"));
        exeToolBox = new QToolBox(centralWidget);
        exeToolBox->setObjectName(QStringLiteral("exeToolBox"));
        exeDataPage = new QWidget();
        exeDataPage->setObjectName(QStringLiteral("exeDataPage"));
        exeDataPage->setGeometry(QRect(0, 0, 474, 324));
        horizontalLayout_6 = new QHBoxLayout(exeDataPage);
        horizontalLayout_6->setSpacing(6);
        horizontalLayout_6->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_6->setObjectName(QStringLiteral("horizontalLayout_6"));
        exeDataList = new QListWidget(exeDataPage);
        exeDataList->setObjectName(QStringLiteral("exeDataList"));
        exeDataList->setStyleSheet(QLatin1String("QListView::item:selected {\n"
"    background: #99ff99;\n"
"	color: #000;\n"
"}\n"
"QListView::item:selected:!active {\n"
"	background: #99ff99;\n"
"	color: #000;\n"
"}"));

        horizontalLayout_6->addWidget(exeDataList);

        exeToolBox->addItem(exeDataPage, QStringLiteral("Page 1"));
        exeFunctionPage = new QWidget();
        exeFunctionPage->setObjectName(QStringLiteral("exeFunctionPage"));
        exeFunctionPage->setGeometry(QRect(0, 0, 474, 324));
        horizontalLayout_7 = new QHBoxLayout(exeFunctionPage);
        horizontalLayout_7->setSpacing(6);
        horizontalLayout_7->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_7->setObjectName(QStringLiteral("horizontalLayout_7"));
        exeFunctionsTree = new QTreeWidget(exeFunctionPage);
        QTreeWidgetItem *__qtreewidgetitem = new QTreeWidgetItem();
        __qtreewidgetitem->setText(0, QStringLiteral("1"));
        exeFunctionsTree->setHeaderItem(__qtreewidgetitem);
        exeFunctionsTree->setObjectName(QStringLiteral("exeFunctionsTree"));
        exeFunctionsTree->setStyleSheet(QLatin1String("QTreeView::item:selected {\n"
"    background: #99ff99;\n"
"	color: #000;\n"
"}\n"
"QTreeView::item:selected:!active {\n"
"	background: #99ff99;\n"
"	color: #000;\n"
"}"));

        horizontalLayout_7->addWidget(exeFunctionsTree);

        exeToolBox->addItem(exeFunctionPage, QStringLiteral("Page 2"));

        horizontalLayout_9->addWidget(exeToolBox);

        objTabs = new QTabWidget(centralWidget);
        objTabs->setObjectName(QStringLiteral("objTabs"));
        tab = new QWidget();
        tab->setObjectName(QStringLiteral("tab"));
        horizontalLayout_5 = new QHBoxLayout(tab);
        horizontalLayout_5->setSpacing(6);
        horizontalLayout_5->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_5->setObjectName(QStringLiteral("horizontalLayout_5"));
        objTabs->addTab(tab, QString());
        tab_2 = new QWidget();
        tab_2->setObjectName(QStringLiteral("tab_2"));
        objTabs->addTab(tab_2, QString());

        horizontalLayout_9->addWidget(objTabs);


        verticalLayout->addLayout(horizontalLayout_9);


        verticalLayout_2->addLayout(verticalLayout);

        verticalLayout_3 = new QVBoxLayout();
        verticalLayout_3->setSpacing(6);
        verticalLayout_3->setObjectName(QStringLiteral("verticalLayout_3"));
        infoOutput = new QTextBrowser(centralWidget);
        infoOutput->setObjectName(QStringLiteral("infoOutput"));

        verticalLayout_3->addWidget(infoOutput, 0, Qt::AlignBottom);


        verticalLayout_2->addLayout(verticalLayout_3);

        MainWindow->setCentralWidget(centralWidget);

        retranslateUi(MainWindow);

        exeToolBox->setCurrentIndex(1);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "ELF Detective", 0));
#ifndef QT_NO_TOOLTIP
        checkHex->setToolTip(QApplication::translate("MainWindow", "See instruction hexcodes", 0));
#endif // QT_NO_TOOLTIP
        checkHex->setText(QApplication::translate("MainWindow", "Hexcodes", 0));
#ifndef QT_NO_TOOLTIP
        addExe->setToolTip(QApplication::translate("MainWindow", "Add executable file (CTRL+E)", 0));
#endif // QT_NO_TOOLTIP
        addExe->setText(QString());
#ifndef QT_NO_TOOLTIP
        addObj->setToolTip(QApplication::translate("MainWindow", "Add object file (CTRL+O)", 0));
#endif // QT_NO_TOOLTIP
        addObj->setText(QString());
#ifndef QT_NO_TOOLTIP
        runProj->setToolTip(QApplication::translate("MainWindow", "Run project (CTRL+R)", 0));
#endif // QT_NO_TOOLTIP
        runProj->setText(QString());
#ifndef QT_NO_TOOLTIP
        clearProj->setToolTip(QApplication::translate("MainWindow", "Clear Project (CTRL+C)", 0));
#endif // QT_NO_TOOLTIP
        clearProj->setText(QString());
        exeToolBox->setItemText(exeToolBox->indexOf(exeDataPage), QApplication::translate("MainWindow", "Page 1", 0));
        exeToolBox->setItemText(exeToolBox->indexOf(exeFunctionPage), QApplication::translate("MainWindow", "Page 2", 0));
        objTabs->setTabText(objTabs->indexOf(tab), QApplication::translate("MainWindow", "Tab 1", 0));
        objTabs->setTabText(objTabs->indexOf(tab_2), QApplication::translate("MainWindow", "Tab 2", 0));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
