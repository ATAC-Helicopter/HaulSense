"""Game-derived scene instances and directed road/prefab graph.

Coordinates are [world X, world Z, elevation]. Instances preserve model bounds,
not original meshes/textures. Templates avoid duplicating every intersection.
"""
import html
import json
import math
import re


def clean_text(value):
    if value.startswith('/'):
        if '/atlas/' in value:
            return value.rsplit('/', 1)[-1].split('.')[0].replace('_', ' ').upper()
        return ''
    return html.unescape(re.sub(r'<[^>]*>', '', value)).strip()[:120]


def placement(anchor, origin):
    angle = anchor['rotation'] - origin['rotation']
    cs, sn = math.cos(angle), math.sin(angle)
    def transform(p):
        dx, dz = p[0]-origin['x'], p[1]-origin['y']
        return [round(anchor['x']+dx*cs-dz*sn, 2), round(anchor['y']+dx*sn+dz*cs, 2), round(anchor.get('z', 0)+(p[2] if len(p)>2 else 0)-origin.get('z', 0), 2)]
    return angle, transform


def sampled_curve(a, b, steps=4):
    length = math.hypot(b['x']-a['x'], b['y']-a['y'])
    result = []
    for i in range(steps+1):
        t = i/steps
        h = (2*t**3-3*t*t+1, -2*t**3+3*t*t, t**3-2*t*t+t, t**3-t*t)
        result.append([round(h[0]*a[k]+h[1]*b[k]+length*(h[2]*f(a['rotation'])+h[3]*f(b['rotation'])), 2) for k, f in [('x',math.cos),('y',math.sin)]] + [round(a.get('z',0)*(1-t)+b.get('z',0)*t,2)])
    return result


def prefab_template(desc):
    points = desc.get('mapPoints', [])
    lines, polygons, visited = [], [], set()
    for i, p in enumerate(points):
        if p['type'] != 'road':
            continue
        for j in p.get('neighbors', []):
            if not 0 <= j < len(points) or points[j]['type'] != 'road' or (min(i,j),max(i,j)) in visited:
                continue
            visited.add((min(i,j),max(i,j)))
            q = points[j]
            def count(v):return v if isinstance(v, int) else None
            left, right = count(p.get('lanesLeft')), count(p.get('lanesRight'))
            lines.append([[p['x'],p['y'],p.get('z',0)],[q['x'],q['y'],q.get('z',0)],left,right,p.get('offset',0)])
    # Polygon map points form boundary loops. Follow their explicit adjacency;
    # never fill an unordered point cloud or connect distinct components.
    used = set()
    for i, p in enumerate(points):
        if p['type'] != 'polygon' or i in used:
            continue
        indices, prev, current = [], None, i
        for _ in range(len(points)+1):
            if current in indices:
                break
            indices.append(current);used.add(current)
            nxt=[n for n in points[current].get('neighbors',[]) if 0<=n<len(points) and points[n]['type']=='polygon' and points[n].get('color')==p.get('color') and n!=prev]
            if not nxt:break
            prev,current=current,next((n for n in nxt if n not in indices),nxt[0])
            if current==i:break
        if len(indices)>=3 and current==i:
            polygons.append([p.get('color',0),[[points[j]['x'],points[j]['y'],points[j].get('z',0)] for j in indices]])
    curves=[sampled_curve(c['start'],c['end']) for c in desc.get('navCurves',[])]
    navnodes=desc.get('navNodes',[])
    links=[]
    # Expand AI navigation nodes until another physical entrance is reached.
    for source, nav in enumerate(navnodes):
        if nav['type']!='physical':continue
        queue=[(source,[])];seen={source}
        while queue:
            index, path=queue.pop(0)
            for connection in navnodes[index].get('connections',[]):
                target=connection['targetNavNodeIndex'];segments=path+connection.get('curveIndices',[])
                if not 0<=target<len(navnodes) or any(c<0 or c>=len(curves) for c in segments):continue
                node=navnodes[target]
                if node['type']=='physical':
                    if node['endIndex']==nav['endIndex']:continue
                    length=sum(math.dist(a,b) for c in segments for a,b in zip(curves[c],curves[c][1:]))
                    if length>0:links.append([nav['endIndex'],node['endIndex'],round(length,2),segments])
                elif target not in seen:
                    seen.add(target);queue.append((target,segments))
    return {'nodes':[[p['x'],p['y'],p.get('z',0),p['rotation']] for p in desc['nodes']], 'lines':lines,'areas':polygons,'curves':curves,'links':links,
            'signs':[[p['x'],p['y'],p.get('z',0),p['rotation'],p['model'],p.get('part','')] for p in desc.get('signs',[])],
            'signals':[[p['x'],p['y'],p.get('z',0),p['rotation'],p.get('profile','')] for p in desc.get('semaphores',[])]}


