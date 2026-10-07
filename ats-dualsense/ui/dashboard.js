
'use strict';
const $=id=>document.getElementById(id), clamp=(v,min,max)=>Math.max(min,Math.min(max,v));
let state=null,initialized=false,metric=true,view='cockpit',timer=null,dirty=false;
const history=[],rawRows=new Map();
const navigatorView=createNavigator({document,window});
const jobView=createJobView({document,window});
metric=window.HaulSenseStorage?.settings().units!=='us';$('units').value=metric?'metric':'us';
const sliders=[['rumble_strength','Event rumble','Gear changes, starts and short acknowledgements.',0,1,.01],['road_strength','Road impacts','Transient suspension and cabin feedback.',0,1,.01],['trigger_strength','Adaptive triggers','Progressive L2 brake resistance; subtle R2 end cue.',0,1,.01],['brake_haptic_strength','Heavy brake texture','Restrained pulses only while braking hard and moving.',0,1,.01],['lightbar_strength','Lightbar brightness','Scale every colour without changing its meaning.',0,1,.01],['beacon_strength','Beacon glow','Slow gold breath when the truck beacon is enabled.',0,1,.01],['bump_threshold','Road sensitivity threshold','Higher values ignore smaller road movements.',0,1,.01],['indicator_step_ms','Indicator sweep delay','Time between inner and outer player LEDs.',60,300,5]];
const toggles=[['effects_enabled','Controller effects','Pause or resume all light, trigger and rumble effects.'],['gameplay_cue','Gameplay acknowledgements','Short delivery, fine, toll and transport cues.'],['trailer_cue','Trailer coupling cue','Short tactile acknowledgement on connection or release.'],['reverse_cue','Low-speed reverse cue','A sparse pulse while reversing below 14 km/h.'],['wiper_cue','Wiper rhythm','Very soft synthetic rhythm when wipers are on.'],['overspeed_warning','Speed-limit cue','Subtle violet lightbar when over the limit by 15 km/h.'],['critical_lightbar_warnings','Critical warnings','Reserve red for low-air emergencies, critical wear and engine faults.'],['swap_indicators','Swap left and right','Reverse directional masks on independent revisions; mirrored revisions use HUD-only turns.']];
for(const [key,title,hint,min,max,step]of sliders){const row=document.createElement('div');row.className='slider-row';row.innerHTML=`<label for="${key}">${title}<p>${hint}</p></label><output id="out-${key}"></output><input id="${key}" name="${key}" type="range" min="${min}" max="${max}" step="${step}">`;$('sliders').append(row);$(key).addEventListener('input',()=>{dirty=true;output(key);$('save-status').textContent='Unsaved changes';});}
for(const [key,title,hint]of toggles){const row=document.createElement('div');row.className='toggle-row';row.innerHTML=`<input id="${key}" name="${key}" type="checkbox"><label for="${key}">${title}<p>${hint}</p></label>`;$('toggles').append(row);$(key).addEventListener('change',()=>{dirty=true;$('save-status').textContent='Unsaved changes';});}
function output(key){$('out-'+key).textContent=key==='indicator_step_ms'?$(key).value+' ms':Math.round(Number($(key).value)*100)+'%';}
function loadSettings(config){for(const [k]of sliders){$(k).value=config[k];output(k);}for(const [k]of toggles)$(k).checked=!!config[k];}
function text(id,value){$(id).textContent=value;}
function val(v,d=0){return v==null?'—':Number(v).toFixed(d);}
function unit(v,u,d=0){return val(v,d)+(v==null?'':' '+u);}
function speed(v){return v==null?null:Math.abs(v)*(metric?3.6:2.236936);}
function distance(v){return v==null?null:v/(metric?1000:1609.344);}
function temperature(v){return v==null?null:metric?v:v*1.8+32;}
function pressure(v){return v==null?null:metric?v*.0689476:v;}
function pill(id,on){$(id).classList.toggle('on',!!on);}
function width(id,value){$(id).style.width=clamp(Number(value)||0,0,100)+'%';}
function render(s){
 navigatorView.update(s,metric);jobView.units(metric);jobView.refresh();
 state=s;const ready=s.active,t=ready?s.telemetry:{},fx=s.fx||{};
 $('connection').className='badge '+(s.demo?'demo':ready?'live':'');text('status',s.demo?'DEMO · simulated truck':ready?'Telemetry live':s.paused?'Game paused':'Waiting for truck');
 text('hardware','DualSense · '+s.transport);text('mute',s.config.effects_enabled?'Pause effects':'Resume effects');
 if(!initialized||!dirty){loadSettings(s.config);initialized=true;}
 text('truck-name',ready?(t.truck_brand+' '+t.truck_name).trim()||'Your truck':'Waiting for your truck');
 text('cargo-info',ready?(t.cargo?`${t.cargo} · ${val(t.cargo_mass/1000,1)} t`:'Free driving · no job information'):'Launch ATS and enter the cab to start telemetry.');
 text('route',ready?(t.destination?(t.origin||'Origin')+' → '+t.destination:t.nav_distance>0||t.nav_time>0?'GPS route · no active job':'No route selected'):'—');
 text('route-detail',ready?`${unit(distance(t.nav_distance),metric?'km':'mi',1)} remaining · ${t.nav_time==null?'—':Math.round(t.nav_time/60)+' min'} game ETA`:'Distance and ETA come directly from the game.');
 text('speed',val(speed(t.speed_mps)));text('speed-unit',metric?'km/h':'mph');$('speed').classList.toggle('over',ready&&t.nav_speed_limit>0&&Math.abs(t.speed_mps)>t.nav_speed_limit+1);
 text('gear',t.displayed_gear==null?'—':t.displayed_gear<0?'R'+Math.abs(t.displayed_gear):t.displayed_gear===0?'N':t.displayed_gear);text('rpm',val(t.rpm));text('limit',t.nav_speed_limit>0?val(speed(t.nav_speed_limit)):'—');
 text('rpm-limit',unit(t.rpm_limit,'RPM'));width('rpm-bar',t.rpm_limit>0?t.rpm/t.rpm_limit*100:0);text('cruise',t.cruise_speed>0?'Cruise · '+unit(speed(t.cruise_speed),metric?'km/h':'mph'):'Cruise off');
 pill('state-engine',t.engine_enabled);pill('state-trailer',t.trailer_connected);pill('state-park',t.parking_brake);pill('state-beam',t.low_beam||t.high_beam||t.parking_lights);
 const lampPhase=t.left_blinker_light==null&&t.right_blinker_light==null?true:!!(t.left_blinker_light||t.right_blinker_light);
 $('signal-l').classList.toggle('on',ready&&!!(t.hazards||t.left_blinker)&&lampPhase);$('signal-r').classList.toggle('on',ready&&!!(t.hazards||t.right_blinker)&&lampPhase);
 for(const id of ['truck-light-l','truck-light-r'])$(id).setAttribute('fill',t.high_beam?'#edf7ff':t.low_beam?'#b9d8e6':'#324957');
 for(const led of document.querySelectorAll('[data-led]'))led.classList.toggle('on',ready&&!!(fx.leds&Number(led.dataset.led)));
 $('lightbar').style.background=ready?`rgb(${fx.rgb.join(',')})`:'#172733';
 text('fx-state',s.config.effects_enabled?(ready?'Enabled':'Neutral'):'Paused');text('led-status',s.demo?'Preview only · demo does not write to the controller.':!s.controller?'Connect a DualSense to feel these effects.':s.player_led_layout==='mirrored'?'Mirrored player LEDs · turn indicators in HUD only':s.player_led_layout==='independent'?'Independent player LEDs · ready':s.leds_available?'Player LED revision unknown · physical verification required':'Player LEDs unavailable. Check udev permissions.');
 for(const [id,v,max]of [['brake',fx.brake,8],['throttle',fx.throttle,8],['low',fx.low,255],['high',fx.high,255]])width('fx-'+id,ready?v/max*100:0);
 const warnings=[];for(const [key,message]of [['air_emergency','Air-pressure emergency'],['air_warning','Low brake air'],['oil_warning','Low oil pressure'],['water_warning','Coolant temperature warning'],['battery_warning','Battery warning'],['fuel_warning','Low fuel'],['adblue_warning','Low AdBlue']])if(t[key]&&(!['oil_warning','battery_warning'].includes(key)||t.engine_enabled))warnings.push(message);
 if(t.max_wear>=.85)warnings.push('Critical component wear');if(t.parking_brake&&Math.abs(t.speed_mps)>2)warnings.push('Moving with parking brake');
 $('warnings').classList.toggle('attention',warnings.length>0);text('warnings',!ready?'Waiting or paused · all effects neutral.':warnings.length?warnings.join(' · '):'Systems nominal · '+(t.retarder_level?'retarder '+t.retarder_level:'retarder off')+' · '+(t.engine_brake?'engine brake on':'engine brake off')+' · '+(t.differential_lock?'differential locked':'differential unlocked'));
 text('fuel',unit(t.fuel==null?null:metric?t.fuel:t.fuel/3.785411784,metric?'L':'gal',1));
 const economy=t.fuel_consumption>0?(metric?unit(t.fuel_consumption*100,'L/100 km',1):unit(2.35214583/t.fuel_consumption,'mpg',1)):'—';
 text('fuel-info',unit(t.fuel_range==null?null:metric?t.fuel_range:t.fuel_range/1.609344,metric?'km range':'mi range')+' · '+economy);
 text('air',unit(pressure(t.brake_air_pressure),metric?'bar':'psi',1));text('brake-temp','Brakes · '+unit(temperature(t.brake_temperature),metric?'°C':'°F'));
 text('water',unit(temperature(t.water_temperature),metric?'°C':'°F'));text('oil-temp','Oil · '+unit(temperature(t.oil_temperature),metric?'°C':'°F'));
 text('battery',unit(t.battery_voltage,'V',1));text('oil-pressure','Oil pressure · '+unit(pressure(t.oil_pressure),metric?'bar':'psi',1));
 for(const key of ['throttle','brake','clutch']){width('pedal-'+key,t[key]*100);}
 for(const key of ['throttle','brake','clutch'])text('value-'+key,t[key]==null?'—':Math.round(t[key]*100)+'%');
 $('steering-marker').style.left=clamp(50+(t.steering||0)*48,2,98)+'%';text('steering-value',t.steering==null?'Steering · —':`Steering · ${Math.round(Math.abs(t.steering)*100)}% ${t.steering<0?'left':t.steering>0?'right':''}`);
 const wear=$('wear');if(!wear.children.length)for(const [k,title]of [['engine','Engine'],['transmission','Transmission'],['cabin','Cabin'],['chassis','Chassis'],['wheels','Tyres']]){const row=document.createElement('div');row.className='wear-row';row.innerHTML=`<span>${title}</span><b id="wear-${k}" class="number"></b>`;wear.append(row);}
 for(const k of ['engine','transmission','cabin','chassis','wheels'])text('wear-'+k,t['wear_'+k]==null?'—':Math.round((1-t['wear_'+k])*100)+'%');
 const wheels=t.wheels||[];text('wheel-count',wheels.length?`${wheels.length} wheels`:'No wheel data');
 if($('wheels').children.length!==wheels.length){$('wheels').replaceChildren();wheels.forEach((w,i)=>{const row=document.createElement('div');row.className='wheel';row.innerHTML=`<span>W${i+1}</span><span></span>`;$('wheels').append(row);});}
 wheels.forEach((w,i)=>{const row=$('wheels').children[i];row.classList.toggle('air',w.ground===false);row.lastChild.textContent=w.ground===false?'Airborne':unit(w.suspension,'m',3);});
 text('health',`${s.source_protocol===4?'Legacy plugin · restart ATS for extended data':s.demo?'Demo':ready?'Frame age '+s.age_ms+' ms':'Neutral'} · ${s.rejected} rejected packets · local only`);
 if(view==='channels')renderChannels(s);
}
function renderChannels(s){const t=s.telemetry;let present=0;for(const [key,value]of Object.entries(t)){
 if(value!=null)present++;
 if(!rawRows.has(key)){const row=document.createElement('tr'),name=document.createElement('td'),data=document.createElement('td');name.textContent=key;row.append(name,data);$('channel-rows').append(row);rawRows.set(key,row);}
 const row=rawRows.get(key);row.classList.toggle('missing',value==null||!s.active);row.lastChild.textContent=!s.active?'Stale / paused':value==null?'Unavailable':typeof value==='object'?JSON.stringify(value):typeof value==='number'?Number(value.toFixed(4)):String(value);row.hidden=!key.includes($('channel-filter').value.toLowerCase());
 }text('channel-count',`${s.availability_known===false?'Legacy availability inferred · ':''}${present} available values / ${Object.keys(t).length} fields · scalar, vector, configuration and wheel data`);}
