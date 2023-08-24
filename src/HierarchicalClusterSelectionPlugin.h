#pragma once

// HDPS includes
#include <ViewPlugin.h>
#include <Dataset.h>

#include "actions/Actions.h"
#include "actions/VariantAction.h"

using namespace hdps::plugin;
using namespace hdps::gui;

namespace hdps {
    namespace gui {
        class DropWidget;
    }
}

// =============================================================================
// ViewPlugin
// =============================================================================

class HierarchicalClusterSelectionPlugin : public ViewPlugin
{
    Q_OBJECT

public:
    HierarchicalClusterSelectionPlugin(const hdps::plugin::PluginFactory* factory);

    void init() override;

public: // Serialization

    /**
    * Load plugin from variant map
    * @param Variant map representation of the plugin
    */
    void fromVariantMap(const QVariantMap& variantMap) override;

    /**
    * Save plugin to variant map
    * @return Variant map representation of the plugin
    */
    QVariantMap toVariantMap() const override;

private:
    /**
    * Load one (or more datasets in the view)
    * @param datasets Dataset(s) to load
    */
    void loadData(const hdps::Datasets& datasets) override;

    void datasetsChanged();

    QStringList getSelectionClusterNames() const;

    bool isInLoadedDatasets(const QString& id) const;

protected slots:

    void selectionChanged(const QItemSelection& selectedTreeModelIDs, const QItemSelection& deselectedTreeModelIDs);

private:
    const QString                       _originalName;
    QStandardItemModel                  _model;
    QTreeView*                          _treeView;
    std::vector<hdps::Dataset<hdps::DatasetImpl>>  _datasets;

    hdps::gui::DropWidget*              _dropWidget;

};

// =============================================================================
// Plugin factory
// =============================================================================

class HierarchicalClusterSelectionFactory : public ViewPluginFactory
{
    Q_OBJECT
        Q_PLUGIN_METADATA(IID   "nl.BioVault.HierarchicalClusterSelectionPlugin"
            FILE  "HierarchicalClusterSelectionPlugin.json")
        Q_INTERFACES(hdps::plugin::ViewPluginFactory hdps::plugin::PluginFactory)

public:
    HierarchicalClusterSelectionFactory() {}
    ~HierarchicalClusterSelectionFactory() override {}

    /** Returns the plugin icon */
    QIcon getIcon(const QColor& color = Qt::black) const override;

    HierarchicalClusterSelectionPlugin* produce() override;

    hdps::DataTypes supportedDataTypes() const override;

    hdps::gui::PluginTriggerActions getPluginTriggerActions(const hdps::Datasets& datasets) const override;
        
};
