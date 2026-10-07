/* Original, texture-free geometry rules. No game meshes are decoded or copied. */
export function framePath(input) {
 const points=input.filter((p,i)=>!i||Math.hypot(p[0]-input[i-1][0],p[1]-input[i-1][1])>.001);
 return points.map((p,i)=>{
  const a=points[Math.max(0,i-1)],b=points[Math.min(points.length-1,i+1)];
  const length=Math.hypot(b[0]-a[0],b[1]-a[1])||1;
  return {p,n:[-(b[1]-a[1])/length,(b[0]-a[0])/length]};
 });
}
export function strip(input,left,right,origin,rise=0) {
 const frames=framePath(input),result=[];
 for(let i=1;i<frames.length;i++){
  const vertices=[];
  for(const f of [frames[i-1],frames[i]])for(const offset of [left,right])vertices.push([f.p[0]+f.n[0]*offset-origin[0],(f.p[2]??origin[2])-origin[2]+rise,f.p[1]+f.n[1]*offset-origin[1]]);
  for(const j of [0,2,1,1,2,3])result.push(...vertices[j]);
 }
 return result;
}
export function dashes(input,offset,width,origin,dash=3,gap=6) {
 const result=[];let distance=0,segments=0;
 for(let i=1;i<input.length;i++){
  const a=input[i-1],b=input[i],length=Math.hypot(b[0]-a[0],b[1]-a[1]);if(length<.001)continue;
  let at=0;
  while(at<length&&segments++<8192){const phase=(distance+at)%(dash+gap),end=Math.min(length,at+(phase<dash?dash-phase:dash+gap-phase));
   if(end-at<.00001){at+=.00001;continue;}
   if(phase<dash){const point=t=>[a[0]+(b[0]-a[0])*t,a[1]+(b[1]-a[1])*t,(a[2]??origin[2])+((b[2]??origin[2])-(a[2]??origin[2]))*t];result.push(...strip([point(at/length),point(end/length)],offset-width/2,offset+width/2,origin,.045));}
   at=end;
  }
  distance+=length;
 }
 return result;
}
export function assetShape(path,kind){
 const name=path.toLowerCase();
 if(kind==='vegetation')return /bush|shrub/.test(name)?'bush':'tree';
 if(/panorama/.test(name))return 'landmark';
 if(/house|residential|cottage/.test(name))return 'house';
 if(/lamp|street.?light|light.?pole/.test(name))return 'lamp';
 if(/guard|barrier|fence/.test(name))return 'barrier';
 if(/bollard|post/.test(name))return 'bollard';
 return kind==='building'?'building':'prop';
}
export function signalColour(state,time=0){
 return state===2?'red':state===8?'green':[1,4].includes(state)||state===32&&Math.floor(time/500)%2?'amber':'off';
}
export function shortestAngle(from,to){return from+Math.atan2(Math.sin(to-from),Math.cos(to-from));}
export function mappedGround(roads,position) {
 let best=Infinity,height;
 for(const points of roads)for(let i=1;i<points.length;i++){
  const a=points[i-1],b=points[i];if(a[2]===undefined||b[2]===undefined)continue;
  const dx=b[0]-a[0],dz=b[1]-a[1],length=dx*dx+dz*dz;
  const t=length?Math.max(0,Math.min(1,((position[0]-a[0])*dx+(position[2]-a[1])*dz)/length)):0;
  const y=a[2]+(b[2]-a[2])*t,distance=(position[0]-a[0]-dx*t)**2+(position[2]-a[1]-dz*t)**2,vertical=Math.abs(y-position[1]);
  const score=distance+vertical*vertical;if(distance<=100&&vertical<=5&&score<best){best=score;height=y;}
 }
 return height;
}
