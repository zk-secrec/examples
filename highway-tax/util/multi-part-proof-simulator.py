import sys
import subprocess
import os
import json
import re

from math import floor

from hasher import hash_coords, hash_state


def read_files(base_loc):
    base_public = None
    with open(base_loc + "public.json", 'r') as public:
        base_public = json.load(public)

    base_instance = None
    with open(base_loc + "instance.json", 'r') as instance:
        base_instance = json.load(instance)

    base_witness = None
    with open(base_loc + "witness.json", 'r') as witness:
        base_witness = json.load(witness)

    return base_public, base_instance, base_witness


def write_to_files(json_loc, public_dict, instance_dict, witness_dict):
    with open(json_loc + "public.json", "w") as public:
        public.write(json.dumps(public_dict, indent=4))

    with open(json_loc + "instance.json", "w") as instance:
        instance.write(json.dumps(instance_dict, indent=4))

    with open(json_loc + "witness.json", "w") as witness:
        witness.write(json.dumps(witness_dict, indent=4))


if __name__ == "__main__":
    base_loc = sys.argv[1]          # The directory the base JSON files must be in
    json_loc = sys.argv[2]          # The directory the public.json, instance.json and witness.json should be in
    chunk_size = int(sys.argv[3])   # How many coordinates to process at once
    prime = int(sys.argv[4])        # Prime order of the field of the circuit

    public_dict, instance_dict, witness_dict = read_files(base_loc)

    # Store the total amount of coordinates about to be processed and use-case specific final coordinate point list length
    all_coordinates = witness_dict["car_coords"]
    max_coords = int(public_dict["max_car_coords"])

    # Specific parameters of multi-part-proofs
    public_dict["first_iteration"] = True
    public_dict["last_iteration"] = False

    witness_dict["current_total_distance_coef"] = "0"
    witness_dict["current_off_highway_distance_coef"] = "0"
    witness_dict["last_point"] = all_coordinates[0]

    # Hash the initial state
    init_state_hash = hash_state(
        prime, 
        public_dict, 
        [
            int(witness_dict["current_total_distance_coef"]),
            int(witness_dict["current_off_highway_distance_coef"]),
            int(all_coordinates[0][0]) * 2 ** int(public_dict["pplen"]),    # Coefficient of the x-coordinate will be hashed
            int(all_coordinates[0][1]) * 2 ** int(public_dict["pplen"])     # Coefficient of the y-coordinate will be hashed
        ]
    )
    instance_dict["previous_state_hash"] = list(map(str, init_state_hash))
    instance_dict["new_state_hash"] = instance_dict["previous_state_hash"]  # This is just a placeholder at first

    iterations = int(len(all_coordinates) / chunk_size) + (1 if len(all_coordinates) % chunk_size else 0)
    for i in range(iterations):
        witness_dict["car_coords"] = all_coordinates[i * chunk_size : (i + 1) * chunk_size]

        # Hash the current coordinates and put the result into the instance
        int_coords = list(map(lambda x : list(map(int, x)), witness_dict["car_coords"]))
        coords_hash = hash_coords(prime, public_dict, int_coords, max_coords)
        instance_dict["coordinates_hash"] = list(map(str, coords_hash))
        
        # Finalize JSON inputs
        write_to_files(json_loc, public_dict, instance_dict, witness_dict)

        # Run ZKSC
        cwd = os.getcwd()
    
        # First run to find the hash of the final state
        os.chdir("../../..")
        result = subprocess.run(["./runrust", "docs/M40-highway-tax/highway-tax-multi-part.zksc", json_loc + "public.json", json_loc + "instance.json", json_loc + "witness.json", "-o", "/tmp/fff"], capture_output=True, text=True)

        words = re.split(" |\n", result.stdout)
        # This is a highly use-case specific way to parse the hash out of the stdout
        hash_counter = 0
        for w, word in enumerate(words):
            if word == "Hash:":
                hash_counter += 1
            if hash_counter == 3:
                instance_dict["new_state_hash"] = [words[w + i] for i in range(1, 6, 1)]
                break

        write_to_files(json_loc, public_dict, instance_dict, witness_dict)

        # Second run to verify the hash to the Verifier
        result = subprocess.run(["./runrust", "docs/M40-highway-tax/highway-tax-multi-part.zksc", json_loc + "public.json", json_loc + "instance.json", json_loc + "witness.json", "-o", "/tmp/fff"], capture_output=True, text=True)

        words = re.split(" |\n", result.stdout)
        # This is a highly use-case specific way to parse the new EV state out of the stdout
        for w, word in enumerate(words):
            if word == "Newdistances:":
                witness_dict["current_total_distance_coef"] = words[w + 1]
                witness_dict["current_off_highway_distance_coef"] = words[w + 2]
                break

        # Renew some parameters for the next round
        public_dict["first_iteration"] = False
        public_dict["last_iteration"] = False if i < iterations - 1 else True

        instance_dict["previous_state_hash"] = instance_dict["new_state_hash"]

        witness_dict["last_point"] = witness_dict["car_coords"][-1]

        # Print current witness state for debug reasons
        print(witness_dict)

        os.chdir(cwd)
