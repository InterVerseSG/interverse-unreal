"""Fetch mapped green polygons for InterVerseSG from OpenStreetMap only.

Included when mapped as closed ways:
- landuse=grass
- landcover=grass
- natural=grassland
- leisure=garden
- leisure=park
- leisure=pitch (kept separate as recreation_green)

No green area geometry is inferred or invented.
"""
from __future__ import annotations
import json, pathlib, time, urllib.parse, urllib.request, xml.etree.ElementTree as ET

ROOT=pathlib.Path(__file__).resolve().parents[1]
SOURCES=ROOT/'Data'/'campus_geometry_sources.json'
OUTPUT=ROOT/'Data'/'campus_green_areas.geojson'
OSM_WAY_FULL='https://api.openstreetmap.org/api/0.6/way/{way_id}/full'
OVERPASS='https://overpass-api.de/api/interpreter'
UA='InterVerseSG-green-areas/0.1 (https://github.com/InterVerseSG/interverse-unreal)'

def request(url,data=None,ctype=None):
    h={'User-Agent':UA,'Accept':'application/json, application/xml'}
    if ctype:h['Content-Type']=ctype
    with urllib.request.urlopen(urllib.request.Request(url,data=data,headers=h),timeout=60) as r:return r.read()

def boundary_id():
    data=json.loads(SOURCES.read_text(encoding='utf-8'))
    for x in data.get('sources',[]):
        if x.get('id')=='CampusMain' and x.get('source')=='OpenStreetMap':return int(x['osm_id'])
    raise RuntimeError('CampusMain OSM boundary missing')

def bbox(way_id,margin=0.0010):
    root=ET.fromstring(request(OSM_WAY_FULL.format(way_id=way_id)))
    pts=[(float(n.attrib['lat']),float(n.attrib['lon'])) for n in root.findall('node')]
    lats=[p[0] for p in pts];lons=[p[1] for p in pts]
    return min(lats)-margin,min(lons)-margin,max(lats)+margin,max(lons)+margin

def main():
    bid=boundary_id(); south,west,north,east=bbox(bid); box=f'{south},{west},{north},{east}'
    q=f'''[out:json][timeout:40];(
 way["landuse"="grass"]({box});
 way["landcover"="grass"]({box});
 way["natural"="grassland"]({box});
 way["leisure"="garden"]({box});
 way["leisure"="park"]({box});
 way["leisure"="pitch"]({box});
);out body geom;'''
    time.sleep(1)
    payload=urllib.parse.urlencode({'data':q}).encode()
    data=json.loads(request(OVERPASS,payload,'application/x-www-form-urlencoded').decode())
    features=[]
    for e in data.get('elements',[]):
        if e.get('type')!='way':continue
        geom=e.get('geometry') or []
        coords=[(float(p['lon']),float(p['lat'])) for p in geom if 'lon'in p and 'lat'in p]
        if len(coords)<4 or coords[0]!=coords[-1]:continue
        tags=e.get('tags') or {}
        cat='recreation_green' if tags.get('leisure')=='pitch' else 'green'
        features.append({'type':'Feature','properties':{'id':f"osm_way_{e['id']}",'category':cat,'source':'OpenStreetMap','osm_id':int(e['id']),'osm_tags':tags,'geometry_status':'verified_from_overpass'},'geometry':{'type':'Polygon','coordinates':[coords]}})
    out={'type':'FeatureCollection','name':'InterVerseSG mapped green areas','metadata':{'source':'OpenStreetMap','campus_boundary_osm_way':bid,'policy':'Only mapped closed OSM ways; no inferred green areas.'},'features':features}
    OUTPUT.write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(f'Wrote {len(features)} mapped green polygons -> {OUTPUT}')
if __name__=='__main__':main()