def scene_export(source, prefix, nodes, region=None):
    missing=[]
    def read(kind):
        path=source/f'{prefix}-{kind}.json'
        if not path.exists():missing.append(kind);return []
        data=json.loads(path.read_text())
        if not isinstance(data,list):raise ValueError(f'{kind} must be an array')
        return data
    def selected(x,z):return not region or region[0]<=x<=region[2] and region[1]<=z<=region[3]
    graph_nodes=[];graph_ids={}
    def graph_node(uid):
        uid=str(uid)
        if uid not in nodes:return None
        if uid not in graph_ids:
            n=nodes[uid];graph_ids[uid]=len(graph_nodes);graph_nodes.append([round(n['x'],2),round(n['y'],2),round(n.get('z',0),2),uid])
        return graph_ids[uid]
    looks={a['token']:a for a in read('roadLooks')}
    edges=[]
    for r in read('roads'):
        if r.get('hidden') or r.get('secret'):continue
        a=nodes.get(str(r['startNodeUid']));b=nodes.get(str(r['endNodeUid']))
        if not a or not b or region and not (selected(a['x'],a['y']) or selected(b['x'],b['y'])):continue
        ia,ib=graph_node(r['startNodeUid']),graph_node(r['endNodeUid'])
        look=looks.get(r.get('roadLookToken'));length=max(math.dist(graph_nodes[ia][:3],graph_nodes[ib][:3]),r.get('length',0))
        # An absent road look cannot establish driveable direction.
        if not look:continue
        geometry=sampled_curve(a,b,8)
        length=max(length,sum(math.dist(p,q) for p,q in zip(geometry,geometry[1:])))
        if look.get('lanesRight'):edges.append([ia,ib,round(length,2),geometry])
        if look.get('lanesLeft'):edges.append([ib,ia,round(length,2),geometry[::-1]])
    descriptions=read('prefabDescriptions');templates=[prefab_template(d) for d in descriptions];template_ids={d['token']:i for i,d in enumerate(descriptions)}
    instances=[];unmapped=0
    for p in read('prefabs'):
        if p.get('hidden') or p.get('secret') or not selected(p['x'],p['y']):continue
        ti=template_ids.get(p['token']);anchor=nodes.get(str(p['nodeUids'][0])) if p['nodeUids'] else None
        if ti is None or not anchor:unmapped+=1;continue
        desc=descriptions[ti]
        if not 0<=p['originNodeIndex']<len(desc['nodes']):unmapped+=1;continue
        origin=desc['nodes'][p['originNodeIndex']];angle,tx=placement(anchor,origin)
        base=tx([0,0,0]);mapping=[]
        candidates=[(uid,nodes[str(uid)]) for uid in p['nodeUids'] if str(uid) in nodes]
        for local in templates[ti]['nodes']:
            world=tx(local);match=min(candidates,key=lambda pair:math.hypot(world[0]-pair[1]['x'],world[1]-pair[1]['y']),default=None)
            mapping.append(graph_node(match[0]) if match and math.hypot(world[0]-match[1]['x'],world[1]-match[1]['y'])<2 else -1)
        instances.append([ti,*base,round(angle,6),mapping])
    assets=[];asset_ids={}
    for d in read('modelDescriptions'):
        path=d['path'];kind='building' if '/building/' in path or '/panorama/' in path else 'vegetation' if any(s in path for s in ('tree','vegetation','bush')) else 'prop'
        asset_ids[d['token']]=len(assets)
        assets.append([d['token'],path,kind,round(d['start']['x'],2),round(d['start']['y'],2),round(d['end']['x'],2),round(d['end']['y'],2),round(d['height'],2)])
    objects=[];missing_models=0
    for obj in read('models'):
        n=nodes.get(str(obj['nodeUid']));asset=asset_ids.get(obj['token'])
        if not n or asset is None:missing_models+=1;continue
        if not selected(n['x'],n['y']):continue
        scale=obj['scale'];objects.append([asset,round(n['x'],2),round(n['y'],2),round(n.get('z',0),2),round(n['rotation'],6),round(scale['x'],4),round(scale['y'],4),round(scale['z'],4)])
    sign_defs=read('signDescriptions');definitions=[[d['token'],d['name'],d['modelDesc'],d['category']] for d in sign_defs];def_ids={d['token']:i for i,d in enumerate(sign_defs)}
    signs=[]
    for sign in read('signs'):
        n=nodes.get(str(sign['nodeUid']))
        if not n or not selected(n['x'],n['y']):continue
        words=[clean_text(s) for s in sign.get('textItems',[])];label=' · '.join(x for x in words if x)[:180]
        signs.append([round(n['x'],2),round(n['y'],2),round(n.get('z',0),2),round(n['rotation'],6),def_ids.get(sign['token'],-1),label])
    barriers=[]
    for divider in read('dividers'):
        a=nodes.get(str(divider['startNodeUid']));b=nodes.get(str(divider['endNodeUid']))
        if a and b and (selected(a['x'],a['y']) or selected(b['x'],b['y'])):
            barriers.append([sampled_curve(a,b),divider.get('scheme') or ', '.join(x['model'] for x in divider.get('subcurves',[]))])
    pois=[]
    for poi in read('pois'):
        if selected(poi['x'],poi['y']):pois.append([round(poi['x'],2),round(poi['y'],2),poi.get('type',''),poi.get('icon',''),poi.get('label','')])
    return {'templates':templates,'prefabs':instances,'assets':assets,'objects':objects,'signDefinitions':definitions,'signs':signs,'barriers':barriers,'pois':pois,
            'graph':{'nodes':graph_nodes,'edges':edges},'coverage':{'missingFiles':missing,'missingPrefabs':unmapped,'missingModels':missing_models,'meshRepresentation':'measured model bounds','signalState':'unknown'}}
