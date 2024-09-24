# usage: python3 data_visual.py <location of JSON files>

import sys
import json
import numpy as np
import matplotlib.pyplot as plt

# Visualize data in public.json and witness.json
public_loc = sys.argv[1]
witness_loc = sys.argv[2]

# Read in the road coordinates, triangles and the bounding circle for the road
road_x = []
road_y = []
triangles = []
center = []
radius = 0

with open(public_loc, "r") as public:
    json_dict = json.load(public)
    for coord in json_dict["road_coords"]:
        road_x.append(int(coord[0]))
        road_y.append(int(coord[1]))
    for triangle in json_dict["triangles"]:
        triangles.append(list(map(lambda x: list(map(int, x)), triangle)))

# Read in the car coordinates
car_x = []
car_y = []
with open(witness_loc, "r") as witness:
    json_dict = json.load(witness)
    for coord in json_dict["car_coords"]:
        car_x.append(int(coord[0]))
        car_y.append(int(coord[1]))

# Show everything
fig, ax = plt.subplots()
ax.scatter(car_x, car_y, color="green")
ax.plot(road_x, road_y, color="red")
for triangle in triangles:
    tx = []
    ty = []
    for point in triangle:
        tx.append(point[0])
        ty.append(point[1])
    tx.append(triangle[0][0])
    ty.append(triangle[0][1])
    ax.plot(tx, ty, color="orange")
ax.axis("equal")
plt.show()
