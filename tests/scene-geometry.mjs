import assert from 'node:assert/strict';
import {strip,dashes,framePath,assetShape,signalColour,shortestAngle,mappedGround} from '../ats-dualsense/ui/scene-geometry.mjs';
const road=[[100000,200000,10],[100010,200000,12],[100020,200010,14]],origin=[100000,200000,8];
const mesh=strip(road,-4,4,origin);assert.equal(mesh.length,36);assert(mesh.every(Number.isFinite));assert.equal(mesh[1],2);assert(mesh.some((v,i)=>i%3===1&&v===6));assert(Math.max(...mesh)<30);
const bend=strip([[0,0,0],[10,0,0],[10,10,0]],-2,2,[0,0,0]);assert.deepEqual(bend.slice(3,6),bend.slice(18,21));
assert.deepEqual(strip([[0,0],[0,0]],-1,1,[0,0,0]),[]);
const paint=dashes([[0,0,0],[2,0,0],[10,0,0]],0,.1,[0,0,0]);assert(paint.every(Number.isFinite));const xs=paint.filter((_,i)=>i%3===0);assert(xs.every(x=>x<=3||x>=9));
assert.equal(assetShape('/model/building/house/test.pmd','building'),'house');assert.equal(assetShape('/model/tree/oak.pmd','vegetation'),'tree');assert.equal(assetShape('/model/lamps/street_light.pmd','prop'),'lamp');assert.equal(assetShape('/model/panorama/rest_stop.pmd','building'),'landmark');
assert.equal(signalColour(2),'red');assert.equal(signalColour(8),'green');assert.equal(signalColour(4),'amber');assert.equal(signalColour(undefined),'off');assert.equal(signalColour(32,0),'off');assert.equal(signalColour(32,500),'amber');
assert(Math.abs(shortestAngle(6.2,.1)-6.383185307179586)<1e-8);
console.log('Road elevation, connected ribbons, continuous dash gaps, local precision, proxy classification and signal states passed.');

assert(dashes([[0,0,0],[1e8,0,0]],0,.1,[0,0,0]).length<100000);

assert.equal(mappedGround([[[0,-20,0],[0,20,0]],[[0,-20,15],[0,20,15]]],[0,17,0]),15);
assert.equal(mappedGround([[[0,-20,0],[0,20,0]]],[100,2,0]),undefined);

assert.deepEqual(framePath([[0,0],[10,0],[10,10]])[1].n,[-1,1]);
