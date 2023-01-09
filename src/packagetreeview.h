#ifndef PACKAGETREEVIEW_H
#define PACKAGETREEVIEW_H

#include <QTreeView>
#include <QObject>

class PackageTreeView : public QTreeView
{
    Q_OBJECT
public:
    PackageTreeView(QWidget *parent=nullptr);
    void refresh(const QString &packageName);

protected:
    QItemSelectionModel::SelectionFlags	selectionCommand(const QModelIndex &index, const QEvent *event = nullptr) const override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

signals:
    void startLoad();
    void finishLoad();
    void message(QString);

private:
    QString m_packageName;
};

#endif // PACKAGETREEVIEW_H
