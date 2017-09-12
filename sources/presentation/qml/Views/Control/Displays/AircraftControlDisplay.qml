import QtQuick 2.6
import QtQuick.Layouts 1.3
import JAGCS 1.0

import "qrc:/Controls" as Controls

GridLayout {
    id: root

    property int waypoint: 0
    property alias waypoints: waypointBox.model
    property bool armed: false
    property bool guided: false

    property int groundspeed: 0
    property int indicatedAirspeed: 0
    property int command: commandBox.currentItem ? commandBox.currentItem.command : Command.UnknownCommand

    signal commandSetWaypoint(int item)

    signal commandStatusChanged(var command, var status)
    signal executeCommand(int command, var arguments)
    signal rejectCommand(int command)

    onWaypointChanged: waypointBox.currentIndex = waypointBox.model.indexOf(waypoint.toString())

    columns: 3

    Controls.Label {
        text: qsTr("WP:")
        font.pixelSize: palette.fontPixelSize * 0.75
        font.bold: true
        Layout.fillWidth: true
    }

    Controls.ComboBox {
        id: waypointBox
        model: []
        onCurrentTextChanged: commandSetWaypoint(currentText)
        enabled: guided
        Layout.columnSpan: 2
        Layout.fillWidth: true
    }

    Controls.Label {
        text: qsTr("Command:")
        font.pixelSize: palette.fontPixelSize * 0.75
        font.bold: true
    }

    Controls.ComboBox {
        id: commandBox
        model: [
            { icon: "", text: "", command: Command.UnknownCommand },
            { icon: armed ? "qrc:/icons/unlock.svg" : "qrc:/icons/lock.svg",
              text: armed ? qsTr("Disarm") : qsTr("Arm"), command: Command.ArmDisarm },
            { icon: "qrc:/icons/home.svg", text: qsTr("Return"), command: Command.Return },
            { icon: "qrc:/icons/play.svg", text: qsTr("Start mission"), command: Command.Start },
            { icon: "qrc:/icons/takeoff.svg", text: qsTr("Takeoff"), command: Command.Takeoff },
            { icon: "qrc:/icons/landing.svg", text: qsTr("Land"), command: Command.Land },
            { icon: "qrc:/icons/go_around.svg", text: qsTr("Go around"), command: Command.GoAround },
            { icon: "qrc:/icons/speed.svg", text: qsTr("Airspeed"), command: Command.SetSpeed, type: 0 },
            { icon: "qrc:/icons/speed.svg", text: qsTr("Groundspeed"), command: Command.SetSpeed, type: 1 },
        ]
        Layout.fillWidth: true
    }

    Controls.CommandButton {
        command: root.command
        iconSource: "qrc:/icons/right.svg"
        enabled: commandBox.currentIndex > 0
        args: {
            switch (command) {
            case Command.ArmDisarm: return [!armed];
            case Command.SetSpeed: return [commandBox.currentItem.type, speedBox.value, -1, 0];
            default: return [];
            }
        }
    }

    Controls.Label {
        text: qsTr("Speed:")
        font.pixelSize: palette.fontPixelSize * 0.75
        font.bold: true
        visible: command === Command.SetSpeed
    }

    Controls.SpinBox {
        id: speedBox
        visible: command === Command.SetSpeed
        Layout.fillWidth: true
    }

    Controls.Button {
        iconSource: "qrc:/icons/restore.svg"
        onClicked: speedBox.value = commandBox.currentItem.type ? groundspeed : indicatedAirspeed
        visible: command === Command.SetSpeed
    }
}
