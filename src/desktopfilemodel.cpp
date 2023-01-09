#include "desktopfilemodel.h"

#include "../../dde-file-manager-lib/models/desktopfileinfo.h"

#include <QDir>
#include <QDirIterator>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QtConcurrent/QtConcurrent>
#include <QIcon>

struct AppInfo
{
public:
    QString m_icon;
    QString m_displayName;
    struct Package m_package;
    QStringList m_fileList;

    bool m_packageInfoChecked = false;
    bool m_fileListChecked = false;
};


static QSet<QString> m_files;//desktop files
static QMap<QString, QString> m_desktopPackage;//desktop => package
static QMap<QString, AppInfo> m_maps;//package => AppInfo

DesktopFileModel *DesktopFileModel::instance() {
    static DesktopFileModel *i = new DesktopFileModel;
    return i;
}

DesktopFileModel::DesktopFileModel(QObject *parent) : QObject(parent) {
    QStringList paths = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    QFileSystemWatcher *watcher = new QFileSystemWatcher(this);
    watcher->addPaths(paths);
    connect(watcher, &QFileSystemWatcher::directoryChanged, this, &DesktopFileModel::initDesktopFiles);
    initDesktopFiles();
}

QString DesktopFileModel::getPackageName(QString desktopFile) {
    if(!m_desktopPackage.contains(desktopFile)) {
        QProcess *process = new QProcess(this);
        initPackageName(process, desktopFile);
        process->deleteLater();
    }

    return m_desktopPackage.value(desktopFile);
}

Package DesktopFileModel::getPackageInfo(QString packageName) {
    if(!packageName.isEmpty() && m_desktopPackage.values().contains(packageName)) {
        AppInfo &info = m_maps[packageName];
        if(info.m_packageInfoChecked == false) {
            QProcess *process = new QProcess(this);
            initPackageInfo(process, packageName, info);
            process->deleteLater();
        }
        return info.m_package;
    }
    return {};
}

QStringList DesktopFileModel::getPackageFiles(QString packageName) {
    if(!packageName.isEmpty() && m_desktopPackage.values().contains(packageName)) {
        AppInfo &info = m_maps[packageName];
        if(!info.m_fileListChecked)
            initPackageFile(packageName, info);
        return info.m_fileList;
    }
    return {};
}

QPair<QString, QString> DesktopFileModel::getPackageNameIcon(QString packageName) {
    QPair<QString, QString> pair;
    if(!packageName.isEmpty() && m_desktopPackage.values().contains(packageName)) {
        AppInfo &info = m_maps[packageName];
        if(info.m_displayName.isEmpty()) {
            for(auto it = m_desktopPackage.cbegin(); it != m_desktopPackage.cend(); it++)
                if(it.value() == packageName) {
                    DesktopFileInfo fileInfo(DUrl::fromLocalFile(it.key()));
                    info.m_displayName = fileInfo.fileDisplayName();
                    info.m_icon = fileInfo.iconName();
                    break;
                }
        }
        pair.first = info.m_displayName;
        pair.second = info.m_icon;
    }

    return pair;
}

void DesktopFileModel::refresh() {
    if(m_files.isEmpty())
        initDesktopFiles();
    else
        QTimer::singleShot(10, this, [this]{
            QStringList files = m_files.values();
//            qSort(files.begin(), files.end(), [](const QString& s1, const QString& s2){
//                QString left = s1.mid(s1.lastIndexOf('/') + 1);
//                int pos =left.lastIndexOf('.', -9);
//                if(pos > -1) left = left.mid(pos+1, left.length()-pos-1);

//                QString right = s2.mid(s2.lastIndexOf('/') + 1);
//                pos = right.lastIndexOf('.', -9);
//                if(pos > -1) right = right.mid(pos+1, right.length()-pos-1);
//                return left < right;
//            });
            emit directoryChanged(files, {});
        });
}

void DesktopFileModel::requestCompletionList(QString search) {
    QtConcurrent::run([this, search] {
        if(!m_files.isEmpty()) {
            if(search.isEmpty()) {
                emit completionFound(m_maps.keys());
                return;
            }

            QStringList result;
            auto desktopFiles = m_maps.keys();
            for(auto f : desktopFiles) {
                if(f.mid(0, f.lastIndexOf(".desktop")).contains(search, Qt::CaseInsensitive))
                    result.append(f);
            }
            if(result.isEmpty() == false)
                emit completionFound(result);
        }
    });
}

