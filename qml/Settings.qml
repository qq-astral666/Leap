import QtQml
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Leap

// Settings: the key tree on the left, the selected key's editor on the
// right; general options on their own page.
ApplicationWindow {
    id: win

    required property Controller controller

    width: 1180
    height: 700
    minimumWidth: 1060
    minimumHeight: 540
    visible: false
    title: "Leap"
    color: Theme.windowBg

    property string page: "keys"
    property var selectedPath: []
    readonly property string selectedKey: JSON.stringify(selectedPath)

    function select(path) {
        editor.flush() // pending title/value edits belong to the old node
        selectedPath = path && path.length ? path.slice() : []
        Qt.callLater(revealSelected)
    }
    // Scrolls the list to the selected row (new items land at the bottom).
    function revealSelected() {
        const rows = controller.rows
        for (let i = 0; i < rows.length; ++i)
            if (JSON.stringify(rows[i].path) === selectedKey) {
                tree.positionViewAtIndex(i, ListView.Contain)
                return
            }
    }
    function removeSelected() {
        if (!selectedPath.length)
            return
        editor.flush()
        const p = selectedPath
        selectedPath = []
        controller.removeNode(p)
    }
    // The group new items go into: the selection if it's a group, else its parent.
    function targetGroup() {
        if (!selectedPath.length)
            return []
        const n = controller.node(selectedPath)
        return n.group ? selectedPath : selectedPath.slice(0, -1)
    }

    Shortcut {
        sequences: [StandardKey.Close]
        onActivated: win.close()
    }
    Shortcut {
        sequences: [StandardKey.New]
        onActivated: win.select(win.controller.addNode(win.targetGroup(), false))
    }

    Connections {
        target: win.controller
        function onToast(message) { toast.show(message) }
        function onKeymapChanged() {
            // Selection gone (removed, reset, file edited): pick nothing.
            if (win.selectedPath.length && !win.controller.node(win.selectedPath).path)
                win.select([])
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ================================================= sidebar
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 220
            color: Theme.sidebarBg

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                anchors.topMargin: 22
                spacing: 4

                RowLayout {
                    spacing: 10
                    Layout.bottomMargin: 16
                    Layout.leftMargin: 4
                    Image {
                        source: "qrc:/qt/qml/Leap/resources/AppIcon.png"
                        sourceSize: Qt.size(64, 64)
                        Layout.preferredWidth: 30
                        Layout.preferredHeight: 30
                        mipmap: true
                    }
                    ColumnLayout {
                        spacing: 0
                        Text { text: "Leap"; color: Theme.text; font.pixelSize: 16; font.weight: Font.Bold }
                        Text { text: "Действия по клавишам"; color: Theme.textTertiary; font.pixelSize: 11 }
                    }
                }

                NavItem { title: "Клавиши"; icon: "keyboard"; current: win.page === "keys"; onActivated: win.page = "keys" }
                NavItem { title: "Основные"; icon: "sliders"; current: win.page === "general"; onActivated: win.page = "general" }

                Item { Layout.fillHeight: true }

                // ---- status card
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: statusCol.implicitHeight + 24
                    radius: 12
                    color: Theme.card
                    border.color: Theme.border

                    ColumnLayout {
                        id: statusCol
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 12
                        spacing: 8

                        RowLayout {
                            spacing: 8
                            Rectangle {
                                width: 8
                                height: 8
                                radius: 4
                                color: !win.controller.accessibilityGranted ? Theme.warning
                                       : win.controller.listening ? Theme.success : Theme.textTertiary
                            }
                            Text {
                                text: !win.controller.accessibilityGranted ? "Нет доступа"
                                      : win.controller.paused ? "На паузе"
                                      : win.controller.listening ? "Работает" : "Запускается…"
                                color: Theme.text
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                            }
                        }
                        Text {
                            Layout.fillWidth: true
                            visible: win.controller.accessibilityGranted
                            text: "Нажмите " + win.controller.leaderLabel + ", чтобы начать"
                            color: Theme.textSecondary
                            font.pixelSize: 11
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
        }

        Rectangle { Layout.fillHeight: true; implicitWidth: 1; color: Theme.border }

        // ================================================= content
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // ---- permission banner
            Rectangle {
                visible: !win.controller.accessibilityGranted
                Layout.fillWidth: true
                Layout.margins: 16
                Layout.bottomMargin: 0
                implicitHeight: bannerRow.implicitHeight + 24
                radius: 12
                color: Qt.rgba(1, 0.62, 0.04, Theme.dark ? 0.14 : 0.12)
                border.color: Qt.rgba(1, 0.62, 0.04, 0.35)

                RowLayout {
                    id: bannerRow
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 14
                    anchors.rightMargin: 12
                    spacing: 12
                    Icon { name: "shield"; size: 22; color: Theme.warning }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text {
                            text: "Разрешите Leap слышать клавиатуру"
                            color: Theme.text
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                        }
                        Text {
                            Layout.fillWidth: true
                            text: "Системные настройки → Конфиденциальность и безопасность → Универсальный доступ → включите Leap. Leap подхватит разрешение сам, перезапуск не нужен."
                            color: Theme.textSecondary
                            font.pixelSize: 12
                            wrapMode: Text.WordWrap
                        }
                    }
                    PushButton {
                        text: "Открыть настройки"
                        kind: "primary"
                        onClicked: win.controller.requestAccessibility()
                    }
                }
            }

            // ---- keys page
            RowLayout {
                visible: win.page === "keys"
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // tree
                ColumnLayout {
                    Layout.fillHeight: true
                    // Fixed width: its children fill width, which would make the
                    // column greedy and squeeze the editor to nothing.
                    Layout.fillWidth: false
                    Layout.preferredWidth: 380
                    Layout.minimumWidth: 320
                    Layout.maximumWidth: 380
                    Layout.margins: 16
                    Layout.rightMargin: 8
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text {
                            Layout.fillWidth: true
                            text: "Клавиши"
                            color: Theme.text
                            font.pixelSize: 18
                            font.weight: Font.DemiBold
                        }
                        PushButton {
                            text: "Действие"
                            icon: "plus"
                            kind: "primary"
                            onClicked: win.select(win.controller.addNode(win.targetGroup(), false))
                        }
                        PushButton {
                            text: "Группа"
                            icon: "layers"
                            onClicked: win.select(win.controller.addNode(win.targetGroup(), true))
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 12
                        color: Theme.card
                        border.color: Theme.border
                        clip: true

                        ListView {
                            id: tree
                            anchors.fill: parent
                            anchors.margins: 6
                            model: win.controller.rows
                            spacing: 1
                            boundsBehavior: Flickable.StopAtBounds
                            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                            // ---- drag and drop
                            property var dragItem: null   // row data being dragged
                            property bool dragging: false
                            property var drop: null       // { parent, index, into, y, depth } or null
                            property real pointerY: 0     // in the ListView's own coordinates

                            function isInside(path, ancestor) {
                                if (path.length < ancestor.length)
                                    return false
                                for (let i = 0; i < ancestor.length; ++i)
                                    if (path[i] !== ancestor[i])
                                        return false
                                return true
                            }
                            function rootCount() {
                                let n = 0
                                for (const r of win.controller.rows)
                                    if (r.depth === 0)
                                        ++n
                                return n
                            }
                            // Where a drop at the pointer would go.
                            function computeDrop() {
                                const cy = pointerY + contentY // content coordinates
                                let idx = indexAt(width / 2, cy)
                                if (idx < 0) // the 1 px gap between rows
                                    idx = indexAt(width / 2, cy - 2) >= 0 ? indexAt(width / 2, cy - 2) : indexAt(width / 2, cy + 2)
                                let d = null
                                if (idx < 0) {
                                    const last = count > 0 ? itemAtIndex(count - 1) : null
                                    if (last && cy > last.y + last.height)
                                        d = { parent: [], index: rootCount(), into: false, y: last.y + last.height, depth: 0 }
                                    else if (count > 0)
                                        d = { parent: [], index: 0, into: false, y: itemAtIndex(0) ? itemAtIndex(0).y : 0, depth: 0 }
                                } else {
                                    const item = itemAtIndex(idx)
                                    const r = win.controller.rows[idx]
                                    if (item && r) {
                                        const local = (cy - item.y) / item.height
                                        const parentPath = r.path.slice(0, -1)
                                        const at = r.path[r.path.length - 1]
                                        if (r.group && local > 0.3 && local < 0.7)
                                            d = { parent: r.path, index: 1e6, into: true, y: item.y, depth: r.depth + 1, row: idx }
                                        else if (local <= 0.5)
                                            d = { parent: parentPath, index: at, into: false, y: item.y, depth: r.depth }
                                        else if (r.group)
                                            d = { parent: r.path, index: 0, into: false, y: item.y + item.height, depth: r.depth + 1 }
                                        else
                                            d = { parent: parentPath, index: at + 1, into: false, y: item.y + item.height, depth: r.depth }
                                    }
                                }
                                // A group can't go inside itself.
                                if (d && isInside(d.parent, dragItem.path))
                                    d = null
                                drop = d
                            }
                            function startDrag(rowData) {
                                win.select(rowData.path)
                                dragItem = rowData
                                dragging = true
                                computeDrop()
                            }
                            function moveDrag(y) {
                                pointerY = y
                                if (dragging)
                                    computeDrop()
                            }
                            function endDrag() {
                                const d = drop
                                const from = dragItem ? dragItem.path : null
                                dragging = false
                                drop = null
                                dragItem = null
                                if (!d || !from)
                                    return
                                // Later: the move rebuilds the rows, and this runs inside a
                                // row's mouse handler.
                                Qt.callLater(() => {
                                    const moved = win.controller.moveNodeTo(from, d.parent, d.index)
                                    if (moved.length)
                                        win.select(moved)
                                })
                            }

                            // Scrolls while the pointer is near the top or bottom edge.
                            Timer {
                                interval: 16
                                repeat: true
                                running: tree.dragging && (tree.pointerY < 36 || tree.pointerY > tree.height - 36)
                                onTriggered: {
                                    const step = tree.pointerY < 36 ? -8 : 8
                                    const top = tree.originY
                                    const bottom = tree.originY + tree.contentHeight - tree.height
                                    tree.contentY = Math.max(top, Math.min(bottom, tree.contentY + step))
                                    tree.computeDrop()
                                }
                            }

                            // Insertion line / "into group" frame, in content coordinates.
                            Rectangle {
                                parent: tree.contentItem
                                z: 10
                                visible: tree.dragging && tree.drop !== null && !tree.drop.into
                                x: 8 + (tree.drop ? tree.drop.depth : 0) * 20
                                y: (tree.drop ? tree.drop.y : 0) - 1.5
                                width: tree.width - x - 8
                                height: 3
                                radius: 1.5
                                color: Theme.accent
                                Rectangle {
                                    x: -4
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 9
                                    height: 9
                                    radius: 4.5
                                    color: Theme.windowBg
                                    border.width: 2
                                    border.color: Theme.accent
                                }
                            }
                            Rectangle {
                                parent: tree.contentItem
                                z: 10
                                visible: tree.dragging && tree.drop !== null && tree.drop.into
                                x: 2
                                y: tree.drop ? tree.drop.y : 0
                                width: tree.width - 4
                                height: 40
                                radius: 8
                                color: "transparent"
                                border.width: 2
                                border.color: Theme.accent
                            }

                            // What's being dragged, following the pointer (viewport
                            // coordinates: Flickable children default to the content).
                            Rectangle {
                                parent: tree
                                z: 20
                                visible: tree.dragging
                                x: 24
                                y: tree.pointerY - height / 2
                                width: ghostRow.implicitWidth + 24
                                height: 34
                                radius: 8
                                color: Theme.card
                                border.color: Theme.accent
                                border.width: 1
                                opacity: 0.95
                                Row {
                                    id: ghostRow
                                    anchors.centerIn: parent
                                    spacing: 10
                                    KeyCap {
                                        anchors.verticalCenter: parent.verticalCenter
                                        label: tree.dragItem ? (tree.dragItem.keyLabel || "?") : ""
                                        size: 22
                                    }
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: tree.dragItem ? (tree.dragItem.title || "Без названия") : ""
                                        color: Theme.text
                                        font.pixelSize: 13
                                        font.weight: Font.DemiBold
                                    }
                                }
                            }

                            // Root row: clicking it adds to the top level.
                            header: Item {
                                width: tree.width
                                height: 34
                                Rectangle {
                                    anchors.fill: parent
                                    anchors.bottomMargin: 2
                                    radius: 8
                                    color: !win.selectedPath.length ? Theme.selection : rootMouse.containsMouse ? Theme.hover : "transparent"
                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 10
                                        spacing: 8
                                        KeyCap { label: win.controller.leaderLabel; size: 22 }
                                        Text {
                                            Layout.fillWidth: true
                                            text: "Ведущая клавиша"
                                            color: Theme.textSecondary
                                            font.pixelSize: 12
                                        }
                                    }
                                    MouseArea {
                                        id: rootMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onClicked: win.select([])
                                    }
                                }
                            }

                            delegate: Rectangle {
                                id: row
                                required property var modelData
                                readonly property bool selected: JSON.stringify(modelData.path) === win.selectedKey
                                width: tree.width
                                height: 40
                                radius: 8
                                opacity: tree.dragging && tree.dragItem && JSON.stringify(tree.dragItem.path) === JSON.stringify(modelData.path) ? 0.35 : 1
                                color: selected ? Theme.selection : rowMouse.containsMouse && !tree.dragging ? Theme.hover : "transparent"

                                // Tree guide for nested rows.
                                Repeater {
                                    model: row.modelData.depth
                                    Rectangle {
                                        required property int index
                                        x: 22 + index * 20
                                        width: 1
                                        height: row.height + 1
                                        color: Theme.border
                                    }
                                }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 10 + row.modelData.depth * 20
                                    anchors.rightMargin: 10
                                    spacing: 10
                                    KeyCap {
                                        label: row.modelData.keyLabel || "?"
                                        size: 24
                                        error: row.modelData.conflict
                                    }
                                    ItemIcon { item: row.modelData; size: 22 }
                                    Text {
                                        Layout.fillWidth: true
                                        text: row.modelData.title || "Без названия"
                                        color: Theme.text
                                        font.pixelSize: 13
                                        font.weight: row.modelData.group ? Font.DemiBold : Font.Normal
                                        elide: Text.ElideRight
                                    }
                                    Icon {
                                        visible: row.modelData.sticky
                                        name: "repeat"
                                        size: 13
                                        color: Theme.textTertiary
                                    }
                                    Item { implicitWidth: 24; implicitHeight: 24 } // room for the trash button
                                }
                                // Click selects; press and drag reorders. preventStealing keeps
                                // the ListView from turning the drag into a scroll.
                                MouseArea {
                                    id: rowMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    preventStealing: true
                                    cursorShape: tree.dragging ? Qt.ClosedHandCursor : Qt.ArrowCursor
                                    property real pressY: 0
                                    property bool dragged: false
                                    onPressed: mouse => {
                                        pressY = mapToItem(tree, mouse.x, mouse.y).y
                                        dragged = false
                                    }
                                    onPositionChanged: mouse => {
                                        if (!pressed)
                                            return
                                        const y = mapToItem(tree, mouse.x, mouse.y).y
                                        if (!tree.dragging && Math.abs(y - pressY) > 6) {
                                            dragged = true
                                            tree.pointerY = y
                                            tree.startDrag(row.modelData)
                                        }
                                        tree.moveDrag(y)
                                    }
                                    onReleased: if (tree.dragging) tree.endDrag()
                                    onCanceled: if (tree.dragging) { tree.dragging = false; tree.drop = null; tree.dragItem = null }
                                    onClicked: if (!dragged) win.select(row.modelData.path)
                                }
                                // Delete right from the list (above rowMouse, so it gets the click).
                                Rectangle {
                                    visible: !tree.dragging && (rowMouse.containsMouse || trashMouse.containsMouse || row.selected)
                                    anchors.right: parent.right
                                    anchors.rightMargin: 8
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 24
                                    height: 24
                                    radius: 6
                                    color: trashMouse.containsMouse ? Qt.rgba(1, 0.27, 0.23, 0.16) : "transparent"
                                    Icon {
                                        anchors.centerIn: parent
                                        name: "trash"
                                        size: 14
                                        color: trashMouse.containsMouse ? Theme.danger : Theme.textTertiary
                                    }
                                    MouseArea {
                                        id: trashMouse
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            win.select(row.modelData.path)
                                            win.removeSelected()
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Text {
                        visible: win.controller.conflictCount > 0
                        Layout.fillWidth: true
                        text: "Есть повторяющиеся клавиши в одной группе — они отмечены красным."
                        color: Theme.danger
                        font.pixelSize: 12
                        wrapMode: Text.WordWrap
                    }
                }

                // editor
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumWidth: 400
                    Layout.margins: 16
                    Layout.leftMargin: 8
                    radius: 12
                    color: Theme.card
                    border.color: Theme.border

                    Editor {
                        id: editor
                        anchors.fill: parent
                        anchors.margins: 20
                        visible: win.selectedPath.length > 0
                        controller: win.controller
                        path: win.selectedPath
                        onRemoved: win.removeSelected()
                        onMoved: newPath => win.select(newPath)
                    }

                    // Nothing selected: how it works.
                    ColumnLayout {
                        visible: !editor.visible
                        anchors.centerIn: parent
                        width: Math.min(parent.width - 60, 420)
                        spacing: 16

                        Row {
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 8
                            KeyCap { label: win.controller.leaderLabel; size: 40; highlighted: true }
                            Text { text: "→"; color: Theme.textTertiary; font.pixelSize: 22; anchors.verticalCenter: parent.verticalCenter }
                            KeyCap { label: "W"; size: 40 }
                            Text { text: "→"; color: Theme.textTertiary; font.pixelSize: 22; anchors.verticalCenter: parent.verticalCenter }
                            KeyCap { label: "H"; size: 40 }
                        }
                        Text {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                            text: "Ведущая клавиша, потом по одной клавише"
                            color: Theme.text
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                            wrapMode: Text.WordWrap
                        }
                        Text {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                            text: "Нажмите " + win.controller.leaderLabel + " — появится подсказка со всеми клавишами. "
                                  + "Буква открывает приложение или запускает действие, группа открывает следующий уровень. "
                                  + "Клавиши привязаны к месту на клавиатуре, поэтому работают и в русской раскладке.\n\n"
                                  + "Выберите строку слева, чтобы изменить её."
                            color: Theme.textSecondary
                            font.pixelSize: 13
                            lineHeight: 1.25
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }

            // ---- general page
            GeneralPage {
                visible: win.page === "general"
                controller: win.controller
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 24
            }
        }
    }

    // ---- toast
    Rectangle {
        id: toast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: shown ? 24 : -height
        width: Math.min(toastText.implicitWidth + 32, parent.width - 80)
        height: 38
        radius: 19
        color: Theme.toastBg
        opacity: shown ? 1 : 0
        property bool shown: false
        Behavior on anchors.bottomMargin { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
        Behavior on opacity { NumberAnimation { duration: 180 } }

        function show(message) {
            toastText.text = message
            shown = true
            toastTimer.restart()
        }
        Text {
            id: toastText
            anchors.centerIn: parent
            width: parent.width - 32
            horizontalAlignment: Text.AlignHCenter
            color: "white"
            font.pixelSize: 13
            elide: Text.ElideRight
        }
        Timer { id: toastTimer; interval: 2800; onTriggered: toast.shown = false }
    }

    component NavItem: Rectangle {
        id: nav
        property string title
        property string icon
        property bool current: false
        signal activated()
        Layout.fillWidth: true
        implicitHeight: 34
        radius: 8
        color: current ? Theme.selection : navMouse.containsMouse ? Theme.hover : "transparent"
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            spacing: 10
            Icon { name: nav.icon; size: 16; color: nav.current ? Theme.accentText : Theme.textSecondary }
            Text {
                Layout.fillWidth: true
                text: nav.title
                color: nav.current ? Theme.text : Theme.textSecondary
                font.pixelSize: 13
                font.weight: nav.current ? Font.DemiBold : Font.Normal
            }
        }
        MouseArea {
            id: navMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: nav.activated()
        }
    }
}
