#include "packagetreeview.h"

#include "desktopfilemodel.h"

#include "../../dde-file-manager-lib/shutil/fileutils.h"
#include "../../dde-file-manager-lib/app/filesignalmanager.h"
#include "../../dde-file-manager-lib/interfaces/dfmevent.h"
#include "../../dde-file-manager-lib/interfaces/dfileservices.h"
#include <singleton.h>
#include "../../dde-file-manager-lib/app/define.h"
#include "../../dde-file-manager-lib/models/desktopfileinfo.h"
#include "dfileinfo.h"

#include <QAbstractItemModel>
#include <QFileInfo>
#include <QFileIconProvider>
#include <QDateTime>
#include <QString>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QVariant>
#include <QScrollBar>
#include <QHeaderView>
#include <QMouseEvent>
#include <QGuiApplication>
#include <QClipboard>
#include <QMimeData>
#include <QMenu>
#include <QTimer>
#include <QDebug>

class Item {
public:
    Item() : p(nullptr) {}

    QString name;
    QIcon icon;
    QString time;
    QString size;
    QString type;
    bool isDir;
    QString path;
    QString parentDir;

    Item *p;
    QList<Item*> children;
};

QString formatNetworkSize(qint64 size) {
    if(size < 1024) return QString("%1B").arg(size);
    if(size < 1024 * 1024) return QString("%1KB").arg(size / 1024., 1, 'f', 2);
    if(size < 1024 * 1024 * 1024) return QString("%1MB").arg(size / 1024. / 1024, 1, 'f', 2);
    if(size < (float)1024 * 1024 * 1024 * 1024) return QString("%1GB").arg(size / 1024. / 1024 / 1024, 1, 'f', 2);
    return QString("%1TB").arg(size / 1024. / 1024 / 1024 / 1024, 1, 'f', 2);
}

void deleteItem(Item *item) {
    for(auto i : item->children)
        deleteItem(i);
    delete item;
}

class DesktopTreeModel : public QAbstractItemModel {
    Q_OBJECT
public:

    enum DesktopRole {
        IconRole = Qt::UserRole + 1,
        TypeRole = Qt::UserRole + 2,
        PathRole = Qt::UserRole + 3,
        ParentRole = Qt::UserRole + 4
    };

    DesktopTreeModel(QObject *parent = nullptr) : QAbstractItemModel(parent), rootItem(new Item) {

    }

    void initData(QStringList files) {
        int c=0, s=0;
        if(!files.isEmpty()) {
            Item *root=new Item;
            QMap<QString, Item*> map;
            map.insert("/", root);

            for(auto f : files) {
                if(f == "/.") continue;
                Item *item = new Item;
                DFileInfo info(f);
                item->name = info.fileName();
                item->icon = info.isDesktopFile() ? DesktopFileInfo(info.fileUrl()).fileIcon() : info.fileIcon();
                item->size = formatNetworkSize(info.size());
                item->time = info.lastModified().toString("yyyy/MM/dd hh:mm:ss");
                item->isDir = info.isDir();
                item->path = info.absoluteFilePath();
                item->parentDir = info.absolutePath();
                item->type = info.mimeTypeDisplayName();

                map.value(info.absolutePath())->children.append(item);
                item->p = map.value(info.absolutePath());
                if(info.isDir())
                    map.insert(f, item);
                else {
                    c++;
                    s += info.size();
                }
            }

            beginInsertRows(QModelIndex(), 0, root->children.size()-1);
            Item *old = rootItem;
            rootItem = root;
            deleteItem(old);
            endInsertRows();
        }
        emit finishLoad();
        if(c>0) emit message(QString("包内共 %1 个文件, 总大小 %2").arg(c).arg(formatNetworkSize(s)));
    }

    void reset() {
        if(!rootItem->children.isEmpty()) {
            beginResetModel();
            deleteItem(rootItem);
            rootItem = new Item;
            endResetModel();
        }
    }

    int	columnCount(const QModelIndex &parent = QModelIndex()) const override {
        Q_UNUSED(parent)
        return 4;
    }
    int	rowCount(const QModelIndex &parent = QModelIndex()) const override {
        if(parent.isValid())
            return static_cast<Item*>(parent.internalPointer())->children.at(parent.row())->children.count();
        else
            return rootItem->children.count();

    }

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override {
        if(!parent.isValid() && row < rootItem->children.count())
            return createIndex(row, column, rootItem);
        else if(parent.isValid())
            return createIndex(row, column, static_cast<Item*>(parent.internalPointer())->children.at(parent.row()));
        return QModelIndex();
    }

