/* HaulSense local schematic 3D scene. Bounds are measured; textures are not game assets. */
import * as T from 'three';
window.HaulSenseScene=function(canvas){
 const renderer=new T.WebGLRenderer({canvas,antialias:true,alpha:false,powerPreference:'low-power'});
 renderer.setPixelRatio(Math.min(window.devicePixelRatio||1,1.5));
 const scene=new T.Scene();scene.background=new T.Color('#101a23');scene.fog=new T.Fog('#101a23',350,1600);
 const camera=new T.PerspectiveCamera(48,1,1,5000);
 scene.add(new T.HemisphereLight(0xcde5f5,0x33452d,2));const sun=new T.DirectionalLight(0xffe4bd,2);sun.position.set(-100,250,80);scene.add(sun);
 let lineBatches=new Map(),roadVertices=[];
 let lastData=null,content=new T.Group(),builtKey=null;scene.add(content);
 const marker=new T.Mesh(new T.ConeGeometry(4,13,3),new T.MeshBasicMaterial({color:0xd8ee86}));marker.rotation.x=-Math.PI/2;scene.add(marker);
 const unitBox=new T.BoxGeometry(1,1,1),matrix=new T.Matrix4(),quat=new T.Quaternion(),scale=new T.Vector3(),position=new T.Vector3(),up=new T.Vector3(0,1,0);
 const materials={building:new T.MeshLambertMaterial({color:0x506574}),vegetation:new T.MeshLambertMaterial({color:0x446951}),prop:new T.MeshLambertMaterial({color:0x826d52}),road:new T.MeshLambertMaterial({color:0x374149,side:T.DoubleSide}),area:new T.MeshLambertMaterial({color:0x273831,side:T.DoubleSide}),lane:new T.MeshBasicMaterial({color:0xcab983}),signal:new T.MeshLambertMaterial({color:0x68737b})};
 const textures=new Map();
 function clear(){content.traverse(o=>{if(o.isInstancedMesh)o.dispose();if(o.geometry&&o.geometry!==unitBox)o.geometry.dispose();if(o.material?.userData?.temporary)o.material.dispose();});scene.remove(content);content=new T.Group();scene.add(content);builtKey=null;}
 function line(points,color){if(points.length<2)return;let batch=lineBatches.get(color);if(!batch){batch=[];lineBatches.set(color,batch);}for(let i=1;i<points.length;i++)batch.push(...points[i-1],...points[i]);}
 function ribbon(points,width,origin){if(points.length<2)return;const vertices=roadVertices;for(let i=1;i<points.length;i++){const a=points[i-1],b=points[i],dx=b[0]-a[0],dz=b[1]-a[1],length=Math.hypot(dx,dz);if(length<.01)continue;const ox=-dz/length*width/2,oz=dx/length*width/2;const v=[[a[0]+ox-origin[0],(a[2]??origin[2])-origin[2],a[1]+oz-origin[1]],[a[0]-ox-origin[0],(a[2]??origin[2])-origin[2],a[1]-oz-origin[1]],[b[0]+ox-origin[0],(b[2]??origin[2])-origin[2],b[1]+oz-origin[1]],[b[0]-ox-origin[0],(b[2]??origin[2])-origin[2],b[1]-oz-origin[1]]];for(const index of [0,2,1,1,2,3])vertices.push(...v[index]);}}
 function area(points,kind,origin){if(points.length<3)return;const shape=new T.Shape();points.forEach((p,i)=>{const x=p[0]-origin[0],z=-(p[1]-origin[1]);i?shape.lineTo(x,z):shape.moveTo(x,z);});const g=new T.ShapeGeometry(shape);g.rotateX(-Math.PI/2);const mesh=new T.Mesh(g,kind===0?materials.road:materials.area);mesh.position.y=(points[0][2]??origin[2])-origin[2]-.1;content.add(mesh);}
 function label(text,p,color=0x96b8a5){if(!text)return;const key=String(text).slice(0,100);let texture=textures.get(key);if(!texture){const c=document.createElement('canvas');c.width=512;c.height=96;const x=c.getContext('2d');x.fillStyle='#19342cee';x.fillRect(0,0,512,96);x.strokeStyle='#c2d0b2';x.lineWidth=3;x.strokeRect(2,2,508,92);x.fillStyle='#eef5dc';x.font='bold 26px system-ui';x.textAlign='center';x.textBaseline='middle';x.fillText(key,256,48,480);texture=new T.CanvasTexture(c);textures.set(key,texture);while(textures.size>96){const oldest=textures.keys().next().value;textures.get(oldest).dispose();textures.delete(oldest);}}const mat=new T.SpriteMaterial({map:texture,color,depthTest:true});mat.userData.temporary=true;const sprite=new T.Sprite(mat);sprite.position.set(...p);sprite.scale.set(22,4.2,1);content.add(sprite);}
 function boxes(rows,material){if(!rows.length)return;const mesh=new T.InstancedMesh(unitBox,material,rows.length);rows.forEach((r,i)=>{position.set(r[0],r[1]+r[4]/2,r[2]);quat.setFromAxisAngle(up,-r[6]);scale.set(Math.max(.1,r[3]),Math.max(.1,r[4]),Math.max(.1,r[5]));matrix.compose(position,quat,scale);mesh.setMatrixAt(i,matrix);});mesh.instanceMatrix.needsUpdate=true;mesh.computeBoundingSphere();content.add(mesh);}
 function build(data,visible,origin,layers){clear();lineBatches=new Map();roadVertices=[];
  for(const i of visible.roads){const points=data.roads[i],style=data.roadStyles?.[i],width=style?Math.max(3.5,(style[0]+style[1])*3.5+style[2]+style[3]+style[4]):7;ribbon(points,width,origin);if(layers.details)line(points.map(p=>[p[0]-origin[0],(p[2]??origin[2])-origin[2]+.15,p[1]-origin[1]]),0x8f8b70);}
  const groups={building:[],prop:[],vegetation:[]};
  if(layers.props)for(const i of visible.objects.slice(0,2200)){const o=data.objects[i],a=data.assets[o[0]],cx=(a[3]+a[5])/2*o[5],cz=(a[4]+a[6])/2*o[6],cs=Math.cos(o[4]),sn=Math.sin(o[4]);groups[a[2]].push([o[1]+cx*cs-cz*sn-origin[0],o[3]-origin[2],o[2]+cx*sn+cz*cs-origin[1],Math.abs(a[5]-a[3])*Math.abs(o[5]),Math.abs(a[7]*o[7]),Math.abs(a[6]-a[4])*Math.abs(o[6]),o[4]]);}
  const poles=[];let labels=0;
  for(const i of visible.prefabs){const instance=data.prefabs[i],template=data.templates[instance[0]],cs=Math.cos(instance[4]),sn=Math.sin(instance[4]);const tx=p=>[instance[1]+p[0]*cs-p[1]*sn,instance[2]+p[0]*sn+p[1]*cs,instance[3]+p[2]];
   for(const polygon of template.areas)area(polygon[1].map(tx),polygon[0],origin);
   for(const l of template.lines){const points=[tx(l[0]),tx(l[1])],lanes=(l[2]??1)+(l[3]??1);ribbon(points,Math.max(3.5,lanes*3.5+(l[4]||0)),origin);}
   if(layers.details)for(const curve of template.curves)line(curve.map(p=>{const q=tx(p);return [q[0]-origin[0],q[2]-origin[2]+.2,q[1]-origin[1]];}),0x7e968b);
   if(layers.signs){for(const sign of template.signs){const p=tx(sign);poles.push([p[0]-origin[0],p[2]-origin[2],p[1]-origin[1],.3,3,.3,0]);}for(const signal of template.signals){const p=tx(signal);poles.push([p[0]-origin[0],p[2]-origin[2],p[1]-origin[1],.3,4,.3,0]);poles.push([p[0]-origin[0],p[2]-origin[2]+3.4,p[1]-origin[1],.5,1.2,.4,0]);}}
  }
  if(layers.signs)for(const i of visible.signs.slice(0,180)){const sign=data.signs[i],name=sign[5]||data.signDefinitions[sign[4]]?.[1]||'Sign';const x=sign[0]-origin[0],z=sign[1]-origin[1];poles.push([x,sign[2]-origin[2],z,.25,3,.25,sign[3]]);if(Math.hypot(x,z)<220&&labels++<35)label(name,[x,sign[2]-origin[2]+5,z]);}
  boxes(poles,materials.signal);for(const kind of Object.keys(groups))boxes(groups[kind],materials[kind]);
  if(layers.details)for(const i of (visible.pois||[]).slice(0,30)){const poi=data.pois[i];if(Math.hypot(poi[0]-origin[0],poi[1]-origin[1])<600)label(poi[4]||poi[2],[poi[0]-origin[0],12,poi[1]-origin[1]],0xcdd39d);}
  if(layers.details)for(const i of visible.barriers){const b=data.barriers[i];line(b[0].map(p=>[p[0]-origin[0],p[2]-origin[2]+1,p[1]-origin[1]]),0xb6a489);}
  if(roadVertices.length){const g=new T.BufferGeometry();g.setAttribute('position',new T.Float32BufferAttribute(roadVertices,3));g.computeVertexNormals();content.add(new T.Mesh(g,materials.road));}
  for(const [color,vertices]of lineBatches){const g=new T.BufferGeometry();g.setAttribute('position',new T.Float32BufferAttribute(vertices,3));const m=new T.LineBasicMaterial({color});m.userData.temporary=true;content.add(new T.LineSegments(g,m));}
  lineBatches.clear();roadVertices=[];
 }
 let origin=null,contextLost=false;
 function update({data,visible,position:p,heading,north,zoom,layers,route,trace,signals,width,height}){
  if(contextLost)throw Error('WebGL context lost');
  renderer.setSize(width,height,false);camera.aspect=width/height;camera.updateProjectionMatrix();
  if(!data||!p){clear();renderer.render(scene,camera);return;}
  const key=[Math.floor(p[0]/80),Math.floor(p[2]/80),Math.round(zoom*10),JSON.stringify(layers),data.name].join(':');
  if(data!==lastData||key!==builtKey){origin=[p[0],p[2],p[1]-2];build(data,visible,origin,layers);builtKey=key;lastData=data;}
  const x=p[0]-origin[0],z=p[2]-origin[1],y=p[1]-origin[2];
  marker.position.set(x,y+3,z);marker.rotation.z=heading*2*Math.PI;
  const angle=north?0:heading*2*Math.PI,range=300/zoom;camera.position.set(x+Math.sin(angle)*range*.8,y+range*.85,z+Math.cos(angle)*range*.8);camera.lookAt(x-Math.sin(angle)*range*.18,y,z-Math.cos(angle)*range*.18);
  // Replace only the dynamic overlay, keeping static instance buffers bounded.
  const old=content.getObjectByName('journey-overlay');if(old){old.traverse(o=>{o.geometry?.dispose();o.material?.dispose();});content.remove(old);}
  const overlay=new T.Group();overlay.name='journey-overlay';content.add(overlay);
  function overlayLine(points,color){let segment=[];const flush=()=>{if(segment.length>1){const g=new T.BufferGeometry().setFromPoints(segment);const material=new T.LineBasicMaterial({color});material.userData.temporary=true;overlay.add(new T.Line(g,material));}segment=[];};for(const q of points){if(!q){flush();continue;}segment.push(new T.Vector3(q[0]-origin[0],(q[2]??p[1]-2)-origin[2]+.5,q[1]-origin[1]));}flush();}
  if(layers.signs)for(const signal of signals||[]){if(signal.type!==1)continue;const [sx,sy,sz]=signal.position;if(Math.hypot(sx-p[0],sz-p[2])>500)continue;const color=signal.state===2?0xff6666:signal.state===8?0x77ee77:[1,4].includes(signal.state)?0xffbb66:signal.state===32&&Math.floor(Date.now()/500)%2?0xffbb66:0x65747b;
   const material=new T.MeshBasicMaterial({color});material.userData.temporary=true;const marker=new T.Mesh(new T.SphereGeometry(1.8,8,6),material);marker.position.set(sx-origin[0],sy-origin[2]+4,sz-origin[1]);overlay.add(marker);
  }
  overlayLine(route||[],0xff9b65);overlayLine(trace||[],0x8ed9e7);
  renderer.render(scene,camera);
 }
 const onContextLost=e=>{e.preventDefault();contextLost=true;builtKey=null;};canvas.addEventListener('webglcontextlost',onContextLost);
 return {update,stats(){return {...renderer.info.render,geometries:renderer.info.memory.geometries,textures:renderer.info.memory.textures};},dispose(){canvas.removeEventListener('webglcontextlost',onContextLost);clear();for(const texture of textures.values())texture.dispose();for(const material of Object.values(materials))material.dispose();unitBox.dispose();marker.geometry.dispose();marker.material.dispose();renderer.dispose();}};
};
