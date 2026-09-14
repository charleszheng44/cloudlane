import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "Actions.js" as Actions
import "Models.js" as Models
Rectangle {
    id: panel
    required property var app
    color: Backend.theme.background
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 16; spacing: 12
        RowLayout {
            Layout.fillWidth: true
            Label { text: app.panel; font.pixelSize: 20; Layout.fillWidth: true }
            ActionButton { text: "Close"; quiet: true; onClicked: app.panel="" }
        }
        Label {
            visible: app.panel === "Comments"
            text: app.contextTrack.name || ""; Layout.fillWidth: true; elide: Text.ElideRight
            color: Backend.theme.accent
        }
        RowLayout {
            visible: app.panel === "Queue"; Layout.fillWidth: true
            Label { text: app.queue.length + " tracks"; Layout.fillWidth: true }
            ActionButton { text: "Shuffle"; enabled: !app.fm && app.queue.length>1; onClicked: Actions.shuffle() }
        }
        ColumnLayout {
            visible: app.panel === "Lyrics"; Layout.fillWidth: true
            CheckBox { text:"Translation"; checked:app.translation; onToggled:app.translation=checked }
            RowLayout {
                Label { text:"Offset" }
                SpinBox { from:-100; to:100; value:app.lyricOffset*10; onValueModified:app.lyricOffset=value/10; Layout.fillWidth:true }
                Label { text:"× 0.1 sec"; font.pixelSize:11 }
            }
        }
        ComboBox {
            visible: app.panel === "Comments"; model:["Top comments","Newest comments"]
            onActivated:{app.commentSort=currentIndex===0?99:3;Actions.comments(app.contextTrack,false)}
        }
        Label { text:app.panelError; visible:app.panel==="Comments"&&text.length>0; wrapMode:Text.Wrap; Layout.fillWidth:true; color:Backend.theme.red }
        ColumnLayout {
            visible: app.panel === "Now Playing"
            Layout.fillWidth: true; Layout.fillHeight: true
            spacing: 16
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: width
                color: Backend.theme.selection
                PlayerIcon { anchors.centerIn: parent; width: 64; height: 64; name: "music"; opacity: .35 }
                Image { anchors.fill: parent; source: panel.app.currentTrack.cover || ""; fillMode: Image.PreserveAspectCrop; asynchronous: true }
            }
            Label { text: app.currentTrack.name || "Choose a track"; font.pixelSize: 22; font.weight: Font.DemiBold; Layout.fillWidth: true; wrapMode: Text.Wrap; textFormat: Text.PlainText }
            Label { text: app.currentTrack.artist || ""; Layout.fillWidth: true; wrapMode: Text.Wrap; opacity: .65; textFormat: Text.PlainText }
            Label { text: app.currentTrack.album || ""; visible: !!text; Layout.fillWidth: true; wrapMode: Text.Wrap; opacity: .65; textFormat: Text.PlainText }
            Label { text: app.playerStatus || app.actualQuality; visible: !!text; Layout.fillWidth: true; wrapMode: Text.Wrap; opacity: .7 }
            Item { Layout.fillHeight: true }
            Flow {
                Layout.fillWidth:true; spacing:8
                ActionButton { text: "Lyrics"; onClicked: panel.app.panel = "Lyrics" }
                ActionButton { text: "Queue"; onClicked: panel.app.panel = "Queue" }
                ActionButton { text: "Comments"; enabled: !!Models.commentThread(panel.app.currentTrack); onClicked: Actions.comments(panel.app.currentTrack, false) }
            }
        }
        ListView {
            id: queueList; visible:app.panel==="Queue"; Layout.fillWidth:true; Layout.fillHeight:true; clip:true
            model:app.queue; spacing:4; ScrollBar.vertical:ScrollBar{}
            delegate:Rectangle {
                required property var modelData; required property int index
                width:queueList.width; height:Math.max(82,panel.app.font.pixelSize*5.4)
                color:modelData.entryId===panel.app.currentTrack.entryId?Backend.theme.selection:"transparent"
                ColumnLayout {
                    anchors.fill:parent; anchors.margins:6; spacing:2
                    ActionButton { text:modelData.name+" · "+modelData.artist; quiet:true; Layout.fillWidth:true; onClicked:Actions.queuePlay(index) }
                    RowLayout {
                        Layout.alignment:Qt.AlignRight
                        ActionButton { text:"↑"; Accessible.name:"Move up"; quiet:true; enabled:index>0; onClicked:Actions.moveQueue(index,index-1) }
                        ActionButton { text:"↓"; Accessible.name:"Move down"; quiet:true; enabled:index+1<panel.app.queue.length; onClicked:Actions.moveQueue(index,index+1) }
                        ActionButton { text:"Remove"; quiet:true; onClicked:Actions.removeQueue(index) }
                    }
                }
            }
            Label { anchors.centerIn:parent; visible:!panel.app.queue.length; text:"Your queue is empty" }
        }
        ListView {
            id: lyricsList; visible:app.panel==="Lyrics"; Layout.fillWidth:true; Layout.fillHeight:true; clip:true
            model:app.lyrics; spacing:12; ScrollBar.vertical:ScrollBar{}
            property int activeLine:Models.lyricIndex(panel.app.lyrics,Backend.position+panel.app.lyricOffset)
            onActiveLineChanged:if(!moving&&activeLine>=0)positionViewAtIndex(activeLine,ListView.Center)
            delegate:ItemDelegate {
                required property var modelData; required property int index
                width:lyricsList.width; implicitHeight:lyricColumn.implicitHeight+24
                contentItem:ColumnLayout {
                    id:lyricColumn; spacing:8
                    Label { text:modelData.text||"♪"; font.pixelSize:18; Layout.fillWidth:true; wrapMode:Text.Wrap; color:index===lyricsList.activeLine?Backend.theme.accent:Backend.theme.foreground }
                    Label { visible:panel.app.translation&&!!modelData.translation; text:modelData.translation||""; wrapMode:Text.Wrap; Layout.fillWidth:true; opacity:.8 }
                }
                onClicked:Backend.seek(Math.max(0,modelData.time-panel.app.lyricOffset))
            }
            Label { anchors.centerIn:parent; visible:!panel.app.lyrics.length; text:"No synced lyrics available" }
        }
        ListView {
            id: commentsList; visible:app.panel==="Comments"; Layout.fillWidth:true; Layout.fillHeight:true; clip:true
            model:app.comments; spacing:12; ScrollBar.vertical:ScrollBar{}
            delegate:Rectangle {
                required property var modelData
                width:commentsList.width; height:commentContent.implicitHeight+24; color:Backend.theme.dark_background
                ColumnLayout {
                    id:commentContent; anchors.left:parent.left; anchors.right:parent.right; anchors.top:parent.top; anchors.margins:12
                    Label { text:(modelData.user||{}).nickname||""; Layout.fillWidth:true; elide:Text.ElideRight; color:Backend.theme.accent }
                    Label { text:modelData.content||""; Layout.fillWidth:true; wrapMode:Text.Wrap; textFormat:Text.PlainText }
                    Label { visible:!!(modelData.beReplied||[]).length; text:(modelData.beReplied||[]).length?"Reply · "+(modelData.beReplied[0].content||""):""; Layout.fillWidth:true; wrapMode:Text.Wrap; opacity:.7; textFormat:Text.PlainText }
                    ActionButton { text:(modelData.liked?"Liked · ":"Like · ")+(modelData.likedCount||0); quiet:true; enabled:!panel.app.writeBusy; onClicked:Actions.likeComment(modelData) }
                }
            }
            footer:ActionButton { text:"More comments"; width:commentsList.width; visible:panel.app.commentMore; onClicked:{panel.app.commentPage++;Actions.comments(panel.app.contextTrack,true)} }
            Label { anchors.centerIn:parent; visible:!panel.app.comments.length&&!panel.app.panelError; text:"No comments yet" }
        }
    }
}
