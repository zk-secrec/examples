# PROVENANCE highway tax use-case

## Description

The aim of the use-case is to prove in Zero-Knowledge that a vehicle point trail contains no more than `max_allowed_distance` m of segments that are close to a particular highway. The `main` function at `highway-tax.zksc` finds the total distance traveled and the distance traveled within predefined triangles (these represent the area around the given highway) and checks that their difference doesn't exceed `max_allowed_distance`. The application logic is implemented in `highway-tax.zksc`, supporting datatypes in `Coord.zksc` and `Triangle.zksc`, and helper functions regarding square roots and distance calculations in `SqrtUtil.zksc` and `DistUtil.zksc`. Necessary ZKSC code for generating the proof of in multiple parts is in `highway-tax-multi-part.zksc`.

## Inputs to ZKSC

In `public.json`, the program expects
- `len` - length of the fixed point numbers' coefficients in bits. This should be at most floor of `(log2(N) - 2) / 2`, where `N` is the prime used for the ZK-circuit. 
- `pplen` - how many bits of the coefficient to reserve for the fractional part of a fixed point number.
- `triangles` - a list of triangles, where each triangle is a triplet of coordinate pairs. This list of triangles will be interpreted as the area around a particular highway. We expect the coordinates of points to be positive integers in string format. Note that we can certainly choose a coordinate system where enough precision is provided for the coordinates to be integral and the (0, 0) point is such that all of them are positive. 
- `max_car_coords` - an upper bound for the amount of vehicle points to be processed (positive integer in string format).
- `max_allowed_distance` - the upper bound for the distance travelled on the highway (positive integer in string format).
- parameters for the Poseidon hash function (for a description of necessary parameters, see the Poseidon [readme](../electric-vehicle/POSEIDON.md)).

In `witness.json`, the program expects `car_coords` - a list of points with positive integers in string format as coordinates. This list will be interpreted as the vehicle point trail. We expect this list of locations to be temporally ordered. Again note that we can certainly choose a coordinate system where enough precision is provided for the coordinates to be integral and the (0, 0) point is such that all of them are positive. The length of the list can be at most `max_car_coords`.

In `instance.json`, the program expects `coordinates_hash` - the Poseidon hash of the point list in `witness.json`, calculated after padding the witness point trail up to `max_car_coords` points.

## Application logic

The script `highway-tax.zksc` find the total length `total_distance` of the vehicle trail and the total length `off_distance` of those segments that are not on the highway in question. The proof that the no more than `max_allowed_distance` m of the vehicle point trail was along the highway, is structured as follows:
- First, we hash the witness coordinate point trail, padded with the last point in the trail to ensure the trail length visible to the Verifier is `max_car_coords`. Then we assert that the hash we calculated matches the one in the instance.
- Second, we check for each segment (a pair of consecutive points) in the vehicle trail, whether both of its endpoints of lie in some triangle from the public list of triangles. If yes, then the trail segment is considered as distance traveled off the highway and the distance between the endpoints is added to `off_distance`. Otherwise, the segment is considered as distance traveled on the highway. In any case, we add the distance between the segment endpoints to the total distance `total_distance`.
- Finally, we assert that `total_distance - off_distance <= max_allowed_distance`.

## Point in triangle check in zero knowledge

Note that the Prover knows for each point whether or not it lies in some triangle. Thus for each point in the vehicle trail, Prover provides a triangle - if the point actually lies in some triangle, then this will be the triangle it lies in. If not, then it will be an arbitrary triangle. The check of whether a point lies in some triangle mentioned above then means checking if the point lies in the provided triangle. Indeed, this is sufficient to convince the Verifier, because the Prover's incentive is to show for as many vehicle points as possible that they fall into some triangle.

Note that the Verifier must be convinced that the triangle provided is indeed from the public triangle list, but not know the exact triangle itself. The triangle is thus chosen as follows: for each triangle in the list, we provide a boolean multiplier in the Prover domain. For the triangle we want to choose, it will be `1` and for all other triangles it will be `0`. We assert that the sum of those multipliers is `1`, convincing Verifier that exactly one of them is `1` and all others are `0`. Then we multiply each coordinate in the triangle by that triangle's multiplier. The result is the triangle which had the multiplier `1`.

