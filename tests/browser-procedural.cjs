// Rendered regression with original synthetic geometry. No game data required.
const {chromium}=require('playwright'),assert=require('node:assert/strict');
(async()=>{
 const browser=await chromium.launch({headless:true,executablePath:process.env.PLAYWRIGHT_CHROMIUM_EXECUTABLE_PATH||undefined,args:['--enable-unsafe-swiftshader']});
 try{
 const page=await browser.newPage({viewport:{width:1500,height:1000}}),errors=[];page.on('pageerror',e=>errors.push(e.message));let active=true,state=8,x=1.75,z=0;
 const map={version:2,game:'ats',coordinates:'scs-xz-metres',name:'DEMO · original synthetic road diorama',roads:[[[0,200,0],[0,-55,0]],[[0,-85,0],[0,-600,0]],[[-150,-70,0],[-15,-70,0]],[[15,-70,0],[150,-70,0]]],roadStyles:Array(4).fill([1,1,0,1,1]),labels:[],graph:{nodes:[],edges:[]},assets:[['a','model/building/house/demo.pmd','building',-5,-4,5,4,5],['b','model/vegetation/tree/demo.pmd','vegetation',-2,-2,2,2,6],['c','model/street_light/demo.pmd','prop',-.3,-.3,.3,.3,6]],objects:[[0,24,-34,0,0,1,1,1],[0,-28,-46,0,0,1,1,1],[1,-14,-18,0,0,1,1,1],[1,-15,-35,0,0,1,1,1],[1,16,-105,0,0,1,1,1],[2,8,-24,0,0,1,1,1]],templates:[{nodes:[],lines:[],areas:[[0,[[-15,-85,0],[15,-85,0],[15,-55,0],[-15,-55,0]]]],curves:[],links:[],signs:[],signals:[[7,-48,0,0,'demo']]}],prefabs:[[0,0,0,0,0,[]]],signDefinitions:[['stop','STOP','demo','demo'],['nav','HaulSense avenue','demo','demo']],signs:[[7,-53,0,0,0,'STOP'],[-8,-28,0,0,1,'HaulSense avenue']],barriers:[[[[-6,20,0],[-6,-45,0]],'symbolic divider']],pois:[]};
 await page.route('**/api/state',async route=>{const s=await(await route.fetch()).json();s.active=active;s.demo=false;s.telemetry={...s.telemetry,game:'ats',world_position:[x,1,z],heading:0,pitch:0,roll:0,trailer_connected:true,destination:''};await route.fulfill({json:s});});
 await page.route('**/api/signals',route=>route.fulfill({json:{source:'ets2la',available:true,fresh:active,objects:[{type:1,state,position:[7,0,-48],remaining_s:12,id:1}]}}));
 await page.goto(process.argv[2]||'http://127.0.0.1:39076');await page.waitForFunction(()=>document.querySelector('#status').textContent!=='Connecting');
 await page.evaluate(map=>navigatorView.loadMap(map),map);
 await page.waitForFunction(()=>navigatorView.stats()?.frame>2&&navigatorView.stats()?.signals.some(s=>s.state===8));
 await page.evaluate(()=>{document.querySelector('#status').textContent='DEMO · synthetic road / signal fixtures';});
 await page.screenshot({path:'build/procedural-chase-demo.png'});const first=await page.evaluate(()=>navigatorView.stats());assert.equal(first.camera,'chase');assert(first.truck&&first.trailer);assert.equal(first.frameCap,30);assert.equal(first.signals[0].colours[2],0x83efb3);
 x=2;z=-10;await page.waitForTimeout(600);await page.locator('#map-camera').click();await page.waitForFunction(()=>navigatorView.stats()?.camera==='overview');await page.screenshot({path:'build/procedural-overview-demo.png'});
 await page.reload();await page.waitForFunction(()=>navigatorView.stats()?.camera==='overview');await page.locator('#map-camera').click();await page.waitForFunction(()=>navigatorView.stats()?.camera==='chase');
 const memories=[];for(let i=0;i<12;i++){state=i%2?2:8;await page.waitForFunction(expected=>navigatorView.stats()?.signals.some(s=>s.state===expected),state);memories.push(await page.evaluate(()=>navigatorView.stats()));}
 assert(Math.max(...memories.map(s=>s.geometries))<=first.geometries+5);assert(Math.max(...memories.map(s=>s.textures))<=first.textures+2);
 active=false;await page.waitForFunction(()=>document.querySelector('#road-scene').hidden);const paused=await page.evaluate(()=>navigatorView.stats().frame);await page.waitForTimeout(300);assert.equal(await page.evaluate(()=>navigatorView.stats().frame),paused);
 active=true;await page.waitForFunction(()=>!document.querySelector('#road-scene').hidden);await page.waitForTimeout(300);assert((await page.evaluate(()=>navigatorView.stats().frame))>paused);
 assert.deepEqual(errors,[]);console.log('Original fixture: chase/overview, persistent camera, pause/resume, lamps, bounded geometry/textures passed',first);
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
