/*
 * \file mapwindow.cpp
 * \author Igor Bereznyak <igor.bereznyak@gmail.com>
 * \brief An implementation of the main window widget.
 **/
#include "mainwindow.h"
#include "ui_mapeditor.h"

#include <QtGui>
#include <QSplitter>
#include <QFormLayout>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), mDontChangeTileHack(false)
{   
    map = new MapWidget;
    connect(map, SIGNAL(cellSelected()), this, SLOT(onCellSelected()));
    connect(map, SIGNAL(cellDeselected()), this, SLOT(onCellDeselected()));

    props = new QGroupBox;
    props->setTitle("Свойства");

    QFormLayout * propsLayout = new QFormLayout;
        propsLayout->addRow(createLabel("Свойства карты"));
        mapRows = new QSpinBox;
        mapCols = new QSpinBox;
        connect(mapRows, SIGNAL(valueChanged(int)), this, SLOT(onMapSizeChanged(int)));
        connect(mapCols, SIGNAL(valueChanged(int)), this, SLOT(onMapSizeChanged(int)));
        propsLayout->addRow(createLabel("Строк:"), mapRows);
        propsLayout->addRow(createLabel("Столбцов:"), mapCols);
        selectTileset = new QPushButton;
        selectTileset->setText("(нет)");
        connect(selectTileset, SIGNAL(clicked()), this, SLOT(onSelectTileset()));
        propsLayout->addRow(createLabel("Набор тайлов:"), selectTileset);
        QComboBox * scaleCombo = new QComboBox;
        scaleCombo->addItem("200%");
        scaleCombo->addItem("150%");
        scaleCombo->addItem("100%");
        scaleCombo->addItem("90%");
        scaleCombo->addItem("75%");
        scaleCombo->addItem("50%");
        scaleCombo->addItem("25%");
        scaleCombo->addItem("10%");
        scaleCombo->addItem("5%");
        scaleCombo->setCurrentIndex(2);
        connect(scaleCombo, SIGNAL(currentIndexChanged(QString)), this, SLOT(onScaleSet(QString)));
        connect(scaleCombo, SIGNAL(editTextChanged(QString)), this, SLOT(onScaleSet(QString)));
        propsLayout->addRow(createLabel("Масштаб: "), scaleCombo);

        propsLayout->addRow(createLabel("Свойства элемента"));
        tiles = new QComboBox;
        connect(tiles, SIGNAL(currentIndexChanged(int)), this, SLOT(onTileChanged(int)));
        propsLayout->addRow(createLabel("Тип"), tiles);
    props->setLayout(propsLayout);

    QSplitter * splitter = new QSplitter;
    splitter->addWidget(props);
    splitter->addWidget(map);
    setCentralWidget(splitter);
    setWindowTitle("MapEd");

    QAction * act;
    QMenu * menu;

    // for ref. see: http://www.zetcode.com/gui/qt4/menusandtoolbars/

    menu = menuBar()->addMenu("&Файл");
    act = menu->addAction("&Новый");
    act = menu->addAction("&Открыть...");
    act = menu->addAction("&Сохранить...");
    menu->addSeparator();
    act = menu->addAction("В&ыход");
    connect(act, SIGNAL(triggered()), qApp, SLOT(quit()));

    menu = menuBar()->addMenu("Справ&ка");
    act = menu->addAction("&О программе...");

    loadTileSet("/home/igor/workspace/RainbowCrash/res/drawable-nodpi/");
}

void MainWindow::onTileChanged(int indx) {
    if (mDontChangeTileHack) { // note: because i don't know who emit signal - external user or tiles->setCurrentIndex()
        mDontChangeTileHack = false;
    } else {
        qDebug() << "onTileChanged" << indx;
        map->setSelectedTile(indx);
    }
}

void MainWindow::onCellSelected() {
    qDebug() << "onCellSelected";
    mDontChangeTileHack = true;
    tiles->setCurrentIndex(map->getSelectedTile());
}

void MainWindow::onCellDeselected() {
    qDebug() << "onCellDeselected";
    mDontChangeTileHack = true;
    tiles->setCurrentIndex(-1);
}

void MainWindow::onScaleSet(QString s) {
    int scale = s.split('%')[0].toInt();
    map->setScale(((float)scale) / 100.0f);
}

void MainWindow::onMapSizeChanged(int) {
    map->setMapSize(mapRows->value(), mapCols->value());
}

void MainWindow::loadTileSet(QString const & dir) {
    if (map->loadTiles(dir)) {
        QString dir2 = dir;
        dir2.truncate(15);
        dir2 += "...";
        selectTileset->setText(dir2);
        tiles->clear();
        map->insertInto(tiles);
        tiles->setCurrentIndex(-1);
        map->update();
    }
}

void MainWindow::onSelectTileset() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Директория с тайлами"), "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    loadTileSet(dir);
}

QLabel *MainWindow::createLabel(const QString &text)
{
    QLabel *lbl = new QLabel(text);
    lbl->setAlignment(Qt::AlignHCenter | Qt::AlignCenter);
    return lbl;
}

MainWindow::~MainWindow()
{
}
