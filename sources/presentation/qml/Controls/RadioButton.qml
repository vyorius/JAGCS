import QtQuick 2.6
import QtQuick.Templates 2.0 as T

import "../Shaders" as Shaders

T.RadioButton {
    id: control

    font.pixelSize: sizings.fontPixelSize
    height: sizings.controlBaseSize
    leftPadding: 0
    spacing: sizings.spacing
    implicitWidth: text.length > 0 ? contentItem.implicitWidth + spacing : indicator.implicitWidth

    indicator: Rectangle {
        implicitWidth: sizings.controlBaseSize * 0.8
        implicitHeight: sizings.controlBaseSize * 0.8
        x: control.leftPadding
        y: parent.height * 0.5 - height * 0.5
        radius: width * 0.5
        color: palette.sunkenColor
        border.color: control.activeFocus ? palette.highlightColor : "transparent"

        Rectangle {
            anchors.fill: parent
            anchors.margins: parent.width * 0.25
            radius: width * 0.5
            color: control.down ? palette.highlightColor : palette.selectionColor
            visible: control.checked || control.down
        }

        Shaders.Hatch {
            anchors.fill: parent
            color: palette.sunkenColor
            visible: !control.enabled
        }
    }

    contentItem: Label {
        text: control.text
        font: control.font
        leftPadding: indicator.width + spacing
        verticalAlignment: Text.AlignVCenter
    }
}
