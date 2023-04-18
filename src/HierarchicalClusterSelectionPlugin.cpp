#include "HierarchicalClusterSelectionPlugin.h"

// Local includes
#include "SelectedDatasetsAction.h"

// HDPS includes
#include "PointData/PointData.h"
#include "ClusterData/ClusterData.h"
#include "event/Event.h"


#include <actions/PluginTriggerAction.h>
#include <actions/WidgetAction.h>

// QT includes

#include <QMimeData>
#include <QDebug>


#include <iostream>
#include <cassert>
#include <set>
#include <algorithm>


#ifdef __cpp_lib_parallel_algorithm
#include <execution>
#endif

#include <omp.h>






Q_PLUGIN_METADATA(IID "nl.BioVault.HierarchicalClusterSelectionPlugin")
//Q_DECLARE_METATYPE(QWidget*)
using namespace hdps;
using namespace hdps::gui;
using namespace hdps::plugin;
using namespace hdps::util;

namespace 
{
    namespace local
    {

        template<typename T>
        bool is_exact_type(const QVariant& variant)
        {
            auto variantType = variant.metaType();
            auto requestedType = QMetaType::fromType<T>();
            return (variantType == requestedType);
        }
        template<typename T>
        T get_strict_value(const QVariant& variant)
        {
            if (is_exact_type<T>(variant))
                return variant.value<T>();
            else
            {
#ifdef _DEBUG
                qDebug() << "Error: requested " << QMetaType::fromType<T>().name() << " but value is of type " << variant.metaType().name();
#endif
                return T();
            }
        }

        


        QString fromCamelCase(const QString& s, QChar c = '_') {

            static QRegularExpression regExp1{ "(.)([A-Z][a-z]+)" };
            static QRegularExpression regExp2{ "([a-z0-9])([A-Z])" };

            QString result = s;

            QString s2("\\1");
            s2 += QString(c);
            s2 += "\\2";
            result.replace(regExp1, s2);
            result.replace(regExp2, s2);

            return result.toLower();

        }

        QString toCamelCase(const QString& s, QChar c = '_') {

            QStringList parts = s.split(c, Qt::SkipEmptyParts);
            for (int i = 1; i < parts.size(); ++i)
                parts[i].replace(0, 1, parts[i][0].toUpper());

            return parts.join("");

        }

        QStringList get_stringlist(QStandardItem* item)
        {
            QStringList result;
            if (item->hasChildren())
            {
                const int nrOfChilden = item->rowCount();
                for (int c = 0; c < nrOfChilden; ++c)
                {
                    result.append(get_stringlist(item->child(c, 0)));
                }
            }
            else
            {
                result << item->text();
            }
            return result;
        }

    }


    
}

namespace CytosploreViewerPlugin
{
    HierarchicalClusterSelectionPlugin::HierarchicalClusterSelectionPlugin(const hdps::plugin::PluginFactory* factory)
        : ViewPlugin(factory)
        , _originalName(getGuiName())
        , _selectedDatasetsAction(this,3,"SelectedDatasets", "Selected Datasets")

        , _settingsAction(this)
        , _treeView(nullptr)
        , _selectedOptionsAction(this, "Selection")
        , _commandAction(this,"command")
    {
        setSerializationName(getGuiName());

        for (qsizetype i = 0; i < _selectedDatasetsAction.size(); ++i)
        {
            datasetAdded(i);
        }


        connect(&_selectedDatasetsAction, &SelectedDatasetsAction::datasetAdded, this, &HierarchicalClusterSelectionPlugin::datasetAdded);

        publishAndSerializeAction(&_selectedOptionsAction, true);
        serializeAction(&_selectedDatasetsAction);
    	serializeAction(&_settingsAction);
        serializeAction(&_commandAction);
    }

    QString HierarchicalClusterSelectionPlugin::getOriginalName() const
    {
        return _originalName;
    }


