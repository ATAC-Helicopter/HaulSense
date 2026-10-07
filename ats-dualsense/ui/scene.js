/* HaulSense's original low-poly road diorama. Only labels use small generated canvases. */
import * as T from 'three';
import {signDesign,signalDesign,signPolygon,mapFacing,createSignAtlas,groupSignalApproaches,boundsCrossRoad} from './road-furniture.mjs';
import {strip,dashes,assetShape,signalColour,shortestAngle,mappedGround} from './scene-geometry.mjs';
window.HaulSenseScene=function(canvas){
 const renderer=new T.WebGLRenderer({canvas,antialias:true,alpha:false,powerPreference:'low-power'});
 renderer.setPixelRatio(Math.min(window.devicePixelRatio||1,1.5));
 const scene=new T.Scene();scene.background=new T.Color('#c0cbce');scene.fog=new T.Fog('#c0cbce',250,1250);renderer.toneMapping=T.ACESFilmicToneMapping;renderer.toneMappingExposure=1.05;
 const camera=new T.PerspectiveCamera(52,1,.5,3500);
 scene.add(new T.HemisphereLight(0xd4e4f2,0x505341,1.7));
 const sun=new T.DirectionalLight(0xffefd5,2.1);sun.position.set(-100,250,80);scene.add(sun);
 const geometry={box:new T.BoxGeometry(1,1,1),pole:new T.CylinderGeometry(.5,.5,1,6),cone:new T.ConeGeometry(.7,1,6),roof:new T.ConeGeometry(Math.SQRT1_2,1,4),sphere:new T.SphereGeometry(.5,8,6),stop:new T.CylinderGeometry(.5,.5,.07,8)};
 geometry.roof.rotateY(Math.PI/4);geometry.stop.rotateX(Math.PI/2);
 geometry.wheel=new T.CylinderGeometry(.5,.5,1,16);geometry.lens=new T.CylinderGeometry(.5,.5,1,16);geometry.lens.rotateX(Math.PI/2);geometry.visor=new T.CylinderGeometry(.5,.5,1,12,1,true);geometry.visor.rotateX(Math.PI/2);
 for(const shape of ['octagon','triangle','triangle-up','diamond','circle']){const panel=new T.Shape();signPolygon(shape).forEach(([x,y],i)=>i?panel.lineTo(x,y):panel.moveTo(x,y));panel.closePath();geometry[shape]=new T.ExtrudeGeometry(panel,{depth:1,bevelEnabled:false});geometry[shape].translate(0,0,-.5);}
 const colours={road:0x303234,shoulder:0x595950,white:0xe0e6cf,yellow:0xe8c46d,ground:0x626d50,building:0xa59e87,roof:0x655e52,trunk:0x75614d,tree:0x497c61,prop:0x8d8c72,steel:0x839b99,black:0x1b292d,glass:0x2b5964,body:0xd6ee9b,trailer:0xa2bdb6,red:0xfa6459,amber:0xffbd57,green:0x83efb3,off:0x29383b,sign:0x166044,brown:0x695039,blue:0x194d79};
 const materials={};for(const [name,color]of Object.entries(colours))materials[name]=new T.MeshLambertMaterial({color,flatShading:true,side:['road','shoulder','white','yellow','ground'].includes(name)?T.DoubleSide:T.FrontSide});
 for(const name of ['red','amber','green']){materials[name].emissive.set(colours[name]);materials[name].emissiveIntensity=.7;}
 materials.lens=new T.MeshBasicMaterial({color:0xffffff,toneMapped:false});
 const atlas=createSignAtlas(document),atlasTexture=new T.CanvasTexture(atlas.canvas);atlasTexture.colorSpace=T.SRGBColorSpace;atlasTexture.anisotropy=Math.min(4,renderer.capabilities.getMaxAnisotropy());const panelMaterial=new T.MeshBasicMaterial({map:atlasTexture,alphaTest:.5,toneMapped:false});
 const matrix=new T.Matrix4(),quat=new T.Quaternion(),scale=new T.Vector3(),position=new T.Vector3(),up=new T.Vector3(0,1,0);
 let renderDetails=true,renderGame='ats',content=new T.Group(),lastData=null,builtKey=null,origin=null,batches,faces,lines;scene.add(content);
 const textures=new Map();let contextLost=false,disposed=false,animation=0,lastFrame=0,target=null,pose=null,liveHeads=[],signalHeads=[],lensRows=[],lensBatch=null,panelVertices=[],panelUV=[],unmatchedSignals=0,mastCount=0,viewportWidth=0,viewportHeight=0;
 function clear(){content.traverse(o=>{if(o.isInstancedMesh)o.dispose();if(o.geometry&&!Object.values(geometry).includes(o.geometry))o.geometry.dispose();if(o.material?.userData?.temporary)o.material.dispose();});scene.remove(content);content=new T.Group();scene.add(content);builtKey=null;liveHeads=[];signalHeads=[];lensRows=[];lensBatch=null;mastCount=0;}
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
  if(median>.5&&left&&right){surface('road',strip(points,-innerRight,-median/2,origin));surface('road',strip(points,median/2,innerLeft,origin));surface('ground',strip(points,-median/2,median/2,origin,.01));}
  else surface('road',strip(points,-innerRight,innerLeft,origin));
  if(!details)return;
  for(const offset of [-innerRight+.15,innerLeft-.15])surface('white',strip(points,offset-.055,offset+.055,origin,.04));
  if(left&&right){for(const offset of [-median/2-.08,median/2+.08])surface(renderGame==='ats'?'yellow':'white',strip(points,offset-.05,offset+.05,origin,.05));}
  for(let lane=1;lane<left;lane++)surface('white',dashes(points,median/2+lane*3.5,.11,origin));
  for(let lane=1;lane<right;lane++)surface('white',dashes(points,-median/2-lane*3.5,.11,origin));
 }
 function area(points,kind){if(points.length<3)return;const contour=points.map(p=>new T.Vector2(p[0],p[1])),triangles=T.ShapeUtils.triangulateShape(contour,[]),vertices=[];for(const triangle of triangles)for(const i of triangle){const p=points[i];vertices.push(p[0]-origin[0],(p[2]??origin[2])-origin[2]-.025,p[1]-origin[1]);}surface(kind===0?'road':'ground',vertices);}
 function sign(x,y,z,angle,model,words){
  const d=signDesign(model,words,renderGame),h=d.overhead?7.2:d.height/2+2.05,shape=d.shape==='rectangle'?'box':d.shape;
  if(d.overhead){const top=h+d.height/2+.3;for(const side of [-1,1])instance('pole','steel',x+Math.cos(angle)*side*(d.width/2+.9),y+top/2,z-Math.sin(angle)*side*(d.width/2+.9),.24,top,.24);instance('box','steel',x,y+top,z,d.width+2.3,.22,.24,angle);}
  else {const posts=d.width>1.7?[-d.width*.3,d.width*.3]:[0];for(const offset of posts)instance('pole','steel',x+Math.cos(angle)*offset,y+(h+d.height/2)/2,z-Math.sin(angle)*offset,.09,h+d.height/2,.09);}
  instance(shape,d.colour,x,y+h,z,d.width,d.height,.065,angle);
  if(Math.hypot(x,z)>240)return;const tile=atlas.slot(d);if(!tile)return;
  const poly=signPolygon(d.shape),vertex=([u,v])=>{panelVertices.push(x+u*d.width*Math.cos(angle)+.045*Math.sin(angle),y+h+v*d.height,z-u*d.width*Math.sin(angle)+.045*Math.cos(angle));panelUV.push((tile.col+(u+.5))/8,1-(tile.row+(.5-v))/4);};
  for(let i=0;i<poly.length;i++){vertex([0,0]);vertex(poly[i]);vertex(poly[(i+1)%poly.length]);}
 }
 function staticSignal(x,y,z,angle,profile,world,mount){
  const design=signalDesign(profile,renderGame);if(!design.visible||Math.hypot(x,z)>320||signalHeads.length>=120)return;
  // A schematic stop bar at the extracted lane-control point, not a decoded material marking.
  const tangent=[Math.cos(angle),-Math.sin(angle)];if(renderDetails)surface('white',strip([[world[0]-tangent[0]*1.5,world[1]-tangent[1]*1.5,world[2]],[world[0]+tangent[0]*1.5,world[1]+tangent[1]*1.5,world[2]]],-.13,.13,origin,.055));
  const height=design.mountHeight,local=(lx,lz)=>[x+Math.cos(angle)*lx+Math.sin(angle)*lz,z-Math.sin(angle)*lx+Math.cos(angle)*lz];
  if(mount){const [px,pz]=local(mount.support,0);instance('pole','steel',px,y+(height+.8)/2,pz,.18,height+.8,.18);const [ax,az]=local(mount.center,0);instance('box','steel',ax,y+height+.8,az,mount.width,.15,.18,angle);mastCount++;}
  const horizontal=design.horizontal,w=horizontal?1.55:.8,h=horizontal?.8:1.65;if(horizontal)instance('pole','steel',x,y+height+.6,z,.1,.4,.1);const [bx,bz]=local(0,-.22);instance('box',design.backplate,bx,y+height,bz,w,h,.075,angle);instance('box','black',x,y+height,z,horizontal?1.35:.54,horizontal?.54:1.35,.32,angle);
  const head={world,angle,state:null,colours:[colours.off,colours.off,colours.off],indices:[]};signalHeads.push(head);
  for(let i=0;i<3;i++){const lx=horizontal?(i-1)*.41:0,ly=horizontal?0:(1-i)*.41,[vx,vz]=local(lx,.31);instance('visor','black',vx,y+height+ly,vz,.36,.36,.28,angle);const [sx,sz]=local(lx,.35);head.indices.push(lensRows.length);lensRows.push([sx,y+height+ly,sz,angle]);}
 }
 function flushLenses(){if(!lensRows.length)return;lensBatch=new T.InstancedMesh(geometry.lens,materials.lens,lensRows.length);const colour=new T.Color(colours.off);lensRows.forEach(([x,y,z,a],i)=>{position.set(x,y,z);quat.setFromAxisAngle(up,a);scale.set(.28,.28,.035);matrix.compose(position,quat,scale);lensBatch.setMatrixAt(i,matrix);lensBatch.setColorAt(i,colour);});lensBatch.instanceMatrix.needsUpdate=true;lensBatch.computeBoundingSphere();content.add(lensBatch);}
 function prop(shape,row){const [x,y,z,w,h,d,a]=row;
  if(shape==='landmark'){const local=(lx,lz)=>[x+lx*Math.cos(a)+lz*Math.sin(a),z-lx*Math.sin(a)+lz*Math.cos(a)];for(const side of [-1,1]){let p=local(0,side*d/2);instance('box','building',p[0],y+.03,p[1],w,.06,.14,a);p=local(side*w/2,0);instance('box','building',p[0],y+.03,p[1],.14,.06,d,a);}return;}
  if(shape==='tree'||shape==='bush'){if(shape==='tree')instance('pole','trunk',x,y+h*.25,z,Math.min(w,.4),h*.5,Math.min(d,.4));instance('cone','tree',x,y+h*.63,z,w,h*.74,d,a);return;}
  if(shape==='lamp'){instance('pole','steel',x,y+h/2,z,.15,h,.15);instance('box','steel',x,y+h,z,Math.max(1,w),.15,.2,a);return;}
  if(shape==='bollard'){instance('pole','yellow',x,y+Math.min(h,1.2)/2,z,.2,Math.min(h,1.2),.2);return;}
  if(shape==='barrier'){instance('box','steel',x,y+Math.min(h,.8),z,w,.2,d,a);return;}
  const house=shape==='house';instance('box',shape==='prop'?'prop':'building',x,y+h*(house?.4:.5),z,w,h*(house?.8:1),d,a);
  if(house)instance('roof','roof',x,y+h*.9,z,w,h*.2,d,a);
  else if(shape==='building'){instance('box','roof',x,y+h+.08,z,w+.2,.16,d+.2,a);if(h>4&&w>8&&d>8){const local=(lx,lz)=>[x+lx*Math.cos(a)+lz*Math.sin(a),z-lx*Math.sin(a)+lz*Math.cos(a)];for(const level of [.35,.7])for(const side of [-1,1]){let p=local(0,side*(d/2+.012));instance('box','glass',p[0],y+h*level,p[1],w*.72,Math.min(1.1,h*.08),.025,a);p=local(side*(w/2+.012),0);instance('box','glass',p[0],y+h*level,p[1],.025,Math.min(1.1,h*.08),d*.72,a);}}}
 }
 function build(data,visible,layers){
  clear();renderGame=data.game;renderDetails=layers.details;batches=new Map();faces=new Map();lines=new Map();atlas.begin();panelVertices=[];panelUV=[];
  let floor=origin[2];for(const i of visible.roads)for(const p of data.roads[i])floor=Math.min(floor,p[2]??origin[2]);instance('box','ground',0,floor-origin[2]-.75,0,3000,.1,3000);const signCandidates=[],signalCandidates=[];
  for(const i of visible.roads)road(data.roads[i],data.roadStyles?.[i],layers.details);
  const nearbyRoads=visible.roads.map(i=>data.roads[i]);
  if(layers.props)for(const i of visible.objects.slice(0,2200)){const o=data.objects[i],a=data.assets[o[0]],cx=(a[3]+a[5])/2*o[5],cz=(a[4]+a[6])/2*o[6],cs=Math.cos(o[4]),sn=Math.sin(o[4]),row=[o[1]+cx*cs-cz*sn-origin[0],o[3]-origin[2],o[2]+cx*sn+cz*cs-origin[1],Math.abs(a[5]-a[3])*Math.abs(o[5]),Math.abs(a[7]*o[7]),Math.abs(a[6]-a[4])*Math.abs(o[6]),-o[4]];let shape=assetShape(a[1],a[2]);if(['building','house'].includes(shape)&&(row[3]>25||row[5]>25)&&Math.hypot(row[0],row[2])<450&&boundsCrossRoad(row,nearbyRoads,origin))shape='landmark';prop(shape,row);}

  let posts=0;
  for(const i of visible.prefabs){const p=data.prefabs[i],t=data.templates[p[0]],cs=Math.cos(p[4]),sn=Math.sin(p[4]);const tx=q=>[p[1]+q[0]*cs-q[1]*sn,p[2]+q[0]*sn+q[1]*cs,p[3]+q[2]];
   for(const polygon of t.areas)area(polygon[1].map(tx),polygon[0]);
   for(const l of t.lines){const points=[tx(l[0]),tx(l[1])];road(points,[l[2]??1,l[3]??1,l[4]||0,0,0],false);}
   // Reconstruct paved lane connections from measured AI paths; never paint turn trajectories.
   if(Math.hypot(p[1]-origin[0],p[2]-origin[1])<350)for(const curve of t.curves)surface('road',strip(curve.map(tx),-1.75,1.75,origin,-.02));
   if(layers.signs){for(const s of t.signs){const q=tx(s);signCandidates.push([q[0]-origin[0],q[2]-origin[2],q[1]-origin[1],mapFacing(p[4]+s[3]),s[4],s[5]]);}for(const s of t.signals){const q=tx(s);signalCandidates.push([q[0]-origin[0],q[2]-origin[2],q[1]-origin[1],mapFacing(p[4]+s[3],true),s[4],q]);}}
  }
  if(layers.signs){for(const i of visible.signs){const s=data.signs[i],definition=data.signDefinitions[s[4]];signCandidates.push([s[0]-origin[0],s[2]-origin[2],s[1]-origin[1],mapFacing(s[3]),definition?definition[1]+' '+definition[2]:'',s[5]]);}signCandidates.sort((a,b)=>Math.hypot(a[0],a[2])-Math.hypot(b[0],b[2]));for(const s of signCandidates.slice(0,180))sign(...s);}
  if(layers.details)for(const i of visible.barriers){const points=data.barriers[i][0];for(let j=1;j<points.length;j++){const a=points[j-1],b=points[j],length=Math.hypot(b[0]-a[0],b[1]-a[1]),steps=Math.min(256,Math.ceil(length/12));instance('box','steel',(a[0]+b[0])/2-origin[0],(a[2]+b[2])/2-origin[2]+.6,(a[1]+b[1])/2-origin[1],length,.22,.16,-Math.atan2(b[1]-a[1],b[0]-a[0]));for(let k=0;k<steps&&posts++<2400;k++){const f=k/steps;instance('pole','steel',a[0]+(b[0]-a[0])*f-origin[0],(a[2]+(b[2]-a[2])*f)-origin[2]+.35,a[1]+(b[1]-a[1])*f-origin[1],.09,.7,.09);}}}
  if(layers.details)for(const i of visible.pois.slice(0,20)){const poi=data.pois[i];if(Math.hypot(poi[0]-origin[0],poi[1]-origin[1])<220)label(poi[4]||poi[2],[poi[0]-origin[0],8,poi[1]-origin[1]],10);}
  for(const group of groupSignalApproaches(signalCandidates.filter(s=>signalDesign(s[4],renderGame).visible&&Math.hypot(s[0],s[2])<320).sort((a,b)=>Math.hypot(a[0],a[2])-Math.hypot(b[0],b[2])).slice(0,120))){const ref=group[0],offsets=group.map(s=>(s[0]-ref[0])*Math.cos(ref[3])-(s[2]-ref[2])*Math.sin(ref[3])),outer=Math.max(...offsets)+2.3,inner=Math.min(...offsets)-.4;group.forEach((s,i)=>staticSignal(...s,i?null:{support:outer,center:(outer+inner)/2,width:outer-inner}));}
  flushInstances();flushLenses();
  if(panelVertices.length){const g=new T.BufferGeometry();g.setAttribute('position',new T.Float32BufferAttribute(panelVertices,3));g.setAttribute('uv',new T.Float32BufferAttribute(panelUV,2));content.add(new T.Mesh(g,panelMaterial));}atlasTexture.needsUpdate=true;
  for(const [name,vertices]of faces){if(!vertices.length)continue;const g=new T.BufferGeometry();g.setAttribute('position',new T.Float32BufferAttribute(vertices,3));g.computeVertexNormals();content.add(new T.Mesh(g,materials[name]));}
  faces.clear();
  // Textures only serve currently visible text and are bounded independently of map size.
  const used=new Set();content.traverse(o=>{if(o.isSprite)used.add(o.material.map);});for(const [key,texture]of textures)if(!used.has(texture)){texture.dispose();textures.delete(key);}
 }
 const truck=new T.Group(),trailer=new T.Group();scene.add(truck);truck.add(trailer);
 function part(parent,shape,material,x,y,z,sx,sy,sz){const mesh=new T.Mesh(geometry[shape],materials[material]);mesh.position.set(x,y,z);mesh.scale.set(sx,sy,sz);parent.add(mesh);return mesh;}
 part(truck,'box','black',0,.9,0,2.15,.45,5.8);part(truck,'box','body',0,2,-1.25,2.4,2.3,2.55);part(truck,'box','body',0,1.55,-3,2.25,1.25,1.4);part(truck,'box','glass',0,2.6,-2.55,2,.65,.06);part(truck,'box','steel',0,.8,-3.75,2.4,.35,.2);
 for(const x of [-1.25,1.25])for(const z of [-2.5,1,2.2]){const wheel=part(truck,'wheel','black',x,.53,z,1.05,.3,1.05);wheel.rotation.z=Math.PI/2;}
 part(trailer,'box','trailer',0,2.25,10,2.55,3.3,12);part(trailer,'box','black',0,.85,10,2.1,.25,12);
 for(const x of [-1.3,1.3])for(const z of [13.5,14.7]){const wheel=part(trailer,'wheel','black',x,.53,z,1.05,.3,1.05);wheel.rotation.z=Math.PI/2;}
 part(truck,'box','body',0,3.2,-1.25,2.5,.2,2.65);for(const x of [-1.215,1.215])part(truck,'box','glass',x,2.55,-1.7,.025,.72,1.2);part(truck,'box','black',0,1.55,-3.73,1.25,.94,.025);
 const grille=new T.BufferGeometry(),bars=[];for(let i=0;i<9;i++){const x=-.6+i*.15;for(const [u,v]of [[0,0],[0,.85],[.025,0],[.025,0],[0,.85],[.025,.85]])bars.push(x+u,1.12+v,-3.755);}grille.setAttribute('position',new T.Float32BufferAttribute(bars,3));grille.computeVertexNormals();truck.add(new T.Mesh(grille,materials.steel));
 const headlights=[-1,1].map(x=>part(truck,'box','white',x,1.45,-3.72,.3,.25,.06));
 function overlayLine(group,points,colour){let segment=[];const flush=()=>{if(segment.length>1){const g=new T.BufferGeometry().setFromPoints(segment),material=new T.LineBasicMaterial({color:colour});material.userData.temporary=true;group.add(new T.Line(g,material));}segment=[];};for(const q of points){if(!q){flush();continue;}segment.push(new T.Vector3(q[0]-origin[0],(q[2]??origin[2])-origin[2]+.12,q[1]-origin[1]));}flush();}
 function updateOverlay(route,trace,signals,layers){const old=content.getObjectByName('journey-overlay');if(old){old.traverse(o=>{if(o.geometry&&!Object.values(geometry).includes(o.geometry))o.geometry.dispose();if(o.material?.userData?.temporary)o.material.dispose();});content.remove(old);}const group=new T.Group();group.name='journey-overlay';content.add(group);liveHeads=[];
  overlayLine(group,route||[],0xffae73);overlayLine(group,trace||[],0x8ed9e7);
  for(const head of signalHeads)head.state=null;liveHeads=[];unmatchedSignals=0;const matched=new Set();
  if(layers.signs)for(const signal of (signals||[]).slice(0,40)){if(signal.type!==1)continue;const [x,y,z]=signal.position;let best=null,distance=9,ambiguous=false;for(const head of signalHeads){if(matched.has(head))continue;const d=(head.world[0]-x)**2+(head.world[1]-z)**2+(head.world[2]-y)**2;if(Math.abs(d-distance)<.001&&best&&Math.abs(shortestAngle(head.angle,best.angle)-head.angle)>.1)ambiguous=true;else if(d<distance){best=head;distance=d;ambiguous=false;}}if(best&&!ambiguous){best.state=signal.state;matched.add(best);liveHeads.push(best);}else unmatchedSignals++;}

 }
 function render(time){if(disposed||contextLost||!target||canvas.hidden||!canvas.getClientRects().length||document.hidden){animation=0;return;}animation=window.requestAnimationFrame(render);if(time-lastFrame<1000/30)return;const dt=Math.min(.1,(time-lastFrame)/1000)||1/30;lastFrame=time;
  const blend=1-Math.exp(-dt*12);for(let i=0;i<3;i++)pose.p[i]+=(target.p[i]-pose.p[i])*blend;pose.angle+=(shortestAngle(pose.angle,target.angle)-pose.angle)*blend;
  const [wx,wy,wz]=pose.p,x=wx-origin[0],z=wz-origin[1],y=wy-origin[2];pose.floor+=(target.floor-pose.floor)*blend;truck.position.set(x,pose.floor-origin[2],z);truck.rotation.set(target.pitch,pose.angle,target.roll,'YXZ');
  const angle=target.north?0:pose.angle,range=target.chase?Math.max(trailer.visible?32:23,35/target.zoom):300/target.zoom;
  const shoulder=target.chase?range*.2:0;camera.position.set(x+Math.sin(angle)*range+Math.cos(angle)*shoulder,y+range*(target.chase?.45:.85),z+Math.cos(angle)*range-Math.sin(angle)*shoulder);
  camera.lookAt(x-Math.sin(angle)*range*(target.chase?.55:.18),y+1,z-Math.cos(angle)*range*(target.chase?.55:.18));
  if(lensBatch){const colour=new T.Color();for(const head of signalHeads){const active=signalColour(head.state,time);head.colours=['red','amber','green'].map(c=>c===active?colours[c]:colours.off);head.indices.forEach((index,i)=>lensBatch.setColorAt(index,colour.setHex(head.colours[i])));}lensBatch.instanceColor.needsUpdate=true;}

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
 return {update,stats(){return {...renderer.info.render,geometries:renderer.info.memory.geometries,textures:renderer.info.memory.textures,camera:target?.chase?'chase':'overview',truck:true,trailer:trailer.visible,frameCap:30,signFaces:atlas.count(),signalHousings:signalHeads.length,signalMasts:mastCount,unmatchedSignals,signals:liveHeads.map(s=>({state:s.state,angle:s.angle,colours:s.colours.slice()}))};},dispose(){disposed=true;window.cancelAnimationFrame(animation);canvas.removeEventListener('webglcontextlost',onContextLost);clear();grille.dispose();atlasTexture.dispose();panelMaterial.dispose();for(const texture of textures.values())texture.dispose();for(const material of Object.values(materials))material.dispose();for(const g of Object.values(geometry))g.dispose();renderer.dispose();}};
};
