import QtQuick
import QtQuick.Controls

Button {
    id: control
    property string symbol: "play"
    property string label: ""
    property bool primary: false
    property bool selected: false
    implicitWidth: primary ? 44 : 34
    implicitHeight: implicitWidth
    padding: primary ? 10 : 6
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    Accessible.name: label
    ToolTip.visible: hovered || activeFocus
    ToolTip.delay: 500
    ToolTip.text: label
    contentItem: PlayerIcon {
        name: control.symbol
        color: control.primary ? Backend.theme.background : control.selected ? Backend.theme.accent : Backend.theme.foreground
        opacity: control.enabled ? 1 : .32
    }
    background: Rectangle {
        radius: control.primary ? width / 2 : 4
        color: control.primary ? Backend.theme.accent : control.down || control.selected ? Backend.theme.selection : control.hovered ? Qt.rgba(1,1,1,.08) : "transparent"
        opacity: control.enabled ? (control.primary && control.down ? .75 : 1) : .35
        border.width: control.activeFocus ? 1 : 0
        border.color: Backend.theme.foreground
    }
}
