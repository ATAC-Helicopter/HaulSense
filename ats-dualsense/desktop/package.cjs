'use strict';
const path=require('node:path'),fs=require('node:fs/promises'),os=require('node:os');
const localMaps=process.argv.includes('--local-maps');
(async()=>{
 const provider=path.resolve(__dirname,'../../build/ets2la-provider/ets2la_plugin.dll');
 const expected='0e1893719f84f28079857451b82a22a263401d98b8fc299cd561eb7695bfa88f';let providerBytes;
 try{providerBytes=await fs.readFile(provider);}catch(error){if(error.code!=='ENOENT')throw error;const response=await fetch('https://raw.githubusercontent.com/ETS2LA/ETS2LA/41945bc41e36191fbdf277c7fa6f7dd6f101d679/Assets/SDKs/1.61/Windows/ets2la_plugin.dll',{signal:AbortSignal.timeout(90000)});if(!response.ok||Number(response.headers.get('content-length'))>1024*1024)throw Error('Provider download failed');providerBytes=Buffer.from(await response.arrayBuffer());}
 if(providerBytes.length>1024*1024||require('./maps.cjs').digest(providerBytes)!==expected)throw Error('Provider checksum mismatch');
 const {packager}=await import('@electron/packager');
 const output=path.resolve(__dirname,'../../build/desktop');
 const {flipFuses,FuseVersion,FuseV1Options}=await import('@electron/fuses');
 const paths=await packager({dir:__dirname,out:output,name:'HaulSense',executableName:'haulsense-app',platform:'linux',arch:'x64',electronVersion:require('./package.json').devDependencies.electron,overwrite:true,asar:true,prune:true,ignore:[/^\/test(?:\/|$)/,/^\/(?:package|embed-maps)\.cjs$/,/^\/node_modules$/]});
 for(const directory of paths){await flipFuses(path.join(directory,'haulsense-app'),{version:FuseVersion.V1,[FuseV1Options.RunAsNode]:false,[FuseV1Options.EnableNodeOptionsEnvironmentVariable]:false,[FuseV1Options.EnableNodeCliInspectArguments]:false,[FuseV1Options.OnlyLoadAppFromAsar]:true});const target=path.join(directory,'resources','daemon');await fs.mkdir(target,{recursive:true});await fs.copyFile(path.resolve(__dirname,'../../build/ats-dualsense/daemon/haulsense'),path.join(target,'haulsense'));await fs.chmod(path.join(target,'haulsense'),0o755);await fs.copyFile(path.resolve(__dirname,'../../LICENSE'),path.join(directory,'HAULSENSE-LICENSE'));
 if(localMaps)await require('./embed-maps.cjs').embed(path.join(directory,'resources'),os.homedir());
 const destination=path.join(directory,'resources','providers');await fs.mkdir(destination,{recursive:true});await fs.writeFile(path.join(destination,'ets2la_plugin.dll'),providerBytes);await fs.copyFile(path.resolve(__dirname,'../third_party/ets2la-provider/LICENSE'),path.join(destination,'LICENSE'));
 }

 console.log(paths.join('\n'));
})().catch(e=>{console.error(e.message);process.exitCode=1;});
