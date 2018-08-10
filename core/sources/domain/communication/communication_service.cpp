#include "communication_service.h"

// Qt
#include <QThread>
#include <QMap>
#include <QDebug>

// Internal
#include "settings_provider.h"

#include "link_description.h"
#include "link_protocol.h"
#include "link_statistics.h"

#include "description_link_factory.h"
#include "notification_bus.h"

#include "service_registry.h"
#include "serial_ports_service.h"

#include "link_manager.h"
#include "communicator_worker.h"

using namespace domain;

class CommunicationService::Impl
{
public:
    LinkManager linkManager;

    QMap<int, dto::LinkStatisticsPtr> linkStatistics;
    QMap<ICommunicationPlugin*, data_source::AbstractCommunicator*> pluginCommunicators;
    QMap<data_source::AbstractCommunicator*, QString> communicatorProtocols;
    QMap<dto::LinkDescriptionPtr, QString> descriptedDevices;

    SerialPortService* serialPortService = serviceRegistry->serialPortService();

    QThread* commThread;
    CommunicatorWorker* commWorker;

    dto::LinkStatisticsPtr getlinkStatistics(int linkId)
    {
        if (!linkStatistics.contains(linkId))
        {
            linkStatistics[linkId] = dto::LinkStatisticsPtr::create();
            linkStatistics[linkId]->setLinkId(linkId);
        }
        return linkStatistics[linkId];
    }
};

CommunicationService::CommunicationService(QObject* parent):
    QObject(parent),
    d(new Impl())
{
    qRegisterMetaType<dto::LinkDescriptionPtr>("dto::LinkDescriptionPtr");
    qRegisterMetaType<dto::LinkProtocolPtr>("dto::LinkProtocolPtr");
    qRegisterMetaType<dto::LinkStatisticsPtr>("dto::LinkStatisticsPtr");
    qRegisterMetaType<data_source::LinkFactoryPtr>("data_source::LinkFactoryPtr");

    connect(&d->linkManager, &LinkManager::descriptionAdded,
            this, &CommunicationService::descriptionAdded);
    connect(&d->linkManager, &LinkManager::descriptionRemoved,
            this, &CommunicationService::descriptionRemoved);
    connect(&d->linkManager, &LinkManager::descriptionChanged,
            this, &CommunicationService::descriptionChanged);

    d->commThread = new QThread(this);
    d->commThread->setObjectName("Communication thread");

    d->commWorker = new CommunicatorWorker();
    d->commWorker->moveToThread(d->commThread);
    d->commThread->start();

    connect(d->commThread, &QThread::finished, d->commWorker, &QObject::deleteLater);
    connect(d->commWorker, &CommunicatorWorker::linkStatusChanged,
            this, &CommunicationService::onLinkStatusChanged);
    connect(d->commWorker, &CommunicatorWorker::linkStatisticsChanged,
            this, &CommunicationService::onLinkStatisticsChanged);
    connect(d->commWorker, &CommunicatorWorker::linkSent,
            this, &CommunicationService::linkSent);
    connect(d->commWorker, &CommunicatorWorker::linkRecv,
            this, &CommunicationService::linkRecv);
    connect(d->commWorker, &CommunicatorWorker::linkErrored,
            this, &CommunicationService::onLinkErrored);

    connect(d->serialPortService, &SerialPortService::devicesChanged,
            this, [this]() {
        QStringList devices = d->serialPortService->devices();

        for (const dto::LinkDescriptionPtr& description: d->descriptedDevices.keys())
        {
            if (!description->isConnected() && description->isAutoConnect() &&
                devices.contains(description->parameter(dto::LinkDescription::Device).toString()))
            {
                this->setLinkConnected(description, true);
            }
        }
    });

    for (const dto::LinkDescriptionPtr& description: d->linkManager.descriptions())
    {
        data_source::LinkFactoryPtr factory(new data_source::DescriptionLinkFactory(description));
        d->commWorker->updateLink(description->id(), factory, description->isAutoConnect());

        QString device = description->parameter(dto::LinkDescription::Device).toString();
        if (description->type() == dto::LinkDescription::Serial && device.length() > 0)
        {
            d->serialPortService->holdDevice(device);
            d->descriptedDevices[description] = device;
        }
    }
}

