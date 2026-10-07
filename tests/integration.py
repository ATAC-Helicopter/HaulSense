#!/usr/bin/env python3
"""Exercise the real sockets, packet validation, lifecycle, HTTP bounds and settings."""
import http.client
import json
import pathlib
import socket
import struct
import subprocess
import sys
import tempfile
import time

build=pathlib.Path(sys.argv[1]).resolve()
binary=build/'ats-dualsense/daemon/haulsense'

def request(method='GET',path='/api/state',body=None,headers=None):
    conn=http.client.HTTPConnection('127.0.0.1',39066,timeout=3)
    conn.request(method,path,body=body,headers=headers or {})
    response=conn.getresponse();data=response.read();code=response.status;conn.close()
    return code,json.loads(data) if path.startswith('/api') else data

def ready(process):
    for _ in range(100):
        if process.poll() is not None:raise AssertionError('Daemon exited during startup')
        try:
            if request()[0]==200:return
        except (OSError,http.client.HTTPException):pass
        time.sleep(.02)
    raise AssertionError('Dashboard did not start')

def await_state(predicate):
    deadline=time.monotonic()+3
    while time.monotonic()<deadline:
        state=request()[1]
        if predicate(state):return state
        time.sleep(.01)
    raise AssertionError(f'Expected state not reached: {state}')

def stop(process):
    process.terminate();process.wait(timeout=4)

with tempfile.TemporaryDirectory(prefix='haulsense-test-') as directory:
    config=pathlib.Path(directory)/'settings.conf';config.write_text('# preserve me\ncustom_setting=42\n')
    process=subprocess.Popen([str(binary),'--no-controller','--telemetry-port','39065','--dashboard-port','39066','--config',str(config),'--data-dir',str(pathlib.Path(directory)/'jobs')],stdout=subprocess.DEVNULL)
    try:
        ready(process)
        code,hud=request(path='/hud');assert code==200 and b'HaulSense HUD' in hud and b'/hud.js' in hud
        code,script=request(path='/navigation.js');assert code==200 and b'createNavigator' in script
        code,scene=request(path='/scene.js');assert code==200 and b'HaulSenseScene' in scene
        code,worker=request(path='/routing-worker.js');assert code==200 and b'createRoadRouter' in worker
        code,route=request(path='/api/route');assert code==200 and not route['fresh'] and route['node_uids']==[]
        code,dashboard=request(path='/');assert code==200 and b'Compact HUD' in dashboard
        packet=subprocess.check_output([str(build/'telemetry-fixture')]);udp=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
        def send(data):udp.sendto(data,('127.0.0.1',39065));time.sleep(.08)
        send(packet);code,state=request();assert code==200 and state['active'] and state['telemetry']['speed_mps']==18
        assert state['source_protocol']==7 and state['telemetry']['world_position']==[12345.125,42,-6789.5]
        assert state['telemetry']['heading']==.75
        assert state['fx']['leds']&24 and not state['fx']['leds']&3
        V6_SIZE=863
        invalid_position=bytearray(packet);struct.pack_into('<d',invalid_position,V6_SIZE-37,float('nan'))
        for bad in [packet[:-1],packet+b'X',b'bad',packet[:4]+b'\x04\x00'+packet[6:],invalid_position]:send(bad)
        await_state(lambda s:s['rejected']==5)
        v6=subprocess.check_output([str(build/'telemetry-fixture'),'--v6']);send(v6);await_state(lambda s:s['active'] and s['source_protocol']==6)
        v5=subprocess.check_output([str(build/'telemetry-fixture'),'--v5']);send(v5);await_state(lambda s:s['active'] and s['source_protocol']==5 and s['telemetry']['world_position'] is None)
        legacy=subprocess.check_output([str(build/'telemetry-fixture'),'--legacy']);send(legacy);await_state(lambda s:s['active'] and s['source_protocol']==4 and not s['availability_known'])
        send(packet[:32]+b'\x01'+packet[33:]);await_state(lambda s:not s['active'] and s['fx']['leds']==0)
        for action in ['--job-start','--job-step','--job-finish']:send(subprocess.check_output([str(build/'telemetry-fixture'),action]))
        deadline=time.monotonic()+3
        while time.monotonic()<deadline and not request(path='/api/jobs')[1]['reports']:time.sleep(.02)
        history=request(path='/api/jobs')[1];assert len(history['reports'])==1
        report=request(path='/api/jobs/'+history['reports'][0]['id'])[1];assert report['official']['revenue']==12345 and report['official']['earned_xp']==500 and len(report['route'])==3
        assert request(path='/api/jobs/../config')[0]==404
        send(packet);time.sleep(.65);assert not request()[1]['active'];assert request()[1]['fx']['brake']==0
        assert request('POST','/api/config','rumble_strength=0')[0]==403
        assert request('POST','/api/config','rumble_strength=0',{'X-HaulSense':'1','Origin':'https://example.com'})[0]==403
        headers={'X-HaulSense':'1','Content-Type':'application/x-www-form-urlencoded'}
        assert request('POST','/api/config','trigger_strength=nan',headers)[0]==400
        assert request('POST','/api/config','effects_enabled=false&rumble_strength=0',headers)[0]==200
        assert not request()[1]['config']['effects_enabled'];assert 'custom_setting=42' in config.read_text()
        assert request(headers={'Host':'evil.example'})[0]==403
        assert request(path='/missing')[0]==404
        slow=socket.create_connection(('127.0.0.1',39066));slow.sendall(b'GET / HTTP/1.1\r\nHost: ')
        send(packet);assert request()[1]['active'];slow.close()
        udp.close()
    finally:stop(process)
    process=subprocess.Popen([str(binary),'--mock','--telemetry-port','39065','--dashboard-port','39066','--config',str(config),'--data-dir',str(pathlib.Path(directory)/'jobs')],stdout=subprocess.DEVNULL)
    try:
        ready(process);time.sleep(.1);state=request()[1];assert state['demo'] and not state['controller'] and state['active']
        (build/'ui-state.json').write_text(json.dumps(state))
    finally:stop(process)
print('Integration: compact HUD, exact packet size/version, left-only LEDs, pause, timeout, config validation/preservation, CSRF/Host rejection, slow clients, hardware-free demo passed')