    QModelIndex	parent(const QModelIndex &index) const override {
        if(index.isValid()) {
            Item *item =  static_cast<Item*>(index.internalPointer());
            if(item != rootItem)
                return createIndex(item->p->children.indexOf(item), 0, item->p);
        }
        return QModelIndex();

    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override {
        if(orientation == Qt::Horizontal) {
            switch (role) {
            case Qt::DisplayRole:
                if(section == 0) return QVariant::fromValue(QString("名称"));
                else if(section == 1) return QVariant::fromValue(QString("最近修改时间"));
                else if(section == 2) return QVariant::fromValue(QString("大小"));
                else if(section == 3) return QVariant::fromValue(QString("类型"));
                break;
            default:
                break;

            }
        }
        return QAbstractItemModel::headerData(section, orientation, role);
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if(!index.isValid()) return QVariant();
        const int col = index.column();
        Item *item = static_cast<Item*>(index.internalPointer())->children.at(index.row());

        switch (role) {
        case Qt::DisplayRole:
            if(col == 0) return QVariant::fromValue(item->name);
            else if(col == 1) return QVariant::fromValue(item->time);
            else if(col == 2) return QVariant::fromValue(item->size);
            else return QVariant::fromValue(item->type);
            break;
        case IconRole:
            if(col == 0) return QVariant::fromValue(item->icon);
            break;
        case Qt::TextAlignmentRole:
            if(col == 0) return QVariant(Qt::AlignLeft);
            else return QVariant(Qt::AlignCenter);
        case TypeRole:
            return QVariant(item->isDir);
            break;
        case PathRole:
            return QVariant::fromValue(item->path);
            break;
        case ParentRole:
            return QVariant::fromValue(item->parentDir);
            break;
        default:
            break;
        }

        return QVariant();
    }

    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        if (!index.isValid())
            return { };
        Qt::ItemFlags f = QAbstractItemModel::flags(index);
        if(!index.data(TypeRole).value<bool>())
            f |= Qt::ItemIsDragEnabled;

        return f;
    }

    QStringList mimeTypes() const override
    {
         return QStringList("text/uri-list");
    }

    QMimeData *mimeData(const QModelIndexList &indexes) const override
    {
        if (indexes.count() != 1 || indexes.first().data(TypeRole).value<bool>())
            return nullptr;
        QStringList types = mimeTypes();
        if (types.isEmpty())
            return nullptr;
        QMimeData *data = new QMimeData();
//        QString format = types.at(0);
//        QByteArray encoded;
//        QDataStream stream(&encoded, QIODevice::WriteOnly);
//        encodeData(indexes, stream);
//        data->setData(format, encoded);

        data->setUrls({QUrl::fromLocalFile(indexes.first().data(PathRole).value<QString>())});

        return data;
    }

signals:
    void finishLoad();
    void message(QString);

private:
    Item *rootItem;

};

class DesktopItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
private:
    static const int IconSize = 20;

public:
        DesktopItemDelegate(QObject *parent=nullptr) : QStyledItemDelegate(parent), treeView(qobject_cast<PackageTreeView*>(parent)) {}

        void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {

            painter->save();
            painter->translate(option.rect.topLeft());
            if(index.column() == 0) {
                painter->drawPixmap(QRect(5, 5, IconSize, IconSize), index.data(DesktopTreeModel::IconRole).value<QIcon>().pixmap(IconSize));
                QFont font = option.font;
                font.setBold(true);
                font.setPixelSize(16);
//            	QFontMetrics fontMetrics(font);
                painter->setFont(font);
                painter->drawText(QRect(10 + IconSize + 5, 0, option.rect.width()-10-IconSize-5, option.rect.height()), Qt::AlignLeft | Qt::AlignVCenter, index.data(Qt::DisplayRole).value<QString>());
            } else
                painter->drawText(QRect(0, 0, option.rect.width(), option.rect.height()), Qt::AlignCenter, index.data(Qt::DisplayRole).value<QString>());
            painter->restore();

        }

        QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
            if(!index.isValid())
                return QSize(option.rect.size());

            switch(index.column()){
            case 0:
                return QSize(400, 30);
                break;
             case 1:
                return QSize(200, 30);
                break;
            case 2:
                return QSize(100, 30);
                break;
            case 3:
                return QSize(100, 30);
                break;
            }

            return QSize(option.rect.size());
        }


private:
        PackageTreeView *treeView;
};

