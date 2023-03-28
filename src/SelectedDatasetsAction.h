#pragma once

#include "PluginAction.h"

#include "actions/DatasetPickerAction.h"
#include "actions/OptionsAction.h"
#include "actions/TriggerAction.h"
#include "actions/VariantAction.h"


using namespace hdps::gui;


class HierarchicalClusterSelectionPlugin;



class SelectedDatasetsAction : public PluginAction
{
    Q_OBJECT
protected:

    struct Data : QStandardItem
    {
    public:
        Data() = delete;
        // Data(const Data&) = default;

        ~Data() = default;

        explicit Data(SelectedDatasetsAction* parent, int index = -1);


        virtual QStandardItem* clone() const;
        virtual QVariant data(int role = Qt::UserRole + 1) const override;
        virtual void 	setData(const QVariant& value, int role = Qt::UserRole + 1) override;

        DatasetPickerAction	      datasetPickerAction;

        hdps::Dataset<hdps::DatasetImpl>   currentDataset;
        StringAction              datasetNameStringAction;
        ToggleAction              datasetSelectedAction;
    };

    class Widget : public WidgetActionWidget {
    public:
        Widget(QWidget* parent, SelectedDatasetsAction* currentDatasetAction, const std::int32_t& widgetFlags);
    };

    QWidget* getWidget(QWidget* parent, const std::int32_t& widgetFlags) override {
        return new Widget(parent, this, widgetFlags);
    };

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

public:

    SelectedDatasetsAction(ViewPlugin* plugin, qsizetype initialNrOfDatasets = 2, QString instanceName="SelectedDatasets", QString tooltipName = "Selected Datasets");

    hdps::gui::ToggleAction& getDatasetSelectedAction(const std::size_t index);


    hdps::Dataset<hdps::DatasetImpl>& getDataset(std::size_t index) const;





    QWidget* getDatasetNameWidget(std::size_t index, QWidget* parent, const std::int32_t& flags);

    qsizetype size() const;

    Data* data(qsizetype index) const;


    QStandardItemModel& model();

public slots:
    void addDataset();
signals:
    void datasetAdded(int index);

protected:
    QString         _originalPluginName;
    TriggerAction   _addDatasetTriggerAction;
    QStandardItemModel  _model;
    //std::vector<QSharedPointer<Data>> _data;
    friend class Widget;
};
