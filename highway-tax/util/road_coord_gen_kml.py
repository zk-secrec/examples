# usage: python3 road_coord_gen_kml.py <EPSG code for the target area> <y lower bound> <y upper bound> <x lower bound> <x upper bound> <input KML file> <output JSON> <error>
# example: python3 road_coord_gen_kml.py 3301 6375274 6658862 282560 751542 $HOME/Provenance/roads/example2/trt-tln-mnt.kml example_data/trt-tln/public.json 200

import sys
import numpy as np
import matplotlib.pyplot as plt
import json
import rdp
import pyproj
import cartopy.crs as ccrs
from pykml import parser

from area_triangulation import triangles

target_epsg = sys.argv[1]           # The EPSG code for the target coordinate system
south_bound = int(sys.argv[2])      # The lowest value any of the car y coordinates can have, in the target EPSG coordinate system
north_bound = int(sys.argv[3])      # The highest value any of the car y coordinates can have, in the target EPSG coordinate system
west_bound = int(sys.argv[4])       # The lowest value any of the car x coordinates can have, in the target EPSG coordinate system
east_bound = int(sys.argv[5])       # The highest value any of the car x coordinates can have, in the target EPSG coordinate system
kml_file_path = sys.argv[6]         # Path to the input KML file
output_file_path = sys.argv[7]      # Path to the output JSON file
error = int(sys.argv[8])            # Road approximation maximal error

source_crs = 'epsg:4326'                                                # Global lat-lon coordinate system
target_crs = pyproj.CRS.from_user_input(int(target_epsg))               # Coordinate system of the target area
source_to_target = pyproj.Transformer.from_crs(source_crs, target_crs)  # Coordinate transformer

x = []
y = []
with open(kml_file_path, 'r', encoding="utf-8") as kml_file:
    root = parser.parse(kml_file).getroot()
    place = root.Document.Placemark
    points = str(place.LineString.coordinates).strip().split("\n")

    # Iterate over the road points
    for p in range(0, len(points), 1):
        # Parse the latitude and longitude from the string
        point = points[p]
        coords_str = point.strip().split(",")[:2]
        lon = float(coords_str[0])
        lat = float(coords_str[1])

        # Convert to Cartesian coordinates in the target EPSG system
        a, b = source_to_target.transform(lat, lon)
        # Decide which coordinate is x and which is y based on target CRS axis information
        cartesian_x = None
        cartesian_y = None
        if target_crs.axis_info[0].name == "Northing":
            cartesian_y = a
            cartesian_x = b
        else:
            cartesian_x = a
            cartesian_y = b

        x.append(cartesian_x)
        y.append(cartesian_y)

# Remove as much road points as necessary so that the approximation error remains below epsilon
road_coords = rdp.rdp(list(zip(x, y)), epsilon=error)
road_x, road_y = zip(*road_coords)

# Find the triangulation of the area around the highway
tri, (center, radius) = triangles(road_coords, error, north_bound, south_bound, west_bound, east_bound)

# Write the road points in the approximation to the given JSON file
with open(output_file_path, 'r', encoding="utf-8") as public:
    json_dict = json.load(public)
    sr = lambda x: str(round(x))
    json_dict["road_coords"] = list(map(lambda x: list(map(sr, x)), road_coords))
    json_dict["triangles"] = list(map(lambda x: list(map(lambda y: list(map(sr, tri["vertices"][y])), x)), tri["triangles"]))
    json_dict["road_bound_circle_center"] = list(map(sr, center))
    json_dict["road_bound_circle_radius"] = sr(radius)
    json_obj = json.dumps(json_dict, indent=4)

with open(output_file_path, 'w', encoding="utf-8") as public:
    public.write(json_obj)

# Plot the initial road points, the approximation, the triangulation and the bounding circle for the road
circle = plt.Circle(center, radius)

fig, ax = plt.subplots()
ax.add_patch(circle)
ax.scatter(x, y, color="green")
ax.plot(road_x, road_y, color="red")
for triple in tri["triangles"]:
    tx = []
    ty = []
    for index in triple:
        point = tri["vertices"][index]
        tx.append(point[0])
        ty.append(point[1])
    tx.append(tri["vertices"][triple[0]][0])
    ty.append(tri["vertices"][triple[0]][1])
    ax.plot(tx, ty, color="orange")
ax.axis("equal")
plt.show()
