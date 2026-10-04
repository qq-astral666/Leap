import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Leap

// Leader key, timing, window gap, login item, config file.
ScrollView {
    id: root

    required property Controller controller
    contentWidth: availableWidth
    clip: true

    ColumnLayout {
        width: Math.min(parent.width, 640)
        spacing: 26

        // ---- leader
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 10
            SectionTitle { text: "Ведущая клавиша" }
            Text {
                Layout.fillWidth: true
                text: "С неё начинается любая последовательность: ведущая, потом одна или несколько клавиш."
                color: Theme.textSecondary
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }
            Repeater {
                model: root.controller.leaderOptions
                Rectangle {
                    id: opt
                    required property var modelData
                    readonly property bool selected: root.controller.leader === modelData.id
                    Layout.fillWidth: true
                    implicitHeight: optRow.implicitHeight + 22
                    radius: 10
                    color: selected ? Theme.selection : optMouse.containsMouse ? Theme.hover : Theme.card
                    border.color: selected ? Theme.accent : Theme.border
                    border.width: selected ? 2 : 1

                    RowLayout {
                        id: optRow
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 14
                        anchors.rightMargin: 14
                        spacing: 12
                        Rectangle {
                            Layout.alignment: Qt.AlignVCenter
                            width: 16
                            height: 16
                            radius: 8
                            color: "transparent"
                            border.width: opt.selected ? 5 : 1.5
                            border.color: opt.selected ? Theme.accent : Theme.borderStrong
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: opt.modelData.title
                                color: Theme.text
                                font.pixelSize: 13
                                font.weight: Font.DemiBold
                            }
                            Text {
                                visible: opt.modelData.hint !== ""
                                Layout.fillWidth: true
                                text: opt.modelData.hint
                                color: Theme.textSecondary
                                font.pixelSize: 12
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                    MouseArea {
                        id: optMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.controller.leader = opt.modelData.id
                    }
                }
            }
        }

        // ---- timing and windows
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 14
            SectionTitle { text: "Подсказка и окна" }

            SliderRow {
                title: "Показывать подсказку через"
                description: "Если успеть нажать клавиши быстрее, подсказка не появится вовсе."
                from: 0
                to: 800
                step: 50
                value: root.controller.overlayDelay
                suffix: "мс"
                onMoved: v => root.controller.overlayDelay = v
            }
            SliderRow {
                title: "Отступ между окнами"
                description: "Для команд «половина», «треть» и других."
                from: 0
                to: 32
                step: 2
                value: root.controller.windowGap
                suffix: "px"
                onMoved: v => root.controller.windowGap = v
            }
        }

        // ---- system
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 14
            SectionTitle { text: "Система" }
            Toggle {
                Layout.fillWidth: true
                title: "Запускать при входе"
                checked: root.controller.launchAtLogin
                onToggled: on => root.controller.launchAtLogin = on
            }
            Toggle {
                Layout.fillWidth: true
                title: "Пауза"
                description: "Leap перестаёт слушать ведущую клавишу (и Caps Lock остаётся переназначенным, если выбран он)."
                checked: root.controller.paused
                onToggled: on => root.controller.paused = on
            }
        }

        // ---- file
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 10
            SectionTitle { text: "Файл настроек" }
            Text {
                Layout.fillWidth: true
                text: "Все клавиши хранятся в JSON. Его можно править руками или держать в git — Leap подхватит изменения сам."
                color: Theme.textSecondary
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 34
                radius: 8
                color: Theme.field
                border.color: Theme.border
                Text {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    verticalAlignment: Text.AlignVCenter
                    text: root.controller.configPath
                    color: Theme.textSecondary
                    font.family: Theme.monoFont
                    font.pixelSize: 12
                    elide: Text.ElideMiddle
                }
            }
            RowLayout {
                spacing: 8
                PushButton { text: "Показать в Finder"; icon: "external"; onClicked: root.controller.revealConfig() }
                PushButton { text: "Сбросить клавиши"; icon: "refresh"; kind: "danger"; onClicked: resetConfirm.open() }
            }
        }

        Text {
            Layout.fillWidth: true
            text: "Leap " + root.controller.version
            color: Theme.textTertiary
            font.pixelSize: 11
        }
        Item { implicitHeight: 4 }
    }

    Popup {
        id: resetConfirm
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        padding: 20
        width: 360
        background: Rectangle { radius: 14; color: Theme.card; border.color: Theme.borderStrong }
        Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, 0.35) }
        contentItem: ColumnLayout {
            spacing: 14
            Text {
                Layout.fillWidth: true
                text: "Сбросить клавиши?"
                color: Theme.text
                font.pixelSize: 15
                font.weight: Font.DemiBold
            }
            Text {
                Layout.fillWidth: true
                text: "Все ваши клавиши заменятся стандартным набором. Ведущая клавиша останется прежней."
                color: Theme.textSecondary
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 8
                PushButton { text: "Отмена"; onClicked: resetConfirm.close() }
                PushButton {
                    text: "Сбросить"
                    kind: "primary"
                    onClicked: {
                        resetConfirm.close()
                        root.controller.resetToDefaults()
                    }
                }
            }
        }
    }

    component SectionTitle: Text {
        color: Theme.text
        font.pixelSize: 15
        font.weight: Font.DemiBold
    }

    component SliderRow: ColumnLayout {
        id: sr
        property string title
        property string description
        property real from
        property real to
        property real step
        property real value
        property string suffix
        signal moved(int value)
        Layout.fillWidth: true
        spacing: 4
        RowLayout {
            Layout.fillWidth: true
            Text {
                Layout.fillWidth: true
                text: sr.title
                color: Theme.text
                font.pixelSize: 13
                font.weight: Font.Medium
            }
            Text {
                text: Math.round(slider.value) + " " + sr.suffix
                color: Theme.textSecondary
                font.pixelSize: 12
                font.family: Theme.monoFont
            }
        }
        Text {
            Layout.fillWidth: true
            text: sr.description
            color: Theme.textSecondary
            font.pixelSize: 12
            wrapMode: Text.WordWrap
        }
        Slider {
            id: slider
            Layout.fillWidth: true
            from: sr.from
            to: sr.to
            stepSize: sr.step
            snapMode: Slider.SnapAlways
            value: sr.value
            onMoved: sr.moved(Math.round(value))
            background: Rectangle {
                x: slider.leftPadding
                y: slider.topPadding + slider.availableHeight / 2 - height / 2
                width: slider.availableWidth
                height: 4
                radius: 2
                color: Theme.borderStrong
                Rectangle {
                    width: slider.visualPosition * parent.width
                    height: parent.height
                    radius: 2
                    color: Theme.accent
                }
            }
            handle: Rectangle {
                x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
                y: slider.topPadding + slider.availableHeight / 2 - height / 2
                width: 18
                height: 18
                radius: 9
                color: "white"
                border.color: Theme.borderStrong
            }
        }
    }
}
