# PROVENANCE EV use-case

The statement verifies that a path (witness) of GPS coordinates is
- at least 80% of its total length within some bounds defined by a set of circles;
- at least as long as a length given by the instance.
The `main` function in `ev-inf-mod.zksc` will generate the proof of the statement. Some additional helper functions for calculating square roots of fixed point numbers are defined in `SqrtUtil.zksc` and necessary ZKSC for generating the proof of in multiple parts is in `ev-inf-mod-multi-part.zksc`.

## Inputs to ZKSC

In `public.json`, the program expects
- `len` - length of the fixed point numbers' coefficients in bits. This should be at most floor of `(log2(N) - 2) / 2`, where `N` is the prime used for the ZK-circuit. 
- `pplen` - how many bits of the coefficient to reserve for the fractional part of a fixed point number.
- `circles` - a list of circles, where each circle is a triplet of positive integers in string format. The three integers are:
    - x-coordinate of the circle center.
    - y-coordinate of the circle center.
    - radius of the circle squared. This value is stored multiplied by `2^pplen`, which is the coefficient of the fixed point number corresponding to the radius squared.
    
    This list of circles will be interpreted as the area in which driving is subsidized.
- `max_coordinates` - an upper bound for the amount of vehicle points to be processed (positive integer in string format).
- parameters for the Poseidon hash function (for a description of necessary parameters, see the Poseidon [readme](POSEIDON.md)).

In `witness.json`, the program expects `coordinates` - a list of points with positive integers in string format as coordinates. This list will be interpreted as the vehicle point trail. We expect this list of locations to be temporally ordered. Note that we can certainly choose a coordinate system where enough precision is provided for the coordinates to be integral and the (0, 0) point is such that all of them are positive. The length of the list can be at most `max_coordinates`.

In `instance.json`, the program expects
- `hash_root` - the Poseidon hash of the point list in `witness.json`, calculated after padding the witness point trail up to `max_coordinates` points. 
- `required_total_distance` - a lower bound for the total length of the point trail in `witness.json`.

## Multi-part-proofs

It is also possible to generate the proof of being in the set of circles in several parts. The `main` function in `ev-inf-mod-multi-part.zksc` is meant for that purpose. It starts from an initial state consisting of:
- total distance currently travelled (coefficient of the corresponding fixed point number)
- distance currently travelled in the predefined set of circles (coefficient of the corresponding fixed point number)
- last vehicle coordinate point that was processed (a pair of integers)

and verifies the state. Then it processes a new batch of coordinates, finding the total distance traveled and the distance traveled within the circles. Finally, it saves the new state and verifies it. Note that since sending data from the Prover to the Verifier is impossible in ZKSC, then in practice the ZKSC program needs to be run twice to verify the hash of the new state to the Verifier. The first run is to find the hash and give it to the Verifier, for example by printing it out and saving it in `instance.json`. The second run is to verify the hash by hashing the new state on circuit and asserting that it equals the one in `instance.json`.

This structure allows the total amount of coordinates to be processed in several steps. The check of adherence to subsidy rules is done only if the current step is the last one (if `last_iteration = true` in `public.json`). 

In the `util/` folder is a script `multi-part-proof-simulator.py`, which emulates processing coordinates in batches. It divides a set of coordinates into batches of given size and then calls ZKSC (twice) on each batch. For each batch, it generates the input JSON-s itself based on input JSON-s where multi-part-proof specific parameters have not been added yet. The script takes 4 arguments:
- Path to the folder with the base JSON files wihout multi-part-proof specific parameters. NB! Files in the folder must be named public.json, instance.json and witness.json.
- Path to the folder where the generated input JSON-s to `ev-inf-mod-multi-part.zksc` will be put
- size of the batches in which the total amount of coordinates will be processed
- prime order of the field of the resulting circuit

Usage:
```
python3 multi-part-proof-simulator.py <base inputs folder> <generated inputs folder> <chunk size> <prime>
```

### Additional inputs in the multi-part-proof case

The additional multi-part-proof case-specific parameters in input JSON-s needed to run the `main` function in `ev-inf-mod-multi-part.zksc` are as follows:
- in `public.json`:
    - `first_iteration` - a boolean value, `true` if the current step is the first one, `false` otherwise
    - `last_iteration` - a boolean value, `true` if the current step is the last one, `false` otherwise
- in `instance.json`:
    - `previous_state_hash` - a list of integers corresponding to the Poseidon hash of the previous state
    - `previous_state_hash` - a list of integers corresponding to the Poseidon hash of the current batch of coordinates
    - `new_state_hash` - a list of integers corresponding to the Poseidon hash of the new state that will be reached by the end of the current step
- in `witness.json`:
    - `current_total_distance_coef` - an integer in string format, corresponding to the coefficient of the FixedPoint representing total distance traveled so far
    - `current_circles_distance_coef` - an integer in string format, corresponding to the coefficient of the FixedPoint representing distance traveled in the circles so far
    - `last_point` - a list of two integers in string format, corresponding to the last vehicle coordinate point that was processed.


