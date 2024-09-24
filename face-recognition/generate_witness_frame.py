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

s = int(sys.argv[2]) # the height and width of the image after adding the frame
h1 = (s - h) // 2
w1 = (s - w) // 2

def f(i, j):
    i2 = i - h1
    j2 = j - w1
    if i2 >= 0 and i2 < h and j2 >= 0 and j2 < w:
        return prover_image[i2][j2]
    else:
        return 0

prover_image2 = [[f(i,j) for j in range(s)] for i in range(s)]
prover_image = [[F"{math.floor(scaling_factor*prover_image2[i][j]+0.5)}" for j in range(s)] for i in range(s)]

public_json = {
    "num_rows": F"{s}",
    "num_cols": F"{s}",
}
with open('public.json', 'w', encoding='utf-8') as f:
    json.dump(public_json, f, indent=2)

witness_json = {
    "rect_left": F"{w1}",
    "rect_right": F"{w1+w}",
    "rect_top": F"{h1}",
    "rect_bottom": F"{h1+h}",
    "image": prover_image,
}
with open('witness.json', 'w', encoding='utf-8') as f:
    json.dump(witness_json, f, indent=2)
