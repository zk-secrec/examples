# usage: python3 car_coord_gen_kml.py <prime> <input KML file> <EPSG code for the target area> <input public.json file> <output instance.json file> <output witness.json file> <sampling period>
# example: python3 car_coord_gen_kml.py 2305843009213693951 $HOME/Provenance/roads/example2/tartu-turi.kml 3301 example_data/trt-tln/public.json example_data/trt-tln/instance.json example_data/trt-tln/witness.json 5

import sys
import json
import pyproj
import cartopy.crs as ccrs
from pykml import parser
from math import pi, sin, cos
from random import randint

from hasher import hash_coords

prime = int(sys.argv[1])            # Prime field order for hashing
kml_file = sys.argv[2]              # KML file path
target_epsg = sys.argv[3]           # The EPSG code for the target coordinate system
public_json = sys.argv[4]           # public.json file path (contains hashing parameters and max_car_coords)
instance_json = sys.argv[5]         # instance.json file path
witness_json = sys.argv[6]          # witness.json file path
sampling_period = int(sys.argv[7])  # After how many road points should we generate a car point

source_crs = 'epsg:4326'                                                # Global lat-lon coordinate system
target_crs = pyproj.CRS.from_user_input(int(target_epsg))               # Coordinate system of the target area
source_to_target = pyproj.Transformer.from_crs(source_crs, target_crs)  # Coordinate transformer

# Get hash parameters and max_car_coords from the public dictionary
public_dict = None
with open(public_json, 'r') as public:
    public_dict = json.load(public)

car_coords = []
car_distance = 0

# Highway tax parameters
epsilon = 0  # Maximum distance that the generated car points can have from the road
max_car_coords = int(public_dict["max_car_coords"])

# Read the road points from the KML file
road_points = []
with open(kml_file, 'r', encoding="utf-8") as kml:
    root = parser.parse(kml).getroot()
    place = root.Document.Placemark
    road_points = str(place.LineString.coordinates).strip().split("\n")

# Generate car points along the road
for i in range(0, len(road_points), sampling_period):
    point = road_points[i]    # The road point our car point will be close to
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

    direction = randint(0, 359) * pi / 180  # Pick a random direction and convert it to radians
    # Generate car points that are an epsilon / 2 distance away from the road
    car_x = int(cartesian_x) + epsilon / 2 * cos(direction)
    car_y = int(cartesian_y) + epsilon / 2 * sin(direction)
    car_coords.append([round(car_x), round(car_y)])

    # Can't calculate distance if there is only one car point so far
    if len(car_coords) <= 1:
        continue

    # Find the distance from the last generated car point
    last_x = car_coords[-2][0]
    last_y = car_coords[-2][1]
    car_distance += ((car_x - last_x) ** 2 + (car_y - last_y) ** 2) ** 0.5

# Hash car coordinates
true_hash = hash_coords(prime, public_dict, car_coords, max_car_coords)

# Write distance travelled by the car and hash of coordinates into instance.json
instance_dict = None
with open(instance_json, 'r') as instance:
    instance_dict = json.load(instance)
    instance_dict["coordinates_hash"] = [str(el) for el in true_hash]
    instance_dict["total_distance"] = str(round(car_distance))

with open(instance_json, 'w') as instance:
    instance_obj = json.dumps(instance_dict, indent=4)
    instance.write(instance_obj)

# Write car coordinates into witness.json
witness_dict = None
with open(witness_json, 'r') as witness:
    witness_dict = json.load(witness)
    witness_dict["car_coords"] = list(map(lambda x: [str(x[0]), str(x[1])], car_coords))

with open(witness_json, 'w') as witness:
    witness_obj = json.dumps(witness_dict, indent=4)
    witness.write(witness_obj)


        
