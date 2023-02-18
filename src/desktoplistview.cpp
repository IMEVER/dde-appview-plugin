#include "desktoplistview.h"

#include "desktopfilemodel.h"
#include "launcherdbusinterface.h"

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
#include <QMenu>
#include <QClipboard>
#include <QProcess>
#include <QLineEdit>
#include <QTimer>
#include <QStandardPaths>
#include <QtConcurrent>
#include <QFutureWatcher>

DGUI_USE_NAMESPACE

static const QString bashPath(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/dde-appview-plugin.sh");

bool copyBash() {
    static bool init = false;
    QFile toFile(bashPath);
    if(init && toFile.exists(bashPath)) return true;
    if(QFile::copy(":/sh/dde-appview-plugin.sh", bashPath) == false) return false;

    toFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner | QFile::ReadGroup | QFile::ReadOther);
    init = true;
    return true;
}

struct DesktopFileInfo{
    QString file;
    QString name;
    QString fileName;
    QString packageName;
    QString type;
    QString icon;
    QString displayName;
    QString categories;
    QSet<QString> mimeType;
    bool canDrop = false;
    bool canWrite = false;

    DesktopFileInfo(){}

    DesktopFileInfo& operator=(const DesktopFileInfo &info) {
        file = info.file;
        name = info.name;
        displayName = info.displayName;
        type = info.type;
        icon = info.icon;
        categories = info.categories;
        mimeType.clear();
        mimeType += info.mimeType;
        canDrop = info.canDrop;
        canWrite = info.canWrite;
        return *this;
    }
};

class DesktopItemView : public QWidget {
    Q_OBJECT
public:
    DesktopItemView(const QString &file, LauncherDbus *launcher, DBusDock *dock, DesktopListView *parent) : QWidget(parent),
        m_launcher(launcher),
        m_dock(dock)
    {
        m_info.file = file;
        m_info.fileName = file.mid(file.lastIndexOf('/') + 1).remove(".desktop");

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setSpacing(5);
        layout->setContentsMargins(10, 10, 10, 10);
        iconLabel = new QLabel(this);
        iconLabel->setFixedSize(70, 70);
        iconLabel->setScaledContents(true);
        iconLabel->setAlignment(Qt::AlignCenter);        
        layout->addWidget(iconLabel, 0, Qt::AlignCenter);

        titleLabel = new QLabel(m_info.fileName, this);
        titleLabel->setFixedSize(105, 30);
        titleLabel->setAlignment(Qt::AlignCenter);
        QFont font = titleLabel->font();
        font.setBold(true);
        titleLabel->setFont(font);
        layout->addWidget(titleLabel);
        setFixedSize(125, 125);        

        currentChanged(false);
        setAcceptDrops(true);
        setMouseTracking(true);
    }

    void open() {
        FileUtils::launchApp(m_info.file);
    }

    void currentChanged(bool current, bool hover=false) {
        m_current = current;
        setStyleSheet(QString("QLabel {color: %1} DesktopItemView { background-color: %2; border-radius: 20px; }")
                      .arg(m_current ? palette().highlight().color().name() : palette().windowText().color().name())
                      .arg(color(current, hover)));
    }

protected:
    void enterEvent(QEvent *event) override {
        if(!m_current)
            currentChanged(m_current, true);
        QWidget::enterEvent(event);
    }

    void leaveEvent(QEvent *event) override {
        if(!m_current)
            currentChanged(m_current, false);
        QWidget::leaveEvent(event);
    }