    void HierarchicalClusterSelectionPlugin::init()
    {
        QWidget& mainWidget = getWidget();
        auto mainLayout = new QGridLayout();
        delete mainWidget.layout();
        mainWidget.setLayout(mainLayout);

       // mainWidget.setAcceptDrops(true);
        mainWidget.setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

        const int margin = 0;
        mainLayout->setContentsMargins(margin, margin, margin, margin);
        mainLayout->setSpacing(0);

        
        _settingsAction.addAction(_selectedDatasetsAction, 1);
        _settingsAction.addAction(_selectedOptionsAction, 2);
        auto *widget = _settingsAction.createWidget(&mainWidget, 1);
        widget->hide();
        mainLayout->addWidget(widget, 0, 0);

        

       _treeView = new QTreeView(&mainWidget);
        _treeView->setModel(&_model);
        _treeView->setHeaderHidden(true);
        _treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        _treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        
        QItemSelectionModel *selectionModel = _treeView->selectionModel();
        connect(selectionModel, &QItemSelectionModel::selectionChanged, this, &HierarchicalClusterSelectionPlugin::selectionChanged);
        mainLayout->addWidget(_treeView, 1, 0);

        
        
    }


    void HierarchicalClusterSelectionPlugin::onDataEvent(hdps::DataEvent* dataEvent)
    {
        // Event which gets triggered when a dataset is added to the system.
        if (dataEvent->getType() == EventType::DataAdded)
        {
            //    _differentialExpressionWidget->addDataOption(dataEvent->getDataset()->getGuiName());
        }
        // Event which gets triggered when the data contained in a dataset changes.
        if (dataEvent->getType() == EventType::DataChanged)
        {
            //dataEvent->getDataset()
        }
    }

    void HierarchicalClusterSelectionPlugin::loadData(const hdps::Datasets& datasets)
    {
        // Exit if there is nothing to load
        if (datasets.isEmpty())
            return;

        // Load the first dataset
        getDataset(0) = datasets.first();

        if (datasets.size() > 1)
            getDataset(1) = datasets[1];
    }



    void HierarchicalClusterSelectionPlugin::fromVariantMap(const QVariantMap& variantMap)
    {
        ViewPlugin::fromVariantMap(variantMap);
        auto version = variantMap.value("HierarchicalClusterSelectionPluginVersion", QVariant::fromValue(uint(0))).toUInt();
        if (version > 0)
        {
            for (auto action : _serializedActions)
            {
                if (variantMap.contains(action->getSerializationName()))
                    action->fromParentVariantMap(variantMap);

            }
        }

        _selectedOptionsAction.connectToPublicActionByName("Cluster Differential Expression 1::SelectClusters1");
        _selectedOptionsAction.connectToPublicActionByName("Cluster Differential Expression 1::SelectClusters2");
        _selectedOptionsAction.connectToPublicActionByName("Cluster Differential Expression 1::SelectClusters3");
        
        _treeView->expandRecursively(QModelIndex(), 1);
    }

    QVariantMap HierarchicalClusterSelectionPlugin::toVariantMap() const
    {
        QVariantMap variantMap = ViewPlugin::toVariantMap();
        variantMap["HierarchicalClusterSelectionPluginVersion"] = 2;
        for (auto action : _serializedActions)
        {
            assert(action->getSerializationName() != "#Properties");
            action->insertIntoVariantMap(variantMap);
        }
        
        return variantMap;

    }

    void HierarchicalClusterSelectionPlugin::serializeAction(WidgetAction* w)
    {
        assert(w != nullptr);
        if (w == nullptr)
            return;
        QString name = w->text();
        assert(!name.isEmpty());
        QString apiName = local::toCamelCase(name, ' ');
        w->setSerializationName(apiName);
    	_serializedActions.append(w);
    }

