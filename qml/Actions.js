.pragma library
.import "Models.js" as Models
var app;
var pending = {};
var pageSpec = null;
var history = [];
var accountEpoch = 0;
var qrEpoch = 0;
var qrBusy = false;
var savedQueue = null;
function init(window) {
    app=window;
    navigate("首页");
    account(false);
    try { app.localTracks=JSON.parse(app.backend.state("localTracks")||"[]"); } catch(e) {}
}
function request(path, args, callback, mode) {
    var epoch=accountEpoch, id=app.backend.request(path,args||{},mode||"weapi");
    pending[id]=function(d,e){if(epoch===accountEpoch)callback(d,e);};
    return id;
}
function response(id, data, error) {var callback=pending[id];delete pending[id];if(callback)callback(data,error);}
function failure(d,e) {return e || (d.code!==200 ? d.message||d.msg||("服务暂不可用（"+d.code+"）") : "");}
function push() {
    history.push({page:app.page,nav:app.nav,category:app.category,items:app.items,kind:app.viewKind,
        resource:app.resource,spec:pageSpec,offset:app.offset,more:app.more,scroll:app.scrollPosition});
    if(history.length>30)history.shift();app.canBack=history.length>0;
}
function back() {
    if(!history.length)return;
    var s=history.pop();app.viewGeneration++;app.loading="";app.error="";
    app.page=s.page;app.nav=s.nav;app.category=s.category;app.items=s.items;app.viewKind=s.kind;
    app.resource=s.resource;pageSpec=s.spec;app.offset=s.offset;app.more=s.more;app.restoreScroll=s.scroll;
    app.canBack=history.length>0;
}
function begin(title,kind,remember) {
    if(remember)push();
    app.viewGeneration++;app.page=title;app.viewKind=kind;app.items=[];app.resource={};
    app.loading="正在加载…";app.error="";app.more=false;app.offset=0;app.restoreScroll=0;pageSpec=null;
}
function needAccount() {if(app.profile.userId)return false;app.loading="";app.error="登录后查看你的音乐";return true;}
function spec(path,args,mode,field,kind) {return {path:path,args:args||{},mode:mode||"weapi",field:field,kind:kind};}
function select(data,path) {return path.split(".").reduce(function(v,k){return v?v[k]:undefined;},data);}
function fetch(specification,append) {
    pageSpec=specification;var gen=app.viewGeneration;
    var args=Object.assign({},specification.args);if(append)args.offset=app.offset;
    app.loading=append?"正在加载更多…":"正在加载…";
    request(specification.path,args,function(d,e){
        if(gen!==app.viewGeneration)return;app.loading="";app.error=failure(d,e);if(app.error)return;
        var raw=typeof specification.field==="function"?specification.field(d):select(d,specification.field);
        if(!Array.isArray(raw)){app.error="服务返回的内容格式暂不支持";return;}
        var list=specification.kind==="raw"?raw:Models.resources(raw,specification.kind);
        app.items=append?app.items.concat(list):list;app.offset=(append?app.offset:0)+raw.length;
        var more=d.more; if(more===undefined && d.data)more=d.data.more;
        app.more=specification.paged!==false && (more!==undefined?!!more:!!args.limit&&raw.length===args.limit);
        if(!app.items.length)app.error="暂无内容";
    },specification.mode);
}
function more(){if(app.more&&!app.loading&&pageSpec)fetch(pageSpec,true);}
function refresh(){if(pageSpec){app.viewGeneration++;app.offset=0;fetch(pageSpec,false);}else navigate(app.nav,app.category);}
function navigate(nav,category) {
    push();app.nav=nav;app.category=category||({"发现":"精选歌单","我的音乐":"歌单","动态":"关注动态"}[nav]||"");
    begin(nav,"cards",false);
    if(nav==="首页") {
        fetch(spec("/api/personalized/playlist",{limit:30,total:true,n:1000},"weapi","result","playlist"));
    } else if(nav==="发现") {
        var cat=app.category;
        if(cat==="精选歌单")fetch(spec("/api/playlist/list",{cat:"全部",order:"hot",limit:30,offset:0,total:true},"weapi","playlists","playlist"));
        else if(cat==="排行榜"){var chart=spec("/api/toplist",{},"eapi","list","playlist");chart.paged=false;fetch(chart);}
        else if(cat==="新歌"){app.viewKind="songs";fetch(spec("/api/v1/discovery/new/songs",{areaId:0,total:true},"weapi","data","song"));}
        else if(cat==="新碟")fetch(spec("/api/album/new",{area:"ALL",limit:30,offset:0,total:true},"weapi","albums","album"));
        else if(cat==="播客")fetch(spec("/api/djradio/recommend/v1",{},"weapi","djRadios","radio"));
        else if(cat==="MV")fetch(spec("/api/mv/all",{tags:JSON.stringify({"地区":"全部","类型":"全部","排序":"上升最快"}),limit:30,offset:0,total:"true"},"eapi","data","mv"));
    } else if(nav==="我的音乐") library(app.category);
    else if(nav==="动态") activity(app.category);
}
function library(category) {
    if(category==="本地音乐"){app.loading="";app.items=app.localTracks;app.viewKind="songs";return;}
    if(category==="下载"){app.loading="";app.viewKind="downloads";return;}
    if(needAccount())return;
    var uid=String(app.profile.userId), standard={limit:50,offset:0,total:true};
    if(category==="歌单")fetch(spec("/api/user/playlist",Object.assign({uid:uid},standard),"weapi","playlist","playlist"));
    else if(category==="喜欢的音乐") {
        app.viewKind="songs";var gen=app.viewGeneration;
        request("/api/song/like/get",{uid:uid},function(d,e){if(gen!==app.viewGeneration)return;app.error=failure(d,e);if(app.error){app.loading="";return;}app.likedIds=(d.ids||[]).map(String);loadTrackIds(app.likedIds,gen);},"eapi");
    } else if(category==="专辑")fetch(spec("/api/album/sublist",standard,"weapi","data","album"));
    else if(category==="歌手")fetch(spec("/api/artist/sublist",standard,"weapi","data","artist"));
    else if(category==="播客")fetch(spec("/api/djradio/get/subed",standard,"weapi","djRadios","radio"));
    else if(category==="收藏视频")fetch(spec("/api/cloudvideo/allvideo/sublist",standard,"weapi",function(d){return (d.data||[]).map(function(t){return Object.assign({},t,{id:t.vid});});},"video"));
    else if(category==="最近播放"){app.viewKind="songs";fetch(spec("/api/play-record/song/list",{limit:100},"weapi",function(d){return ((d.data||{}).list||[]).map(function(t){return t.data;});},"song"));}
    else if(category==="已购专辑")fetch(spec("/api/digitalAlbum/purchased",standard,"weapi","paidAlbums","album"));
    else if(category==="音乐云盘"){app.viewKind="songs";fetch(spec("/api/v1/cloud/get",standard,"weapi","data","cloud"));}
}
function loadTrackIds(ids,gen) {
    var offset=0, tracks=[];
    function chunk() {
        if(gen!==app.viewGeneration)return;
        if(offset>=ids.length){app.items=tracks;app.loading="";if(!tracks.length)app.error="暂无歌曲";return;}
        var part=ids.slice(offset,offset+300);
        request("/api/v3/song/detail",{c:JSON.stringify(part.map(function(id){return {id:id};}))},function(d,e){
            if(gen!==app.viewGeneration)return;app.error=failure(d,e);
            if(app.error){app.loading="";app.items=tracks;app.error+=" · 已加载 "+tracks.length+" / "+ids.length;return;}
            var map={};Models.resources(d.songs,"song").forEach(function(t){map[t.id]=t;});
            part.forEach(function(id){if(map[String(id)])tracks.push(map[String(id)]);else tracks.push({id:String(id),name:"歌曲信息暂不可用",artist:"",album:"",kind:"song",duration:0});});
            offset+=part.length;app.items=tracks.slice();app.loading="已加载 "+offset+" / "+ids.length;chunk();
        });
    } chunk();
}
var searchTypes=[{name:"歌曲",type:1,field:"songs",kind:"song"},{name:"专辑",type:10,field:"albums",kind:"album"},
    {name:"歌手",type:100,field:"artists",kind:"artist"},{name:"歌单",type:1000,field:"playlists",kind:"playlist"},
    {name:"播客",type:1009,field:"djRadios",kind:"radio"},{name:"MV",type:1004,field:"mvs",kind:"mv"},
    {name:"用户",type:1002,field:"userprofiles",kind:"user"}];
