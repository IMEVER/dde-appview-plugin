#include "appview.h"

#include "desktoplistview.h"

#include <QLayout>
#include <QStackedWidget>

AppView::AppView(QWidget *parent) :QWidget(parent), m_desktopListView(nullptr)
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
    if(url.scheme() == "app") {
        if(m_rootUrl != url) {
            m_rootUrl = url;
            refresh(false);
//            emit urlChanged();
        }
        return true;
    }
    return false;
}

void AppView::refresh(bool force) {
    if(m_rootUrl.host().isEmpty()) {
        if(!m_desktopListView) {
            m_desktopListView = new DesktopListView(this);
            connect(m_desktopListView, &DesktopListView::startLoad, this, [this] {
                statusBar->setText("加载中......");
                emit startLoad();
            });
            connect(m_desktopListView, &DesktopListView::fileCount, this, [this](int count){
                statusBar->setText(QString("%1 项").arg(count));
            });
            connect(m_desktopListView, &DesktopListView::finishLoad, this, &AppView::finishLoad);
            m_mainWidget->addWidget(m_desktopListView);
        } else {
           m_desktopListView->refresh(force);
           m_mainWidget->setCurrentWidget(m_desktopListView);
        }
    } else {

    }
}

