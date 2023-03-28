#include "SelectedDatasetsAction.h"
#include "HierarchicalClusterSelectionPlugin.h"



#include <cstdint>
#include <QMenu>
#include <Dataset.h>


using namespace hdps;
using namespace hdps::gui;

namespace
{
    namespace localNamespace
    {
        QString toCamelCase(const QString& s, QChar c = '_') {

            QStringList parts = s.split(c, Qt::SkipEmptyParts);
            for (int i = 1; i < parts.size(); ++i)
                parts[i].replace(0, 1, parts[i][0].toUpper());

            return parts.join("");

        }
    }

}

SelectedDatasetsAction::Data::Data(SelectedDatasetsAction* parent, int index)
    :QStandardItem()
    , datasetPickerAction(parent, "Dataset")

    , datasetNameStringAction(parent, "Dataset")
    , datasetSelectedAction(parent, "Active Dataset", true, true)
{


    if (index >= 0)
    {
        {
            QString datasetGuiName = QString("Dataset ") + QString::number(index + 1);

            datasetPickerAction.setText(datasetGuiName);
            datasetNameStringAction.setString(datasetGuiName);
            datasetNameStringAction.setText(datasetGuiName);
            datasetSelectedAction.setText("");
        }

        {
            QString baseName = parent->_plugin->getGuiName() + "::";
            {
                QString datasetPickerActionName = QString("Dataset") + QString::number(index + 1);
                datasetPickerAction.setConnectionPermissionsFlag(ConnectionPermissionFlag::All);
                datasetPickerAction.publish(baseName + datasetPickerActionName);
                datasetPickerAction.setSerializationName(datasetPickerActionName);
            }

            {
                QString datasetNameStringActionName = QString("DatasetName") + QString::number(index + 1);
                datasetNameStringAction.setConnectionPermissionsFlag(ConnectionPermissionFlag::All);
                datasetNameStringAction.publish(baseName + datasetNameStringActionName);
                datasetNameStringAction.setSerializationName(datasetNameStringActionName);
            }




            {
                QString actionName = QString("SelectedDataset") + QString::number(index + 1);
                datasetSelectedAction.setConnectionPermissionsFlag(ConnectionPermissionFlag::All);
                datasetSelectedAction.publish(baseName + actionName);
                datasetSelectedAction.setSerializationName(actionName);
            }


        }
        QObject::connect(&currentDataset, &Dataset<hdps::DatasetImpl>::changed, [this](const hdps::Dataset<hdps::DatasetImpl>& dataset) -> void {this->datasetNameStringAction.setText(dataset->getGuiName()); });


        // setCheckable(true);
    }

    connect(&datasetPickerAction, &DatasetPickerAction::datasetPicked, [this](Dataset<hdps::DatasetImpl> pickedDataset) -> void {
        currentDataset = pickedDataset;
        });



    connect(&currentDataset, &Dataset<DatasetImpl>::changed, [this](Dataset<hdps::DatasetImpl> dataset) -> void {


        if (datasetPickerAction.getCurrentDataset() != dataset)
            datasetPickerAction.setCurrentDataset(dataset);

        });


    currentDataset = datasetPickerAction.getCurrentDataset();

    connect(&datasetNameStringAction, &StringAction::stringChanged, [this](const QString&)->void {this->emitDataChanged(); });
    connect(&datasetSelectedAction, &ToggleAction::changed, [this]()->void {this->emitDataChanged(); });



    setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    if (datasetSelectedAction.isChecked())
        setData(Qt::Checked, Qt::CheckStateRole);
    else
        setData(Qt::Unchecked, Qt::CheckStateRole);

}

QStandardItem* SelectedDatasetsAction::Data::clone() const
{
    auto* parent = qobject_cast<SelectedDatasetsAction*>(datasetPickerAction.parent());
    if (parent)
    {
        return new Data(parent, parent->size());
    }
    return nullptr;
}



QVariant SelectedDatasetsAction::Data::data(int role) const
{
    if (role == Qt::DisplayRole)
        return datasetNameStringAction.getString();
    else if (role == Qt::CheckStateRole)
        if (datasetSelectedAction.isChecked())
            return Qt::Checked;
        else
            return  Qt::Unchecked;

    return QStandardItem::data(role);
}

