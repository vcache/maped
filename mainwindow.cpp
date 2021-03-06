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
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{   
    /* Map widget */

    map = new MapWidget;
    connect(map, SIGNAL(cellSelected()), this, SLOT(onCellSelected()));
    connect(map, SIGNAL(cellDeselected()), this, SLOT(onCellDeselected()));
    connect(map, SIGNAL(miscellaneousNotification(QString const&)), this, SLOT(onMiscNotify(QString const&)));

    /* Properites bar */

    props = new QGroupBox;
    props->setTitle("Properties");

    QFormLayout * propsLayout = new QFormLayout;
        propsLayout->addRow(createLabel("Map properties"));
        mapRows = new QSpinBox;
        mapCols = new QSpinBox;
        connect(mapRows, SIGNAL(valueChanged(int)), this, SLOT(onMapSizeChanged(int)));
        connect(mapCols, SIGNAL(valueChanged(int)), this, SLOT(onMapSizeChanged(int)));
        propsLayout->addRow(createLabel("Rows:"), mapRows);
        propsLayout->addRow(createLabel("Columns:"), mapCols);
        selectTileset = new QPushButton;
        selectTileset->setText("Add tiles...");
        connect(selectTileset, SIGNAL(clicked()), this, SLOT(onSelectTileset()));
        propsLayout->addRow(createLabel("Tile set:"), selectTileset);
        scaleCombo = new QComboBox;
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
        propsLayout->addRow(createLabel("Scale: "), scaleCombo);

        propsLayout->addRow(createLabel("Cell properties"));
        tiles = new QComboBox;
        connect(tiles, SIGNAL(currentIndexChanged(int)), this, SLOT(onTileChanged(int)));
        propsLayout->addRow(createLabel("Type:"), tiles);
    props->setLayout(propsLayout);

    QSplitter * splitter = new QSplitter;
    splitter->addWidget(props);
    splitter->addWidget(map);
    setCentralWidget(splitter);
    setWindowTitle("MapEd");

    /* Status bar */

    status = new QStatusBar;
    status->addPermanentWidget(lblSelected = new QLabel);
    lblSelected->setText("None selected");
    setStatusBar(status);

    /* Main menu */

    QAction * act;
    QMenu * menu;

    // for ref. see: http://www.zetcode.com/gui/qt4/menusandtoolbars/

    menu = menuBar()->addMenu("&File");
    act = menu->addAction("&New");
    act = menu->addAction("&Open...");
    connect(act, SIGNAL(triggered()), this, SLOT(onOpenRequest()));
    act = menu->addAction("&Save...");
    connect(act, SIGNAL(triggered()), this, SLOT(onSaveRequest()));
    menu->addSeparator();
    act = menu->addAction("&Quit");
    connect(act, SIGNAL(triggered()), qApp, SLOT(quit()));

    menu = menuBar()->addMenu("&Help");
    act = menu->addAction("&About...");
}

void MainWindow::onOpenRequest()
{
    QString fname = QFileDialog::getOpenFileName(this, "Select file", "", "JSON plain-text (*.json);; Binary JSON (*.bson)");
    try {
        map->loadMap(fname);
        disconnect(tiles, SIGNAL(currentIndexChanged(int)), this, 0);
        disconnect(mapRows, SIGNAL(valueChanged(int)), this, 0);
        disconnect(mapCols, SIGNAL(valueChanged(int)), this, 0);
        tiles->clear();
        map->insertInto(tiles);
        tiles->setCurrentIndex(-1);
        mapRows->setValue(map->getRows());
        mapCols->setValue(map->getCols());
        scaleCombo->setCurrentIndex(2);
        connect(tiles, SIGNAL(currentIndexChanged(int)), this, SLOT(onTileChanged(int)));
        connect(mapRows, SIGNAL(valueChanged(int)), this, SLOT(onMapSizeChanged(int)));
        connect(mapCols, SIGNAL(valueChanged(int)), this, SLOT(onMapSizeChanged(int)));
    } catch (QString & s) {
        QMessageBox msg(QMessageBox::Critical, "Failed to open map", s);
        msg.exec();
    }
}

void MainWindow::onSaveRequest()
{
    QString fname = QFileDialog::getSaveFileName(this, "Select file", "", "JSON plain-text (*.json);; Binary JSON (*.bson)");
    if (!map->saveMap(fname)) {
        QMessageBox msg(QMessageBox::Critical, "Failed to save map", "TODO HERE");
        msg.exec();
    }
}

void MainWindow::onMiscNotify(QString const & msg) {
    status->showMessage(msg, 2000);
}

void MainWindow::onTileChanged(int indx) {
    map->setSelectedTile(indx);
}

void MainWindow::onCellSelected() {
    disconnect(tiles, SIGNAL(currentIndexChanged(int)), this, 0);
    tiles->setCurrentIndex(map->getSelectedTile());
    QRect sel = map->getSelectedTilesCount();
    QString s = QString("Selected %1 cells (%2X%3)").arg(sel.width() * sel.height()).arg(sel.width()).arg(sel.height());
    lblSelected->setText(s);
    connect(tiles, SIGNAL(currentIndexChanged(int)), this, SLOT(onTileChanged(int)));
}

void MainWindow::onCellDeselected() {
    disconnect(tiles, SIGNAL(currentIndexChanged(int)), this, 0);
    tiles->setCurrentIndex(-1);
    lblSelected->setText("None selected");
    connect(tiles, SIGNAL(currentIndexChanged(int)), this, SLOT(onTileChanged(int)));
}

void MainWindow::onScaleSet(QString s) {
    int scale = s.split('%')[0].toInt();
    map->setScale(((float)scale) / 100.0f);
}

void MainWindow::onMapSizeChanged(int) {
    map->setMapSize(mapRows->value(), mapCols->value());
}

void MainWindow::loadTileSet(QStringList const & files) {
    if (map->addTiles(files)) {
        map->insertInto(tiles);
        tiles->setCurrentIndex(-1);
        map->update();
    }
}

void MainWindow::onSelectTileset() {
    QStringList files = QFileDialog::getOpenFileNames(this, "Select one or more files to open", "", "Images (*.png *.xpm *.jpg *.bmp *.jpeg)");
    loadTileSet(files);
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