CommunicationService::~CommunicationService()
{
    d->commThread->quit();
    d->commThread->wait();

    for (const dto::LinkDescriptionPtr& description: d->linkManager.descriptions())
    {
        description->setAutoConnect(description->isConnected());
        this->save(description);

        if (d->descriptedDevices.contains(description))
        {
            d->serialPortService->releaseDevice(d->descriptedDevices.take(description));
        }
    }
}

dto::LinkDescriptionPtr CommunicationService::description(int id) const
{
    return d->linkManager.description(id);
}

dto::LinkDescriptionPtrList CommunicationService::descriptions() const
{
    return d->linkManager.descriptions();
}

dto::LinkStatisticsPtr CommunicationService::statistics(int descriptionId) const
{
     return d->linkStatistics.value(descriptionId);
}

dto::LinkStatisticsPtrList CommunicationService::statistics() const
{
    return d->linkStatistics.values();
}

void CommunicationService::addPlugin(ICommunicationPlugin* plugin)
{
    data_source::AbstractCommunicator* communicator = plugin->createCommunicator();
    if (!communicator) return;

    d->pluginCommunicators[plugin] = communicator;

    for (const QString& protocol: communicator->availableProtocols())
    {
        d->communicatorProtocols.insert(communicator, protocol);
    }
    communicator->moveToThread(d->commThread);
    d->commWorker->addCommunicator(communicator);
}

void CommunicationService::removePlugin(ICommunicationPlugin* plugin)
{
    if (!d->pluginCommunicators.contains(plugin)) return;

    data_source::AbstractCommunicator* communicator = d->pluginCommunicators.take(plugin);
    d->communicatorProtocols.remove(communicator);
    d->commWorker->deleteCommunicator(communicator);
}

QStringList CommunicationService::availableProtocols() const
{
    return d->communicatorProtocols.values();
}

bool CommunicationService::save(const dto::LinkDescriptionPtr& description)
{
    if (!d->linkManager.save(description)) return false;

    data_source::LinkFactoryPtr factory(new data_source::DescriptionLinkFactory(description));
    d->commWorker->updateLink(description->id(), factory, description->isAutoConnect());

    QString device = description->parameter(dto::LinkDescription::Device).toString();
    if (d->descriptedDevices.contains(description) && device != d->descriptedDevices[description])
    {
        d->serialPortService->releaseDevice(d->descriptedDevices.take(description));
    }

    if (description->type() == dto::LinkDescription::Serial &&
        device.length() > 0 && !d->descriptedDevices.contains(description))
    {
        d->serialPortService->holdDevice(device);
        d->descriptedDevices[description] = device;
    }

    return true;
}

bool CommunicationService::remove(const dto::LinkDescriptionPtr& description)
{
    if (!d->linkManager.remove(description)) return false;

    d->commWorker->removeLink(description->id());

    if (d->descriptedDevices.contains(description))
    {
        d->serialPortService->releaseDevice(d->descriptedDevices.take(description));
    }

    if (d->linkStatistics.contains(description->id()))
    {
        d->linkStatistics.remove(description->id());
    }

    return true;
}

void CommunicationService::setLinkConnected(const dto::LinkDescriptionPtr& description,
                                            bool connected)
{
    d->commWorker->setLinkConnected(description->id(), connected);

    description->setAutoConnect(connected);
}

void CommunicationService::onLinkStatusChanged(int linkId, bool connected)
{
    dto::LinkDescriptionPtr description = this->description(linkId);

    if (description->isConnected() != connected)
    {
        notificationBus->notify(tr("Link") + " " + description->name(),
                             connected ? tr("Connection established") : tr("Connection lost"),
                             connected ? dto::Notification::Positive : dto::Notification::Warning);
        description->setConnected(connected);

        emit linkStatusChanged(description);
    }
}

void CommunicationService::onLinkStatisticsChanged(int linkId,
                                                   int timestamp,
                                                   int bytesReceivedSec,
                                                   int bytesSentSec)
{
    dto::LinkStatisticsPtr statistics = d->getlinkStatistics(linkId);

    statistics->setTimestamp(timestamp);
    statistics->setBytesRecv(bytesReceivedSec);
    statistics->setBytesSent(bytesSentSec);

    emit linkStatisticsChanged(statistics);
}

void CommunicationService::onLinkErrored(int linkId, const QString& error)
{
    dto::LinkDescriptionPtr description = this->description(linkId);

    notificationBus->notify(tr("Link") + " " + description->name(),
                            error, dto::Notification::Warning);
}