    void HierarchicalClusterSelectionPlugin::publishAndSerializeAction(WidgetAction* w, bool serialize)
    {
        assert(w != nullptr);
        if (w == nullptr)
            return;
        QString name = w->text();
        assert(!name.isEmpty());
        QString apiName = local::toCamelCase(name, ' ');
        w->setConnectionPermissionsFlag(ConnectionPermissionFlag::All);
        w->publish(_originalName + "::" + apiName);
        w->setSerializationName(apiName);
        if (serialize)
            _serializedActions.append(w);
    }



    

    namespace  local
    {
	    void get_recursive_cluster_tree(QStandardItem *item, Dataset<hdps::DatasetImpl> currentDataset, const QVector<QString> &hierarchy, qsizetype h,  bool firstTime, bool intersection = true, const std::vector<uint32_t> &indices = {})
	    {
            if(h >= hierarchy.size())
                return;
            
            auto childDatasets = currentDataset->getChildren({ ClusterType });
            for (qsizetype c=0; c < childDatasets.size(); ++c)
            {
                if (childDatasets[c]->getGuiName() == hierarchy[h])
                {
                    hdps::Dataset<Clusters> clusterData = childDatasets[c];
                    auto clusters = clusterData->getClusters();

                   
                    if(intersection && !firstTime)
                    {
                        QSet<QString> clusterNames;
                        for (auto cluster : clusters)
                        {
                            QString name = cluster.getName();
                            clusterNames.insert(name);
                        }
                        for (qsizetype row = item->rowCount()-1; row >=0; --row)
                        {
                            QStandardItem* child = item->child(row, 0);
                            if(!clusterNames.contains(child->text()))
                            {
                                item->removeRow(row);
                            }
                        }
                    }
                    
                    for (auto cluster : clusters)
                    {
                        QString name = cluster.getName();
                        QStandardItem* correspondingItem = nullptr;
                        
                        if(!firstTime)
                        {
                           for(qsizetype row = 0; row < item->rowCount(); ++row)
                           {
                               QStandardItem* child = item->child(row, 0);
                               if (child->text() == name)
                               {
                                   correspondingItem = child;
									break;
                               }
                           }
                           if (intersection && (correspondingItem == nullptr))
                               continue;;
                        }

                        
                        
                        {
                            std::vector<uint32_t> clusterIndices = cluster.getIndices();
                            std::sort(clusterIndices.begin(), clusterIndices.end());
                            std::vector<uint32_t> intersectingIndices;
                            if (h == 0)
                                intersectingIndices = clusterIndices;
                            else
                            {
                                std::set_intersection(indices.cbegin(), indices.cend(), clusterIndices.cbegin(), clusterIndices.cend(), std::back_inserter(intersectingIndices));
                            }
                            if (!intersectingIndices.empty())
                            {
                               if(correspondingItem == nullptr)
                               {
                                   QPixmap pixmap(16, 16);
                                   pixmap.fill(cluster.getColor());
                                   correspondingItem = new QStandardItem(pixmap,name);
                                   correspondingItem->setData(h, Qt::UserRole);
									
                                   item->appendRow(correspondingItem);
                               }
                            	get_recursive_cluster_tree(correspondingItem, currentDataset, hierarchy, h + 1, firstTime, intersection, intersectingIndices);
                            }
                     
                        }
                    }
                    break;
                }
            }
	    }
    }

