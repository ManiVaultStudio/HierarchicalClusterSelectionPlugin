#pragma once



// Local includes

#include "SelectedDatasetsAction.h"

// MV includes
#include <cstdio>
#include <ViewPlugin.h>
#include <Dataset.h>

#include "actions/Actions.h"
#include "actions/VariantAction.h"





using mv::plugin::ViewPluginFactory;
using mv::plugin::ViewPlugin;


class Points;
class Clusters;


namespace mv {
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
        HierarchicalClusterSelectionPlugin(const mv::plugin::PluginFactory* factory);

        QString getOriginalName() const;

        void init() override;

        void onDataEvent(mv::DatasetEvent* dataEvent);


        /**
        * Load one (or more datasets in the view)
        * @param datasets Dataset(s) to load
        */
        void loadData(const mv::Datasets& datasets) override;


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

        void serializeAction(WidgetAction* w);
        void publishAndSerializeAction(WidgetAction* w, bool serialize = true);


        mv::Dataset<mv::DatasetImpl>& getDataset(qsizetype index)
        {

            return _selectedDatasetsAction.getDataset(index);
        }


       
        void datasetChanged(qsizetype index, const mv::Dataset<mv::DatasetImpl>& dataset);


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


        OptionsAction                       _selectedOptionsAction;
        VariantAction                       _commandAction;
    };



    class HierarchicalClusterSelectionFactory : public ViewPluginFactory
    {

            Q_INTERFACES(mv::plugin::ViewPluginFactory mv::plugin::PluginFactory)
            Q_OBJECT
            Q_PLUGIN_METADATA(IID   "nl.BioVault.HierarchicalClusterSelectionPlugin"
                FILE  "HierarchicalClusterSelectionPlugin.json")



    public:
        HierarchicalClusterSelectionFactory() {}
        ~HierarchicalClusterSelectionFactory() override {}

        /** Returns the plugin icon */
        QIcon getIcon(const QColor& color = Qt::black) const override;

        HierarchicalClusterSelectionPlugin* produce() override;

        mv::DataTypes supportedDataTypes() const override;


        mv::gui::PluginTriggerActions getPluginTriggerActions(const mv::Datasets& datasets) const override;

        
    };
}
