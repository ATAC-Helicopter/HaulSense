'use strict';
const {test}=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs/promises'),os=require('node:os'),path=require('node:path'),zlib=require('node:zlib');
const {EmbeddedMaps,identify,digest,libraries}=require('../maps.cjs');
test('embedded map selects the installed game and refuses updated or missing archives',async()=>{
 const temp=await fs.mkdtemp(path.join(os.tmpdir(),'haulsense-map-'));try{
 const home=path.join(temp,'home'),library=path.join(temp,'library'),resources=path.join(temp,'resources'),game=path.join(library,'steamapps','common','American Truck Simulator');
 await fs.mkdir(path.join(home,'.steam','steam','steamapps'),{recursive:true});await fs.mkdir(game,{recursive:true});await fs.mkdir(path.join(resources,'maps'),{recursive:true});
 await fs.writeFile(path.join(home,'.steam','steam','steamapps','libraryfolders.vdf'),'"path" "'+library+'"');await fs.writeFile(path.join(game,'version.scs'),'1.61');await fs.writeFile(path.join(game,'base_map.scs'),'roads');
 assert((await libraries(home)).includes(library));const data=Buffer.from(JSON.stringify({game:'ats',gameVersion:'1.61.3.1',roads:[]})),gz=zlib.gzipSync(data),file='ats-1.61.3.1.json.gz';
 const pack={game:'ats',gameVersion:'1.61.3.1',file,bytes:data.length,sha256:digest(gz),...await identify(game)};
 await fs.writeFile(path.join(resources,'maps',file),gz);await fs.writeFile(path.join(resources,'maps','manifest.json'),JSON.stringify({format:1,packs:[pack]}));const maps=new EmbeddedMaps(resources,home);await maps.init();
 assert.equal((await maps.status('ats')).state.ready,true);assert.deepEqual(await maps.read('ats'),data);assert.equal((await maps.status('ets2')).state.ready,false);
 await fs.writeFile(path.join(resources,'maps',file),'tampered');await assert.rejects(maps.read('ats'),/checksum/);await fs.writeFile(path.join(resources,'maps',file),gz);
 await fs.rename(path.join(resources,'maps',file),path.join(resources,'maps','real.gz'));await fs.symlink('real.gz',path.join(resources,'maps',file));await assert.rejects(maps.read('ats'),{code:'ELOOP'});await fs.unlink(path.join(resources,'maps',file));await fs.rename(path.join(resources,'maps','real.gz'),path.join(resources,'maps',file));
 await fs.writeFile(path.join(game,'version.scs'),'1.62');assert.equal((await maps.status('ats')).state.reason,'version-mismatch');await assert.rejects(maps.read('ats'),/changed/);
 await fs.writeFile(path.join(game,'version.scs'),'1.61');await fs.writeFile(path.join(game,'dlc_new.scs'),'new DLC');assert.equal((await maps.status('ats')).state.reason,'version-mismatch');
 await fs.unlink(path.join(game,'dlc_new.scs'));await fs.unlink(path.join(game,'base_map.scs'));assert.equal((await maps.status('ats')).state.ready,false);
 }finally{await fs.rm(temp,{recursive:true,force:true});}
});