function search(text,index) {
    if(!text.trim())return;var type=searchTypes[index||0];app.query=text;app.searchType=index||0;
    begin("搜索 · "+text,type.kind==="song"?"songs":"cards",true);
    fetch(spec("/api/cloudsearch/pc",{s:text,type:type.type,limit:50,offset:0},"eapi","result."+type.field,type.kind));
}
function open(item) {
    if(item.kind==="song"||item.kind==="episode"||item.kind==="local"){enqueue(item,true);return;}
    if(item.kind==="mv"||item.kind==="video"){playVideo(item);return;}
    begin(item.name,"songs",true);app.resource=item;
    var gen=app.viewGeneration;
    if(item.kind==="playlist") request("/api/v6/playlist/detail",{id:item.id,n:1000,s:0},function(d,e){
        if(gen!==app.viewGeneration)return;app.error=failure(d,e);if(app.error){app.loading="";return;}
        var p=d.playlist||{};app.resource=Models.resource(p,"playlist");
        if(p.trackIds)loadTrackIds(p.trackIds.map(function(t){return String(t.id);}),gen);
        else{app.items=Models.resources(p.tracks,"song");app.loading="";}
    });
    else if(item.kind==="album")fetch(spec("/api/v1/album/"+item.id,{},"weapi","songs","song"));
    else if(item.kind==="artist")fetch(spec("/api/v1/artist/songs",{id:item.id,private_cloud:"true",work_type:1,order:"hot",limit:50,offset:0},"eapi","songs","song"));
    else if(item.kind==="radio")fetch(spec("/api/dj/program/byradio",{radioId:item.id,limit:50,offset:0,asc:false},"weapi","programs","episode"));
    else if(item.kind==="user"){app.viewKind="cards";fetch(spec("/api/user/playlist",{uid:item.id,limit:50,offset:0},"weapi","playlist","playlist"));}
}
function artistAlbums() {var artist=app.resource;begin(artist.name+" · 专辑","cards",true);fetch(spec("/api/artist/albums/"+artist.id,{limit:30,offset:0,total:true},"weapi","hotAlbums","album"));}
function daily() {if(!app.profile.userId){app.openLogin();return;}begin("每日推荐","songs",true);fetch(spec("/api/v3/discovery/recommend/songs",{},"eapi","data.dailySongs","song"));}
function activity(category) {
    app.viewKind="feed";if(needAccount())return;
    if(category==="关注动态")fetch(spec("/api/v1/event/get",{pagesize:30,lasttime:-1},"weapi","event","raw"));
    else if(category==="通知")fetch(spec("/api/msg/notices",{limit:30,time:-1},"weapi","notices","raw"));
    else if(category==="私信")fetch(spec("/api/msg/private/users",{limit:30,offset:0,total:"true"},"weapi","msgs","raw"));
}
function feedText(item) {
    var raw=item.json||item.notice||item.lastMsg||item.msg||"";
    try{var data=typeof raw==="string"?JSON.parse(raw):raw;return data.msg||data.content||data.title||"音乐分享";}catch(e){return String(raw);}
}
function conversation(item) {
    var person=item.fromUser||item.user||{};if(!person.userId)return;
    begin(person.nickname||"私信","feed",true);app.resource={kind:"conversation",id:String(person.userId)};
    fetch(spec("/api/msg/private/history",{userId:String(person.userId),limit:50,time:0,total:"true"},"weapi","msgs","raw"));
}
function account(showError) {
    request("/api/w/nuser/account/get",{},function(d,e){
        if(!e&&d.profile){app.profile=d.profile;app.backend.saveAccount(String(d.profile.userId));app.closeLogin();app.loginPolling=false;
            request("/api/song/like/get",{uid:String(d.profile.userId)},function(data,error){if(!error&&data.code===200)app.likedIds=(data.ids||[]).map(String);},"eapi");
            if(showError)navigate("我的音乐");
        } else if(showError)app.loginStatus=e||"账户同步失败，请刷新登录";
    });
}
function login() {
    var gen=++qrEpoch;qrBusy=false;app.loginPolling=false;app.qr="";app.loginStatus="正在获取二维码…";
    request("/api/login/qrcode/unikey",{type:3},function(d,e){if(gen!==qrEpoch)return;
        var key=d.unikey||(d.data||{}).unikey;
        if(e||!key){app.loginStatus=e||"二维码获取失败，请刷新";return;}
        app.qrKey=key;app.qr=app.backend.qrImage("https://music.163.com/login?codekey="+key);
        app.loginStatus="使用网易云音乐手机 App 扫码确认";app.loginPolling=true;
    },"eapi");
}
function cancelLogin(){qrEpoch++;app.loginPolling=false;qrBusy=false;}
function pollLogin(){
    if(qrBusy)return;qrBusy=true;var gen=qrEpoch;
    request("/api/login/qrcode/client/login",{key:app.qrKey,type:3},function(d,e){if(gen!==qrEpoch)return;qrBusy=false;
        if(e){app.loginStatus=e;return;}
        if(d.code===803){app.loginPolling=false;app.loginStatus="正在同步账户…";account(true);}
        else if(d.code===800){app.loginPolling=false;app.loginStatus="二维码已过期，请刷新";}
        else if(d.code===802)app.loginStatus="已扫码，请在手机上确认";
    },"eapi");
}
function logout(){
    accountEpoch++;app.viewGeneration++;app.playbackGeneration++;cancelLogin();pending={};
    app.backend.logout();app.profile={};app.queue=[];app.queueIndex=-1;app.currentTrack={};app.likedIds=[];
    app.comments=[];app.contextTrack={};app.lyrics=[];app.panel="";savedQueue=null;app.fm=false;history=[];
    app.backend.setMetadata({});app.closeLogin();navigate("首页");
}
function updateMetadata() {
    var metadata=Object.assign({},app.currentTrack);
    metadata.canNext=app.fm||app.queueIndex+1<app.queue.length||app.repeatMode===1;
    metadata.canPrevious=app.queueIndex>0;app.backend.setMetadata(metadata);
}
function play(track,startPaused) {
    var gen=++app.playbackGeneration;
    app.backend.stop();app.currentTrack=track;app.actualQuality="";app.playerStatus="正在获取播放地址…";app.videoVisible=false;
    app.backend.setSpeed(track.kind==="episode"?app.podcastSpeed:1);updateMetadata();loadLyrics(track);
    if(track.kind==="local") {app.playerStatus="";app.backend.load(track.url,0,0,!!startPaused);return;}
    request("/api/song/enhance/player/url/v1",{ids:JSON.stringify([track.id]),level:app.quality,encodeType:"flac"},function(d,e){
        if(gen!==app.playbackGeneration)return;
        var result=(d.data||[])[0], issue=failure(d,e);
        if(issue||!result||!result.url){app.playerStatus=issue||"服务未提供播放地址，请检查登录或歌曲权限";return;}
        app.actualQuality=result.level||result.type||"";
        var trial=result.freeTrialInfo;
        app.playerStatus=trial?"试听 · "+Number(trial.start||0)+"–"+Number(trial.end||30)+" 秒":"";
        app.backend.load(result.url,trial?Number(trial.end||30):0,trial?Number(trial.start||0):0,!!startPaused);
    },"xeapi");
}
function enqueue(track,playNow,next) {
    if(app.fm)leaveFm();
    var entry=Models.makeQueue([track],function(){return app.backend.newId();})[0];var q=app.queue.slice();
    var position=next?app.queueIndex+1:q.length;q.splice(position,0,entry);app.queue=q;
    if(playNow){app.queueIndex=position;play(entry,false);}updateMetadata();
}
function playAll(){
    if(!app.items.length)return;if(app.fm)leaveFm();
    app.queue=Models.makeQueue(app.items,function(){return app.backend.newId();});app.queueIndex=0;play(app.queue[0],false);
}
function queuePlay(index){if(index<0||index>=app.queue.length)return;app.queueIndex=index;play(app.queue[index],false);}
function next(automatic,keepPaused){
    if(app.fm){loadFm();return;}
    if(automatic&&app.repeatMode===2&&app.queueIndex>=0){play(app.queue[app.queueIndex],false);return;}
    var i=app.queueIndex+1;
    if(i>=app.queue.length&&app.repeatMode===1)i=0;
    if(i<app.queue.length){app.queueIndex=i;play(app.queue[i],!!keepPaused);}
    else {app.backend.stop();app.playerStatus="播放完毕";}
}
function previous(keepPaused){if(app.queueIndex>0){app.queueIndex--;play(app.queue[app.queueIndex],!!keepPaused);}else if(app.backend.loaded)app.backend.seek(0);}
function toggle(){if(app.backend.loaded)app.backend.toggle();else if(app.currentTrack.id)play(app.currentTrack,false);}
function removeQueue(index){
    var active=app.currentTrack.entryId, q=app.queue.slice();q.splice(index,1);app.queue=q;
    app.queueIndex=q.findIndex(function(t){return t.entryId===active;});
    if(app.queueIndex<0&&active){app.playbackGeneration++;app.backend.stop();app.currentTrack={};app.lyrics=[];}
    updateMetadata();
}
function moveQueue(from,to){
    var active=app.currentTrack.entryId;app.queue=Models.moveEntry(app.queue,from,to);
    app.queueIndex=app.queue.findIndex(function(t){return t.entryId===active;});updateMetadata();
}
function shuffle(){app.queue=Models.shuffleAfter(app.queue,app.queueIndex,Math.random);updateMetadata();}
function fm(){
    if(!app.profile.userId){app.openLogin();return;}
    if(app.fm){leaveFm();return;}
    savedQueue={queue:app.queue,index:app.queueIndex,track:app.currentTrack,position:app.backend.position};
    app.fm=true;loadFm();
}
function loadFm(){
    var gen=++app.playbackGeneration;app.playerStatus="正在加载私人 FM…";
    request("/api/v1/radio/get",{},function(d,e){if(gen!==app.playbackGeneration||!app.fm)return;
        var tracks=Models.resources(d.data,"song");app.playerStatus=failure(d,e);
        if(app.playerStatus||!tracks.length)return;
        app.queue=Models.makeQueue(tracks,function(){return app.backend.newId();});app.queueIndex=0;play(app.queue[0],false);
    });
}
function leaveFm(){
    app.fm=false;app.playbackGeneration++;app.backend.stop();
    if(savedQueue){app.queue=savedQueue.queue;app.queueIndex=savedQueue.index;app.currentTrack=savedQueue.track;
        if(app.currentTrack.id){app.resumeAt=savedQueue.position;play(app.currentTrack,true);}savedQueue=null;}
    updateMetadata();
}
function like(track){
    if(!app.profile.userId){app.openLogin();return;}
    if(!track.id||track.kind==="local"||app.writeBusy)return;
    var enabled=app.likedIds.indexOf(track.id)<0;app.writeBusy=true;
    request("/api/radio/like",{trackId:track.id,like:enabled,alg:"itembased",time:"3"},function(d,e){
        app.writeBusy=false;var issue=failure(d,e);if(issue){app.error=issue;return;}
        var ids=app.likedIds.filter(function(id){return id!==track.id;});if(enabled)ids.push(track.id);app.likedIds=ids;
    });
}
function loadLyrics(track){
    var gen=app.playbackGeneration;app.lyrics=[];
    if(track.kind!=="song")return;
    request("/api/song/lyric/v1",{id:track.id,cp:false,tv:0,lv:0,rv:0,kv:0,yv:0,ytv:0,yrv:0},function(d,e){
        if(gen!==app.playbackGeneration||e)return;
        app.lyrics=Models.mergeLyrics(Models.parseLyrics((d.lrc||{}).lyric),Models.parseLyrics((d.tlyric||{}).lyric));
    },"eapi");
}
function comments(track,append){
    var thread=Models.commentThread(track);if(!thread){app.error="此内容暂无评论入口";return;}
    if(!append){app.contextTrack=Object.assign({},track);app.comments=[];app.commentPage=1;app.commentCursor="";}
    app.panel="评论";app.panelError="";var gen=++app.commentGeneration;
    request("/api/v2/resource/comments",{threadId:thread,pageNo:app.commentPage,pageSize:30,showInner:true,sortType:app.commentSort,cursor:app.commentCursor},function(d,e){
        if(gen!==app.commentGeneration||Models.commentThread(app.contextTrack)!==thread)return;
        app.panelError=failure(d,e);if(app.panelError)return;
        var data=d.data||{};app.comments=append?app.comments.concat(data.comments||[]):data.comments||[];
        app.commentCursor=String(data.cursor||"");app.commentMore=!!data.hasMore;
    },"eapi");
}
function likeComment(comment){
    if(!app.profile.userId){app.openLogin();return;}if(app.writeBusy)return;
    var target=Models.commentThread(app.contextTrack), gen=app.commentGeneration;app.writeBusy=true;
    request("/api/v1/comment/"+(comment.liked?"unlike":"like"),{threadId:target,commentId:String(comment.commentId)},function(d,e){
        app.writeBusy=false;app.panelError=failure(d,e);if(app.panelError||gen!==app.commentGeneration)return;
        app.comments=app.comments.map(function(c){return c.commentId===comment.commentId?Object.assign({},c,{liked:!c.liked,likedCount:Math.max(0,(c.likedCount||0)+(c.liked?-1:1))}):c;});
    });
}
function playVideo(item){
    var gen=++app.playbackGeneration;app.backend.stop();app.currentTrack=Object.assign({},item,{entryId:app.backend.newId()});
    app.playerStatus="获取视频地址…";app.lyrics=[];app.videoVisible=true;updateMetadata();
    var mv=item.kind==="mv"||item.videoType===0;
    request(mv?"/api/song/enhance/play/mv/url":"/api/cloudvideo/playurl",mv?{id:item.id,r:1080}:{ids:JSON.stringify([item.id]),resolution:1080},function(d,e){
        if(gen!==app.playbackGeneration)return;var result=mv?d.data:(d.urls||[])[0];
        app.playerStatus=failure(d,e)||(!result||!result.url?"服务未提供视频播放地址":"");
        if(!app.playerStatus)app.backend.load(result.url);
    },mv?"weapi":"eapi");
}
function collect(item,subscribe){
    if(!app.profile.userId){app.openLogin();return;}if(app.writeBusy)return;
    var paths={album:"/api/album/",artist:"/api/artist/",radio:"/api/djradio/",mv:"/api/mv/"};
    if(!paths[item.kind]){app.error="当前预览版尚未接入此收藏操作";return;}
    var args={id:item.id};if(item.kind==="artist")args={artistId:item.id,artistIds:JSON.stringify([item.id])};
    if(item.kind==="mv")args={mvId:item.id,mvIds:JSON.stringify([item.id])};app.writeBusy=true;
    request(paths[item.kind]+(subscribe?"sub":"unsub"),args,function(d,e){app.writeBusy=false;app.error=failure(d,e)|| (subscribe?"已收藏":"已取消收藏");});
}
function createPlaylist(name,privateList){
    if(!name.trim()||app.writeBusy)return;app.writeBusy=true;
    request("/api/playlist/create",{name:name.trim(),privacy:privateList?"10":"0",type:"NORMAL"},function(d,e){
        app.writeBusy=false;var issue=failure(d,e);if(issue){app.editError=issue+(e?" · 结果待确认，请先刷新歌单后再重试":"");return;}
        app.closePlaylistEditor();navigate("我的音乐","歌单");
    });
}
function renamePlaylist(item,name){
    if(!name.trim()||app.writeBusy)return;app.writeBusy=true;
    request("/api/playlist/update/name",{id:item.id,name:name.trim()},function(d,e){app.writeBusy=false;var issue=failure(d,e);
        if(issue){app.editError=issue;return;}app.closePlaylistEditor();open(Object.assign({},item,{name:name.trim()}));
    },"eapi");
}
function deletePlaylist(item){
    if(app.writeBusy)return;app.writeBusy=true;
    request("/api/playlist/remove",{ids:JSON.stringify([item.id])},function(d,e){app.writeBusy=false;app.error=failure(d,e);if(!app.error)navigate("我的音乐","歌单");});
}
function addTrack(playlist,track,remove){
    if(app.writeBusy)return;app.writeBusy=true;
    request("/api/playlist/manipulate/tracks",{op:remove?"del":"add",pid:playlist.id,trackIds:JSON.stringify([track.id]),imme:"true"},function(d,e){app.writeBusy=false;
        app.error=failure(d,e)||(remove?"已从歌单移除":"已添加到歌单");if(!e&&d.code===200)app.closeAddDialog();
    },"eapi");
}
function ownedPlaylists(track){
    if(!app.profile.userId){app.openLogin();return;}app.menuTrack=track;app.ownedPlaylists=[];app.openAddDialog();
    request("/api/user/playlist",{uid:String(app.profile.userId),limit:1000,offset:0},function(d,e){
        app.editError=failure(d,e);app.ownedPlaylists=Models.resources((d.playlist||[]).filter(function(p){return String((p.creator||{}).userId)===String(app.profile.userId);}),"playlist");
    });
}
function copyLink(item){var kind=item.kind==="episode"?"program":item.kind==="radio"?"djradio":item.kind;
    if(item.kind==="local")return;app.backend.copyText("https://music.163.com/#/"+kind+"?id="+(item.programId||item.id));app.error="链接已复制";
}
function localReady(tracks){
    var map={};app.localTracks.concat(tracks).forEach(function(t){map[t.id]=t;});
    app.localTracks=Object.keys(map).map(function(id){return map[id];});app.backend.setState("localTracks",JSON.stringify(app.localTracks));
    navigate("我的音乐","本地音乐");
}
function loaded(){
    if(app.resumeAt>0){app.backend.seek(app.resumeAt);app.resumeAt=0;}
    else if(app.currentTrack.kind==="episode"){
        var pos=Number(app.backend.state("resume/"+String(app.profile.userId||"guest")+"/"+app.currentTrack.programId));
        if(pos>0&&pos<app.backend.duration()-10)app.backend.seek(pos);
    }
}
function saveProgress(){if(app.currentTrack.kind==="episode"&&app.backend.loaded)
    app.backend.setState("resume/"+String(app.profile.userId||"guest")+"/"+app.currentTrack.programId,String(app.backend.position));}
function desktopAction(action){
    if(action==="next")next(false,!app.backend.playing);
    else if(action==="previous")previous(!app.backend.playing);
    else if(action==="play")toggle();
    else if(action==="stop"){app.playbackGeneration++;app.backend.stop();}
}
