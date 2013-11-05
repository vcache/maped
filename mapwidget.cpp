#include "mapwidget.h"
#include <QDir>

MapWidget::MapWidget(QWidget *parent) :
    QWidget(parent), mRows(0), mCols(0), mTileSize(-1, -1), mViewportPos(.0f, .0f), mCellUnderMouse(-1, -1), mSelectedCell(-1, -1), mScale(1.0f)
{
    setMouseTracking(true);
}

void MapWidget::setMapSize(int rows, int cols)
{
    QVector<int> newCells(rows * cols);
    if (!mCells.isEmpty()) {
        int i, j;
        int mrow = qMin<int>(rows, mRows);
        int mcol = qMin<int>(cols, mCols);
        for(i = 0; i < mrow; i++) {
            for(j = 0; j < mcol; j++)
                newCells[i + j * cols] = mCells[i + j * mCols];
        }
    }
    mRows = rows;
    mCols = cols;
    mCells = newCells;
    update();
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
    return QPoint(global.rx() / mTileSize.width(), global.ry() / mTileSize.height());
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
    }
}

void MapWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MidButton) {
        mViewportPos += mDragOffset;
        mDragOffset.rx() = 0;
        mDragOffset.ry() = 0;
    } else if (event->button() == Qt::RightButton) {
        QPoint cell = getCellUnderMouse(event->localPos());
        if (isValidCell(cell)) {
            mSelectedCell = cell;
            emit cellSelected();
        } else {
            mSelectedCell = QPoint(-1, -1);
            emit cellDeselected();
        }
        update();
    }
}

void MapWidget::wheelEvent(QWheelEvent * event)
{
    mScale += (((float)event->angleDelta().y()) / 500.0f);
    if (mScale < .0001f) mScale = .0001f;
    update();
}

void MapWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QRectF rectf(0, 0, width()-1, height()-1);

    painter.fillRect(rectf, Qt::black);

    painter.save();
    painter.translate(mViewportPos);
    painter.translate(mDragOffset);
    painter.scale(mScale, mScale);

    if (mTileSize.isValid()) {
        // TODO: draw tile here

        if (mSelectedCell != mCellUnderMouse && isValidCell(mCellUnderMouse)) {
            int x = mCellUnderMouse.x() * mTileSize.width();
            int y = mCellUnderMouse.y() * mTileSize.height();
            QString s;
            QTextStream(&s) << mCellUnderMouse.x() << "x" << mCellUnderMouse.y();
            painter.fillRect(x, y, mTileSize.width(), mTileSize.height(), QColor(127, 127, 255, 127));
            painter.drawText(x, y, s);

        }

        if (isValidCell(mSelectedCell))
            painter.fillRect(mSelectedCell.x() * mTileSize.width(), mSelectedCell.y() * mTileSize.height(), mTileSize.width(), mTileSize.height(), QColor(0, 255, 0, 127));

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
