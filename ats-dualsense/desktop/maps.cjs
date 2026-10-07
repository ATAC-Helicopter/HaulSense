'use strict';
// Trusted application resources only. Renderer input never supplies filesystem paths.
const fs=require('node:fs/promises'),path=require('node:path'),crypto=require('node:crypto'),zlib=require('node:zlib');
const GAMES={ats:{id:'270880',directory:'American Truck Simulator'},ets2:{id:'227300',directory:'Euro Truck Simulator 2'}};
const MAX=96*1024*1024;
const digest=data=>crypto.createHash('sha256').update(data).digest('hex');
async function readResource(file,max=MAX){
 const handle=await fs.open(file,fs.constants.O_RDONLY|fs.constants.O_NOFOLLOW);
 try{const stat=await handle.stat();if(!stat.isFile()||stat.size>max)throw Error('Invalid application resource');const data=Buffer.alloc(stat.size);let offset=0;while(offset<data.length){const {bytesRead}=await handle.read(data,offset,data.length-offset,offset);if(!bytesRead)throw Error('Resource changed during read');offset+=bytesRead;}const after=await handle.stat();if(after.size!==stat.size||after.mtimeMs!==stat.mtimeMs)throw Error('Resource changed during read');return {data,stat};}finally{await handle.close();}
}
async function regular(file,max=MAX){return (await readResource(file,max)).data;}
async function libraries(home){
 const roots=[path.join(home,'.steam','steam'),path.join(home,'.local','share','Steam'),path.join(home,'.var','app','com.valvesoftware.Steam','data','Steam')];const result=new Set();
 for(const root of roots){try{const vdf=await fs.readFile(path.join(root,'steamapps','libraryfolders.vdf'),'utf8');result.add(root);for(const m of vdf.matchAll(/"path"\s+"([^"\r\n]+)"/g))if(path.isAbsolute(m[1]))result.add(m[1].replace(/\\\\/g,'\\'));}catch{}}
 return [...result];
}
async function identify(directory){
 const versionSha256=digest(await regular(path.join(directory,'version.scs'),1024*1024));
 const archives=[];for(const name of (await fs.readdir(directory)).filter(n=>/^[a-z0-9_]+\.scs$/i.test(n)).sort()){const s=await fs.lstat(path.join(directory,name));if(!s.isFile())throw Error('Invalid game archive');archives.push([name,s.size]);}
 return {versionSha256,archiveSignature:digest(JSON.stringify(archives))};
}
function compatible(pack,identity){return pack.versionSha256===identity.versionSha256&&pack.archiveSignature===identity.archiveSignature;}
class EmbeddedMaps{
 constructor(resources,home){this.resources=resources;this.home=home;this.checked=new Map();}
 async init(){try{this.manifest=JSON.parse(await regular(path.join(this.resources,'maps','manifest.json'),1024*1024));if(this.manifest.format!==1||!Array.isArray(this.manifest.packs))throw Error('Invalid map catalog');}catch{this.manifest={format:1,packs:[]};}this.roots=await libraries(this.home);}
 async status(game='ats'){
  if(!GAMES[game])game='ats';const packs=this.manifest.packs.filter(p=>p.game===game);let installation;
  for(const root of this.roots){const directory=path.join(root,'steamapps','common',GAMES[game].directory);try{const identity=await identify(directory);installation={directory,identity};break;}catch{}}
  const pack=installation&&packs.find(p=>compatible(p,installation.identity));
  const state={game,ready:!!pack,version:pack?.gameVersion||null,reason:!installation?'game-not-installed':!packs.length?'map-not-bundled':!pack?'version-mismatch':'ready',provider:'waiting',message:!installation?'Steam game installation not found.':!packs.length?'This app has no embedded map for '+game.toUpperCase()+'.':!pack?'Game content changed. A matching app map pack is required; incompatible roads are disabled.':'Embedded '+game.toUpperCase()+' '+pack.gameVersion+' map · verified against installed game archives'};
  if(pack&&pack.gameVersion.startsWith('1.61.')){try{state.provider=await this.provision(installation.directory);}catch{state.provider='install-failed';}}
  if(pack&&!pack.gameVersion.startsWith('1.61.'))state.provider='unsupported-version';
  return {state,pack};
 }
 async provision(directory){
  const source=path.join(this.resources,'providers','ets2la_plugin.dll');const expected='0e1893719f84f28079857451b82a22a263401d98b8fc299cd561eb7695bfa88f';
  const data=await regular(source,1024*1024);if(digest(data)!==expected)throw Error('Invalid provider checksum');
  const plugins=path.join(directory,'bin','win_x64','plugins');await fs.mkdir(plugins,{recursive:true});if(!(await fs.lstat(plugins)).isDirectory())throw Error('Invalid plugins directory');const target=path.join(plugins,'ets2la_plugin.dll');
  try{if(digest(await regular(target,1024*1024))===expected){const status=this.checked.get(directory)||'installed';this.checked.set(directory,status);return status;}await fs.copyFile(target,target+'.backup-'+Date.now(),fs.constants.COPYFILE_EXCL);}catch(error){if(error.code!=='ENOENT')throw error;}
  const temporary=path.join(plugins,'.haulsense-provider-'+crypto.randomBytes(8).toString('hex'));try{await fs.writeFile(temporary,data,{mode:0o644,flag:'wx'});await fs.rename(temporary,target);}finally{await fs.unlink(temporary).catch(()=>{});}
  this.checked.set(directory,'restart-required');return 'restart-required';
 }
 async read(game){const {state,pack}=await this.status(game);if(!state.ready)throw Error(state.message);if(!/^(ats|ets2)-[0-9.]+\.json\.gz$/.test(pack.file))throw Error('Invalid map pack filename');const bytes=await regular(path.join(this.resources,'maps',pack.file));if(digest(bytes)!==pack.sha256)throw Error('Map pack checksum mismatch');const data=zlib.gunzipSync(bytes,{maxOutputLength:MAX});if(data.length!==pack.bytes)throw Error('Map pack size mismatch');return data;}
}
module.exports={EmbeddedMaps,libraries,identify,compatible,digest,regular,readResource};