The check that a point `p` is inside a triangle of points `p0`, `p1`, and `p2` works as follows: we first find the doubled area `A` of the triangle in zero knowledge. Then we put two unsigned integers `s` and `t` on the circuit, in the `@prover` domain. We find in the `$pre` stage, whether `p` actually is in the triangle or not. If it is, then `s` and `t` are values such that `Ap = Ap0 + s(p1 - p0) + t(p2 - p0)` - the unnormalized barycentric coordinates of `p` with respect to the triangle composed of `p0`, `p1`, and `p2`. If not, then `s = t = 0` - these are the (unnormalized) barycentric coordinates of `p0` with respect to the triangle composed of `p0`, `p1`, and `p2`. We assert that `s + t <= A` to convince the Verifier that `s` and `t` indeed are the unnormalized barycentric coordinates of some point in the triangle (point `p` is in the triangle `[p0, p1, p2]` if and only if its unnormalized barycentric coordinates `s` and `t` satisfy `0 <= s, t <= A` and `s + t <= A`, where `A` is the doubled area of the triangle). Finally, we check in zero-knowledge whether `A(p0 - p) + s(p1 - p0) + t(p2 - p0) = 0` for the `s` and `t` provided (this checks if the bayrcentric coordinates we provided really were those of point `p`).

## Generating inputs

In the `util/` folder, there are a couple of helpful scripts:
- a script `road_coord_gen_kml.py` to generate the triangulation of the area around the highway for `public.json`, given a KML file containing the road points, a desired maximum error `e` for approximating the road by line segments and a bounding rectangle for the area to be triangulated. It first approximates the road, given as a point trail, by as few line segments as possible. The number of line segments is determined by the maximum error `e` given to the script \- no point in the actual road can be further from the approximation than this error. Next, the script triangulates the given bounding rectangle, from where the segmented line of the road approximation (surrounded by a polygonal buffer of radius `e`) has been cut out as a hole. The output will be a list of triangles, represented as 3 element lists of pairs of coordinates (coordinates are integers in string format). The script requires the following arguments:
    - an EPSG area code to translate longitude and latitude to Cartesian coordinates.
    - the lower bound for the `y`-coordinate (already in the target Cartesian coordinate system) for the area to be triangulated.
    - the upper bound for the `y`-coordinate (already in the target Cartesian coordinate system) for the area to be triangulated.
    - the lower bound for the `x`-coordinate (already in the target Cartesian coordinate system) for the area to be triangulated.
    - the upper bound for the `x`-coordinate (already in the target Cartesian coordinate system) for the area to be triangulated.
    - path to the KML file containing the points of the road.
    - path to a JSON file to write the triangulation output to.
    - maximum error for approximating road points as a segmented line (in meters).
    
    Example usage:
    ```
    python3 road_coord_gen_kml.py <EPSG code for the target area> <y lower bound> <y upper bound> <x lower bound> <x upper bound> <input KML file> <output JSON> <error> 
    python3 road_coord_gen_kml.py 3301 6375274 6658862 282560 751542 $HOME/Provenance/roads/example2/trt-tln-mnt.kml example_data/trt-tln/public.json 200
    ```
    Note that the `road_coords_gen_kml.py` script uses the `area_triangulation.py` script, also in the `util/` folder. It also requires the Python packages numpy, matplotlib, rdp, pyproj, cartopy and kml.

    The triangulation software is originally created by Jonathan Richard Shewchuk and introduced in:

    <cite>Jonathan Richard Shewchuk, Triangle: Engineering a 2D Quality Mesh Generator and Delaunay Triangulator, in ''Applied Computational Geometry: Towards Geometric Engineering'' (Ming C. Lin and Dinesh Manocha, editors), volume 1148 of Lecture Notes in Computer Science, pages 203-222, Springer-Verlag, Berlin, May 1996. (From the First ACM Workshop on Applied Computational Geometry.)</cite>
