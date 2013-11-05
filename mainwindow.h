#ifndef MAPEDITOR_H
#define MAPEDITOR_H

#include <QtGui>
#include <QMainWindow>
#include <QLabel>
#include <QGroupBox>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QVBoxLayout>
#include "mapwidget.h"

namespace Ui {
class MapEditor;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
//    bool event(QEvent *event);

private:
    MapWidget *map;
    QGroupBox *props;
    QComboBox *tiles;
    QSpinBox *mapRows;
    QSpinBox *mapCols;
    QPushButton *selectTileset;
    QVBoxLayout *propsLayout;
    QLabel *createLabel(const QString &text);

protected slots:
    void onSelectTileset();
    void onMapSizeChanged(int);
    void onScaleSet(QString);
    void onCellSelected();
    void onCellDeselected();
};

#endif // MAPEDITOR_H