void SelectedDatasetsAction::Data::setData(const QVariant& value, int role)
{
    if (role == Qt::DisplayRole)
    {
        datasetNameStringAction.setString(value.toString());
        emitDataChanged();
    }
    else if (role == Qt::CheckStateRole)
    {
        if (value == Qt::Checked)
            datasetSelectedAction.setChecked(true);
        else
            datasetSelectedAction.setChecked(false);
        emitDataChanged();
    }
    else
    {
        int x = 0;
        x++;

        QStandardItem::setData(value, role);
    }

}

QVariantMap SelectedDatasetsAction::toVariantMap() const
{
    auto variantMap = WidgetAction::toVariantMap();

    qsizetype nrOfDatasets = _model.rowCount(); // _data.size();

    variantMap["LoadedDatasetsActionVersion"] = 1;
    variantMap["NrOfDatasets"] = nrOfDatasets;

    for (qsizetype i = 0; i < nrOfDatasets; ++i)
    {

        Data* data = dynamic_cast<Data*>(_model.item(i, 0));

        QVariantMap subMap;

        data->datasetPickerAction.insertIntoVariantMap(subMap);

        data->datasetNameStringAction.insertIntoVariantMap(subMap);
        data->datasetSelectedAction.insertIntoVariantMap(subMap);

        QString key = "Data" + QString::number(i);
        variantMap[key] = subMap;
    }
    return variantMap;
    return variantMap;
}

void SelectedDatasetsAction::fromVariantMap(const QVariantMap& variantMap)
{
    WidgetAction::fromVariantMap(variantMap);

    auto version = variantMap.value("LoadedDatasetsActionVersion", QVariant::fromValue(uint(0))).toUInt();
    if (version > 0)
    {
        auto found = variantMap.find("NrOfDatasets");
        if (found == variantMap.cend())
            return;
        const qsizetype NrOfDatasets = found->value<qsizetype>();

        const qsizetype datasetsToBeAdded = NrOfDatasets - _model.rowCount();//_data.size();
        for (qsizetype i = 0; i < datasetsToBeAdded; ++i)
            addDataset();
        for (auto i = 0; i < NrOfDatasets; ++i)
        {
            QString key = "Data" + QString::number(i);
            auto found = variantMap.find(key);
            if (found != variantMap.cend())
            {
                QVariantMap subMap = found->value<QVariantMap>();
                data(i)->datasetPickerAction.fromParentVariantMap(subMap);

                data(i)->datasetNameStringAction.fromParentVariantMap(subMap);

            }
        }
    }
}

SelectedDatasetsAction::SelectedDatasetsAction(ViewPlugin* plugin, qsizetype initialNrOfDatasets, QString instanceName, QString tooltipName)
    : PluginAction(plugin, plugin, instanceName)
    //    , _data(2)
    , _addDatasetTriggerAction(nullptr, "addDataset")
{
    _originalPluginName = plugin->getGuiName();
    setSerializationName(instanceName);
    for (auto i = 0; i < initialNrOfDatasets; ++i)
    {
        addDataset();
    }
    // for (auto i = 0; i < _data.size();++i)
    //     _data[i].reset(new Data(this,i));
    setIcon(hdps::Application::getIconFont("FontAwesome").getIcon("database"));
    setToolTip(tooltipName);


    connect(&_addDatasetTriggerAction, &TriggerAction::triggered, this, &SelectedDatasetsAction::addDataset);
    _addDatasetTriggerAction.setIcon(hdps::Application::getIconFont("FontAwesome").getIcon("plus"));
    QString name = _addDatasetTriggerAction.text();
    assert(!name.isEmpty());
    QString apiName = localNamespace::toCamelCase(name, ' ');
    _addDatasetTriggerAction.setConnectionPermissionsFlag(ConnectionPermissionFlag::All);
    _addDatasetTriggerAction.publish(_originalPluginName + "::" + instanceName + "::" + apiName);
	
    _addDatasetTriggerAction.setSerializationName(apiName);

}

