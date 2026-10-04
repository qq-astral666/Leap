pragma Singleton
import QtQuick

// Design tokens shared by the overlay and the settings window.
QtObject {
    readonly property bool dark: Application.styleHints.colorScheme !== Qt.ColorScheme.Light

    // ---- surfaces
    readonly property color windowBg: dark ? "#16161a" : "#f5f5f7"
    readonly property color sidebarBg: dark ? "#1c1c21" : "#ececf0"
    readonly property color card: dark ? "#212127" : "#ffffff"
    readonly property color panel: dark ? Qt.rgba(0.11, 0.11, 0.13, 0.96) : Qt.rgba(0.99, 0.99, 1.0, 0.97)
    readonly property color border: dark ? Qt.rgba(1, 1, 1, 0.08) : Qt.rgba(0, 0, 0, 0.09)
    readonly property color borderStrong: dark ? Qt.rgba(1, 1, 1, 0.16) : Qt.rgba(0, 0, 0, 0.18)
    readonly property color hover: dark ? Qt.rgba(1, 1, 1, 0.06) : Qt.rgba(0, 0, 0, 0.045)
    readonly property color selection: dark ? Qt.rgba(0.49, 0.36, 1.0, 0.22) : Qt.rgba(0.49, 0.36, 1.0, 0.13)
    readonly property color field: dark ? Qt.rgba(1, 1, 1, 0.06) : Qt.rgba(0, 0, 0, 0.05)
    readonly property color chip: dark ? Qt.rgba(1, 1, 1, 0.07) : Qt.rgba(0, 0, 0, 0.05)
    readonly property color toastBg: dark ? "#3a3a42" : "#1c1c1f"

    // ---- key caps
    readonly property color capTop: dark ? "#34343c" : "#ffffff"
    readonly property color capSide: dark ? "#1f1f25" : "#d4d4dc"
    readonly property color capText: dark ? "#f4f4f6" : "#18181b"

    // ---- text
    readonly property color text: dark ? "#f4f4f6" : "#18181b"
    readonly property color textSecondary: dark ? Qt.rgba(1, 1, 1, 0.60) : Qt.rgba(0, 0, 0, 0.58)
    readonly property color textTertiary: dark ? Qt.rgba(1, 1, 1, 0.38) : Qt.rgba(0, 0, 0, 0.38)

    readonly property color accent: "#7c5cff"
    readonly property color accentText: dark ? "#bcaeff" : "#5a3fe0"
    readonly property color danger: "#ff453a"
    readonly property color warning: "#ff9f0a"
    readonly property color success: "#30d158"
    readonly property string monoFont: Qt.platform.os === "osx" ? "Menlo" : "monospace"

    // ---- action type colors (icon tiles)
    function typeColor(type) {
        switch (type) {
        case "group": return "#7c5cff"
        case "url": return "#0a84ff"
        case "shell": return "#30d158"
        case "text": return "#ff9f0a"
        case "window": return "#64d2ff"
        case "system": return "#ff375f"
        default: return "#8e8e93"
        }
    }
    function typeTint(type) {
        const c = Qt.color(typeColor(type))
        return Qt.rgba(c.r, c.g, c.b, dark ? 0.20 : 0.14)
    }

    function plural(n, one, few, many) {
        const m10 = n % 10, m100 = n % 100
        if (m10 === 1 && m100 !== 11) return one
        if (m10 >= 2 && m10 <= 4 && (m100 < 10 || m100 >= 20)) return few
        return many
    }
}
