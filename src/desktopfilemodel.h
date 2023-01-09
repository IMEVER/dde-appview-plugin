#ifndef DESKTOPFILEMODEL_H
#define DESKTOPFILEMODEL_H

#include <QSet>
#include <QMap>
#include <QObject>
#include <QFileSystemWatcher>
#include <QStandardPaths>
#include <QString>

struct AppInfo;
class QProcess;

struct Package
{
    public:
    Package(){}
    Package(QString str)
    {
        QStringList infos = str.split("\n");
        QString name;
        QStringList desc;
        for(QString info : infos)
        {
            if (!info.at(0).isSpace() && info.contains(":"))
            {
                int index = info.indexOf(":");
                QString key = info.mid(0, index);
                QString value = info.mid(index+1);
                if(key == "Conffiles") {
                    if(!name.isEmpty()) {
                        data << QPair<QString, QString>(name, desc.join("\n"));
                        desc.clear();
                    }
                    name = "Conffiles";
                    desc.append(value.trimmed());
                } else if(key == "Description") {
                    if(!name.isEmpty()) {
                        data << QPair<QString, QString>(name, desc.join("\n"));
                        desc.clear();
                    }
                    name = "Description";
                    desc.append(value.trimmed());
                }
                else
                    data.append(QPair<QString, QString>(key, value.trimmed()));
            }
            else
                desc.append(info.trimmed());
        }
        if(!desc.isEmpty())
            data << QPair<QString, QString>(name, desc.join("\n"));
    }

    int count() const
    {
        return data.count();
    }

    QVector<QPair<QString, QString>> data;
};

class DesktopFileModel : public QObject {
    Q_OBJECT
private:
    explicit DesktopFileModel(QObject *parent=nullptr);
public:
    static DesktopFileModel *instance();
    void refresh();

    QString getPackageName(QString desktopFile);
    Package getPackageInfo(QString packageName);
    QStringList getPackageFiles(QString packageName);
    QPair<QString, QString> getPackageNameIcon(QString packageName);

    void requestCompletionList(QString search=QString());

private:
    void initDesktopFiles(const QString pathChanged=QString());
    void initPackageName(QProcess *process, const QString &desktopFile);
    void initPackageInfo(QProcess *process, QString packageName, AppInfo &info);
    void initPackageFile(QString packageName, AppInfo &info);

signals:
    void directoryChanged(QStringList addFiles, QStringList removeFiles);
    void startLoad();
    void completionFound(const QStringList completions);

private:

};

#endif // DESKTOPFILEMODEL_H
