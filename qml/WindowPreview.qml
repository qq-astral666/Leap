import QtQuick
import Leap

// A tiny screen showing where a window command puts the window.
// `rect` is [x, y, w, h] in 0..1; empty for "next display".
Item {
    id: root

    property var rect: []
    property color color: Theme.typeColor("window")
    property real size: 24

    implicitWidth: size
    implicitHeight: size * 0.72

    Rectangle {
        id: screen
        anchors.fill: parent
        radius: 3
        color: "transparent"
        border.color: root.color
        border.width: 1.4
        opacity: 0.85
    }

    Rectangle {
        visible: root.rect.length === 4
        x: 2.5 + (root.rect[0] ?? 0) * (root.width - 5)
        y: 2.5 + (root.rect[1] ?? 0) * (root.height - 5)
        width: Math.max(2, (root.rect[2] ?? 0) * (root.width - 5))
        height: Math.max(2, (root.rect[3] ?? 0) * (root.height - 5))
        radius: 1.5
        color: root.color
    }

    // "Next display": an arrow out of the screen.
    Icon {
        visible: root.rect.length !== 4
        anchors.centerIn: parent
        name: "chevronRight"
        size: root.height * 0.9
        color: root.color
        strokeWidth: 2.4
    }
}