    void mousePressEvent(QMouseEvent *event) override {
        if(event->button() == Qt::RightButton) {
            const bool isPackage = checkPackageName();

            QMenu *menu = new QMenu(this);
            menu->addAction("打开", [this]{ open(); });
            menu->addAction(m_info.canWrite ? "编辑" : "以管理员身份编辑", [this]{
                if(m_info.canWrite)
                    QProcess::startDetached("xdg-open", {m_info.file});
                else if(copyBash())
                    QProcess::startDetached("bash", {bashPath, "-e", m_info.file});
            });
            menu->addAction("复制", [this]{
                QClipboard *clip = QGuiApplication::clipboard();
                QMimeData *data = new QMimeData;
                data->setUrls(QList<QUrl>{QUrl::fromLocalFile(m_info.file)});
                data->setText(m_info.file);
                QByteArray ba = "copy\n";
                ba.append(QUrl::fromLocalFile(m_info.file).toString().toUtf8());
                data->setData("x-special/gnome-copied-files", ba);
                clip->setMimeData(data);
            });

            menu->addSeparator();

            auto reply = m_launcher->IsItemOnDesktop(m_info.fileName);
            reply.waitForFinished();
            const bool isOnDesktop = !reply.isError() && reply.value();
            menu->addAction(isOnDesktop ? "从桌面删除" : "发送到桌面", this, [this, isOnDesktop]{
                if(isOnDesktop)
                    m_launcher->RequestRemoveFromDesktop(m_info.fileName);
                else
                    m_launcher->RequestSendToDesktop(m_info.fileName);
            });

            reply = m_dock->IsDocked(m_info.file);
            reply.waitForFinished();
            const bool isDocked = !reply.isError() && reply.value();
            menu->addAction(isDocked ? "从任务栏删除" : "发送到任务栏", this, [this, isDocked]{
                if(isDocked)
                    m_dock->RequestUndock(m_info.file);
                else
                    m_dock->RequestDock(m_info.file, -1);
            });

            menu->addSeparator();
            menu->addAction("查看包信息", [this]{
                emit requestCd(DUrl("plugin://app/" + m_info.packageName + "#info"));
            })->setEnabled(isPackage);
            menu->addAction("查看包文件列表", [this]{
                emit requestCd(DUrl("plugin://app/" + m_info.packageName + "#file"));
            })->setEnabled(isPackage);
            menu->addAction("提取包", [this]{
                emit requestCd(DUrl("plugin://app/" + m_info.packageName + "#package"));
            })->setEnabled(isPackage);
            menu->addAction("卸载", [this]{
                //                    if(QProcess::startDetached("pkexec", {"dpkg", "-r", package}))
                //                        qInfo()<<"'卸载软件成功: " << package;
                //                    emit requestUninstall(package, false);
                if(copyBash())
                    QProcess::startDetached("bash", {bashPath, "-u", m_info.packageName});
            })->setEnabled(isPackage);

            menu->exec(event->globalPos());
            menu->close();
            menu->deleteLater();
            return;
        }
        QWidget::mousePressEvent(event);
    }

    void dragEnterEvent(QDragEnterEvent *event) override {
        bool accept = false;
        if(m_info.canDrop && !m_info.mimeType.isEmpty() && event->mimeData()->hasUrls()) {
            QMimeDatabase dataBase;
            for(QUrl url : event->mimeData()->urls()) {
                accept = url.isLocalFile();
                if(accept) {
                    QFileInfo info(url.toLocalFile());
                    accept = info.exists();
                    if(accept) {
                        QSet<QString> types;
                        for(QMimeType type : dataBase.mimeTypesForFileName(url.toLocalFile()))
                            types.insert(type.name());

                        accept = !types.isEmpty() && m_info.mimeType.intersects(types);
                    }
                }
                if(!accept) break;
            }
        }
        event->setAccepted(accept);
    }

    void dropEvent(QDropEvent *event) override {
        QStringList files;
        for(QUrl url : event->mimeData()->urls())
            files.append(url.toLocalFile());

         FileUtils::openFilesByApp(m_info.file, files);
         event->accept();
    }

    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event)
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    }

private:
    QString color(bool on, bool hover) {
        if(DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType)
            return QString("rgba(0, 0, 0, %1)").arg(on ? "40%" : (hover ? "30%" : "20%"));
        else
            return QString("rgba(255,255,255, %1)").arg(on ? "40%" : (hover ? "30%" : "20%"));
    }

    bool checkPackageName() {
        if(m_info.packageName.isEmpty())
            m_info.packageName = DesktopFileModel::instance()->getPackageName(m_info.file);
        return !m_info.packageName.isEmpty();
    }

    void updateInfo(const DesktopFileInfo &info) {
        m_info = info;
        iconLabel->setPixmap(QIcon::fromTheme(m_info.icon).pixmap(QSize(70, 70)));
        titleLabel->setText(titleLabel->fontMetrics().elidedText(m_info.displayName, Qt::ElideRight, titleLabel->width()));
        checkPackageName();
        setToolTip(QString("名称: %1\n分类: %2\n类型: %3\n包名: %4\n位置: %5").arg(m_info.name).arg(m_info.categories).arg(m_info.type).arg(m_info.packageName).arg(m_info.file));
    }

signals:
    void requestCd(DUrl url);

private:
    QLabel *iconLabel;
    QLabel *titleLabel;

    DesktopFileInfo m_info;
    bool m_current;

    LauncherDbus *m_launcher;
    DBusDock *m_dock;
    friend class DesktopListView;
};

std::function<DesktopFileInfo*(const QString &)> map = [](const QString &file){
    DesktopFileInfo *info(new DesktopFileInfo);
    info->file = file;

    QSettings settings(file, QSettings::IniFormat);
    if(settings.status() == QSettings::NoError) {
        settings.setIniCodec("utf-8");
        settings.beginGroup("Desktop Entry");

        info->name = settings.value("Name").toString();
        info->displayName = settings.value(QString("Name[%1]").arg(QLocale::system().name()), info->name).toString();
        info->icon = settings.value("Icon").toString();
        info->type = settings.value("Type").value<QString>();
        info->categories = settings.value("Categories").toString();
        info->canDrop = settings.value("Exec").toString().trimmed().contains(QRegExp("(\\s%f|\\s%u)$", Qt::CaseInsensitive));
        info->canWrite = settings.isWritable();

        for(auto mt : settings.value("MimeType").toString().split(";", Qt::SkipEmptyParts))
            info->mimeType << mt;
    }
    return info;
};

