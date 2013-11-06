/*
 * \file mapwidget.cpp
 * \author Igor Bereznyak <igor.bereznyak@gmail.com>
 * \brief An implementation of the main MapEd widget.
 *
 * TODO features:
 *  - duplicate region
 *  - grab region
 *  - cleanup region (fill -1)
 *
 **/
#include "mapwidget.h"
#include <QDir>

MapWidget::MapWidget(QWidget *parent) :
    QWidget(parent), mRows(0), mCols(0), mTileSize(-1, -1), mViewportPos(.0f, .0f), mCellUnderMouse(-1, -1), mSelectionBegin(NULL), mSelectionEnd(NULL), mScale(1.0f)
{
    setMouseTracking(true);
}

void MapWidget::setMapSize(int rows, int cols)
{
    QVector<int> newCells(rows * cols);
    newCells.fill(-1);
    if (!mCells.isEmpty()) {
        int i, j;
        int mrow = qMin<int>(rows, mRows);
        int mcol = qMin<int>(cols, mCols);
        for(i = 0; i < mcol; i++) {
            for(j = 0; j < mrow; j++)
                newCells[i + j * cols] = mCells[i + j * mCols];
        }
    }
    mCells = newCells;
    mRows = rows;
    mCols = cols;
    update();
}

int MapWidget::getSelectedTile() const
{
    if (mSelectionBegin && mSelectionEnd) {
        int common_tile_id = -1, tile_id, i, j;
        bool have_first = false;
        QRect selArea = getSelectedArea(*mSelectionBegin, *mSelectionEnd);
        for(i = selArea.left(); i <= selArea.right(); i++) {
            for(j = selArea.top(); j <= selArea.bottom(); j++) {
                tile_id = mCells[i + j * mCols];
                if (have_first) {
                    if (tile_id != common_tile_id)return -1;
                } else {
                    have_first = true;
                    common_tile_id = tile_id;
                }
            } // for j
        } // for i
        return common_tile_id;
    } // if selected
    return -1;
}

void MapWidget::setSelectedTile(int tile)
{
    if (mSelectionBegin && mSelectionEnd) {
        int i, j;
        QRect selArea = getSelectedArea(*mSelectionBegin, *mSelectionEnd);
        for(i = selArea.left(); i <= selArea.right(); i++) {
            for(j = selArea.top(); j <= selArea.bottom(); j++) {
                mCells[i + j * mCols] = tile;
            } // for j
        } // for i
        update();
    } // if (selected)
}

bool MapWidget::loadTiles(QString const & dir)
{
    QSize tileSize(-1, -1);
    QVector<MapTile> tiles;

    QDir d(dir);
    d.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    QStringList filters;
    filters << "*.png" << "*.bmp" << "*.jpg" << "*.jpeg";
    d.setNameFilters(filters);

    QFileInfoList list = d.entryInfoList();
    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);
        QImage im(fileInfo.absoluteFilePath());

        if (!im.isNull()) {
            qDebug() << "Loading tile: " << fileInfo.absoluteFilePath() << " / " << im.size();
            if (tileSize.isEmpty()) {
                tileSize = im.size();
            } else if (tileSize != im.size()) {
                qDebug() << "Non-monotonic tile sizes: " << im.size() << "; while expecting: " << tileSize;
                return false;
            }
            tiles << MapTile(fileInfo.fileName(), im);
        } else
            qDebug() << "Cannot read file " << fileInfo.absoluteFilePath();
    }

    // If succeed
    mTileSize = tileSize;
    mTiles = tiles;

    return true;
}

void MapWidget::insertInto(QComboBox * tiles) {
    qDebug() << mTiles.size();
    for(int i = 0; i < mTiles.size(); i++) {
        tiles->addItem(QIcon(QPixmap::fromImage(mTiles[i].im)), mTiles[i].fileName);
    }
}

QPointF MapWidget::localToGlobal(QPointF const & local) const
{
    return local - (mViewportPos + mDragOffset);
}

QPoint MapWidget::getCellUnderMouse(QPointF const & mouse) const
{
    QPointF global = localToGlobal(mouse);
    return QPoint(
        floor(global.x() / (((float)mTileSize.width()) * mScale)),
        floor(global.y() / (((float)mTileSize.height()) * mScale)));
}

void MapWidget::mouseMoveEvent(QMouseEvent * event)
{
    if (event->buttons() & Qt::MidButton) {
        mDragOffset = event->localPos() - mDragOrigin;
        update();
    }

    QPoint cell = getCellUnderMouse(event->localPos());
    if (cell != mCellUnderMouse) {
        mCellUnderMouse = cell;
        update();
    }

}

void MapWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MidButton) {
        mDragOrigin = event->localPos();
    } else if (event->button() == Qt::RightButton) {
        if (mSelectionBegin) delete mSelectionBegin;
        mSelectionBegin = new QPoint;
        *mSelectionBegin = getCellUnderMouse(event->localPos());

        if (mSelectionEnd) {
            delete mSelectionEnd;
            mSelectionEnd = NULL;
        }
    }
}

void MapWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MidButton) {
        mViewportPos += mDragOffset;
        mDragOffset.rx() = 0;
        mDragOffset.ry() = 0;
    } else if (event->button() == Qt::RightButton) {
        QPoint tmp = getCellUnderMouse(event->localPos());

        if (mSelectionEnd) {
            delete mSelectionEnd;
            mSelectionEnd = NULL;
        }

        if (mSelectionBegin) {
            if (!isValidCell(tmp) && tmp == *mSelectionBegin) {
                delete mSelectionBegin;
                mSelectionBegin = NULL;
                emit cellDeselected();
            } else {
                mSelectionEnd = new QPoint;
                *mSelectionEnd = tmp;
                clipCellCoord(*mSelectionBegin);
                clipCellCoord(*mSelectionEnd);
                emit cellSelected();
            }
            update();
        }
    }
}

void MapWidget::wheelEvent(QWheelEvent * event)
{
    if (event->angleDelta().y() > 0) mScale /= 1.1; else mScale *= 1.1;
    update();
}

void MapWidget::clipCellCoord(QPoint & c) const {
    if (c.x() <= 0) {
        c.setX(0);
    } else if (c.x() >= mCols) {
        c.setX(mCols - 1);
    }

    if (c.y() <= 0) {
        c.setY(0);
    } else if (c.y() >= mRows) {
        c.setY(mRows - 1);
    }
}

QRect MapWidget::getSelectedArea(QPoint const & fst, QPoint const & snd) const
{
    QRect frame;

    frame.setLeft(qMin<int>(fst.x(), snd.x()));
    if (frame.left() <= 0)
        frame.setLeft(0);
    else if (frame.left() >= mCols)
        frame.setLeft(mCols - 1);

    frame.setTop(qMin<int>(fst.y(), snd.y()));
    if (frame.top() <= 0)
        frame.setTop(0);
    else if (frame.top() >= mRows)
        frame.setTop(mRows - 1);

    frame.setRight(frame.left() + qAbs(fst.x() - snd.x()));
    if (frame.right() <= 0)
        frame.setRight(0);
    else if (frame.right() >= mCols)
        frame.setRight(mCols - 1);

    frame.setBottom(frame.top() + qAbs(fst.y() - snd.y()));
    if (frame.bottom() <= 0)
        frame.setBottom(0);
    else if (frame.bottom() >= mRows)
        frame.setBottom(mRows - 1);

    return frame;
}

void MapWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QRectF rectf(0, 0, width(), height());
    QPointF vpTopLeft = mViewportPos + mDragOffset;

    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(rectf, Qt::black);

    painter.save();
    painter.translate(vpTopLeft);
    painter.scale(mScale, mScale);

    if (mTileSize.isValid()) {
        int i, j, tile_indx;
        QPoint visAreaBeg = getCellUnderMouse(QPoint(0,0));
        clipCellCoord(visAreaBeg);
        QPoint visAreaEnd = getCellUnderMouse(visAreaBeg + QPoint(this->width(), this->height()));
        clipCellCoord(visAreaEnd);

        for(i = visAreaBeg.x(); i <= visAreaEnd.x(); i++) {
            for(j = visAreaBeg.y(); j <= visAreaEnd.y(); j++) {
                tile_indx = mCells.at(i + j * mCols);
                if (tile_indx >= 0)
                    painter.drawImage(i * mTileSize.width(), j * mTileSize.height(), mTiles[tile_indx].im);
            }
        }

        if (mSelectionBegin && !mSelectionEnd) {
            QRect frame = getSelectedArea(*mSelectionBegin, mCellUnderMouse);
            painter.fillRect(
                frame.left() * mTileSize.width(),
                frame.top() * mTileSize.height(),
                frame.width() * mTileSize.width(),
                frame.height() * mTileSize.height(),
                QColor(127, 127, 255, 127));
        } else if (isValidCell(mCellUnderMouse)) {
            int x = mCellUnderMouse.x() * mTileSize.width();
            int y = mCellUnderMouse.y() * mTileSize.height();
            painter.fillRect(x, y, mTileSize.width(), mTileSize.height(), QColor(127, 127, 255, 127));
        }

        if (mSelectionBegin && mSelectionEnd) {
            QRect frame = getSelectedArea(*mSelectionBegin, *mSelectionEnd);
            painter.fillRect(
                frame.left() * mTileSize.width(),
                frame.top() * mTileSize.height(),
                frame.width() * mTileSize.width(),
                frame.height() * mTileSize.height(),
                QColor(0, 255, 0, 127));
        }

        painter.setPen(Qt::red);
        painter.drawRect(-1, -1, mCols * mTileSize.width(), mRows * mTileSize.height());
    } else {
        painter.setPen(Qt::red);
        painter.drawLine(-10, 0, 10, 0);
        painter.drawLine(0, -10, 0, 10);
    }
    painter.restore();

    painter.setPen(Qt::green);
    painter.drawRect(rectf);
}
