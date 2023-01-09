#ifndef PACKAGEINFOVIEW_H
#define PACKAGEINFOVIEW_H

#include "packagetreeview.h"

#include <QTabWidget>

class PackageDetailView;
class PackagePackView;

class PackageInfoView : public QTabWidget
{
    Q_OBJECT
public:

    enum ViewType {
        Info = 0,
        File = 1,
        Package = 2
    };

    PackageInfoView(QWidget *parent=nullptr);
    void setPackageName(QString packageName, QString fragment);
    void refresh();

private:
    inline QString fromViewTypeToString(ViewType type) {
        if(type == Info) return "info";
        else if(type == File) return "file";
        else return "package";
    }

    inline ViewType fromStringToViewType(QString type) {
        if(type == "file") return File;
        else if(type == "package") return Package;
        else return Info;

    }

protected:
    void focusInEvent(QFocusEvent *event) override;

private:
    ViewType currentType() const;

signals:
    void startLoad();
    void finishLoad();
    void message(QString msg);
    void typeChanged(QString type);

private:
    QString m_packageName;

    PackageDetailView *m_detailView;
    PackageTreeView *m_treeView;
    PackagePackView *m_packView;
};

#endif // PACKAGEINFOVIEW_H
