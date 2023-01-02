#include "appplugin.h"

#include "viewplugin.h"

DFM_USE_NAMESPACE

AppPlugin::AppPlugin() : DFMViewPlugin() {
//    Q_INIT_RESOURCE(appview);
}

DFMBaseView *AppPlugin::create(const QString &key)
{
    Q_UNUSED(key)

    ViewPlugin *p = new ViewPlugin();

    return p;
}
