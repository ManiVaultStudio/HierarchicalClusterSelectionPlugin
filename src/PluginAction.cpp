#include "PluginAction.h"

#include <ViewPlugin.h>


using namespace mv::gui;

PluginAction::PluginAction(QObject* parent, ViewPlugin* plugin, const QString& title) :
    WidgetAction(parent,title),
    _plugin(plugin)
{
    _plugin->getWidget().addAction(this);

    setText(title);
    setToolTip(title);
}

