.pragma library
.import "Models.js" as Models
var app;
var pending = {};
var pageSpec = null;
var history = [];
var accountEpoch = 0;
var savedQueue = null;
var savedVideoQueue = null;
function init(window) {
    app=window;
    navigate("Home");
    account(false);

}
function request(path, args, callback, mode, cacheRead) {
    var epoch=accountEpoch, id=app.backend.request(path,args||{},mode||"weapi",!!cacheRead);
    pending[id]=function(d,e){if(epoch===accountEpoch)callback(d,e);};
    return id;
}
function response(id, data, error) {var callback=pending[id];delete pending[id];if(callback)callback(JSON.parse(JSON.stringify(data)),error);}
function failure(d,e) {
    if (e) return /[\u3400-\u9fff]/.test(e) ? "The request could not be completed. Please try again." : e;
    if (Number(d.code)===200) return "";
    if (Number(d.code)===301) return "Please sign in again.";
    var detail=String(d.message||d.msg||"");
    return detail && !/[\u3400-\u9fff]/.test(detail) ? detail : "Service unavailable (code "+(d.code||"unknown")+"). Please try again.";
}
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
    app.loading="Loading…";app.error="";app.staleText="";app.more=false;app.offset=0;app.restoreScroll=0;pageSpec=null;
}
function needAccount() {if(app.profile.userId)return false;app.loading="";app.error="Sign in to view your library";return true;}
function spec(path,args,mode,field,kind) {return {path:path,args:args||{},mode:mode||"weapi",field:field,kind:kind};}
function select(data,path) {return path.split(".").reduce(function(v,k){return v?v[k]:undefined;},data);}
function fetch(specification,append) {
    pageSpec=specification;var gen=app.viewGeneration;
    var args=Object.assign({},specification.args);if(append)args.offset=app.offset;
    app.loading=append?"Loading more…":"Loading…";
    request(specification.path,args,function(d,e){
        if(gen!==app.viewGeneration)return;app.loading="";app.error=failure(d,e);if(app.error)return;
        app.staleText=d._stale?"Offline cache · "+new Date(Number(d._cacheTimestamp)*1000).toLocaleString():"";
        var raw=typeof specification.field==="function"?specification.field(d):select(d,specification.field);
        if(!Array.isArray(raw)){app.error="Unsupported response format from the service";return;}
        var list=specification.kind==="raw"?raw:Models.resources(raw,specification.kind);
        app.items=append?app.items.concat(list):list;app.offset=(append?app.offset:0)+raw.length;
        if(specification.path==="/api/user/playlist"&&String(args.uid)===String(app.profile.userId))app.libraryPlaylists=app.items.slice();
        var more=d.more; if(more===undefined && d.data)more=d.data.more;
        app.more=specification.paged!==false && (more!==undefined?!!more:!!args.limit&&raw.length===args.limit);
        if(!app.items.length)app.error="Nothing here yet";
    },specification.mode,true);
}
function more(){if(app.more&&!app.loading&&pageSpec)fetch(pageSpec,true);}
function refresh(){if(pageSpec){app.viewGeneration++;app.offset=0;fetch(pageSpec,false);}else navigate(app.nav,app.category);}
function navigate(nav,category) {
    push();app.nav=nav;app.category=category||({"Discover":"Featured playlists","Your Library":"Playlists","Activity":"Following"}[nav]||"");
    begin(nav,"cards",false);
    if(nav==="Home") {
        fetch(spec("/api/personalized/playlist",{limit:30,total:true,n:1000},"weapi","result","playlist"));
    } else if(nav==="Discover") {
        var cat=app.category;
        if(cat==="Featured playlists")fetch(spec("/api/playlist/list",{cat:"全部",order:"hot",limit:30,offset:0,total:true},"weapi","playlists","playlist"));
        else if(cat==="Charts"){var chart=spec("/api/toplist",{},"eapi","list","playlist");chart.paged=false;fetch(chart);}
        else if(cat==="New songs"){app.viewKind="songs";fetch(spec("/api/v1/discovery/new/songs",{areaId:0,total:true},"weapi","data","song"));}
        else if(cat==="New albums")fetch(spec("/api/album/new",{area:"ALL",limit:30,offset:0,total:true},"weapi","albums","album"));
        else if(cat==="Podcasts")fetch(spec("/api/djradio/recommend/v1",{},"weapi","djRadios","radio"));
        else if(cat==="MV")fetch(spec("/api/mv/all",{tags:JSON.stringify({"地区":"全部","类型":"全部","排序":"上升最快"}),limit:30,offset:0,total:"true"},"eapi","data","mv"));
    } else if(nav==="Your Library") library(app.category);
    else if(nav==="Activity") activity(app.category);
}
function library(category) {
    if(category==="Local music"){app.loading="";app.items=app.localTracks;app.viewKind="songs";return;}
    if(category==="Downloads"){app.loading="";app.viewKind="downloads";return;}
    if(needAccount())return;
    var uid=String(app.profile.userId), standard={limit:50,offset:0,total:true};
    if(category==="Playlists")fetch(spec("/api/user/playlist",Object.assign({uid:uid},standard),"weapi","playlist","playlist"));
    else if(category==="Liked songs") {
        app.viewKind="songs";var gen=app.viewGeneration;
        request("/api/song/like/get",{uid:uid},function(d,e){if(gen!==app.viewGeneration)return;app.error=failure(d,e);if(app.error){app.loading="";return;}app.likedIds=(d.ids||[]).map(String);loadTrackIds(app.likedIds,gen);},"eapi");
    } else if(category==="Albums")fetch(spec("/api/album/sublist",standard,"weapi","data","album"));
    else if(category==="Artists")fetch(spec("/api/artist/sublist",standard,"weapi","data","artist"));
    else if(category==="Podcasts")fetch(spec("/api/djradio/get/subed",standard,"weapi","djRadios","radio"));
    else if(category==="Saved videos")fetch(spec("/api/cloudvideo/allvideo/sublist",standard,"weapi",function(d){return (d.data||[]).map(function(t){return Object.assign({},t,{id:t.vid});});},"video"));
    else if(category==="Recently played"){app.viewKind="songs";fetch(spec("/api/play-record/song/list",{limit:100},"weapi",function(d){return ((d.data||{}).list||[]).map(function(t){return t.data;});},"song"));}
    else if(category==="Purchased albums")fetch(spec("/api/digitalAlbum/purchased",standard,"weapi","paidAlbums","album"));
    else if(category==="Cloud library"){app.viewKind="songs";fetch(spec("/api/v1/cloud/get",standard,"weapi","data","cloud"));}
}
function loadTrackIds(ids,gen) {
    var offset=0, tracks=[];
    function chunk() {
        if(gen!==app.viewGeneration)return;
        if(offset>=ids.length){app.items=tracks;app.loading="";if(!tracks.length)app.error="No tracks found";return;}
        var part=ids.slice(offset,offset+300);
        request("/api/v3/song/detail",{c:JSON.stringify(part.map(function(id){return {id:id};}))},function(d,e){
            if(gen!==app.viewGeneration)return;app.error=failure(d,e);
            if(app.error){app.loading="";app.items=tracks;app.error+=" · Loaded "+tracks.length+" / "+ids.length;return;}
            var map={};Models.resources(d.songs,"song").forEach(function(t){map[t.id]=t;});
            part.forEach(function(id){if(map[String(id)])tracks.push(map[String(id)]);else tracks.push({id:String(id),name:"Track information unavailable",artist:"",album:"",kind:"song",duration:0});});
            offset+=part.length;app.items=tracks.slice();app.loading="Loaded "+offset+" / "+ids.length;chunk();
        });
    } chunk();
}
var searchTypes=[{name:"Songs",type:1,field:"songs",kind:"song"},{name:"Albums",type:10,field:"albums",kind:"album"},
    {name:"Artists",type:100,field:"artists",kind:"artist"},{name:"Playlists",type:1000,field:"playlists",kind:"playlist"},
    {name:"Podcasts",type:1009,field:"djRadios",kind:"radio"},{name:"MV",type:1004,field:"mvs",kind:"mv"},
    {name:"Users",type:1002,field:"userprofiles",kind:"user"}];
