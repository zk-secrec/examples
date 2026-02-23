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

// Functions for operations in prime fields and binary fields
#ifndef BIGINTLIB_H
#define BIGINTLIB_H

//#include <chrono>
//#include <iostream>

// If USE_FSTREAM is undefined then dependence on fstream is avoided but
// then reading the instance and witness files and reading and writing the proof do not work
// thus the code will compile but will get a runtime error.
#define USE_FSTREAM

// If USE_OPENSSL is undefined then dependence on OpenSSL is avoided but
// then operations that use OpenSSL do not work
// thus the code will compile but will get a runtime error.
#define USE_OPENSSL

// If USE_CSTDLIB is undefined then dependence on cstdlib is avoided but
// then exit does not work and there will probably be an error elsewhere, e.g. segmentation fault
#define USE_CSTDLIB

#ifdef USE_CSTDLIB
#include <cstdlib> // exit
#else
#define exit exit2
void exit2(int status);
#endif

#ifdef USE_OPENSSL
#include <openssl/evp.h> // For AES
#endif

#define WORD_SIZE 64

//typedef unsigned short uint16;
//typedef unsigned int uint32;
//typedef unsigned long uint64;
//typedef __uint128_t uint128;
//typedef unsigned char uchar;
//typedef uint64 word;
//typedef uint128 doubleword;
//typedef long signedword;

typedef __UINT16_TYPE__ uint16;
typedef __UINT32_TYPE__ uint32;
typedef __UINT64_TYPE__ uint64;
typedef __uint128_t uint128;
typedef unsigned char uchar;
typedef uint64 word;
typedef uint128 doubleword;
typedef __INT64_TYPE__ signedword;

class BigIntLib {
public:
  BigIntLib() {}
  ~BigIntLib() {}

  void Init(uint32 branch_size_bits, uint32 field_type, uint32 ring_variant);
  void CleanUp();
  static void InitStatic();
  static void CleanUpStatic();

  // Wrappers
  void Add(word* c, word* a, word* b);
  //void AddSpec(word* c, word* a, word* b); // Addition without reduction
  void Sub(word* c, word* a, word* b);
  void ShlC(word* b, word* a, uint32 n); // a << n
  void ShrC(word* b, word* a, uint32 n); // a >> n
  void Mul(word* c, word* a, word* b);
  //void MulSpec(word* c, word* a); // Multiplication with a constant value
  //void SolinasReduc(word* out, word* in);
  //void DoubleAdd(word* c, word* a, word* b);
  void Times2(word* c, word* a);
  void Times3(word* c, word* a);
  bool IsZero(word* a);
  bool Smaller(word* a, word* b); // Not yet a wrapper, probably not needed as wrapper, neither is Greater(.)
  bool Greater(word* a, word* b);
  uchar Compare(word* a, word* b); // Not yet a wrapper, probably not needed as wrapper, neither is Greater(.)
  void Power(word* c, word* a, word* b, uint32 n);
  bool Sqrt(word* c, word* a); // Not a wrapper
  void Inverse(BigIntLib* ringBW, word* result, word* x);
  void TryReduce(word* a);
  void GetRandomFieldElements(void* destination, uchar* seed, uint32 num_elements);
  //void GetRandomFieldElementsNonZero(void* destination, uchar* seed, uint32 num_elements);

  // Util
  static void SetSeed(uchar* seed);
  static void GetNextRandomnessFromSeed(uchar *dest, uint32 num_bytes = 16); // using the seed set by SetSeed
  static void FillRandom(void* destination, uchar* seed, uint32 num_bytes);
  void Print(void* source, uint32 num_bytes);
  //std::string ToString(void* source, uint32 num_bytes = 0); // Always in field size

  // Variables
  uint32 field_type_;
  uint32 ring_variant_;
  uint32 field_size_bits_;
  uint32 field_size_bytes_;
  uint32 field_num_words_;
  uint64 msb_word_mask_;
  uint32 fastreduc_num_words_;
  word* modulo_;
  word* minus_modulo_; // 2^field_size_bits_ - modulo_
  word* minus_modulo_bits_; // all bits of minus_modulo_bits_[i] are equal to the ith bit of minus_modulo_
#ifdef USE_OPENSSL
  static EVP_CIPHER_CTX* evp_cipher_ctx_;
#endif
  static uchar* evp_cipher_buffer_;
  //int** reduction_matrix_;
  //int** reduction_matrix_positive_;
  //int** reduction_matrix_negative_;
  //uint32 mod_addition_weight_;
  //int** mod_addition_matrix_;
  //uint32 mod_subtraction_weight_;
  //int** mod_subtraction_matrix_;
  uint32 num_mul_gates_;
  uint32 num_random_gates_;
  uint32 num_intermediate_results_;
  uint32 num_assert_zeros_;
  uint32 num_instance_values_;
  uint32 num_witness_values_;
  uint32 num_ring_conversions_; // conversions from a bitwise ring to an additive ring with the same number of bits; num_ring_conversions_ should be set only for the bitwise ring
  uint32 num_ecdsa_verifications_;
  uint32 num_declassify3s_;
  uint32 gate_size_;
  uint32 gate_num_words_;
  uint32 value_size_;
  BigIntLib* ringP_; // the ring modulo P corresponding to the elliptic curve
  BigIntLib* ringBW_; // the bitwise ring with the same number of bits as the ring modulo P corresponding to the elliptic curve

