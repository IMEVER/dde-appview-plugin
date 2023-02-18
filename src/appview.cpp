#include "appview.h"

#include "desktoplistview.h"
#include "packageinfoview.h"

#include <QLayout>
#include <QStackedWidget>

AppView::AppView(QWidget *parent) :QWidget(parent), m_desktopListView(nullptr), m_desktopInfolView(nullptr)
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    QVBoxLayout *leftLayout = new QVBoxLayout;
    mainLayout->addLayout(leftLayout);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_mainWidget = new QStackedWidget(this);
    m_mainWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    leftLayout->addWidget(m_mainWidget);

    statusBar = new QLabel(this);
    statusBar->setAlignment(Qt::AlignCenter);
    statusBar->setFixedHeight(24);
    statusBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    statusBar->setText("加载中......");
    leftLayout->addWidget(statusBar);
}

DUrl AppView::rootUrl() {
    return m_rootUrl;
}

bool AppView::setRootUrl(DUrl url) {
    auto isSame = [](const DUrl &newUrl, const DUrl &oldUrl) {
        QString newPath = newUrl.path();
        QString oldPath = oldUrl.path();

        if(!newPath.isEmpty() && newPath.startsWith('/')) newPath.remove(0, 1);
        if(!newPath.isEmpty() && newPath.endsWith('/')) newPath.remove(newPath.count()-1, 1);

        if(!oldPath.isEmpty() && oldPath.startsWith('/')) oldPath.remove(0, 1);
        if(!oldPath.isEmpty() && oldPath.endsWith('/')) oldPath.remove(oldPath.count()-1, 1);

        return newPath == oldPath;
    };
    if(url.scheme() == PLUGIN_SCHEME && url.host() == "app") {
        if(!m_rootUrl.isValid() || !isSame(url, m_rootUrl) || (m_mainWidget->currentIndex() == 1 && m_rootUrl.fragment() != url.fragment())) {
            m_rootUrl = url;
            refresh(false);
        }
        return true;
    }
    return false;
}

void AppView::refresh(bool force) {
    if(m_rootUrl.path().isEmpty() || m_rootUrl.path() == "/") {
        if(!m_desktopListView) {
//            statusBar->setText("加载中......");
            m_desktopListView = new DesktopListView(this);
            connect(m_desktopListView, &DesktopListView::requestCd, this, [this](DUrl url) {
                setRootUrl(url);
                emit urlChanged();
            });
            connect(m_desktopListView, &DesktopListView::startLoad, this, [this] {
                statusBar->setText("加载中......");
                emit startLoad();
            });
            connect(m_desktopListView, &DesktopListView::fileCount, this, [this](int count){
                statusBar->setText(QString("%1 项").arg(count));
            });
            connect(m_desktopListView, &DesktopListView::finishLoad, this, &AppView::finishLoad);
            connect(m_desktopListView, &DesktopListView::message, statusBar, &QLabel::setText);
            m_mainWidget->addWidget(m_desktopListView);
        } else {
            if(force)
                m_desktopListView->refresh();
        }
        m_mainWidget->setCurrentWidget(m_desktopListView);
        m_desktopListView->setFocus();
    } else {
        if(m_rootUrl.hasFragment() == false) m_rootUrl.setFragment("info");
        if(!m_desktopInfolView) {
//            statusBar->setText("加载中......");
            m_desktopInfolView = new PackageInfoView(this);
            m_desktopInfolView->setPackageName(m_rootUrl.path(), m_rootUrl.fragment());
            m_mainWidget->addWidget(m_desktopInfolView);
            connect(m_desktopInfolView, &PackageInfoView::startLoad, this, [this]{
                statusBar->setText("加载中......");
                emit startLoad();
            });
            connect(m_desktopInfolView, &PackageInfoView::finishLoad, this, [this]{
                statusBar->setText("加载完成");
                emit finishLoad();
            });
            connect(m_desktopInfolView, &PackageInfoView::typeChanged, this, [this](QString type){
                if(m_rootUrl.fragment() != type) {
                    m_rootUrl.setFragment(type);
                    emit urlChanged();
                }
            });
            connect(m_desktopInfolView, &PackageInfoView::message, statusBar, &QLabel::setText);
        } else {
            if(!force)
                m_desktopInfolView->setPackageName(m_rootUrl.path(), m_rootUrl.fragment());
            else
                m_desktopInfolView->refresh();
        }
        m_mainWidget->setCurrentWidget(m_desktopInfolView);
        m_desktopInfolView->setFocus();
    }
}

