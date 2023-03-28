#pragma once

#include "actions/Actions.h"
#include "ViewPlugin.h"


using hdps::plugin::ViewPlugin;

class PluginAction : public hdps::gui::WidgetAction
{
public:
    PluginAction(QObject* parent, ViewPlugin* plugin, const QString& title);


protected:
    ViewPlugin* _plugin;
};
