// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "viewplugin.h"
#include <QLabel>

DFM_USE_NAMESPACE

ViewPlugin::ViewPlugin() :
    state(DFMBaseView::ViewIdle),
    QObject()
    /*ViewInterface(parent)*/
{
//    Q_INIT_RESOURCE(appview);

        appView = new AppView;
        connect(appView, &AppView::urlChanged, this, [this] {
//            notifyUrlChanged();
        });
        connect(appView, &AppView::startLoad, [this]{
            if(state == DFMBaseView::ViewIdle) {
                state = DFMBaseView::ViewBusy;
                notifyStateChanged();
            }
        });
        connect(appView, &AppView::finishLoad, [this]{
            if(state == DFMBaseView::ViewBusy) {
                state = DFMBaseView::ViewIdle;
                notifyStateChanged();
            }
        });
}

ViewPlugin::~ViewPlugin() {
    disconnect(appView, &AppView::startLoad, nullptr, nullptr);
    disconnect(appView, &AppView::finishLoad, nullptr, nullptr);
    appView->deleteLater();
}

//QString ViewPlugin::bookMarkText()
//{
//    return "应用";
//}

//QIcon ViewPlugin::bookMarkNormalIcon()
//{
//    return QIcon(":/images/release.png");
//}

//QIcon ViewPlugin::bookMarkHoverIcon()
//{
//    return QIcon(":/images/hover.png");
//}

//QIcon ViewPlugin::bookMarkPressedIcon()
//{
//    return QIcon(":/images/pressed.svg");
//}

//QIcon ViewPlugin::bookMarkCheckedIcon()
//{
//    return QIcon(":/images/checked.svg");
//}

//QString ViewPlugin::crumbText()
//{
//    return "应用";
//}

//QIcon ViewPlugin::crumbNormalIcon()
//{
//    return QIcon(":/images/release.png");
//}

//QIcon ViewPlugin::crumbHoverIcon()
//{
//    return QIcon(":/images/hover.png");
//}

//QIcon ViewPlugin::crumbPressedIcon()
//{
//    return QIcon(":/images/hover.png");
//}

//QIcon ViewPlugin::crumbCheckedIcon()
//{
//    return QIcon(":/images/checked.svg");
//}

//bool ViewPlugin::isAddSeparator()
//{
//    return true;
//}

//QString ViewPlugin::scheme()
//{
//    return "app";
//}

//QWidget *ViewPlugin::createView()
//{
//    if(nullptr == appView) {
//        appView = new AppView;
//    }
//    return appView;
//}

QWidget *ViewPlugin::widget() const {//QLabel *l=new QLabel;l->setText("test");return l;
    return appView;
}

DUrl ViewPlugin::rootUrl() const {//return DUrl("app:///");
    return appView->rootUrl();
}

DFMBaseView::ViewState ViewPlugin::viewState() const {
    return state;
}

bool ViewPlugin::setRootUrl(const DUrl &url) {//return true;
    return appView->setRootUrl(url);
}
void ViewPlugin::refresh() {
    appView->refresh();
}
