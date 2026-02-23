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

#ifndef CIRCUITCONTAINER_H
#define CIRCUITCONTAINER_H

#include "common.h"
//#include "Circuit.h"
#include "BigIntLib.h"
//#include <vector>

class CircuitContainer {
public:
  CircuitContainer();
  ~CircuitContainer();
  void initConsts();
  void initBigIntLib();
  void init(uint32 value_size, uint32 random_tape_size, uint32 key_size, uint32 branch_bits, uint32 num_branches, uint32 field_tpye, uint32 party_size, bool need_prove);
  void initCipher(uint32 cipher_type);
  void initMiMC();
  void readInstance();
  void readWitness();
  void runSign(uchar* x, SignData* sign_data);
  void runVerify(Proof* p, uchar* x, uchar* y, VerifyData* verify_data, uint32 iteration);
  void directEncryption(uchar* x, uchar* y); // Wrapper
  void directMiMC(uchar* x, uchar* y);
  void directImpl();
  void getParams(uint32* value_size, uint32* random_tape_size, uint32* key_size, uint32* gate_size);
  void randomizeKey();
  void setRing(uint32 ring_no);
  uchar* getKey();
  uint32 getLastCircuitSignNS();
  uint32 getLastCircuitVerifyNS();
  uint64 getLastDirectCallCycles();
  uint32 getLastDirectCallNS();
  uint32 getCipherNumBranches();

  uint32 num_rings_;
  uint32 ring_no_;
  uint32 bw_word_ring_no_; // bitwise ring with WORD_SIZE bits
  BigIntLib *all_big_int_libs_;
  BigIntLib *big_int_lib_;
  
private:
  uint32 party_size_;
  uint32 value_size_; // Size of x and y (assuming x_size == y_size)
  uint32 random_tape_size_;
  uint32 key_size_; // Size of the key
  uint32 min_key_hash_size_; // min(key_size_, hash_size_), to avoid memory overflow in some cases
  uint32 branch_size_; // Part of the gate which is actually used
  uint32 gate_size_; // Size of the gate operations (MUL, ADD, ...)
  uint32 gate_num_words_;
  uint32 branch_bits_;
  uint32 hash_size_;
  uint32 num_mul_gates_;
  uint32 num_feistel_branches_; // Used for Feistel networks
  word*** all_intermediate_results_; // indices are: ring, i, word 
  word** intermediate_results_; // Intermediate multiplication results of the unshared implementation
  uchar* value_;
  uchar** value_shares_; // Should always be party_size entries with size value_size
  uchar* key_;
  uchar** key_shares_;
  word*** all_random_numbers_; // indices are: ring, party, i
  word** random_numbers_; // indices are: party, i
  word** all_instance_values_; // indices are: ring, i
  word*** all_instance_shares_; // indices are: ring, party, i
  word** all_witness_values_; // indices are: ring, i
  word*** all_witness_shares_; // indices are: ring, party, i
  word** assert_eq_bw_shares_; // indices are: ring, i
  word** assert_eq_ad_shares_; // indices are: ring, i; the ring is the bitwise ring, not the additive ring
  word* squaring_precomp_;
  //std::vector<uint32> feistel_branch_indices_;
  SignData* sign_data_;
  Proof* proof_;
  VerifyData* verify_data_;

  // Circuit inner computation
  uint32 e_; // e chosen be ZKB++ protocol
  uint32 iteration_; // iteration number in the ZKB++ protocol (maybe better to just pass the right data from the outside?)
  bool verify_; // true if we are verifying, false if we are proving

  // Helpers
  uint32 last_circuit_sign_time_;
  uint32 last_circuit_verify_time_;
  uint64 last_direct_call_cycles_;
  uint32 last_direct_call_time_;

