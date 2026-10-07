import assert from 'node:assert/strict';
import {signDesign,signalDesign,signPolygon,mapFacing,groupSignalApproaches,boundsCrossRoad} from '../ats-dualsense/ui/road-furniture.mjs';
const stop=signDesign('/model/sign/traffic/us_stop_76x76.pmd','');assert.equal(stop.shape,'octagon');assert.equal(stop.width,.76);assert.equal(stop.kind,'stop');
assert.equal(signDesign('/model/sign/traffic/us_yield.pmd','').kind,'yield');
const limit=signDesign('/model/sign/traffic/us_speed_limit.pmd','55');assert.equal(limit.speed,'55');assert.equal(limit.shape,'rectangle');assert.equal(signDesign('us speed limit','').speed,null);
const guide=signDesign('/model/sign/navigation/tx_fwy1_ovh_board_g426x381_eo.pmd','Topeka · SMALL ARROW 11C · US 400');assert.equal(guide.width,4.26);assert.equal(guide.height,3.81);assert(guide.overhead&&guide.exitOnly);assert.deepEqual(guide.words,['Topeka','US 400']);
assert.deepEqual(signDesign('us hw shield','US 75').route,{type:'US',number:'75'});assert.equal(signDesign('us sharp left turn','').direction,'left');
assert(!signalDesign('gas').visible);assert(!signalDesign('crossing_c').visible);assert(signalDesign('212x111').visible);assert.equal(signalDesign('212x111').backplate,'yellow');assert.equal(signalDesign('1x1','ets2').backplate,'black');
assert(Math.abs(Math.sin(mapFacing(0))-1)<1e-8);assert(Math.abs(Math.cos(mapFacing(-Math.PI/2,true))-1)<1e-8);
for(const shape of ['octagon','triangle','triangle-up','diamond','circle','rectangle']){const p=signPolygon(shape);assert(p.every(v=>v.every(Number.isFinite)));assert(p.every(v=>v.every(x=>Math.abs(x)<=.501)));const area=p.reduce((s,a,i)=>{const b=p[(i+1)%p.length];return s+a[0]*b[1]-a[1]*b[0];},0);assert(area>0,'front face winding: '+shape);}
console.log('Road furniture: mapped design/dimensions, real text, unknown values, profile filtering, orientation and front-face winding passed.');

const groups=groupSignalApproaches([[0,0,0,0],[3.5,0,0,0],[0,0,0,Math.PI],[0,3,0,0],[0,0,3,0]]);assert.deepEqual(groups.map(g=>g.length),[2,1,1,1]);
assert(boundsCrossRoad([0,0,0,20,10,20,0],[[[-30,0,0],[30,0,0]]],[0,0,0]));assert(!boundsCrossRoad([0,0,0,20,10,20,0],[[[-30,0,20],[30,0,20]]],[0,0,0]));assert(!boundsCrossRoad([0,0,0,20,10,20,0],[[[-30,30,0],[30,30,0]]],[0,0,0]));
