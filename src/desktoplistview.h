#ifndef DESKTOPLISTVIEW_H
#define DESKTOPLISTVIEW_H

#include <QListWidget>
#include <QMap>
#include <durl.h>
#include <com_deepin_dde_daemon_dock.h>

using DBusDock = com::deepin::dde::daemon::Dock;

class LauncherDbus;
class QTimer;
struct DesktopFileInfo;

class DesktopListView : public QListWidget
{
    Q_OBJECT
public:
    DesktopListView(QWidget *parent=nullptr);
    ~DesktopListView();
    void refresh();

protected:
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QList<QListWidgetItem *> items) const override;
    bool dropMimeData(int index, const QMimeData *data, Qt::DropAction action) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

signals:
    void startLoad();
    void finishLoad();
    void fileCount(int num);
    void requestCd(DUrl url);
    void message(QString msg);

private:
    QMap<QString, QListWidgetItem*> cachedItem;
    bool m_loading;
    LauncherDbus *launcher;
    DBusDock *dock;

    QLineEdit *m_searchEditor;
    QTimer *m_searchTimer;
    QFutureWatcher<DesktopFileInfo*> *m_watcher;
    bool m_update;
};

#endif // DESKTOPLISTVIEW_H
