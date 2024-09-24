# Small ZK-SecreC examples

## Introduction

The examples in this folder demonstrate the use of some particular feature of the language, or the compiler, or the standard library.

To run the example `.zksc`-files, execute `./runrust` with the following arguments in the following order:

1. If there is a `.ccc` file bundled with the example you wish to run, pass `-c name_of_ccc_file.ccc` as the first option.
1. The `.zksc`-file.
1. The JSON file that is used by `get_public` to read the values. If no public values are read in the `.zksc` program, pass `""`.
1. The JSON file that is used by `get_instance` to read the values. It is used by both the prover and the verifier during the runtime, or for generating the `instance` file of SIEVE IR. If no instance values are read in the `.zksc` program, pass `""`.
1. The JSON file that is used by `get_witness` to read the values. It is used by the prover during the runtime, or for generating the `short_witness` file of SIEVE IR. If no witness values are read in the `.zksc` program, pass `""`.
1. (optional) The path to the folder, where Bristol format circuits are stored.  These are normally in the `circuits` folder.
1. (optional) The prefix of the names of the files (together with the path) that the compiler generates for the descriptions of the relation, the instance, and the witness in the CircuitIR format. If `""` is passed, then the output is the standard output.

In this way, you can create the description of the circuit (describing the statement that we want to prove under ZK) and both of its inputs (one from the prover, the other one from both the prover and the verifier) in the [Circuit IR](https://github.com/sieve-zk/ir) format. See the documentation of the compiler to execute these statements with either the Mac'n'Cheese or EMP backends.

An overview of all possible parameters of `runrust` is also given if you execute `./runrust` without any arguments.

## Content of example ZKSC files in alphabetical order

### example_assert.zksc & assert_unknown/example_assert_unknown.zksc

These demonstrates the use of the ZK-SecreC built-in function assert, which is used to assert that a boolean value (either defined in the program source code or read from the instance JSON file) equals `true`.

#### public parameters, instance, witness

Use `assert_unknown/example_assert_unknown_instance.json` for `example_assert_unknown.zksc`.

### example_bigint.zksc

This demonstrates the use of the `BigInt` datatype and some of the functionality from the `BigInt.zksc` standard library module.

### challenge/example_challenge.zksc

ZK-SecreC has the call `challenge()` to incorporate proving-time challenges from verifier. These are useful for e.g. implementing random access memory. This example code basically describes the use of the challenge value generating call on a dummy example.

The type of `challenge` is

```
challenge[N : Nat]() -> uint[N] $pre @verifier
```

SIEVE IR does not support challenges; if a code using challenges is compiled into CircuitIR, then the `instance` file will contain some random values at this point, and the `witness` file the values that have been computed in response to these challenges. Executing these again does not prove anything. We have worked with the teams developing ZK protocols to get a proper support for challenges.

#### public parameters, instance, witness

Use e.g. `challenge/example_challenge_instance.json` and `challenge/example_challenge_witness.json`.

### circuit_hash/example_circuit_hash.zksc

This demonstrates the use of Bristol circuits. There is a function

```
call[N : Nat, $S, @D](name : string $pre @public, inps : list[list[bool[N] $S @D]]) -> list[list[bool[N] $S @D]]
```

that invokes the circuit named `name` in the `circuits`-folder in the ZK-SecreC compiler repository. The specification of the circuit states, how many bitstrings of which length it inputs and outputs. This list of bit-strings is represented as list of lists of booleans.

#### public parameters, instance, witness

Use `circuit_hash/example_circuit_hash_instance.json` and `circuit_hash/example_circuit_hash_witness.json`.

### conv/example_conv.zksc

This example demonstrates how to convert integers from one field to another.

#### CCC file

Use `example_conv.ccc`.

#### public parameters, instance, witness

Use `conv/example_conv_instance.json` and `conv/example_conv_witness.json`. Also use the `example_conv.ccc` as the CCC-file.

### cube/example_cube.zksc

The simplest example there is. It states that the prover knows a cube root of a value known by the verifier.

#### public parameters, instance, witness

Use `cube/example_cube_instance.json` and `cube/example_cube_witness.json`

### dfa/example_dfa.zksc

This example demonstrates the use of the DFA execution functionality that is included in the standard library.

#### public parameters, instance, witness

Use `dfa/example_dfa_public.json` and `dfa/example_dfa_instance.json`. Note that the DFA or the input string could also be known to the prover only.

### disjunction/example_disjunction.zksc

This examples demonstrates the use of the disjunction/switch statement in ZK-SecreC.

#### public parameters, instance, witness

Use `disjunction/example_disjunction_witness.json`.

### example_domain_polymorphism.zksc

This example demonstrates how to define a function whose inputs and outputs are polymorphic over the domain (the same function can be called with arguments in either the `@public`, `@verifier` or `@prover` domain).

### factor/example_factor.zksc

This example states that the prover knows a factorization of a value known by the verifier. The factorization consists of two integers, one of which is read using `get_witness`. Both are stated to be not equal to `1`.

#### public parameters, instance, witness

Use `factor/example_factor_instance.json` and `factor/example_factor_witness.json`.

### ffi/example_ffi.zksc

This example demonstrates the use of the Foreign Function Interface (FFI) of ZK-SecreC, allowing one to call functions implemented in Rust, and to directly use Rust's datatypes. There is a longer comment explaining ZK-SecreC's FFI at the beginning of this example.

#### public parameters, instance, witness

There are no public parameters, instance, or witness. But there is some code that has been implemented in Rust. This code is located in `ffi/example_ffi.rs`, and has to be included using the `-e` flag of `runrust`.

### example_fixed.zksc

This example demonstrates the use of the datatype for fixed-point numbers defined in the `FixedPoint.zksc` file of the standard library.

### example_forbiddeneffect.zksc

This example demonstrates an effect constraint that our compiler can detect. In this example, code branches based on a `$post` stage boolean, which means that the value of the boolean can be detected from the shape of the resulting circuit. Our compiler detects this unsolved effect constraint and throws an error.

### example_parametric_polymorphism.zksc

Shows the possible polymorphism over domains, stages, and data types.

Not shown here, but: the polymorphism may also be over moduli (i.e. field sizes).

There is also ad-hoc polymorphism. See e.g. the function `uint_cond` (computing the ternary `?:` operator from C-like languages) in `Std.zksc` in the standard library. It is possible to branch according to the values of the type variables.

### example_permutation_check.zksc

This example demonstrates the usage of the `PermutationCheck` plugin of CircuitIR in more detail. It states that tensors of different dimensions (as vectors of smaller-dimensional tensors) are permutations of each other.

### plugins/example_plugins.zksc

This example demonstrates the usage of the `ExtendedArithmetic`, `Vectors`, and `PermutationCheck` plugins of CircuitIR. 

#### public parameters, instance, witness

Use `plugins/example_plugins_public.json`, `plugins/example_plugins_instance.json` and `plugins/example_plugins_witness.json`.

### example_primitive_types.zksc

This is another example (besides the FFI example) of using Rust's features directly in ZK-SecreC code. In this case, we are using the primitive data types of Rust.

### example_sieve.zksc

This example demonstrates the use of sieve functions. These are functions marked with the `sieve` keyword that are compiled to their own separate function gates in CircuitIR. Functions marked with the `sieve` keyword can also be applied vectorized. This example shows how to apply a sieve function with integer arguments vectorized as well as partially. See the ZK-SecreC documentation for more information about sieve functions.

### example_stores.zksc

These are certain structures with finalizers, for which we have introduced special syntax. A _store_ is a mutable dictonary, i.e. a RAM. An empty store is denoted by `{#}`. One can read a store `s` at position `i` by the code snippet `s{# i}`, and write to it by the code snippet `s{# i} = v`. Both the indices and values are located on-circuit.

Stores in ZK-SecreC predate the finalizers. Together with finalizers, this special syntax would not be necessary any more.

### string/example_string.zksc

This example demonstrates the usage of the `String.zksc` module of the standard library. It shows how to prove that two substrings of given strings equal each other.

#### public parameters, instance, witness

Use `string/example_string_public.json`, `string/example_string_instance.json` and `string/example_string_witness.json`.

### example_structvec.zksc

This example further illustrates the usage of vectorization and sieve functions by applying a sieve function vectorized on a vector of structs.

### example_tensor.zksc

This example illustrates the use of "multi-dimensional vectors": tensors. It shows how vectors are unflattened into tensors and flattened back to vectors, how the elements of a tensor are read, and how slices can be taken.

### example_vectorized_operations.zksc

This demonstrates the use of vectors and the functionality from the `Vec.zksc` standard library module.

## Support by back-ends

Some of these small examples can be executed with Mac'n'Cheese and/or EMP back-ends. Others cannot: they may be using features / extensions of CircuitIR that the publicly available versions of these back-ends do not support. We have checked that the examples `assert`, `circuit_hash`, `cube`, `domain_polymorphism`, and `fixed` are supported by the Mac'n'Cheese back-end. At least the examples `bigint` and `cube` are supported by the EMP back-end.

