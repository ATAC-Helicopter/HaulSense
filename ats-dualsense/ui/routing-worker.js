/* Directed local routing. Worker never changes the game's GPS or controls. */
'use strict';
function createRoadRouter(data) {
 const nodes=data.graph.nodes, base=data.graph.edges;
 const count=nodes.length;if(!count||count>1000000||base.length>2000000)throw Error('Invalid graph size.');
 const edges=base.map(e=>[Number(e[0]),Number(e[1]),Number(e[2]),-1,-1]);
 const tx=(p,instance)=>{const [_,x,z,y,a]=instance,c=Math.cos(a),s=Math.sin(a);return [x+p[0]*c-p[1]*s,z+p[0]*s+p[1]*c,y+(p[2]||0)];};
 for(let i=0;i<(data.prefabs||[]).length;i++){
  const instance=data.prefabs[i],template=data.templates[instance[0]],mapping=instance[5];
  for(let j=0;j<template.links.length;j++){const link=template.links[j],a=mapping[link[0]],b=mapping[link[1]];if(a>=0&&b>=0)edges.push([a,b,Math.max(link[2],Math.hypot(nodes[a][0]-nodes[b][0],nodes[a][1]-nodes[b][1])),i,j]);}
 }
 const incoming=new Uint32Array(count),starts=new Uint32Array(count+1);
 for(const e of edges){const from=Number(e[0]),to=Number(e[1]);if(!Number.isSafeInteger(from)||!Number.isSafeInteger(to)||from<0||to<0||from>=count||to>=count||!Number.isFinite(e[2])||e[2]<0)throw Error('Invalid graph edge.');starts[from+1]++;incoming[to]++;}
 for(let i=1;i<starts.length;i++)starts[i]+=starts[i-1];
 const destinations=new Uint32Array(edges.length),costs=new Float32Array(edges.length),refs=new Uint32Array(edges.length),cursor=starts.slice();
 edges.forEach((e,i)=>{const from=Number(e[0]),j=Number(cursor[from]++);destinations[j]=e[1];costs[j]=e[2];refs[j]=i;});
 const uidIndex=new Map(nodes.map((n,i)=>[n[3],i]));
 const buckets=new Map(),step=500;
 nodes.forEach((n,i)=>{const key=Math.floor(n[0]/step)+','+Math.floor(n[1]/step);if(!buckets.has(key))buckets.set(key,[]);buckets.get(key).push(i);});
 function nearest(point,needOutgoing=true){const x=Math.floor(point[0]/step),z=Math.floor(point[1]/step);let best=-1,distance=Infinity;for(let radius=0;radius<=8;radius++){for(let dx=-radius;dx<=radius;dx++)for(let dz=-radius;dz<=radius;dz++){if(radius&&Math.abs(dx)!==radius&&Math.abs(dz)!==radius)continue;for(const i of buckets.get((x+dx)+','+(z+dz))||[]){if(needOutgoing?starts[i]===starts[i+1]:!incoming[i])continue;const d=Math.hypot(nodes[i][0]-point[0],nodes[i][1]-point[1]);if(d<distance){distance=d;best=i;}}}if(distance<radius*step)break;}return {index:best,distance};}
 function edgePoints(ref){const e=edges[ref];if(e[3]<0)return base[ref][3]||[nodes[e[0]].slice(0,3),nodes[e[1]].slice(0,3)];const instance=data.prefabs[e[3]],template=data.templates[instance[0]],link=template.links[e[4]];return link[3].flatMap(id=>template.curves[id].map(p=>tx(p,instance)));}
 function join(path){const result=[];for(const e of path){const segment=edgePoints(e);for(const p of segment){const last=result.at(-1);if(!last||Math.hypot(last[0]-p[0],last[1]-p[1])>.05)result.push(p);}}return result;}
 function route(start,end){
  const a=nearest(start),b=nearest(end,false);
  if(a.index<0||b.index<0||a.distance>750||b.distance>2000)return {error:'Destination or truck is too far from mapped roads.'};
  const goal=b.index,dist=new Float64Array(count);dist.fill(Infinity);dist[a.index]=0;
  const previous=new Int32Array(count);previous.fill(-1);const closed=new Uint8Array(count),heap=[];
  const heuristic=i=>Math.hypot(nodes[i][0]-nodes[goal][0],nodes[i][1]-nodes[goal][1]);
  function push(i,value){heap.push([value,i]);let k=heap.length-1;while(k>0){const parent=(k-1)>>1;if(heap[parent][0]<=value)break;heap[k]=heap[parent];k=parent;}heap[k]=[value,i];}
  function pop(){const top=heap[0],last=heap.pop();if(heap.length){let k=0;while(k*2+1<heap.length){let child=k*2+1;if(child+1<heap.length&&heap[child+1][0]<heap[child][0])child++;if(heap[child][0]>=last[0])break;heap[k]=heap[child];k=child;}heap[k]=last;}return top[1];}
  push(a.index,heuristic(a.index));let visits=0;
  while(heap.length&&visits<count){const i=pop();if(closed[i])continue;closed[i]=1;visits++;if(i===goal)break;for(let j=starts[i];j<starts[i+1];j++){const to=destinations[j],candidate=dist[i]+costs[j];if(candidate<dist[to]){dist[to]=candidate;previous[to]=refs[j];push(to,candidate+heuristic(to));}}}
  if(!Number.isFinite(dist[goal]))return {error:'No connected driveable route found in this map.'};
  const path=[];let i=goal;while(i!==a.index){const ref=previous[i];if(ref<0||path.length>count)return {error:'Invalid route graph.'};path.push(ref);i=edges[ref][0];}
  path.reverse();return {points:join(path),mapDistance:dist[goal],startOffset:a.distance,endOffset:b.distance,visits,source:'haulsense'};
 }
 function resolve(uids){const indexes=uids.map(uid=>uidIndex.get(uid));if(indexes.some(i=>i===undefined))return {error:'GPS nodes do not match the loaded game map.'};const path=[];for(let j=1;j<indexes.length;j++){const a=indexes[j-1],b=indexes[j];if(a===b)continue;let found=-1;for(let k=starts[a];k<starts[a+1];k++)if(destinations[k]===b){found=refs[k];break;}if(found<0)return {error:'GPS route has unmapped connections; no synthetic links were added.'};path.push(found);}return {points:join(path),source:'ets2la'};}
 return {route,resolve,nearest,count,edgeCount:edges.length};
}
if(typeof module!=='undefined')module.exports={createRoadRouter};
else {let router=null;self.onmessage=e=>{if(e.origin!==''&&e.origin!==self.location.origin)return;const m=e.data;if(!m||typeof m!=='object'||!['load','gps','route'].includes(m.type))return;try{if(m.type==='load'){router=createRoadRouter(m.map);self.postMessage({type:'ready',nodes:router.count,edges:router.edgeCount});}else if(router)self.postMessage({type:'result',id:m.id,...(m.type==='gps'?router.resolve(m.uids):router.route(m.start,m.end))});}catch(error){self.postMessage({type:'error',id:m.id,error:error.message});}};}