  // Function pointers
  void (CircuitContainer::*direct_function_)(uchar* x, uchar* y);
  void (CircuitContainer::*circuit_function_)(word* value_shares_f, word* key_shares_f, uint32 party_size);
  void (CircuitContainer::*get_instance_shared_function_)(word* a_shares);
  void (CircuitContainer::*get_witness_shared_function_)(word* a_shares);
  void (CircuitContainer::*assert_zero_shared_function_)(word* a_shares);
  void (CircuitContainer::*assert_eq_bw_ad_shared_function_)(uint32 bw_ring_no, uint32 ad_ring_no, word* bw_shares, word* ad_shares);
  void (CircuitContainer::*bitwise_to_bitwise_shared_function_)(uint32 from_ring_no, uint32 to_ring_no, word* a_shares, word* b_shares);
  void (CircuitContainer::*bitwise_vec_to_bitwise_vec_shared_function_)(uint32 from_ring_no, uint32 to_ring_no, uint32 n_from, word* a_shares, word* b_shares);
  void (CircuitContainer::*bitwise_matrix_transpose_shared_function_)(uint32 from_ring_no, uint32 to_ring_no, uint32 nr, uint32 nc, word* a_shares, word* b_shares);
  void (CircuitContainer::*ecdsa_verification_shared_function_)(uint32 bits_per_block, word* fixpowers1, word* scalar1, word* fixpowers2, word* scalar2, word* r_x, word* r_y);
  void (CircuitContainer::*copy_c_shared_function_)(word* b, word* c_shares);
  void (CircuitContainer::*add_c_shared_function_)(word* a_shares, word* b, word* c_shares);
  void (CircuitContainer::*mul_c_shared_function_)(word* a_shares, word* b, word* c_shares);
  void (CircuitContainer::*shl_c_shared_function_)(word* a_shares, uint32 n, word* b_shares);
  void (CircuitContainer::*shr_c_shared_function_)(word* a_shares, uint32 n, word* b_shares);
  void (CircuitContainer::*add_shared_function_)(word* a_shares, word* b_shares, word* c_shares);
  void (CircuitContainer::*sub_shared_function_)(word* a_shares, word* b_shares, word* c_shares);
  void (CircuitContainer::*mul_shared_function_)(word* a_shares, word* b_shares, word* c_shares);
  void (CircuitContainer::*squ_shared_function_)(word* a_shares, word* b_shares, word* c_shares);
  void (CircuitContainer::*squ_shared_experimental_function_)(word* a_shares, word* b_shares, word* c_shares);
  void (CircuitContainer::*cube_shared_function_)(word* a_shares, word* c_shares); // b_shares probably not needed
  void (CircuitContainer::*gen_new_subkeys_shared_function_)(word* key_shares_f, word* key_share_f_temp, uint32 num_rounds_remaining, uint32 party_size);
  void (CircuitContainer::*prepare_shares_field_sign_function_)(uchar* x, word* value_shares_f, word* key_shares_f);
  void (CircuitContainer::*prepare_shares_field_verify_function_)(word* value_shares_f, word* key_shares_f);
  void (CircuitContainer::*output_shares_to_bytes_function_)(word* output_shares, uint32 party_size);
  void (CircuitContainer::*verify_calc_last_share_function_)(uchar* y, word* value_shares_f);

  // View allocation (used for SignData* and VerifyData*)
  //View* createView(uint32 num_gates, uint32 gate_size);

  // Specific circuits (all circuits should take the same parameters!)
  void circuitMiMC(word* value_shares_f, word* key_shares_f, uint32 party_size);
  void circuitImpl(uint32 party_size);

  // Circuit preparation and finalization
  void beforeSign(uchar* x, SignData* sign_data, word* value_shares_f, word* key_shares_f);
  void afterSign(word* value_shares_f);
  void beforeVerify(Proof* p, uchar* x, uchar* y, VerifyData* verify_data, uchar** key_shares, word* value_shares_f, word* key_shares_f, uint32 iteration);
  void afterVerify(uchar* y, word* value_shares_f);

  // Direct
  void getInstanceDirect(word* a);
  void getWitnessDirect(word* a);
  void assertZeroDirect(word* a);
  void mulDirect(word* c, word* a, word* b);
  void bitwiseToBitwiseDirect(uint32 to_ring_no, uint32 from_ring_no, word* b, word* a);
  void bitwiseVecToBitwiseVecDirect(uint32 from_ring_no, uint32 to_ring_no, uint32 n_from, word* b, word* a);
  void bitwiseMatrixTransposeDirect(uint32 from_ring_no, uint32 to_ring_no, uint32 nr, uint32 nc, word* b, word* a);
  void concatVecDirect(uint32 ring_no, uint32 n1, uint32 n2, word* c, word* a, word* b);

