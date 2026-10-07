'use strict';
const {app,BrowserWindow,Menu,protocol,session,dialog,net}=require('electron');
const {spawn}=require('node:child_process');
const fs=require('node:fs'),path=require('node:path'),{Readable}=require('node:stream');
const {ORIGIN,allowedURL,allowedPath}=require('./policy.cjs');
const demo=process.argv.includes('--demo');
const service=demo?'http://127.0.0.1:39076':'http://127.0.0.1:39056';let window,ownedDaemon;
protocol.registerSchemesAsPrivileged([{scheme:'haulsense',privileges:{standard:true,secure:true,supportFetchAPI:true,stream:true}}]);
app.setName('HaulSense');
app.setPath('userData',path.join(app.getPath('appData'),'haulsense'));
if(!app.isPackaged&&process.env.HAULSENSE_TEST_PROFILE)app.setPath('userData',process.env.HAULSENSE_TEST_PROFILE);
if(!app.requestSingleInstanceLock()){app.quit();}else{
 app.on('second-instance',()=>{if(window){if(window.isMinimized())window.restore();window.show();window.focus();}});
 app.whenReady().then(start).catch(e=>{dialog.showErrorBox('HaulSense could not start',e.message);app.quit();});
}
async function probe(){try{const r=await net.fetch(service+'/api/state',{signal:AbortSignal.timeout(1000)});const data=await r.json();if(r.ok&&data.name==='HaulSense'&&data.version!==require('./package.json').version)throw Error('The running HaulSense service needs an upgrade. Install the matching daemon and restart haulsense.service.');return r.ok&&data.name==='HaulSense'&&(!demo||data.demo===true);}catch(error){if(error.message?.includes('needs an upgrade'))throw error;return false;}}
async function ensureService(){
 if(await probe())return;
 const executable=app.isPackaged?path.join(process.resourcesPath,'daemon','haulsense'):path.resolve(__dirname,'../../build/ats-dualsense/daemon/haulsense');
 if(!fs.existsSync(executable))throw Error('Build or install the HaulSense daemon first. See the README.');
 ownedDaemon=spawn(executable,demo?['--mock','--no-controller','--dashboard-port','39076','--telemetry-port','39075','--data-dir',path.join(app.getPath('userData'),'demo-jobs')]:[],{stdio:'ignore',shell:false});let failure;
 ownedDaemon.on('error',e=>{failure=e;});
 for(let i=0;i<50;i++){if(failure)throw failure;if(ownedDaemon.exitCode!==null)break;if(await probe())return;await new Promise(resolve=>setTimeout(resolve,100));}
 throw Error('The local service is unavailable, or port 39056 belongs to another application.');
}
async function start(){
 await ensureService();
 const ses=session.fromPartition('persist:haulsense');
 ses.setPermissionRequestHandler((_contents,_permission,callback)=>callback(false));ses.setPermissionCheckHandler(()=>false);
 ses.webRequest.onBeforeRequest((details,callback)=>callback({cancel:!allowedURL(details.url)}));
 ses.protocol.handle('haulsense',async request=>{
  const url=new URL(request.url);if(!allowedURL(request.url)||url.search||!allowedPath(url.pathname,request.method))return new Response('Not found',{status:404});
  if(url.pathname==='/api/map'){
   const dataHome=process.env.XDG_DATA_HOME||path.join(app.getPath('home'),'.local','share');
   const map=path.join(dataHome,'haulsense','maps','ats.json');
   try{const stat=await fs.promises.lstat(map);if(!stat.isFile()||stat.size>96*1024*1024)throw Error('Invalid map');return new Response(Readable.toWeb(fs.createReadStream(map)),{headers:{'Content-Type':'application/json','Cache-Control':'no-store'}});}catch{return new Response('No installed map',{status:404});}
  }
  const headers={};if(request.method==='POST'){headers['X-HaulSense']='1';headers['Content-Type']='application/x-www-form-urlencoded';headers.Origin=service;}
  try{return await net.fetch(service+url.pathname,{method:request.method,headers,body:request.method==='POST'?await request.text():undefined,signal:AbortSignal.timeout(3000)});}catch{return new Response('Local service unavailable',{status:503});}
 });
 const preferences=path.join(app.getPath('userData'),'window.json');let bounds={width:1500,height:980};
 try{const saved=JSON.parse(await fs.promises.readFile(preferences,'utf8'));if(Number.isFinite(saved.width)&&Number.isFinite(saved.height))bounds={width:Math.max(900,Math.min(2200,saved.width)),height:Math.max(650,Math.min(1600,saved.height))};}catch{}
 window=new BrowserWindow({...bounds,minWidth:900,minHeight:650,title:'HaulSense',backgroundColor:'#101516',show:false,webPreferences:{session:ses,nodeIntegration:false,contextIsolation:true,sandbox:true,webSecurity:true,webviewTag:false,backgroundThrottling:false}});
 window.webContents.setWindowOpenHandler(({url})=>allowedURL(url)&&new URL(url).pathname==='/hud'?{action:'allow',overrideBrowserWindowOptions:{width:520,height:340,title:'HaulSense HUD',backgroundColor:'#101516',webPreferences:{session:ses,nodeIntegration:false,contextIsolation:true,sandbox:true,webSecurity:true,webviewTag:false,backgroundThrottling:false}}}:{action:'deny'});
 window.webContents.on('did-create-window',child=>{child.webContents.setWindowOpenHandler(()=>({action:'deny'}));child.webContents.on('will-navigate',(event,url)=>{if(!allowedURL(url))event.preventDefault();});});
 window.webContents.on('will-navigate',(event,url)=>{if(!allowedURL(url))event.preventDefault();});
 window.webContents.on('will-attach-webview',event=>event.preventDefault());
 window.webContents.on('select-bluetooth-device',(event)=>event.preventDefault());
 ses.on('will-download',async(event,item)=>{
  // Exports are generated locally; arbitrary remote downloads are rejected.
  if(!item.getURL().startsWith('blob:haulsense://app/')){event.preventDefault();return;}
  item.pause();const selected=await dialog.showSaveDialog(window,{title:'Export job report',defaultPath:'haulsense-job.json',filters:[{name:'Job report',extensions:['json']}]});
  if(selected.canceled){item.cancel();return;}item.setSavePath(selected.filePath);item.resume();
 });
 Menu.setApplicationMenu(Menu.buildFromTemplate([{label:'HaulSense',submenu:[{label:'Reload cockpit',accelerator:'CmdOrCtrl+R',click:()=>window.reload()},{label:'Compact HUD',click:()=>window.loadURL(ORIGIN+'/hud')},{label:'Cockpit',click:()=>window.loadURL(ORIGIN+'/')},{type:'separator'},{role:'quit'}]},{label:'View',submenu:[{role:'togglefullscreen'},{role:'resetZoom'},{role:'zoomIn'},{role:'zoomOut'}]}]));
 window.once('ready-to-show',()=>window.show());
 window.on('close',()=>{try{fs.writeFileSync(preferences,JSON.stringify(window.getBounds()),{mode:0o600});}catch{}});
 await window.loadURL(ORIGIN+'/');
}
app.on('window-all-closed',()=>app.quit());
app.on('before-quit',()=>{if(ownedDaemon&&ownedDaemon.exitCode===null)ownedDaemon.kill('SIGTERM');});