  uint32 witness_3_size() { return this->num_witness_values_ * this->gate_num_words_ * (WORD_SIZE / 8); }
  uint32 declassify3_size() { return this->num_declassify3s_ * this->gate_size_; }
  uint32 view_size() { return this->num_mul_gates_ * this->gate_size_; }

  uint32 bitwise_ring_no_; // the number of the bitwise ring with the same number of bits as the current additive ring or prime field
  uint32 ring_no_; // the number of the current ring

  // Circuit inner computation
  uint32 current_mul_gate_;
  uint32 current_random_gate_;
  uint32 current_intermediate_result_;
  uint32 current_declassify3_;
  uint32 current_assert_zero_;
  uint32 current_assert_eq_bw_ad_;
  uint32 current_instance_;
  uint32 current_witness_;

private:
  // Helpers for reduction
  //void ComputeReductionMatrix(int* t_coeff);
  //void ComputeModularAdditionMatrix();
  //void ComputeModularSubtractionMatrix();

  // Utils
  bool GreaterPF(word* a, word* b);
  bool GreaterBF(word* a, word* b);
  void TryReducePF(word* a);
  void TryReduceBF(word* a);
  void GetRandomFieldElementsEC(void* destination, uchar* seed, uint32 num_elements);
  void GetRandomFieldElementsPF(void* destination, uchar* seed, uint32 num_elements);
  void GetRandomFieldElementsBF(void* destination, uchar* seed, uint32 num_elements);
  //void GetRandomFieldElementsNonZeroPF(void* destination, uchar* seed, uint32 num_elements); // Probably slightly redundant
  //void GetRandomFieldElementsNonZeroBF(void* destination, uchar* seed, uint32 num_elements); // Probably slightly redundant
  void InverseGeneric(BigIntLib* ringBW, word* result, word* x);

  // Function variables
  void (BigIntLib::*ADD_FUNCTION)(word*, word*, word*);
  //void (BigIntLib::*ADD_SPEC_FUNCTION)(word*, word*, word*);
  void (BigIntLib::*SUB_FUNCTION)(word*, word*, word*);
  void (BigIntLib::*MUL_FUNCTION)(word*, word*, word*);
  //void (BigIntLib::*MUL_SPEC_FUNCTION)(word*, word*);
  void (BigIntLib::*SHLC_FUNCTION)(word*, word*, uint32);
  void (BigIntLib::*SHRC_FUNCTION)(word*, word*, uint32);
  //void (BigIntLib::*REDUC_FUNCTION)(word*, word*);
  //void (BigIntLib::*DOUBLE_ADD_FUNCTION)(word*, word*, word*);
  //void (BigIntLib::*TIMES_2_FUNCTION)(word*, word*);
  //void (BigIntLib::*TIMES_3_FUNCTION)(word*, word*);
  bool (BigIntLib::*GREATER_FUNCTION)(word*, word*);
  void (BigIntLib::*TRY_REDUCE_FUNCTION)(word*);
  void (BigIntLib::*GET_RANDOM_FIELD_ELEMENTS_FUNCTION)(void*, uchar*, uint32);
  //void (BigIntLib::*GET_RANDOM_FIELD_ELEMENTS_NON_ZERO_FUNCTION)(void*, uchar*, uint32);

  //// Prime field with 3 bits
  //void AddPF3(word* c, word* a, word* b);
  //void SubPF3(word* c, word* a, word* b);
  //void MulPF3Fast(word* c, word* a, word* b);

  //// Prime field with 4 bits
  //void AddPF4(word* c, word* a, word* b);
  //void SubPF4(word* c, word* a, word* b);
  //void MulPF4Fast(word* c, word* a, word* b);
  //void MulPF4FastCrandall(word* c, word* a, word* b);
  //void DoubleAddPF4(word* c, word* a, word* b);

  //// Prime field with 16 bits
  //void AddPF16(word* c, word* a, word* b);
  //void AddSpecPF16(word* c, word* a, word* b);
  //void SubPF16(word* c, word* a, word* b);
  //void MulPF16Fast(word* c, word* a, word* b);
  //void MulSpecPF16(word* c, word* a);
  //void SolinasReducPF16(word* out, word* in);
  //void MulPF16FastCrandall(word* c, word* a, word* b);

  //// Prime field with 32 bits
  //void AddPF32(word* c, word* a, word* b);
  //void SubPF32(word* c, word* a, word* b);
  //void MulPF32Fast(word* c, word* a, word* b);
  //void MulPF32FastCrandall(word* c, word* a, word* b);
  //
  //// Prime field with 61 bits
  //void AddPF61(word* c, word* a, word* b);
  //void SubPF61(word* c, word* a, word* b);
  //void MulPF61Fast(word* c, word* a, word* b);

