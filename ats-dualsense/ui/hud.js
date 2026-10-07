
'use strict';
const $=id=>document.getElementById(id);
const saved=key=>{try{return localStorage.getItem('haulsense-hud-'+key)}catch{return null}};
const remember=(key,value)=>{try{localStorage.setItem('haulsense-hud-'+key,value)}catch{}};
const number=(value,factor=1)=>Number.isFinite(value)?Math.round(Math.abs(value)*factor):null;
let state=null;
function render(){
 const s=state,live=!!s?.active,t=live?s.telemetry:{},metric=$('units').value==='metric',factor=metric?3.6:2.236936;
 const speed=number(t.speed_mps,factor),limit=t.nav_speed_limit>0?number(t.nav_speed_limit,factor):null;
 $('status').textContent=live?(s.demo?'DEMO':'LIVE'):s?.paused?'PAUSA':'ATTESA';$('status').classList.toggle('live',live);
 $('speed').textContent=speed??'—';$('speedunit').textContent=metric?'km/h':'mph';$('speed').classList.toggle('over',limit!==null&&speed!==null&&speed>limit+2);
 $('limit').textContent=limit??'—';$('limit').classList.toggle('empty',limit===null);
 const routed=!!t.destination||t.nav_distance>0||t.nav_time>0;
 $('route-label').textContent=t.destination?'Destinazione incarico':'Navigazione';
 $('city').textContent=t.destination||(routed?'Percorso GPS':live?'Nessun percorso':'—');
 $('city').title=t.destination||'Il SDK fornisce il nome della città soltanto per un incarico.';
 $('truck').textContent=live?`${t.truck_brand||''} ${t.truck_name||''}`.trim()||'Camion connesso':'In attesa del camion';
 const dist=Number.isFinite(t.nav_distance)?(t.nav_distance/(metric?1000:1609.344)).toFixed(1):null;
 const mins=Number.isFinite(t.nav_time)?Math.round(t.nav_time/60):null;
 const duration=mins===null?'':mins>=60?`${Math.floor(mins/60)} h ${String(mins%60).padStart(2,'0')} min`:`${mins} min`;
 $('trip').textContent=routed?[dist===null?'':dist+(metric?' km':' mi'),duration].filter(Boolean).join(' · ')||'—':'—';
 const gear=t.displayed_gear,gearText=gear==null?'—':gear<0?'R'+Math.abs(gear):gear===0?'N':String(gear);
 const info=['Marcia '+gearText];if(Number.isFinite(t.fuel_range))info.push('Autonomia '+Math.round(t.fuel_range/(metric?1:1.609344))+(metric?' km':' mi'));
 if(t.cruise_speed>0)info.push('Cruise '+number(t.cruise_speed,factor));$('info').textContent=live?info.join(' · '):'—';
 const warnings=[['air_emergency','PRESSIONE ARIA'],['fuel_warning','RISERVA'],['water_warning','TEMPERATURA'],['oil_warning','OLIO'],['battery_warning','BATTERIA'],['parking_brake','FRENO A MANO']].filter(([key])=>t[key]).map(([,label])=>label);
 $('warning').textContent=warnings.join(' · ');$('warning').hidden=!warnings.length;
 const lampPhase=t.left_blinker_light==null&&t.right_blinker_light==null?true:!!(t.left_blinker_light||t.right_blinker_light);
 for(const side of ['left','right'])$(side).classList.toggle('on',live&&!!(t[side+'_blinker']||t.hazards)&&lampPhase);
}
function appearance(){document.documentElement.style.setProperty('--panel',String(Number($('opacity').value)/100));document.documentElement.style.fontSize=(16*Number($('scale').value)/100)+'px';}
$('units').value=saved('units')==='us'?'us':'metric';$('opacity').value=saved('opacity')||'70';$('scale').value=saved('scale')||'100';appearance();
for(const id of ['units','opacity','scale'])$(id).addEventListener('input',()=>{remember(id,$(id).value);appearance();render()});
$('settings').addEventListener('click',()=>{const open=!$('options').open;$('options').open=open;$('settings').setAttribute('aria-expanded',String(open))});
async function poll(){try{const response=await fetch('/api/state',{cache:'no-store',signal:AbortSignal.timeout(900)});if(!response.ok)throw Error('Disconnected');const next=await response.json();state=next&&typeof next.telemetry==='object'&&next.telemetry?next:null}catch{state=null}render();setTimeout(poll,document.hidden?1200:state?.active?250:700)}poll();
