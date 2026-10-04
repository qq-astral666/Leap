import QtQuick
import QtQuick.Controls.Basic
import Leap

TextField {
    id: root

    property bool mono: false

    implicitHeight: 32
    leftPadding: 10
    rightPadding: 10
    color: Theme.text
    placeholderTextColor: Theme.textTertiary
    selectionColor: Theme.accent
    selectedTextColor: "white"
    font.pixelSize: 13
    font.family: mono ? Theme.monoFont : Qt.application.font.family
    selectByMouse: true

    background: Rectangle {
        radius: 8
        color: Theme.field
        border.width: root.activeFocus ? 2 : 1
        border.color: root.activeFocus ? Theme.accent : Theme.border
    }
}