function search(text,index) {
    if(!text.trim())return;if(/^https?:\/\//i.test(text.trim())){handleLink(text);return;}var type=searchTypes[index||0];app.query=text;app.searchType=index||0;
    begin("Search · "+text,type.kind==="song"?"songs":"cards",true);
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
function artistAlbums() {var artist=app.resource;begin(artist.name+" · Albums","cards",true);fetch(spec("/api/artist/albums/"+artist.id,{limit:30,offset:0,total:true},"weapi","hotAlbums","album"));}
function daily() {if(!app.profile.userId){app.openLogin();return;}begin("Daily recommendations","songs",true);fetch(spec("/api/v3/discovery/recommend/songs",{},"eapi","data.dailySongs","song"));}
function activity(category) {
    app.viewKind="feed";if(needAccount())return;
    if(category==="Following")fetch(spec("/api/v1/event/get",{pagesize:30,lasttime:-1},"weapi","event","raw"));
    else if(category==="Notifications")fetch(spec("/api/msg/notices",{limit:30,time:-1},"weapi","notices","raw"));
    else if(category==="Messages")fetch(spec("/api/msg/private/users",{limit:30,offset:0,total:"true"},"weapi","msgs","raw"));
}
function feedText(item) {
    var raw=item.json||item.notice||item.lastMsg||item.msg||"";
    try{var data=typeof raw==="string"?JSON.parse(raw):raw;return data.msg||data.content||data.title||"Shared music";}catch(e){return String(raw);}
}
function conversation(item) {
    var person=item.fromUser||item.user||{};if(!person.userId)return;
    begin(person.nickname||"Messages","feed",true);app.resource={kind:"conversation",id:String(person.userId)};
    fetch(spec("/api/msg/private/history",{userId:String(person.userId),limit:50,time:0,total:"true"},"weapi","msgs","raw"));
}
function account(showError){if(showError)app.backend.retryLogin();else app.backend.restoreAccount();}
function refreshSidebar(){
    if(!app.profile.userId){app.libraryPlaylists=[];return;}
    request("/api/user/playlist",{uid:String(app.profile.userId),limit:100,offset:0},function(d,e){
        if(!failure(d,e)&&Array.isArray(d.playlist))app.libraryPlaylists=Models.resources(d.playlist,"playlist");
    },"weapi",true);
}
function acceptAccount(profile,afterLogin){
    profile=JSON.parse(JSON.stringify(profile));
    if(app.profile.userId&&String(app.profile.userId)!==String(profile.userId)){
        accountEpoch++;pending={};app.viewGeneration++;app.playbackGeneration++;app.backend.stop();
        app.queue=[];app.queueIndex=-1;app.currentTrack={};app.backend.setMetadata({});app.comments=[];
        app.lyrics=[];app.contextTrack={};app.panel="";history=[];savedQueue=null;savedVideoQueue=null;app.fm=false;app.videoSession=false;
    }
    app.libraryPlaylists=[];app.profile=profile;app.closeLogin();
    request("/api/song/like/get",{uid:String(profile.userId)},function(data,error){if(!error&&Number(data.code)===200)app.likedIds=(data.ids||[]).map(String);},"eapi");
    if(afterLogin)navigate("Your Library");else refreshSidebar();
}
function login(){app.backend.startLogin();}
function cancelLogin(){app.backend.cancelLogin();}
function logout(){
    accountEpoch++;app.viewGeneration++;app.playbackGeneration++;cancelLogin();pending={};
    app.backend.logout();app.writeBusy=false;app.profile={};app.libraryPlaylists=[];app.queue=[];app.queueIndex=-1;app.currentTrack={};app.likedIds=[];
    app.comments=[];app.contextTrack={};app.lyrics=[];app.panel="";savedQueue=null;app.fm=false;history=[];
    app.backend.setMetadata({});app.closeLogin();navigate("Home");
}
function updateMetadata() {
    var metadata=Object.assign({},app.currentTrack);
    metadata.canNext=!app.videoSession&&(app.fm||app.queueIndex+1<app.queue.length||app.repeatMode===1);
    metadata.canPrevious=!app.videoSession&&app.queueIndex>0;app.backend.setMetadata(metadata);
}
function play(track,startPaused) {
    app.videoSession=false;savedVideoQueue=null;
    var gen=++app.playbackGeneration;
    app.backend.stop();app.currentTrack=track;app.actualQuality="";app.playerStatus="Getting playback access…";app.videoVisible=false;
    app.backend.setSpeed(track.kind==="episode"?app.podcastSpeed:1);updateMetadata();loadLyrics(track);
    if(track.kind==="local") {app.videoVisible=/\.(mp4|mkv|webm)$/i.test(track.url||"");app.playerStatus="";app.backend.load(track.url,0,0,!!startPaused);return;}
    request("/api/song/enhance/player/url/v1",{ids:JSON.stringify([track.id]),level:app.quality,encodeType:"flac"},function(d,e){
        if(gen!==app.playbackGeneration)return;
        var result=(d.data||[])[0], issue=failure(d,e);
        if(issue||!result||!result.url){app.playerStatus=issue||"Playback is unavailable. Check your sign-in or track access.";return;}
        app.actualQuality=result.level||result.type||"";
        var trial=result.freeTrialInfo;
        app.playerStatus=trial?"Preview · "+Number(trial.start||0)+"–"+Number(trial.end||30)+" sec":"";
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
    if(app.fm){if(app.queueIndex+1<app.queue.length){app.queueIndex++;play(app.queue[app.queueIndex],!!keepPaused);}else loadFm();return;}
    if(automatic&&app.repeatMode===2&&app.queueIndex>=0){play(app.queue[app.queueIndex],false);return;}
    var i=app.queueIndex+1;
    if(i>=app.queue.length&&app.repeatMode===1)i=0;
    if(i<app.queue.length){app.queueIndex=i;play(app.queue[i],!!keepPaused);}
    else {app.backend.stop();app.playerStatus="Playback finished";}
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
    var gen=++app.playbackGeneration;app.playerStatus="Loading Personal FM…";
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
    var thread=Models.commentThread(track);if(!thread){app.error="Comments are not available for this item";return;}
    if(!append){app.contextTrack=Object.assign({},track);app.comments=[];app.commentPage=1;app.commentCursor="";}
    app.panel="Comments";app.panelError="";var gen=++app.commentGeneration;
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
    if(!app.videoSession) savedVideoQueue={queue:app.queue,index:app.queueIndex,track:app.currentTrack,position:app.backend.position};
    app.videoSession=true;
    var gen=++app.playbackGeneration;app.backend.stop();app.currentTrack=Object.assign({},item,{entryId:app.backend.newId()});
    app.playerStatus="Getting video access…";app.lyrics=[];app.videoVisible=true;updateMetadata();
    var mv=item.kind==="mv"||item.videoType===0;
    request(mv?"/api/song/enhance/play/mv/url":"/api/cloudvideo/playurl",mv?{id:item.id,r:1080}:{ids:JSON.stringify([item.id]),resolution:1080},function(d,e){
        if(gen!==app.playbackGeneration)return;var result=mv?d.data:(d.urls||[])[0];
        app.playerStatus=failure(d,e)||(!result||!result.url?"Video playback is unavailable":"");
        if(!app.playerStatus)app.backend.load(result.url);
    },"weapi");
}
function collect(item,subscribe){
    if(!app.profile.userId){app.openLogin();return;}if(app.writeBusy)return;
    var paths={album:"/api/album/",artist:"/api/artist/",radio:"/api/djradio/",mv:"/api/mv/"};
    if(!paths[item.kind]){app.error="Saving this item is not supported in this preview";return;}
    var args={id:item.id};if(item.kind==="artist")args={artistId:item.id,artistIds:JSON.stringify([item.id])};
    if(item.kind==="mv")args={mvId:item.id,mvIds:JSON.stringify([item.id])};app.writeBusy=true;
    request(paths[item.kind]+(subscribe?"sub":"unsub"),args,function(d,e){app.writeBusy=false;app.error=failure(d,e)|| (subscribe?"Saved":"Removed from saved items");});
}
function createPlaylist(name,privateList){
    if(!name.trim()||app.writeBusy)return;app.writeBusy=true;
    request("/api/playlist/create",{name:name.trim(),privacy:privateList?"10":"0",type:"NORMAL"},function(d,e){
        app.writeBusy=false;var issue=failure(d,e);if(issue){app.editError=issue+(e?" · Result unconfirmed. Refresh your playlists before trying again.":"");return;}
        app.closePlaylistEditor();navigate("Your Library","Playlists");
    });
}
function renamePlaylist(item,name){
    if(!name.trim()||app.writeBusy)return;app.writeBusy=true;
    request("/api/playlist/update/name",{id:item.id,name:name.trim()},function(d,e){app.writeBusy=false;var issue=failure(d,e);
        if(issue){app.editError=issue;return;}app.closePlaylistEditor();refreshSidebar();open(Object.assign({},item,{name:name.trim()}));
    },"eapi");
}
function deletePlaylist(item){
    if(app.writeBusy)return;app.writeBusy=true;
    request("/api/playlist/remove",{ids:JSON.stringify([item.id])},function(d,e){app.writeBusy=false;app.error=failure(d,e);if(!app.error)navigate("Your Library","Playlists");});
}
function addTrack(playlist,track,remove){
    if(app.writeBusy)return;app.writeBusy=true;
    request("/api/playlist/manipulate/tracks",{op:remove?"del":"add",pid:playlist.id,trackIds:JSON.stringify([track.id]),imme:"true"},function(d,e){app.writeBusy=false;
        app.error=failure(d,e)||(remove?"Removed from playlist":"Added to playlist");if(!e&&d.code===200)app.closeAddDialog();
    },"eapi");
}
function ownedPlaylists(track){
    if(!app.profile.userId){app.openLogin();return;}app.menuTrack=track;app.ownedPlaylists=[];app.openAddDialog();
    request("/api/user/playlist",{uid:String(app.profile.userId),limit:1000,offset:0},function(d,e){
        app.editError=failure(d,e);app.ownedPlaylists=Models.resources((d.playlist||[]).filter(function(p){return String((p.creator||{}).userId)===String(app.profile.userId);}),"playlist");
    });
}
function copyLink(item){var kind=item.kind==="episode"?"program":item.kind==="radio"?"djradio":item.kind;
    if(item.kind==="local")return;app.backend.copyText("https://music.163.com/#/"+kind+"?id="+(item.programId||item.id));app.error="Link copied";
}
function localReady(tracks){
    tracks=JSON.parse(JSON.stringify(tracks));
    var map={};app.localTracks.concat(tracks).forEach(function(t){map[t.id]=t;});
    app.localTracks=Object.keys(map).map(function(id){return map[id];});app.backend.storeLocal(app.localTracks);
    navigate("Your Library","Local music");
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

function download(track,retryId){
    if(!app.profile.userId){app.openLogin();return;}
    if(app.downloadPending)return;app.downloadPending=true;
    request("/api/song/enhance/download/url/v1",{id:track.id,immerseType:"c51",level:app.quality},function(d,e){
        app.downloadPending=false;var grant=Array.isArray(d.data)?d.data[0]:d.data;
        app.error=failure(d,e)||(!grant||!grant.url?"Download access is unavailable":"");
        if(!app.error){app.backend.download(track,grant,retryId||"");navigate("Your Library","Downloads");}
    },"eapi");
}
function playDownload(task){
    enqueue(Object.assign({},task,{id:task.fileUrl,url:task.fileUrl,kind:"local"}),true);
}

function handleLink(text){
    var resource=app.backend.parseLink(text);
    if(!resource.id){app.error="Enter a valid NetEase Cloud Music link or local media file";return;}
    if(resource.kind==="song") {
        request("/api/v3/song/detail",{c:JSON.stringify([{id:resource.id}])},function(d,e){app.error=failure(d,e);if(!app.error&&(d.songs||[]).length)enqueue(Models.song(d.songs[0]),true);});
    } else open(resource);
}

function leaveVideo(){
    var saved=savedVideoQueue;savedVideoQueue=null;app.videoSession=false;app.videoVisible=false;app.playbackGeneration++;app.backend.stop();
    if(saved){app.queue=saved.queue;app.queueIndex=saved.index;app.currentTrack=saved.track;if(saved.track.id){app.resumeAt=saved.position;play(saved.track,true);}}
    updateMetadata();
}
