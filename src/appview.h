#ifndef APPVIEW_H
#define APPVIEW_H

#include <durl.h>

#include <QObject>
#include <QWidget>
#include <QLabel>

class DesktopListView;
class PackageInfoView;
class QStackedWidget;

class AppView : public QWidget
{
    Q_OBJECT
public:
    explicit AppView(QWidget *parent = nullptr);
    DUrl rootUrl();
    bool setRootUrl(DUrl url);
    void refresh(bool force=true);

signals:
    void startLoad();
    void finishLoad();
    void urlChanged();

private:
    QStackedWidget *m_mainWidget;
    QLabel *statusBar;

    DUrl m_rootUrl;
    DesktopListView *m_desktopListView;
    PackageInfoView *m_desktopInfolView;
};

#endif // APPVIEW_H
