'use strict';
// Local game-derived packs stay in build/resources, outside source and public archives.
const fs=require('node:fs/promises'),path=require('node:path'),zlib=require('node:zlib');
const {libraries,identify,digest,regular,readResource}=require('./maps.cjs');
async function embed(target,home){
 const root=path.resolve(__dirname,'../..'),packs=[];
 for(const game of ['ats','ets2']){
  const source=path.join(root,'build',game+'-scene.json');let map;try{map=JSON.parse(await regular(source));}catch(error){if(error.code==='ENOENT')continue;throw error;}
  if(map.game!==game||map.version!==2||!/^\d+\.\d+\.\d+\.\d+$/.test(map.gameVersion||''))throw Error('Map must contain its extracted gameVersion: '+source);
  let directory;for(const library of await libraries(home)){const candidate=path.join(library,'steamapps','common',game==='ats'?'American Truck Simulator':'Euro Truck Simulator 2');try{await identify(candidate);directory=candidate;break;}catch{}}
  if(!directory)throw Error('Cannot verify the game installation for '+game);
  // Check extracted version against the current game's log before binding archive identity.
  const id=game==='ats'?'270880':'227300';const steamapps=path.dirname(path.dirname(directory));const log=path.join(steamapps,'compatdata',id,'pfx','drive_c','users','steamuser','Documents',path.basename(directory),'game.log.txt');
  const versionResource=await readResource(path.join(directory,'version.scs'),1024*1024),logResource=await readResource(log,16*1024*1024);if(versionResource.stat.mtimeMs>logResource.stat.mtimeMs)throw Error('Run the updated game once before embedding its map');
  const text=logResource.data.toString('utf8');const match=text.match(/Loaded pack set version (\d+\.\d+\.\d+\.\d+)/);if(match?.[1]!==map.gameVersion)throw Error('Extracted map does not match the installed game log');
  const identity=await identify(directory),data=Buffer.from(JSON.stringify(map)),compressed=zlib.gzipSync(data,{level:9}),file=game+'-'+map.gameVersion+'.json.gz';
  await fs.mkdir(path.join(target,'maps'),{recursive:true});await fs.writeFile(path.join(target,'maps',file),compressed);
  packs.push({game,gameVersion:map.gameVersion,file,bytes:data.length,sha256:digest(compressed),...identity});
 }
 await fs.mkdir(path.join(target,'maps'),{recursive:true});await fs.writeFile(path.join(target,'maps','manifest.json'),JSON.stringify({format:1,packs},null,2));return packs;
}
module.exports={embed};
