# The triangulation software is originally created by Jonathan Richard Shewchuk and introduced in:
#
# Jonathan Richard Shewchuk, Triangle: Engineering a 2D Quality Mesh Generator and Delaunay Triangulator, 
# in ``Applied Computational Geometry: Towards Geometric Engineering'' (Ming C. Lin and Dinesh Manocha, editors), 
# volume 1148 of Lecture Notes in Computer Science, pages 203-222, Springer-Verlag, Berlin, May 1996. 
# (From the First ACM Workshop on Applied Computational Geometry.)
#
# More can be read about it here: https://www.cs.cmu.edu/~quake/triangle.html

import sys
import numpy as np
import matplotlib.pyplot as plt
import random
import json
import shapely
import triangle


def buffer_road(road_points, epsilon):
    road = shapely.LineString(road_points)
    left_buf_line = road.offset_curve(epsilon, join_style=2)
    right_buf_line = road.offset_curve(-epsilon, join_style=2)
    buffered_points = list(left_buf_line.coords) + list(right_buf_line.coords[::-1])
    return buffered_points


def trivial_circle(boundary):
    if len(boundary) == 0:
        return np.array([0, 0]), 0
    elif len(boundary) == 1:
        return boundary[0], 0
    elif len(boundary) == 2:
        diam_vec = boundary[1] - boundary[0]
        radius = np.linalg.norm(diam_vec) / 2
        center = boundary[0] + diam_vec / 2
        return center, radius
    elif len(boundary) == 3:
        x1 = boundary[0][0]
        x2 = boundary[1][0]
        x3 = boundary[2][0]

        y1 = boundary[0][1]
        y2 = boundary[1][1]
        y3 = boundary[2][1]

        A = [
            [2 * (x1 - x2), 2 * (y1 - y2)],
            [2 * (x1 - x3), 2 * (y1 - y3)]
        ]

        b = [
            x1 ** 2 - x2 ** 2 + y1 ** 2 - y2 ** 2,
            x1 ** 2 - x3 ** 2 + y1 ** 2 - y3 ** 2
        ]

        center = np.linalg.solve(A, b)
        radius = np.linalg.norm(center - boundary[0])
        return center, radius


def smallest_enclosing_circle(points, boundary):
    if len(points) == 0 or len(boundary) == 3:
        boundary = np.asarray(boundary)
        return trivial_circle(boundary)

    p = random.randint(0, len(points) - 1)
    point = points[p]
    points = np.delete(points, p, 0)
    center, radius = smallest_enclosing_circle(points, boundary)

    if np.linalg.norm(center - point) < radius:
        return center, radius

    return smallest_enclosing_circle(points, boundary + [point])


def circle_enclosing_rectangle(center, radius, padding):
    r = radius + padding
    return [
        center + r * np.array([1, 1]),
        center + r * np.array([1,-1]),
        center + r * np.array([-1, -1]),
        center + r * np.array([-1, 1]),
    ]


def triangles(road_points, epsilon, north_bound, south_bound, west_bound, east_bound):
    buffered_points = buffer_road(road_points, epsilon)
    buffered_points = np.asarray(buffered_points)
    center, radius = smallest_enclosing_circle(buffered_points, [])
    # rectangle = circle_enclosing_rectangle(center, radius, epsilon)
    rectangle = [
        [east_bound, north_bound],
        [east_bound, south_bound],
        [west_bound, south_bound],
        [west_bound, north_bound]
    ]

    N = len(buffered_points)
    i = np.arange(N)
    j = np.arange(4)

    v = np.concatenate((buffered_points, rectangle))
    s1 = np.stack([i, i + 1], axis=1) % N
    s2 = np.stack([N + j % 4, N + (j + 1) % 4], axis=1)

    info = dict(vertices=v, segments=np.concatenate((s1, s2)), holes=[road_points[1]])
    tri = triangle.triangulate(info, 'p')

    return tri, (center, radius)


if __name__ == "__main__":
    # Example usage: python3 area_triangulation.py ../scrap/scrap.json 5 150 -50 0 150

    public_json = sys.argv[1]
    error = int(sys.argv[2])
    north_bound = int(sys.argv[3])      # The highest value any of the car y coordinates can have, in the target EPSG coordinate system
    south_bound = int(sys.argv[4])      # The lowest value any of the car y coordinates can have, in the target EPSG coordinate system
    west_bound = int(sys.argv[5])       # The lowest value any of the car x coordinates can have, in the target EPSG coordinate system
    east_bound = int(sys.argv[6])       # The highest value any of the car x coordinates can have, in the target EPSG coordinate system

    public_dict = None
    with open(public_json, 'r') as public:
        public_dict = json.load(public)

    road_coords = [[int(pair[0]), int(pair[1])] for pair in public_dict["road_coords"]]
    tri, (center, radius) = triangles(road_coords, error, north_bound, south_bound, west_bound, east_bound)

    circle = plt.Circle(center, radius)
    road_x, road_y = zip(*road_coords)

    fig, ax = plt.subplots()
    # ax.add_patch(circle)
    ax.plot(road_x, road_y, color="purple")
    for triple in tri["triangles"]:
        x = []
        y = []
        for index in triple:
            point = tri["vertices"][index]
            x.append(point[0])
            y.append(point[1])
        x.append(tri["vertices"][triple[0]][0])
        y.append(tri["vertices"][triple[0]][1])
        ax.plot(x, y, color="orange")
    ax.axis("equal")
    plt.show()