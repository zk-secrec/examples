#!/bin/bash
OPT="-O0"
#OPT="-O2"
OPT2="-O2"
#OPT2="$OPT"

#CXX=clang
CXX=g++

if [ "$CXX" = clang ]; then
    /usr/bin/time clang "$OPT" -c generated_direct.cpp &&
    /usr/bin/time clang "$OPT" -c generated.cpp &&
    /usr/bin/time clang "$OPT2" main.cpp ZKBPP.cpp CircuitContainer.cpp BigIntLib.cpp generated.o generated_direct.o -o zkbpp_test -lcrypto -lstdc++
else
    CFLAGS+="-I/usr/include/openssl -Wunused"
    /usr/bin/time ${CXX} -std=c++0x -g "$OPT" -march=native -mtune=native ${CFLAGS} -c generated_direct.cpp &&
    /usr/bin/time ${CXX} -std=c++0x -g "$OPT" -march=native -mtune=native ${CFLAGS} -c generated.cpp &&
    /usr/bin/time ${CXX} -std=c++0x -g "$OPT2" -march=native -mtune=native ${CFLAGS} main.cpp ZKBPP.cpp CircuitContainer.cpp BigIntLib.cpp generated.o generated_direct.o -o zkbpp_test -lcrypto
fi
