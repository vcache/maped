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
    QMainWindow(parent)
{   
    map = new MapWidget;
    connect(map, SIGNAL(cellSelected()), this, SLOT(onCellSelected()));
    connect(map, SIGNAL(cellDeselected()), this, SLOT(onCellDeselected()));

    props = new QGroupBox;
    props->setTitle("Свойства");

    propsLayout = new QVBoxLayout;
        QFormLayout * locationPropsLayout = new QFormLayout;
        locationPropsLayout->addRow(createLabel("Свойства карты"));
        mapRows = new QSpinBox;
        mapCols = new QSpinBox;
        connect(mapRows, SIGNAL(valueChanged(int)), this, SLOT(onMapSizeChanged(int)));
        connect(mapCols, SIGNAL(valueChanged(int)), this, SLOT(onMapSizeChanged(int)));
        locationPropsLayout->addRow(createLabel("Строк:"), mapRows);
        locationPropsLayout->addRow(createLabel("Столбцов:"), mapCols);
        selectTileset = new QPushButton;
        selectTileset->setText("(нет)");
        connect(selectTileset, SIGNAL(clicked()), this, SLOT(onSelectTileset()));
        locationPropsLayout->addRow(createLabel("Набор тайлов:"), selectTileset);
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
        locationPropsLayout->addRow(createLabel("Масштаб: "), scaleCombo);
        propsLayout->addLayout(locationPropsLayout);
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
}

void MainWindow::onCellSelected() {
    QFormLayout * tilePropsLayout = new QFormLayout;

    tilePropsLayout->addRow(createLabel("Свойства элемента"));
    tiles = new QComboBox;
    tilePropsLayout->addRow(createLabel("Тип"), tiles);

    propsLayout->addLayout(tilePropsLayout);
}

void MainWindow::onCellDeselected() {

}


void MainWindow::onScaleSet(QString s) {
    int scale = s.split('%')[0].toInt();
    map->setScale(((float)scale) / 100.0f);
}

void MainWindow::onMapSizeChanged(int) {
    map->setMapSize(mapRows->value(), mapCols->value());
}

void MainWindow::onSelectTileset() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Директория с тайлами"), "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (map->loadTiles(dir)) {
        dir.truncate(15);
        dir += "...";
        selectTileset->setText(dir);
        tiles->clear();
        map->insertInto(tiles);
        map->update();
    }
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
