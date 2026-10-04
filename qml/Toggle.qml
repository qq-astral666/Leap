import QtQuick
import Leap

// A label, an optional description and a switch.
Item {
    id: root

    property string title: ""
    property string description: ""
    property bool checked: false
    signal toggled(bool checked)

    implicitHeight: Math.max(knobTrack.height, texts.implicitHeight)

    Column {
        id: texts
        anchors.left: parent.left
        anchors.right: knobTrack.left
        anchors.rightMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2
        Text {
            width: parent.width
            text: root.title
            color: Theme.text
            font.pixelSize: 13
            font.weight: Font.Medium
            wrapMode: Text.WordWrap
        }
        Text {
            visible: root.description !== ""
            width: parent.width
            text: root.description
            color: Theme.textSecondary
            font.pixelSize: 12
            wrapMode: Text.WordWrap
        }
    }

    Rectangle {
        id: knobTrack
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        width: 38
        height: 22
        radius: 11
        color: root.checked ? Theme.accent : Theme.borderStrong
        Behavior on color { ColorAnimation { duration: 120 } }

        Rectangle {
            width: 18
            height: 18
            radius: 9
            y: 2
            x: root.checked ? parent.width - width - 2 : 2
            color: "white"
            Behavior on x { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.toggled(!root.checked)
    }
}
