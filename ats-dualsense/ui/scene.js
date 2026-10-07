/* HaulSense's original low-poly road diorama. Only labels use small generated canvases. */
import * as T from 'three';
import {strip,dashes,assetShape,signalColour,shortestAngle,mappedGround} from './scene-geometry.mjs';
window.HaulSenseScene=function(canvas){
 const renderer=new T.WebGLRenderer({canvas,antialias:true,alpha:false,powerPreference:'low-power'});
 renderer.setPixelRatio(Math.min(window.devicePixelRatio||1,1.5));
 const scene=new T.Scene();scene.background=new T.Color('#132326');scene.fog=new T.Fog('#132326',220,1300);
 const camera=new T.PerspectiveCamera(52,1,.5,3500);
 scene.add(new T.HemisphereLight(0xc9e9e6,0x344833,2.2));
 const sun=new T.DirectionalLight(0xffe3bb,2.3);sun.position.set(-100,250,80);scene.add(sun);
 const geometry={box:new T.BoxGeometry(1,1,1),pole:new T.CylinderGeometry(.5,.5,1,6),cone:new T.ConeGeometry(.7,1,6),roof:new T.ConeGeometry(Math.SQRT1_2,1,4),sphere:new T.SphereGeometry(.5,8,6),stop:new T.CylinderGeometry(.5,.5,.07,8)};
 geometry.roof.rotateY(Math.PI/4);geometry.stop.rotateX(Math.PI/2);
 const colours={road:0x384749,shoulder:0x56605a,white:0xe0e6cf,yellow:0xe8c46d,ground:0x253c35,building:0x738b87,roof:0x465c5b,trunk:0x75614d,tree:0x497c61,prop:0x8d8c72,steel:0x839b99,black:0x1b292d,glass:0x2b5964,body:0xd6ee9b,trailer:0xa2bdb6,red:0xfa6459,amber:0xffbd57,green:0x83efb3,off:0x29383b,sign:0x2d735e};
 const materials={};for(const [name,color]of Object.entries(colours))materials[name]=new T.MeshLambertMaterial({color,flatShading:true,side:['road','shoulder','white','yellow','ground'].includes(name)?T.DoubleSide:T.FrontSide});
 for(const name of ['red','amber','green']){materials[name].emissive.set(colours[name]);materials[name].emissiveIntensity=.7;}
 const matrix=new T.Matrix4(),quat=new T.Quaternion(),scale=new T.Vector3(),position=new T.Vector3(),up=new T.Vector3(0,1,0);
 let renderGame='ats',content=new T.Group(),lastData=null,builtKey=null,origin=null,batches,faces,lines;scene.add(content);
 const textures=new Map();let contextLost=false,disposed=false,animation=0,lastFrame=0,target=null,pose=null,liveHeads=[],viewportWidth=0,viewportHeight=0;
 function clear(){content.traverse(o=>{if(o.isInstancedMesh)o.dispose();if(o.geometry&&!Object.values(geometry).includes(o.geometry))o.geometry.dispose();if(o.material?.userData?.temporary)o.material.dispose();});scene.remove(content);content=new T.Group();scene.add(content);builtKey=null;liveHeads=[];}
 function instance(shape,material,x,y,z,sx,sy,sz,angle=0){const key=shape+':'+material;if(!batches.has(key))batches.set(key,[]);batches.get(key).push([x,y,z,sx,sy,sz,angle]);}
 function flushInstances(group=content){for(const [key,rows]of batches){const [shape,material]=key.split(':'),mesh=new T.InstancedMesh(geometry[shape],materials[material],rows.length);rows.forEach((r,i)=>{position.set(r[0],r[1],r[2]);quat.setFromAxisAngle(up,r[6]);scale.set(Math.max(.01,r[3]),Math.max(.01,r[4]),Math.max(.01,r[5]));matrix.compose(position,quat,scale);mesh.setMatrixAt(i,matrix);});mesh.instanceMatrix.needsUpdate=true;mesh.computeBoundingSphere();group.add(mesh);}batches.clear();}
 function surface(name,vertices){let batch=faces.get(name);if(!batch){batch=[];faces.set(name,batch);}for(const v of vertices)batch.push(v);}
 function line(points,color){if(points.length<2)return;let batch=lines.get(color);if(!batch){batch=[];lines.set(color,batch);}for(let i=1;i<points.length;i++)batch.push(...points[i-1],...points[i]);}
 function label(text,p,size=12){if(!text)return;const key=String(text).slice(0,100);let texture=textures.get(key);if(!texture){const c=document.createElement('canvas');c.width=512;c.height=96;const x=c.getContext('2d');x.fillStyle='#193c32';x.fillRect(0,0,512,96);x.strokeStyle='#dce5cf';x.lineWidth=3;x.strokeRect(2,2,508,92);x.fillStyle='#eef5dc';x.font='bold 26px system-ui';x.textAlign='center';x.textBaseline='middle';x.fillText(key,256,48,480);texture=new T.CanvasTexture(c);textures.set(key,texture);}const mat=new T.SpriteMaterial({map:texture});mat.userData.temporary=true;const sprite=new T.Sprite(mat);sprite.position.set(...p);sprite.scale.set(size,size*96/512,1);content.add(sprite);}
 function road(points,style,details){
  const left=style?.[0]??1,right=style?.[1]??1,median=Math.max(0,style?.[2]||0),sl=Math.max(0,style?.[3]||0),sr=Math.max(0,style?.[4]||0);
  const innerLeft=left*3.5+median/2,innerRight=right*3.5+median/2;
  // Asymmetric one-way carriageways retain their measured lane side.
  surface('shoulder',strip(points,-innerRight-sr,innerLeft+sl,origin,-.035));
  surface('road',strip(points,-innerRight,innerLeft,origin));
  if(!details)return;
  for(const offset of [-innerRight+.15,innerLeft-.15])surface('white',strip(points,offset-.055,offset+.055,origin,.04));
  if(left&&right){for(const offset of [-median/2-.08,median/2+.08])surface(renderGame==='ats'?'yellow':'white',strip(points,offset-.05,offset+.05,origin,.05));}
  for(let lane=1;lane<left;lane++)surface('white',dashes(points,median/2+lane*3.5,.11,origin));
  for(let lane=1;lane<right;lane++)surface('white',dashes(points,-median/2-lane*3.5,.11,origin));
 }
 function area(points,kind){if(points.length<3)return;const shape=new T.Shape();points.forEach((p,i)=>{const x=p[0]-origin[0],z=-(p[1]-origin[1]);i?shape.lineTo(x,z):shape.moveTo(x,z);});const g=new T.ShapeGeometry(shape);g.rotateX(-Math.PI/2);const mesh=new T.Mesh(g,materials[kind===0?'road':'ground']);mesh.position.y=(points[0][2]??origin[2])-origin[2]-.08;content.add(mesh);}
 function sign(x,y,z,angle,name){instance('pole','steel',x,y+1.6,z,.12,3.2,.12);const stop=/\bstop\b/i.test(name);instance(stop?'stop':'box',stop?'red':'sign',x,y+3.25,z,stop?1:.16,stop?1:1.15,stop?1:2.5,angle);}
 function staticSignal(x,y,z,angle){instance('pole','steel',x,y+2,z,.14,4,.14);instance('box','black',x,y+4.1,z,.48,1.4,.4,angle);for(const height of [4.55,4.1,3.65])instance('sphere','off',x+Math.sin(angle)*.23,y+height,z+Math.cos(angle)*.23,.28,.28,.28);}
 function prop(shape,row){const [x,y,z,w,h,d,a]=row;
  if(shape==='landmark'){instance('box','building',x,y+.25,z,w,.5,d,a);return;}
  if(shape==='tree'||shape==='bush'){if(shape==='tree')instance('pole','trunk',x,y+h*.25,z,Math.min(w,.4),h*.5,Math.min(d,.4));instance('cone','tree',x,y+h*.63,z,w,h*.74,d,a);return;}
  if(shape==='lamp'){instance('pole','steel',x,y+h/2,z,.15,h,.15);instance('box','steel',x,y+h,z,Math.max(1,w),.15,.2,a);return;}
  if(shape==='bollard'){instance('pole','yellow',x,y+Math.min(h,1.2)/2,z,.2,Math.min(h,1.2),.2);return;}
  if(shape==='barrier'){instance('box','steel',x,y+Math.min(h,.8),z,w,.2,d,a);return;}
  const house=shape==='house';instance('box',shape==='prop'?'prop':'building',x,y+h*(house?.4:.5),z,w,h*(house?.8:1),d,a);
  if(house)instance('roof','roof',x,y+h*.9,z,w,h*.2,d,a);
 }
 function build(data,visible,layers){
  clear();renderGame=data.game;batches=new Map();faces=new Map();lines=new Map();
  for(const i of visible.roads)road(data.roads[i],data.roadStyles?.[i],layers.details);
  if(layers.props)for(const i of visible.objects.slice(0,2200)){const o=data.objects[i],a=data.assets[o[0]],cx=(a[3]+a[5])/2*o[5],cz=(a[4]+a[6])/2*o[6],cs=Math.cos(o[4]),sn=Math.sin(o[4]);prop(assetShape(a[1],a[2]),[o[1]+cx*cs-cz*sn-origin[0],o[3]-origin[2],o[2]+cx*sn+cz*cs-origin[1],Math.abs(a[5]-a[3])*Math.abs(o[5]),Math.abs(a[7]*o[7]),Math.abs(a[6]-a[4])*Math.abs(o[6]),-o[4]]);}
  let labels=0,posts=0;
  for(const i of visible.prefabs){const p=data.prefabs[i],t=data.templates[p[0]],cs=Math.cos(p[4]),sn=Math.sin(p[4]);const tx=q=>[p[1]+q[0]*cs-q[1]*sn,p[2]+q[0]*sn+q[1]*cs,p[3]+q[2]];
   for(const polygon of t.areas)area(polygon[1].map(tx),polygon[0]);
   for(const l of t.lines){const points=[tx(l[0]),tx(l[1])];road(points,[l[2]??1,l[3]??1,l[4]||0,0,0],false);}
   // AI curves are route geometry, not lane paint. Never paint every possible turn.
   if(layers.signs){for(const s of t.signs){const q=tx(s);sign(q[0]-origin[0],q[2]-origin[2],q[1]-origin[1],-p[4]-s[3],s[4]);}for(const s of t.signals){const q=tx(s);staticSignal(q[0]-origin[0],q[2]-origin[2],q[1]-origin[1],-p[4]-s[3]);}}
  }
  if(layers.signs)for(const i of visible.signs.slice(0,180)){const s=data.signs[i],name=s[5]||data.signDefinitions[s[4]]?.[1]||'Sign',x=s[0]-origin[0],z=s[1]-origin[1];sign(x,s[2]-origin[2],z,-s[3],name);if(Math.hypot(x,z)<150&&labels++<24)label(/\bstop\b/i.test(name)?'STOP':name,[x,s[2]-origin[2]+4.5,z],6);}
  if(layers.details)for(const i of visible.barriers){const points=data.barriers[i][0];for(let j=1;j<points.length;j++){const a=points[j-1],b=points[j],length=Math.hypot(b[0]-a[0],b[1]-a[1]),steps=Math.min(256,Math.ceil(length/12));instance('box','steel',(a[0]+b[0])/2-origin[0],(a[2]+b[2])/2-origin[2]+.6,(a[1]+b[1])/2-origin[1],length,.22,.16,-Math.atan2(b[1]-a[1],b[0]-a[0]));for(let k=0;k<steps&&posts++<2400;k++){const f=k/steps;instance('pole','steel',a[0]+(b[0]-a[0])*f-origin[0],(a[2]+(b[2]-a[2])*f)-origin[2]+.35,a[1]+(b[1]-a[1])*f-origin[1],.09,.7,.09);}}}
  if(layers.details)for(const i of visible.pois.slice(0,20)){const poi=data.pois[i];if(Math.hypot(poi[0]-origin[0],poi[1]-origin[1])<220)label(poi[4]||poi[2],[poi[0]-origin[0],8,poi[1]-origin[1]],10);}
  flushInstances();
  for(const [name,vertices]of faces){if(!vertices.length)continue;const g=new T.BufferGeometry();g.setAttribute('position',new T.Float32BufferAttribute(vertices,3));g.computeVertexNormals();content.add(new T.Mesh(g,materials[name]));}
  faces.clear();
  // Textures only serve currently visible text and are bounded independently of map size.
  const used=new Set();content.traverse(o=>{if(o.isSprite)used.add(o.material.map);});for(const [key,texture]of textures)if(!used.has(texture)){texture.dispose();textures.delete(key);}
 }
 const truck=new T.Group(),trailer=new T.Group();scene.add(truck);truck.add(trailer);
 function part(parent,shape,material,x,y,z,sx,sy,sz){const mesh=new T.Mesh(geometry[shape],materials[material]);mesh.position.set(x,y,z);mesh.scale.set(sx,sy,sz);parent.add(mesh);return mesh;}
 part(truck,'box','black',0,.9,0,2.15,.45,5.8);part(truck,'box','body',0,2,-1.25,2.4,2.3,2.55);part(truck,'box','body',0,1.55,-3,2.25,1.25,1.4);part(truck,'box','glass',0,2.6,-2.55,2,.65,.06);part(truck,'box','steel',0,.8,-3.75,2.4,.35,.2);
 for(const x of [-1.25,1.25])for(const z of [-2.5,1,2.2]){const wheel=part(truck,'pole','black',x,.53,z,1.05,.3,1.05);wheel.rotation.z=Math.PI/2;}
 part(trailer,'box','trailer',0,2.25,10,2.55,3.3,12);part(trailer,'box','black',0,.85,10,2.1,.25,12);
 for(const x of [-1.3,1.3])for(const z of [13.5,14.7]){const wheel=part(trailer,'pole','black',x,.53,z,1.05,.3,1.05);wheel.rotation.z=Math.PI/2;}
 const headlights=[-1,1].map(x=>part(truck,'box','white',x,1.45,-3.72,.3,.25,.06));
 function overlayLine(group,points,colour){let segment=[];const flush=()=>{if(segment.length>1){const g=new T.BufferGeometry().setFromPoints(segment),material=new T.LineBasicMaterial({color:colour});material.userData.temporary=true;group.add(new T.Line(g,material));}segment=[];};for(const q of points){if(!q){flush();continue;}segment.push(new T.Vector3(q[0]-origin[0],(q[2]??origin[2])-origin[2]+.12,q[1]-origin[1]));}flush();}
 function updateOverlay(route,trace,signals,layers){const old=content.getObjectByName('journey-overlay');if(old){old.traverse(o=>{if(o.geometry&&!Object.values(geometry).includes(o.geometry))o.geometry.dispose();if(o.material?.userData?.temporary)o.material.dispose();});content.remove(old);}const group=new T.Group();group.name='journey-overlay';content.add(group);liveHeads=[];
  overlayLine(group,route||[],0xffae73);overlayLine(group,trace||[],0x8ed9e7);
  if(layers.signs)for(const signal of (signals||[]).slice(0,40)){if(signal.type!==1)continue;const [x,y,z]=signal.position;if(Math.hypot(x-origin[0],z-origin[1])>500)continue;const head=new T.Group();head.position.set(x-origin[0],y-origin[2],z-origin[1]);group.add(head);part(head,'pole','steel',0,1.8,0,.14,3.6,.14);part(head,'box','black',0,4,0,.6,1.7,.5);const lights=[4.52,4,3.48].map(h=>part(head,'sphere','off',0,h,.29,.36,.36,.2));liveHeads.push({head,lights,state:signal.state});}
 }
 function render(time){if(disposed||contextLost||!target||canvas.hidden||!canvas.getClientRects().length||document.hidden){animation=0;return;}animation=window.requestAnimationFrame(render);if(time-lastFrame<1000/30)return;const dt=Math.min(.1,(time-lastFrame)/1000)||1/30;lastFrame=time;
  const blend=1-Math.exp(-dt*12);for(let i=0;i<3;i++)pose.p[i]+=(target.p[i]-pose.p[i])*blend;pose.angle+=(shortestAngle(pose.angle,target.angle)-pose.angle)*blend;
  const [wx,wy,wz]=pose.p,x=wx-origin[0],z=wz-origin[1],y=wy-origin[2];pose.floor+=(target.floor-pose.floor)*blend;truck.position.set(x,pose.floor-origin[2],z);truck.rotation.set(target.pitch,pose.angle,target.roll,'YXZ');
  const angle=target.north?0:pose.angle,range=target.chase?Math.max(26,42/target.zoom):300/target.zoom;
  const shoulder=target.chase?range*.2:0;camera.position.set(x+Math.sin(angle)*range+Math.cos(angle)*shoulder,y+range*(target.chase?.6:.85),z+Math.cos(angle)*range-Math.sin(angle)*shoulder);
  camera.lookAt(x-Math.sin(angle)*range*(target.chase?.48:.18),y+1,z-Math.cos(angle)*range*(target.chase?.48:.18));
  for(const {head,lights,state}of liveHeads){head.rotation.y=pose.angle;const active=signalColour(state,time);lights.forEach((mesh,i)=>mesh.material=materials[active===['red','amber','green'][i]?active:'off']);}
  renderer.render(scene,camera);
 }
 function update({data,visible,position:p,heading,pitch=0,roll=0,trailerConnected=false,lights=false,north,zoom,chase=true,layers,route,trace,signals,width,height}){
  if(contextLost)throw Error('WebGL context lost');
  if(width!==viewportWidth||height!==viewportHeight){renderer.setSize(width,height,false);viewportWidth=width;viewportHeight=height;}
  camera.aspect=width/height;camera.updateProjectionMatrix();
  if(!data||!p){target=null;clear();return;}
  const key=[Math.floor(p[0]/80),Math.floor(p[2]/80),Math.round(zoom*10),chase,JSON.stringify(layers)].join(':');
  if(data!==lastData||key!==builtKey){origin=[p[0],p[2],p[1]-1];build(data,visible,layers);builtKey=key;lastData=data;}
  target={floor:mappedGround(visible.roads.map(i=>data.roads[i]),p)??p[1]-1,p:p.slice(),angle:heading*2*Math.PI,pitch:Number.isFinite(pitch)?pitch*2*Math.PI:0,roll:Number.isFinite(roll)?roll*2*Math.PI:0,north,zoom,chase};
  if(!pose||Math.hypot(pose.p[0]-p[0],pose.p[2]-p[2])>100)pose={p:p.slice(),angle:target.angle,floor:target.floor};
  trailer.visible=!!trailerConnected;for(const head of headlights)head.visible=!!lights;
  updateOverlay(route,trace,signals,layers);
  if(!animation){lastFrame=0;animation=window.requestAnimationFrame(render);}
 }
 const onContextLost=e=>{e.preventDefault();contextLost=true;builtKey=null;window.cancelAnimationFrame(animation);animation=0;};canvas.addEventListener('webglcontextlost',onContextLost);
 return {update,stats(){return {...renderer.info.render,geometries:renderer.info.memory.geometries,textures:renderer.info.memory.textures,camera:target?.chase?'chase':'overview',truck:true,trailer:trailer.visible,frameCap:30,signals:liveHeads.map(s=>({state:s.state,colours:s.lights.map(m=>m.material.color.getHex())}))};},dispose(){disposed=true;window.cancelAnimationFrame(animation);canvas.removeEventListener('webglcontextlost',onContextLost);clear();for(const texture of textures.values())texture.dispose();for(const material of Object.values(materials))material.dispose();for(const g of Object.values(geometry))g.dispose();renderer.dispose();}};
};
