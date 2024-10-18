# Running ZKSC code with Circom - examples
In the `execute` folder there are scripts that help you get zksc-generated circom code running with snarkjs. In order to use this, both circom and snarkjs must be installed.

# Simple cube example

In your ZKSC compiler folder, run 
```
./runrust <location-to-examples>/circom/simple-cube/example_cube_circom.zksc -i <location-to-examples>/circom/simple-cube/example_cube_instance.json -w <location-to-examples>/circom/simple-cube/example_cube_witness.json --circom -o <location-to-examples>/circom/execute/example
```

Then in the `execute` folder

```
export POWER=8
./prove_and_verify
```

# EV example
This is done as above, but a choice of prime is also possible. To use the `bls12-381` prime instead of `bn128`, change the modulus in `ev-inf-mod.zksc` and instead of `ev_instance.json` use `ev_instance_255.json` as the instance.
```
./runrust <location-to-examples>/circom/ev/ev-inf-mod.zksc -p <location-to-examples>/circom/ev/ev_public.json -i <location-to-examples>/circom/ev/ev_instance.json -w <location-to-examples>/circom/ev/ev_witness.json -c <location-to-examples>/circom/ev/ev.ccc --circom -o <location-to-examples>/circom/execute/example
```

Then in the `execute` folder

```
export POWER=17
./prove_and_verify
```

POWER=17 is enough for 50 coordinates. To use 200 coordinates, set POWER=19 and change the json inputs.