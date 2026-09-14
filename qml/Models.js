.pragma library
function song(t) {
    t = t || {};
    var artists = t.ar || t.artists || [];
    var album = t.al || t.album || {};
    return {id: String(t.id || t.songId || ""), name: t.name || t.songName || "Untitled",
        artist: artists.map(function(a) { return a.name; }).join(" / ") || t.artist || "",
        artistId: artists.length ? String(artists[0].id) : "", album: album.name || t.albumName || "",
        albumId: album.id ? String(album.id) : "", cover: album.picUrl || t.picUrl || "",
        duration: t.dt || t.duration || 0, kind: "song", mv: t.mv || t.mvid || 0};
}
function resource(t, kind) {
    if (kind === "song") return song(t);
    if (kind === "episode") {
        var item = song(t.mainSong); item.kind = "episode";
        item.programId = String(t.id); item.name = t.name || item.name;
        item.artist = (t.dj || {}).nickname || item.artist; item.cover = t.coverUrl || item.cover;
        return item;
    }
    if (kind === "cloud") {
        var cloud = song(t.simpleSong || {}); cloud.id = String(t.songId || cloud.id);
        cloud.name = t.songName || cloud.name; cloud.artist = t.artist || cloud.artist;
        cloud.album = t.album || cloud.album; return cloud;
    }
    return {id: String(t.id || t.vid || t.userId || ""), kind: kind,
        name: t.name || t.title || t.nickname || "Untitled",
        cover: t.picUrl || t.coverImgUrl || t.cover || t.imgurl || t.avatarUrl || t.pic || "",
        artist: t.artistName || (t.creator || {}).nickname || (t.dj || {}).nickname ||
            (t.artists || []).map(function(a) {return a.name;}).join(" / "),
        creatorId: String((t.creator || {}).userId || ""),
        description: t.description || t.briefDesc || "", duration: t.duration || 0,
        videoType: t.type, raw: t};
}
function resources(list, kind) { return (list || []).map(function(t) { return resource(t, kind); }); }
function parseLyrics(lrc) {
    var lines = [];
    String(lrc || "").split(/\r?\n/).forEach(function(line) {
        var re = /\[(\d+):(\d+(?:\.\d+)?)\]/g, match;
        var text = line.replace(/\[[^\]]*\]/g, "").trim();
        while ((match = re.exec(line))) lines.push({time:Number(match[1])*60+Number(match[2]), text:text});
    });
    return lines.sort(function(a,b){return a.time-b.time;});
}
function lyricIndex(lines, time) {
    var lo=0, hi=lines.length;
    while(lo<hi) { var mid=(lo+hi)>>>1; if(lines[mid].time<=time)lo=mid+1;else hi=mid; }
    return lo-1;
}
function mergeLyrics(original, translated) {
    return original.map(function(line) {
        var i=lyricIndex(translated,line.time+0.05), value="";
        if(i>=0 && Math.abs(translated[i].time-line.time)<0.15)value=translated[i].text;
        return {time:line.time,text:line.text,translation:value};
    });
}
function makeQueue(tracks, newId) {
    return tracks.map(function(track){var result=Object.assign({},track);result.entryId=newId();return result;});
}
function shuffleAfter(queue, index, random) {
    var result=queue.slice(), start=Math.max(0,index+1);
    for(var i=result.length-1;i>start;i--){var j=start+Math.floor(random()*(i-start+1));var t=result[i];result[i]=result[j];result[j]=t;}
    return result;
}
function moveEntry(queue, from, to) {
    var result=queue.slice();
    if(from<0||from>=result.length||to<0||to>=result.length)return result;
    result.splice(to,0,result.splice(from,1)[0]);return result;
}
function commentThread(track) {
    var prefix={song:"R_SO_4_",episode:"A_DJ_1_",mv:"R_MV_5_",video:"R_VI_62_",playlist:"A_PL_0_",album:"R_AL_3_",radio:"A_DR_14_"};
    return prefix[track.kind] ? prefix[track.kind]+(track.programId||track.id) : "";
}
