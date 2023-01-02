#include "desktoplistview.h"

#include "../../dde-file-manager-lib/models/desktopfileinfo.h"
#include <singleton.h>
#include "../../dde-file-manager-lib/app/define.h"
#include "../../dde-file-manager-lib/app/filesignalmanager.h"
#include "../../dde-file-manager-lib/shutil/fileutils.h"

#include <QFileSystemWatcher>
#include <QStandardPaths>
#include <QDir>
#include <QTimer>
#include <QLayout>
#include <QScrollBar>
#include <QLabel>
#include <QDirIterator>
#include <QMimeData>
#include <QFile>
#include <QPainter>
#include <DGuiApplicationHelper>
#include <QApplication>

DGUI_USE_NAMESPACE

class DesktopFileModel : public QObject {
    Q_OBJECT
public:
    DesktopFileModel(QObject *parent=nullptr) : QObject(parent) {
        QStringList paths = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
        QFileSystemWatcher *watcher = new QFileSystemWatcher(this);
        watcher->addPaths(paths);
        connect(watcher, &QFileSystemWatcher::directoryChanged, this, &DesktopFileModel::initDesktopFiles);
    }

    void refresh() {
        files.clear();
        initDesktopFiles();
    }

private:
    void initDesktopFiles(const QString pathChanged=QString()) {
        Q_UNUSED(pathChanged)
        emit startLoad();
        const QStringList paths = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
        QStringList newFiles, addFiles, removeFiles;
        QSet<QString> newFilesSet;
        for(auto path : paths) {
//            newFiles.append(QDir(path).entryList(QStringList("*.desktop"), QDir::Files | QDir::NoDotAndDotDot));
              QDirIterator it(path, QStringList("*.desktop"),
                              QDir::Files | QDir::NoDotAndDotDot,
                              QDirIterator::NoIteratorFlags);
              while (it.hasNext()) {
                it.next();
                newFiles.append(it.filePath());
                newFilesSet.insert(it.filePath());
              }
        }

        for(auto file : newFiles)
            if(!files.contains(file))
                addFiles.append(file);

        for(auto file : files)
            if(!newFilesSet.contains(file))
                removeFiles.append(file);

       emit directoryChanged(addFiles, removeFiles);

       files.clear();
       files.reserve(newFilesSet.size());
       for(auto f : newFilesSet) files.insert(f);
    }

signals:
    void directoryChanged(QStringList addFiles, QStringList removeFiles);
    void startLoad();

private:
    QSet<QString> files;
};

class DesktopItemView : public QWidget {
    Q_OBJECT
public:
    DesktopItemView(QString file, QWidget *parent) : QWidget(parent) {
        info = new DesktopFileInfo(DUrl::fromLocalFile(file));

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setSpacing(5);
        layout->setContentsMargins(10, 10, 10, 10);
        QLabel *iconLabel = new QLabel(this);
        iconLabel->setFixedSize(70, 70);
        iconLabel->setScaledContents(true);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setPixmap(info->fileIcon().pixmap(QSize(70, 70)));
        layout->addWidget(iconLabel, 0, Qt::AlignCenter);

        QLabel *titleLabel = new QLabel(info->fileDisplayName(), this);
        titleLabel->setFixedSize(115, 30);
        titleLabel->setAlignment(Qt::AlignCenter);
        QFont font = titleLabel->font();
        font.setBold(true);
        titleLabel->setFont(font);
        layout->addWidget(titleLabel);
        setFixedSize(125, 125);

        setStyleSheet(QString("DesktopItemView { background-color: %1; border-radius: 20px; }").arg(color(false)));
    }

    ~DesktopItemView() {
        delete info;
    }

    void open() {
        FileUtils::launchApp(info->absoluteFilePath());
    }

