#include "default_params_migration.h"

// Qt
#include <QCoreApplication>
#include <QDebug>

// Internal
#include "settings_provider.h"

using namespace data_source;

bool DefaultParamsMigration::up()
{
    if (!m_query.prepare(QString("INSERT INTO vehicles (mavId, name, type) "
                                 "VALUES (1, \'%1\', 1)").arg(
                             qApp->translate("DefaultParamsMigration", "MAV 1")))
        || !m_query.exec()) return false;

    settings::Provider::setValue(settings::vehicle::vehicle + QString::number(1) +
                                 "/" + settings::vehicle::instruments::status +
                                 "/" + settings::visibility, true);
    settings::Provider::setValue(settings::vehicle::vehicle + QString::number(1) +
                                 "/" + settings::vehicle::instruments::fd +
                                 "/" + settings::visibility, true);
    settings::Provider::setValue(settings::vehicle::vehicle + QString::number(1) +
                                 "/" + settings::vehicle::instruments::hsi +
                                 "/" + settings::visibility, true);

    return DbMigration::up();
}

bool DefaultParamsMigration::down()
{
    // No need to drop default values

    return true;
}

QDateTime DefaultParamsMigration::version() const
{
    return QDateTime::fromString("2018.08.07-09:38:00", format);
}