DesktopListView::DesktopListView(QWidget *parent) : QListWidget(parent),
    m_loading(false),
    launcher(new LauncherDbus(QDBusConnection::sessionBus(), this)),
    dock(new DBusDock("com.deepin.dde.daemon.Dock", "/com/deepin/dde/daemon/Dock", QDBusConnection::sessionBus(), this)),
    m_searchEditor(nullptr),
    m_searchTimer(nullptr),
    m_watcher(new QFutureWatcher<DesktopFileInfo*>(this)),
    m_update(false)
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

    connect(m_watcher, &QFutureWatcher<DesktopFileInfo*>::resultReadyAt, this, [this](const int index){
        DesktopFileInfo *info = m_watcher->resultAt(index);
        if(QListWidgetItem *item = cachedItem.value(info->file)) {
            if(!m_update) item->setText(info->displayName);
            if(auto desktopItemView = qobject_cast<DesktopItemView*>(itemWidget(item)))
                desktopItemView->updateInfo(*info);
        }
    });
    connect(m_watcher, &QFutureWatcher<DesktopFileInfo*>::finished, this, [this]{
        if(m_update == false) {
            sortItems();
            for(auto i : cachedItem.values()) i->setText("");
        }
    });

    connect(DesktopFileModel::instance(), &DesktopFileModel::startLoad, this, [this]{
        m_loading = true;
        startLoad();
    });
    connect(DesktopFileModel::instance(), &DesktopFileModel::directoryChanged, this, [this](QStringList addFiles, QStringList removeFiles){
        m_update = count() > 0;
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
            if(m_update)
                this->insertItem(0, item);
            else
                this->addItem(item);

            DesktopItemView *desktopItemView = new DesktopItemView(file, launcher, dock, this);
            connect(desktopItemView, &DesktopItemView::requestCd, this, &DesktopListView::requestCd);
            this->setItemWidget(item, desktopItemView);
            cachedItem.insert(file, item);
        }
        m_loading = false;
        emit finishLoad();
        emit fileCount(cachedItem.count());

        if(!addFiles.isEmpty())
            m_watcher->setFuture(QtConcurrent::mapped(addFiles, map));
    });

    connect(this, &DesktopListView::itemDoubleClicked, [this](QListWidgetItem *item){
        qobject_cast<DesktopItemView*>(itemWidget(item))->open();
    });
    connect(this, &DesktopListView::currentItemChanged, [this](QListWidgetItem *current, QListWidgetItem *prev){
        if(current)
            if(DesktopItemView *v = qobject_cast<DesktopItemView*>(itemWidget(current))) {
                v->currentChanged(true);
                emit message(QString("%1 -- %2 -- %3").arg(v->m_info.displayName).arg(v->m_info.packageName).arg(v->m_info.file));
            }
        if(prev)
            if(DesktopItemView *v = qobject_cast<DesktopItemView*>(itemWidget(prev)))
                v->currentChanged(false);
    });

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, [this]{
        const int current = currentRow();
        for(int i=0, len=count(); i<len; i++) {
            qobject_cast<DesktopItemView*>(itemWidget(item(i)))->currentChanged(current == i);
        }
    });    

    refresh();
}

DesktopListView::~DesktopListView() {
    m_watcher->cancel();
}

void DesktopListView::refresh() {
    if(m_searchEditor && !m_searchEditor->isHidden()) {
        m_searchTimer->stop();
        m_searchEditor->hide();
    }
    if(m_loading) return;

    m_loading = true;
    m_watcher->cancel();
    QTimer::singleShot(10, this, [this] {
        emit startLoad();
        while(count()>0) {
            QListWidgetItem *item = takeItem(0);
            itemWidget(item)->deleteLater();
            delete item;
        }
        cachedItem.clear();
        DesktopFileModel::instance()->refresh();
    });
}

QStringList DesktopListView::mimeTypes() const {
    return QStringList("text/uri-list");
}

QMimeData *DesktopListView::mimeData(const QList<QListWidgetItem *> items) const {
    QMimeData *data = new QMimeData();
    QList<QUrl> urls;
    for(auto item : items) {
        urls << QUrl::fromLocalFile(qobject_cast<DesktopItemView*>(itemWidget(item))->m_info.file);
    }
    data->setUrls(urls);
    return data;
}

