#pragma once

#include "actions/Actions.h"
#include "ViewPlugin.h"

using mv::plugin::ViewPlugin;

class PluginAction : public mv::gui::WidgetAction
{
public:
    PluginAction(QObject* parent, ViewPlugin* plugin, const QString& title);


protected:
    ViewPlugin* _plugin;
};
