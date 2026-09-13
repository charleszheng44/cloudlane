const fs = require('node:fs');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const models = vm.createContext({});
vm.runInContext(fs.readFileSync('qml/Models.js','utf8').replace(/^\.pragma library\s*/,''),models);
let count=0;
function test(name, fn) { fn(); console.log('PASS '+name); count++; }
function native(v) { return JSON.parse(JSON.stringify(v)); }
test('same song has distinct queue entries',()=>{
 let id=0; const q=models.makeQueue([{id:'12'},{id:'12'}],()=>String(++id));
 assert.equal(q[0].id,q[1].id);assert.notEqual(q[0].entryId,q[1].entryId);
});
test('queue movement preserves duplicates and current entry identity',()=>{
 const q=[{entryId:'a',id:'12'},{entryId:'b',id:'12'},{entryId:'c',id:'13'}];
 assert.deepEqual(native(models.moveEntry(q,0,2)).map(t=>t.entryId),['b','c','a']);assert.equal(q[0].entryId,'a');
});
test('shuffle leaves played history and current entry in place',()=>{
 const q=Array.from({length:8},(_,i)=>({entryId:String(i)}));const shuffled=native(models.shuffleAfter(q,2,()=>.1));
 assert.deepEqual(shuffled.slice(0,3),q.slice(0,3));assert.deepEqual(shuffled.map(t=>t.entryId).sort(),q.map(t=>t.entryId).sort());
});
test('lyrics parse repeated timestamps and fractional seconds',()=>{
 const l=native(models.parseLyrics('[ar:test]\n[00:12.5][01:00.25]你好\n[00:01.00]first'));
 assert.deepEqual(l,[{time:1,text:'first'},{time:12.5,text:'你好'},{time:60.25,text:'你好'}]);
});
test('lyric seeking and translation do not borrow a neighboring line',()=>{
 const lines=[{time:1,text:'a'},{time:10,text:'b'}];assert.equal(models.lyricIndex(lines,0),-1);assert.equal(models.lyricIndex(lines,10),1);
 const merged=native(models.mergeLyrics(lines,[{time:1.03,text:'译'}]));assert.equal(merged[0].translation,'译');assert.equal(merged[1].translation,'');
});
test('podcast comments use program id, playback uses song id',()=>{
 const episode=models.resource({id:42,name:'episode',mainSong:{id:123,name:'audio'}},'episode');
 assert.equal(episode.id,'123');assert.equal(models.commentThread(episode),'A_DJ_1_42');
});
test('typed resources cannot collide in comment targets',()=>{
 assert.notEqual(models.commentThread({kind:'song',id:'1'}),models.commentThread({kind:'playlist',id:'1'}));
 assert.equal(models.commentThread({kind:'local',id:'file:///a'}),'');
});
test('music names and identifiers survive normalization',()=>{
 const song=models.song({id:'9007199254740993',name:'晴天',ar:[{id:1,name:'周杰伦'}],al:{id:2,name:'叶惠美'},dt:12345});
 assert.equal(song.id,'9007199254740993');assert.equal(song.artist,'周杰伦');assert.equal(song.albumId,'2');
});
console.log(`${count} model checks passed.`);
