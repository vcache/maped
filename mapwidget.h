#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QWidget>
#include <QtGui>
#include <QVector>
#include <QComboBox>

class MapWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MapWidget(QWidget *parent = 0);
    inline void setMapSize(int rows, int cols) { mRows = rows; mCols = cols; update(); }
    inline void setScale(float s) { mScale = s; update(); }
    bool loadTiles(QString const & dir);
    void insertInto(QComboBox * tiles);

protected:
    void paintEvent(QPaintEvent * event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    inline QPointF localToGlobal(QPointF const & local) const;
    inline QPoint getCellUnderMouse(QPointF const & mouse) const;
    inline bool isValidCell(QPoint const & cell) const;
    void wheelEvent(QWheelEvent * event);

signals:

public slots:

private:
    int mRows, mCols;
    QSize mTileSize;
    struct MapTile {
        QImage im;
        QString fileName;
        MapTile() {}
        MapTile(QString const & fname, QImage const & i): im(i), fileName(fname) {}
    };
    QVector<MapTile> mTiles;
    QPointF mViewportPos;
    QPointF mDragOffset;
    QPointF mDragOrigin;
    QPoint cellUnderMouse;
    QPoint selectedCell;
    float mScale;
};

#endif // MAPWIDGET_H
