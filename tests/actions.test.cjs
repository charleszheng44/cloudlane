const fs=require('node:fs'), vm=require('node:vm'), assert=require('node:assert/strict');
const models=vm.createContext({});vm.runInContext(fs.readFileSync('qml/Models.js','utf8').replace(/^\.pragma library\s*/,''),models);
const source=fs.readFileSync('qml/Actions.js','utf8').replace(/^\.(?:pragma|import).*$/gm,'');
function fixture(){
 let id=0,uuid=0;const requests=[],loads=[];
 const backend={request:(path,args,mode,cache)=>{requests.push({id:++id,path,args,mode,cache});return id;},state:()=>'',
  newId:()=>String(++uuid),stop:()=>{},load:(...args)=>loads.push(args),setMetadata:()=>{},setSpeed:()=>{},saveAccount:()=>{},logout:()=>{},
  restoreAccount:()=>{},retryLogin:()=>{},startLogin:()=>{},cancelLogin:()=>{},qrImage:t=>t,position:19,loaded:true,playing:true,storeLocal:()=>{},setState:()=>{},seek:()=>{},setPaused:()=>{}};
 const app={backend,page:'Home',nav:'Home',category:'',items:[],resource:{},viewKind:'cards',viewGeneration:0,playbackGeneration:0,
  profile:{},queue:[],queueIndex:-1,currentTrack:{},likedIds:[],quality:'standard',lyrics:[],comments:[],contextTrack:{},commentGeneration:0,commentPage:1,commentSort:99,
  panel:'',fm:false,videoSession:false,podcastSpeed:1,repeatMode:0,localTracks:[],scrollPosition:0,openLogin:()=>{},closeLogin:()=>{}};
 const actions=vm.createContext({Models:models,console});vm.runInContext(source,actions);actions.init(app);
 return {app,backend,requests,loads,actions,reply:(request,data,error='')=>actions.response(request.id,data,error)};
}
let count=0;function test(name,fn){fn();console.log('PASS '+name);count++;}
const song=id=>({id,kind:'song',name:'Song '+id,artist:'',album:'',duration:10000});
test('navigation discards a stale search response',()=>{let f=fixture();f.actions.search('old',0);let old=f.requests.at(-1);f.actions.search('new',0);let current=f.requests.at(-1);
 f.reply(current,{code:200,result:{songs:[song('2')]}});f.reply(old,{code:200,result:{songs:[song('1')]}});assert.equal(f.app.items[0].id,'2');});
test('a late playback grant cannot replace a newer song',()=>{let f=fixture();f.actions.play(song('1'),false);let old=f.requests.at(-1);f.actions.play(song('2'),false);let current=f.requests.at(-1);
 f.reply(old,{code:200,data:[{url:'https://cdn/old',type:'mp3'}]});assert.equal(f.loads.length,0);f.reply(current,{code:200,data:[{url:'https://cdn/new',type:'mp3'}]});assert.equal(f.loads[0][0],'https://cdn/new');});
test('trial start/end are passed to the native player',()=>{let f=fixture();f.actions.play(song('1'),false);f.reply(f.requests.at(-1),{code:200,data:[{url:'https://cdn/trial',freeTrialInfo:{start:40,end:70}}]});assert.equal(f.loads[0][1],70);assert.equal(f.loads[0][2],40);});
test('null playback grants remain unavailable',()=>{let f=fixture();f.actions.play(song('1'),false);f.reply(f.requests.at(-1),{code:200,data:[{url:null,code:404}]});assert.equal(f.loads.length,0);assert.match(f.app.playerStatus,/unavailable/);});
test('login delegates polling and cancellation to the native worker',()=>{let f=fixture(),started=0,cancelled=0;f.backend.startLogin=()=>started++;f.backend.cancelLogin=()=>cancelled++;f.actions.login();f.actions.cancelLogin();assert.equal(started,1);assert.equal(cancelled,1);});
test('native account confirmation closes login and loads the library',()=>{let f=fixture(),closed=0;f.app.closeLogin=()=>closed++;f.actions.acceptAccount({userId:42,nickname:'Fixture'},true);
 assert.equal(closed,1);assert.equal(f.app.profile.userId,42);assert.equal(f.app.nav,'Your Library');assert.equal(f.requests.at(-1).path,'/api/user/playlist');assert.equal(f.requests.at(-1).args.uid,'42');
 f.reply(f.requests.at(-1),{code:200,playlist:[{id:1,name:'Fixture list'}]});assert.equal(f.app.items.length,1);assert.equal(f.app.libraryPlaylists.length,1);});
test('logout drops outstanding account requests and queue',()=>{let f=fixture();f.app.profile={userId:1};f.actions.search('song',0);let old=f.requests.at(-1);f.app.queue=[song('1')];f.actions.logout();f.reply(old,{code:200,result:{songs:[song('secret')]}});assert.equal(f.app.queue.length,0);assert.equal(f.app.items.length,0);});
test('comment responses stay bound to their target resource',()=>{let f=fixture();f.actions.comments(song('1'),false);let old=f.requests.at(-1);f.actions.comments(song('2'),false);let current=f.requests.at(-1);
 f.reply(current,{code:200,data:{comments:[{content:'two'}]}});f.reply(old,{code:200,data:{comments:[{content:'one'}]}});assert.equal(f.app.comments[0].content,'two');assert.equal(f.app.contextTrack.id,'2');});
test('leaving private FM restores the old queue paused',()=>{let f=fixture();let old=Object.assign(song('1'),{entryId:'a'});f.app.profile={userId:1};f.app.currentTrack=old;f.app.queue=[old];f.app.queueIndex=0;
 f.actions.fm();f.reply(f.requests.at(-1),{code:200,data:[song('2')]});f.actions.leaveFm();let request=f.requests.at(-1);f.reply(request,{code:200,data:[{url:'https://cdn/restored'}]});assert.equal(f.app.currentTrack.id,'1');assert.equal(f.loads.at(-1)[3],true);assert.equal(f.app.resumeAt,19);});
test('leaving video restores music paused',()=>{let f=fixture();let old=Object.assign(song('1'),{entryId:'a'});f.app.currentTrack=old;f.app.queue=[old];f.app.queueIndex=0;
 f.actions.playVideo({id:'9',name:'video',kind:'mv'});f.actions.leaveVideo();f.reply(f.requests.at(-1),{code:200,data:[{url:'https://cdn/restored'}]});assert.equal(f.app.currentTrack.id,'1');assert.equal(f.loads.at(-1)[3],true);});
test('browse requests opt into caching while media grants do not',()=>{let f=fixture();f.actions.search('test',0);assert.equal(f.requests.at(-1).cache,true);f.actions.play(song('1'),false);assert.equal(f.requests.at(-1).cache,false);});
test('service failures stay English without changing service content',()=>{let f=fixture();
 assert.match(f.actions.failure({code:500,message:'服务异常'},''),/code 500/);
 assert.equal(f.actions.failure({code:301},''),'Please sign in again.');
 assert.equal(f.actions.failure({code:403,message:'Access denied'},''),'Access denied');
 assert.equal(f.actions.failure({code:200},''),'');});
console.log(`${count} action checks passed.`);
