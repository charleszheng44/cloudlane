import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "Actions.js" as Actions

Rectangle {
    id: sidebar
    required property var app
    readonly property bool expanded: width > 100
    signal settingsRequested()
    color: Backend.theme.background
    radius: 8
    component NavigationButton: Button {
        id: navigation
        property string symbol: "library"
        property bool selected: false
        implicitHeight: 38
        padding: 8
        hoverEnabled: true
        Accessible.name: text
        ToolTip.visible: hovered && !sidebar.expanded
        ToolTip.text: text
        contentItem: RowLayout {
            spacing: 12
            PlayerIcon { name: navigation.symbol; Layout.preferredWidth: 20; Layout.preferredHeight: 20; color: navigation.selected ? Backend.theme.accent : Backend.theme.foreground }
            Label { visible: sidebar.expanded; text: navigation.text; elide: Text.ElideRight; Layout.fillWidth: true; opacity: navigation.selected ? 1 : .8 }
        }
        background: Rectangle { radius: 4; color: navigation.selected || navigation.hovered ? Backend.theme.selection : "transparent"; border.width: navigation.activeFocus ? 1 : 0; border.color: Backend.theme.accent }
    }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 10
        spacing: 6
        NavigationButton { text: "发现音乐"; symbol: "discover"; selected: sidebar.app.nav === "发现"; Layout.fillWidth: true; onClicked: Actions.navigate("发现") }
        NavigationButton { text: "动态"; symbol: "activity"; selected: sidebar.app.nav === "动态"; Layout.fillWidth: true; onClicked: Actions.navigate("动态") }
        Rectangle { Layout.fillWidth: true; height: 1; color: Backend.theme.selection; Layout.topMargin: 4; Layout.bottomMargin: 4 }
        NavigationButton { objectName: "libraryNavigation"; text: "我的音乐"; symbol: "library"; selected: sidebar.app.nav === "我的音乐"; Layout.fillWidth: true; onClicked: Actions.navigate("我的音乐") }
        Flickable {
            visible: sidebar.expanded
            Layout.fillWidth: true; Layout.preferredHeight: 32
            clip: true; contentWidth: filters.implicitWidth
            RowLayout {
                id: filters; spacing: 6
                Repeater {
                    model: ["歌单", "专辑", "歌手", "播客"]
                    ActionButton { required property string modelData; text: modelData; quiet: true; selected: sidebar.app.nav === "我的音乐" && sidebar.app.category === modelData; onClicked: Actions.navigate("我的音乐", modelData) }
                }
            }
        }
        NavigationButton { text: "喜欢的音乐"; symbol: "heart"; selected: sidebar.app.nav === "我的音乐" && sidebar.app.category === "喜欢的音乐"; Layout.fillWidth: true; onClicked: Actions.navigate("我的音乐", "喜欢的音乐") }
        ListView {
            id: playlists
            objectName: "sidebarPlaylists"
            Layout.fillWidth: true; Layout.fillHeight: true
            clip: true; spacing: 2
            model: sidebar.app.libraryPlaylists
            ScrollBar.vertical: ScrollBar {}
            delegate: ItemDelegate {
                id: playlist
                required property var modelData
                width: playlists.width; height: 56; padding: 6
                background: Rectangle { radius: 4; color: playlist.hovered || sidebar.app.resource.id === playlist.modelData.id ? Backend.theme.selection : "transparent" }
                contentItem: RowLayout {
                    spacing: 10
                    Rectangle {
                        Layout.preferredWidth: 40; Layout.preferredHeight: 40
                        color: Backend.theme.selection
                        PlayerIcon { anchors.centerIn: parent; name: "music"; opacity: .4 }
                        Image { anchors.fill: parent; source: playlist.modelData.cover; sourceSize.width: 80; sourceSize.height: 80; asynchronous: true; fillMode: Image.PreserveAspectCrop }
                    }
                    ColumnLayout {
                        visible: sidebar.expanded
                        Layout.fillWidth: true; spacing: 3
                        Label { text: playlist.modelData.name; Layout.fillWidth: true; elide: Text.ElideRight; textFormat: Text.PlainText }
                        Label { text: "歌单 · " + playlist.modelData.artist; Layout.fillWidth: true; elide: Text.ElideRight; font.pixelSize: 11; opacity: .6; textFormat: Text.PlainText }
                    }
                }
                Accessible.name: modelData.name
                ToolTip.visible: hovered && !sidebar.expanded; ToolTip.text: modelData.name
                onClicked: Actions.open(modelData)
            }
            Label { anchors.centerIn: parent; width: parent.width - 12; visible: sidebar.expanded && !sidebar.app.profile.userId; text: "登录后，你的歌单会显示在这里"; wrapMode: Text.Wrap; opacity: .6; font.pixelSize: 12 }
        }
        NavigationButton { text: "下载"; symbol: "download"; Layout.fillWidth: true; onClicked: Actions.navigate("我的音乐", "下载") }
        NavigationButton { text: "本地音乐"; symbol: "music"; Layout.fillWidth: true; onClicked: Actions.navigate("我的音乐", "本地音乐") }
        NavigationButton { text: "偏好设置"; symbol: "settings"; Layout.fillWidth: true; onClicked: sidebar.settingsRequested() }
    }
}
