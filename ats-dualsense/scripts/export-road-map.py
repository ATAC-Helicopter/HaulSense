#!/usr/bin/env python3
"""Convert TruckSim Maps parser output to a bounded local HaulSense road layer.

Roads, prefab lanes/surfaces, signs, measured model bounds and directed routing. Node x/y are the parser's map plane, corresponding to SDK x/z.
UIDs remain strings so 64-bit identifiers never lose precision.
"""
import argparse
import json
import math
from pathlib import Path
from scene_map import scene_export


def curve(a, b, steps=5, elevation=False):
    start, end = (a['x'], a['y']), (b['x'], b['y'])
    length = math.dist(start, end)
    angles = (a.get('rotation', 0), b.get('rotation', 0))
    tangents = [(length * math.cos(r), length * math.sin(r)) for r in angles]
    points = []
    for i in range(steps + 1):
        t = i / steps
        weights = (2*t**3-3*t**2+1, -2*t**3+3*t**2, t**3-2*t**2+t, t**3-t**2)
        points.append([round(sum(w*p[axis] for w, p in zip(weights, (start, end, *tangents))), 2) for axis in range(2)])
    if elevation:
        for i, point in enumerate(points):point.append(round(a.get('z',0)*(1-i/steps)+b.get('z',0)*i/steps,2))
    return points


def simplify(points, tolerance=1.0):
    # Iterative Ramer-Douglas-Peucker; retain endpoints and bound world error.
    if len(points) <= 2:
        return points
    keep = {0, len(points)-1}
    pending = [(0, len(points)-1)]
    while pending:
        first, last = pending.pop()
        base = points[first]
        delta = [b-a for a,b in zip(base,points[last])]
        length2 = sum(d*d for d in delta)
        furthest, index = 0, None
        for i in range(first+1, last):
            offset = [b-a for a,b in zip(base,points[i])]
            t = min(1,max(0,sum(p*d for p,d in zip(offset,delta))/length2)) if length2 else 0
            error = sum((p-t*d)**2 for p,d in zip(offset,delta))
            if error > furthest:
                furthest,index = error,i
        if index is not None and furthest > tolerance*tolerance:
            keep.add(index)
            pending.extend(((first, index), (index, last)))
    return [points[i] for i in sorted(keep)]


def merge_roads(edges, return_styles=False):
    # Join only degree-two endpoints. Never bridge prefab gaps or junctions.
    neighbors = {}
    for i, edge in enumerate(edges):
        a,b=edge[:2]
        neighbors.setdefault(a, []).append(i)
        neighbors.setdefault(b, []).append(i)
    visited = set()
    result, styles = [], []
    def walk(start, edge):
        points = []
        node = start
        first_style = None
        while edge not in visited:
            a,b,line=edges[edge][:3]
            style=edges[edge][3] if len(edges[edge])>3 else None
            if style is not None and node!=a:style=(style[1],style[0],style[2],style[4],style[3])
            if points and style!=first_style:break
            first_style=style
            visited.add(edge)
            segment = line if node == a else list(reversed(line))
            points.extend(segment if not points else segment[1:])
            node = b if node == a else a
            links = neighbors[node]
            if len(links) != 2:
                break
            remaining = [e for e in links if e not in visited]
            if not remaining:
                break
            edge = remaining[0]
        result.append(simplify(points));styles.append(first_style)
    for node, links in neighbors.items():
        if len(links) != 2:
            for edge in links:
                if edge not in visited:
                    walk(node, edge)
    for i, edge in enumerate(edges):
        a=edge[0]
        if i not in visited:
            walk(a, i)
    return (result,styles) if return_styles else result


def export(source, game, region=None):
    prefix = 'usa' if game == 'ats' else 'europe'
    def read(kind):
        data = json.loads((source / f'{prefix}-{kind}.json').read_text())
        if not isinstance(data, list):
            raise ValueError(f'{kind} must be a parser JSON array')
        return data
    nodes = {str(n['uid']): n for n in read('nodes')}
    looks_path=source/f'{prefix}-roadLooks.json'
    looks={x['token']:x for x in json.loads(looks_path.read_text())} if looks_path.exists() else {}
    edges, missing = [], 0
    for r in read('roads'):
        if r.get('hidden') or r.get('secret'):
            continue
        a, b = nodes.get(str(r['startNodeUid'])), nodes.get(str(r['endNodeUid']))
        if not a or not b:
            missing += 1
            continue
        points = curve(a,b,elevation=True)
        if region:
            left, top, right, bottom = region
            if max(p[0] for p in points) < left or min(p[0] for p in points) > right or max(p[1] for p in points) < top or min(p[1] for p in points) > bottom:
                continue
        look=looks.get(r.get('roadLookToken'),{})
        style=(len(look.get('lanesLeft',[])),len(look.get('lanesRight',[])),look.get('offset',0),look.get('shoulderSpaceLeft',0),look.get('shoulderSpaceRight',0))
        edges.append((str(r['startNodeUid']),str(r['endNodeUid']),points,style))
    roads,styles = merge_roads(edges,True)
    labels = []
    for city in read('cities'):
        x, z = city['x'], city['y']
        if region and not (region[0] <= x <= region[2] and region[1] <= z <= region[3]):
            continue
        labels.append({'position': [x, z], 'name': str(city.get('nameLocalized') or city['name'])[:100]})
    if len(roads) > 150000 or sum(map(len, roads)) > 1500000 or len(labels) > 2000:
        raise ValueError('Map exceeds browser limits; use --bounds to export a region')
    if not roads:
        raise ValueError('No roads in export; check source and bounds')
    result = {'version': 2, 'game': game, 'coordinates': 'scs-xz-metres', 'name': f'{game.upper()} · extracted roads', 'roads': roads, 'roadStyles':styles, 'labels':labels}
    result.update(scene_export(source, prefix, nodes, region))
    # Reject invalid source coordinates before emitting a file accepted by the UI.
    for p in [p for road in roads for p in road] + [l['position'] for l in labels]:
        if any(not math.isfinite(v) or abs(v) >= 1e8 for v in p):
            raise ValueError('Invalid world coordinates')
    return result, missing


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path, help='TruckSim Maps parser output directory')
    parser.add_argument('output', type=Path, help='HaulSense map JSON to create')
    parser.add_argument('--game', choices=('ats', 'ets2'), required=True)
    parser.add_argument('--bounds', type=float, nargs=4, metavar=('MIN_X', 'MIN_Z', 'MAX_X', 'MAX_Z'))
    args = parser.parse_args()
    try:
        if args.bounds and (not all(math.isfinite(v) for v in args.bounds) or args.bounds[0] >= args.bounds[2] or args.bounds[1] >= args.bounds[3]):
            raise ValueError('Bounds must be finite and ordered')
        data, missing = export(args.source, args.game, args.bounds)
        encoded = json.dumps(data, ensure_ascii=False, separators=(',', ':'), allow_nan=False)
        if len(encoded.encode()) > 96 * 1024 * 1024:
            raise ValueError('Map exceeds 96 MiB; use --bounds')
        args.output.write_text(encoded + '\n')
    except (OSError, ValueError, KeyError, TypeError) as error:
        parser.exit(1, f'Export failed: {error}\n')
    print(f"Exported {len(data['roads'])} road centerlines and {len(data['labels'])} city labels; {missing} roads skipped for missing nodes. Scene: {len(data['prefabs'])} prefabs, {len(data['objects'])} model bounds, {len(data['signs'])} signs.")


if __name__ == '__main__':
    main()
