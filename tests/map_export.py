#!/usr/bin/env python3
"""Validate conversion against parser-shaped fixtures, not game-map qualification."""
import importlib.util
import json
import tempfile
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).parents[1] / 'ats-dualsense/scripts'))

spec = importlib.util.spec_from_file_location('exporter', Path(__file__).parents[1] / 'ats-dualsense/scripts/export-road-map.py')
exporter = importlib.util.module_from_spec(spec)
spec.loader.exec_module(exporter)
with tempfile.TemporaryDirectory() as directory:
    source = Path(directory)
    # Real parser UID encoding: hexadecimal strings, wider than JS safe integers.
    fixtures = {
        'nodes': [{'uid': 'ffffffffffffffff', 'x': 0, 'y': -100, 'z': 42, 'rotation': 0}, {'uid': 'fffffffffffffffe', 'x': 100, 'y': -100, 'z': 50, 'rotation': 0}],
        'roads': [{'startNodeUid': 'ffffffffffffffff', 'endNodeUid': 'fffffffffffffffe', 'roadLookToken':'test'}, {'startNodeUid': 'missing', 'endNodeUid': 'fffffffffffffffe'}],
        'roadLooks':[{'token':'test','lanesLeft':[],'lanesRight':['lane']}],
        'modelDescriptions':[{'token':'building','path':'/model/building/test.pmd','start':{'x':-2,'y':-3},'end':{'x':2,'y':3},'height':8}],
        'models':[{'token':'building','nodeUid':'ffffffffffffffff','scale':{'x':2,'y':3,'z':4}}],
        'signDescriptions':[{'token':'exit','name':'Exit','modelDesc':'/model/sign/exit.pmd','category':'road'}],
        'signs':[{'token':'exit','nodeUid':'ffffffffffffffff','textItems':['<b>Test city</b>','/def/sign/atlas/us_400.sii']}],
        'cities': [{'name': 'Test city', 'x': 50, 'y': -100}],
    }
    for kind, data in fixtures.items():
        (source / f'usa-{kind}.json').write_text(json.dumps(data))
    result, missing = exporter.export(source, 'ats')
    assert missing == 1 and len(result['roads']) == 1
    assert result['roads'][0][0] == [0,-100,42] and result['roads'][0][-1] == [100,-100,50]
    assert all(p[1] == -100 for p in result['roads'][0])
    assert result['labels'][0]['position'] == [50, -100]
    assert result['graph']['edges'][0][:2]==[0,1] and len(result['graph']['edges'])==1
    assert result['graph']['edges'][0][3][0]==[0,-100,42]
    assert result['objects'][0]==[0,0,-100,42,0,2,3,4]
    assert result['assets'][0][2:] == ['building',-2,-3,2,3,8]
    assert result['signs'][0][-1]=='Test city · US 400'
    try:
        exporter.export(source, 'ats', [200, 200, 300, 300])
    except ValueError:
        pass
    else:
        raise AssertionError('Empty region accepted')
# Connected lines merge, but a T junction stays split into three paths.
line = lambda x, y: [[x, 0], [y, 0]]
assert exporter.merge_roads([('a','b',line(0,10)),('b','c',line(10,20))]) == [[[0,0],[20,0]]]
assert len(exporter.merge_roads([('a','b',line(0,10)),('b','c',line(10,20)),('b','d',[[10,0],[10,10]])])) == 3
ring = exporter.merge_roads([('a','b',[[0,0],[10,0]]),('b','c',[[10,0],[0,10]]),('c','a',[[0,10],[0,0]])])
assert len(ring)==1 and ring[0][0]==ring[0][-1] and len(ring[0])>=4
assert exporter.simplify([[0,0],[5,3],[10,0]]) == [[0,0],[5,3],[10,0]]
print('Map export: parser arrays, 64-bit UIDs, world axes, centerline interpolation, city labels, missing nodes and region selection passed.')

from scene_map import placement, prefab_template, clean_text
import math
angle, tx=placement({'x':100,'y':200,'z':30,'rotation':math.pi/2},{'x':10,'y':20,'z':5,'rotation':0})
assert tx([11,20,7])==[100,201,32]
assert clean_text('<b>Exit</b> &amp; city')=='Exit & city'
assert clean_text('/def/sign/atlas/us_400.sii')=='US 400'
desc={'nodes':[{'x':0,'y':0,'z':0,'rotation':0},{'x':20,'y':0,'z':0,'rotation':0}],
 'mapPoints':[{'type':'polygon','color':0,'x':0,'y':0,'neighbors':[1,2]}, {'type':'polygon','color':0,'x':10,'y':0,'neighbors':[0,2]}, {'type':'polygon','color':0,'x':0,'y':10,'neighbors':[0,1]}],
 'navCurves':[{'start':{'x':0,'y':0,'rotation':0},'end':{'x':20,'y':0,'rotation':0}}],
 'navNodes':[{'type':'physical','endIndex':0,'connections':[{'targetNavNodeIndex':1,'curveIndices':[0]}]},{'type':'physical','endIndex':1,'connections':[]}]}
t=prefab_template(desc)
assert len(t['areas'])==1 and t['links'][0][:3]==[0,1,20]
assert t['curves'][0][0]==[0,0,0] and t['curves'][0][-1]==[20,0,0]
print('Scene export: rotated/elevated prefabs, ordered polygons, directed navigation curves and safe sign text passed.')
