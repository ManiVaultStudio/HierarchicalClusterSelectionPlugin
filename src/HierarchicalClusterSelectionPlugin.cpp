#include "HierarchicalClusterSelectionPlugin.h"

// HDPS includes
#include "PointData/PointData.h"
#include "ClusterData/ClusterData.h"

#include <actions/PluginTriggerAction.h>
#include <actions/WidgetAction.h>
#include <widgets/DropWidget.h>
#include <DatasetsMimeData.h>

// QT includes
#include <QDebug>

#include <iostream>
#include <cassert>
#include <algorithm>

Q_PLUGIN_METADATA(IID "nl.BioVault.HierarchicalClusterSelectionPlugin")

using namespace hdps;
using namespace hdps::gui;
using namespace hdps::plugin;
using namespace hdps::util;

namespace local
{
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

    void get_recursive_cluster_tree(QStandardItem* item, Dataset<hdps::DatasetImpl> currentDataset, const QVector<QString>& hierarchy, qsizetype h, bool firstTime, bool intersection = true, const std::vector<uint32_t>& indices = {})
    {
        if (h >= hierarchy.size())
            return;

        auto childDatasets = currentDataset->getChildren({ ClusterType });
        for (qsizetype c = 0; c < childDatasets.size(); ++c)
        {
            if (childDatasets[c]->getGuiName() == hierarchy[h])
            {
                hdps::Dataset<Clusters> clusterData = childDatasets[c];
                auto clusters = clusterData->getClusters();


                if (intersection && !firstTime)
                {
                    QSet<QString> clusterNames;
                    for (auto cluster : clusters)
                    {
                        QString name = cluster.getName();
                        clusterNames.insert(name);
                    }
                    for (qsizetype row = item->rowCount() - 1; row >= 0; --row)
                    {
                        QStandardItem* child = item->child(row, 0);
                        if (!clusterNames.contains(child->text()))
                        {
                            item->removeRow(row);
                        }
                    }
                }

                for (auto cluster : clusters)
                {
                    QString name = cluster.getName();
                    QStandardItem* correspondingItem = nullptr;

                    if (!firstTime)
                    {
                        for (qsizetype row = 0; row < item->rowCount(); ++row)
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
                            if (correspondingItem == nullptr)
                            {
                                QPixmap pixmap(16, 16);
                                pixmap.fill(cluster.getColor());
                                correspondingItem = new QStandardItem(pixmap, name);
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

// =============================================================================
// Plugin
// =============================================================================

HierarchicalClusterSelectionPlugin::HierarchicalClusterSelectionPlugin(const hdps::plugin::PluginFactory* factory)
    : ViewPlugin(factory)
    , _originalName(getGuiName())
    , _treeView(nullptr)
    , _datasets()
    , _dropWidget(nullptr)
{
    setSerializationName(getGuiName());
}

void HierarchicalClusterSelectionPlugin::init()
{
    QWidget& mainWidget = getWidget();

    // create widgets
    _treeView = new QTreeView(&mainWidget);
    _dropWidget = new DropWidget(&mainWidget);

    // define main widget layout
    auto mainLayout = new QGridLayout();
    mainWidget.setAcceptDrops(true);
    mainWidget.setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    mainWidget.setLayout(mainLayout);

    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // set up cluster tree view
    _treeView->setModel(&_model);
    _treeView->setHeaderHidden(true);
    _treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        
    mainLayout->addWidget(_treeView, 1, 0);

    // connect tree entry to cluster selection
    QItemSelectionModel* selectionModel = _treeView->selectionModel();
    connect(selectionModel, &QItemSelectionModel::selectionChanged, this, &HierarchicalClusterSelectionPlugin::selectionChanged);

    // set up drag&drop
    _dropWidget->setDropIndicatorWidget(new DropWidget::DropIndicatorWidget(&getWidget(), "No data loaded", "Drag an item from the data hierarchy and drop it here to visualize data..."));

    _dropWidget->initialize([this](const QMimeData* mimeData) -> DropWidget::DropRegions {
        DropWidget::DropRegions dropRegions;

        const auto datasetsMimeData = dynamic_cast<const DatasetsMimeData*>(mimeData);

        if (datasetsMimeData == nullptr)
            return dropRegions;

        if (datasetsMimeData->getDatasets().count() > 1)
            return dropRegions;

        const auto dataset = datasetsMimeData->getDatasets().first();
        const auto datasetGuiName = dataset->text();
        const auto datasetId = dataset->getId();
        const auto dataType = dataset->getDataType();

        if (dataType == PointType) {

            const auto candidateDataset = _core->requestDataset(datasetId);

            if (isInLoadedDatasets(datasetId)) {
                dropRegions << new DropWidget::DropRegion(this, "Warning", "Data already loaded", "exclamation-circle", false);
            }
            else {
                dropRegions << new DropWidget::DropRegion(this, "Points", QString("Visualize %1 as parallel coordinates").arg(datasetGuiName), "map-marker-alt", true, [this, candidateDataset]() {
                    loadData({ candidateDataset });
                    _dropWidget->setShowDropIndicator(false);
                    });

            }
        }
        else {
            dropRegions << new DropWidget::DropRegion(this, "Incompatible data", "This type of data is not supported", "exclamation-circle", false);
        }

        return dropRegions;
        });
}

bool HierarchicalClusterSelectionPlugin::isInLoadedDatasets(const QString& id) const
{
    for (const auto& dataset : _datasets)
        if (id == dataset.getDatasetId())
            return true;

    return false;
}

void HierarchicalClusterSelectionPlugin::loadData(const hdps::Datasets& datasets)
{
    // Exit if there is nothing to load
    if (datasets.isEmpty())
        return;

    for (const auto& dataset : datasets)
        _datasets.push_back(dataset);

    datasetsChanged();
}

void HierarchicalClusterSelectionPlugin::datasetsChanged()
{
    _model.clear();

    QStringList setNames;

    for (const auto& dataset : _datasets)
    {
        if (!dataset.isValid())
            return;

        setNames << dataset->getGuiName();
    }

    qDebug() << "HierarchicalClusterSelectionPlugin: load " << setNames;

    const QVector<QString> hierarchy = { "class", "subclass","cross_species_cluster" };

    for (const auto& dataset : _datasets)
        local::get_recursive_cluster_tree(_model.invisibleRootItem(), dataset, hierarchy, 0, _datasets.front() == dataset, true);

}

void HierarchicalClusterSelectionPlugin::fromVariantMap(const QVariantMap& variantMap)
{
    ViewPlugin::fromVariantMap(variantMap);

    //auto version = variantMap.value("HierarchicalClusterSelectionPluginVersion", QVariant::fromValue(uint(0))).toUInt();
    //if (version > 0)
    //{
    //    for (auto action : _serializedActions)
    //    {
    //        if (variantMap.contains(action->getSerializationName()))
    //            action->fromParentVariantMap(variantMap);
    //    }
    //}
        
    _treeView->expandRecursively(QModelIndex(), 1);
}

QVariantMap HierarchicalClusterSelectionPlugin::toVariantMap() const
{
    QVariantMap variantMap = ViewPlugin::toVariantMap();

    //variantMap["HierarchicalClusterSelectionPluginVersion"] = 2;
    //for (auto action : _serializedActions)
    //{
    //    assert(action->getSerializationName() != "#Properties");
    //    action->insertIntoVariantMap(variantMap);
    //}
        
    return variantMap;

}

QStringList HierarchicalClusterSelectionPlugin::getSelectionClusterNames() const
{
    QStringList selected;
    if(_treeView)
    {
        auto indexes = _treeView->selectionModel()->selectedIndexes();
        for (const auto& index : indexes)
        {
            auto* item = _model.itemFromIndex(index);
            selected.append(local::get_stringlist(item));
        }
    }
        
    return selected;
}

void HierarchicalClusterSelectionPlugin::selectionChanged(const QItemSelection& selectedTreeModelIDs, const QItemSelection& deselectedTreeModelIDs)
{
    const auto& selectionClusterNames = getSelectionClusterNames();

    for (const auto& dataset : _datasets)
    {
        qDebug() << dataset->getGuiName();
        for (const auto& child : dataset->getChildren({ ClusterType }))
        {
            if (child->getGuiName() == "cross_species_cluster")
            {
                static_cast<hdps::Dataset<Clusters>>(child)->setSelectionNames(selectionClusterNames);
                qDebug() << selectionClusterNames;
            }
        }
    }
}

// =============================================================================
// Plugin factory
// =============================================================================

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
    supportedTypes.append(PointType);
    return supportedTypes;
}

hdps::gui::PluginTriggerActions HierarchicalClusterSelectionFactory::getPluginTriggerActions(const hdps::Datasets& datasets) const
{
    PluginTriggerActions pluginTriggerActions;

    return pluginTriggerActions;
}
