#include "link_list_provider.h"

// Qt
#include <QSortFilterProxyModel>
#include <QVariant>
#include <QDebug>

// Internal
#include "settings_provider.h"

#include "link_description.h"

#include "service_registry.h"
#include "communication_service.h"

#include "link_list_model.h"

using namespace presentation;

class LinkListProvider::Impl
{
public:
    domain::CommunicationService* const service = serviceRegistry->communicationService();

    LinkListModel linksModel;
    QSortFilterProxyModel filterModel;
};

LinkListProvider::LinkListProvider(QObject* parent):
    QObject(parent),
    d(new Impl())
{
    d->filterModel.setSourceModel(&d->linksModel);
    d->filterModel.setFilterRole(LinkListModel::LinkNameRole);
    d->filterModel.setSortRole(LinkListModel::LinkNameRole);
    d->filterModel.setDynamicSortFilter(true);
    d->filterModel.sort(0, Qt::AscendingOrder);

    d->linksModel.setLinks(d->service->descriptions());

    connect(d->service, &domain::CommunicationService::descriptionAdded,
            &d->linksModel, &LinkListModel::addLink);
    connect(d->service, &domain::CommunicationService::descriptionRemoved,
            &d->linksModel, &LinkListModel::removeLink);
    connect(d->service, &domain::CommunicationService::descriptionChanged,
            &d->linksModel, &LinkListModel::updateLink);
}

LinkListProvider::~LinkListProvider()
{}

QAbstractItemModel* LinkListProvider::links() const
{
    return &d->filterModel;
}

void LinkListProvider::addSerialLink()
{
    dto::LinkDescriptionPtr description = dto::LinkDescriptionPtr::create();

    description->setName(tr("Serial Link"));
    description->setType(dto::LinkDescription::Serial);
    description->setParameter(dto::LinkDescription::BaudRate,
                              settings::Provider::value(settings::communication::baudRate));

    d->service->save(description);
}

void LinkListProvider::addUdpLink()
{
    dto::LinkDescriptionPtr description = dto::LinkDescriptionPtr::create();

    description->setName(tr("UDP Link"));
    description->setType(dto::LinkDescription::Udp);
    description->setParameter(dto::LinkDescription::Port,
                              settings::Provider::value(settings::communication::udpPort));
    description->setParameter(dto::LinkDescription::UdpAutoResponse, true);

    d->service->save(description);
}

void LinkListProvider::addTcpLink()
{
    dto::LinkDescriptionPtr description = dto::LinkDescriptionPtr::create();

    description->setName(tr("TCP Link"));
    description->setType(dto::LinkDescription::Tcp);
    description->setParameter(dto::LinkDescription::Address,
                              settings::Provider::value(settings::communication::tcpAddress));
    description->setParameter(dto::LinkDescription::Port,
                              settings::Provider::value(settings::communication::tcpPort));

    d->service->save(description);
}

void LinkListProvider::addBluetoothLink()
{
    dto::LinkDescriptionPtr description = dto::LinkDescriptionPtr::create();

    description->setName(tr("Bluetooth"));
    description->setType(dto::LinkDescription::Bluetooth);
    description->setParameter(dto::LinkDescription::Address,
                              settings::Provider::value(settings::communication::bluetoothAddress));

    d->service->save(description);
}

void LinkListProvider::filter(const QString& filterString)
{
    d->filterModel.setFilterFixedString(filterString);
}
