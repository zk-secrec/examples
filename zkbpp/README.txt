Using the Modified ZKB++ Backend with the zk-anon-auth Circuit

First check that the file zkbpp/modified-zkbpp/code/_compile_zkbpp.sh contains
the correct directory for openssl header files and change it if necessary.
Currently the directory is /usr/include/openssl.

Then put this zkbpp folder in the compiler root directory.

In order to prove and verify the zk-anon-auth circuit, go to the directory zkbpp
    cd zkbpp
and run the following scripts there:
    ./run_zkscc
    ./compile_circuit
    ./prove_and_verify 1
    ./prove_and_verify 2

./run_zkscc runs the ZK-SecreC compiler to compile the code in zkbpp/zk-anon-auth
into Rust code and then compiles and runs the Rust code, which uses the JSON
files in the directory zkbpp/zk-anon-auth to generate the C++ files
generated_header.h, generated_direct.cpp, and generated.cpp, and the instance
and witness files generated_instance.txt and generated_witness.txt in the
directory zkbpp.

./compile_circuit compiles the generated C++ files together with the static code
in zkbpp/modified-zkbpp/code.

./prove_and_verify 1 runs the compiled C++ code as prover,
reading generated_instance.txt and generated_witness.txt,
writing the proof into the binary file zkbpp/modified-zkbpp/code/proof.bin.

./prove_and_verify 2 runs the compiled C++ code as verifier,
reading generated_instance.txt, as well as the proof from zkbpp/modified-zkbpp/code/proof.bin.
It writes to the standard output whether the proof was accepted or rejected.
