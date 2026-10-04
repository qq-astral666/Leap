import QtQuick
import Leap

// Compact button. kind: "normal", "primary", "danger", "ghost".
Rectangle {
    id: root

    property string text: ""
    property string icon: ""
    property string kind: "normal"
    signal clicked()

    implicitWidth: row.implicitWidth + (text ? 22 : 14)
    implicitHeight: 30
    radius: 8
    opacity: enabled ? 1 : 0.4

    readonly property bool primary: kind === "primary"
    readonly property color fg: primary ? "white" : kind === "danger" ? Theme.danger : Theme.text

    color: {
        if (primary)
            return mouse.pressed ? Qt.darker(Theme.accent, 1.15) : mouse.containsMouse ? Qt.lighter(Theme.accent, 1.08) : Theme.accent
        if (kind === "ghost")
            return mouse.containsMouse ? Theme.hover : "transparent"
        return mouse.pressed ? Theme.selection : mouse.containsMouse ? Theme.hover : Theme.field
    }
    border.color: primary || kind === "ghost" ? "transparent" : Theme.border
    border.width: 1

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 6
        Icon {
            visible: root.icon !== ""
            anchors.verticalCenter: parent.verticalCenter
            name: root.icon
            size: 14
            color: root.fg
            strokeWidth: 2
        }
        Text {
            visible: root.text !== ""
            anchors.verticalCenter: parent.verticalCenter
            text: root.text
            color: root.fg
            font.pixelSize: 13
            font.weight: root.primary ? Font.DemiBold : Font.Medium
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
