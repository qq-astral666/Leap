import QtQuick
import Leap

// Icon for a key-tree item: the Finder icon for apps, files and folders, a
// mini window diagram for window commands, a tinted glyph tile otherwise.
Item {
    id: root

    property var item: ({})
    property real size: 26

    implicitWidth: size
    implicitHeight: size

    readonly property string type: item.type ?? ""
    readonly property bool hasFileIcon: (item.icon ?? "") !== ""

    Image {
        anchors.fill: parent
        visible: root.hasFileIcon
        source: root.hasFileIcon ? root.item.icon : ""
        sourceSize: Qt.size(root.size * 2, root.size * 2)
        smooth: true
        mipmap: true
        asynchronous: true
    }

    Rectangle {
        anchors.fill: parent
        visible: !root.hasFileIcon
        radius: root.size * 0.28
        color: Theme.typeTint(root.type)

        WindowPreview {
            visible: root.type === "window"
            anchors.centerIn: parent
            size: root.size * 0.66
            rect: root.item.preview ?? []
        }

        Icon {
            visible: root.type !== "window"
            anchors.centerIn: parent
            name: root.item.glyph || (root.type === "app" ? "app" : root.type === "open" ? "folder" : "zap")
            size: root.size * 0.58
            color: Theme.typeColor(root.type)
            strokeWidth: 2
        }
    }
}
