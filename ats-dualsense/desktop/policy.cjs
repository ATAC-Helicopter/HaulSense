'use strict';
const ORIGIN='haulsense://app';
function allowedURL(value){try{const u=new URL(value);return u.protocol==='haulsense:'&&u.hostname==='app'&&!u.username&&!u.password&&!u.port;}catch{return false;}}
function allowedPath(path,method='GET'){
 if(method==='POST')return path==='/api/config';
 return method==='GET'&&(['/','/hud','/navigation.js','/scene.js','/routing-worker.js','/dashboard.js','/hud.js','/persistence.js','/jobs.js','/api/state','/api/config','/api/route','/api/signals','/api/jobs','/api/map'].includes(path)||/^\/api\/jobs\/\d{1,24}$/.test(path));
}
module.exports={ORIGIN,allowedURL,allowedPath};
