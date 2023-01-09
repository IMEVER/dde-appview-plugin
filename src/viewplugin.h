// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VIEWPLUGIN_H
#define VIEWPLUGIN_H

#include "dfmbaseview.h"
//#include  "../plugininterfaces/view/viewinterface.h"
#include "appview.h"

#include <QObject>

class ViewPlugin : public QObject, public DFM_NAMESPACE::DFMBaseView/*, public ViewInterface*/
{
    Q_OBJECT
//    Q_PLUGIN_METADATA(IID ViewInterface_iid FILE "appView.json")
//    Q_INTERFACES(ViewInterface)
public:
    explicit ViewPlugin(QObject *parent=nullptr);
    ~ViewPlugin();

//    QString bookMarkText() override;
//    QIcon bookMarkNormalIcon() override;
//    QIcon bookMarkHoverIcon() override;
//    QIcon bookMarkPressedIcon() override;
//    QIcon bookMarkCheckedIcon() override;
//    QString crumbText() override;
//    QIcon crumbNormalIcon() override;
//    QIcon crumbHoverIcon() override;
//    QIcon crumbPressedIcon() override;
//    QIcon crumbCheckedIcon() override;
//    bool isAddSeparator() override;
//    QString scheme() override;
//    QWidget* createView() override;

    QWidget *widget() const override;
    DUrl rootUrl() const override;
    ViewState viewState() const override;
    bool setRootUrl(const DUrl &url) override;
    void refresh() override;

private:
    AppView *appView;
    DFMBaseView::ViewState state;
};

#endif // VIEWPLUGIN_H
