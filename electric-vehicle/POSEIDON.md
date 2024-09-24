# The Poseidon hash algorithm in ZK-SecreC

## Using the ZK-SecreC implementation

#### Getting started

The Poseidon hash algorithm is a sponge function (https://en.wikipedia.org/wiki/Sponge_function) whose inner permutation is the Poseidon permutation, specified in https://eprint.iacr.org/2019/458.pdf. Given a finite field, it is used for hashing variable length arrays of field elements. To hash a message with the Poseidon algorithm in ZK-SecreC, you first need to know the prime order finite field your ZK circuit uses. Denote the order of this field by `p`. You also need to know the desired security level (against collision attacks) of the hash in bits. Denote this by `M` (for example, for 128-bit security, `M = 128`).

#### Choosing parameters for the sponge construction

Once you know `p` and `M`, more parameters need to be specified to use the ZK-SecreC Poseidon implementation:
- `r` - This is the rate (in field elements) at which input data is hashed. Data is added (using field addition) in chunks of `r` field elements to the inner state of Poseidon and the inner permutation is called on the state once for each chunk.
- `c` - Alongside with `r` elements that input data is added to, the Poseidon inner state also has `c` field elements that are not directly affected by input. Those `c` elements should have at least `2M` bits, thus the number of field elements necessary can be calculated from `c = ceil(2M / log2(p))`.
- `t` - This is the width of the inner state, `t = r + c`.
- `o` - This is the length of the hash output in field elements. To prevent birthday attacks, the length of `o` in bits should be at least `2M`, so choose `o >= ceil(2M / log2(p))`.

These are the parameters of the sponge part of Poseidon and need to be chosen by the user. Note that for a fixed length `t` of the inner state, a tradeoff between security (`c`) and efficiency (`r`) needs to be made. It is also not reasonable to use a very large `t`, because the amount of multiplications in the inner permutation depends on `t^2`. For a better understanding of the Poseidon hash algorithm, it is recommended to visit https://www.poseidon-hash.info/ and read https://eprint.iacr.org/2019/458.pdf.

#### Finding parameters for the inner permutation

Once you have decided on the parameters for the sponge, the parameters for the inner permutation are pretty much determined. They are:
- `alpha` - This is the smallest positive integer such that `gcd(alpha, p - 1) = 1`. Note that since `p - 1` is an even integer, only odd `alpha`-s need to be considered.
- `R_F` - This is the number of rounds with full S-boxes in the permutation.
- `R_P` - This is the number of rounds with partial S-boxes in the permutation.
- `round_constants` - This is an array of constants to be added to the inner state in the beginning of each permutation round.
- `MDS_matrix` - This is a `t x t` matrix that the inner state is multiplied by in the end of each permutation round.

Once you know `p`, `M`, `t` and `alpha`, the parameters `R_F`, `R_P`, `round_constants` and `MDS_matrix` can be generated with the SageMath script at https://extgit.iaik.tugraz.at/krypto/hadeshash/-/blob/master/code/generate_params_poseidon.sage, provided by the authors of the Poseidon permutation. Refer to the script and the aforementioned paper for usage instructions. For some `p`, `M` and `t`, the parameters have already been generated and saved in `public.json`. As of the time of writing this README, parameters for the security level `M = 128` and cases `p = 0x1fffffffffffffff, t = 13` and `p = 0x73eda753299d7d483339d80809a1d80553bda402fffe5bfeffffffff00000001, t = 5` have been saved in `public.json`. Since those primes are 61 and 255 bits long, respectively, then the names of their corresponding parameters are followed by "_61" and "_255", respectively. Also note that the Sage and Rust reference implementations use a different `R_P` and thus different round constants and MDS matrices for the same 255-bit prime, `p = 0x73eda753299d7d483339d80809a1d80553bda402fffe5bfeffffffff00000001`. This distinction is made in `public.json` by adding a `_sage` or `_rust` to the parameter names.

#### Using the ZK-SecreC `poseidon` function

To hash a message, use the `poseidon` function in `poseidon.zksc`. All of the listed parameters, except for `c`, are inputs to the `poseidon` function, in addition to the data to be hashed. The output of `poseidon` is the hash of the input data - an array of field elements of length `o`. They should be known to the prover and verifier at compile time and as such should be read from `public.json` (again, the only exception is `c`, because that can be calculated from `t - r`). Note that `poseidon` takes one extra boolean argument, `rate_first` - this indicates whether the rate (`r`) part or the capacity (`c`) part is the first part of the Poseidon inner state of length `t`. If `rate_first = true`, then the rate comes first in the inner state, otherwise the capacity part comes first. This extra argument was added to be compatible with different Poseidon implementations "in the wild", which have implemented the inner state order in different ways.

The input data should be read from `witness.json`. Note that the `poseidon` function **requires data to be padded** according to the rule in https://eprint.iacr.org/2019/458.pdf - a separator value, the field element `1` needs to be appended to the message you wish to hash, followed by enough field elements `0` to make the data length a multiple of `r`. Also note that the length of the padded data should be public (come from `public.json`, for example), because it is evident from the compiled circuit.

An example of using the `poseidon` function can be found in `zksc/test.zksc`.

#### Note on choosing `t`

If you know the approximate length of your messages (for example that they're all ~1000 field elements long), you can use the script `mulgates.py` to find the optimal value of `t` for your instance of Poseidon (optimal in the sense that it minimizes multiplication gates in the ZK circuit). Change the `msg_len` variable in code to suit your application. Note that for large values of `msg_len` (for example, 20 000), the script will take a very long time to run. However, since the script also prints out every `t` considered and the number of multiplication gates for each `t`, you can simply kill the script after you see from the output that one minimum in the number of multiplication gates has been achieved (the number of multiplication gates as a function of `t` has at most two minima).

#### Note on the partial rounds

In the partial rounds of Poseidon, only one element of the inner state goes through an S-box, all other elements stay the same. It is not entirely clear whether it's important *which* element of the inner state is chosen for this role. Since it is implemented in different ways "in the wild", the poseidon inner permutation function `poseidon_perm` takes an argument `partial_round_pow_i` which is the index of the element to go through an S-box in the partial rounds. In practice, this is either `0` or `t - 1`. If you wish to change it, you need to change it in the definition of `poseidon`, in the 2 places `poseidon_perm` is called.

## Reference implementations in Rust and Python

#### Implementation in Rust

We use an implementation of Poseidon in Rust, taken from https://github.com/dusk-network/Poseidon252, to verify the correctness of our ZK-SecreC implementation and potentially to create hashes to verify on the circuit. The implementation is specifically tailored for the prime field of order `0x73eda753299d7d483339d80809a1d80553bda402fffe5bfeffffffff00000001` and can't be used for any other field. The round constants and the MDS matrix for this implementation can be found in `public.json`, in the `round_constants_rust` and `MDS_matrix_rust` fields, respectively, or at https://gitlab.com/nomadic-labs/cryptography/ocaml-bls12-381-hash/-/blob/main/src/poseidon.ml?ref_type=heads, in variables `state_size_256_5_ark` and 
`state_size_256_5_mds`, respectively. The other constants are fixed to `t = 5`, `r = 4`, `c = o = 1`, `alpha = 5`, `R_F = 8` and `R_P = 59`. This is a third-party implementation, so none of these constants can be modified.

An important thing to note is that the `BlsScalar::from_hex_str` function expects the input hexadecimal string to be in **little-endian** byte order. The output of the `sponge::hash` function is also printed as a hexadecimal string in little-endian byte order.

#### Implementation in Python

The Python implementation is originally written in SageMath, the Python files are generated by the `sage` command line tool. Note that when using the `sage` command, the `.sage` file `example.sage` would be output as `example.sage.py`. If you want to import these output `.py` files in another file, you need to manually remove the `.sage` part from the name, because dot implies a submodule in Python imports. For example, if you wish to make changes in the `poseidon.sage` file, you need to manually rename the output of `sage poseidon.sage` to `poseidon.py` instead of `poseidon.sage.py`.

The inner permutation of the Python/SageMath implementation is provided by the authors of Poseidon at https://extgit.iaik.tugraz.at/krypto/hadeshash. In this directory, it has been modified a bit to match the code structure in ZK-SecreC. The outer sponge layer was added by us. All of the algorithm parameters can be modified at will for the Python implementation.

## Running the tests

The purpose of the scripts `test_rust.sh` and `test_python.sh` is to compare the output of the ZK-SecreC implementation with the outputs of the implementations in Rust and Python, respectively. The comparison is made for randomly generated data vectors of length 1 to 100 (the maximum data length can be modified in the Bash scripts).

To test against the Rust reference implementation, simply run
```
./test_rust.sh
```
in the subfolder `test`. To test against the Python reference implementation, run
```
./test_python.sh <prime> <rate>
```
in the subfolder `test`, where `<prime>` is the order of the finite field used for the ZK circuit and `<rate>` is the data rate in field elements. Note that both of these tests assume that the ZKSC test is located at `zksc/test.zksc`.

Note that in the Rust implementation, the first part of the inner state is the capacity part, not the rate part. Thus the `rate_first` parameter of the `poseidon` function should be set to `false` in the `zksc/test.zksc` file. Also note that in the Rust implementation, the *last* element of the inner state goes through an S-box in the partial rounds. Thus the `partial_rounds_pow_i` must be set to `t - 1` everywhere in the ZKSC source code where `poseidon_perm` is called. In the Python implementation, `rate_first` can set to either `True` or `False` at will. However, `partial_rounds_pow_i` must be set to `0` everywhere in the ZKSC source code where `poseidon_perm` is called, because the original implementation of the Poseidon permutation, which the Python implementation is based on, puts the first element through the S-box in the partial rounds.

## Asynchronous implementation in ZK-SecreC

Since Poseidon is a sponge function, then the Poseidon hash can be calculated asynchronously - by letting the sponge layer absorb data in several parts and then squeezing out the hash once all data has been input.
For this end, a new struct `PoseidonInstance` is defined, consisting of:
- a `PoseidonSpongeParams` struct, which is used to collect all parameters relevant to the sponge layer of the Poseidon hash function. Using the same notation as above, these parameters are:
    - `t`
    - `r`
    - `o`
    - `rate_first`
- a `PoseidonPermParams` struct, which is used to collect all parameters relevant to the inner Poseidon permutation. Using the same notation as above, these parameters are:
    - `alpha`
    - `R_F`
    - `R_P`
    - `round_constants` (grouped into a list of sublists of length `t`)
    - `MDS_matrix`
- a field elements list `inner_state` of length `t`, whose purpose is to contain the inner state of the sponge between data absorptions.

The asynchronous hash calculation functionality is provided in `Poseidon.zksc` in the following three functions:
- `poseidon_initialize` - this function gathers the relevant parameters into one `PoseidonInstance` struct and initializes the inner state to zeroes.
- `poseidon_continue` - this function is used to absorb new data into the sponge and update the `inner_state` of a `PoseidonInstance` object. Every call of this function must take as input a list of field elements whose length is divisible by `r`.
- `poseidon_finalize` - this function can be called on the relevant `PoseidonInstance` object once all data has been absorbed to squeeze out the resulting hash. Note that the final `poseidon_continue` call before calling `poseidon_finalize` should be called on data that is padded according to the `10*` rule, as in the standard implementation.

An example of using the asynchronous implementation can be found in the file `zksc/example_async.zksc`.
