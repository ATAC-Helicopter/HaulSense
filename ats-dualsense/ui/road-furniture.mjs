/* Original road furniture reconstructed from map names/text. No game artwork. */
const clamp=(v,a,b)=>Math.max(a,Math.min(b,v));
export function mapFacing(rotation,opposite=false){return Math.PI/2-rotation+(opposite?Math.PI:0);}
export function signDesign(model='',content='',game='ats'){
 const name=String(model).toLowerCase(),words=String(content).split(/\s*·\s*/).map(s=>s.trim()).filter(s=>s&&!/^(?:small arrow|type [a-z]|arrow|road symbol|sign symbol|blank)(?:\s|$)/i.test(s)&&!/^\/(?:def|model)\//.test(s)).slice(0,6);
 const dimension=name.match(/(?:^|[_\s])(?:g|y)?(\d{2,4})x(\d{2,4})(?:[_\s.]|$)/);
 let kind='unknown';
 if(/\bstop\b|[_/]stop(?:[_/.]|$)/.test(name))kind='stop';
 else if(/yield|give.?way/.test(name))kind='yield';
 else if(/speed.?limit|speed.?zone/.test(name))kind=game==='ats'?'speed':'speed-eu';
 else if(/warning|diamond|curve|twisty|sharp|rollover|chevron|roundabout|merge/.test(name))kind=game==='ats'?'warning':'warning-eu';
 else if(/interstate|hw.shield|(?:^|[_\s])us.[12]hw/.test(name))kind='route';
 else if(/navigation|board|ovh|overhead|fwy|fw\d/.test(name))kind='guide';
 const overhead=kind==='guide'&&/ovh|overhead/.test(name),width=dimension?clamp(+dimension[1]/100,.35,12):kind==='guide'?(overhead?5:2.5):kind==='speed'?.65:kind==='route'&&/wide/.test(name)?1.8:.9;
 const height=dimension?clamp(+dimension[2]/100,.35,5):kind==='guide'?(overhead?2.5:1.25):kind==='speed'?1:.9;
 const shape=kind==='stop'?'octagon':kind==='yield'?'triangle':kind==='warning-eu'?'triangle-up':kind==='warning'?'diamond':kind==='speed-eu'?'circle':'rectangle';
 const speed=words.map(w=>w.match(/^(?:SPEED LIMIT\s*)?(\d{1,3})(?:\s*(?:MPH|KM\/H))?$/i)?.[1]).find(Boolean)||(/speed[_ ]limit[_ ](\d{2,3})(?:[_. ]|$)/.exec(name)?.[1]??null);
 const route=words.map(w=>w.match(/^(US|INTERSTATE|I|STATE ROUTE|SR)\s*[- ]\s*(\d{1,3})$/i)).find(Boolean)||null;
 let colour=kind==='stop'||kind==='yield'?'red':kind==='warning'?'yellow':kind==='guide'?'sign':'white';
 if(kind==='guide'&&/brown/.test(name))colour='brown';else if(kind==='guide'&&/blue/.test(name))colour='blue';else if(kind==='guide'&&/white/.test(name))colour='white';else if(kind==='guide'&&/yellow/.test(name))colour='yellow';
 return {kind,shape,width,height,overhead,colour,words,speed,route:route?{type:route[1].toUpperCase(),number:route[2]}:null,exitOnly:/exit.?only|exonly|(?:^|[_ /])eo(?:[_ .]|$)/.test(name),direction:/left/.test(name)?'left':/right/.test(name)?'right':null};
}
export function signalDesign(profile='',game='ats'){
 const p=String(profile).toLowerCase();return {visible:!/(?:gas|gate|barrier|crossing|parking|toll)/.test(p),horizontal:/horizontal|horiz/.test(p),backplate:game==='ats'?'yellow':'black',mountHeight:game==='ats'?5.8:4.2};
}
export function signPolygon(shape){
 if(shape==='octagon')return Array.from({length:8},(_,i)=>{const a=(i+.5)*Math.PI/4;return [Math.cos(a)*.5/Math.cos(Math.PI/8),Math.sin(a)*.5/Math.cos(Math.PI/8)];});
 if(shape==='triangle')return [[-.5,.5],[0,-.5],[.5,.5]];
 if(shape==='triangle-up')return [[-.5,-.5],[.5,-.5],[0,.5]];
 if(shape==='diamond')return [[0,.5],[-.5,0],[0,-.5],[.5,0]];
 if(shape==='circle')return Array.from({length:24},(_,i)=>[Math.cos(i*Math.PI/12)*.5,Math.sin(i*Math.PI/12)*.5]);
 return [[-.5,-.5],[.5,-.5],[.5,.5],[-.5,.5]];
}
// One bounded atlas serves panel faces, replacing floating sign labels.
export function createSignAtlas(document){
 const canvas=document.createElement('canvas');canvas.width=2048;canvas.height=1024;const ctx=canvas.getContext('2d'),slots=new Map();
 function begin(){slots.clear();ctx.clearRect(0,0,canvas.width,canvas.height);}
 function slot(design){const key=JSON.stringify({...design,width:undefined,height:undefined,overhead:undefined,aspect:Math.round(design.width/design.height*1000)/1000});if(slots.has(key))return slots.get(key);if(slots.size>=32)return null;const id=slots.size,col=id%8,row=Math.floor(id/8);slots.set(key,{col,row});ctx.save();ctx.translate(col*256,row*256);paintSign(ctx,design);ctx.restore();return {col,row};}
 return {canvas,begin,slot,count:()=>slots.size};
}
function paintSign(ctx,d){
 const colours={red:'#ad2428',yellow:'#efc54a',white:'#eeeede',sign:'#166044',brown:'#695039',blue:'#194d79',prop:'#737875'};
 const polygon=signPolygon(d.shape),path=()=>{ctx.beginPath();polygon.forEach(([x,y],i)=>i?ctx.lineTo(128+x*240,128-y*240):ctx.moveTo(128+x*240,128-y*240));ctx.closePath();};
 ctx.fillStyle=colours[d.colour]||'#738076';path();ctx.fill();ctx.save();path();ctx.clip();
 ctx.strokeStyle=d.kind==='warning-eu'?'#b12a2d':['stop','yield','guide','route'].includes(d.kind)?'#eeeee2':'#292b29';ctx.lineWidth=d.kind==='warning-eu'?24:6;ctx.lineJoin='round';path();ctx.stroke();
 const dark=['white','yellow'].includes(d.colour);ctx.fillStyle=dark?'#252825':'#f5f3df';
 function text(value,y,size=32,bold=true){ctx.save();ctx.translate(128,y);ctx.scale(d.height/d.width,1);ctx.textAlign='center';ctx.textBaseline='middle';ctx.font=(bold?'600 ':'')+size+'px sans-serif';ctx.fillText(String(value).slice(0,80),0,0,215*d.width/d.height);ctx.restore();}
 if(d.kind==='stop')text('STOP',130,68);
 else if(d.kind==='yield'){ctx.fillStyle='#f4f1df';ctx.beginPath();ctx.moveTo(48,52);ctx.lineTo(208,52);ctx.lineTo(128,199);ctx.closePath();ctx.fill();ctx.fillStyle='#a51e25';text('YIELD',100,38);}
 else if(d.kind==='speed'){text('SPEED',53,32);text('LIMIT',90,32);if(d.speed)text(d.speed,163,83);}
 else if(d.kind==='speed-eu'){ctx.strokeStyle='#b12a2d';ctx.lineWidth=25;ctx.beginPath();ctx.arc(128,128,103,0,2*Math.PI);ctx.stroke();if(d.speed)text(d.speed,130,83);}
 else if(d.kind==='route'&&d.route){ctx.fillStyle='#eaeade';ctx.beginPath();ctx.moveTo(53,57);ctx.lineTo(203,57);ctx.lineTo(190,173);ctx.quadraticCurveTo(128,226,66,173);ctx.closePath();ctx.fill();const interstate=['I','INTERSTATE'].includes(d.route.type);if(interstate){ctx.fillStyle='#224c78';ctx.fill();ctx.fillStyle='#ad3435';ctx.fillRect(56,58,144,40);ctx.fillStyle='#ffffff';text('INTERSTATE',80,19);}else ctx.fillStyle='#222522';text(d.route.number,145,71);const direction=d.words.find(w=>/^(?:NORTH|SOUTH|EAST|WEST|JCT|TO)$/i.test(w));if(direction){ctx.fillStyle='#252825';text(direction,28,24);}}
 else if(d.kind==='warning'||d.kind==='warning-eu'){if(d.direction){ctx.strokeStyle='#252825';ctx.lineWidth=17;ctx.lineCap='round';ctx.lineJoin='round';ctx.beginPath();const flip=d.direction==='left'?-1:1;ctx.moveTo(128,183);ctx.lineTo(128,132);ctx.quadraticCurveTo(128,97,128+flip*48,97);ctx.stroke();ctx.beginPath();ctx.moveTo(128+flip*58,97);ctx.lineTo(128+flip*30,75);ctx.lineTo(128+flip*30,119);ctx.closePath();ctx.fill();}else if(d.words.length){d.words.slice(0,3).forEach((w,i)=>text(w,89+i*36,26));}}
 else if(d.kind==='guide'){const rows=d.words.filter(w=>!/^US\s*\d+$|^INTERSTATE\s*\d+$|^I\s*-?\s*\d+$/.test(w));(rows.length?rows:d.words).slice(0,5).forEach((w,i,a)=>text(w,64+i*(d.exitOnly?135:165)/Math.max(1,a.length),29));if(d.exitOnly){ctx.fillStyle='#eac44c';ctx.fillRect(9,206,238,40);ctx.fillStyle='#252825';text('EXIT ONLY',226,27);}}
 else if(d.words.length)d.words.slice(0,4).forEach((w,i)=>text(w,73+i*40,29));
 ctx.restore();
}
// Lane-control points on the same approach share a reconstructed mast arm.
export function groupSignalApproaches(candidates){
 const groups=[];for(const row of candidates){const a=row[3];let group=groups.find(g=>{const ref=g[0],dx=row[0]-ref[0],dz=row[2]-ref[2],difference=Math.atan2(Math.sin(a-ref[3]),Math.cos(a-ref[3]));return Math.abs(difference)<.08&&Math.abs(row[1]-ref[1])<.5&&Math.abs(dx*Math.sin(ref[3])+dz*Math.cos(ref[3]))<.4&&Math.abs(dx*Math.cos(ref[3])-dz*Math.sin(ref[3]))<12;});if(!group){group=[];groups.push(group);}group.push(row);}return groups;
}
export function boundsCrossRoad(row,roads,origin,margin=3.5){
 const [x,y,z,w,h,d,angle]=row,c=Math.cos(angle),s=Math.sin(angle),wx=w/2+margin,dz=d/2+margin;
 const local=p=>{const dx=p[0]-origin[0]-x,zz=p[1]-origin[1]-z;return [dx*c-zz*s,dx*s+zz*c,(p[2]??origin[2])-origin[2]];};
 for(const road of roads)for(let i=1;i<road.length;i++){const a=local(road[i-1]),b=local(road[i]);if(Math.max(a[2],b[2])<y-.2||Math.min(a[2],b[2])>y+h+.2)continue;let low=0,high=1;for(const [axis,extent]of [[0,wx],[1,dz]]){const delta=b[axis]-a[axis];if(Math.abs(delta)<1e-8){if(Math.abs(a[axis])>extent){low=2;break;}}else{const first=(-extent-a[axis])/delta,last=(extent-a[axis])/delta;low=Math.max(low,Math.min(first,last));high=Math.min(high,Math.max(first,last));}}if(low<=high)return true;}
 return false;
}
