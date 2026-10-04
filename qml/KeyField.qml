import QtQuick
import Leap

// Click, then press a key. Works in any layout: the typed character is
// mapped to its physical key (Cyrillic "е" -> T) by Controller.normalizeKey.
FocusScope {
    id: root

    required property Controller controller
    property string keyName: ""
    property bool error: false
    signal picked(string keyName)

    implicitWidth: 120
    implicitHeight: 40
    activeFocusOnTab: true

    readonly property bool listening: activeFocus

    Row {
        spacing: 10
        anchors.verticalCenter: parent.verticalCenter

        KeyCap {
            anchors.verticalCenter: parent.verticalCenter
            label: root.keyName ? root.controller.keyLabel(root.keyName) : "?"
            size: 36
            highlighted: root.listening
            error: root.error && !root.listening
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.listening ? "Нажмите клавишу…" : "Изменить"
            color: root.listening ? Theme.accentText : Theme.textSecondary
            font.pixelSize: 12
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.forceActiveFocus()
    }

    Keys.onPressed: event => {
        event.accepted = true
        if (event.key === Qt.Key_Escape || event.key === Qt.Key_Tab) {
            root.focus = false
            return
        }
        const special = {
            [Qt.Key_Left]: "left", [Qt.Key_Right]: "right", [Qt.Key_Up]: "up", [Qt.Key_Down]: "down",
            [Qt.Key_Space]: "space", [Qt.Key_Return]: "return", [Qt.Key_Enter]: "return",
            [Qt.Key_F1]: "f1", [Qt.Key_F2]: "f2", [Qt.Key_F3]: "f3", [Qt.Key_F4]: "f4",
            [Qt.Key_F5]: "f5", [Qt.Key_F6]: "f6", [Qt.Key_F7]: "f7", [Qt.Key_F8]: "f8",
            [Qt.Key_F9]: "f9", [Qt.Key_F10]: "f10", [Qt.Key_F11]: "f11", [Qt.Key_F12]: "f12"
        }
        const name = special[event.key] ?? root.controller.normalizeKey(event.text)
        if (name !== "") {
            root.picked(name)
            root.focus = false
        }
    }
}
