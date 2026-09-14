import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Yunjian 1.0
import "Actions.js" as Actions
import "Models.js" as Models
ApplicationWindow {
    id: window
    width:1100; height:760; minimumWidth:640; minimumHeight:480
    visible:true; title:"云间"; color:Backend.theme.dark_background
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
    property var libraryPlaylists:[]
    property var items:[]
    property var resource:({})
    property string viewKind:"cards"
    property int viewGeneration:0
    property string loading:""
    property bool downloadPending:false
    property string staleText:""
    property string error:""
    property bool canBack:false
    property int offset:0
    property bool more:false
    property real scrollPosition: viewKind==="songs"?trackList.contentY:cardGrid.contentY
    property real restoreScroll:0
    onRestoreScrollChanged:Qt.callLater(function(){trackList.contentY=restoreScroll;cardGrid.contentY=restoreScroll})
    property string query:""
    property int searchType:0
    property string qr:Backend.loginQr
    property string loginStatus:Backend.loginStatus
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
    property bool videoSession:false
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
    function openLogin(){loginPopup.open();if(!window.profile.userId)Actions.login()}
    function closeLogin(){loginPopup.close()}
    function closePlaylistEditor(){playlistEditor.close()}
    function openAddDialog(){editError="";addDialog.open()}
    function closeAddDialog(){addDialog.close()}
    function formatTime(seconds){var n=Math.max(0,Math.floor(seconds||0));return Math.floor(n/60)+":"+String(n%60).padStart(2,"0")}
    function showTrackMenu(track){menuTrack=track;trackMenu.popup()}
    function playlistEditorOpen(edit){editingPlaylist=edit;playlistName.text=edit?(resource.name||""):"";editError="";playlistEditor.open()}
    readonly property bool dockPanel: width >= 1280
    function syncPanel(){if(panel && !dockPanel)contextDrawer.open();else contextDrawer.close()}
    onPanelChanged:syncPanel()
    onDockPanelChanged:Qt.callLater(syncPanel)
    Connections {
        target:Backend
        function onAccountReady(profile,afterLogin){Actions.acceptAccount(profile,afterLogin)}
        function onResponse(id,data,error){Actions.response(id,data,error)}
        function onMessage(text){window.error=text}
        function onPlaybackEnded(){if(window.videoSession)Actions.leaveVideo();else{Actions.saveProgress();Actions.next(true,false)}}
        function onMediaLoaded(){Actions.loaded()}
        function onLocalLoaded(tracks){window.localTracks=JSON.parse(JSON.stringify(tracks));if(window.category==="本地音乐"&&window.page===window.nav)window.items=window.localTracks}
        function onLocalReady(tracks){Actions.localReady(tracks)}
        function onOpenRequested(uri){Actions.handleLink(uri)}
        function onDesktopAction(action){Actions.desktopAction(action)}
    }
    Timer { interval:10000; running:true; repeat:true; onTriggered:Actions.saveProgress() }
    Timer { interval:60000; running:window.sleepRemaining>0; repeat:true; onTriggered:{window.sleepRemaining--;if(!window.sleepRemaining)Backend.setPaused(true)} }
    ColumnLayout {
        anchors.fill:parent; spacing:0
        RowLayout {
            Layout.fillWidth: true; Layout.preferredHeight: 60; Layout.leftMargin: 18; Layout.rightMargin: 18
            spacing: 12
            Label { text: "云间"; font.pixelSize: 22; font.weight: Font.DemiBold; color: Backend.theme.accent; Layout.preferredWidth: window.width < 840 ? 32 : 160 }
            PlayerButton { symbol: "back"; label: "返回"; enabled: window.canBack; onClicked: Actions.back() }
            PlayerButton { symbol: "home"; label: "首页"; selected: window.nav === "首页"; onClicked: Actions.navigate("首页") }
            TextField {
                id: searchBox; Layout.fillWidth: true; Layout.maximumWidth: 580; implicitHeight: 40
                placeholderText: "搜索歌曲、专辑、歌手…"; selectByMouse: true; leftPadding: 40
                onAccepted: Actions.search(text, window.searchType)
                background: Rectangle { radius: 20; color: Backend.theme.background; border.width: searchBox.activeFocus ? 1 : 0; border.color: Backend.theme.accent }
                PlayerIcon { x: 12; anchors.verticalCenter: parent.verticalCenter; width: 18; height: 18; name: "search"; opacity: .65 }
            }
            Item { Layout.fillWidth: true; visible: window.width > 1100 }
            ActionButton { text: window.profile.nickname || "登录"; Layout.maximumWidth: window.width < 840 ? 76 : 150; quiet: true; onClicked: window.openLogin() }
        }
        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.leftMargin: 8; Layout.rightMargin: 8; spacing: 8
            LibrarySidebar {
                Layout.preferredWidth: window.width < 840 ? 66 : window.width < 1000 ? 190 : 232
                Layout.fillHeight: true
                app: window
                onSettingsRequested: settingsPopup.open()
            }
            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true
                color: Backend.theme.background; radius: 8
                ColumnLayout {
                    anchors.fill: parent; anchors.margins: window.width < 840 ? 16 : 24; spacing: 14
                RowLayout {
                    Layout.fillWidth:true
                    Label { text:window.page==="首页"?"为你推荐":window.page; font.pixelSize:26; font.weight:Font.DemiBold; Layout.fillWidth:true; elide:Text.ElideRight }
                    ComboBox {
                        id:categoryBox; visible:window.categories.length>0 && window.page===window.nav
                        model:window.categories; currentIndex:Math.max(0,window.categories.indexOf(window.category))
                        Layout.maximumWidth:window.width<840?138:180
                        onActivated:Actions.navigate(window.nav,currentText)
                    }
                    ActionButton { text:"刷新"; quiet:true; enabled:!window.loading; onClicked:Actions.refresh() }
                }
                RowLayout {
                    visible:window.page==="首页"; Layout.fillWidth:true; spacing:8
                    ActionButton { text:"每日推荐"; onClicked:Actions.daily() }
                    ActionButton { text:window.fm?"返回队列":"私人 FM"; selected:window.fm; onClicked:Actions.fm() }
                }
                RowLayout {
                    visible:window.page.indexOf("搜索 · ")===0; Layout.fillWidth:true
                    Label { text:"类型" }
                    ComboBox { model:Actions.searchTypes.map(function(t){return t.name}); currentIndex:window.searchType; onActivated:Actions.search(window.query,currentIndex) }
                }
                RowLayout {
                    visible:window.viewKind==="songs"||window.nav==="我的音乐"; Layout.fillWidth:true
                    ActionButton { visible:window.viewKind==="songs"; text:"播放全部"; enabled:window.items.length>0; onClicked:Actions.playAll() }
                    ActionButton { visible:window.category==="歌单"&&window.page==="我的音乐"; text:"新建歌单"; enabled:!!window.profile.userId; onClicked:window.playlistEditorOpen(false) }
                    ActionButton { visible:window.category==="本地音乐"&&window.page===window.nav; text:"添加文件"; onClicked:localDialog.open() }
                    ActionButton { visible:window.resource.kind==="artist"; text:"专辑"; onClicked:Actions.artistAlbums() }
                    ActionButton { visible:["album","artist","radio"].indexOf(window.resource.kind)>=0; text:"收藏"; enabled:!window.writeBusy; onClicked:Actions.collect(window.resource,true) }
                    ActionButton { visible:window.resource.kind==="playlist"&&window.resource.creatorId===String(window.profile.userId); text:"编辑"; onClicked:resourceMenu.popup() }
                    Item { Layout.fillWidth:true }
                    Label { text:window.items.length+" 项"; opacity:.65; visible:window.items.length>0 }
                }
                Label { visible:!!window.resource.description; text:window.resource.description||""; maximumLineCount:2; elide:Text.ElideRight; wrapMode:Text.Wrap; Layout.fillWidth:true; opacity:.7; textFormat:Text.PlainText }
                Label { text:window.staleText; visible:!!text; Layout.fillWidth:true; elide:Text.ElideRight; opacity:.7 }
                Label { text:window.loading||window.error; visible:!!text; Layout.fillWidth:true; maximumLineCount:3; elide:Text.ElideRight; wrapMode:Text.Wrap; color:window.error&&!window.loading?Backend.theme.red:Backend.theme.foreground; textFormat:Text.PlainText }
                Rectangle {
                    Layout.fillWidth:true; Layout.preferredHeight:window.videoVisible?Math.min(320,window.height*.40):1
                    color:"black"
                    VideoSurface { anchors.fill:parent; backend:Backend }
                    ActionButton { visible:window.videoVisible; anchors.top:parent.top; anchors.right:parent.right; text:window.videoSession?"返回音乐":"收起"; onClicked:window.videoSession?Actions.leaveVideo():window.videoVisible=false }
                }
                GridView {
                    id:cardGrid; visible:window.viewKind==="cards"; Layout.fillWidth:true; Layout.fillHeight:true; clip:true
                    readonly property bool compact:window.height<620
                    property int columns:compact?1:Math.max(2,Math.floor(width/190))
                    cellWidth:width/columns; cellHeight:compact?76:Math.min(190,cellWidth-8)+76; model:window.items
                    ScrollBar.vertical:ScrollBar{}
                    delegate:ItemDelegate {
                        id:card
                        required property var modelData
                        width:cardGrid.compact?cardGrid.cellWidth-8:Math.min(190,cardGrid.cellWidth-8); height:cardGrid.cellHeight-8; padding:8
                        background:Rectangle { radius:6; color:card.hovered?Backend.theme.selection:"transparent"; border.width:card.activeFocus?1:0; border.color:Backend.theme.accent }
                        contentItem:GridLayout {
                            columns:cardGrid.compact?2:1; rowSpacing:8; columnSpacing:12
                            Rectangle {
                                Layout.fillWidth:!cardGrid.compact; Layout.preferredWidth:cardGrid.compact?52:card.availableWidth
                                Layout.preferredHeight:cardGrid.compact?52:card.availableWidth; color:Backend.theme.dark_background
                                Label { text:"♪"; anchors.centerIn:parent; font.pixelSize:32; opacity:.35 }
                                Image { objectName:"browseCover"; anchors.fill:parent; source:card.modelData.cover; sourceSize.width:320; sourceSize.height:320; fillMode:Image.PreserveAspectCrop; asynchronous:true }
                            }
                            ColumnLayout {
                                Layout.fillWidth:true; spacing:4
                                Label { text:card.modelData.name; Layout.fillWidth:true; elide:Text.ElideRight; textFormat:Text.PlainText }
                                Label { text:card.modelData.artist||""; visible:!!text; Layout.fillWidth:true; elide:Text.ElideRight; opacity:.65; font.pixelSize:Math.max(11,window.font.pixelSize-2); textFormat:Text.PlainText }
                            }
                            Item { visible:!cardGrid.compact; Layout.fillHeight:true }
                        }
                        onClicked:Actions.open(modelData)
                        Accessible.name:modelData.name
                    }
                }
                ListView {
                    id:trackList; visible:window.viewKind==="songs"; Layout.fillWidth:true; Layout.fillHeight:true; clip:true; model:window.items
                    ScrollBar.vertical:ScrollBar{}
                    delegate:ItemDelegate {
                        id:trackRow
                        required property var modelData; required property int index
                        width:trackList.width; height:Math.max(54,window.font.pixelSize*3.8)
                        background:Rectangle { color:trackRow.hovered?Backend.theme.selection:"transparent"; border.width:trackRow.activeFocus?1:0; border.color:Backend.theme.accent }
                        contentItem:RowLayout {
                            spacing:12
                            Label { text:trackRow.modelData.id===window.currentTrack.id&&Backend.playing?"▶":trackRow.index+1; Layout.preferredWidth:30; opacity:.65 }
                            ColumnLayout {
                                Layout.fillWidth:true; spacing:3
                                Label { text:trackRow.modelData.name; Layout.fillWidth:true; elide:Text.ElideRight; color:trackRow.modelData.id===window.currentTrack.id?Backend.theme.accent:Backend.theme.foreground }
                                Label { text:trackRow.modelData.artist||""; Layout.fillWidth:true; elide:Text.ElideRight; opacity:.7; font.pixelSize:Math.max(11,window.font.pixelSize-2) }
                            }
                            Label { text:trackRow.modelData.album||""; visible:window.width>1000; Layout.preferredWidth:170; elide:Text.ElideRight; opacity:.7 }
                            Label { text:trackRow.modelData.duration?window.formatTime(trackRow.modelData.duration/1000):""; opacity:.65 }
                            ActionButton { text:"···"; Accessible.name:"歌曲操作："+trackRow.modelData.name; quiet:true; onClicked:window.showTrackMenu(trackRow.modelData) }
                        }
                        onClicked:Actions.enqueue(modelData,true)
                        Accessible.name:modelData.name+" · "+modelData.artist
                    }
                }
                ListView {
                    id:feedList; visible:window.viewKind==="feed"; Layout.fillWidth:true; Layout.fillHeight:true; clip:true; model:window.items; spacing:12
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
                        onClicked:if(window.category==="私信"&&window.page===window.nav)Actions.conversation(modelData)
                    }
                }
                ListView {
                    id:downloadsList; visible:window.viewKind==="downloads"; Layout.fillWidth:true; Layout.fillHeight:true; clip:true
                    model:Backend.downloads; spacing:12; ScrollBar.vertical:ScrollBar{}
                    delegate:Rectangle {
                        required property var modelData
                        width:downloadsList.width; height:downloadColumn.implicitHeight+24; color:Backend.theme.dark_background
                        ColumnLayout {
                            id:downloadColumn; anchors.left:parent.left; anchors.right:parent.right; anchors.top:parent.top; anchors.margins:12; spacing:8
                            Label { text:modelData.name; Layout.fillWidth:true; elide:Text.ElideRight }
                            ProgressBar { Layout.fillWidth:true; from:0; to:Math.max(1,modelData.total); value:modelData.received; visible:modelData.state!=="complete" }
                            Label { text:({running:"下载中",paused:"已暂停",failed:"失败",complete:"已下载"}[modelData.state]||modelData.state)+" · "+(modelData.received/1048576).toFixed(1)+" MB"; opacity:.7 }
                            Label { text:modelData.error||""; visible:!!text; Layout.fillWidth:true; wrapMode:Text.Wrap; color:Backend.theme.red }
                            RowLayout {
                                ActionButton { text:"播放"; visible:modelData.state==="complete"; onClicked:Actions.playDownload(modelData) }
                                ActionButton { text:"暂停"; visible:modelData.state==="running"; onClicked:Backend.pauseDownload(modelData.taskId) }
                                ActionButton { text:"重新获取权限并继续"; visible:modelData.state==="paused"||modelData.state==="failed"; enabled:!window.downloadPending; onClicked:Actions.download(modelData,modelData.taskId) }
                            }
                        }
                    }
                    Label { anchors.centerIn:parent; visible:!downloadsList.count; text:"从歌曲菜单下载有权限的音乐"; opacity:.7 }
                }
                ActionButton { text:"加载更多"; visible:window.more; enabled:!window.loading; Layout.alignment:Qt.AlignHCenter; onClicked:Actions.more() }
                }
            }
            ContextPanel {
                visible: window.dockPanel && !!window.panel
                Layout.preferredWidth: 288; Layout.fillHeight: true
                app: window
                radius: 8
            }
        }
        PlayerBar {
            Layout.fillWidth: true
            app: window
            onOptionsRequested: playerOptions.open()
        }
    }
    Drawer {
        id:contextDrawer; edge:Qt.RightEdge; width:Math.min(400,window.width-32); height:window.height
        modal:true; focus:true; closePolicy:Popup.CloseOnEscape|Popup.CloseOnPressOutside
        onClosed:if(!window.dockPanel)window.panel=""
        contentItem:ContextPanel { app:window }
    }
    Popup {
        id:loginPopup; anchors.centerIn:parent; width:360; modal:true; focus:true; padding:24
        onClosed:Actions.cancelLogin()
        background:Rectangle { color:Backend.theme.background; border.color:Backend.theme.accent }
        contentItem:ColumnLayout {
            spacing:16
            Label { text:window.profile.userId?window.profile.nickname:"扫码登录"; font.pixelSize:22; Layout.fillWidth:true; elide:Text.ElideRight }
            Image { source:window.qr; visible:!window.profile.userId; Layout.preferredWidth:248; Layout.preferredHeight:248; Layout.alignment:Qt.AlignHCenter; fillMode:Image.PreserveAspectFit; smooth:false }
            Label { text:window.loginStatus; Layout.fillWidth:true; wrapMode:Text.Wrap }
            RowLayout {
                ActionButton { text:"刷新"; visible:!window.profile.userId&&Backend.loginPhase!=="authorizing"; onClicked:Actions.login() }
                ActionButton { text:"重试同步"; visible:Backend.loginPhase==="account-error"; onClicked:Backend.retryLogin() }
                ActionButton { text:"取消"; onClicked:loginPopup.close() }
                ActionButton { text:"退出账户"; visible:!!window.profile.userId; onClicked:Actions.logout() }
            }
        }
    }
    Popup {
        id:playerOptions; anchors.centerIn:parent; width:340; modal:true; focus:true; padding:20
        background:Rectangle { color:Backend.theme.background; border.color:Backend.theme.accent }
        contentItem:ColumnLayout {
            spacing:12
            Label { text:"播放选项"; font.pixelSize:20 }
            ComboBox { model:["顺序播放","列表循环","单曲循环"]; currentIndex:window.repeatMode; Layout.fillWidth:true; onActivated:{window.repeatMode=currentIndex;Actions.updateMetadata()} }
            RowLayout {
                ActionButton { text:"正在播放"; onClicked:{playerOptions.close();window.panel="正在播放"} }
                ActionButton { text:"歌词"; onClicked:{playerOptions.close();window.panel="歌词"} }
                ActionButton { text:"评论"; enabled:!!Models.commentThread(window.currentTrack); onClicked:{playerOptions.close();Actions.comments(window.currentTrack,false)} }
                ActionButton { text:window.likedIds.indexOf(window.currentTrack.id)>=0?"已喜欢":"喜欢"; enabled:window.currentTrack.kind==="song"&&!window.writeBusy; onClicked:Actions.like(window.currentTrack) }
            }
            RowLayout {
                Label { text:"播客速度" }
                ComboBox { model:["0.75×","1×","1.25×","1.5×","2×"]; currentIndex:1; Layout.fillWidth:true; onActivated:{window.podcastSpeed=[.75,1,1.25,1.5,2][currentIndex];if(window.currentTrack.kind==="episode")Backend.setSpeed(window.podcastSpeed)} }
            }
            RowLayout {
                Label { text:"睡眠定时" }
                ComboBox { model:["关闭","15 分钟","30 分钟","60 分钟"]; Layout.fillWidth:true; onActivated:window.sleepRemaining=[0,15,30,60][currentIndex] }
            }
            Label { visible:window.sleepRemaining>0; text:window.sleepRemaining+" 分钟后暂停"; opacity:.7 }
            ActionButton { text:"关闭"; Layout.alignment:Qt.AlignRight; onClicked:playerOptions.close() }
        }
    }
    Popup {
        id:settingsPopup; anchors.centerIn:parent; width:Math.min(440,window.width-40); modal:true; focus:true; padding:24
        background:Rectangle { color:Backend.theme.background; border.color:Backend.theme.accent }
        contentItem:ColumnLayout {
            spacing:16
            Label { text:"偏好设置"; font.pixelSize:22 }
            Label { text:"外观跟随 Omarchy 当前主题、系统等宽字体和字号。"; Layout.fillWidth:true; wrapMode:Text.Wrap; opacity:.8 }
            Label { text:"播放音质" }
            ComboBox { model:["标准","极高","无损","Hi-Res"]; currentIndex:Math.max(0,["standard","exhigh","lossless","hires"].indexOf(window.quality)); Layout.fillWidth:true; onActivated:{window.quality=["standard","exhigh","lossless","hires"][currentIndex];Backend.setState("quality",window.quality)} }
            Label { text:"实际音质以服务返回结果为准；需要对应歌曲权限。"; wrapMode:Text.Wrap; Layout.fillWidth:true; opacity:.7 }
            RowLayout { ActionButton { text:"添加文件"; onClicked:localDialog.open() } ActionButton { text:"添加文件夹"; onClicked:folderDialog.open() } }
            Label { text:"云间 · 开发预览版\n使用社区实现的消费端协议。"; wrapMode:Text.Wrap; Layout.fillWidth:true; opacity:.6; font.pixelSize:12 }
            ActionButton { text:"关闭"; Layout.alignment:Qt.AlignRight; onClicked:settingsPopup.close() }
        }
    }
    Menu {
        id:trackMenu
        MenuItem { text:"播放"; onTriggered:Actions.enqueue(window.menuTrack,true) }
        MenuItem { text:"下一首播放"; onTriggered:Actions.enqueue(window.menuTrack,false,true) }
        MenuItem { text:"加入队列"; onTriggered:Actions.enqueue(window.menuTrack,false,false) }
        MenuSeparator {}
        MenuItem { text:window.likedIds.indexOf(window.menuTrack.id)>=0?"取消喜欢":"喜欢"; enabled:window.menuTrack.kind==="song"&&!window.writeBusy; onTriggered:Actions.like(window.menuTrack) }
        MenuItem { text:"添加到歌单…"; enabled:window.menuTrack.kind==="song"; onTriggered:Actions.ownedPlaylists(window.menuTrack) }
        MenuItem { text:"打开专辑"; enabled:!!window.menuTrack.albumId; onTriggered:Actions.open({id:window.menuTrack.albumId,name:window.menuTrack.album,kind:"album"}) }
        MenuItem { text:"打开歌手"; enabled:!!window.menuTrack.artistId; onTriggered:Actions.open({id:window.menuTrack.artistId,name:window.menuTrack.artist,kind:"artist"}) }
        MenuItem { text:"播放 MV"; enabled:!!window.menuTrack.mv; onTriggered:Actions.playVideo({id:String(window.menuTrack.mv),name:window.menuTrack.name,kind:"mv",artist:window.menuTrack.artist}) }
        MenuItem { text:"评论"; enabled:!!Models.commentThread(window.menuTrack); onTriggered:Actions.comments(window.menuTrack,false) }
        MenuItem { text:"下载"; enabled:window.menuTrack.kind==="song"&&!window.downloadPending; onTriggered:Actions.download(window.menuTrack,"") }
        MenuItem { text:"复制链接"; enabled:window.menuTrack.kind!=="local"; onTriggered:Actions.copyLink(window.menuTrack) }
    }
    Menu {
        id:resourceMenu
        MenuItem { text:"重命名"; onTriggered:window.playlistEditorOpen(true) }
        MenuItem { text:"复制链接"; onTriggered:Actions.copyLink(window.resource) }
        MenuItem { text:"删除歌单…"; onTriggered:deleteDialog.open() }
    }
    Popup {
        id:playlistEditor; anchors.centerIn:parent; width:360; modal:true; focus:true; padding:24
        background:Rectangle { color:Backend.theme.background; border.color:Backend.theme.accent }
        contentItem:ColumnLayout {
            spacing:16
            Label { text:window.editingPlaylist?"重命名歌单":"新建歌单"; font.pixelSize:22 }
            TextField { id:playlistName; placeholderText:"歌单名称"; Layout.fillWidth:true; maximumLength:120; selectByMouse:true }
            CheckBox { id:privatePlaylist; text:"私密歌单"; checked:true; visible:!window.editingPlaylist }
            Label { text:window.editError; visible:!!text; wrapMode:Text.Wrap; Layout.fillWidth:true; color:Backend.theme.red }
            RowLayout {
                ActionButton { text:"取消"; enabled:!window.writeBusy; onClicked:playlistEditor.close() }
                ActionButton { text:window.writeBusy?"保存中…":"保存"; enabled:!window.writeBusy&&!!playlistName.text.trim(); onClicked:window.editingPlaylist?Actions.renamePlaylist(window.resource,playlistName.text):Actions.createPlaylist(playlistName.text,privatePlaylist.checked) }
            }
        }
    }
    Popup {
        id:addDialog; anchors.centerIn:parent; width:360; height:Math.min(460,window.height-40); modal:true; focus:true; padding:20
        background:Rectangle { color:Backend.theme.background; border.color:Backend.theme.accent }
        contentItem:ColumnLayout {
            spacing:12
            Label { text:"添加到歌单"; font.pixelSize:22 }
            Label { text:window.menuTrack.name||""; Layout.fillWidth:true; elide:Text.ElideRight; opacity:.7 }
            Label { text:window.editError; visible:!!text; Layout.fillWidth:true; wrapMode:Text.Wrap; color:Backend.theme.red }
            ListView {
                id:ownedList; model:window.ownedPlaylists; Layout.fillWidth:true; Layout.fillHeight:true; clip:true
                delegate:ActionButton { required property var modelData; width:ownedList.width; text:modelData.name; quiet:true; enabled:!window.writeBusy; onClicked:Actions.addTrack(modelData,window.menuTrack,false) }
                Label { visible:!window.ownedPlaylists.length; anchors.centerIn:parent; text:"暂无可编辑歌单" }
            }
            ActionButton { text:"取消"; onClicked:addDialog.close() }
        }
    }
    Dialog {
        id:deleteDialog; anchors.centerIn:parent; title:"删除歌单"; modal:true
        standardButtons:Dialog.Cancel|Dialog.Ok
        Label { text:"删除「"+(window.resource.name||"")+"」？此操作无法撤销。"; width:320; wrapMode:Text.Wrap }
        onAccepted:Actions.deletePlaylist(window.resource)
    }
    FolderDialog { id:folderDialog; title:"添加音乐文件夹"; onAccepted:{window.loading="正在扫描音乐文件夹…";Backend.scanLocal([selectedFolder])} }
    FileDialog {
        id:localDialog; title:"添加本地音乐"; fileMode:FileDialog.OpenFiles
        nameFilters:["音乐文件 (*.mp3 *.flac *.ogg *.opus *.m4a *.wav *.aac)","所有文件 (*)"]
        onAccepted:{window.loading="正在读取音乐标签…";Backend.scanLocal(selectedFiles)}
    }
    Shortcut { sequence:"Ctrl+K"; onActivated:searchBox.forceActiveFocus() }
    Shortcut { sequence:"Alt+Left"; onActivated:Actions.back() }
    Shortcut { sequence:"Ctrl+Right"; onActivated:Actions.next(false,false) }
    Shortcut { sequence:"Ctrl+Left"; onActivated:Actions.previous(false) }
    Shortcut { sequence:"Ctrl+Space"; onActivated:Actions.toggle() }
    Shortcut { sequence:"Ctrl+Q"; onActivated:Qt.quit() }
    Shortcut { sequence:"Ctrl+L"; onActivated:window.panel=window.panel==="歌词"?"":"歌词" }
    Shortcut { sequence:"Ctrl+O"; onActivated:localDialog.open() }
    onClosing:Actions.saveProgress()
    Component.onCompleted:Actions.init(window)
}