  // Circuit
  void prepareRandomNumbers(uchar** random_tapes, uint32 party_size);
  void destroyRandomNumbers();
  void getInstanceSharedSign(word* a_shares);
  void getInstanceSharedVerify(word* a_shares);
  void getWitnessSharedSign(word* a_shares);
  void setWitnessSharedSign(word* witness_value, word* a_shares);
  void getWitnessSharedVerify(word* a_shares);
  void assertZeroSharedSign(word* a_shares);
  void assertZeroSharedVerify(word* a_shares);
  void addSharedSign(word* a_shares, word* b_shares, word* c_shares);
  void subSharedSign(word* a_shares, word* b_shares, word* c_shares);
  void addSharedVerify(word* a_shares, word* b_shares, word* c_shares);
  void subSharedVerify(word* a_shares, word* b_shares, word* c_shares);
  void copyCSharedSign(word* b, word* c_shares);
  void copyCSharedVerify(word* b, word* c_shares);
  void addCSharedSign(word* a_shares, word* b, word* c_shares);
  void addCSharedVerify(word* a_shares, word* b, word* c_shares);
  void shlCSharedSign(word* a_shares, uint32 n, word* b_shares);
  void shlCSharedVerify(word* a_shares, uint32 n, word* b_shares);
  void shrCSharedSign(word* a_shares, uint32 n, word* b_shares);
  void shrCSharedVerify(word* a_shares, uint32 n, word* b_shares);
  void mulCSharedSign(word* a_shares, word* b, word* c_shares);
  void mulCSharedVerify(word* a_shares, word* b, word* c_shares);
  void bitwiseToBitwiseSharedSign(uint32 from_ring_no, uint32 to_ring_no, word* a_shares, word* b_shares);
  void bitwiseToBitwiseSharedVerify(uint32 from_ring_no, uint32 to_ring_no, word* a_shares, word* b_shares);
  void bitwiseVecToBitwiseVecSharedSign(uint32 from_ring_no, uint32 to_ring_no, uint32 n_from, word* a_shares, word* b_shares);
  void bitwiseVecToBitwiseVecSharedVerify(uint32 from_ring_no, uint32 to_ring_no, uint32 n_from, word* a_shares, word* b_shares);
  void bitwiseMatrixTransposeSharedSign(uint32 from_ring_no, uint32 to_ring_no, uint32 nr, uint32 nc, word* a_shares, word* b_shares);
  void bitwiseMatrixTransposeSharedVerify(uint32 from_ring_no, uint32 to_ring_no, uint32 nr, uint32 nc, word* a_shares, word* b_shares);
  void concatVecShared(uint32 ring_no, uint32 n1, uint32 n2, word* a_shares, word* b_shares, word* c_shares);
  void subXorSharedSign(uint32 n, uint32 ring_no, bool compute_carry, word* b_shares, word* r_shares, word* s_shares, word* carry_shares);
  void subXorSharedVerify(uint32 n, uint32 ring_no, bool compute_carry, word* b_shares, word* r_shares, word* s_shares, word* carry_shares);
  void subXorModSharedSign(uint32 n, uint32 ring_no, uint32 modulus_ring_no, word* b_shares, word* r_shares, word* s_shares);
  void subXorModSharedVerify(uint32 n, uint32 ring_no, uint32 modulus_ring_no, word* b_shares, word* r_shares, word* s_shares);
  void declassify3SharedSign(uint32 ring_no, word* a_shares, word* b_shares);
  void declassify3SharedVerify(uint32 ring_no, word* a_shares, word* b_shares);
  void bitwiseToAdditiveShared(bool verify, uint32 n, uint32 bw_ring_no, uint32 ad_ring_no, word* bw_shares, word* ad_shares);
  void assertEqBwAdSharedSign(uint32 bw_ring_no, uint32 ad_ring_no, word* bw_shares, word* ad_shares);
  void assertEqBwAdSharedVerify(uint32 bw_ring_no, uint32 ad_ring_no, word* bw_shares, word* ad_shares);
  void performAssertEqBwAds(bool verify);
  void mulNoIntermediateResultSharedSign(word* a_shares, word* b_shares, word* c_shares);
  void squSharedSign(word* a_shares, word* c_shares);
  void mulSharedSign(word* a_shares, word* b_shares, word* c_shares);
  void mulSharedVerify(word* a_shares, word* b_shares, word* c_shares);
  void copyShares(word* from_shares, word* to_shares, uint32 party_size);

  // ECDSA
  void ec_add_shared(bool verify, BigIntLib* ringEC, word* result, word* p1, word* p2);
  void ec_sub_shared(bool verify, BigIntLib* ringEC, word* result, word* p1, word* p2);
  void ec_scmult_fixbase_local(BigIntLib* ringEC, uint32 bits_per_block, word* fixpowers, word* scalar, word* result);
  void ec_scmult_fixbase_shared(BigIntLib* ringEC, bool verify, uint32 bits_per_block, word* fixpowers, word* scalar, word* result);
  void divide_mod_P_shared(BigIntLib* ringP, BigIntLib* ringBW, bool verify, word* x, word* y, word* r);
  void ec_sub_modPshared(BigIntLib* ringP, BigIntLib* ringBW, bool verify, word* x0, word* x1, word* y0, word* y1, word* r0, word* r1);
  void convert_modP_to_EC_shared(BigIntLib* ringBW, bool verify, word* x, word* y, word* p);
  void ecdsaVerification(bool verify, uint32 bits_per_block, word* fixpowers1, word* scalar1, word* fixpowers2, word* scalar2, word* r_x, word* r_y);
  void ecdsaVerificationSharedSign(uint32 bits_per_block, word* fixpowers1, word* scalar1, word* fixpowers2, word* scalar2, word* r_x, word* r_y);
  void ecdsaVerificationSharedVerify(uint32 bits_per_block, word* fixpowers1, word* scalar1, word* fixpowers2, word* scalar2, word* r_x, word* r_y);

  // Additional calculations
  void prepareSharesFieldSign(uchar* x, word* value_shares_f, word* key_shares_f);
  void prepareSharesFieldVerify(word* value_shares_f, word* key_shares_f);
  void outputSharesToBytes(word* output_shares, uint32 party_size);
  void verifyCalcLastShare(uchar* y, word* value_shares_f);

  // Generated functions
#include "generated_header.h"

  // Util
  uint64 rdtsc();
};

#endif // CIRCUITCONTAINER_H