hdps::gui::ToggleAction& SelectedDatasetsAction::getDatasetSelectedAction(const std::size_t index)
{
    return data(index)->datasetSelectedAction;
}




hdps::Dataset<DatasetImpl>& SelectedDatasetsAction::getDataset(std::size_t index) const
{
    return data(index)->currentDataset;
    //return _data.at(index)->currentDataset;
}



QWidget* SelectedDatasetsAction::getDatasetNameWidget(std::size_t index, QWidget* parent, const std::int32_t& flags)
{
    return data(index)->datasetNameStringAction.createWidget(parent, flags);
    //return _data.at(index)->datasetNameStringAction.createWidget(parent, flags);
}



qsizetype SelectedDatasetsAction::size() const
{
    return _model.rowCount();
}

SelectedDatasetsAction::Data* SelectedDatasetsAction::data(qsizetype index) const
{
    return dynamic_cast<Data*>(_model.item(index, 0));
}

QStandardItemModel& SelectedDatasetsAction::model()
{
    return _model;
}

void SelectedDatasetsAction::addDataset()
{
    int currentSize = _model.rowCount();// _data.size();

    _model.appendRow(new Data(this, currentSize));
    //    _data.resize(currentSize + 1);
    //    _data[currentSize].reset(new Data(this, currentSize));
    emit datasetAdded(currentSize);
}


SelectedDatasetsAction::Widget::Widget(QWidget* parent, SelectedDatasetsAction* currentDatasetAction, const std::int32_t& widgetFlags) :
    WidgetActionWidget(parent, currentDatasetAction)
{

    if (widgetFlags & PopupLayout)
    {
        setFixedWidth(600);
        auto layout = new QGridLayout();




        QWidget* addButton = currentDatasetAction->_addDatasetTriggerAction.createWidget(this, TriggerAction::Icon);
        addButton->setFixedWidth(addButton->height());
        layout->addWidget(addButton, 0, 1);


        const int offset = 1;
        connect(currentDatasetAction, &SelectedDatasetsAction::datasetAdded, this, [this, layout, offset, currentDatasetAction]()->void
            {
                int i = currentDatasetAction->size() - 1;
                int column = 0;

                QWidget* w = currentDatasetAction->data(i)->datasetSelectedAction.createWidget(this, ToggleAction::CheckBox);
                w->setFixedWidth(16);
                layout->addWidget(w, i + offset, column++);
                layout->addWidget(currentDatasetAction->data(i)->datasetNameStringAction.createWidget(this), i + offset, column++);
                layout->addWidget(currentDatasetAction->data(i)->datasetPickerAction.createWidget(this), i + offset, column++);



            });


        for (qsizetype i = 0; i < currentDatasetAction->size(); ++i)
        {
            int column = 0;
            QWidget* w = currentDatasetAction->data(i)->datasetSelectedAction.createWidget(this, ToggleAction::CheckBox);
            w->setFixedWidth(16);
            layout->addWidget(w, i + offset, column++);
            layout->addWidget(currentDatasetAction->data(i)->datasetNameStringAction.createWidget(this), i + offset, column++);
            layout->addWidget(currentDatasetAction->data(i)->datasetPickerAction.createWidget(this), i + offset, column++);

        }




        setPopupLayout(layout);

    }
    else {

        setFixedWidth(800);
        auto layout = new QHBoxLayout();


        QComboBox* datasetSelectionComboBox = new QComboBox(this);
        datasetSelectionComboBox->setModel(&currentDatasetAction->model());
        layout->addWidget(datasetSelectionComboBox);

        QMap<int, QList<QWidget*>> map;

        for (qsizetype i = 0; i < currentDatasetAction->size(); ++i)
        {
            if (currentDatasetAction->data(i)->datasetSelectedAction.isChecked())
            {
                layout->addWidget(currentDatasetAction->data(i)->datasetNameStringAction.createWidget(this), 12);
                layout->addWidget(currentDatasetAction->data(i)->datasetPickerAction.createWidget(this), 12);

            }

        }
        layout->setContentsMargins(0, 0, 0, 0);
        setLayout(layout);
    }
}

