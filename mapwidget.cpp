/*
 * \file mapwidget.cpp
 * \author Igor Bereznyak <igor.bereznyak@gmail.com>
 * \brief An implementation of the main MapEd widget.
 *
 * TODO features:
 *	- sprites list: remove
 *  - refact selArea
 *  - filtering?
 *  - duplicate region
 *  - grab region
 *  - static objects
 **/
#include "mapwidget.h"

#include <QMessageBox>
#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>

MapWidget::MapWidget(QWidget *parent) :
    QWidget(parent),
    mRows(0),
    mCols(0),
    mEditMode(NORMAL),
    mTileSize(-1, -1),
    mViewportPos(.0f, .0f),
    mCellUnderMouse(-1, -1),
    mSelectionBegin(NULL),
    mSelectionEnd(NULL),
    mScale(1.0f)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

void MapWidget::setMapSize(int rows, int cols)
{
    if (rows == mRows && cols == mCols) return;

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

bool MapWidget::saveMap(QString const & filename) const
{
    QJsonObject jsn_map;
    QJsonObject jsn_tiles;
    QJsonArray jsn_cells;

    for(int i = 0; i < mTiles.size(); ++i) {
        QString index = QString("%1").arg(i);
        jsn_tiles.insert(index, QJsonValue(mTiles[i].fileName));
    }

    for(QVector<int>::const_iterator it = mCells.begin(); it != mCells.end(); ++it)
        jsn_cells.push_back(QJsonValue(*it));

    jsn_map.insert("rows", QJsonValue(mRows));
    jsn_map.insert("cols", QJsonValue(mCols));
    jsn_map.insert("tiles", jsn_tiles);
    jsn_map.insert("cells", jsn_cells);

    QJsonDocument jsn_doc(jsn_map);
    QByteArray jsn_out = filename.endsWith("json") ? jsn_doc.toJson() : jsn_doc.toBinaryData();

    QFile qf(filename);
    if (!qf.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    if (-1 == qf.write(jsn_out)) {
        qf.close();
        return false;
    }

    qf.close();
    return true;
}

void MapWidget::loadMap(QString const & filename) {
    // read file

    QFile qf(filename);
    if (!qf.open(QIODevice::ReadOnly)) throw qf.errorString();

    QByteArray jsn_in = qf.readAll();
    qf.close();
    if (jsn_in.isEmpty()) throw QString("Empty file");

    // parse file

    QJsonDocument jsn_doc;
    if (filename.endsWith("json")) {
        jsn_doc = QJsonDocument::fromJson(jsn_in);
    } else {
        jsn_doc = QJsonDocument::fromBinaryData(jsn_in);
    }
    if (jsn_doc.isNull()) throw QString("Failed to validate JSON data");
    if (!jsn_doc.isObject()) throw QString("Top level JSON value is not an object");
    QJsonObject jsn_map = jsn_doc.object();

    // load generic map info

    QJsonObject::const_iterator it;
    it = jsn_map.find("rows");
    if (it == jsn_map.end()) throw QString("File not contains 'rows'");
    if (!it.value().isDouble()) throw QString("'rows' is not a number");
    int rows = int(it.value().toDouble());

    it = jsn_map.find("cols");
    if (it == jsn_map.end()) throw QString("File not contains 'cols'");
    if (!it.value().isDouble()) throw QString("'cols' is not a number");
    int cols = int(it.value().toDouble());

    // load tiles FIXME: each key must be in [0; jsn_tiles.size()-1] or assertion happens

    it = jsn_map.find("tiles");
    if (it == jsn_map.end()) throw QString("File not contains 'tiles'");
    if (!it.value().isObject()) throw QString("'cells' is not an object");
    QJsonObject jsn_tiles = it.value().toObject();
    QVector<MapTile> tiles(jsn_tiles.size());
    QSize tileSize(-1, -1);
    int prev_tile_id = -1;
    for(QJsonObject::const_iterator i = jsn_tiles.begin(); i != jsn_tiles.end(); ++i) {
        int tile_id = i.key().toInt();
        if (tile_id - prev_tile_id != 1) throw QString("Non-monotonic tile keys");
        if (!i.value().isString()) throw QString("Incorrect tile's path");
        QImage im(i.value().toString());
        if (im.isNull()) throw QString("Can't open image");
        if (tileSize.isEmpty()) {
            tileSize = im.size();
        } else if (tileSize != im.size()) {
            throw QString("Tile's dimensions not same");
        }
        tiles[tile_id] = MapTile(i.value().toString(), im);
        prev_tile_id = tile_id;
    }

    // load cells

    QVector<int> cells(cols * rows);
    it = jsn_map.find("cells");
    if (it == jsn_map.end()) throw QString("File not contains 'cells'");
    if (!it.value().isArray()) throw QString("'cells' is not an array");
    QJsonArray jsn_cells = it.value().toArray();
    if (jsn_cells.size() != cols * rows) throw QString("Incorrect 'cells' length");
    int index = -1;
    for(QJsonArray::const_iterator i = jsn_cells.begin(); i != jsn_cells.end(); ++i) {
        if (!(*i).isDouble()) throw QString("Not number in 'cells'");
        int val = int((*i).toDouble());
        if (val < -1 || val >= tiles.size()) throw QString("Incorrect range in 'cells'");
        if (val > 0 && false == tiles[val].isValid()) throw QString("Incorrect link in 'cells'");
        cells[++index] = val;
    }

    // if everything is fine
    mCells = cells;
    mTiles = tiles;
    mRows = rows;
    mCols = cols;
    mTileSize = tileSize;
    mViewportPos = QPointF(.0f, .0f);
    mCellUnderMouse = QPoint(-1, -1);
    mSelectionBegin = NULL;
    mSelectionEnd = NULL;
    mScale = 1.0f;

    update();
}

bool MapWidget::addTiles(QStringList const & files)
{
    QSize tileSize = mTileSize;
    QVector<MapTile> tiles;

    QStringList list = files;
    for (QStringList::Iterator it = list.begin(); it != list.end(); ++it) {
        QImage im(*it);

        if (!im.isNull()) {
            qDebug() << "Loading tile: " << *it << " / " << im.size();
            if (tileSize.isEmpty()) {
                tileSize = im.size();
            } else if (tileSize != im.size()) {
                QMessageBox msg;
                msg.setInformativeText("Operation canceled due to errors");
                QString s = QString("Tile in file %1 have dimensions %2x%3 while expected tile size is %4x%5").arg(*it).arg(im.size().width()).arg(im.size().height()).arg(tileSize.width()).arg(tileSize.height());
                msg.setText(s);
                msg.setIcon(QMessageBox::Critical);
                msg.exec();
                return false; // to do: maybe just throw an exception?
            }
            tiles << MapTile(*it, im);
        } else {
            QMessageBox msg(QMessageBox::Warning, "Cannot read file", "Failed to read file " + *it);
            msg.exec();
        }
    }

    // If succeed
    if (mTileSize.isEmpty())
        mTileSize = tileSize;
    mTiles += tiles;

    return true;
}

void MapWidget::eraseSelected()
{
    if (!mSelectionBegin || !mSelectionEnd) return;

    int i, j;
    QRect selArea = getSelectedArea(*mSelectionBegin, *mSelectionEnd);
    for(i = selArea.left(); i <= selArea.right(); i++) {
        for(j = selArea.top(); j <= selArea.bottom(); j++) {
            mCells[i + j * mCols] = -1;
        } // for j
    } // for i
    update();
}

void MapWidget::selectAll() {
    QPoint b(0,0);
    QPoint e(mCols - 1, mRows - 1);

    if (mSelectionBegin && mSelectionEnd && *mSelectionBegin == b && *mSelectionEnd == e) {
        delete mSelectionBegin;
        delete mSelectionEnd;
        mSelectionBegin = mSelectionEnd = NULL;
        emit cellDeselected();
    } else {
        if (mSelectionBegin) delete mSelectionBegin;
        if (mSelectionEnd) delete mSelectionEnd;
        mSelectionBegin = new QPoint(b);
        mSelectionEnd = new QPoint(e);
        emit cellSelected();
    }
    update();
}

QRect MapWidget::getSelectedTilesCount() const
{
    return (mSelectionBegin && mSelectionEnd) ? getSelectedArea(*mSelectionBegin, *mSelectionEnd) : QRect(0,0,0,0);
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

/*!
 * \brief MapWidget::startModeGrab
 * \todo Maybe just throw an exception?
 */
void MapWidget::startModeGrab()
{
    if (mEditMode != NORMAL) {
        miscellaneousNotification("Cannot grab from this mode");
        return;
    }

    if (!mSelectionBegin || !mSelectionEnd) {
        emit miscellaneousNotification("Nothing selected to grab");
        return;
    }

    QRect gloablOrigin = getSelectedArea(*mSelectionBegin, *mSelectionEnd);
    //mGrabOrigin = getCellUnderMouse() - TODO HERE;
    mEditMode = GRAB;
}

void MapWidget::startModeDuplicate()
{
    emit miscellaneousNotification("Not yet implemented");
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

void MapWidget::insertInto(QComboBox * tiles) {
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

void MapWidget::keyPressEvent(QKeyEvent * event) {
    if (event->key() == Qt::Key_X && event->modifiers() == Qt::NoModifier) {
        eraseSelected();
        update();
    } else if (event->key() == Qt::Key_A && event->modifiers() == Qt::NoModifier) {
        selectAll();
    } else if (event->key() == Qt::Key_G && event->modifiers() == Qt::NoModifier) {
        startModeGrab();
    } else if (event->key() == Qt::Key_D && event->modifiers() == Qt::ShiftModifier) {
        startModeDuplicate();
    } else if (event->key() == Qt::Key_Escape && event->modifiers() == Qt::ShiftModifier) {
        mEditMode = NORMAL;
    } else if (event->key() == Qt::Key_Enter && event->modifiers() == Qt::ShiftModifier) {
        //finishSpecialMode();
    } else
        QWidget::keyPressEvent(event);
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

/*!
 * \brief Get correct area (clipped and sorted) of rectangular selection. fst and snd may come in any order and may be out-of-boundary.
 * \param fst First point of rectange (in "row-col" units).
 * \param snd Second point of rectange (in "row-col" units).
 * \return Nice and correct area in table.
 */
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

    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    painter.fillRect(rectf, Qt::black);

    painter.save();
    painter.translate(vpTopLeft);
    painter.scale(mScale, mScale);

    if (mTileSize.isValid()) {
        int i, j, tile_indx;
        QPoint visAreaBeg = getCellUnderMouse(QPoint(0,0)); // TODO: this is just viewport coords / width-height!
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

        // highlight cursor
        switch (mEditMode) {
        case NORMAL:
            if (mSelectionBegin && !mSelectionEnd) {
                QRect frame = getSelectedArea(*mSelectionBegin, mCellUnderMouse);
                painter.fillRect(
                    frame.left() * mTileSize.width(),
                    frame.top() * mTileSize.height(),
                    frame.width() * mTileSize.width(),
                    frame.height() * mTileSize.height(),
                    QColor(127, 127, 255, 50));
            } else if (isValidCell(mCellUnderMouse)) {
                int x = mCellUnderMouse.x() * mTileSize.width();
                int y = mCellUnderMouse.y() * mTileSize.height();
                painter.fillRect(x, y, mTileSize.width(), mTileSize.height(), QColor(127, 127, 255, 50));
            }
            break;

        case GRAB:
            //
            break;
        }

        // highlight selected
        if (mSelectionBegin && mSelectionEnd) {
            QRect frame = getSelectedArea(*mSelectionBegin, *mSelectionEnd);
            painter.fillRect(
                frame.left() * mTileSize.width(),
                frame.top() * mTileSize.height(),
                frame.width() * mTileSize.width(),
                frame.height() * mTileSize.height(),
                QColor(0, 255, 0, 75));
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
