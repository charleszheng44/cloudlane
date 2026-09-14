import QtQuick
import Quickshell
import Quickshell.Services.Mpris

// Omarchy's documented widget injection surface. No private host services.
Item {
    id: root
    property QtObject bar: null
    property string moduleName: "io.github.charleszheng44.cloudlane"
    property var settings: ({})
    readonly property bool vertical: bar ? bar.vertical : false
    readonly property int barSize: bar ? bar.barSize : 32
    readonly property color foreground: bar ? bar.barForeground : "white"
    readonly property string fontFamily: bar ? bar.fontFamily : "monospace"
    readonly property var players: Mpris.players.values
    readonly property var player: {
        for (const candidate of players)
            if (candidate.dbusName === "org.mpris.MediaPlayer2.cloudlane") return candidate
        return null
    }
    readonly property string nowPlaying: player && player.trackTitle
        ? player.trackTitle + (player.trackArtist ? " · " + player.trackArtist : "") : "Cloudlane"
    implicitWidth: vertical ? barSize : controls.implicitWidth
    implicitHeight: barSize

    component Control: Item {
        id: button
        required property string glyph
        required property string description
        signal triggered()
        implicitWidth: root.barSize
        implicitHeight: root.barSize
        activeFocusOnTab: true
        opacity: enabled ? 1 : .4
        Accessible.role: Accessible.Button
        Accessible.name: description
        Accessible.onPressAction: triggered()
        Keys.onSpacePressed: triggered()
        Keys.onReturnPressed: triggered()
        Rectangle {
            anchors.fill: parent; anchors.margins: 3; radius: 4
            color: root.foreground; opacity: pointer.containsMouse ? .12 : 0
        }
        Rectangle {
            anchors.fill: parent; anchors.margins: 3; radius: 4
            color: "transparent"; border.color: root.foreground
            border.width: button.activeFocus ? 1 : 0
        }
        Text {
            anchors.centerIn: parent; text: button.glyph; textFormat: Text.PlainText
            color: root.foreground; font.family: root.fontFamily
            font.pixelSize: Math.round(root.barSize * .48)
        }
        MouseArea {
            id: pointer; anchors.fill: parent; hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: button.triggered()
            onEntered: if (root.bar) root.bar.showTooltip(button, button.description)
            onExited: if (root.bar) root.bar.hideTooltip(button)
        }
        Component.onDestruction: if (root.bar) root.bar.hideTooltip(button)
    }
    Row {
        id: controls
        Control {
            objectName: "cloudlaneLauncher"
            glyph: "󰝚"; description: "Open Cloudlane · " + root.nowPlaying
            onTriggered: Quickshell.execDetached(["cloudlane"])
        }
        Control {
            objectName: "cloudlanePrevious"
            visible: !!root.player && !root.vertical
            glyph: "󰒮"; description: "Previous track"
            enabled: !!root.player && root.player.canGoPrevious
            onTriggered: if (enabled) root.player.previous()
        }
        Control {
            objectName: "cloudlanePlayPause"
            visible: !!root.player && !root.vertical
            glyph: root.player && root.player.isPlaying ? "󰏤" : "󰐊"
            description: root.player && root.player.isPlaying ? "Pause" : "Play"
            enabled: !!root.player && root.player.canTogglePlaying
            onTriggered: if (enabled) root.player.togglePlaying()
        }
        Control {
            objectName: "cloudlaneNext"
            visible: !!root.player && !root.vertical
            glyph: "󰒭"; description: "Next track"
            enabled: !!root.player && root.player.canGoNext
            onTriggered: if (enabled) root.player.next()
        }
    }
}
