import QtQuick
import QtQuick.Controls.Basic
import Leap

// ComboBox over a list of {id, title} maps.
ComboBox {
    id: root

    property string currentId: ""
    signal picked(string optionId)

    textRole: "title"
    valueRole: "id"
    implicitHeight: 32
    font.pixelSize: 13
    currentIndex: {
        for (let i = 0; i < (model ? model.length : 0); ++i)
            if (model[i].id === currentId)
                return i
        return -1
    }
    onActivated: index => root.picked(model[index].id)

    background: Rectangle {
        radius: 8
        color: root.hovered ? Theme.hover : Theme.field
        border.width: root.activeFocus ? 2 : 1
        border.color: root.activeFocus ? Theme.accent : Theme.border
    }
    contentItem: Text {
        leftPadding: 10
        rightPadding: 28
        text: root.displayText
        color: Theme.text
        font: root.font
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    indicator: Icon {
        x: root.width - width - 10
        y: (root.height - height) / 2
        name: "chevronDown"
        size: 14
        color: Theme.textSecondary
    }
    delegate: ItemDelegate {
        id: option
        required property var modelData
        required property int index
        width: ListView.view ? ListView.view.width : root.width
        height: 30
        contentItem: Text {
            text: option.modelData.title
            color: Theme.text
            font.pixelSize: 13
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: 6
            color: option.highlighted ? Theme.selection : "transparent"
        }
        highlighted: root.highlightedIndex === index
    }
    popup: Popup {
        y: root.height + 4
        width: root.width
        implicitHeight: Math.min(contentItem.implicitHeight + 8, 360)
        padding: 4
        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: root.popup.visible ? root.delegateModel : null
            currentIndex: root.highlightedIndex
        }
        background: Rectangle {
            radius: 10
            color: Theme.card
            border.color: Theme.borderStrong
        }
    }
}