bool DesktopListView::dropMimeData(int index, const QMimeData *data, Qt::DropAction action) {
    Q_UNUSED(index)
    Q_UNUSED(action)

    bool accept = false;
    if(data->hasUrls()) {
        for(QUrl url : data->urls()) {
            accept = url.isLocalFile() && url.fileName().endsWith(".deb");
            if(accept)
                accept = QFile(url.toLocalFile()).exists();
            if(!accept) break;
        }
    }

    return accept;
}

void DesktopListView::dragEnterEvent(QDragEnterEvent *event) {
    bool accept = false;
    if(event->mimeData()->hasUrls()) {
         for(QUrl url : event->mimeData()->urls()) {
             if(url.isLocalFile() && url.fileName().endsWith(".deb"))
                accept = QFile(url.toLocalFile()).exists();
             else
                 accept = false;
             if(!accept) break;
         }
    }
    event->setAccepted(accept);
}

void DesktopListView::dropEvent(QDropEvent *event) {        
    QStringList files;
    for(auto url : event->mimeData()->urls())
        files.append(url.toLocalFile());

    if(!files.isEmpty()) {
        FileUtils::openFiles(files);
        return event->accept();
    }
}

void DesktopListView::keyReleaseEvent(QKeyEvent *event)
{
    if(count() > 0) {
        if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
            if(QListWidgetItem *item = currentItem()) {
                DUrlList urls; urls << DUrl::fromLocalFile(qobject_cast<DesktopItemView*>(itemWidget(item))->m_info.file);
                emit fileSignalManager->requestShowFilePreviewDialog(urls, {});
                return;
            }
        }

        auto searchFunc = [this](bool checkCurrent) {
            QString filter = m_searchEditor->text().trimmed();
            if(filter.isEmpty()) return;

            int max = count();
            int start = currentIndex().isValid() ? currentIndex().row() : 0;
            if(checkCurrent) start -= 1;

            int curr = start+1;
            while(curr < max) {
                if(QListWidgetItem *i = item(curr)) {
                    if(DesktopItemView *v = qobject_cast<DesktopItemView*>(itemWidget(i))) {
                        if(v->m_info.displayName.contains(filter, Qt::CaseInsensitive))
                            return setCurrentRow(curr);;
                    }
                }
                curr++;
            }

            if (start != -1) {
                max = start+1;
                curr = 0;
                while(curr < max) {
                    if(QListWidgetItem *i = item(curr)) {
                        if(DesktopItemView *v = qobject_cast<DesktopItemView*>(itemWidget(i))) {
                            if(v->m_info.displayName.contains(filter, Qt::CaseInsensitive))
                                return setCurrentRow(curr);
                        }
                    }
                    curr++;
                }
            }
        };

        if(event->key() == Qt::Key_F3) {
            if(!m_searchEditor) {
                m_searchEditor = new QLineEdit(this);
                m_searchEditor->setFixedSize(QSize(200, 50));
                m_searchEditor->installEventFilter(this);
                m_searchEditor->show();
                m_searchEditor->raise();
                m_searchEditor->move(rect().topRight() + QPoint(-210, 10));
                m_searchEditor->setFocus();

                m_searchTimer = new QTimer(this);
                m_searchTimer->setSingleShot(true);
                m_searchTimer->setInterval(400);
                connect(m_searchTimer, &QTimer::timeout, this, [searchFunc] { searchFunc(true); });
                connect(m_searchEditor, &QLineEdit::textEdited, this, [this] { m_searchTimer->start(); });
                connect(m_searchEditor, &QLineEdit::returnPressed, this, [this, searchFunc]{
                    m_searchTimer->stop();
                    if(!m_searchEditor->text().trimmed().isEmpty()) searchFunc(false);
                });
            } else if(m_searchEditor->isHidden()) {
                m_searchEditor->show();
                m_searchEditor->setFocus();
            } else if(!m_searchEditor->text().trimmed().isEmpty())
                searchFunc(false);
            return;
        } else if(event->key() == Qt::Key_Escape && m_searchEditor && !m_searchEditor->isHidden()) {
            m_searchTimer->stop();
            m_searchEditor->clear();
            m_searchEditor->hide();
            return;
        }
    }
    QListWidget::keyReleaseEvent(event);
}

bool DesktopListView::eventFilter(QObject *obj, QEvent *event) {
    if(m_searchEditor && obj == m_searchEditor) {
        if(event->type() == QEvent::KeyRelease) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if(keyEvent->key() == Qt::Key_Escape) {
                m_searchTimer->stop();
                m_searchEditor->clear();
                m_searchEditor->hide();
                return true;
            } else if(keyEvent->key() == Qt::Key_F3 && !m_searchEditor->text().trimmed().isEmpty()) {
                emit m_searchEditor->returnPressed();
                return true;
            }
        }
    }

    return QListWidget::eventFilter(obj, event);
}

#include "desktoplistview.moc"

