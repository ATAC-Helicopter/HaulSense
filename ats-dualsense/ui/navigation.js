/* Local SCS world-coordinate navigator. No guessed roads or GPS instructions. */
'use strict';
function createNavigator({document,window}) {
 const el=id=>document.getElementById(id), text=(id,v)=>{el(id).textContent=v;};
 const finite=v=>typeof v==='number'&&Number.isFinite(v), validPoint=p=>Array.isArray(p)&&p.length===2&&p.every(v=>finite(v)&&Math.abs(v)<1e8);
 let perspective=true,north=false,zoom=1,roads=null,trace=[],last=null,baseOdometer=null,previousOdometer=null,driven=0,lastSequence=null,lastDemo=null,lastGame=null;
 let signals=[],signalPolling=false,lastSignalPoll=0,signalExpiry=0,mapRevision=0;
 let current=null,metric=true,mapError='',scene=null,sceneFailed=false;
 let routeWorker=null,routeReady=false,routePoints=[],routeRequest=0,routePending=false,routeGoal=null,lastRoutePosition=null,lastRouteAt=0,routeSource=null;
 let gpsPolling=false,lastGpsPoll=0,gpsUntil=0,gpsKey='',gpsFailedKey='',pendingKind='';
 const layers={props:true,signs:true,details:true};
 const texts=(id,value)=>{const e=el(id);if(e)e.textContent=value;};
 const limit={bytes:96*1024*1024,roads:150000,points:1500000,labels:2000};
 function validateMap(data) {
  if(![1,2].includes(data?.version)||!['ats','ets2'].includes(data.game)||data.coordinates!=='scs-xz-metres'||!Array.isArray(data.roads)||data.roads.length>limit.roads)throw Error('Expected a HaulSense ATS / ETS2 map in SCS world coordinates.');
  let points=0;
  for(const road of data.roads){if(!Array.isArray(road)||road.length<2||(points+=road.length)>limit.points||!road.every(p=>Array.isArray(p)&&[2,3].includes(p.length)&&p.every(v=>finite(v)&&Math.abs(v)<1e8)))throw Error('Invalid road coordinates or map exceeds point limit.');}
  const labels=data.labels??[];if(!Array.isArray(labels)||labels.length>limit.labels||labels.some(l=>!validPoint(l.position)||typeof l.name!=='string'||l.name.length>100))throw Error('Invalid map labels.');
  if(data.version===2){
   const numeric=(v,max=1e8)=>finite(v)&&Math.abs(v)<max;
   const vector=(p,n)=>Array.isArray(p)&&p.length===n&&p.every(v=>numeric(v));
   const index=(i,length)=>Number.isInteger(i)&&i>=0&&i<length;
   const arrays={templates:3000,prefabs:150000,assets:10000,objects:400000,signDefinitions:10000,signs:200000,barriers:10000,pois:30000};
   for(const [key,max]of Object.entries(arrays))if(!Array.isArray(data[key])||data[key].length>max)throw Error('Invalid or excessive '+key+' layer.');
   let total=0;
   for(const t of data.templates){
    for(const key of ['nodes','lines','areas','curves','links','signs','signals'])if(!Array.isArray(t[key])||t[key].length>20000)throw Error('Invalid prefab template.');
    if(t.nodes.some(p=>!vector(p,4)))throw Error('Invalid prefab nodes.');
    for(const line of t.lines)if(!Array.isArray(line)||line.length!==5||!vector(line[0],3)||!vector(line[1],3)||line.slice(2,4).some(v=>v!==null&&(!Number.isInteger(v)||v<0||v>32))||!numeric(line[4],1000))throw Error('Invalid prefab road.');
    for(const area of t.areas)if(!Array.isArray(area)||!Number.isInteger(area[0])||!Array.isArray(area[1])||area[1].length<3||area[1].some(p=>!vector(p,3)))throw Error('Invalid prefab surface.');
    for(const curve of t.curves){total+=curve.length;if(total>1000000||!Array.isArray(curve)||curve.length<2||curve.some(p=>!vector(p,3)))throw Error('Invalid prefab curve.');}
    for(const link of t.links)if(!Array.isArray(link)||link.length!==4||!index(link[0],t.nodes.length)||!index(link[1],t.nodes.length)||!numeric(link[2])||link[2]<=0||!Array.isArray(link[3])||link[3].length>20000||link[3].some(i=>!index(i,t.curves.length)))throw Error('Invalid prefab routing.');
    for(const sign of t.signs)if(!Array.isArray(sign)||sign.length!==6||!vector(sign.slice(0,4),4)||sign.slice(4).some(v=>typeof v!=='string'||v.length>180))throw Error('Invalid prefab sign.');
    for(const signal of t.signals)if(!Array.isArray(signal)||signal.length!==5||!vector(signal.slice(0,4),4)||typeof signal[4]!=='string'||signal[4].length>180)throw Error('Invalid semaphore.');
   }
   for(const a of data.assets)if(!Array.isArray(a)||a.length!==8||typeof a[0]!=='string'||typeof a[1]!=='string'||a[1].length>256||!['building','prop','vegetation'].includes(a[2])||!vector(a.slice(3),5))throw Error('Invalid model bounds.');
   for(const o of data.objects)if(!Array.isArray(o)||o.length!==8||!index(o[0],data.assets.length)||!vector(o.slice(1),7))throw Error('Invalid object placement.');
   for(const d of data.signDefinitions)if(!Array.isArray(d)||d.length!==4||d.some(s=>typeof s!=='string'||s.length>256))throw Error('Invalid sign definition.');
   for(const sign of data.signs)if(!Array.isArray(sign)||sign.length!==6||!vector(sign.slice(0,4),4)||sign[4]!==-1&&!index(sign[4],data.signDefinitions.length)||typeof sign[5]!=='string'||sign[5].length>180)throw Error('Invalid sign placement.');
   const graph=data.graph;if(!graph||!Array.isArray(graph.nodes)||graph.nodes.length>700000||!Array.isArray(graph.edges)||graph.edges.length>1200000)throw Error('Invalid routing graph.');
   const ids=new Set();for(const n of graph.nodes){if(!Array.isArray(n)||n.length!==4||!vector(n.slice(0,3),3)||typeof n[3]!=='string'||!/^[a-f0-9]{1,16}$/.test(n[3])||ids.has(n[3]))throw Error('Invalid graph node.');ids.add(n[3]);}
   for(const edge of graph.edges)if(!Array.isArray(edge)||![3,4].includes(edge.length)||!index(edge[0],graph.nodes.length)||!index(edge[1],graph.nodes.length)||!numeric(edge[2])||edge[2]<0||edge.length===4&&(!Array.isArray(edge[3])||edge[3].length<2||edge[3].length>64||edge[3].some(p=>!vector(p,3))))throw Error('Invalid graph edge.');
   for(const p of data.prefabs)if(!Array.isArray(p)||p.length!==6||!index(p[0],data.templates.length)||!vector(p.slice(1,5),4)||!Array.isArray(p[5])||p[5].length!==data.templates[p[0]].nodes.length||p[5].some(i=>i!==-1&&!index(i,graph.nodes.length)))throw Error('Invalid prefab placement.');
   for(const b of data.barriers)if(!Array.isArray(b)||!Array.isArray(b[0])||b[0].length<2||b[0].some(p=>!vector(p,3))||typeof b[1]!=='string'||b[1].length>256)throw Error('Invalid barrier.');
   for(const p of data.pois)if(!Array.isArray(p)||p.length!==5||!vector(p.slice(0,2),2)||p.slice(2).some(s=>typeof s!=='string'||s.length>180))throw Error('Invalid point of interest.');
   if(data.roadStyles&&(!Array.isArray(data.roadStyles)||data.roadStyles.length!==data.roads.length||data.roadStyles.some(s=>!vector(s,5))))throw Error('Invalid road style.');
  }
  return {...data,name:typeof data.name==='string'?data.name.slice(0,100):data.game.toUpperCase(),labels};
 }
 function reset(){trace=[];last=null;baseOdometer=null;previousOdometer=null;driven=0;lastSequence=null;}
 function formatKm(km,digits=0){return finite(km)?(metric?km:km/1.609344).toFixed(digits)+(metric?' km':' mi'):'—';}
 function update(s,isMetric=true){
  metric=isMetric;current=s;
  const t=s.active?s.telemetry:{},p=t.world_position;
  if(lastDemo!==s.demo||lastGame!==t.game&&s.active){reset();routePoints=[];gpsUntil=0;gpsKey='';gpsFailedKey='';routeRequest++;routePending=false;routeGoal=null;lastDemo=s.demo;if(s.active)lastGame=t.game;}
  const fresh=s.active&&s.sequence!==lastSequence;
  if(fresh){lastSequence=s.sequence;
   if(finite(t.odometer)){
    if(baseOdometer===null||previousOdometer!==null&&t.odometer<previousOdometer){baseOdometer=t.odometer;driven=0;}
    previousOdometer=t.odometer;driven=Math.max(0,t.odometer-baseOdometer);
   }
   if(Array.isArray(p)&&p.length===3&&p.every(v=>finite(v)&&Math.abs(v)<1e8)&&finite(t.heading)){
    const pos=[p[0],p[2]];
    if(last&&Math.hypot(pos[0]-last[0],pos[1]-last[1])>500){trace=[];last=null;}
    if(!last||Math.hypot(pos[0]-last[0],pos[1]-last[1])>=2){trace.push(pos);if(trace.length>1200)trace.shift();last=pos;}
   }else {if(last)trace.push(null);last=null;}
  }
  if(!s.active){if(last)trace.push(null);last=null;lastSequence=null;}
  const positioned=s.active&&Array.isArray(p)&&p.length===3&&p.every(v=>finite(v)&&Math.abs(v)<1e8)&&finite(t.heading);
  const matching=roads&&roads.game===t.game;
  el('map-empty').hidden=positioned;
  text('map-empty-detail',s.active?'World position unavailable. Restart the game with the current plugin.':'Waiting for fresh telemetry. Paused and disconnected positions are cleared.');
  text('map-source',!positioned?'Position unavailable':s.demo?'DEMO · simulated position':matching?roads.name:'Live journey trace');
  const heading=finite(t.heading)?((360-t.heading*360)%360+360)%360:null;
  text('map-heading',positioned?`${Math.round(heading)%360}° · ${north?'N ↑':'Heading ↑'}`:'—');
  if(roads)text('map-note',matching?`${roads.name} · ${roads.roads.length.toLocaleString()} road lines · ${perspective&&roads.version===2?'3D schematic':perspective?'Perspective view':'Top-down view'}. Cyan is travelled history, not the GPS route.`:s.active?`Loaded ${roads.game.toUpperCase()} map does not match ${t.game||'unknown game'}. Roads hidden.`:'Map loaded. Waiting for live game position.');
  else text('map-note','No road map loaded · cyan shows your measured trace. Load a local ATS / ETS2 map for roads and city labels.');
  if(mapError)text('map-note',mapError);
  text('trip-mode',s.demo?'Demo session':'Local session');
  text('journey-distance',formatKm(finite(t.nav_distance)?t.nav_distance/1000:null,1));
  text('journey-eta','Game travel time · '+(finite(t.nav_time)?Math.round(t.nav_time/60)+' min':'—'));
  const margin=finite(t.fuel_range)&&finite(t.nav_distance)&&t.nav_distance>0?t.fuel_range-t.nav_distance/1000:null;
  text('journey-margin',formatKm(margin));text('journey-driven',s.active&&finite(t.odometer)?formatKm(driven,1):'—');
  text('journey-pitch',finite(t.pitch)?(t.pitch*360).toFixed(1)+'° pitch':'—');text('journey-roll','Roll · '+(finite(t.roll)?(t.roll*360).toFixed(1)+'°':'—'));
  text('journey-cargo','Cargo condition · '+(finite(t.cargo_damage)?Math.round((1-Math.min(1,Math.max(0,t.cargo_damage)))*100)+'%':'—'));
  text('journey-trailer',t.trailer_connected===true?'Trailer connected':t.trailer_connected===false?'Trailer detached':'Trailer · —');
  const advice=[];
  if(t.air_emergency)advice.push('Brake air critical — stop safely');
  if(finite(margin)&&margin<0)advice.push('Plan a fuel stop before arrival');
  else if(finite(margin)&&margin<50)advice.push('Small fuel reserve at arrival');
  if(finite(t.nav_speed_limit)&&t.nav_speed_limit>0&&finite(t.speed_mps)&&Math.abs(t.speed_mps)>t.nav_speed_limit+1)advice.push('Above the posted speed limit');
  if(finite(t.cargo_damage)&&t.cargo_damage>.05)advice.push('Cargo damage reported');
  if(t.parking_brake&&finite(t.speed_mps)&&Math.abs(t.speed_mps)>2)advice.push('Release the parking brake');
  if(t.water_warning)advice.push('Coolant warning — check engine temperature');
  text('journey-advice',!s.active?'Waiting for live truck data.':advice.length?advice.join(' · '):margin!==null?'Fuel estimate covers your route. Keep a reserve for detours.':'Choose a GPS route to compare distance with fuel range.');
  el('journey-advice').classList.toggle('attention',s.active&&advice.length>0);
  pollGps();pollSignals();if(Date.now()>signalExpiry)signals=[];maybeRoute();draw();
 }
 function project(x,z,w,h,t){
  const p=t.world_position,theta=north?0:t.heading*2*Math.PI,dx=x-p[0],dz=z-p[2];
  const a=(dx*Math.cos(theta)-dz*Math.sin(theta))*zoom*.32,b=(dx*Math.sin(theta)+dz*Math.cos(theta))*zoom*.32;
  if(!perspective)return [w/2+a,h*.62+b];
  // Pitched ground-plane camera. World north is -Z; clip behind the camera.
  const depth=1-b/650;if(depth<.16)return null;
  return [w/2+a/depth,h*.69+b*.55/depth];
 }
 function draw(){
  const canvas=el('navigation-map'),box=canvas.getBoundingClientRect(),w=box.width,h=box.height||390;
  if(!w)return;const ratio=Math.min(window.devicePixelRatio||1,2);canvas.width=Math.round(w*ratio);canvas.height=Math.round(h*ratio);
  const c=canvas.getContext('2d');if(!c)return;c.scale(ratio,ratio);c.fillStyle='#0b141a';c.fillRect(0,0,w,h);
  const t=current?.active?current.telemetry:null;
  if(!t||!Array.isArray(t.world_position)||t.world_position.length!==3||!t.world_position.every(v=>finite(v)&&Math.abs(v)<1e8)||!finite(t.heading)){el('road-scene').hidden=true;return;}
  const p=t.world_position;
  const visible=roads&&roads.game===t.game?queryScene(roads,p[0],p[2],Math.min(2400/zoom,1400)):null;
  if(perspective&&visible&&roads.version===2&&window.HaulSenseScene&&!sceneFailed){
   try{if(!scene)scene=window.HaulSenseScene(el('road-scene'));el('road-scene').hidden=false;
    scene.update({data:roads,visible,position:p,heading:t.heading,north,zoom,layers,route:routePoints,trace,signals,width:w,height:h});
    texts('scene-status',`${visible.prefabs.length} nearby prefabs · ${visible.objects.length} objects · ${visible.signs.length} signs. Measured bounds; signal phases unknown.`);
    return;
   }catch(error){sceneFailed=true;if(scene){scene.dispose();scene=null;}texts('scene-status','WebGL unavailable · using the 2D/perspective map.');}
  }
  el('road-scene').hidden=true;
  function line(points,color,width){c.beginPath();let pen=false;for(const v of points){if(!v){pen=false;continue;}const q=project(v[0],v[1],w,h,t);if(!q){pen=false;continue;}if(pen)c.lineTo(...q);else{c.moveTo(...q);pen=true;}}c.strokeStyle=color;c.lineWidth=width;c.lineCap='round';c.lineJoin='round';c.stroke();}
  const reach=2400/zoom,step=200;
  for(let x=Math.floor((p[0]-reach)/step)*step;x<p[0]+reach;x+=step)line([[x,p[2]-reach],[x,p[2]+reach]],'#172631',1);
  for(let z=Math.floor((p[2]-reach)/step)*step;z<p[2]+reach;z+=step)line([[p[0]-reach,z],[p[0]+reach,z]],'#172631',1);
  if(roads&&roads.game===t.game){
   const bounds=[p[0]-reach,p[2]-reach,p[0]+reach,p[2]+reach];
   for(let i=0;i<roads.roads.length;i++){const b=roads.bounds[i];if(b[2]<bounds[0]||b[0]>bounds[2]||b[3]<bounds[1]||b[1]>bounds[3])continue;line(roads.roads[i],'#485052',6);line(roads.roads[i],'#d1a64d',2.5);}
   if(visible&&roads.version===2){
    for(const i of visible.prefabs){const instance=roads.prefabs[i],template=roads.templates[instance[0]],cs=Math.cos(instance[4]),sn=Math.sin(instance[4]),tx=p=>[instance[1]+p[0]*cs-p[1]*sn,instance[2]+p[0]*sn+p[1]*cs];for(const l of template.lines)line([tx(l[0]),tx(l[1])],'#9a925c',3);}
    if(layers.props)for(const i of visible.objects.slice(0,1000)){const o=roads.objects[i],q=project(o[1],o[2],w,h,t);if(q){c.fillStyle='#49616e';c.fillRect(q[0]-2,q[1]-2,4,4);}}
    if(layers.details){c.font='11px system-ui';c.fillStyle='#d8dca8';for(const i of visible.pois.slice(0,20)){const poi=roads.pois[i],q=project(poi[0],poi[1],w,h,t);if(q)c.fillText(poi[4]||poi[2],...q);}}
    if(layers.signs){c.font='10px system-ui';c.fillStyle='#9ad6b9';let labels=0;for(const i of visible.signs){const sign=roads.signs[i],q=project(sign[0],sign[1],w,h,t);if(q&&q[0]>0&&q[0]<w&&q[1]>25&&q[1]<h-25){c.fillRect(q[0]-2,q[1]-2,4,4);if(sign[5]&&labels++<12)c.fillText(sign[5].slice(0,45),q[0]+5,q[1]);}}}
   }
   c.font='12px system-ui';c.fillStyle='#c2ccd3';c.textAlign='center';for(const l of roads.labels){if(Math.hypot(l.position[0]-p[0],l.position[1]-p[2])>reach)continue;const q=project(...l.position,w,h,t);if(q&&q[0]>0&&q[0]<w&&q[1]>12&&q[1]<h-20)c.fillText(l.name,...q);}
  }
  if(layers.signs){for(const signal of signals){if(signal.type!==1)continue;const q=project(signal.position[0],signal.position[2],w,h,t);if(!q)continue;c.fillStyle=signal.state===2?'#ff7770':signal.state===8?'#91e598':[1,4].includes(signal.state)?'#ffc577':signal.state===32&&(Math.floor(Date.now()/500)%2)?'#ffc577':'#62747b';c.beginPath();c.arc(q[0],q[1],5,0,Math.PI*2);c.fill();}}
  line(routePoints,'#ff9b65',4);line(trace,'#8ed9e7',3);
  const q=project(p[0],p[2],w,h,t);c.save();c.translate(...q);if(north)c.rotate(-t.heading*2*Math.PI);
  c.shadowColor='#d8ee8680';c.shadowBlur=18;c.fillStyle='#d8ee86';c.strokeStyle='#0b141a';c.lineWidth=2;c.beginPath();c.moveTo(0,-15);c.lineTo(11,12);c.lineTo(0,7);c.lineTo(-11,12);c.closePath();c.fill();c.stroke();c.restore();
  c.font='10px system-ui';c.fillStyle='#8096a4';c.textAlign='left';c.fillText('SCS WORLD · '+(perspective?'PERSPECTIVE':'2D'),16,h-16);
 }
 function loadMap(data){mapRevision++;const candidate=validateMap(data);candidate.bounds=candidate.roads.map(r=>{const b=[Infinity,Infinity,-Infinity,-Infinity];for(const [x,z]of r){b[0]=Math.min(b[0],x);b[1]=Math.min(b[1],z);b[2]=Math.max(b[2],x);b[3]=Math.max(b[3],z);}return b;});buildSceneIndex(candidate);roads=candidate;mapError='';if(scene){scene.dispose();scene=null;}sceneFailed=false;setupRouting();el('map-unload').hidden=false;if(current)update(current,metric);}

 function buildSceneIndex(data){
  const grid=new Map(),size=500;
  const add=(kind,i,x,z)=>{const key=Math.floor(x/size)+','+Math.floor(z/size);if(!grid.has(key))grid.set(key,{});const bucket=grid.get(key);(bucket[kind]??=[]).push(i);};
  if(data.version===2){for(const kind of ['prefabs','objects','signs','barriers','pois']){data[kind].forEach((o,i)=>{const p=kind==='barriers'?o[0][0]:['signs','pois'].includes(kind)?o.slice(0,2):o.slice(1,3);if(kind==='barriers'){for(const point of o[0])add(kind,i,...point.slice(0,2));}else add(kind,i,...p);});}}
  data.sceneGrid=grid;
 }
 function queryScene(data,x,z,reach){
  const result={roads:[],prefabs:[],objects:[],signs:[],barriers:[],pois:[]},bound=[x-reach,z-reach,x+reach,z+reach];
  data.bounds.forEach((b,i)=>{if(!(b[2]<bound[0]||b[0]>bound[2]||b[3]<bound[1]||b[1]>bound[3]))result.roads.push(i);});
  if(data.version!==2)return result;
  for(let i=Math.floor(bound[0]/500);i<=Math.floor(bound[2]/500);i++)for(let j=Math.floor(bound[1]/500);j<=Math.floor(bound[3]/500);j++){
   const bucket=data.sceneGrid.get(i+','+j);if(bucket)for(const kind of ['prefabs','objects','signs','barriers','pois'])for(const index of bucket[kind]||[]){const o=data[kind][index],p=kind==='barriers'?o[0][0]:['signs','pois'].includes(kind)?o.slice(0,2):o.slice(1,3);if(kind==='barriers'?o[0].some(p=>Math.hypot(p[0]-x,p[1]-z)<reach):Math.hypot(p[0]-x,p[1]-z)<reach)result[kind].push(index);}
  }
  result.barriers=[...new Set(result.barriers)];
  for(const kind of ['objects','signs','pois'])result[kind].sort((a,b)=>{const aa=data[kind][a],bb=data[kind][b],offset=['signs','pois'].includes(kind)?0:1;return Math.hypot(aa[offset]-x,aa[offset+1]-z)-Math.hypot(bb[offset]-x,bb[offset+1]-z);});
  return result;
 }
 function stopRouting(){routeWorker?.terminate();routeWorker=null;routeReady=false;routePoints=[];routeSource=null;routePending=false;gpsUntil=0;gpsKey='';gpsFailedKey='';routeRequest++;routeGoal=null;lastRoutePosition=null;}
 function setupRouting(){stopRouting();const select=el('route-destination');select.replaceChildren();const auto=document.createElement('option');auto.value='auto';auto.textContent='Job destination · automatic';select.append(auto);roads.labels.forEach((label,i)=>{const option=document.createElement('option');option.value=String(i);option.textContent=label.name;select.append(option);});
  if(roads.version!==2||!window.Worker){texts('route-status','Routing needs a detailed map and Web Worker support.');return;}
  texts('route-status','Preparing the directed road graph…');routeWorker=new window.Worker('/routing-worker.js');
  routeWorker.onmessage=e=>{const m=e.data;if(m.type==='ready'){routeReady=true;texts('route-status',`${m.nodes.toLocaleString()} navigation nodes ready. Choose a city or load an active job.`);maybeRoute(true);}else if(m.type==='error'&&m.id===undefined||m.id===routeRequest){routePending=false;if(m.error){routePoints=[];gpsUntil=0;routeSource=null;if(pendingKind==='gps'){gpsFailedKey=gpsKey;routeGoal=null;texts('gps-status',m.error);}texts('route-status',m.error);}else{routePoints=m.points;routeSource=m.source;texts('route-status',m.source==='ets2la'?'Game GPS route · ETS2LA provider':'HaulSense route to '+(routeGoal?.name||'destination')+' · calculated on mapped roads');draw();}}};
  routeWorker.onerror=()=>{routePending=false;routeReady=false;texts('route-status','Routing worker failed. Reload the map to retry.');};
  routeWorker.postMessage({type:'load',map:{graph:roads.graph,prefabs:roads.prefabs,templates:roads.templates}});
 }
 async function pollSignals(){
  if(!window.fetch||!current?.active||current.demo||signalPolling||Date.now()-lastSignalPoll<200)return;
  signalPolling=true;lastSignalPoll=Date.now();try{const response=await window.fetch('/api/signals',{cache:'no-store',signal:window.AbortSignal?.timeout(1000)});if(!response.ok)throw Error('Provider unavailable');const data=await response.json();if(!current?.active||current.demo){signals=[];signalExpiry=0;return;}
   const valid=data.fresh===true&&Array.isArray(data.objects)&&data.objects.length<=40&&data.objects.every(o=>Array.isArray(o.position)&&o.position.length===3&&o.position.every(v=>finite(v)&&Math.abs(v)<1e8)&&[1,2].includes(o.type)&&Number.isInteger(o.state)&&finite(o.remaining_s));
   signals=valid?data.objects:[];signalExpiry=valid?Date.now()+1000:0;
   const lights=signals.filter(o=>o.type===1),position=current?.telemetry?.world_position;
   if(valid&&position){lights.sort((a,b)=>Math.hypot(a.position[0]-position[0],a.position[2]-position[2])-Math.hypot(b.position[0]-position[0],b.position[2]-position[2]));const nearest=lights[0],phase={0:'OFF',1:'AMBER → RED',2:'RED',4:'AMBER → GREEN',8:'GREEN',32:'FLASHING AMBER'};texts('signal-status',nearest?'ETS2LA · nearest signal '+phase[nearest.state]+' · '+nearest.remaining_s.toFixed(1)+' s · '+Math.round(Math.hypot(nearest.position[0]-position[0],nearest.position[2]-position[2]))+' m. Check your approach in-game.':'ETS2LA · no nearby traffic lights');}
   else texts('signal-status',data.available?'Signal feed stale · phases hidden':'Live signal provider unavailable · static positions only');
  }catch{signals=[];signalExpiry=0;texts('signal-status','Live signal feed unavailable · phases hidden');}finally{signalPolling=false;}
 }
 async function pollGps(){
  if(!window.fetch||!routeReady||!current?.active||current.demo||!roads||roads.game!==current.telemetry.game||(el('route-destination').value||'auto')!=='auto'||gpsPolling||Date.now()-lastGpsPoll<1000)return;
  gpsPolling=true;lastGpsPoll=Date.now();const worker=routeWorker;
  try{const response=await window.fetch('/api/route',{cache:'no-store',signal:window.AbortSignal?.timeout(1500)});if(!response.ok)throw Error('Provider unavailable');const m=await response.json();
   if(worker!==routeWorker||!current?.active||current.demo)return;
   if(m.source==='ets2la'&&m.fresh===true&&Array.isArray(m.node_uids)&&m.node_uids.length>=2&&m.node_uids.length<=6000&&m.node_uids.every(uid=>typeof uid==='string'&&/^[a-f0-9]{1,16}$/.test(uid))){
    const key=m.node_uids.join(',');if(key===gpsFailedKey)return;gpsUntil=Date.now()+1500;texts('gps-status','ETS2LA provider · fresh route received');if(key!==gpsKey||routeSource!=='ets2la'){gpsKey=key;pendingKind='gps';routePending=true;worker.postMessage({type:'gps',id:++routeRequest,uids:m.node_uids});}
   }else {texts('gps-status',m.available?'ETS2LA route unchanged or stale · using local routing':'Game GPS provider unavailable · local routing available');if(routeSource==='ets2la'){routePoints=[];routeSource=null;gpsUntil=0;gpsKey='';routeGoal=null;maybeRoute(true);}}
  }catch{if(routeSource==='ets2la'){routePoints=[];routeSource=null;gpsUntil=0;routeGoal=null;maybeRoute(true);}}
  finally{gpsPolling=false;}
 }
 function maybeRoute(force=false){
  if(Date.now()<gpsUntil&&!force)return;
  if(!routeReady||routePending||!current?.active||!roads||roads.game!==current.telemetry.game)return;
  const t=current.telemetry,p=t.world_position;if(!Array.isArray(p)||p.length!==3||!p.every(v=>finite(v)&&Math.abs(v)<1e8))return;
  const selected=el('route-destination').value||'auto';
  const normalize=s=>String(s||'').normalize('NFD').replace(/[\u0300-\u036f]/g,'').toLowerCase();
  const goal=selected==='auto'?roads.labels.find(l=>normalize(l.name)===normalize(t.destination)):roads.labels[Number(selected)];
  if(!goal){routePoints=[];routeGoal=null;texts('route-status','Choose a city. A GPS waypoint without a job has no destination name in the SDK.');return;}
  const moved=!lastRoutePosition||Math.hypot(p[0]-lastRoutePosition[0],p[2]-lastRoutePosition[1])>150;
  if(!force&&routeGoal===goal&&!moved)return;
  if(!force&&Date.now()-lastRouteAt<3000)return;
  pendingKind='route';routeGoal=goal;lastRoutePosition=[p[0],p[2]];lastRouteAt=Date.now();routePending=true;routeSource='haulsense';
  texts('route-status','Calculating driveable route to '+goal.name+'…');routeWorker.postMessage({type:'route',id:++routeRequest,start:[p[0],p[2]],end:goal.position});
 }
 for(const kind of ['props','signs','details'])el('map-'+kind).addEventListener('click',()=>{layers[kind]=!layers[kind];el('map-'+kind).setAttribute('aria-pressed',String(layers[kind]));draw();});
 el('route-destination').addEventListener('change',()=>{routeRequest++;routePending=false;routeGoal=null;gpsUntil=0;routeSource=null;gpsKey='';routePoints=[];texts('gps-status','Automatic mode can use the game GPS; selected cities use HaulSense routing.');maybeRoute(true);});
 el('route-plan').addEventListener('click',()=>{routeReady=!!routeWorker;maybeRoute(true);});
 el('route-clear').addEventListener('click',()=>{routePoints=[];routeGoal=null;routeRequest++;lastRoutePosition=null;routePending=false;routeReady=false;texts('route-status','Route cleared. Calculate route to start again.');draw();});
 el('map-file').addEventListener('change',async()=>{const file=el('map-file').files?.[0];if(!file)return;mapRevision++;try{if(file.size>limit.bytes)throw Error('Map exceeds 96 MiB. Export a smaller region.');loadMap(JSON.parse(await file.text()));try{await window.HaulSenseStorage?.saveMap(file,file.name);texts('map-storage','Saved on this device · restored automatically next launch');}catch(error){texts('map-storage','Map loaded, but persistence failed: '+error.message);}}catch(e){mapError='Map not loaded: '+e.message;text('map-note',mapError);}finally{el('map-file').value='';}});
 el('map-unload').addEventListener('click',()=>{mapRevision++;window.HaulSenseStorage?.forgetMap().then(()=>texts('map-storage','Saved map removed')).catch(error=>texts('map-storage',error.message));roads=null;mapError='';stopRouting();if(scene){scene.dispose();scene=null;}el('road-scene').hidden=true;el('map-unload').hidden=true;if(current)update(current,metric);});
 el('map-perspective').addEventListener('click',()=>{perspective=!perspective;el('map-perspective').setAttribute('aria-pressed',String(perspective));el('map-perspective').textContent=perspective?'3D / Perspective':'2D';if(current)update(current,metric);});
 el('map-north').addEventListener('click',()=>{north=!north;el('map-north').setAttribute('aria-pressed',String(north));if(current)update(current,metric);});
 el('map-zoom-in').addEventListener('click',()=>{zoom=Math.min(4,zoom*1.4);draw();});el('map-zoom-out').addEventListener('click',()=>{zoom=Math.max(.25,zoom/1.4);draw();});
 el('map-clear').addEventListener('click',()=>{trace=[];last=null;draw();});el('trip-reset').addEventListener('click',()=>{reset();if(current)update(current,metric);});
 el('map-fullscreen').addEventListener('click',async()=>{try{if(document.fullscreenElement)await document.exitFullscreen();else await el('navigator').requestFullscreen();}catch{ text('map-note','Fullscreen unavailable in this browser.');}});
 async function restoreMap(){const revision=mapRevision;try{const entry=await window.HaulSenseStorage.readMap();if(revision!==mapRevision)return;
   let blob=entry?.blob,name=entry?.name||'Installed local map';
   if(!blob&&window.location?.protocol==='haulsense:'){const response=await window.fetch('/api/map');if(response.ok)blob=await response.blob();}
   if(!blob)return;if(blob.size>limit.bytes)throw Error('Saved map exceeds import limit');texts('map-storage','Restoring '+name+'…');const data=JSON.parse(await blob.text());if(revision!==mapRevision)return;loadMap(data);texts('map-storage','Restored automatically · saved on this device');if(!entry)await window.HaulSenseStorage.saveMap(blob,name);
  }catch(error){texts('map-storage','Saved map unavailable: '+error.message);}}
 if(window.HaulSenseStorage)restoreMap();
 document.addEventListener('fullscreenchange',draw);window.addEventListener('resize',draw);
 return {update,draw,loadMap,validateMap,queryScene,stats:()=>scene?.stats()};
}
if(typeof module!=='undefined')module.exports={createNavigator};
