#include "mission_vehicle.h"

// Qt
#include <QDebug>

// Internal
#include "mission.h"
#include "vehicle.h"
#include "home_mission_item.h"

using namespace domain;

MissionVehicle::MissionVehicle(Mission* mission, Vehicle* vehicle):
    QObject(mission),
    m_mission(mission),
    m_vehicle(vehicle),
    m_currentProgress(0),
    m_totalProgress(0)
{
    Q_ASSERT(mission);
}

MissionVehicle::~MissionVehicle()
{
    if (m_vehicle) m_vehicle->unassignMission();
}

Mission* MissionVehicle::mission() const
{
    return m_mission;
}

Vehicle* MissionVehicle::vehicle() const
{
    return m_vehicle;
}

uint8_t MissionVehicle::vehicleId() const
{
    if (!m_vehicle) return 0;

    return m_vehicle->vehicleId();
}

int MissionVehicle::currentProgress() const
{
    return m_currentProgress;
}

int MissionVehicle::totalProgress() const
{
    return m_totalProgress;
}

void MissionVehicle::setVehicle(Vehicle* vehicle)
{
    if (m_vehicle == vehicle) return;

    Vehicle* oldVehicle = nullptr;
    if (m_vehicle)
    {
        oldVehicle = m_vehicle;
        disconnect(m_vehicle, &Vehicle::homePositionChanged,
                   this, &MissionVehicle::onHomePositionChanged);
    }

    m_vehicle = vehicle;
    if (oldVehicle) oldVehicle->unassignMission();

    if (m_vehicle)
    {
        m_vehicle->assignMission(m_mission);
        connect(m_vehicle, &Vehicle::homePositionChanged,
                this, &MissionVehicle::onHomePositionChanged);
        this->onHomePositionChanged(m_vehicle->homePosition());
    }

    emit vehicleChanged(m_vehicle);
}

void MissionVehicle::onHomePositionChanged(const Position& homePosition)
{
    if (!m_mission->count())
    {
        m_mission->addNewMissionItem();
    }

    HomeMissionitem* home = qobject_cast<HomeMissionitem*>(m_mission->item(0));
    if (home)
    {
        home->setLatitude(homePosition.coordinate().latitude());
        home->setLongitude(homePosition.coordinate().longitude());
    }
}

void MissionVehicle::setTotalProgress(int totalProgress)
{
    if (m_totalProgress == totalProgress) return;

    m_totalProgress = totalProgress;
    emit totalProgressChanged(totalProgress);
}

void MissionVehicle::setCurrentProgress(int currentProgress)
{
    if (m_currentProgress == currentProgress) return;

    m_currentProgress = currentProgress;
    emit currentProgressChanged(currentProgress);
}