    void currentChanged(bool on) {
        setStyleSheet(QString("DesktopItemView { background-color: %1; border-radius: 20px; }").arg(color(on)));
    }

protected:
    void paintEvent(QPaintEvent *event)
    {
        Q_UNUSED(event)
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    }

private:
    QString color(bool on) {
        if(DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType)
            return QString("rgba(0, 0, 0, %1)").arg(on ? "40%" : "20%");
        else
            return QString("rgba(255,255,255, %1)").arg(on ? "40%" : "20%");
    }

private:
    DesktopFileInfo *info;
    friend class DesktopListView;
};

DesktopListView::DesktopListView(QWidget *parent) : QListWidget(parent)
{
    setFlow(QListWidget::LeftToRight);
    setViewMode(QListWidget::IconMode);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setResizeMode(QListView::Adjust);
    setSelectionBehavior(QListWidget::SelectItems);
    setSelectionMode(QListWidget::SingleSelection);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollMode(QListWidget::ScrollPerPixel);
    setDragDropMode(QListWidget::DragDrop);
    setUniformItemSizes(true);
    setMouseTracking(true);
    verticalScrollBar()->setSingleStep(30);

    setSpacing(30);
    setContentsMargins(30, 10, 30, 10);

    m_model = new DesktopFileModel(this);
    connect(m_model, &DesktopFileModel::startLoad, this, &DesktopListView::startLoad);
    connect(m_model, &DesktopFileModel::directoryChanged, [this](QStringList addFiles, QStringList removeFiles){
        for(auto file : removeFiles) {
            if(QListWidgetItem *item = cachedItem.take(file)) {
                DesktopItemView *desktopItemView = qobject_cast<DesktopItemView*>(itemWidget(item));
                delete item;
                desktopItemView->deleteLater();
            }
        }

        for(auto file : addFiles) {
            if(cachedItem.contains(file)) continue;

            QListWidgetItem *item = new QListWidgetItem(this);
            item->setSizeHint(QSize(125, 125));
            this->addItem(item);

            DesktopItemView *desktopItemView = new DesktopItemView(file, this);
            this->setItemWidget(item, desktopItemView);
            cachedItem.insert(file, item);
        }
        emit finishLoad();
        emit fileCount(cachedItem.count());
    });

    connect(this, &DesktopListView::itemDoubleClicked, [this](QListWidgetItem *item){
        qobject_cast<DesktopItemView*>(itemWidget(item))->open();
    });
    connect(this, &DesktopListView::currentItemChanged, [this](QListWidgetItem *current, QListWidgetItem *prev){
        qobject_cast<DesktopItemView*>(itemWidget(current))->currentChanged(true);
        if(prev) qobject_cast<DesktopItemView*>(itemWidget(prev))->currentChanged(false);
    });

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, [this]{
        const int current = currentRow();
        for(int i=0, len=count(); i<len; i++) {
            qobject_cast<DesktopItemView*>(itemWidget(item(i)))->currentChanged(current == i);
        }
    });

    QTimer::singleShot(10, this, &DesktopListView::refresh);
}

void DesktopListView::refresh() {
    emit startLoad();
    while(count()>0) {
        QListWidgetItem *item = takeItem(0);
        itemWidget(item)->deleteLater();
        delete item;
    }
    cachedItem.clear();
    m_model->refresh();
}

QStringList DesktopListView::mimeTypes() const {
    return QStringList("text/uri-list");
}

QMimeData *DesktopListView::mimeData(const QList<QListWidgetItem *> items) const {
    QMimeData *data = new QMimeData();
    QList<QUrl> urls;
    for(auto item : items) {
        urls << qobject_cast<DesktopItemView*>(itemWidget(item))->info->fileUrl();
    }
    data->setUrls(urls);
    return data;
}

