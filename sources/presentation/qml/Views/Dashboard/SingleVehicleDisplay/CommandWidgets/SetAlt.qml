import QtQuick 2.6
import QtQuick.Layouts 1.3
import JAGCS 1.0

import "qrc:/Controls" as Controls
import "../../DashboardControls" as DashboardControls

RowLayout {
    id: root

    spacing: sizings.spacing

    Connections {
        target: vehicleDisplay
        onUpdateCommandStatus: if (command == altitudeBox.command) altitudeBox.status = status;
    }

    Connections {
        target: vehicle.barometric
        enabled: vehicle.barometric.present
        onDisplayedAltitudeChanged: {
            if (altitudeBox.isValid) return;
            altitudeBox.realValue = units.convertDistanceTo(altitudeUnits,
                                                            vehicle.barometric.displayedAltitude);
        }
    }

    onVisibleChanged: altitudeBox.realValue = units.convertDistanceTo(
                          altitudeUnits, vehicle.barometric.displayedAltitude)

    DashboardControls.Label {
        text: qsTr("Alt") + ", " + altitudeSuffix
        Layout.fillWidth: true
    }

    DashboardControls.CommandRealSpinBox {
        id: altitudeBox
        command: Command.SetAltitude
        args: [ vehicle.barometric.fromDisplayedAltitude(units.convertDistanceFrom(
                                                             altitudeUnits, altitudeBox.realValue)) ]
        tipText: qsTr("Set altitude")
        realFrom: settings.value("Parameters/minAltitude")
        realTo: settings.value("Parameters/maxAltitude")
        precision: settings.value("Parameters/precisionAltitude")
        Layout.preferredWidth: sizings.controlBaseSize * 4
    }
}