PackageTreeView::PackageTreeView(QWidget *parent) : QTreeView(parent)
{
    setRootIsDecorated(true);
    setContentsMargins(0, 0, 0, 0);
//    setResizeMode(QTreeView::Adjust);
    setAllColumnsShowFocus(true);
    setAlternatingRowColors(true);
    setSizeAdjustPolicy(QTreeView::AdjustToContents);
    setVerticalScrollMode(QTreeView::ScrollPerPixel);
    setSelectionMode(QTreeView::SingleSelection);
    setSelectionBehavior(QTreeView::SelectItems);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    verticalScrollBar()->setSingleStep(10);
    setAutoScroll(false);

    setUniformRowHeights(true);
    header()->setStretchLastSection(false);
    header()->setDefaultAlignment(Qt::AlignCenter);
    setMouseTracking(true);

    setDragDropMode(QTreeView::DragOnly);

    DesktopTreeModel *model = new DesktopTreeModel(this);
    connect(model, &DesktopTreeModel::finishLoad, this, &PackageTreeView::finishLoad);
    connect(model, &DesktopTreeModel::message, this, &PackageTreeView::message);
    setModel(model);
    setItemDelegate(new DesktopItemDelegate(this));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    header()->setSectionResizeMode(0, QHeaderView::Stretch);
    header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    connect(this, &PackageTreeView::entered, [this](const QModelIndex &index){
        if(!index.isValid() ||  index.data(DesktopTreeModel::TypeRole).value<bool>())
            unsetCursor();
        else
            setCursor(Qt::PointingHandCursor);
    });
}

void PackageTreeView::refresh(const QString &packageName) {
    if(m_packageName == packageName) return;

    m_packageName = packageName;
    emit startLoad();
    qobject_cast<DesktopTreeModel*>(model())->reset();
    QTimer::singleShot(10, this, [this, packageName]{
        qobject_cast<DesktopTreeModel*>(model())->initData(DesktopFileModel::instance()->getPackageFiles(packageName));
        expandRecursively(QModelIndex(), 1);
    });
}

QItemSelectionModel::SelectionFlags	PackageTreeView::selectionCommand(const QModelIndex &index, const QEvent *event) const {
//    if(index.column() == 0)
//        return QItemSelectionModel::ClearAndSelect;
//    return QItemSelectionModel::NoUpdate;
    return QTreeView::selectionCommand(index, event);
}

void PackageTreeView::leaveEvent(QEvent *event) {
    QTreeView::leaveEvent(event);
}

void PackageTreeView::mousePressEvent(QMouseEvent *event) {
    QModelIndex index = indexAt(event->pos());
    if(event->button() == Qt::RightButton && index.isValid()) {
        const bool isDir = index.data(DesktopTreeModel::TypeRole).value<bool>();
        const QString path = index.data(DesktopTreeModel::PathRole).value<QString>();

        QMenu *menu = new QMenu(this);
        menu->addAction("打开", [path]{ FileUtils::openFile(path); });

        if(isDir)
            menu->addAction(QIcon::fromTheme("utilities-terminal"), "在终端中打开", [this, path]{
                fileService->openInTerminal(this, DUrl::fromLocalFile(path));
            });
        else {
            menu->addAction(QIcon::fromTheme("edit-copy"), "复制", [path]{
                QClipboard *clip = QGuiApplication::clipboard();
                QMimeData *data = new QMimeData;
                data->setUrls(QList<QUrl>{QUrl(path)});
                clip->setMimeData(data);
            });
            menu->addSeparator();
            menu->addAction(QIcon::fromTheme("document-properties"), "属性", [this, path]{
                emit fileSignalManager->requestShowPropertyDialog(DFMUrlListBaseEvent(this, {DUrl::fromLocalFile(path)}));
            });
        }
        menu->exec(event->globalPos());
        menu->close();
        menu->deleteLater();
        return;
    }

    QTreeView::mousePressEvent(event);
}

void PackageTreeView::mouseMoveEvent(QMouseEvent *event) {
    if(event->button() == Qt::LeftButton) {
        QModelIndex index = indexAt(event->pos());
        if(index.isValid() && (index.flags() & Qt::ItemIsDragEnabled)) {
            return startDrag(Qt::CopyAction);
        }
    }

    QTreeView::mouseMoveEvent(event);
}

void PackageTreeView::mouseDoubleClickEvent(QMouseEvent *event) {
    QModelIndex index = indexAt(event->pos());
    if(index.isValid() && index.data(DesktopTreeModel::TypeRole).value<bool>() == false) {
        FileUtils::openFile(index.data(DesktopTreeModel::PathRole).value<QString>());
        return;
    }
    QTreeView::mouseDoubleClickEvent(event);
}

void PackageTreeView::keyReleaseEvent(QKeyEvent *event) {
    QModelIndex index = currentIndex();
    if(index.isValid() && event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        DUrl url = DUrl::fromLocalFile(index.data(DesktopTreeModel::PathRole).value<QString>());
        emit fileSignalManager->requestShowFilePreviewDialog(DUrlList{{url}}, {});
        return;
    }
    QTreeView::keyReleaseEvent(event);
}

#include "packagetreeview.moc"
