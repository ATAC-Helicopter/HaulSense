'use strict';
const path=require('node:path'),fs=require('node:fs/promises');
(async()=>{
 const {packager}=await import('@electron/packager');
 const output=path.resolve(__dirname,'../../build/desktop');
 const {flipFuses,FuseVersion,FuseV1Options}=await import('@electron/fuses');
 const paths=await packager({dir:__dirname,out:output,name:'HaulSense',executableName:'haulsense-app',platform:'linux',arch:'x64',electronVersion:require('./package.json').devDependencies.electron,overwrite:true,asar:true,prune:true,ignore:[/^\/test(?:\/|$)/,/^\/package\.cjs$/,/^\/node_modules$/]});
 for(const directory of paths){await flipFuses(path.join(directory,'haulsense-app'),{version:FuseVersion.V1,[FuseV1Options.RunAsNode]:false,[FuseV1Options.EnableNodeOptionsEnvironmentVariable]:false,[FuseV1Options.EnableNodeCliInspectArguments]:false,[FuseV1Options.OnlyLoadAppFromAsar]:true});const target=path.join(directory,'resources','daemon');await fs.mkdir(target,{recursive:true});await fs.copyFile(path.resolve(__dirname,'../../build/ats-dualsense/daemon/haulsense'),path.join(target,'haulsense'));await fs.chmod(path.join(target,'haulsense'),0o755);await fs.copyFile(path.resolve(__dirname,'../../LICENSE'),path.join(directory,'HAULSENSE-LICENSE'));}
 console.log(paths.join('\n'));
})().catch(e=>{console.error(e.message);process.exitCode=1;});
