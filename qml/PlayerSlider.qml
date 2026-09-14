import QtQuick
import QtQuick.Controls

Slider {
    id: control
    implicitHeight: 22
    implicitWidth: 100
    leftPadding: 5; rightPadding: 5
    hoverEnabled: true
    background: Rectangle {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: control.availableWidth; height: 3
        color: Backend.theme.selection
        Rectangle {
            width: parent.width * control.visualPosition; height: parent.height
            color: Backend.theme.accent
            opacity: control.enabled ? 1 : .3
        }
    }
    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: 10; implicitHeight: 10
        color: Backend.theme.foreground
        visible: control.enabled && (control.hovered || control.pressed || control.activeFocus)
    }
}
