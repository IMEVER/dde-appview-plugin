#ifndef APPPLUGIN_H
#define APPPLUGIN_H

//#include "dfmviewplugin.h"

//class AppPlugin : public DFM_NAMESPACE::DFMViewPlugin
//{
//    Q_OBJECT
//    Q_PLUGIN_METADATA(IID DFMViewFactoryInterface_iid FILE "appView.json")
//    Q_INTERFACES(DFM_NAMESPACE::DFMViewPlugin)

//public:
//    explicit AppPlugin();

//public slots:
//    DFM_NAMESPACE::DFMBaseView *create(const QString &key) override;
//};


#include "../../dde-file-manager-lib/plugins/schemeplugininterface.h"

class AppPlugin : public QObject , public SchemePluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DFMFileSchemePluginInterface_iid FILE "resource/appView.json")
    Q_INTERFACES(SchemePluginInterface)

public:
    explicit AppPlugin();

     ~AppPlugin() = default;

    // 创建SiseBarItem对应的右侧BaseView的回调函数
    ViewCreatorFunc createViewTypeFunc();

    // 初始化SideBarItem，添加自定义的图标，文字，URL等构造信息
    dde_file_manager::DFMSideBarItem *createSideBarItem();

    // 初始化SideBarItem对应的Handler，可重载右键菜单功能
    SideBarInterfaceCreaterFunc createSideBarInterfaceTypeFunc();

    // 初始化面包屑控制器CrumbControl
    CrumbCreaterFunc createCrumbCreaterTypeFunc();

    // 返回插件的名称，用于判断插件的类型，与SideBarItem、DFMBaseView、CrumbControl关联起来
    // [protocol]://[host]:[port]/[path]?[query]#[hash] 对应于QUrl::host()返回的Url
    QString pluginName();

    // 查询插件SideBarItem是否支持拖拽
    bool isSideBarItemSupportedDrop();
    // 向SideBarItem拖拽文件调用DBus接口
    bool dropFileToPlugin(QList<DUrl> srcUrls, DUrl desUrl);

public slots:
};


#endif // APPPLUGIN_H
