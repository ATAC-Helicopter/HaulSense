import {createRequire} from 'node:module';
const {parseHTML}=createRequire(import.meta.url)('linkedom');
import fs from 'node:fs';
import vm from 'node:vm';
import assert from 'node:assert/strict';
const html=fs.readFileSync(new URL('../ats-dualsense/ui/hud.html',import.meta.url),'utf8');
const {window,document}=parseHTML(html);
for(const input of document.querySelectorAll('input,select'))Object.defineProperty(input,'value',{value:'',writable:true});
const context=vm.createContext({document,window,console,Number,String,Math,Object,JSON,AbortSignal,localStorage:{getItem:()=>null,setItem(){}},setTimeout(){},fetch:async()=>({ok:true,json:async()=>({active:false,telemetry:{}})})});
vm.runInContext(html.split('<script>')[1].split('</script>')[0],context);
await new Promise(resolve=>setTimeout(resolve,10));
const el=id=>document.getElementById(id);
vm.runInContext("state={active:true,telemetry:{speed_mps:25,nav_distance:54593,nav_time:2079.87,displayed_gear:8,fuel_range:888,left_blinker:true,left_blinker_light:false}};render()",context);
assert.equal(el('city').textContent,'Percorso GPS');
assert.equal(el('trip').textContent,'54.6 km · 35 min');
assert.equal(el('speed').textContent,'90');
assert.equal(el('limit').textContent,'—');
assert.ok(!el('left').classList.contains('on')); // lamp phase, not permanent logical state
assert.match(el('info').textContent,/Marcia 8 · Autonomia 888 km/);
vm.runInContext("state.telemetry.left_blinker_light=true;state.telemetry.right_blinker_light=true;state.telemetry.right_blinker=false;render()",context);
assert.ok(el('left').classList.contains('on'));assert.ok(!el('right').classList.contains('on')); // shared clock must not create a second direction

vm.runInContext("state.telemetry.destination='San José';state.telemetry.nav_distance=null;state.telemetry.nav_time=null;render()",context);
assert.equal(el('city').textContent,'San José');assert.equal(el('trip').textContent,'—');
vm.runInContext("state.telemetry.destination='';state.telemetry.nav_distance=0;state.telemetry.nav_time=0;render()",context);
assert.equal(el('city').textContent,'Nessun percorso');assert.equal(el('trip').textContent,'—');
vm.runInContext("state.telemetry.nav_distance=160934.4;state.telemetry.nav_time=7260;state.telemetry.fuel_warning=true;state.telemetry.hazards=true;state.telemetry.left_blinker_light=null;state.telemetry.right_blinker_light=null;document.getElementById('units').value='us';render()",context);
assert.equal(el('trip').textContent,'100.0 mi · 2 h 01 min');assert.equal(el('speed').textContent,'56');
assert.equal(el('warning').hidden,false);assert.ok(el('left').classList.contains('on'));assert.ok(el('right').classList.contains('on'));
vm.runInContext("state.active=false;state.paused=true;render()",context);
assert.equal(el('city').textContent,'—');assert.equal(el('trip').textContent,'—');assert.equal(el('speed').textContent,'—');assert.equal(el('status').textContent,'PAUSA');assert.equal(el('warning').hidden,true);
vm.runInContext("state.active=true;state.demo=true;render()",context);assert.equal(el('status').textContent,'DEMO');
console.log('HUD DOM: GPS without job, job city, no route, null channels, US units, long ETA, lamp phase, hazards, warnings, pause and demo passed.');
