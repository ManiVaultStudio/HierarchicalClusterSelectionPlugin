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

    /**
    * Load one (or more datasets in the view)
    * @param datasets Dataset(s) to load
    */
    void loadData(const hdps::Datasets& datasets) override;

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
    * Repopulates the tree view, called when new data is loaded
    * @return 
    */
    void reloadTree();

    /**
    * Get the names of clusters that are currently selected
    * @return List GUI cluster names
    */
    QStringList getSelectionClusterNames() const;

    /**
    * Check if a data set is loaded
    * @param id: GUID of data set to be checked
    * @return Whether the checked data set is loaded
    */
    bool isInLoadedDatasets(const QString& id) const;

protected slots:

    /**
    * Notify core that the currently selected clusters are selected
    * @param id: GUID of selected clusters, unused (here for compatability with QItemSelectionModel::selectionChanged)
    * @param id: GUID of de-selected clusters, unused (here for compatability with QItemSelectionModel::selectionChanged)
    */
    void selectionChanged(const QItemSelection& selectedTreeModelIDs, const QItemSelection& deselectedTreeModelIDs);

private:
    QStandardItemModel                  _model;         // data model
    QTreeView*                          _treeView;      // hierarchical tree view
    hdps::Datasets                      _datasets;      // loaded data sets

    StringsAction                       _dataGUIDs;     // Action that serialized the loaded data sets
    hdps::gui::DropWidget*              _dropWidget;    // ManiVault widget to enable drag&drop data loading

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
