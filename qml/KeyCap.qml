import QtQuick
import Leap

// A keyboard key: a raised top over a darker base.
Item {
    id: root

    property string label: ""
    property real size: 30
    property bool highlighted: false // accent, e.g. the key being recorded
    property bool error: false       // e.g. bound twice
    property bool pressed: false

    implicitWidth: Math.max(size, labelText.implicitWidth + size * 0.5)
    implicitHeight: size

    Rectangle {
        anchors.fill: parent
        radius: root.size * 0.24
        color: root.error ? Qt.darker(Theme.danger, 1.6) : root.highlighted ? Qt.darker(Theme.accent, 1.5) : Theme.capSide
    }
    Rectangle {
        id: top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: parent.height - (root.pressed ? 1 : root.size * 0.09)
        radius: root.size * 0.24
        color: root.error ? Theme.danger : root.highlighted ? Theme.accent : Theme.capTop
        border.color: root.error || root.highlighted ? "transparent" : Theme.border
        border.width: 1

        Behavior on height { NumberAnimation { duration: 60 } }

        Text {
            id: labelText
            anchors.centerIn: parent
            text: root.label
            color: root.error || root.highlighted ? "white" : Theme.capText
            font.family: Theme.monoFont
            font.pixelSize: root.label.length > 2 ? root.size * 0.34 : root.size * 0.46
            font.weight: Font.DemiBold
        }
    }
}