function draw(){const canvas=$('history'),box=canvas.getBoundingClientRect(),ratio=window.devicePixelRatio||1;canvas.width=Math.round(box.width*ratio);canvas.height=90*ratio;const c=canvas.getContext('2d');c.scale(ratio,ratio);const w=box.width,h=90;c.strokeStyle='#26333e';c.lineWidth=1;for(let y=15;y<h;y+=25){c.beginPath();c.moveTo(0,y);c.lineTo(w,y);c.stroke();}for(const [index,color]of [[0,'#d8ee86'],[1,'#8ed9e7']]){c.strokeStyle=color;c.lineWidth=1.5;c.beginPath();let started=false;history.forEach(point=>{const v=point.values;if(v==null){started=false;return;}const x=w*clamp((point.time-(Date.now()-24000))/24000,0,1),y=h-clamp(v[index],0,1)*(h-8)-4;if(!started){c.moveTo(x,y);started=true;}else c.lineTo(x,y);});c.stroke();}}
async function save(body){const response=await fetch('/api/config',{method:'POST',headers:{'X-HaulSense':'1','Content-Type':'application/x-www-form-urlencoded'},body});const config=await response.json();if(!response.ok)throw Error(config.error||'Could not save settings');return config;}
$('settings').addEventListener('submit',async e=>{e.preventDefault();text('save-status','Saving…');try{const body=[...sliders.map(([k])=>k+'='+$(k).value),...toggles.map(([k])=>k+'='+$(k).checked)].join('&');const config=await save(body);loadSettings(config);dirty=false;text('save-status','Saved & applied · '+new Date().toLocaleTimeString());}catch(e){text('save-status',e.message);}});
$('mute').addEventListener('click',async()=>{if(!state)return;try{const config=await save('effects_enabled='+!state.config.effects_enabled);state.config=config;if(!dirty)$('effects_enabled').checked=config.effects_enabled;render(state);}catch(e){text('save-status',e.message);text('status','Could not save');}});
for(const button of document.querySelectorAll('[data-tab]'))button.addEventListener('click',()=>{view=button.dataset.tab;for(const b of document.querySelectorAll('[data-tab]'))b.setAttribute('aria-selected',String(b===button));for(const id of ['cockpit','tuning','channels','jobs'])$(id).hidden=id!==view;if(state)render(state);if(view==='cockpit'){draw();navigatorView.draw();}if(view==='jobs'){jobView.refresh(true);jobView.drawRoute();}});
$('channel-filter').addEventListener('input',()=>{if(state)renderChannels(state);});$('units').addEventListener('change',()=>{metric=$('units').value==='metric';window.HaulSenseStorage?.remember({units:metric?'metric':'us'});if(state)render(state);});
const profiles={calm:[.15,.18,.45,.18,.40,.15,.60,150],balanced:[.22,.28,.66,.38,.68,.24,.46,120],immersive:[.36,.45,.75,.50,.75,.35,.38,105]};
for(const button of document.querySelectorAll('[data-profile]'))button.addEventListener('click',()=>{profiles[button.dataset.profile].forEach((value,i)=>{$(sliders[i][0]).value=value;output(sliders[i][0]);});dirty=true;text('save-status',button.textContent+' profile selected · save to apply');});
async function poll(){if(document.hidden&&window.location.protocol!=='haulsense:'){timer=setTimeout(poll,1000);return;}try{const response=await fetch('/api/state',{cache:'no-store',signal:AbortSignal.timeout(2500)});if(!response.ok)throw Error('Disconnected');const s=await response.json();render(s);history.push({time:Date.now(),values:s.active?[Math.abs(s.telemetry.speed_mps||0)*3.6/130,(s.telemetry.rpm||0)/(s.telemetry.rpm_limit||2500)]:null});while(history.length>120||history[0]?.time<Date.now()-24000)history.shift();if(view==='cockpit')draw();timer=setTimeout(poll,s.active?200:1000);}catch(e){text('status','Service disconnected');text('warnings','Service unavailable. Check that haulsense.service is running.');$('connection').className='badge';if(state){state.active=false;render(state);text('status','Service disconnected');text('warnings','Service unavailable. Check that haulsense.service is running.');}timer=setTimeout(poll,2000);}}
document.addEventListener('visibilitychange',()=>{clearTimeout(timer);if(!document.hidden){history.length=0;poll();}});window.addEventListener('resize',()=>{if(view==='cockpit')draw();});poll();
