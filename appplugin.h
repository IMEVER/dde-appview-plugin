#ifndef APPPLUGIN_H
#define APPPLUGIN_H

#include "dfmviewplugin.h"

class AppPlugin : public DFM_NAMESPACE::DFMViewPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DFMViewFactoryInterface_iid FILE "appView.json")
    Q_INTERFACES(DFM_NAMESPACE::DFMViewPlugin)

public:
    explicit AppPlugin();

public slots:
    DFM_NAMESPACE::DFMBaseView *create(const QString &key) override;
};

#endif // APPPLUGIN_H