  //// Prime field with 64 bits
  //void AddPF64(word* c, word* a, word* b);
  //void SubPF64(word* c, word* a, word* b);
  //void MulPF64(word* c, word* a, word* b);
  //void MulPF64Fast(word* c, word* a, word* b);
  //void MulPF64FastCrandall(word* c, word* a, word* b);

  //// Prime field with 136 bits
  //void AddPF136(word* c, word* a, word* b);
  //void SubPF136(word* c, word* a, word* b);
  //void MulPF136Fast(word* c, word* a, word* b);

  // Prime fields with 256 bits
  void AddPF256(word* c, word* a, word* b);
  void SubPF256(word* c, word* a, word* b);
  //void MulPF256Fast(word* c, word* a, word* b);
  void Mul256_512(word* c_temp, word* a, word* b); // multiply two 256-bit values, getting a 512-bit value
  void MulPF256PFast(word* c, word* a, word* b); // modulus P, reduction by additions and subtractions
  void MulPF256QFast(word* c, word* a, word* b); // modulus Q, reduction by multiplications
  void Inverse256(BigIntLib* ringBW, word* result, word* x);

  //// Prime field with 272 bits
  //void AddPF272(word* c, word* a, word* b);
  //void SubPF272(word* c, word* a, word* b);
  //void MulPF272Fast(word* c, word* a, word* b);

  // Prime fields with 384 bits
  bool GreaterEq384(word* a, word* b);
  void AddPF384(word* c, word* a, word* b);
  void SubPF384(word* c, word* a, word* b);
  void MulPF384PFast(word* c, word* a, word* b); // modulus P, reduction by additions and subtractions, version 1
  void MulPF384PFast1(word* c, word* a, word* b); // modulus P, reduction by additions and subtractions, version 2
  void MulPF384PFast2(word* c, word* a, word* b); // modulus P, reduction by multiplications
  void MulPF384QFast(word* c, word* a, word* b); // modulus Q, reduction by multiplications
  void Mul384_768(word* c_temp, word* a, word* b); // multiply two 384-bit values, getting a 768-bit value
  void Inverse384(BigIntLib* ringBW, word* result, word* x);

  //// Prime fields
  //void Times2PF(word* c, word* a);
  //void Times3PF(word* c, word* a);

  //// Binary field with 3 bits
  //void XorBF3(word* c, word* a, word* b);
  //void MulBF3Fast(word* c, word* a, word* b);
  //void Times2BF3(word* c, word* a);
  //void Times3BF3(word* c, word* a);

  //// Binary field with 17 bits
  //void XorBF17(word* c, word* a, word* b);
  //void MulBF17Fast(word* c, word* a, word* b);
  //void Times2BF17(word* c, word* a);
  //void Times3BF17(word* c, word* a);

  //// Binary field with 33 bits
  //void XorBF33(word* c, word* a, word* b);
  //void MulBF33Fast(word* c, word* a, word* b);
  //void Times2BF33(word* c, word* a);
  //void Times3BF33(word* c, word* a);

  //// Binary field with 65 bits
  //void XorBF65(word* c, word* a, word* b);
  //void MulBF65Fast(word* c, word* a, word* b);
  //void Times2BF65(word* c, word* a);
  //void Times3BF65(word* c, word* a);

  // Ring of integers modulo 2^32
  void TryReduceR32(word* a);
  void AddR32(word* c, word* a, word* b);
  void SubR32(word* c, word* a, word* b);
  void MulR32(word* c, word* a, word* b);

  // Ring of integers modulo 2^64
  void TryReduceR64(word* a);
  void AddR64(word* c, word* a, word* b);
  void SubR64(word* c, word* a, word* b);
  void MulR64(word* c, word* a, word* b);

  // Ring of 32-bit vectors with bitwise operations
  void ShlcBW32(word* b, word* a, uint32 n);

  // Ring of 64-bit vectors with bitwise operations
  void XorBW64(word* c, word* a, word* b);
  void AndBW64(word* c, word* a, word* b);
  void ShlcBW64(word* b, word* a, uint32 n);
  void ShrcBW64(word* b, word* a, uint32 n);

  // Ring of n-bit vectors with bitwise operations for n <= 64
  void TryReduceRAtMost64(word* a);
  void ShlcBWAtMost64(word* b, word* a, uint32 n);

  // Ring of 256-bit vectors with bitwise operations
  void XorBW256(word* c, word* a, word* b);
  void AndBW256(word* c, word* a, word* b);
  void ShlcBW256(word* b, word* a, uint32 n);
  void ShrcBW256(word* b, word* a, uint32 n);

  // Ring of 384-bit vectors with bitwise operations
  void XorBW384(word* c, word* a, word* b);
  void AndBW384(word* c, word* a, word* b);
  void ShlcBW384(word* b, word* a, uint32 n);
  void ShrcBW384(word* b, word* a, uint32 n);

  // Elliptic curve additive group
  void AddEC(word* result, word* p1, word* p2);
  void NegateEC(word* result, word* p);
  void SubEC(word* result, word* p1, word* p2);
};

#endif // BIGINTLIB_H
