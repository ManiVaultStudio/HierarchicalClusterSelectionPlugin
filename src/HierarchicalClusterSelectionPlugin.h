#pragma once



// Local includes

#include "SelectedDatasetsAction.h"

#include "SettingsAction.h"

// HDPS includes
#include <ViewPlugin.h>
#include <Dataset.h>

#include "actions/Actions.h"
#include "actions/VariantAction.h"




using hdps::plugin::ViewPluginFactory;
using hdps::plugin::ViewPlugin;


class Points;
class Clusters;


namespace hdps {
    namespace gui {
        class DropWidget;
    }
}

namespace  CytosploreViewerPlugin
{
    class HierarchicalClusterSelectionPlugin : public ViewPlugin
    {
        Q_OBJECT

            typedef std::pair<QString, std::vector<ptrdiff_t>> DimensionNameMatch;


    public:
        HierarchicalClusterSelectionPlugin(const hdps::plugin::PluginFactory* factory);

        QString getOriginalName() const;

        void init() override;

        void onDataEvent(hdps::DataEvent* dataEvent);

        /**
        * Load one (or more datasets in the view)
        * @param datasets Dataset(s) to load
        */
        void loadData(const hdps::Datasets& datasets) override;


        void addConfigurableWidget(const QString& name, QWidget* widget);
        QWidget* getConfigurableWidget(const QString& name);

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


        void publishAndSerializeAction(WidgetAction* w, bool serialize = true);


        hdps::Dataset<hdps::DatasetImpl>& getDataset(qsizetype index)
        {

            return _selectedDatasetsAction.getDataset(index);
        }


       
        void datasetChanged(qsizetype index, const hdps::Dataset<hdps::DatasetImpl>& dataset);


        QStringList getSelection() const;

    protected slots:

        void datasetAdded(int index);
        void updateModel();
        void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

    private:
        const QString                       _originalName;
        QStandardItemModel                  _model;
        QTreeView*                          _treeView;

        QVector<WidgetAction*>              _serializedActions;

        //actions
        SelectedDatasetsAction              _selectedDatasetsAction;
     
        SettingsAction                      _settingsAction;

        OptionsAction                       _selectedOptionsAction;
        VariantAction                       _commandAction;
    };



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
}
