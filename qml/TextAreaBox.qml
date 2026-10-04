import QtQuick
import QtQuick.Controls.Basic
import Leap

// Multi-line field with its own scrolling.
ScrollView {
    id: root

    property alias text: area.text
    property alias placeholderText: area.placeholderText
    property bool mono: false
    signal textEdited()

    implicitHeight: 110
    clip: true

    background: Rectangle {
        radius: 8
        color: Theme.field
        border.width: area.activeFocus ? 2 : 1
        border.color: area.activeFocus ? Theme.accent : Theme.border
    }

    TextArea {
        id: area
        padding: 10
        color: Theme.text
        placeholderTextColor: Theme.textTertiary
        selectionColor: Theme.accent
        selectedTextColor: "white"
        font.pixelSize: 13
        font.family: root.mono ? Theme.monoFont : Qt.application.font.family
        wrapMode: root.mono ? TextEdit.NoWrap : TextEdit.Wrap
        selectByMouse: true
        background: null
        onTextChanged: if (activeFocus) root.textEdited()
    }
}
