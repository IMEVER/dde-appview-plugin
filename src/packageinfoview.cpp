#include "packageinfoview.h"

#include "desktopfilemodel.h"

#include <QTableView>
#include <QHeaderView>
#include <QRegExp>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QFont>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QTextStream>
#include <QGuiApplication>
#include <QClipboard>
#include <QMimeData>
#include <QUrl>
#include <QtConcurrent/QtConcurrent>

class PackageModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    PackageModel(QObject *parent = nullptr) : QAbstractTableModel(parent){ }

    void clear() {
        if(m_package.count() > 0) {
            beginResetModel();
            m_package = Package();
            endResetModel();
        }
    }

    void initData(Package package) {
        beginInsertRows(QModelIndex(), 0, package.data.count()-1);
        m_package = package;
        endInsertRows();
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        if(parent.isValid())
            return 0;
        else
            return m_package.count();
    }
    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        if(parent.isValid())
            return 0;
        else
            return 1;
    }

    QString formatByteSize(int size) const
    {
        if(size < 1024)
            return QString("%1 KB").arg(size);
        else if(size > 1024 * 1024)
            return QString("%1 GB").arg(QString::number(size / 1024.0 / 1024.0, 'f', 2));
        else
            return QString("%1 MB").arg(QString::number(size / 1024.0, 'f', 2));
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override {
        if(orientation == Qt::Vertical) {
            switch(role){
                case Qt::DisplayRole:
                    if (section < m_package.count())
                    {
                        QPair<QString, QString> pair = m_package.data.at(section);
                        return translate(pair.first);
                    }
                    break;
                case Qt::FontRole: {
                        QFont font;
                        font.setBold(true);
                        font.setPixelSize(20);

                        return font;
                    }
                    break;
                case Qt::TextAlignmentRole:
                    return Qt::AlignCenter;
            }
        }

        return QVariant();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        static const QStringList keys{"Depends", "Pre-Depends", "Build-Depends", "Replaces", "Breaks" , "Conflicts", "Suggests", "Provides", "Recommends"};

        const int row = index.row();        
        switch(role){
            case Qt::DisplayRole:                
                {
                    QPair<QString, QString> pair = m_package.data.at(row);                
                    if(pair.first == "Installed-Size") {
                        int size = pair.second.toInt();
                        if(size > 0)
                            return formatByteSize(size);
                        else
                            return pair.second.trimmed();
                    } else if(keys.contains(pair.first))
                        return pair.second.replace(QRegExp(",\\s*"), "\n");
                    else
                        return pair.second.trimmed();
                }
                break;
            case Qt::FontRole: {
                QFont font;
                if (row == 0) {
                    font.setBold(true);
                    font.setPixelSize(20);
                } else
                     font.setPixelSize(14);

                 return font;
                break;
            }
            case Qt::TextAlignmentRole:
                return QVariant(Qt::AlignLeft|Qt::AlignVCenter);
        }
        return QVariant();
    }

    QVariant translate(QString field) const
    {
        if(field == "Package")
            return "包名";
        else if(field == "Status")
            return "状态";
        else if(field == "Priority")
            return "优先级";
        else if(field == "Section")
            return "分类";
        else if(field == "Installed-Size")
            return "安装大小";
        else if(field == "Download-Size")
            return "下载大小";
        else if(field == "Maintainer")
            return "维护者";
        else if(field == "Architecture")
            return "架构";
        else if(field == "Version")
            return "版本";
        else if(field == "Depends")
            return "依赖";
        else if(field == "Description")
            return "简介";
        else if(field == "Homepage")
            return "主页";
        else if(field == "Replaces")
            return "代替";
        else if(field == "Provides")
            return "提供";
        else if(field == "Conflicts")
            return "冲突";
        else if(field == "Conffiles")
            return "配置文件";
        else if(field == "Recommends")
            return "推荐";
        else if(field == "Breaks")
            return "破坏";
        else if(field == "Suggests")
            return "建议";
        else if(field == "Source")
            return "源码";
        else if(field == "Multi-Arch")
            return "多架构";
        else if(field == "License")
            return "版权";
        else if(field == "Vendor")
            return "供应商";
        else if(field == "Build-Depends")
            return "构建依赖";
        else if(field == "Standards-Version")
            return "标准版本";
        else if(field == "Pre-Depends")
            return "预先依赖";
        else
            return field;
    }

private:
    Package m_package;
};

