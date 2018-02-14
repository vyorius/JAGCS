import QtQuick 2.6
import QtQuick.Controls 2.0 as T

T.Frame {
    id: control

    property color backgroundColor: palette.raisedColor

    padding: sizings.padding

    background:  Rectangle {
        color: backgroundColor
        radius: 2

        Shadow {
            source: parent
        }
    }
}
