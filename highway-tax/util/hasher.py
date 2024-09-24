from sage.all_cmdline import *   # import sage library
from itertools import chain

# Find Poseidon and import
sys.path.insert(1, "../../M38-Poseidon/sage")
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


def hash_state(prime, public_dict, state_as_list):
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

    # Pad stuff to be hashed (Poseidon requirements)
    state_as_list.append(1)
    if len(state_as_list) % r:  # If data length is not a multiple of r
        state_as_list.extend([0] * (r - len(state_as_list) % r))  # Add enough 0-s to make data length a multiple of r
    # Calculate hash
    true_hash = poseidon.sponge(F, state_as_list, t, r, o, alpha, R_F, R_P, round_constants_field, MDS_matrix_field, True)
    return true_hash