- a script `car_coord_gen_kml.py`, which generates a (vehicle) point trail along a given road and hashes the generated coordinates using the Poseidon hash algorithm. This script is meant to provide `witness.json` and `instance.json` content to the `highway-tax.zksc` script (when real location data is not available). The script needs the following parameters:
    - order of the prime field used for hashing.
    - path to the KML file containing the road points.
    - an EPSG area code to translate longitude and latitude of road points to Cartesian coordinates.
    - path to a JSON file (`public.json`) containing necessary parameters for the Poseidon hash function (see [readme](../electric-vehicle/POSEIDON.md) for an overview of these parameters) and the maximum number of vehicle coordinates.
    - path to a JSON file (`instance.json`), where the hash of the coordinates will be stored.
    - path to a JSON file (`witness.json`), where the generated car points will be stored.
    - a positive integer denoting after how many road points in the KML file should a car point be generated (sampling period).
    
    Example usage:
    ```
    python3 car_coord_gen_kml.py <prime> <input KML file> <EPSG code for the target area> <input public.json file> <output instance.json file> <output witness.json file> <sampling period>
    python3 car_coord_gen_kml.py 2305843009213693951 $HOME/Provenance/roads/example2/tartu-turi.kml 3301 example_data/trt-tln/public.json example_data/trt-tln/instance.json example_data/trt-tln/witness.json 5
    ```
    Note that installing SageMath is necessary for running this script, because the `car_coord_gen_kml.py` depends on the `coord_hasher.py` script in the `util/` folder, which in turn depends on the sage library of Python.

- a script `data_visual.py`, which takes as inputs
    - a path to a `public.json` ZKSC input file
    - a path to a `witness.json` ZKSC input file

    and visualizes the triangulation from `public.json` and the car points from `witness.json` using matplotlib.

    Example usage:
    ```
    python3 data_visual.py <path to public.json> <path to witness.json>
    python3 data_visual.py example_data/tln-trt/public.json example_data/tln-trt/witness.json
    ```

## Multi-part-proofs

It is also possible to generate the proof of not being on a highway in several parts. The `main` function in `highway-tax-multi-part.zksc` is meant for that purpose. It starts from an initial state consisting of:
- total distance currently travelled (coefficient of the corresponding fixed point number)
- distance currently travelled in predefined triangles (coefficient of the corresponding fixed point number)
- last vehicle coordinate point that was processed (a pair of integers)

and verifies the state. Then it processes a new batch of coordinates, finding the total distance traveled and the distance traveled within predefined triangles. Finally, it saves the new state and verifies it. Note that since sending data from the Prover to the Verifier is impossible in ZKSC, then in practice the ZKSC program needs to be run twice to verify the hash of the new state to the Verifier. The first run is to find the hash and give it to the Verifier, for example by printing it out and saving it in `instance.json`. The second run is to verify the hash by hashing the new state on circuit and asserting that it equals the one in `instance.json`.

This structure allows the total amount of coordinates to be processed in several steps. The check of distance travelled outside of predefined triangles being smaller than `max_allowed_distance` is done only if the current step is the last one (if `last_iteration = true` in `public.json`). 

In the `util/` folder is a script `multi-part-proof-simulator.py`, which emulates processing coordinates in batches. It divides a set of coordinates into batches of given size and then calls ZKSC (twice) on each batch. For each batch, it generates the input JSON-s itself based on input JSON-s where multi-part-proof specific parameters have not been added yet. The script takes 4 arguments:
- Path to the folder with the base JSON files wihout multi-part-proof specific parameters. NB! Files in the folder must be named public.json, instance.json and witness.json.
- Path to the folder where the generated input JSON-s to `highway-tax-multi-part.zksc` will be put
- size of the batches in which the total amount of coordinates will be processed
- prime order of the field of the resulting circuit

Usage:
```
python3 multi-part-proof-simulator.py <base inputs folder> <generated inputs folder> <chunk size> <prime>
```

### Additional inputs in the multi-part-proof case

The additional multi-part-proof case-specific parameters in input JSON-s needed to run the `main` function in `highway-tax-multi-part.zksc` are as follows:
- in `public.json`:
    - `first_iteration` - a boolean value, `true` if the current step is the first one, `false` otherwise
    - `last_iteration` - a boolean value, `true` if the current step is the last one, `false` otherwise
- in `instance.json`:
    - `previous_state_hash` - a list of integers corresponding to the Poseidon hash of the previous state
    - `previous_state_hash` - a list of integers corresponding to the Poseidon hash of the current batch of coordinates
    - `new_state_hash` - a list of integers corresponding to the Poseidon hash of the new state that will be reached by the end of the current step
- in `witness.json`:
    - `current_total_distance_coef` - an integer in string format, corresponding to the coefficient of the FixedPoint representing total distance traveled so far
    - `current_off_highway_distance_coef` - an integer in string format, corresponding to the coefficient of the FixedPoint representing distance traveled in the triangles so far
    - `last_point` - a list of two integers in string format, corresponding to the last vehicle coordinate point that was processed.

