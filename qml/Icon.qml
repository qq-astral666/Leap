import QtQuick
import QtQuick.Shapes

// Stroke icons from SVG path data (24×24 grid, Lucide-style): no image assets.
Item {
    id: root

    property string name: ""
    property color color: "white"
    property real size: 16
    property real strokeWidth: 1.9

    implicitWidth: size
    implicitHeight: size
    width: size
    height: size

    readonly property var paths: ({
        "layers": "M12 2l10 5l-10 5l-10 -5z M2 17l10 5l10 -5 M2 12l10 5l10 -5",
        "globe": "M2 12a10 10 0 1 0 20 0a10 10 0 1 0 -20 0 M2 12h20 M12 2a15.3 15.3 0 0 1 4 10a15.3 15.3 0 0 1 -4 10a15.3 15.3 0 0 1 -4 -10a15.3 15.3 0 0 1 4 -10z",
        "terminal": "M4 17l6 -6l-6 -6 M12 19h8",
        "type": "M4 7V4h16v3 M9 20h6 M12 4v16",
        "window": "M4 4h16a2 2 0 0 1 2 2v12a2 2 0 0 1 -2 2H4a2 2 0 0 1 -2 -2V6a2 2 0 0 1 2 -2z M2 9h20",
        "app": "M4 4h6v6H4z M14 4h6v6h-6z M4 14h6v6H4z M14 14h6v6h-6z",
        "folder": "M4 4h5l2 3h9a2 2 0 0 1 2 2v9a2 2 0 0 1 -2 2H4a2 2 0 0 1 -2 -2V6a2 2 0 0 1 2 -2z",
        "file": "M14 2H6a2 2 0 0 0 -2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2 -2V8z M14 2v6h6",
        "volumeUp": "M11 5L6 9H2v6h4l5 4z M15.5 8.5a5 5 0 0 1 0 7 M19 5a10 10 0 0 1 0 14",
        "volumeDown": "M11 5L6 9H2v6h4l5 4z M15.5 8.5a5 5 0 0 1 0 7",
        "volumeX": "M11 5L6 9H2v6h4l5 4z M22 9l-6 6 M16 9l6 6",
        "moon": "M12 3a6 6 0 0 0 9 9a9 9 0 1 1 -9 -9z",
        "monitor": "M4 3h16a2 2 0 0 1 2 2v10a2 2 0 0 1 -2 2H4a2 2 0 0 1 -2 -2V5a2 2 0 0 1 2 -2z M8 21h8 M12 17v4",
        "lock": "M5 11h14a2 2 0 0 1 2 2v7a2 2 0 0 1 -2 2H5a2 2 0 0 1 -2 -2v-7a2 2 0 0 1 2 -2z M7 11V7a5 5 0 0 1 10 0v4",
        "zap": "M13 2L3 14h9l-1 8l10 -12h-9z",
        "plus": "M12 5v14 M5 12h14",
        "x": "M18 6L6 18 M6 6l12 12",
        "trash": "M3 6h18 M8 6V4h8v2 M6 6l1 14h10l1 -14 M10 11v6 M14 11v6",
        "check": "M5 12l5 5L20 7",
        "alert": "M12 9v4 M12 17h.01 M10.3 3.9L1.8 18a2 2 0 0 0 1.7 3h17a2 2 0 0 0 1.7 -3L13.7 3.9a2 2 0 0 0 -3.4 0z",
        "external": "M15 3h6v6 M10 14L21 3 M18 13v6a2 2 0 0 1 -2 2H5a2 2 0 0 1 -2 -2V8a2 2 0 0 1 2 -2h6",
        "chevronUp": "M18 15l-6 -6l-6 6",
        "chevronDown": "M6 9l6 6l6 -6",
        "chevronRight": "M9 18l6 -6l-6 -6",
        "play": "M7 4l12 8l-12 8z",
        "keyboard": "M4 6h16a2 2 0 0 1 2 2v8a2 2 0 0 1 -2 2H4a2 2 0 0 1 -2 -2V8a2 2 0 0 1 2 -2z M6 10h.01 M10 10h.01 M14 10h.01 M18 10h.01 M8 14h8",
        "sliders": "M4 21v-7 M4 10V3 M12 21v-9 M12 8V3 M20 21v-5 M20 12V3 M1 14h6 M9 8h6 M17 16h6",
        "shield": "M12 22s8 -4 8 -10V5l-8 -3l-8 3v7c0 6 8 10 8 10z",
        "refresh": "M21 12a9 9 0 1 1 -2.64 -6.36L21 8 M21 3v5h-5",
        "repeat": "M17 2l4 4l-4 4 M3 11V10a4 4 0 0 1 4 -4h14 M7 22l-4 -4l4 -4 M21 13v1a4 4 0 0 1 -4 4H3"
    })

    Shape {
        width: 24
        height: 24
        scale: root.size / 24
        transformOrigin: Item.TopLeft
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            strokeColor: root.color
            strokeWidth: root.strokeWidth
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            PathSvg { path: root.paths[root.name] ?? "" }
        }
    }
}
