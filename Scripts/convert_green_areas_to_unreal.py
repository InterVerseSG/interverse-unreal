"""Convert OSM green polygons from WGS84 to local Unreal centimeters."""
from __future__ import annotations
import json, math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
INPUT=ROOT/'Data'/'campus_green_areas.geojson'
OUTPUT=ROOT/'Config'/'InterVerseGreenAreas.local.json'
ANCHORS=ROOT/'Config'/'InterVerseCampusAnchors.json'
R=6_378_137.0

def origin():
    o=json.loads(ANCHORS.read_text(encoding='utf-8'))['origin']
    return float(o['longitude']),float(o['latitude'])

def point(lon,lat,olon,olat):
    east=R*math.cos(math.radians(olat))*math.radians(lon-olon)
    north=R*math.radians(lat-olat)
    return [round(east*100,2),round(north*100,2),0.0]

def main():
    if not INPUT.exists():raise SystemExit(f'Missing {INPUT}; run fetch_osm_green_areas.py first.')
    src=json.loads(INPUT.read_text(encoding='utf-8')); olon,olat=origin(); out=[]
    for f in src.get('features',[]):
        g=f.get('geometry') or {}
        if g.get('type')!='Polygon':continue
        rings=[]
        for ring in g.get('coordinates',[]):
            rings.append([point(float(p[0]),float(p[1]),olon,olat) for p in ring])
        out.append({'type':'Feature','properties':f.get('properties') or {},'geometry':{'type':'Polygon','coordinates_cm':rings}})
    result={'version':'0.1.0','source':'Data/campus_green_areas.geojson','coordinate_system':'local Unreal centimeters','origin':{'longitude':olon,'latitude':olat,'poi_id':'MarquisScienceHall'},'features':out}
    OUTPUT.write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(f'Converted {len(out)} green polygons -> {OUTPUT}')
if __name__=='__main__':main()