class PackageDetailView : public QTableView {
    Q_OBJECT
public:
    PackageDetailView(PackageInfoView *parent) : QTableView(parent) {
        setColumnWidth(0, 250);
        horizontalHeader()->setStretchLastSection(true);
//        horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        // packageInfoWidget->verticalHeader()->setDefaultSectionSize(1);
        setAlternatingRowColors(true);
        setWordWrap(true);
        // packageInfoWidget->setTextElideMode(Qt::ElideMiddle);
        setModel(new PackageModel(this));
        // packageInfoWidget->resizeRowsToContents();

//        verticalHeader()->hide();
        verticalHeader()->setFixedWidth(180);
        horizontalHeader()->hide();

        setStyleSheet("QTableView::item {padding-top: 10px; padding-bottom: 10px;}");
    }

    void refresh(const QString &packageName) {
        if(m_packageName != packageName) {
            m_packageName = packageName;
            PackageModel *m = qobject_cast<PackageModel*>(model());
            m->clear();
            m->initData(DesktopFileModel::instance()->getPackageInfo(packageName));
        }
    }

private:
    QString m_packageName;
};

class PackagePackView : public QWidget {
    Q_OBJECT
public:
    PackagePackView(QWidget *parent) : QWidget(parent) {
        QVBoxLayout *layout = new QVBoxLayout(this);

        m_label = new QLabel(this);
        QFont font = m_label->font();
        font.setBold(true);
        font.setPixelSize(32);
        m_label->setFont(font);
        layout->addWidget(m_label, 0, Qt::AlignLeft | Qt::AlignTop);

        m_listWidget = new QListWidget(this);
        m_listWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        layout->addWidget(m_listWidget, 1);

        m_packageButton = new QPushButton("开始提取", this);
        connect(m_packageButton, &QPushButton::clicked, this, [this]{
            QtConcurrent::run([this]{
                static QStringList debianFiles{"control", "conffiles", "config", "copyright", "list", "md5sums", "postinst", "postrm", "preinst", "prerm", "shlibs", "symbols", "templates", "triggers"};

                emit message("创建打包根目录");
                QDir dir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
                const QString parentPath = dir.absolutePath();
                const QString deb = dir.absoluteFilePath(m_packageName + ".deb");
                dir.mkdir(m_packageName);
                dir.cd(m_packageName);
                dir.mkdir("DEBIAN");
                const QString rootPath = dir.absolutePath();

                emit message("拷贝包文件");
                for(QString file : DesktopFileModel::instance()->getPackageFiles(m_packageName)) {
                    QFileInfo info(file);
                    if(info.isDir()) {
                        m_listWidget->addItem("创建目录 " + file);
                        dir.mkpath(file.remove(0, 1));
                    } else if(info.isFile()) {
                        m_listWidget->addItem("拷贝文件: " + file);
                        QFile::copy(file, rootPath + file);
                    }
                }

                emit message("拷贝包控制文件");
                for(auto debian : debianFiles) {
                    QString file("/var/lib/dpkg/info/" + m_packageName + "." + debian);
                    if(QFileInfo(file).exists()) {
                        m_listWidget->addItem("拷贝DEBIAN文件: " + debian);
                        QFile::copy(file, rootPath + "/DEBIAN/" + debian);
                    }
                }

                QProcess process;
                m_listWidget->addItem("生成control文件");
                process.start("dpkg", {"--status", m_packageName});
                if(process.waitForStarted() && process.waitForFinished()) {
                    QByteArray reply = process.readAll();
                    if(!reply.isEmpty()) {
                        QFile control(rootPath + "/DEBIAN/control");
                        control.open(QFile::WriteOnly | QFile::Text);
                        QTextStream stream(&control);
                        for(QString s : QString(reply).split("\n", Qt::SkipEmptyParts))
                            if(!s.contains("Status: install ok installed"))
                                stream << s << "\n";
                    }
                }
                process.close();
                emit message("开始打包" + m_packageName);
                m_listWidget->addItem("开始打包: " + deb);
                process.start("dpkg-deb", {"--build", rootPath, deb});
                if(process.waitForStarted() && process.waitForFinished()) {
                    QByteArray reply = process.readAll();
                    process.close();
                    if(!reply.isEmpty())
                        m_listWidget->addItems(QString(reply).split("\n", Qt::SkipEmptyParts));
                    if(QFileInfo(deb).exists()) {
                        emit message("打包完成： " + m_packageName);
                        m_listWidget->addItem("打包完成: " + deb);
                        m_packageButton->setEnabled(false);
                        m_openButton->setEnabled(true);
                        m_copyButton->setEnabled(true);
                        dir.removeRecursively();
                    }
                } else
                    m_listWidget->addItem(process.errorString());
                process.close();
            });
        });

        QHBoxLayout *l = new QHBoxLayout;
        l->setSpacing(30);
        layout->addLayout(l);
        l->addWidget(m_packageButton, 0, Qt::AlignLeft);
        l->addStretch();

        m_openButton = new QPushButton("进入包所在目录", this);
        connect(m_openButton, &QPushButton::clicked, this, []{
            QProcess process;
            process.startDetached("xdg-open", {QStandardPaths::writableLocation(QStandardPaths::CacheLocation)});
        });
        l->addWidget(m_openButton, 0, Qt::AlignRight);

        m_copyButton = new QPushButton("拷贝包", this);
        connect(m_copyButton, &QPushButton::clicked, this, [this]{
            QClipboard *clip = QGuiApplication::clipboard();
            QMimeData *data = new QMimeData;
            QString deb(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/" + m_packageName + ".deb");
            data->setUrls(QList<QUrl>{QUrl::fromLocalFile(deb)});
            data->setText(deb);
            QByteArray ba = "copy";
            ba.append("\n").append(QUrl::fromLocalFile(deb).toString());
            data->setData("x-special/gnome-copied-files", ba);
            clip->setMimeData(data);
        });
        l->addWidget(m_copyButton, 0, Qt::AlignRight);
    }

    void refresh(QString packageName) {
        if(m_packageName != packageName) {
            m_packageName = packageName;

            m_label->setText(m_packageName);

            bool exist = QFileInfo(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/" + m_packageName + ".deb").exists();

            m_packageButton->setEnabled(!exist);
            m_openButton->setEnabled(exist);
            m_copyButton->setEnabled(exist);
        }
    }

signals:
    void message(QString);

private:
    QString m_packageName;

    QLabel *m_label;
    QListWidget *m_listWidget;
    QPushButton *m_packageButton;
    QPushButton *m_openButton;
    QPushButton *m_copyButton;
};

PackageInfoView::PackageInfoView(QWidget *parent) : QTabWidget(parent),
    m_detailView(new PackageDetailView(this)),
    m_treeView(new PackageTreeView(this)),
    m_packView(new PackagePackView(this))
{
    setTabShape(QTabWidget::Rounded);
    setTabPosition(QTabWidget::South);

    addTab(m_detailView, "包信息");
    addTab(m_treeView, "包文件");
    addTab(m_packView, "包提取");

    connect(m_treeView, &PackageTreeView::startLoad, this, &PackageInfoView::startLoad);
    connect(m_treeView, &PackageTreeView::finishLoad, this, &PackageInfoView::finishLoad);
    connect(m_treeView, &PackageTreeView::message, this, &PackageInfoView::message);

    connect(m_packView, &PackagePackView::message, this, &PackageInfoView::message);

    connect(this, &PackageInfoView::currentChanged, this, [this](const int &index){
        emit typeChanged(fromViewTypeToString((ViewType)index));
        refresh();
    });
}

void PackageInfoView::setPackageName(QString packageName, QString fragment) {
    ViewType type = fromStringToViewType(fragment);
    packageName = packageName.split("/", Qt::SkipEmptyParts).first();

    if(m_packageName != packageName) {
        m_packageName = packageName;
        if(currentType() == type)
            refresh();
    }

    if(currentType() != type)
        setCurrentIndex((int)type);    
}

void PackageInfoView::refresh() {
    switch(currentType()){
    case Info:
        m_detailView->refresh(m_packageName);
        break;
    case File:
        m_treeView->refresh(m_packageName);
        break;
    case Package:
        m_packView->refresh(m_packageName);
        break;
    }
}

void PackageInfoView::focusInEvent(QFocusEvent *event) {
    QTabWidget::focusInEvent(event);
    currentWidget()->setFocus();
}

PackageInfoView::ViewType PackageInfoView::currentType() const {
    return (ViewType)currentIndex();
}

#include "packageinfoview.moc"
