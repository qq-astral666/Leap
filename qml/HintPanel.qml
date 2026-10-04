import QtQuick
import QtQuick.Window
import Leap

// The hint panel shown after the leader key: the keys of the current group.
// A click-through, non-activating panel (see native::configureOverlay); the
// window is bigger than the card so the card can grow without resizing it.
Window {
    id: win

    required property Controller controller

    width: 760
    height: 600
    visible: false
    color: "transparent"
    title: "Leap"
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
           | Qt.WindowDoesNotAcceptFocus | Qt.WindowTransparentForInput

    readonly property bool shown: controller.overlayShown
    readonly property var items: controller.overlayItems
    readonly property int columns: items.length > 7 ? 2 : 1
    readonly property real cellWidth: columns === 2 ? 300 : 340

    Item {
        id: stage
        anchors.fill: parent
        opacity: win.shown ? 1 : 0
        scale: win.shown ? 1 : 0.97
        Behavior on opacity { NumberAnimation { duration: win.shown ? 90 : 120; easing.type: Easing.OutCubic } }
        Behavior on scale { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }

        Item {
            id: card
            anchors.centerIn: parent
            width: content.width + 28
            height: content.height + 24
            transform: Translate { id: shakeShift }

            // Soft shadow without effects modules: a few widening outlines.
            Repeater {
                model: 4
                Rectangle {
                    required property int index
                    anchors.fill: parent
                    anchors.margins: -(index + 1) * 3
                    anchors.topMargin: -(index + 1) * 2
                    anchors.bottomMargin: -(index + 1) * 5
                    radius: 18 + (index + 1) * 3
                    color: "transparent"
                    border.width: 3
                    border.color: Qt.rgba(0, 0, 0, Theme.dark ? 0.11 - index * 0.025 : 0.05 - index * 0.011)
                }
            }

            Rectangle {
                anchors.fill: parent
                radius: 18
                color: Theme.panel
                border.color: Theme.borderStrong
                border.width: 1
            }

            Column {
                id: content
                x: 14
                y: 12
                spacing: 10

                // ---- breadcrumb
                Row {
                    spacing: 6
                    height: 22

                    Icon {
                        anchors.verticalCenter: parent.verticalCenter
                        name: "keyboard"
                        size: 15
                        color: Theme.accentText
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: "Leap"
                        color: win.controller.overlayPath.length ? Theme.textTertiary : Theme.text
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                    }
                    Repeater {
                        model: win.controller.overlayPath
                        Row {
                            required property string modelData
                            required property int index
                            spacing: 6
                            anchors.verticalCenter: parent.verticalCenter
                            Icon {
                                anchors.verticalCenter: parent.verticalCenter
                                name: "chevronRight"
                                size: 12
                                color: Theme.textTertiary
                            }
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData
                                color: index === win.controller.overlayPath.length - 1 ? Theme.text : Theme.textTertiary
                                font.pixelSize: 13
                                font.weight: Font.DemiBold
                            }
                        }
                    }
                    Rectangle {
                        visible: win.controller.overlaySticky
                        anchors.verticalCenter: parent.verticalCenter
                        width: stickyRow.width + 12
                        height: 18
                        radius: 9
                        color: Theme.selection
                        Row {
                            id: stickyRow
                            anchors.centerIn: parent
                            spacing: 4
                            Icon { name: "repeat"; size: 11; color: Theme.accentText; anchors.verticalCenter: parent.verticalCenter }
                            Text { text: "повтор"; color: Theme.accentText; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter }
                        }
                    }
                }

                // ---- keys
                Grid {
                    id: grid
                    // Column by column: down the left side, then down the right,
                    // in the order of the list in settings.
                    flow: Grid.TopToBottom
                    rows: Math.max(1, Math.ceil(win.items.length / win.columns))
                    columnSpacing: 6
                    rowSpacing: 2
                    visible: win.items.length > 0

                    Repeater {
                        model: win.items

                        Item {
                            id: cell
                            required property var modelData
                            width: win.cellWidth
                            height: 40

                            KeyCap {
                                id: cap
                                anchors.verticalCenter: parent.verticalCenter
                                x: 2
                                label: cell.modelData.keyLabel
                                size: 28
                            }
                            ItemIcon {
                                id: icon
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: cap.right
                                anchors.leftMargin: 12
                                item: cell.modelData
                                size: 24
                            }
                            Text {
                                id: title
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: icon.right
                                anchors.leftMargin: 10
                                width: Math.min(implicitWidth, parent.width - x - (cell.modelData.group ? 26 : 4))
                                text: cell.modelData.title
                                color: Theme.text
                                font.pixelSize: 14
                                font.weight: cell.modelData.group ? Font.DemiBold : Font.Normal
                                elide: Text.ElideRight
                            }
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: title.right
                                anchors.leftMargin: 8
                                anchors.right: chevron.visible ? chevron.left : parent.right
                                anchors.rightMargin: 6
                                visible: !cell.modelData.group && text !== "" && width > 30
                                text: cell.modelData.subtitle
                                color: Theme.textTertiary
                                font.pixelSize: 12
                                elide: Text.ElideRight
                            }
                            Icon {
                                id: chevron
                                visible: cell.modelData.group
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.right: parent.right
                                anchors.rightMargin: 4
                                name: "chevronRight"
                                size: 14
                                color: Theme.textTertiary
                            }
                        }
                    }
                }

                Text {
                    visible: win.items.length === 0
                    width: win.cellWidth
                    height: 40
                    verticalAlignment: Text.AlignVCenter
                    text: "Здесь пока нет клавиш — добавьте их в настройках"
                    color: Theme.textSecondary
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                }

                // ---- hints
                Row {
                    spacing: 14
                    height: 18
                    Repeater {
                        model: win.controller.overlayPath.length
                               ? [["esc", "закрыть"], ["⌫", "назад"]]
                               : [["esc", "закрыть"]]
                        Row {
                            required property var modelData
                            spacing: 5
                            anchors.verticalCenter: parent.verticalCenter
                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                width: hintKey.implicitWidth + 8
                                height: 16
                                radius: 4
                                color: Theme.chip
                                Text {
                                    id: hintKey
                                    anchors.centerIn: parent
                                    text: modelData[0]
                                    color: Theme.textSecondary
                                    font.family: Theme.monoFont
                                    font.pixelSize: 10
                                }
                            }
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData[1]
                                color: Theme.textTertiary
                                font.pixelSize: 11
                            }
                        }
                    }
                }
            }
        }
    }

    // Group changes: a quick fade so the swap doesn't look like a glitch.
    Connections {
        target: win.controller
        function onOverlayShake() { shake.restart() }
    }
    onItemsChanged: if (shown) swap.restart()

    NumberAnimation {
        id: swap
        target: grid
        property: "opacity"
        from: 0.35
        to: 1
        duration: 120
        easing.type: Easing.OutCubic
    }

    SequentialAnimation {
        id: shake
        NumberAnimation { target: shakeShift; property: "x"; to: -10; duration: 40 }
        NumberAnimation { target: shakeShift; property: "x"; to: 8; duration: 60 }
        NumberAnimation { target: shakeShift; property: "x"; to: -5; duration: 55 }
        NumberAnimation { target: shakeShift; property: "x"; to: 0; duration: 50 }
    }
}
