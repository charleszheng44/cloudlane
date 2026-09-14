import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "Actions.js" as Actions

Rectangle {
    id: bar
    objectName: "playerBar"
    required property var app
    property real audibleVolume: 65
    signal optionsRequested()
    implicitHeight: content.implicitHeight + 24
    color: Backend.theme.dark_background
    function toggleMute() {
        if (Backend.volume > 0) {
            audibleVolume = Backend.volume
            Backend.setVolume(0)
        } else Backend.setVolume(audibleVolume)
    }
    Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Backend.theme.selection }
    ColumnLayout {
        id: content
        anchors.fill: parent; anchors.margins: 12
        spacing: 6
        RowLayout {
            Layout.fillWidth: true
            spacing: 16
            RowLayout {
                Layout.preferredWidth: (bar.width - 24 - 32 - transport.implicitWidth) / 2
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                spacing: 10
                Rectangle {
                    visible: bar.width >= 760
                    Layout.preferredWidth: 46; Layout.preferredHeight: 46
                    color: Backend.theme.selection
                    PlayerIcon { anchors.centerIn: parent; name: "music"; opacity: .5; visible: artwork.status !== Image.Ready }
                    Image { id: artwork; anchors.fill: parent; source: bar.app.currentTrack.cover || ""; fillMode: Image.PreserveAspectCrop; asynchronous: true }
                }
                ColumnLayout {
                    Layout.fillWidth: true; Layout.minimumWidth: 0
                    Layout.maximumWidth: bar.width >= 1000 ? 240 : Infinity
                    spacing: 2
                    Label { text: bar.app.currentTrack.name || "选择一首音乐"; Layout.fillWidth: true; elide: Text.ElideRight; textFormat: Text.PlainText }
                    Label { text: bar.app.currentTrack.artist || ""; visible: !!text; Layout.fillWidth: true; elide: Text.ElideRight; opacity: .65; font.pixelSize: Math.max(11, bar.app.font.pixelSize - 2); textFormat: Text.PlainText }
                    Label {
                        text: bar.app.playerStatus || bar.app.actualQuality
                        visible: !!text; Layout.fillWidth: true; elide: Text.ElideRight
                        font.pixelSize: 10; opacity: .75
                        ToolTip.visible: statusHover.hovered; ToolTip.text: text
                        HoverHandler { id: statusHover }
                    }
                }
                PlayerButton {
                    visible: bar.width >= 1000
                    symbol: selected ? "heart-filled" : "heart"
                    selected: bar.app.likedIds.indexOf(bar.app.currentTrack.id) >= 0
                    label: selected ? "取消喜欢" : "喜欢当前歌曲"
                    enabled: bar.app.currentTrack.kind === "song" && !bar.app.writeBusy
                    onClicked: Actions.like(bar.app.currentTrack)
                }
                Item { visible: bar.width >= 1000; Layout.fillWidth: true }
            }
            RowLayout {
                id: transport
                spacing: 6
                PlayerButton {
                    visible: bar.width >= 900
                    symbol: "shuffle"; label: "打乱后续歌曲"
                    enabled: !bar.app.fm && !bar.app.videoSession && bar.app.queue.length > 1
                    onClicked: Actions.shuffle()
                }
                PlayerButton {
                    objectName: "previousButton"
                    symbol: "previous"; label: "上一首"
                    enabled: !bar.app.videoSession && (bar.app.queueIndex > 0 || Backend.loaded)
                    onClicked: Actions.previous(false)
                }
                PlayerButton {
                    objectName: "playPauseButton"
                    symbol: Backend.playing ? "pause" : "play"
                    label: Backend.playing ? "暂停" : "播放"
                    primary: true
                    enabled: !!bar.app.currentTrack.id
                    onClicked: Actions.toggle()
                }
                PlayerButton {
                    objectName: "nextButton"
                    symbol: "next"; label: "下一首"
                    enabled: !bar.app.videoSession && (bar.app.fm || bar.app.queueIndex + 1 < bar.app.queue.length || (bar.app.repeatMode === 1 && bar.app.queue.length > 0))
                    onClicked: Actions.next(false, false)
                }
                PlayerButton {
                    visible: bar.width >= 900
                    symbol: bar.app.repeatMode === 2 ? "repeat-one" : "repeat"
                    label: ["顺序播放", "列表循环", "单曲循环"][bar.app.repeatMode]
                    selected: bar.app.repeatMode > 0
                    onClicked: {bar.app.repeatMode = (bar.app.repeatMode + 1) % 3; Actions.updateMetadata()}
                }
            }
            RowLayout {
                Layout.preferredWidth: (bar.width - 24 - 32 - transport.implicitWidth) / 2
                Layout.fillWidth: true
                spacing: 4
                Item { Layout.fillWidth: true }
                PlayerButton {
                    objectName: "muteButton"
                    symbol: Backend.volume > 0 ? "volume" : "mute"
                    label: Backend.volume > 0 ? "静音" : "取消静音"
                    onClicked: bar.toggleMute()
                }
                PlayerSlider {
                    id: volumeControl
                    objectName: "volumeSlider"
                    Layout.preferredWidth: bar.width >= 900 ? 100 : 72
                    from: 0; to: 100; stepSize: 1
                    Binding { target: volumeControl; property: "value"; value: Backend.volume; when: !volumeControl.pressed; restoreMode: Binding.RestoreNone }
                    onMoved: Backend.setVolume(value)
                    Accessible.name: "音量"
                    ToolTip.visible: hovered || pressed || activeFocus
                    ToolTip.text: Math.round(value) + "%"
                }
                PlayerButton {
                    visible: bar.width >= 1100
                    symbol: "now-playing"; label: "正在播放"
                    selected: bar.app.panel === "正在播放"
                    onClicked: bar.app.panel = selected ? "" : "正在播放"
                }
                PlayerButton {
                    visible: bar.width >= 900
                    symbol: "lyrics"; label: "歌词"
                    selected: bar.app.panel === "歌词"
                    onClicked: bar.app.panel = selected ? "" : "歌词"
                }
                PlayerButton {
                    objectName: "queueButton"
                    symbol: "queue"; label: "播放队列（" + bar.app.queue.length + "）"
                    selected: bar.app.panel === "队列"
                    onClicked: bar.app.panel = selected ? "" : "队列"
                }
                PlayerButton { symbol: "more"; label: "播放器选项"; onClicked: bar.optionsRequested() }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: bar.width >= 900 ? bar.width * .27 : 0
            Layout.rightMargin: Layout.leftMargin
            spacing: 8
            Label { text: bar.app.formatTime(Backend.position); Layout.minimumWidth: 36; font.pixelSize: 11; opacity: .65 }
            PlayerSlider {
                id: seekControl
                objectName: "seekSlider"
                Layout.fillWidth: true
                from: Backend.seekMinimum; to: Math.max(Backend.seekMinimum + 1, Backend.duration)
                enabled: Backend.loaded
                Binding { target: seekControl; property: "value"; value: Backend.position; when: !seekControl.pressed; restoreMode: Binding.RestoreNone }
                onMoved: Backend.seek(value)
                Accessible.name: "播放进度"
            }
            Label { text: bar.app.formatTime(Backend.duration); Layout.minimumWidth: 36; horizontalAlignment: Text.AlignRight; font.pixelSize: 11; opacity: .65 }
        }
    }
}
