/*
MIT License

Copyright (c) 2018 Markus Schofnegger

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// Common includes and definitions between files
#ifndef COMMON_H
#define COMMON_H

// If USE_CHRONO is not #defined then time measurement is skipped
//#define USE_CHRONO
#ifdef USE_CHRONO
#include <chrono>
#endif

//#include <iostream>

//typedef unsigned int uint32;
//typedef unsigned long uint64;

typedef __UINT32_TYPE__ uint32;
typedef __UINT64_TYPE__ uint64;
typedef unsigned char uchar;

// This struct is returned from the Circuit to ZKBPP::sign(.)
typedef struct {
  uchar* x_3_;
  uchar** witness_3_; // indices are: ring, byte number
  uchar* random_tapes_;
  uchar* random_tapes_hashs_;
  uchar*** views_; // indices are: ring, party, byte number
  uchar* y_;
  uchar* y_shares_;
  uchar** declassify3_1_; // indices are: ring, byte number; shares of 1st party sent to 3rd party during declassify3
  uchar** declassify3_2_; // indices are: ring, byte number; shares of 2nd party sent to 3rd party during declassify3
  uchar** assert_zero_shares_; // indices are: ring, byte number; for each ring: arguments of assert_zero, first all shares of 1st party, followed by all shares of 2nd party
} SignData;

// This struct is returned from the Circuit to ZKBPP:verify(.)
typedef struct {
  uchar** view_; // indices are: ring, byte number
  uchar* y_share_; // y share of the given branch
  uchar* y_e2_; // Has to be calculated from circuit, y_e+2 = y + y_e + y_e+1
  uchar** declassify3_1_; // indices are: ring, byte number; shares of prover's 1st party sent to prover's 3rd party during declassify3
  uchar** declassify3_2_; // indices are: ring, byte number; shares of prover's 2nd party sent to prover's 3rd party during declassify3
  uchar** assert_zero_shares_; // indices are: ring, byte number; for each ring: arguments of assert_zero, first all shares of verifier's 1st party, followed by all shares of 2nd party, followed by all shares of 3rd party
} VerifyData;

// This struct contains all SignData* from the iterations
typedef struct {
  SignData** sds_;
} ContainerSignData;

// This struct contains all VerifyData* from the iterations
typedef struct {
  VerifyData** vds_;
} ContainerVerifyData;

// C_ij = [H'(k_ij, View_ij)]
typedef struct {
  uchar* H_k_View_;
} C;

// Container for C's and D's (all iterations, C_ij and D_ij)
typedef struct {
  C*** Cs_; // Matrix, because (party_size * iterations) entries
} ContainerCD;

// a_ij = [y_i1, y_i2, y_i3, C_i1, C_i2, C_i3] (concatenation)
typedef struct {
  uchar* ys_C_hashs_;
} A;

// Container for A structs
// The data that will be hashed to get the challenge.
typedef struct {
  A** as_;
} ContainerA;

// z_i = [View_i+1, k_i, k_i+1, x_3] where e = i, + key_shares + y share
typedef struct {
  uchar** view_; // indices are: ring, byte number
  uchar* k_1_; // random tape of verifier's 1st party
  uchar* k_2_; // random tape of verifier's 2nd party
  uchar* k_1_hash_; // hash of random tape and key share of prover's 1st party
  uchar* k_2_hash_; // hash of random tape and key share of prover's 2nd party
  uchar* x_3_; // Not used if e_i = 0, key share of prover's 3rd party
  uchar** witness_3_; // indices are: ring, byte number; Not used if e_i = 0, witness shares of prover's 3rd party
  uchar** declassify3_; // indices are: ring, byte number; Not used if e_i = 0, shares of prover's 1st or 2nd party
  //uchar** key_shares_;
  uchar* y_share_;
} Z;

// This struct is returned from ZKBOO to the user (and can be used with ZKBPP::Verify(.))
// Proof contains the challenge E, all b_i's and all z_i's (i for iteration)
// p = [e, (b_1, z_1), ..., (b_t, z_t)]
typedef struct {
  uint32 num_iterations_;
  uchar* e_;
  uchar* e_i_; // derived from e_; e_i_[i] is 0, 1, or 2 for each iteration i; does not need to be serialized
  // b_i Part
  uchar** y_e2_;
  uchar** H_k_View_;
  Z** zs_;
} Proof;

// Forward declarations
class Circuit;

#endif
