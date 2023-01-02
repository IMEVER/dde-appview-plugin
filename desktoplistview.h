#ifndef DESKTOPLISTVIEW_H
#define DESKTOPLISTVIEW_H

#include <QListWidget>
#include <QMap>

class DesktopFileModel;

class DesktopListView : public QListWidget
{
    Q_OBJECT
public:
    DesktopListView(QWidget *parent=nullptr);
    void refresh(bool force=false);

protected:
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QList<QListWidgetItem *> items) const override;
    bool dropMimeData(int index, const QMimeData *data, Qt::DropAction action) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *e) override;
    void dropEvent(QDropEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

signals:
    void startLoad();
    void finishLoad();
    void fileCount(int num);

private:
    DesktopFileModel *m_model;
    QMap<QString, QListWidgetItem*> cachedItem;
};

#endif // DESKTOPLISTVIEW_H
