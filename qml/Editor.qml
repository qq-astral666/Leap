import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import Leap

// Edits one node of the key tree. `path` changes with the selection: text
// fields are refilled then and belong to the user while typing.
Item {
    id: root

    required property Controller controller
    property var path: []
    signal removed() // delete pressed: the owner removes the node
    signal moved(var newPath)

    // Re-read whenever the tree changes (icons, conflicts, titles).
    readonly property var node: { controller.rows; return path.length ? controller.node(path) : ({}) }
    readonly property bool isGroup: node.group ?? false
    readonly property string type: node.type ?? ""
    readonly property bool conflict: {
        const rows = controller.rows
        const key = JSON.stringify(path)
        for (let i = 0; i < rows.length; ++i)
            if (JSON.stringify(rows[i].path) === key)
                return rows[i].conflict
        return false
    }

    // Not "update": that's QQuickItem::update() (repaint), which would
    // silently win and drop every edit.
    function apply(fields) {
        if (path.length)
            controller.updateNode(path, fields)
    }

    // Text fields are filled once per node, then belong to the user.
    onPathChanged: {
        commitTitle.stop()
        commitValue.stop()
        if (!path.length)
            return
        // Read directly: `node` may not have re-evaluated for the new path yet.
        const n = controller.node(path)
        titleField.text = n.title ?? ""
        valueField.text = n.value ?? ""
        valueArea.text = n.value ?? ""
    }

    Timer {
        id: commitTitle
        interval: 350
        onTriggered: root.apply({ title: titleField.text })
    }
    Timer {
        id: commitValue
        interval: 400
        property string text: ""
        onTriggered: root.apply({ value: text })
    }
    function commitValueLater(text) {
        commitValue.text = text
        commitValue.restart()
    }
    // Writes pending edits now: before the node moves or the editor goes.
    function flush() {
        if (commitTitle.running) {
            commitTitle.stop()
            root.apply({ title: titleField.text })
        }
        if (commitValue.running) {
            commitValue.stop()
            root.apply({ value: commitValue.text })
        }
    }
    Component.onDestruction: flush()

    FileDialog {
        id: appPicker
        title: "Приложение"
        currentFolder: "file:///Applications"
        nameFilters: ["Приложения (*.app)"]
        onAccepted: {
            const p = root.controller.localPath(selectedFile)
            const fields = { value: p }
            const t = titleField.text
            if (t === "" || t === "Новое действие" || t === root.controller.appName(root.node.value ?? ""))
                fields.title = root.controller.appName(p)
            root.apply(fields)
            if (fields.title)
                titleField.text = fields.title
        }
    }
    FileDialog {
        id: filePicker
        title: "Файл"
        onAccepted: {
            const p = root.controller.localPath(selectedFile)
            valueField.text = p
            root.apply({ value: p })
        }
    }
    FolderDialog {
        id: folderPicker
        title: "Папка"
        onAccepted: {
            const p = root.controller.localPath(selectedFolder)
            valueField.text = p
            root.apply({ value: p })
        }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: 18

            // ---- header
            RowLayout {
                Layout.fillWidth: true
                spacing: 14
                ItemIcon {
                    item: root.node
                    size: 44
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        Layout.fillWidth: true
                        text: root.node.title || "Без названия"
                        color: Theme.text
                        font.pixelSize: 18
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }
                    Text {
                        Layout.fillWidth: true
                        text: root.isGroup
                              ? (root.node.subtitle + " " + Theme.plural(Number(root.node.subtitle), "клавиша", "клавиши", "клавиш"))
                              : (root.controller.actionTypes.find(t => t.id === root.type)?.title ?? "")
                        color: Theme.textSecondary
                        font.pixelSize: 12
                    }
                }
            }

            // ---- key + title
            RowLayout {
                Layout.fillWidth: true
                spacing: 20

                ColumnLayout {
                    spacing: 6
                    FieldLabel { text: "Клавиша" }
                    KeyField {
                        controller: root.controller
                        keyName: root.node.key ?? ""
                        error: root.conflict
                        onPicked: name => root.apply({ key: name })
                    }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    FieldLabel { text: "Название" }
                    TextBox {
                        id: titleField
                        Layout.fillWidth: true
                        onTextEdited: commitTitle.restart()
                        onEditingFinished: if (commitTitle.running) { commitTitle.stop(); root.apply({ title: text }) }
                    }
                }
            }
            Text {
                visible: root.conflict
                Layout.fillWidth: true
                Layout.topMargin: -8
                text: (root.node.key ?? "") === ""
                      ? "Назначьте клавишу — без неё действие не запустить."
                      : "Эта клавиша уже занята в этой группе: сработает только первая."
                color: Theme.danger
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }

            // ---- group options
            Toggle {
                visible: root.isGroup
                Layout.fillWidth: true
                title: "Не закрывать после действия"
                description: "Группа остаётся открытой, и действие можно повторять: например, громкость — ведущая, ;, затем U U U."
                checked: root.node.sticky ?? false
                onToggled: on => root.apply({ sticky: on })
            }

            // ---- action
            ColumnLayout {
                visible: !root.isGroup
                Layout.fillWidth: true
                spacing: 6
                FieldLabel { text: "Действие" }
                Select {
                    Layout.preferredWidth: 280
                    model: root.controller.actionTypes
                    currentId: root.type
                    onPicked: optionId => {
                        root.apply({ type: optionId })
                        valueField.text = root.controller.node(root.path).value ?? ""
                        valueArea.text = valueField.text
                    }
                }
            }

            // app
            RowLayout {
                visible: !root.isGroup && root.type === "app"
                Layout.fillWidth: true
                spacing: 12
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 52
                    radius: 10
                    color: Theme.field
                    border.color: Theme.border
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        spacing: 10
                        Image {
                            visible: (root.node.value ?? "") !== ""
                            source: visible ? root.node.icon : ""
                            sourceSize: Qt.size(64, 64)
                            Layout.preferredWidth: 32
                            Layout.preferredHeight: 32
                            mipmap: true
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1
                            Text {
                                Layout.fillWidth: true
                                text: (root.node.value ?? "") !== "" ? root.controller.appName(root.node.value) : "Приложение не выбрано"
                                color: (root.node.value ?? "") !== "" ? Theme.text : Theme.textTertiary
                                font.pixelSize: 13
                                font.weight: Font.Medium
                                elide: Text.ElideRight
                            }
                            Text {
                                visible: (root.node.value ?? "") !== ""
                                Layout.fillWidth: true
                                text: root.node.value ?? ""
                                color: Theme.textTertiary
                                font.pixelSize: 11
                                elide: Text.ElideMiddle
                            }
                        }
                    }
                }
                PushButton {
                    text: "Выбрать…"
                    onClicked: appPicker.open()
                }
            }

            // open / url
            ColumnLayout {
                visible: !root.isGroup && (root.type === "open" || root.type === "url")
                Layout.fillWidth: true
                spacing: 8
                TextBox {
                    id: valueField
                    Layout.fillWidth: true
                    mono: true
                    placeholderText: root.type === "url" ? "https://… или tg://…" : "~/Downloads"
                    onTextEdited: root.commitValueLater(text)
                }
                RowLayout {
                    visible: root.type === "open"
                    spacing: 8
                    PushButton { text: "Папка…"; icon: "folder"; onClicked: folderPicker.open() }
                    PushButton { text: "Файл…"; icon: "file"; onClicked: filePicker.open() }
                }
            }

            // shell / text
            ColumnLayout {
                visible: !root.isGroup && (root.type === "shell" || root.type === "text")
                Layout.fillWidth: true
                spacing: 6
                TextAreaBox {
                    id: valueArea
                    Layout.fillWidth: true
                    Layout.preferredHeight: 130
                    mono: root.type === "shell"
                    placeholderText: root.type === "shell" ? "open -a Simulator && cd ~/src && make" : "Текст, который вставится в активное поле"
                    onTextEdited: root.commitValueLater(text)
                }
                Text {
                    Layout.fillWidth: true
                    text: root.type === "shell"
                          ? "Выполняется в zsh из домашней папки, в фоне. Переменные из ~/.zprofile доступны."
                          : "Leap вставит текст через буфер обмена и вернёт в буфер то, что там было."
                    color: Theme.textTertiary
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }
            }

            // window
            GridLayout {
                visible: !root.isGroup && root.type === "window"
                Layout.fillWidth: true
                columns: 3
                columnSpacing: 8
                rowSpacing: 8
                Repeater {
                    model: root.type === "window" ? root.controller.windowCommands : []
                    Rectangle {
                        id: wc
                        required property var modelData
                        readonly property bool selected: root.node.value === modelData.id
                        Layout.fillWidth: true
                        implicitHeight: 64
                        radius: 10
                        color: selected ? Theme.selection : wcMouse.containsMouse ? Theme.hover : Theme.field
                        border.color: selected ? Theme.accent : Theme.border
                        border.width: selected ? 2 : 1
                        Column {
                            anchors.centerIn: parent
                            spacing: 6
                            WindowPreview {
                                anchors.horizontalCenter: parent.horizontalCenter
                                size: 34
                                rect: root.controller.windowPreview(wc.modelData.id)
                                color: wc.selected ? Theme.accent : Theme.textSecondary
                            }
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                width: wc.width - 12
                                horizontalAlignment: Text.AlignHCenter
                                text: wc.modelData.title
                                color: Theme.text
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }
                        }
                        MouseArea {
                            id: wcMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                const t = titleField.text
                                const fields = { value: wc.modelData.id }
                                if (t === "" || t === "Новое действие" || t === root.controller.commandTitle("window", root.node.value))
                                    fields.title = wc.modelData.title
                                root.apply(fields)
                                if (fields.title) titleField.text = fields.title
                            }
                        }
                    }
                }
            }

            // system
            Flow {
                visible: !root.isGroup && root.type === "system"
                Layout.fillWidth: true
                spacing: 8
                Repeater {
                    model: root.type === "system" ? root.controller.systemCommands : []
                    Rectangle {
                        id: sc
                        required property var modelData
                        readonly property bool selected: root.node.value === modelData.id
                        width: scText.implicitWidth + 28
                        height: 32
                        radius: 16
                        color: selected ? Theme.accent : scMouse.containsMouse ? Theme.hover : Theme.field
                        border.color: selected ? "transparent" : Theme.border
                        Text {
                            id: scText
                            anchors.centerIn: parent
                            text: sc.modelData.title
                            color: sc.selected ? "white" : Theme.text
                            font.pixelSize: 13
                        }
                        MouseArea {
                            id: scMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                const t = titleField.text
                                const fields = { value: sc.modelData.id }
                                if (t === "" || t === "Новое действие" || t === root.controller.commandTitle("system", root.node.value))
                                    fields.title = sc.modelData.title
                                root.apply(fields)
                                if (fields.title) titleField.text = fields.title
                            }
                        }
                    }
                }
            }
            Text {
                visible: !root.isGroup && root.node.value === "dark-mode"
                Layout.fillWidth: true
                text: "При первом запуске macOS спросит разрешение управлять «System Events»."
                color: Theme.textTertiary
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }

            // ---- buttons
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 1
                color: Theme.border
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                PushButton {
                    visible: !root.isGroup
                    text: "Проверить"
                    icon: "play"
                    onClicked: {
                        root.flush()
                        root.controller.testNode(root.path)
                    }
                }
                PushButton {
                    icon: "chevronUp"
                    onClicked: {
                        root.flush()
                        root.moved(root.controller.moveNode(root.path, -1))
                    }
                }
                PushButton {
                    icon: "chevronDown"
                    onClicked: {
                        root.flush()
                        root.moved(root.controller.moveNode(root.path, 1))
                    }
                }
                Item { Layout.fillWidth: true }
                PushButton {
                    text: root.isGroup ? "Удалить группу" : "Удалить"
                    icon: "trash"
                    kind: "danger"
                    onClicked: {
                        commitTitle.stop()
                        commitValue.stop()
                        root.removed() // Settings removes it and clears the selection
                    }
                }
            }
            Item { implicitHeight: 8 }
        }
    }

    component FieldLabel: Text {
        color: Theme.textSecondary
        font.pixelSize: 12
        font.weight: Font.Medium
    }
}