void DesktopFileModel::initDesktopFiles(const QString pathChanged) {
    Q_UNUSED(pathChanged)
    emit startLoad();

    QtConcurrent::run([this]{
        QSet<QString> newFilesSet;
        QStringList addFiles, removeFiles;

        const QStringList paths = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
        for(auto path : paths) {
            QDirIterator it(path, QStringList("*.desktop"),
                            QDir::Files | QDir::NoDotAndDotDot,
                            QDirIterator::NoIteratorFlags);
            while (it.hasNext()) {
                it.next();
                QString current = it.filePath();
                QFileInfo fileInfo(current);
                while(fileInfo.exists() && fileInfo.isSymLink()) {
                    current = fileInfo.symLinkTarget();
                    fileInfo = QFileInfo(current);
                }

                if(!fileInfo.exists()) continue;

                newFilesSet.insert(current);
                if(!m_files.contains(current)) {
                    addFiles.append(current);
                    m_files.insert(current);
                }
            }
        }

        m_files -= newFilesSet;
        if(m_files.isEmpty() == false) {
            removeFiles.append(m_files.values());
            for(auto f : removeFiles) {
                QString package = m_desktopPackage.take(f);
                m_maps.remove(package);
            }
        }

//        if(addFiles.isEmpty() == false) {
//            qSort(addFiles.begin(), addFiles.end(), [](const QString& s1, const QString& s2){
//                QString left = s1.mid(s1.lastIndexOf('/') + 1);
//                int pos =left.lastIndexOf('.', -9);
//                if(pos > -1) left = left.mid(pos+1, left.length()-pos-1);

//                QString right = s2.mid(s2.lastIndexOf('/') + 1);
//                pos = right.lastIndexOf('.', -9);
//                if(pos > -1) right = right.mid(pos+1, right.length()-pos-1);
//                return left < right;
//            });
//        }

        emit directoryChanged(addFiles, removeFiles);
        m_files.swap(newFilesSet);

        if(!addFiles.isEmpty()) {
            QtConcurrent::run([addFiles] {
                QStringList newFiles;
                for(QString file : addFiles) {
                    if(m_desktopPackage.contains(file) == false)
                        newFiles.append(file);
                }
                if(newFiles.isEmpty() == false) {
                    QProcess process;
                    process.start("dpkg", QStringList()<<"-S"<<newFiles);
                    if(process.waitForStarted() && process.waitForFinished()) {
                        QByteArray reply = process.readAll();
                        if (reply.isEmpty() == false) {
                            for(QString package : QString(reply).split("\n", Qt::SkipEmptyParts)) {
                                QStringList info = package.split(": ");
                                m_desktopPackage.insert(info.at(1), info.first());
                                newFiles.removeOne(info.at(1));
                            }
                            for(auto f : newFiles) m_desktopPackage.insert(f, QString());
                        }
                    }
                    process.close();
                }
            });
        }
    });
}

void DesktopFileModel::initPackageName(QProcess *process, const QString &desktopFile) {
    QString &packageName = m_desktopPackage[desktopFile];
    process->start("dpkg", QStringList()<<"-S"<<desktopFile);
    if(process->waitForStarted() && process->waitForFinished()) {
        QByteArray reply = process->readLine();
        if (reply.isEmpty() == false)
            packageName.append(QString(reply).split(":").first());
    }
    process->close();
}

void DesktopFileModel::initPackageInfo(QProcess *process, QString packageName, AppInfo &info) {
    if(info.m_packageInfoChecked) return;

    info.m_packageInfoChecked = true;

    process->start("dpkg", {"--status", packageName});
    if (process->waitForStarted() && process->waitForFinished())
    {
        QByteArray reply = process->readAll();
        if (!reply.isEmpty())
            info.m_package = Package(reply);
    }
    process->close();

}

void DesktopFileModel::initPackageFile(QString packageName, AppInfo &info) {
    if(info.m_fileListChecked) return;

    info.m_fileListChecked = true;
    QProcess *process = new QProcess(this);

    process->start("dpkg", {"--listfiles", packageName});
    if (process->waitForStarted() && process->waitForFinished())
    {
        QByteArray reply = process->readAll();
        if (!reply.isEmpty())
            info.m_fileList = QString(reply).split("\n", Qt::SkipEmptyParts);

    }
    process->close();
    process->deleteLater();
}
