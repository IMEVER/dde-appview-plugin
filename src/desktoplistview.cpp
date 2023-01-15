#include "desktoplistview.h"

#include "desktopfilemodel.h"
#include "launcherdbusinterface.h"

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
#include <QMenu>
#include <QClipboard>
#include <QProcess>
#include <QLineEdit>
#include <QTimer>
#include <QStandardPaths>
#include <QtConcurrent>

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


class DesktopItemView : public QWidget {
    Q_OBJECT
public:
    DesktopItemView(QString file, LauncherDbus *launcher, DBusDock *dock, DesktopListView *parent) : QWidget(parent),
        m_file(file),
        m_launcher(launcher) ,
        m_dock(dock)
    {
        DesktopFileInfo info(DUrl::fromLocalFile(m_file));
        m_fileName = m_file.mid(m_file.lastIndexOf('/') + 1).remove(".desktop");
        m_displayName = info.fileDisplayName();        

        canDrop = info.canDrop();
        canWrite = info.isWritable();

        {
            QSettings settings(m_file, QSettings::IniFormat);
            settings.setIniCodec("utf-8");
            settings.beginGroup("Desktop Entry");
            Properties desktop(m_file, "Desktop Entry");

            m_mimeType = desktop.value("MimeType", settings.value("MimeType")).toString().split(";", Qt::SkipEmptyParts).toSet();
        }

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setSpacing(5);
        layout->setContentsMargins(10, 10, 10, 10);
        QLabel *iconLabel = new QLabel(this);
        iconLabel->setFixedSize(70, 70);
        iconLabel->setScaledContents(true);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setPixmap(info.fileIcon().pixmap(QSize(70, 70)));
        layout->addWidget(iconLabel, 0, Qt::AlignCenter);

        QLabel *titleLabel = new QLabel(m_displayName, this);
        titleLabel->setFixedSize(115, 30);
        titleLabel->setAlignment(Qt::AlignCenter);
        QFont font = titleLabel->font();
        font.setBold(true);
        titleLabel->setFont(font);
        layout->addWidget(titleLabel);
        setFixedSize(125, 125);
        setToolTip(QString("名称: %1\n分类: %2\n类型: %3").arg(info.getName()).arg(info.getCategories().join(" ")).arg(info.getType()));

        currentChanged(false);

        setAcceptDrops(true);
        setMouseTracking(true);
    }

