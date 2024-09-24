import json
from itertools import chain
from sage.all_cmdline import *

import poseidon

def hash_coords(prime, public_dict, car_coords, max_car_coords):
    # Poseidon parameters
    F = GF(prime)
    r = int(public_dict["r"])  # Data ingestion rate of Poseidon
    t = int(public_dict["t"])
    o = int(public_dict["o"])
    R_F = int(public_dict["R_F"])
    R_P = int(public_dict["R_P"])
    alpha = int(public_dict["alpha"])
    round_constants = public_dict["round_constants"]
    MDS_matrix = public_dict["mds_matrix"]
    MDS_matrix_field = matrix(F, t, t)
    for i in range(0 , t):
        for j in range(0 , t):
            MDS_matrix_field[i, j] = F(int(MDS_matrix[i][j], 16))
    round_constants_field = []
    for i in range(0 , (R_F + R_P) * t):
        round_constants_field.append(F(int(round_constants[i], 16)))

    # Hashing the coordinates
    hashable_coords = list(chain.from_iterable(car_coords))
    # Pad coordinates to be hashed (use-case requirements)
    for i in range(max_car_coords - len(car_coords)):
        hashable_coords.append(car_coords[-1][0])
        hashable_coords.append(car_coords[-1][1])
    # Pad coordinares to be hashed (Poseidon requirements)
    hashable_coords.append(1)
    if len(hashable_coords) % r:  # If data length is not a multiple of r
        hashable_coords.extend([0] * (r - len(hashable_coords) % r))  # Add enough 0-s to make data length a multiple of r
    # Calculate hash
    true_hash = poseidon.sponge(F, hashable_coords, t, r, o, alpha, R_F, R_P, round_constants_field, MDS_matrix_field, True)
    return true_hash


def generate_public_input(sample_public_file, size):
    public_dict = None
    with open(sample_public_file, 'r', encoding="utf-8") as public:
        public_dict = json.load(public)
        public_dict["max_car_coords"] = str(size)
    return public_dict


def generate_instance_input(prime, public_input, car_coords, size):
    true_hash = hash_coords(prime, public_input, car_coords, size)
    return {
        "coordinates_hash" : list(map(str, true_hash))
    }


def generate_witness_input(sample_witness_file):
    witness_dict = None
    with open(sample_witness_file, 'r', encoding="utf-8") as witness:
        witness_dict = json.load(witness)
    return witness_dict


def main(args):
    prime = args["prime"]
    size = 2 ** (10 + args["size"])
    sample_public_file = args["in_prefix"] + "public.json"
    sample_witness_file = args["in_prefix"] + "witness.json"
    
    public_input = generate_public_input(sample_public_file, size)
    witness_input = generate_witness_input(sample_witness_file)
    car_coords = list(map(lambda x: list(map(int, x)), witness_input["car_coords"]))
    with open(f"{args['out_prefix']}public.json", "w") as f:
        json.dump(public_input, f)

    with open(f"{args['out_prefix']}witness.json", "w") as f:
        json.dump(witness_input, f)

    with open(f"{args['out_prefix']}instance.json", "w") as f:
        json.dump(generate_instance_input(prime, public_input, car_coords, size), f)


if __name__ == "__main__":
    import sys

    prime = int(sys.argv[1])
    n = int(sys.argv[2])
    in_prefix = sys.argv[3]
    out_prefix = sys.argv[4]

    main({"prime" : prime, "size" : n, "in_prefix" : in_prefix, "out_prefix" : out_prefix})
