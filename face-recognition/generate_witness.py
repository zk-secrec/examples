# coding: utf-8

import matplotlib.pyplot as plt
import math
import json
import sys

prover_image_file = sys.argv[1]
prover_image = plt.imread(prover_image_file);

h, w = prover_image.shape

scaling_factor = 255
for i in range(h):
    for j in range(w):
        if prover_image[i][j] > 1.5:
            scaling_factor = 1
prover_image = [[F"{math.floor(scaling_factor*prover_image[i][j]+0.5)}" for j in range(w)] for i in range(h)]

public_json = {
    "num_rows": F"{h}",
    "num_cols": F"{w}",
}
with open('public.json', 'w', encoding='utf-8') as f:
    json.dump(public_json, f, indent=2)

witness_json = {
    "rect_left": "0",
    "rect_right": F"{w}",
    "rect_top": "0",
    "rect_bottom": F"{h}",
    "image": prover_image,
}
with open('witness.json', 'w', encoding='utf-8') as f:
    json.dump(witness_json, f, indent=2)
