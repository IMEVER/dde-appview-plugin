#include "appplugin.h"

#include "viewplugin.h"

//#include "../../dde-file-manager-lib/shutil/fileutils.h"
//#include "../../dde-file-manager-lib/views/dfilemanagerwindow.h"
//#include "../../dde-file-manager-lib/views/windowmanager.h"
//#include "../../dde-file-manager-lib/views/dfmsidebar.h"
//#include "dfilemenu.h"
//#include "singleton.h"
//#include "../../dde-file-manager-lib/app/filesignalmanager.h"

#include "desktopfilemodel.h"

//DFM_USE_NAMESPACE

//AppPlugin::AppPlugin() : DFMViewPlugin() {
////    Q_INIT_RESOURCE(appview);
//}

//DFMBaseView *AppPlugin::create(const QString &key)
//{
//    Q_UNUSED(key)

//    ViewPlugin *p = new ViewPlugin();

//    return p;
//}


class Crumb : public DFM_NAMESPACE::DFMCrumbInterface {
    Q_OBJECT
public:
    Crumb(QObject *parent=nullptr) : DFM_NAMESPACE::DFMCrumbInterface(parent) {
        connect(DesktopFileModel::instance(), &DesktopFileModel::completionFound, this, [this](QStringList files){
            if(files.isEmpty()) return;
            emit completionFound(files);
            emit completionListTransmissionCompleted();
        });
    }

    bool supportedUrl(DUrl url) {
        return url.scheme() == PLUGIN_SCHEME && url.host() == "app";
    }

    QList<DFM_NAMESPACE::CrumbData> seprateUrl(const DUrl &url)
    {
        QList<DFM_NAMESPACE::CrumbData> list;
        list.append(DFM_NAMESPACE::CrumbData(DUrl("plugin://app"), "应用", "app-launcher"));

        if(!url.path().isEmpty() && url.path() != "/") {
            QString paths = url.path();
            if(paths.at(0) == '/') paths.remove(0, 1);
            if(paths.endsWith('/')) paths.remove(paths.count()-1, 1);
            QString part("plugin://app");
            QString p = paths.split('/').first();
                part.append("/").append(p);                
                DFM_NAMESPACE::CrumbData data(DUrl(part), p);
                QPair<QString, QString> pair = DesktopFileModel::instance()->getPackageNameIcon(p);
                if(pair.first.isEmpty() == false) {
                    data.displayText = pair.first;
                    if(pair.second.at(0) != '/')
                        data.iconName = pair.second;
                }
                list.append(data);
        }

        return list;
    }

    void requestCompletionList(const DUrl &url) {
        if(url.path().isEmpty() || url.path() == "/")
            DesktopFileModel::instance()->requestCompletionList();
    }
    void cancelCompletionListTransmission() {

    }

private:

};

class SideBar : public DFM_NAMESPACE::DFMSideBarItemInterface {
    Q_OBJECT
public:
    SideBar(QObject *parent) : DFM_NAMESPACE::DFMSideBarItemInterface(parent) {}

    QMenu * contextMenu(const DFM_NAMESPACE::DFMSideBar *sidebar, const DFM_NAMESPACE::DFMSideBarItem* item) override {
        QMenu *menu = DFM_NAMESPACE::DFMSideBarItemInterface::contextMenu(sidebar, item);
        while (menu->actions().count() > 2) {
            QAction *action = menu->actions().last();
            menu->removeAction(action);
            action->deleteLater();
        }
        return menu;
    }
};

AppPlugin::AppPlugin() {
    DesktopFileModel::instance();
}

// 创建SiseBarItem对应的右侧BaseView的回调函数
ViewCreatorFunc AppPlugin::createViewTypeFunc() {
    return ViewCreatorFunc(typeid(ViewPlugin).name(), []{ return new ViewPlugin; });
}

// 初始化SideBarItem，添加自定义的图标，文字，URL等构造信息
dde_file_manager::DFMSideBarItem *AppPlugin::createSideBarItem() {
    QIcon icon;
    icon.addFile(":/images/launcher_on.svg", QSize(32, 32), QIcon::Selected);
    icon.addFile(":/images/launcher_off.svg", QSize(32, 32), QIcon::Normal);
    return new DFM_NAMESPACE::DFMSideBarItem(icon, "应用", DUrl("plugin://app"), "bookmark");
}

// 初始化SideBarItem对应的Handler，可重载右键菜单功能
SideBarInterfaceCreaterFunc AppPlugin::createSideBarInterfaceTypeFunc() {
    return SideBarInterfaceCreaterFunc(pluginName(), [this]{ return new SideBar(this); });
}

// 初始化面包屑控制器CrumbControl
CrumbCreaterFunc AppPlugin::createCrumbCreaterTypeFunc() {
    return CrumbCreaterFunc(pluginName(), [this]{ return new Crumb(this); });
}

// 返回插件的名称，用于判断插件的类型，与SideBarItem、DFMBaseView、CrumbControl关联起来
// [protocol]://[host]:[port]/[path]?[query]#[hash] 对应于QUrl::host()返回的Url
QString AppPlugin::pluginName() {
    return "app";
}

// 查询插件SideBarItem是否支持拖拽
bool AppPlugin::isSideBarItemSupportedDrop() {
    return false;
}
// 向SideBarItem拖拽文件调用DBus接口
bool AppPlugin::dropFileToPlugin(QList<DUrl> srcUrls, DUrl desUrl) {
//        QStringList debs;
//        for(auto url : srcUrls) {
//            if(url.isLocalFile() && url.fileName().endsWith(".deb") && QFile(url.toLocalFile()).exists())
//                debs << url.toLocalFile();
//        }
//        if(!debs.isEmpty()) {
//            FileUtils::openFiles(debs);
//            return true;
//        }
    return false;
}

#include "appplugin.moc"
