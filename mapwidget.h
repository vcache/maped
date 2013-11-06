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
    void setMapSize(int rows, int cols);
    inline void setScale(float s) { mScale = s; update(); }
    bool loadTiles(QString const & dir);
    void insertInto(QComboBox * tiles);
    int getSelectedTile() const;
    void setSelectedTile(int tile);

protected:
    void paintEvent(QPaintEvent * event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    inline QPointF localToGlobal(QPointF const & local) const;
    inline QPoint getCellUnderMouse(QPointF const & mouse) const;
    inline bool isValidCell(QPoint const & cell) const { return cell.x() >= 0 && cell.y() >= 0 && cell.x() < mCols && cell.y() < mRows; }
    void wheelEvent(QWheelEvent * event);
    void clipCellCoord(QPoint & c) const;
    QRect getSelectedArea(QPoint const & fst, QPoint const & snd) const;

signals:
    void cellSelected();
    void cellDeselected();

public slots:

private:
    int mRows, mCols;
    QVector<int> mCells;

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
    QPoint mCellUnderMouse;
    QPoint * mSelectionBegin, * mSelectionEnd;
    float mScale;
};

#endif // MAPWIDGET_H
