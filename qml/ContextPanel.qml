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
            ActionButton { text: "关闭"; quiet: true; onClicked: app.panel="" }
        }
        Label {
            visible: app.panel === "评论"
            text: app.contextTrack.name || ""; Layout.fillWidth: true; elide: Text.ElideRight
            color: Backend.theme.accent
        }
        RowLayout {
            visible: app.panel === "队列"; Layout.fillWidth: true
            Label { text: app.queue.length + " 首"; Layout.fillWidth: true }
            ActionButton { text: "打乱后续"; enabled: !app.fm && app.queue.length>1; onClicked: Actions.shuffle() }
        }
        RowLayout {
            visible: app.panel === "歌词"; Layout.fillWidth: true
            CheckBox { text:"翻译"; checked:app.translation; onToggled:app.translation=checked }
            Label { text:"偏移" }
            SpinBox { from:-100; to:100; value:app.lyricOffset*10; onValueModified:app.lyricOffset=value/10; Layout.fillWidth:true }
            Label { text:"×0.1秒"; font.pixelSize:11 }
        }
        ComboBox {
            visible: app.panel === "评论"; model:["推荐评论","最新评论"]
            onActivated:{app.commentSort=currentIndex===0?99:3;Actions.comments(app.contextTrack,false)}
        }
        Label { text:app.panelError; visible:app.panel==="评论"&&text.length>0; wrapMode:Text.Wrap; Layout.fillWidth:true; color:Backend.theme.red }
        ListView {
            id: queueList; visible:app.panel==="队列"; Layout.fillWidth:true; Layout.fillHeight:true; clip:true
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
                        ActionButton { text:"上移"; quiet:true; enabled:index>0; onClicked:Actions.moveQueue(index,index-1) }
                        ActionButton { text:"下移"; quiet:true; enabled:index+1<panel.app.queue.length; onClicked:Actions.moveQueue(index,index+1) }
                        ActionButton { text:"移除"; quiet:true; onClicked:Actions.removeQueue(index) }
                    }
                }
            }
            Label { anchors.centerIn:parent; visible:!panel.app.queue.length; text:"队列为空" }
        }
        ListView {
            id: lyricsList; visible:app.panel==="歌词"; Layout.fillWidth:true; Layout.fillHeight:true; clip:true
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
            Label { anchors.centerIn:parent; visible:!panel.app.lyrics.length; text:"暂无同步歌词" }
        }
        ListView {
            id: commentsList; visible:app.panel==="评论"; Layout.fillWidth:true; Layout.fillHeight:true; clip:true
            model:app.comments; spacing:12; ScrollBar.vertical:ScrollBar{}
            delegate:Rectangle {
                required property var modelData
                width:commentsList.width; height:commentContent.implicitHeight+24; color:Backend.theme.dark_background
                ColumnLayout {
                    id:commentContent; anchors.left:parent.left; anchors.right:parent.right; anchors.top:parent.top; anchors.margins:12
                    Label { text:(modelData.user||{}).nickname||""; Layout.fillWidth:true; elide:Text.ElideRight; color:Backend.theme.accent }
                    Label { text:modelData.content||""; Layout.fillWidth:true; wrapMode:Text.Wrap; textFormat:Text.PlainText }
                    Label { visible:!!(modelData.beReplied||[]).length; text:(modelData.beReplied||[]).length?"回复 · "+(modelData.beReplied[0].content||""):""; Layout.fillWidth:true; wrapMode:Text.Wrap; opacity:.7; textFormat:Text.PlainText }
                    ActionButton { text:(modelData.liked?"已赞 ":"赞 ")+(modelData.likedCount||0); quiet:true; enabled:!panel.app.writeBusy; onClicked:Actions.likeComment(modelData) }
                }
            }
            footer:ActionButton { text:"更多评论"; width:commentsList.width; visible:panel.app.commentMore; onClicked:{panel.app.commentPage++;Actions.comments(panel.app.contextTrack,true)} }
            Label { anchors.centerIn:parent; visible:!panel.app.comments.length&&!panel.app.panelError; text:"暂无评论" }
        }
    }
}