bool DesktopListView::dropMimeData(int index, const QMimeData *data, Qt::DropAction action) {
    QList<QUrl> urls = data->urls();
    bool accept = false;
    if(urls.count() == 1) {
        auto url = urls.first();
        if(url.isLocalFile() && url.fileName().endsWith(".deb")) {
           accept = QFile(url.toLocalFile()).exists();
        }
    }

    if(!accept && data->hasUrls()) {
        bool allFile = true;
        for(auto url : data->urls()) {
            if(!url.isLocalFile() || !QFile(url.toLocalFile()).exists()) {
                allFile = false;
                break;
            }
        }

        if(allFile) {
            if(QListWidgetItem *i = item(index)) {
                DesktopItemView *v = qobject_cast<DesktopItemView*>(itemWidget(i));
                accept = v->info->canDrop();
            }
        }
    }

    return accept;
}

void DesktopListView::dragEnterEvent(QDragEnterEvent *event) {
     QList<QUrl> urls = event->mimeData()->urls();
     bool accept = false;
     if(urls.count() == 1) {
         auto url = urls.first();
         if(url.isLocalFile() && url.fileName().endsWith(".deb")) {
            accept = QFile(url.toLocalFile()).exists();
         }
     }

     if(accept)
         return event->accept();
//     else
//         return event->ignore();
    QListWidget::dragEnterEvent(event);
}

void DesktopListView::dragMoveEvent(QDragMoveEvent *e) {
    QList<QUrl> urls = e->mimeData()->urls();
    bool accept = false;
    if(urls.count() == 1) {
        auto url = urls.first();
        if(url.isLocalFile() && url.fileName().endsWith(".deb")) {
           accept = QFile(url.toLocalFile()).exists();
        }
    }

    if(!accept && e->mimeData()->hasUrls()) {
        bool allFile = true;
        for(auto url : e->mimeData()->urls()) {
            if(!url.isLocalFile() || !QFile(url.toLocalFile()).exists()) {
                allFile = false;
                break;
            }
        }
        if(allFile) {
            if(QListWidgetItem *i = itemAt(e->pos())) {
                DesktopItemView *v = qobject_cast<DesktopItemView*>(itemWidget(i));
                accept = v->info->canDrop();
            }
        }
    }

    if(accept)
        return e->accept();
//    else
//        return e->ignore();

    QListWidget::dragMoveEvent(e);
}

void DesktopListView::dropEvent(QDropEvent *event) {
    bool accept = false;
    QList<QUrl> urls = event->mimeData()->urls();
    if(urls.count() == 1) {
        auto url = urls.first();
        if(url.isLocalFile() && url.fileName().endsWith(".deb")) {
           if(QFile(url.toLocalFile()).exists()) {
                FileUtils::openFile(url.toLocalFile());
                accept = true;
           }
        }
    }

    if(!accept && event->mimeData()->hasUrls()) {
        bool allFile = true;
        QStringList files;
        for(auto url : event->mimeData()->urls()) {
            if(!url.isLocalFile() || !QFile(url.toLocalFile()).exists()) {
                allFile = false;
                break;
            }
            files.append(url.toLocalFile());
        }

        if(allFile) {
            if(QListWidgetItem *i = itemAt(event->pos())) {
                DesktopItemView *v = qobject_cast<DesktopItemView*>(itemWidget(i));
                accept = v->info->canDrop();
                if(accept)
                    FileUtils::openFilesByApp(v->info->absoluteFilePath(), files);
            }
        }
    }

    if(accept) return event->accept();

    QListWidget::dropEvent(event);
}

void DesktopListView::keyReleaseEvent(QKeyEvent *event)
{
    if(this == QApplication::focusWidget()){
        if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
            if(QListWidgetItem *item = currentItem()) {
                DUrlList urls; urls << qobject_cast<DesktopItemView*>(itemWidget(item))->info->fileUrl();
                emit fileSignalManager->requestShowFilePreviewDialog(urls, {});
                return;
            }
        }
    }

    DesktopListView::keyReleaseEvent(event);
}

//#include "moc_desktoplistview.cpp"
#include "desktoplistview.moc"

