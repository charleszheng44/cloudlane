import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Yunjian 1.0
import "Actions.js" as Actions
import "Models.js" as Models
ApplicationWindow {
    id: app
    width:1100; height:760; minimumWidth:640; minimumHeight:480
    visible:true; title:"云间"; color:Backend.theme.background
    font.family:"monospace"; font.pixelSize:Backend.fontSize
    palette.window:Backend.theme.background; palette.windowText:Backend.theme.foreground
    palette.base:Backend.theme.dark_background; palette.text:Backend.theme.foreground
    palette.button:Backend.theme.background; palette.buttonText:Backend.theme.foreground
    palette.highlight:Backend.theme.accent; palette.highlightedText:Backend.theme.background
    property var backend:Backend
    property string nav:"首页"
    property string page:"首页"
    property string category:""
    property var profile:({})
    property var items:[]
    property var resource:({})
    property string viewKind:"cards"
    property int viewGeneration:0
    property string loading:""
    property string error:""
    property bool canBack:false
    property int offset:0
    property bool more:false
    property real scrollPosition: viewKind==="songs"?trackList.contentY:cardGrid.contentY
    property real restoreScroll:0
    onRestoreScrollChanged:Qt.callLater(function(){trackList.contentY=restoreScroll;cardGrid.contentY=restoreScroll})
    property string query:""
    property int searchType:0
    property string qr:""
    property string qrKey:""
    property string loginStatus:""
    property bool loginPolling:false
    property var currentTrack:({})
    property var queue:[]
    property int queueIndex:-1
    property int playbackGeneration:0
    property string playerStatus:""
    property string actualQuality:""
    property string quality:Backend.state("quality")||"standard"
    property int repeatMode:0
    property bool fm:false
    property real resumeAt:0
    property real podcastSpeed:1
    property bool videoVisible:false
    property var likedIds:[]
    property var localTracks:[]
    property var lyrics:[]
    property bool translation:true
    property real lyricOffset:0
    property string panel:""
    property string panelError:""
    property var contextTrack:({})
    property var comments:[]
    property int commentPage:1
    property string commentCursor:""
    property int commentSort:99
    property int commentGeneration:0
    property bool commentMore:false
    property bool writeBusy:false
    property string editError:""
    property var menuTrack:({})
    property var ownedPlaylists:[]
    property bool editingPlaylist:false
    property int sleepMinutes:0
    property int sleepRemaining:0
    property var categories:nav==="发现"?["精选歌单","排行榜","新歌","新碟","播客","MV"]:
        nav==="我的音乐"?["歌单","喜欢的音乐","专辑","歌手","播客","收藏视频","最近播放","已购专辑","音乐云盘","下载","本地音乐"]:
        nav==="动态"?["关注动态","通知","私信"]:[]
    function login(){openLogin()}
    function openLogin(){loginPopup.open();Actions.login()}
    function closeLogin(){loginPopup.close()}
    function closePlaylistEditor(){playlistEditor.close()}
    function openAddDialog(){editError="";addDialog.open()}
    function closeAddDialog(){addDialog.close()}
    function formatTime(seconds){var n=Math.max(0,Math.floor(seconds||0));return Math.floor(n/60)+":"+String(n%60).padStart(2,"0")}
    function showTrackMenu(track){menuTrack=track;trackMenu.popup()}
    function playlistEditorOpen(edit){editingPlaylist=edit;playlistName.text=edit?(resource.name||""):"";editError="";playlistEditor.open()}
    onPanelChanged:{if(panel)contextDrawer.open();else contextDrawer.close()}
    Connections {
        target:Backend
        function onResponse(id,data,error){Actions.response(id,data,error)}
        function onMessage(text){app.error=text}
        function onPlaybackEnded(){Actions.saveProgress();Actions.next(true,false)}
        function onMediaLoaded(){Actions.loaded()}
        function onLocalReady(tracks){Actions.localReady(tracks)}
        function onDesktopAction(action){Actions.desktopAction(action)}
    }
    Timer { interval:2500; running:app.loginPolling; repeat:true; onTriggered:Actions.pollLogin() }
    Timer { interval:10000; running:true; repeat:true; onTriggered:Actions.saveProgress() }
    Timer { interval:60000; running:app.sleepRemaining>0; repeat:true; onTriggered:{app.sleepRemaining--;if(!app.sleepRemaining)Backend.setPaused(true)} }
    ColumnLayout {
        anchors.fill:parent; spacing:0
        RowLayout {
            Layout.fillWidth:true; Layout.fillHeight:true; spacing:0
            Rectangle {
                Layout.preferredWidth:app.width<840?66:164; Layout.fillHeight:true; color:Backend.theme.dark_background
                ColumnLayout {
                    anchors.fill:parent; anchors.margins:12; spacing:6
                    Label { text:"云间"; font.pixelSize:24; color:Backend.theme.accent; Layout.topMargin:14; Layout.bottomMargin:24 }
                    Repeater {
                        model:["首页","发现","我的音乐","动态"]
                        ActionButton {
                            required property string modelData
                            text:app.width<840?modelData.slice(0,2):modelData
                            selected:app.nav===modelData; quiet:true; Layout.fillWidth:true
                            onClicked:Actions.navigate(modelData)
                            ToolTip.visible:hovered&&app.width<840; ToolTip.text:modelData
                        }
                    }
                    Item { Layout.fillHeight:true }
                    Label { visible:app.width>=840; text:"音乐，留在此刻。"; font.pixelSize:11; opacity:.6; Layout.bottomMargin:8 }
                    ActionButton { text:app.width<840?"设置":"偏好设置"; quiet:true; Layout.fillWidth:true; onClicked:settingsPopup.open() }
                    Label { text:"PREVIEW"; font.pixelSize:9; opacity:.5; Layout.alignment:Qt.AlignHCenter; Layout.bottomMargin:8 }
                }
            }
            ColumnLayout {
                Layout.fillWidth:true; Layout.fillHeight:true; Layout.margins:app.width<840?16:24; spacing:14
                RowLayout {
                    Layout.fillWidth:true; spacing:8
                    ActionButton { text:"‹"; Accessible.name:"返回"; enabled:app.canBack; quiet:true; onClicked:Actions.back() }
                    TextField {
                        id:searchBox; Layout.fillWidth:true; placeholderText:"搜索歌曲、专辑、歌手…"; selectByMouse:true
                        onAccepted:Actions.search(text,app.searchType)
                        background:Rectangle { color:Backend.theme.dark_background; border.color:searchBox.activeFocus?Backend.theme.accent:"#737c9d"; border.width:1 }
                    }
                    ActionButton { text:app.profile.nickname||"登录"; Layout.maximumWidth:150; quiet:true; onClicked:app.openLogin() }
                }
                RowLayout {
                    Layout.fillWidth:true
                    Label { text:app.page; font.pixelSize:24; Layout.fillWidth:true; elide:Text.ElideRight }
                    ComboBox {
                        id:categoryBox; visible:app.categories.length>0 && app.page===app.nav
                        model:app.categories; currentIndex:Math.max(0,app.categories.indexOf(app.category))
                        Layout.maximumWidth:app.width<840?138:180
                        onActivated:Actions.navigate(app.nav,currentText)
                    }
                    ActionButton { text:"刷新"; quiet:true; enabled:!app.loading; onClicked:Actions.refresh() }
                }
                RowLayout {
                    visible:app.page==="首页"; Layout.fillWidth:true; spacing:8
                    ActionButton { text:"每日推荐"; onClicked:Actions.daily() }
                    ActionButton { text:app.fm?"返回队列":"私人 FM"; selected:app.fm; onClicked:Actions.fm() }
                    Label { text:"为你推荐"; visible:app.width>950; Layout.fillWidth:true; horizontalAlignment:Text.AlignRight; opacity:.7 }
                }
                RowLayout {
                    visible:app.page.indexOf("搜索 · ")===0; Layout.fillWidth:true
                    Label { text:"类型" }
                    ComboBox { model:Actions.searchTypes.map(function(t){return t.name}); currentIndex:app.searchType; onActivated:Actions.search(app.query,currentIndex) }
                }
                RowLayout {
                    visible:app.viewKind==="songs"||app.nav==="我的音乐"; Layout.fillWidth:true
                    ActionButton { visible:app.viewKind==="songs"; text:"播放全部"; enabled:app.items.length>0; onClicked:Actions.playAll() }
                    ActionButton { visible:app.category==="歌单"&&app.page==="我的音乐"; text:"新建歌单"; enabled:!!app.profile.userId; onClicked:app.playlistEditorOpen(false) }
                    ActionButton { visible:app.category==="本地音乐"; text:"添加文件"; onClicked:localDialog.open() }
                    ActionButton { visible:app.resource.kind==="artist"; text:"专辑"; onClicked:Actions.artistAlbums() }
                    ActionButton { visible:["album","artist","radio"].indexOf(app.resource.kind)>=0; text:"收藏"; enabled:!app.writeBusy; onClicked:Actions.collect(app.resource,true) }
                    ActionButton { visible:app.resource.kind==="playlist"&&app.resource.creatorId===String(app.profile.userId); text:"编辑"; onClicked:resourceMenu.popup() }
                    Item { Layout.fillWidth:true }
                    Label { text:app.items.length+" 项"; opacity:.65; visible:app.items.length>0 }
                }
                Label { visible:!!app.resource.description; text:app.resource.description||""; maximumLineCount:2; elide:Text.ElideRight; wrapMode:Text.Wrap; Layout.fillWidth:true; opacity:.7; textFormat:Text.PlainText }
                Label { text:app.loading||app.error; visible:!!text; Layout.fillWidth:true; maximumLineCount:3; elide:Text.ElideRight; wrapMode:Text.Wrap; color:app.error&&!app.loading?Backend.theme.red:Backend.theme.foreground; textFormat:Text.PlainText }
                Rectangle {
                    Layout.fillWidth:true; Layout.preferredHeight:app.videoVisible?Math.min(320,app.height*.40):1
                    color:"black"
                    VideoSurface { anchors.fill:parent; backend:Backend }
                    ActionButton { visible:app.videoVisible; anchors.top:parent.top; anchors.right:parent.right; text:"收起"; onClicked:app.videoVisible=false }
                }
                GridView {
                    id:cardGrid; visible:app.viewKind==="cards"; Layout.fillWidth:true; Layout.fillHeight:true; clip:true
                    property int columns:Math.max(2,Math.floor(width/174))
                    cellWidth:width/columns; cellHeight:cellWidth+76; model:app.items
                    ScrollBar.vertical:ScrollBar{}
                    delegate:ItemDelegate {
                        id:card
                        required property var modelData
                        width:cardGrid.cellWidth-16; height:cardGrid.cellHeight-16; padding:0
                        background:Rectangle { color:card.hovered?Backend.theme.selection:"transparent"; border.width:card.activeFocus?1:0; border.color:Backend.theme.accent }
                        contentItem:ColumnLayout {
                            spacing:8
                            Rectangle {
                                Layout.fillWidth:true; Layout.preferredHeight:card.width; color:Backend.theme.dark_background
                                Label { text:"♪"; anchors.centerIn:parent; font.pixelSize:32; opacity:.35 }
                                Image { anchors.fill:parent; source:card.modelData.cover; sourceSize.width:320; sourceSize.height:320; fillMode:Image.PreserveAspectCrop; asynchronous:true }
                            }
                            Label { text:card.modelData.name; Layout.fillWidth:true; elide:Text.ElideRight }
                            Label { text:card.modelData.artist||""; Layout.fillWidth:true; elide:Text.ElideRight; opacity:.65; font.pixelSize:Math.max(11,app.font.pixelSize-2) }
                            Item { Layout.fillHeight:true }
                        }
                        onClicked:Actions.open(modelData)
                        Accessible.name:modelData.name
                    }
                }
                ListView {
                    id:trackList; visible:app.viewKind==="songs"; Layout.fillWidth:true; Layout.fillHeight:true; clip:true; model:app.items
                    ScrollBar.vertical:ScrollBar{}
                    delegate:ItemDelegate {
                        id:trackRow
                        required property var modelData; required property int index
                        width:trackList.width; height:Math.max(54,app.font.pixelSize*3.8)
                        background:Rectangle { color:trackRow.hovered?Backend.theme.selection:"transparent"; border.width:trackRow.activeFocus?1:0; border.color:Backend.theme.accent }
                        contentItem:RowLayout {
                            spacing:12
                            Label { text:trackRow.modelData.id===app.currentTrack.id&&Backend.playing?"▶":trackRow.index+1; Layout.preferredWidth:30; opacity:.65 }
                            ColumnLayout {
                                Layout.fillWidth:true; spacing:3
                                Label { text:trackRow.modelData.name; Layout.fillWidth:true; elide:Text.ElideRight; color:trackRow.modelData.id===app.currentTrack.id?Backend.theme.accent:Backend.theme.foreground }
                                Label { text:trackRow.modelData.artist||""; Layout.fillWidth:true; elide:Text.ElideRight; opacity:.7; font.pixelSize:Math.max(11,app.font.pixelSize-2) }
                            }
                            Label { text:trackRow.modelData.album||""; visible:app.width>1000; Layout.preferredWidth:170; elide:Text.ElideRight; opacity:.7 }
                            Label { text:trackRow.modelData.duration?app.formatTime(trackRow.modelData.duration/1000):""; opacity:.65 }
                            ActionButton { text:"···"; Accessible.name:"歌曲操作："+trackRow.modelData.name; quiet:true; onClicked:app.showTrackMenu(trackRow.modelData) }
                        }
                        onClicked:Actions.enqueue(modelData,true)
                        Accessible.name:modelData.name+" · "+modelData.artist
                    }
                }
                ListView {
                    id:feedList; visible:app.viewKind==="feed"; Layout.fillWidth:true; Layout.fillHeight:true; clip:true; model:app.items; spacing:12
                    ScrollBar.vertical:ScrollBar{}
                    delegate:ItemDelegate {
                        required property var modelData
                        width:feedList.width; implicitHeight:feedTextColumn.implicitHeight+24
                        background:Rectangle { color:Backend.theme.dark_background }
                        contentItem:ColumnLayout {
                            id:feedTextColumn; spacing:8
                            Label { text:((modelData.user||modelData.fromUser||{}).nickname)||"网易云音乐"; color:Backend.theme.accent; Layout.fillWidth:true; elide:Text.ElideRight }
                            Label { text:Actions.feedText(modelData); wrapMode:Text.Wrap; Layout.fillWidth:true; textFormat:Text.PlainText }
                        }
                        onClicked:if(app.category==="私信"&&app.page===app.nav)Actions.conversation(modelData)
                    }
                }
                Item {
                    visible:app.viewKind==="downloads"; Layout.fillWidth:true; Layout.fillHeight:true
                    Label { anchors.centerIn:parent; text:"下载管理正在接入，已下载文件可从本地音乐打开"; width:parent.width; wrapMode:Text.Wrap; horizontalAlignment:Text.AlignHCenter; opacity:.7 }
                }
                ActionButton { text:"加载更多"; visible:app.more; enabled:!app.loading; Layout.alignment:Qt.AlignHCenter; onClicked:Actions.more() }
            }
        }
        Rectangle {
            Layout.fillWidth:true; Layout.preferredHeight:Math.max(98,app.font.pixelSize*6.8); color:Backend.theme.dark_background
            RowLayout {
                anchors.fill:parent; anchors.margins:12; spacing:app.width<800?8:16
                ColumnLayout {
                    Layout.preferredWidth:Math.min(230,app.width*.23); spacing:6
                    Label { text:app.currentTrack.name||"选择一首音乐"; Layout.fillWidth:true; elide:Text.ElideRight }
                    Label { text:app.currentTrack.artist||""; Layout.fillWidth:true; elide:Text.ElideRight; opacity:.7; font.pixelSize:Math.max(11,app.font.pixelSize-2) }
                    Label { text:app.playerStatus||app.actualQuality; visible:!!text; Layout.fillWidth:true; elide:Text.ElideRight; color:app.playerStatus?Backend.theme.accent:Backend.theme.foreground; font.pixelSize:11; ToolTip.text:text; ToolTip.visible:playerHover.hovered; HoverHandler{id:playerHover} }
                }
                ActionButton { visible:app.width>880; text:app.likedIds.indexOf(app.currentTrack.id)>=0?"♥":"♡"; Accessible.name:"喜欢当前歌曲"; quiet:true; enabled:app.currentTrack.kind==="song"&&!app.writeBusy; onClicked:Actions.like(app.currentTrack) }
                ColumnLayout {
                    Layout.fillWidth:true; spacing:4
                    RowLayout {
                        Layout.alignment:Qt.AlignHCenter; spacing:6
                        ActionButton { text:"‹"; Accessible.name:"上一首"; quiet:true; enabled:app.queueIndex>0; onClicked:Actions.previous(false) }
                        ActionButton { text:Backend.playing?"暂停":"播放"; enabled:!!app.currentTrack.id; onClicked:Actions.toggle() }
                        ActionButton { text:"›"; Accessible.name:"下一首"; quiet:true; enabled:app.queue.length>0; onClicked:Actions.next(false,false) }
                        ActionButton { visible:app.width>900; text:["顺序","循环","单曲"][app.repeatMode]; quiet:true; onClicked:{app.repeatMode=(app.repeatMode+1)%3;Actions.updateMetadata()} }
                    }
                    RowLayout {
                        Layout.fillWidth:true; spacing:6
                        Label { text:app.formatTime(Backend.position); font.pixelSize:11; opacity:.7 }
                        Slider { Layout.fillWidth:true; Layout.minimumWidth:40; from:0; to:Math.max(1,Backend.duration); value:Backend.position; enabled:Backend.loaded; onMoved:Backend.seek(value); Accessible.name:"播放进度" }
                        Label { text:app.formatTime(Backend.duration); font.pixelSize:11; opacity:.7 }
                    }
                }
                ActionButton { text:"歌词"; quiet:true; visible:app.width>760; selected:app.panel==="歌词"; onClicked:app.panel=app.panel==="歌词"?"":"歌词" }
                ActionButton { text:"队列"; quiet:true; selected:app.panel==="队列"; onClicked:app.panel=app.panel==="队列"?"":"队列" }
                ActionButton { text:"···"; Accessible.name:"播放器选项"; quiet:true; onClicked:playerOptions.open() }
            }
        }
    }
    Drawer {
        id:contextDrawer; edge:Qt.RightEdge; width:Math.min(400,app.width-32); height:app.height
        modal:true; focus:true; closePolicy:Popup.CloseOnEscape|Popup.CloseOnPressOutside
        onClosed:app.panel=""
        contentItem:ContextPanel { app:app }
    }
    Popup {
        id:loginPopup; anchors.centerIn:parent; width:360; modal:true; focus:true; padding:24
        onClosed:Actions.cancelLogin()
        background:Rectangle { color:Backend.theme.background; border.color:Backend.theme.accent }
        contentItem:ColumnLayout {
            spacing:16
            Label { text:app.profile.userId?app.profile.nickname:"扫码登录"; font.pixelSize:22; Layout.fillWidth:true; elide:Text.ElideRight }
            Image { source:app.qr; Layout.preferredWidth:248; Layout.preferredHeight:248; Layout.alignment:Qt.AlignHCenter; fillMode:Image.PreserveAspectFit; smooth:false }
            Label { text:app.loginStatus; Layout.fillWidth:true; wrapMode:Text.Wrap }
            RowLayout {
                ActionButton { text:"刷新"; onClicked:Actions.login() }
                ActionButton { text:"取消"; onClicked:loginPopup.close() }
                ActionButton { text:"退出账户"; visible:!!app.profile.userId; onClicked:Actions.logout() }
            }
        }
    }
    Popup {
        id:playerOptions; anchors.centerIn:parent; width:340; modal:true; focus:true; padding:20
        background:Rectangle { color:Backend.theme.background; border.color:Backend.theme.accent }
        contentItem:ColumnLayout {
            spacing:12
            Label { text:"播放选项"; font.pixelSize:20 }
            RowLayout { Label { text:"音量" } Slider { Layout.fillWidth:true; from:0; to:100; value:Backend.volume; onMoved:Backend.setVolume(value); Accessible.name:"音量" } Label { text:Math.round(Backend.volume)+"%" } }
            ComboBox { model:["顺序播放","列表循环","单曲循环"]; currentIndex:app.repeatMode; Layout.fillWidth:true; onActivated:{app.repeatMode=currentIndex;Actions.updateMetadata()} }
            RowLayout {
                ActionButton { text:"歌词"; onClicked:{playerOptions.close();app.panel="歌词"} }
                ActionButton { text:"评论"; enabled:!!Models.commentThread(app.currentTrack); onClicked:{playerOptions.close();Actions.comments(app.currentTrack,false)} }
                ActionButton { text:app.likedIds.indexOf(app.currentTrack.id)>=0?"已喜欢":"喜欢"; enabled:app.currentTrack.kind==="song"&&!app.writeBusy; onClicked:Actions.like(app.currentTrack) }
            }
            RowLayout {
                Label { text:"播客速度" }
                ComboBox { model:["0.75×","1×","1.25×","1.5×","2×"]; currentIndex:1; Layout.fillWidth:true; onActivated:{app.podcastSpeed=[.75,1,1.25,1.5,2][currentIndex];if(app.currentTrack.kind==="episode")Backend.setSpeed(app.podcastSpeed)} }
            }
            RowLayout {
                Label { text:"睡眠定时" }
                ComboBox { model:["关闭","15 分钟","30 分钟","60 分钟"]; Layout.fillWidth:true; onActivated:app.sleepRemaining=[0,15,30,60][currentIndex] }
            }
            Label { visible:app.sleepRemaining>0; text:app.sleepRemaining+" 分钟后暂停"; opacity:.7 }
            ActionButton { text:"关闭"; Layout.alignment:Qt.AlignRight; onClicked:playerOptions.close() }
        }
    }
    Popup {
        id:settingsPopup; anchors.centerIn:parent; width:Math.min(440,app.width-40); modal:true; focus:true; padding:24
        background:Rectangle { color:Backend.theme.background; border.color:Backend.theme.accent }
        contentItem:ColumnLayout {
            spacing:16
            Label { text:"偏好设置"; font.pixelSize:22 }
            Label { text:"外观跟随 Omarchy 当前主题、系统等宽字体和字号。"; Layout.fillWidth:true; wrapMode:Text.Wrap; opacity:.8 }
            Label { text:"播放音质" }
            ComboBox { model:["标准","极高","无损","Hi-Res"]; currentIndex:Math.max(0,["standard","exhigh","lossless","hires"].indexOf(app.quality)); Layout.fillWidth:true; onActivated:{app.quality=["standard","exhigh","lossless","hires"][currentIndex];Backend.setState("quality",app.quality)} }
            Label { text:"实际音质以服务返回结果为准；需要对应歌曲权限。"; wrapMode:Text.Wrap; Layout.fillWidth:true; opacity:.7 }
            ActionButton { text:"添加本地音乐"; onClicked:localDialog.open() }
            Label { text:"云间 · 开发预览版\n使用社区实现的消费端协议。"; wrapMode:Text.Wrap; Layout.fillWidth:true; opacity:.6; font.pixelSize:12 }
            ActionButton { text:"关闭"; Layout.alignment:Qt.AlignRight; onClicked:settingsPopup.close() }
        }
    }
    Menu {
        id:trackMenu
        MenuItem { text:"播放"; onTriggered:Actions.enqueue(app.menuTrack,true) }
        MenuItem { text:"下一首播放"; onTriggered:Actions.enqueue(app.menuTrack,false,true) }
        MenuItem { text:"加入队列"; onTriggered:Actions.enqueue(app.menuTrack,false,false) }
        MenuSeparator {}
        MenuItem { text:app.likedIds.indexOf(app.menuTrack.id)>=0?"取消喜欢":"喜欢"; enabled:app.menuTrack.kind==="song"&&!app.writeBusy; onTriggered:Actions.like(app.menuTrack) }
        MenuItem { text:"添加到歌单…"; enabled:app.menuTrack.kind==="song"; onTriggered:Actions.ownedPlaylists(app.menuTrack) }
        MenuItem { text:"打开专辑"; enabled:!!app.menuTrack.albumId; onTriggered:Actions.open({id:app.menuTrack.albumId,name:app.menuTrack.album,kind:"album"}) }
        MenuItem { text:"打开歌手"; enabled:!!app.menuTrack.artistId; onTriggered:Actions.open({id:app.menuTrack.artistId,name:app.menuTrack.artist,kind:"artist"}) }
        MenuItem { text:"播放 MV"; enabled:!!app.menuTrack.mv; onTriggered:Actions.playVideo({id:String(app.menuTrack.mv),name:app.menuTrack.name,kind:"mv",artist:app.menuTrack.artist}) }
        MenuItem { text:"评论"; enabled:!!Models.commentThread(app.menuTrack); onTriggered:Actions.comments(app.menuTrack,false) }
        MenuItem { text:"复制链接"; enabled:app.menuTrack.kind!=="local"; onTriggered:Actions.copyLink(app.menuTrack) }
    }
    Menu {
        id:resourceMenu
        MenuItem { text:"重命名"; onTriggered:app.playlistEditorOpen(true) }
        MenuItem { text:"复制链接"; onTriggered:Actions.copyLink(app.resource) }
        MenuItem { text:"删除歌单…"; onTriggered:deleteDialog.open() }
    }
    Popup {
        id:playlistEditor; anchors.centerIn:parent; width:360; modal:true; focus:true; padding:24
        background:Rectangle { color:Backend.theme.background; border.color:Backend.theme.accent }
        contentItem:ColumnLayout {
            spacing:16
            Label { text:app.editingPlaylist?"重命名歌单":"新建歌单"; font.pixelSize:22 }
            TextField { id:playlistName; placeholderText:"歌单名称"; Layout.fillWidth:true; maximumLength:120; selectByMouse:true }
            CheckBox { id:privatePlaylist; text:"私密歌单"; checked:true; visible:!app.editingPlaylist }
            Label { text:app.editError; visible:!!text; wrapMode:Text.Wrap; Layout.fillWidth:true; color:Backend.theme.red }
            RowLayout {
                ActionButton { text:"取消"; enabled:!app.writeBusy; onClicked:playlistEditor.close() }
                ActionButton { text:app.writeBusy?"保存中…":"保存"; enabled:!app.writeBusy&&!!playlistName.text.trim(); onClicked:app.editingPlaylist?Actions.renamePlaylist(app.resource,playlistName.text):Actions.createPlaylist(playlistName.text,privatePlaylist.checked) }
            }
        }
    }
    Popup {
        id:addDialog; anchors.centerIn:parent; width:360; height:Math.min(460,app.height-40); modal:true; focus:true; padding:20
        background:Rectangle { color:Backend.theme.background; border.color:Backend.theme.accent }
        contentItem:ColumnLayout {
            spacing:12
            Label { text:"添加到歌单"; font.pixelSize:22 }
            Label { text:app.menuTrack.name||""; Layout.fillWidth:true; elide:Text.ElideRight; opacity:.7 }
            Label { text:app.editError; visible:!!text; Layout.fillWidth:true; wrapMode:Text.Wrap; color:Backend.theme.red }
            ListView {
                id:ownedList; model:app.ownedPlaylists; Layout.fillWidth:true; Layout.fillHeight:true; clip:true
                delegate:ActionButton { required property var modelData; width:ownedList.width; text:modelData.name; quiet:true; enabled:!app.writeBusy; onClicked:Actions.addTrack(modelData,app.menuTrack,false) }
                Label { visible:!app.ownedPlaylists.length; anchors.centerIn:parent; text:"暂无可编辑歌单" }
            }
            ActionButton { text:"取消"; onClicked:addDialog.close() }
        }
    }
    Dialog {
        id:deleteDialog; anchors.centerIn:parent; title:"删除歌单"; modal:true
        standardButtons:Dialog.Cancel|Dialog.Ok
        Label { text:"删除「"+(app.resource.name||"")+"」？此操作无法撤销。"; width:320; wrapMode:Text.Wrap }
        onAccepted:Actions.deletePlaylist(app.resource)
    }
    FileDialog {
        id:localDialog; title:"添加本地音乐"; fileMode:FileDialog.OpenFiles
        nameFilters:["音乐文件 (*.mp3 *.flac *.ogg *.opus *.m4a *.wav *.aac)","所有文件 (*)"]
        onAccepted:{app.loading="正在读取音乐标签…";Backend.scanLocal(selectedFiles)}
    }
    Shortcut { sequence:"Ctrl+K"; onActivated:searchBox.forceActiveFocus() }
    Shortcut { sequence:"Alt+Left"; onActivated:Actions.back() }
    Shortcut { sequence:"Ctrl+Right"; onActivated:Actions.next(false,false) }
    Shortcut { sequence:"Ctrl+Left"; onActivated:Actions.previous(false) }
    Shortcut { sequence:"Ctrl+Space"; onActivated:Actions.toggle() }
    Shortcut { sequence:"Ctrl+Q"; onActivated:Qt.quit() }
    Shortcut { sequence:"Ctrl+L"; onActivated:app.panel=app.panel==="歌词"?"":"歌词" }
    Shortcut { sequence:"Ctrl+O"; onActivated:localDialog.open() }
    onClosing:Actions.saveProgress()
    Component.onCompleted:Actions.init(app)
}