    void HierarchicalClusterSelectionPlugin::datasetChanged(qsizetype index, const hdps::Dataset<hdps::DatasetImpl>& dataset)
    {
        const qsizetype NrOfDatasets = _selectedDatasetsAction.size();

        
        QVector<QString> hierarchy = { "class", "subclass","cross_species_cluster" };

        for (qsizetype i = 0; i < _selectedDatasetsAction.size(); ++i)
        {
	        if(!_selectedDatasetsAction.getDataset(i).isValid())
                return;
        }
        _model.clear();
      //  _model.invisibleRootItem()->setData("Hierarchy", Qt::DisplayRole);

        QStringList finalLevelItems;
        for (qsizetype i = 0; i < _selectedDatasetsAction.size(); ++i)
        {
            
            Dataset<hdps::DatasetImpl> currentDataset = _selectedDatasetsAction.getDataset(i).get();


            auto childDatasets = currentDataset->getChildren({ ClusterType });
            for (qsizetype c = 0; c < childDatasets.size(); ++c)
            {
                if (childDatasets[c]->getGuiName() == hierarchy.last())
                {
                    hdps::Dataset<Clusters> clusterData = childDatasets[c];
                    auto clusters = clusterData->getClusters();

                    for (auto cluster : clusters)
                    {
                        QString name = cluster.getName();
                        finalLevelItems.append(name);
                    }
                }
            }


            local::get_recursive_cluster_tree(_model.invisibleRootItem(), currentDataset, hierarchy, 0, (i==0), true);
            
        }


        _selectedOptionsAction.setOptions(finalLevelItems);
    	if(finalLevelItems.count())
			_selectedOptionsAction.selectOption(finalLevelItems.first());
    }












    void HierarchicalClusterSelectionPlugin::datasetAdded(int index)
    {
        connect(&_selectedDatasetsAction.getDataset(index), &Dataset<DatasetImpl>::changed, this, [this, index](const hdps::Dataset<hdps::DatasetImpl>& dataset) {datasetChanged(index, dataset); });
        //connect(&_loadedDatasetsAction.getClusterSelectionAction(index), &OptionsAction::selectedOptionsChanged, this, &HierarchicalClusterSelectionPlugin::clusterSelectionChanged);
        connect(&_selectedDatasetsAction.getDatasetSelectedAction(index), &ToggleAction::toggled, this, [this, index](bool)
            {
                const hdps::Dataset<hdps::DatasetImpl>& dataset = this->_selectedDatasetsAction.getDataset(index);
                datasetChanged(index, dataset);
            });





    }

    void HierarchicalClusterSelectionPlugin::updateModel()
    {
        QVariantList hierarchy = {};
    }

    QStringList HierarchicalClusterSelectionPlugin::getSelection() const
    {
        QStringList selected;
        if(_treeView)
        {
            auto indexes = _treeView->selectionModel()->selectedIndexes();
            for (auto index : indexes)
            {
                auto* item = _model.itemFromIndex(index);
                selected.append(::local::get_stringlist(item));
            }

           // qDebug() << _treeView->selectionModel()->selectedIndexes().size() << "\t" << selected;
        }
        
        return selected;
    }

    void HierarchicalClusterSelectionPlugin::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
    {
        const auto& selection = getSelection();
        _selectedOptionsAction.setSelectedOptions(selection);

        
        for (qsizetype i = 0; i < _selectedDatasetsAction.size(); i++)
        {
            
            auto childDatasets = _selectedDatasetsAction.data(i)->currentDataset->getChildren({ ClusterType });
            
            for (qsizetype c = 0; c < childDatasets.size(); ++c)
            {
                if (childDatasets[c]->getGuiName() == "cross_species_cluster")
                {
                    
                    hdps::Dataset<Clusters> clusterData = childDatasets[c];
                    clusterData->setSelectionNames(selection);
                }
            }
        }
    }


    QIcon HierarchicalClusterSelectionFactory::getIcon(const QColor& color /*= Qt::black*/) const
    {
        return Application::getIconFont("FontAwesome").getIcon("table", color);
    }

    HierarchicalClusterSelectionPlugin* HierarchicalClusterSelectionFactory::produce()
    {
        return new HierarchicalClusterSelectionPlugin(this);
    }

    hdps::DataTypes HierarchicalClusterSelectionFactory::supportedDataTypes() const
    {
        DataTypes supportedTypes;
        supportedTypes.append(ClusterType);
        return supportedTypes;
    }

    hdps::gui::PluginTriggerActions HierarchicalClusterSelectionFactory::getPluginTriggerActions(const hdps::Datasets& datasets) const
    {

        PluginTriggerActions pluginTriggerActions;


        return pluginTriggerActions;
    }


}