    void open() {
        FileUtils::launchApp(m_file);
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
            menu->addAction(canWrite ? "编辑" : "以管理员身份编辑", [this]{
                if(canWrite)
                    QProcess::startDetached("xdg-open", {m_file});
                else if(copyBash())
                    QProcess::startDetached("bash", {bashPath, "-e", m_file});
            });
            menu->addAction("复制", [this]{
                QClipboard *clip = QGuiApplication::clipboard();
                QMimeData *data = new QMimeData;
                data->setUrls(QList<QUrl>{QUrl::fromLocalFile(m_file)});
                data->setText(m_file);
                QByteArray ba = "copy";
                ba.append("\n").append(QUrl::fromLocalFile(m_file).toString());
                data->setData("x-special/gnome-copied-files", ba);
                clip->setMimeData(data);
            });

            menu->addSeparator();

            auto reply = m_launcher->IsItemOnDesktop(m_fileName);
            reply.waitForFinished();
            const bool isOnDesktop = !reply.isError() && reply.value();
            menu->addAction(isOnDesktop ? "从桌面删除" : "发送到桌面", this, [this, isOnDesktop]{
                if(isOnDesktop)
                    m_launcher->RequestRemoveFromDesktop(m_fileName);
                else
                    m_launcher->RequestSendToDesktop(m_fileName);
            });

            reply = m_dock->IsDocked(m_file);
            reply.waitForFinished();
            const bool isDocked = !reply.isError() && reply.value();
            menu->addAction(isDocked ? "从任务栏删除" : "发送到任务栏", this, [this, isDocked]{
                if(isDocked)
                    m_dock->RequestUndock(m_file);
                else
                    m_dock->RequestDock(m_file, -1);
            });

            menu->addSeparator();
            menu->addAction("查看包信息", [this]{
                emit requestCd(DUrl("plugin://app/" + m_packageName + "#info"));
            })->setEnabled(isPackage);
            menu->addAction("查看包文件列表", [this]{
                emit requestCd(DUrl("plugin://app/" + m_packageName + "#file"));
            })->setEnabled(isPackage);
            menu->addAction("提取包", [this]{
//                if(copyBash())
                emit requestCd(DUrl("plugin://app/" + m_packageName + "#package"));
//                    QProcess::startDetached("bash", {bashPath, "-p", m_packageName});
            })->setEnabled(isPackage);
            menu->addAction("卸载", [this]{
                //                    if(QProcess::startDetached("pkexec", {"dpkg", "-r", package}))
                //                        qInfo()<<"'卸载软件成功: " << package;
                //                    emit requestUninstall(package, false);
                if(copyBash())
                    QProcess::startDetached("bash", {bashPath, "-u", m_packageName});
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
        if(canDrop && !m_mimeType.isEmpty() && event->mimeData()->hasUrls()) {
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

                        accept = !types.isEmpty() && m_mimeType.intersects(types);
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

         FileUtils::openFilesByApp(m_file, files);
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
        if(m_packageName.isEmpty())
            m_packageName = DesktopFileModel::instance()->getPackageName(m_file);
        return !m_packageName.isEmpty();
    }

signals:
    void requestCd(DUrl url);

private:
    QString m_file;
    QString m_fileName;
    QString m_packageName;
    QString m_displayName;
    QSet<QString> m_mimeType;
    bool canDrop;
    bool canWrite;
    bool m_current;

    LauncherDbus *m_launcher;
    DBusDock *m_dock;
    friend class DesktopListView;
};

DesktopListView::DesktopListView(QWidget *parent) : QListWidget(parent),
    m_loading(false),
    launcher(new LauncherDbus(QDBusConnection::sessionBus(), this)),
    dock(new DBusDock("com.deepin.dde.daemon.Dock", "/com/deepin/dde/daemon/Dock", QDBusConnection::sessionBus(), this)),
    m_searchEditor(nullptr),
    m_searchTimer(nullptr)
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

    connect(DesktopFileModel::instance(), &DesktopFileModel::startLoad, this, [this]{
        m_loading = true;
        startLoad();
    });
    connect(DesktopFileModel::instance(), &DesktopFileModel::directoryChanged, this, [this](QStringList addFiles, QStringList removeFiles){
//        QtConcurrent::run([this, removeFiles, addFiles]{
            const bool isUpdate = count() > 0;
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
                if(isUpdate)
                    this->insertItem(0, item);
                else
                    this->addItem(item);

                DesktopItemView *desktopItemView = new DesktopItemView(file, launcher, dock, this);
                connect(desktopItemView, &DesktopItemView::requestCd, this, &DesktopListView::requestCd);
                this->setItemWidget(item, desktopItemView);
                if(!isUpdate)
                    item->setText(desktopItemView->m_displayName);
                cachedItem.insert(file, item);
            }
            m_loading = false;
            emit finishLoad();
            emit fileCount(cachedItem.count());
            if(isUpdate == false) {
                sortItems();
                for(auto i : cachedItem.values()) i->setText("");
            }
//        });
    });

    connect(this, &DesktopListView::itemDoubleClicked, [this](QListWidgetItem *item){
        qobject_cast<DesktopItemView*>(itemWidget(item))->open();
    });
    connect(this, &DesktopListView::currentItemChanged, [this](QListWidgetItem *current, QListWidgetItem *prev){
        if(current)
            if(DesktopItemView *v = qobject_cast<DesktopItemView*>(itemWidget(current)))
                v->currentChanged(true);
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

void DesktopListView::refresh() {
    if(m_searchEditor && !m_searchEditor->isHidden()) {
        m_searchTimer->stop();
        m_searchEditor->hide();
    }
    if(m_loading) return;

    m_loading = true;
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
        urls << QUrl::fromLocalFile(qobject_cast<DesktopItemView*>(itemWidget(item))->m_file);
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
                DUrlList urls; urls << DUrl::fromLocalFile(qobject_cast<DesktopItemView*>(itemWidget(item))->m_file);
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
                        if(v->m_displayName.contains(filter, Qt::CaseInsensitive))
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
                            if(v->m_displayName.contains(filter, Qt::CaseInsensitive))
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

