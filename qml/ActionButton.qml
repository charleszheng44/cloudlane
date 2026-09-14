import QtQuick
import QtQuick.Controls
Button {
    id: control
    property bool selected: false
    property bool quiet: false
    hoverEnabled: true
    implicitHeight: Math.max(34, font.pixelSize * 2.45)
    implicitWidth: Math.max(34, contentItem.implicitWidth + 22)
    padding: 8
    Accessible.name: text
    contentItem: Text {
        text: control.text; font: control.font
        color: !control.enabled ? Backend.theme.muted : control.selected ? Backend.theme.accent : Backend.theme.foreground
        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        color: control.down || control.selected ? Backend.theme.selection : control.hovered ? Backend.theme.hover : "transparent"
        border.width: control.activeFocus || (!control.quiet && !control.selected) ? 1 : 0
        border.color: control.activeFocus ? Backend.theme.accent : Backend.theme.border
    }
}
