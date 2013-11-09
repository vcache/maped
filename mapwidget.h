/*
 * \file mapwidget.h
 * \author Igor Bereznyak <igor.bereznyak@gmail.com>
 * \brief A header of the main MapEd widget.
 **/
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
    bool addTiles(QStringList const & files);
    void insertInto(QComboBox * tiles);
    int getSelectedTile() const;
    void setSelectedTile(int tile);
    bool saveMap(QString const & filename) const;
    void loadMap(QString const & filename);
    inline int getRows() const { return mRows; }
    inline int getCols() const { return mCols; }
    void eraseSelected();
    void selectAll();
    QRect getSelectedTilesCount() const;
    void startModeGrab();
    void startModeDuplicate();
    void finishSpecialMode(bool confirm);

protected:
    void paintEvent(QPaintEvent * event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent * event);
    void keyPressEvent(QKeyEvent * event);
    inline QPointF localToGlobal(QPointF const & local) const;
    inline QPoint getCellUnderMouse(QPointF const & mouse) const;
    inline bool isValidCell(QPoint const & cell) const { return cell.x() >= 0 && cell.y() >= 0 && cell.x() < mCols && cell.y() < mRows; }
    void clipCellCoord(QPoint & c) const;
    QRect getSelectedArea(QPoint const & fst, QPoint const & snd) const;

signals:
    void cellSelected();
    void cellDeselected();
    void miscellaneousNotification(QString const &);

public slots:

private:
    int mRows, mCols;
    QVector<int> mCells;

    enum EditMode { NORMAL, GRAB };
    EditMode mEditMode;

    QSize mTileSize;
    struct MapTile {
        QImage im;
        QString fileName;
        bool valid;
        MapTile(): valid(false) {}
        MapTile(QString const & fname, QImage const & i): im(i), fileName(fname), valid(true) {}
        inline bool isValid() const { return valid; }
    };
    QVector<MapTile> mTiles;
    QPointF mViewportPos;
    QPointF mDragOffset;
    QPointF mDragOrigin;
    QPoint mCellUnderMouse;
    QPoint * mSelectionBegin, * mSelectionEnd;
    QPoint mGrabOrigin;
    float mScale;
};

#endif // MAPWIDGET_H
