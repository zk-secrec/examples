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

#include "ZKBPP.h"
#include "BigIntLib.h"
#include "utils.h"

#ifdef USE_OPENSSL
#include <openssl/sha.h> // For SHA-256
#endif

#ifdef USE_FSTREAM
#include <fstream> // used only by serialize(.) and deserialize()
#endif

//#define VERBOSE

ZKBPP::ZKBPP() {
  
}

ZKBPP::~ZKBPP() {
  
}

#define memcmp memcmp2

static int memcmp2(const void* s1, const void* s2, uint32 n) {
  if (n & 7) {
#ifdef TESTING
    std::cerr << "memcmp2 failed" << std::endl;
#endif
    exit(1);
  }
  word* t1 = (word*)s1;
  word* t2 = (word*)s2;
  n >>= 3;
  for (uint32 i = 0; i < n; i++) {
    if (t1[i] != t2[i]) {
      return 1; // should actually be -1 or 1 but for our usage it does not matter
    }
  }
  return 0;
}

void ZKBPP::init(uint32 party_size, uint32 num_iterations, CircuitContainer* cc, bool print_result) {
  this->num_rings_ = cc->num_rings_;
#ifdef TESTING
  std::cout << "this->num_rings_ = " << this->num_rings_ << std::endl;
#endif
  this->party_size_ = party_size;
  this->num_iterations_ = num_iterations;
  this->circuit_ = cc;
  // Get cipher params
  cc->getParams(&(this->circuit_value_size_), &(this->random_tape_size_), &(this->circuit_key_size_), &(this->circuit_gate_size_));

  // Hardcode this->hash_size_ to SHA256_DIGEST_LENGTH (32 bytes)
  //this->hash_size_ = SHA256_DIGEST_LENGTH;
  this->hash_size_ = 32;
  this->view_size_ = 0;
  for (uint32 k = 0; k < this->num_rings_; k++) {
    BigIntLib* big_int_lib = &circuit_->all_big_int_libs_[k];
    this->view_size_ += big_int_lib->view_size();
  }

  this->last_gensign_time_ = 0;
  this->last_sign_time_ = 0;
  this->last_genverify_time_ = 0;
  this->last_verify_time_ = 0;

  // Utils
  this->print_result_ = print_result;

  uint32 total_assert_zero_shares_size = 0;
  uint32 total_declassify3_shares_size = 0;
  for (uint32 k = 0; k < this->num_rings_; k++) {
    BigIntLib* big_int_lib = &circuit_->all_big_int_libs_[k];
    uint32 declassify3_size = big_int_lib->declassify3_size();
    total_declassify3_shares_size += 2 * declassify3_size;
    // only the first 2 shares of each assert_zero argument need to be hashed
    uint32 n = 2 * big_int_lib->num_assert_zeros_ * big_int_lib->value_size_;
    total_assert_zero_shares_size += n;
  }
  this->total_a_size_ = total_declassify3_shares_size + total_assert_zero_shares_size + this->party_size_ * this->hash_size_;
}

Proof* ZKBPP::sign(uchar* x) {
  // std::cout << "ZKBPP::sign, start" << std::endl;
#ifdef USE_CHRONO
  auto gensign_start = std::chrono::high_resolution_clock::now();
#endif

  // Maybe use alignas(8) or alignas(32) for the arrays below
  // Create buffer for views
  uchar** sign_views_buffer = new uchar*[this->num_rings_];
  for (uint32 k = 0; k < this->num_rings_; k++) {
    BigIntLib* big_int_lib = &circuit_->all_big_int_libs_[k];
    sign_views_buffer[k] = new uchar[this->num_iterations_ * this->party_size_ * big_int_lib->view_size()]; // views
    memset(sign_views_buffer[k], 0, this->num_iterations_ * this->party_size_ * big_int_lib->view_size());
  }
  
  // Create buffer for random tapes
  uchar* random_tapes_buffer = new uchar[this->num_iterations_ * this->num_rings_ * this->party_size_ * this->random_tape_size_];
  BigIntLib::FillRandom(random_tapes_buffer, (this->circuit_)->getKey(), this->num_iterations_ * this->num_rings_ * this->party_size_ * this->random_tape_size_);

  // Create data
  ContainerSignData* csd = this->createContainerSignData(sign_views_buffer, random_tapes_buffer);
  ContainerCD* ccd = this->createContainerCD(this->party_size_);
  ContainerA* ca = this->createContainerA();
#ifdef USE_CHRONO
  auto gensign_stop = std::chrono::high_resolution_clock::now();
  this->last_gensign_time_ = std::chrono::duration_cast<std::chrono::nanoseconds>(gensign_stop - gensign_start).count();
  auto sign_start = std::chrono::high_resolution_clock::now();
#endif
  // For each iteration (iterations are independent from each other):
  //#pragma omp parallel for
  uint32 hash_data_size = this->num_rings_ * this->random_tape_size_ + this->view_size_;
  //alignas(32) uchar hash_data[hash_data_size];
#ifdef __clang__
  uchar* hash_data = new uchar[hash_data_size] alignas(32);
#else
  uchar* hash_data = new alignas(32) uchar[hash_data_size];
#endif
  //uchar* hash_data = new uchar[hash_data_size];
  for(uint32 i = 0; i < this->num_iterations_; i++) {
    //std::cout << "ZKBPP::sign, iteration " << i << std::endl << std::flush;
    // Call the circuit, pass SignData and let the circuit fill all Views (and the k's)
    (this->circuit_->runSign)(x, csd->sds_[i]);
    // Add to CD and A
    this->fillCDSign(ccd, i, csd->sds_[i], hash_data);
    this->fillASign(ca, i, csd->sds_[i], ccd);
  }
  delete[] hash_data;
  //auto sign_stop = std::chrono::high_resolution_clock::now();

  // Commitment
  this->commitment_ = ccd;
  // Allocate memory for the proof p, containing one b_i and one z_i for each iteration
  Proof* p = this->createProof();
  // Create the callenge E, being the hash value of the concatenation of all a_i's
  this->buildChallengeHash(p->e_, ca);
  this->extendChallengeHash(p);
  // For each iteration, the hash value E should create an e_i in {0, 1, 2}
  // For each iteration (iterations are independent from each other):
  for(uint32 i = 0; i < this->num_iterations_; i++) {
    // 1. Create b_i = (y_e+2, C_e+2), where y and C are this iteration's values and e is in {0, 1, 2} according to the challenge (hash value) E
    // 2. Create z_i for this iteration and use this iteration's View, k values and (if e = 2) x_3
    // Remarks: SignData* from this iteration still contains everything needed for z_i (all views, all random tapes and x_3) so according to e_i, move correct pointers into new z_i and delete others from SignData's memory (e.g. two unneeded views for each iteration)
    this->fillProof(p, i, csd->sds_[i], ccd);
  }

  // Delete unneeded resources
  this->destroyContainerSignData(csd);
  this->destroyContainerA(ca);
  delete[] random_tapes_buffer;

  for (uint32 k = 0; k < this->num_rings_; k++) {
    delete[] sign_views_buffer[k];
  }
  delete[] sign_views_buffer;

#ifdef USE_CHRONO
  auto sign_stop = std::chrono::high_resolution_clock::now();
  this->last_sign_time_ = std::chrono::duration_cast<std::chrono::nanoseconds>(sign_stop - sign_start).count();
#endif
  this->destroyContainerCD(this->commitment_, this->party_size_);
#ifdef USE_CHRONO
  std::cout << "Proving time " << (this->last_sign_time_ / 1000000) << " ms" << std::endl << std::flush;
#endif

  return p;
}

bool ZKBPP::verify(Proof* p, uchar* x, uchar* y) {
  //this->printProof(p, false);
#ifdef USE_CHRONO
  auto genverify_start = std::chrono::high_resolution_clock::now();
#endif

  // Create buffer for views
  uchar** verify_views_buffer = new uchar*[this->num_rings_];
  for (uint32 k = 0; k < this->num_rings_; k++) {
    BigIntLib* big_int_lib = &circuit_->all_big_int_libs_[k];
    verify_views_buffer[k] = new uchar[this->num_iterations_ * big_int_lib->view_size()]; // views
    memset(verify_views_buffer[k], 0, this->num_iterations_ * big_int_lib->view_size());
  }

  bool ret_val;
  //uint32 k_View_size = this->num_rings_ * this->random_tape_size_ + (this->circuit_num_view_gates_ * this->circuit_gate_size_);
  ContainerVerifyData* cvd = this->createContainerVerifyData(verify_views_buffer);
  ContainerCD* ccd_verify = this->createContainerCD(this->party_size_ - 1);
  ContainerA* ca_verify = this->createContainerA();
#ifdef USE_CHRONO
  auto genverify_stop = std::chrono::high_resolution_clock::now();
  this->last_genverify_time_ = std::chrono::duration_cast<std::chrono::nanoseconds>(genverify_stop - genverify_start).count();
  auto verify_start = std::chrono::high_resolution_clock::now();
#endif
  // For each iteration (iterations are independent from each other):
  uint32 hash_data_size = this->num_rings_ * this->random_tape_size_ + this->view_size_;
  //alignas(32) uchar hash_data[hash_data_size];
#ifdef __clang__
  uchar* hash_data = new uchar[hash_data_size] alignas(32);
#else
  uchar* hash_data = new alignas(32) uchar[hash_data_size];
#endif
  //#pragma omp parallel for
  for(uint32 i = 0; i < this->num_iterations_; i++) {
    // Call the circuit, pass VerifyData and let the circuit fill the chosen View
    (this->circuit_->runVerify)(p, x, y, cvd->vds_[i], i);
    // Add to CD and A
    this->fillCDVerify(ccd_verify, i, p, cvd->vds_[i], hash_data);
    this->fillAVerify(ca_verify, i, p, cvd->vds_[i], ccd_verify);
  }
  //delete[] hash_data;

  // Create the callenge E, compare with challenge contained in proof
  uchar* challenge_prime = new uchar[this->hash_size_];
  this->buildChallengeHash(challenge_prime, ca_verify);
#ifdef TESTING
  if(this->print_result_ == true) {
    std::cout << "[ZKBPP] challenge: " << std::endl;
    this->printDataAsHex(p->e_, this->hash_size_, true);
    std::cout << "[ZKBPP] challenge': " << std::endl;
    this->printDataAsHex(challenge_prime, this->hash_size_, true);
  }
#endif
  uint32 result = memcmp(p->e_, challenge_prime, this->hash_size_);
  if(result == 0)
    ret_val = true;
  else
    ret_val = false;

  // Clean up
  this->destroyContainerVerifyData(cvd);
  this->destroyContainerCD(ccd_verify, this->party_size_ - 1);
  this->destroyContainerA(ca_verify);
  delete[] challenge_prime;
  delete[] hash_data;

  for (uint32 k = 0; k < this->num_rings_; k++) {
    delete[] verify_views_buffer[k];
  }
  delete[] verify_views_buffer;

  this->destroyProof(p);

#ifdef USE_CHRONO
  auto verify_stop = std::chrono::high_resolution_clock::now();
  this->last_verify_time_ = std::chrono::duration_cast<std::chrono::nanoseconds>(verify_stop - verify_start).count();
  std::cout << "Verification time " << (this->last_verify_time_ / 1000000) << " ms" << std::endl << std::flush;
#endif

  return ret_val;
}

uint64 ZKBPP::getLastGenSignNS() {
  return this->last_gensign_time_;
}

uint32 ZKBPP::getLastSignNS() {
  return this->last_sign_time_;
}

uint64 ZKBPP::getLastGenVerifyNS() {
  return this->last_genverify_time_;
}

uint32 ZKBPP::getLastVerifyNS() {
  return this->last_verify_time_;
}

SignData* ZKBPP::createSignData(uchar** sign_views_buffer, void* random_tapes_buffer, uint32 iteration) {
  SignData* sign_data = new SignData;
  sign_data->x_3_ = new uchar[this->circuit_value_size_];

  sign_data->witness_3_ = new uchar*[this->num_rings_];
  sign_data->declassify3_1_ = new uchar*[this->num_rings_];
  sign_data->declassify3_2_ = new uchar*[this->num_rings_];
  sign_data->assert_zero_shares_ = new uchar*[this->num_rings_];
  sign_data->views_ = new uchar**[this->num_rings_];
  for (uint32 k = 0; k < this->num_rings_; k++) {
    BigIntLib* big_int_lib = &circuit_->all_big_int_libs_[k];
    sign_data->witness_3_[k] = new uchar[big_int_lib->witness_3_size()];

    uint32 declassify3_size = big_int_lib->declassify3_size();
    sign_data->declassify3_1_[k] = new uchar[declassify3_size];
    sign_data->declassify3_2_[k] = new uchar[declassify3_size];

    // only the first 2 shares of each assert_zero argument need to be hashed
    uint32 n = 2 * big_int_lib->num_assert_zeros_ * big_int_lib->value_size_;
    sign_data->assert_zero_shares_[k] = new uchar[n];

    uint32 view_size = big_int_lib->view_size();
    sign_data->views_[k] = new uchar*[this->party_size_];
    uchar* pointer_1 = sign_views_buffer[k] + iteration * 3 * view_size;
    sign_data->views_[k][0] = pointer_1;
    sign_data->views_[k][1] = pointer_1 + view_size;
    sign_data->views_[k][2] = pointer_1 + 2 * view_size;
  }

  sign_data->y_shares_ = new uchar[this->party_size_ * this->circuit_value_size_];
  sign_data->random_tapes_ = (uchar*)random_tapes_buffer + iteration * this->num_rings_ * this->party_size_ * this->random_tape_size_;
  sign_data->random_tapes_hashs_ = new uchar[(this->party_size_ - 1) * this->hash_size_];
  SHA256Dash(sign_data->random_tapes_hashs_, sign_data->random_tapes_, this->random_tape_size_);
  SHA256Dash(sign_data->random_tapes_hashs_ + this->hash_size_, sign_data->random_tapes_ + this->num_rings_ * this->random_tape_size_, this->random_tape_size_);
  
  sign_data->y_ = new uchar[this->circuit_value_size_];
  return sign_data;
}

void ZKBPP::destroySignData(SignData* sign_data) {
  delete[] sign_data->x_3_;

  for (uint32 k = 0; k < this->num_rings_; k++) {
    delete[] sign_data->witness_3_[k];
    delete[] sign_data->declassify3_1_[k];
    delete[] sign_data->declassify3_2_[k];
    delete[] sign_data->assert_zero_shares_[k];
    delete[] sign_data->views_[k];
  }
  delete[] sign_data->witness_3_;
  delete[] sign_data->declassify3_1_;
  delete[] sign_data->declassify3_2_;
  delete[] sign_data->assert_zero_shares_;
  delete[] sign_data->views_;

  delete[] sign_data->y_shares_;
  //delete[] sign_data->random_tapes_;
  delete[] sign_data->random_tapes_hashs_;
  delete[] sign_data->y_;
  delete sign_data;
}

VerifyData* ZKBPP::createVerifyData(uchar** verify_views_buffer, uint32 iteration) {
  VerifyData* verify_data = new VerifyData;
  //verify_data->view_ = new uchar[this->view_size_];
  //memset(verify_data->view_, 0, this->view_size_);
  verify_data->y_share_ = new uchar[this->circuit_value_size_];
  verify_data->y_e2_ = new uchar[this->circuit_value_size_];

  verify_data->declassify3_1_ = new uchar*[this->num_rings_];
  verify_data->declassify3_2_ = new uchar*[this->num_rings_];
  verify_data->assert_zero_shares_ = new uchar*[this->num_rings_];
  verify_data->view_ = new uchar*[this->num_rings_];
  for (uint32 k = 0; k < this->num_rings_; k++) {
    BigIntLib* big_int_lib = &circuit_->all_big_int_libs_[k];
    uint32 declassify3_size = big_int_lib->declassify3_size();
    verify_data->declassify3_1_[k] = new uchar[declassify3_size];
    verify_data->declassify3_2_[k] = new uchar[declassify3_size];

    verify_data->assert_zero_shares_[k] = new uchar[this->party_size_ * big_int_lib->num_assert_zeros_ * big_int_lib->value_size_];

    uint32 view_size = big_int_lib->view_size();
    verify_data->view_[k] = verify_views_buffer[k] + iteration * view_size;
  }

  return verify_data;
}

void ZKBPP::destroyVerifyData(VerifyData* verify_data) {
  delete[] verify_data->view_;
  delete[] verify_data->y_share_;
  delete[] verify_data->y_e2_;

  for (uint32 k = 0; k < this->num_rings_; k++) {
    delete[] verify_data->declassify3_1_[k];
    delete[] verify_data->declassify3_2_[k];
    delete[] verify_data->assert_zero_shares_[k];
  }
  delete[] verify_data->declassify3_1_;
  delete[] verify_data->declassify3_2_;
  delete[] verify_data->assert_zero_shares_;

  delete verify_data;
}

ContainerSignData* ZKBPP::createContainerSignData(uchar** sign_views_buffer, void* random_tapes_buffer) {
  ContainerSignData* csd = new ContainerSignData;
  csd->sds_ = new SignData*[this->num_iterations_];
  for(uint32 i = 0; i < this->num_iterations_; i++) {
    csd->sds_[i] = this->createSignData(sign_views_buffer, random_tapes_buffer, i);
  }
  return csd;
}

void ZKBPP::destroyContainerSignData(ContainerSignData* csd) {
  for(uint32 i = 0; i < this->num_iterations_; i++) {
    destroySignData(csd->sds_[i]);
  }
  delete[] csd->sds_;
  delete csd;
}

ContainerVerifyData* ZKBPP::createContainerVerifyData(uchar** verify_views_buffer) {
  ContainerVerifyData* cvd = new ContainerVerifyData;
  cvd->vds_ = new VerifyData*[this->num_iterations_];
  for(uint32 i = 0; i < this->num_iterations_; i++) {
    cvd->vds_[i] = this->createVerifyData(verify_views_buffer, i);
  }
  return cvd;
}

void ZKBPP::destroyContainerVerifyData(ContainerVerifyData* cvd) {
  for(uint32 i = 0; i < this->num_iterations_; i++) {
    destroyVerifyData(cvd->vds_[i]);
  }
  delete[] cvd->vds_;
  delete cvd;
}

C* ZKBPP::createC() {
  C* c = new C;
  c->H_k_View_ = new uchar[this->hash_size_];
  return c;
}

void ZKBPP::destroyC(C* c) {
  delete[] c->H_k_View_;
  delete c;
}

ContainerCD* ZKBPP::createContainerCD(uint32 party_size) {
  ContainerCD* ccd = new ContainerCD;
  ccd->Cs_ = new C**[party_size];
  for(uint32 i = 0; i < party_size; i++) {
    ccd->Cs_[i] = new C*[this->num_iterations_];
    for(uint32 j = 0; j < this->num_iterations_; j++) {
      ccd->Cs_[i][j] = this->createC();
    }
  }
  return ccd;
}

void ZKBPP::destroyContainerCD(ContainerCD* ccd, uint32 party_size) {
  // (party_size * iterations) entries
  for(uint32 i = 0; i < party_size; i++) {
    for(uint32 j = 0; j < this->num_iterations_; j++) {
      this->destroyC(ccd->Cs_[i][j]);
    }
    delete[] ccd->Cs_[i];
  }
  delete[] ccd->Cs_;
  delete ccd;
}

A* ZKBPP::createA() {
  A* a = new A;
  a->ys_C_hashs_ = new uchar[this->total_a_size_];
  return a;
}

void ZKBPP::destroyA(A* a) {
  delete[] a->ys_C_hashs_;
  delete a;
}

ContainerA* ZKBPP::createContainerA() {
  ContainerA* ca = new ContainerA;
  ca->as_ = new A*[this->num_iterations_];
  for(uint32 i = 0; i < this->num_iterations_; i++) {
    ca->as_[i] = this->createA();
  }
  return ca;
}

void ZKBPP::destroyContainerA(ContainerA* ca) {
  for(uint32 i = 0; i < this->num_iterations_; i++) {
    this->destroyA(ca->as_[i]);
  }
  delete[] ca->as_;
  delete ca;
}

Z* ZKBPP::createZ(bool create_x_3) {
  Z* z = new Z;

  z->view_ = new uchar*[this->num_rings_];
  for (uint32 k = 0; k < this->num_rings_; k++) {
    BigIntLib* big_int_lib = &circuit_->all_big_int_libs_[k];
    z->view_[k] = new uchar[big_int_lib->view_size()];
  }

  z->k_1_ = new uchar[this->num_rings_ * this->random_tape_size_];
  z->k_2_ = new uchar[this->num_rings_ * this->random_tape_size_];
  z->k_1_hash_ = NULL;
  z->k_2_hash_ = NULL;
  if(create_x_3 == true) {
    z->x_3_ = new uchar[this->circuit_value_size_];
    z->witness_3_ = new uchar*[this->num_rings_];
    z->declassify3_ = new uchar*[this->num_rings_];
    for (uint32 k = 0; k < this->num_rings_; k++) {
      z->witness_3_[k] = new uchar[circuit_->all_big_int_libs_[k].witness_3_size()];
      z->declassify3_[k] = new uchar[circuit_->all_big_int_libs_[k].declassify3_size()];
    }
  } else {
    z->x_3_ = NULL;
    z->witness_3_ = NULL;
  }
  z->y_share_ = new uchar[this->circuit_value_size_];
  return z;
}

void ZKBPP::destroyZ(Z* z) {
  for (uint32 k = 0; k < this->num_rings_; k++) {
    delete[] z->view_[k];
  }
  delete[] z->view_;

  delete[] z->k_1_;
  delete[] z->k_2_;
  if(z->k_1_hash_ != NULL) delete[] z->k_1_hash_;
  if(z->k_2_hash_ != NULL) delete[] z->k_2_hash_;
  if(z->x_3_ != NULL)
    delete[] z->x_3_;
  if(z->witness_3_ != NULL) {
    for (uint32 k = 0; k < this->num_rings_; k++) {
      delete[] z->witness_3_[k];
      delete[] z->declassify3_[k];
    }
    delete[] z->witness_3_;
    delete[] z->declassify3_;
  }
  delete[] z->y_share_;
  delete z;
}

Proof* ZKBPP::createProof() {
  Proof* p = new Proof;
  p->num_iterations_ = this->num_iterations_;
  p->e_ = new uchar[this->hash_size_];
  p->e_i_ = new uchar[this->num_iterations_];
  p->y_e2_ = new uchar*[this->num_iterations_];
  p->H_k_View_ = new uchar*[this->num_iterations_];
  p->zs_ = new Z*[this->num_iterations_];
  for(uint32 i = 0; i < this->num_iterations_; i++) {
    p->y_e2_[i] = new uchar[this->circuit_value_size_];
    p->H_k_View_[i] = new uchar[this->hash_size_];
    p->zs_[i] = NULL; // Create dynamically later, because x_3 is not always needed!
  }
  return p;
}

void ZKBPP::destroyProof(Proof* p) {
  delete[] p->e_;
  delete[] p->e_i_;
  for(uint32 i = 0; i < this->num_iterations_; i++) {
    //this->destroyB(p->bs_[i]);
    delete[] p->y_e2_[i];
    delete[] p->H_k_View_[i];
    this->destroyZ(p->zs_[i]);
  }
  delete[] p->y_e2_;
  delete[] p->H_k_View_;
  delete[] p->zs_;
  delete p;
}

void ZKBPP::fillCDSign(ContainerCD* ccd, uint32 iteration, SignData* sign_data, uchar* hash_data) {
  // All <party_size> needed views and tapes are in sign_data
  uint32 hash_data_size = this->num_rings_ * this->random_tape_size_ + this->view_size_;
  //uchar* hash_data = new uchar[hash_data_size];
  //uchar hash_data[hash_data_size] __attribute__ ((aligned (32)));
  for(uint32 i = 0; i < this->party_size_; i++) {
    memcpy(hash_data, sign_data->random_tapes_ + (i * this->num_rings_ * this->random_tape_size_), this->num_rings_ * this->random_tape_size_);
    uint32 offset = this->num_rings_ * this->random_tape_size_;
    for (uint32 k = 0; k < this->num_rings_; k++) {
      BigIntLib* big_int_lib = &circuit_->all_big_int_libs_[k];
      memcpy(hash_data + offset, sign_data->views_[k][i], big_int_lib->view_size());
      offset += big_int_lib->view_size();
    }
    //memcpy(hash_data + this->random_tape_size_, sign_data->views_ + (i * this->view_size_), this->view_size_);
    // Build hash and store in iteration's C
    this->SHA256Prime((ccd->Cs_[i][iteration])->H_k_View_, hash_data, hash_data_size);
    // Store concatenation in iteration's D
    //memcpy((ccd->Ds_[i][iteration])->k_View_, hash_data, hash_data_size);
  }

  //delete[] hash_data;
}

void ZKBPP::fillCDVerify(ContainerCD* ccd, uint32 iteration, Proof* p, VerifyData* verify_data, uchar* hash_data) {
  // All <party_size - 1> needed views and tapes are in p and verify_data
  uchar* r_tapes_temp[this->party_size_ - 1];
  r_tapes_temp[0] = (p->zs_[iteration])->k_1_;
  r_tapes_temp[1] = (p->zs_[iteration])->k_2_;
  uint32 hash_data_size = this->num_rings_ * this->random_tape_size_ + this->view_size_;
  //uchar* hash_data = new uchar[hash_data_size];
  //uchar hash_data[hash_data_size];
  for(uint32 i = 0; i < (this->party_size_ - 1); i++) {
    memcpy(hash_data, r_tapes_temp[i], this->num_rings_ * this->random_tape_size_);
    uint32 offset = this->num_rings_ * this->random_tape_size_;
    for (uint32 k = 0; k < this->num_rings_; k++) {
      uchar* views_temp[this->party_size_ - 1];
      views_temp[0] = verify_data->view_[k]; // Calculated by Circuit->evaluateVerify(.)
      views_temp[1] = (p->zs_[iteration])->view_[k];
      BigIntLib* big_int_lib = &circuit_->all_big_int_libs_[k];
      memcpy(hash_data + offset, views_temp[i], big_int_lib->view_size());
      offset += big_int_lib->view_size();
    }
    // Build hash and store in iteration's C
    this->SHA256Prime((ccd->Cs_[i][iteration])->H_k_View_, hash_data, hash_data_size);
    // Store concatenation in iteration's D
    //memcpy((ccd->Ds_[i][iteration])->k_View_, hash_data, hash_data_size);
  }

  //delete[] hash_data;
}

// fills a_i = [(declassify3_1_, declassify3_2_, assert_zero_shares_1, assert_zero_shares_2)*num_rings_, C_1, C_2, C_3]_i
void ZKBPP::fillASign(ContainerA* ca, uint32 iteration, SignData* sign_data, ContainerCD* ccd) {
  uint32 offset = 0;
  for (uint32 k = 0; k < this->num_rings_; k++) {
    BigIntLib* big_int_lib = &circuit_->all_big_int_libs_[k];
    uint32 m = big_int_lib->declassify3_size();
    memcpy(ca->as_[iteration]->ys_C_hashs_ + offset, sign_data->declassify3_1_[k], m);
    offset += m;
    memcpy(ca->as_[iteration]->ys_C_hashs_ + offset, sign_data->declassify3_2_[k], m);
    offset += m;

    // only the first 2 shares of each assert_zero argument need to be hashed
    uint32 n = 2 * big_int_lib->num_assert_zeros_ * big_int_lib->value_size_;
    memcpy(ca->as_[iteration]->ys_C_hashs_ + offset, sign_data->assert_zero_shares_[k], n);
    offset += n;
  }

  for(uint32 i = 0; i < this->party_size_; i++) {
    memcpy(ca->as_[iteration]->ys_C_hashs_ + offset, (ccd->Cs_[i][iteration])->H_k_View_, this->hash_size_); // C value
    offset += this->hash_size_;
  }
}

// fills a_i = [(declassify3_1_, declassify3_2_, assert_zero_shares_1, assert_zero_shares_2)*num_rings_, C_1, C_2, C_3]_i
void ZKBPP::fillAVerify(ContainerA* ca, uint32 iteration, Proof* p, VerifyData* verify_data, ContainerCD* ccd) {
  uint32 e = p->e_i_[iteration];
  uint32 offset = 0;

  for (uint32 k = 0; k < this->num_rings_; k++) {
    BigIntLib* big_int_lib = &circuit_->all_big_int_libs_[k];
    uint32 m = big_int_lib->declassify3_size();
    memcpy(ca->as_[iteration]->ys_C_hashs_ + offset, verify_data->declassify3_1_[k], m);
    offset += m;
    memcpy(ca->as_[iteration]->ys_C_hashs_ + offset, verify_data->declassify3_2_[k], m);
    offset += m;

    uint32 size_per_party = big_int_lib->num_assert_zeros_ * big_int_lib->value_size_;
    uchar* assert_zero_party_shares[this->party_size_];
    assert_zero_party_shares[e] = verify_data->assert_zero_shares_[k];
    assert_zero_party_shares[(e + 1) % this->party_size_] = verify_data->assert_zero_shares_[k] + size_per_party;
    assert_zero_party_shares[(e + 2) % this->party_size_] = verify_data->assert_zero_shares_[k] + 2 * size_per_party;
    // only the first 2 shares of each assert_zero argument need to be hashed
    for(uint32 i = 0; i < 2; i++) {
      memcpy(ca->as_[iteration]->ys_C_hashs_ + offset, assert_zero_party_shares[i], size_per_party); // assert_zero argument shares for this party
      offset += size_per_party;
    }
  }

  uchar* C_hashs_temp[this->party_size_];
  C_hashs_temp[e] = ccd->Cs_[0][iteration]->H_k_View_; // Calculated C hash value from calculated View
  C_hashs_temp[(e + 1) % this->party_size_] = ccd->Cs_[1][iteration]->H_k_View_; // Calculated C hash value from View contained in Proof
  C_hashs_temp[(e + 2) % this->party_size_] = p->H_k_View_[iteration];
  for(uint32 i = 0; i < this->party_size_; i++) {
    memcpy(ca->as_[iteration]->ys_C_hashs_ + offset, C_hashs_temp[i], this->hash_size_); // C value
    offset += this->hash_size_;
  }
}

void ZKBPP::fillProof(Proof* p, uint32 iteration, SignData* sign_data, ContainerCD* ccd) {
  uint32 e = p->e_i_[iteration];

  // Add b_i
  memcpy(p->y_e2_[iteration], sign_data->y_shares_ + (((e + 2) % this->party_size_) * this->circuit_value_size_), this->circuit_value_size_); // y_e+2
  memcpy(p->H_k_View_[iteration], ccd->Cs_[(e + 2) % this->party_size_][iteration]->H_k_View_, this->hash_size_); // C_e+2

  // Create and add z_i
  if(e == 0) {
    p->zs_[iteration] = this->createZ(false);
  } else {
    p->zs_[iteration] = this->createZ(true);
    memcpy((p->zs_[iteration])->x_3_, sign_data->x_3_, this->circuit_value_size_); // x_3
    for (uint32 k = 0; k < this->num_rings_; k++) {
      memcpy((p->zs_[iteration])->witness_3_[k], sign_data->witness_3_[k], circuit_->all_big_int_libs_[k].witness_3_size()); // witness_3
      if (e == 1)
        memcpy((p->zs_[iteration])->declassify3_[k], sign_data->declassify3_1_[k], circuit_->all_big_int_libs_[k].declassify3_size());
      else // e == 2
        memcpy((p->zs_[iteration])->declassify3_[k], sign_data->declassify3_2_[k], circuit_->all_big_int_libs_[k].declassify3_size());
    }
  }
  
  // REMARK: If e in {1, 2}, only one hash value is actually needed! This can be further optimized.
  memcpy((p->zs_[iteration])->k_1_, sign_data->random_tapes_ + (e * this->num_rings_ * this->random_tape_size_), this->num_rings_ * this->random_tape_size_); // k_e
  memcpy((p->zs_[iteration])->k_2_, sign_data->random_tapes_ + (((e + 1) % this->party_size_) * this->num_rings_ * this->random_tape_size_), this->num_rings_ * this->random_tape_size_); // k_e+1

  // Needed hash values
  if(e == 0) {
    (p->zs_[iteration])->k_1_hash_ = new uchar[this->hash_size_];
    (p->zs_[iteration])->k_2_hash_ = new uchar[this->hash_size_];
    memcpy((p->zs_[iteration])->k_1_hash_, sign_data->random_tapes_hashs_, this->hash_size_);
    memcpy((p->zs_[iteration])->k_2_hash_, sign_data->random_tapes_hashs_ + this->hash_size_, this->hash_size_);
  }
  else if(e == 1) {
    (p->zs_[iteration])->k_2_hash_ = new uchar[this->hash_size_];
    memcpy((p->zs_[iteration])->k_2_hash_, sign_data->random_tapes_hashs_ + this->hash_size_, this->hash_size_);
  }
  else { // e == 2
    (p->zs_[iteration])->k_1_hash_ = new uchar[this->hash_size_];
    memcpy((p->zs_[iteration])->k_1_hash_, sign_data->random_tapes_hashs_, this->hash_size_);
  }

  // y share
  memcpy((p->zs_[iteration])->y_share_, sign_data->y_shares_ + (((e + 1) % this->party_size_) * this->circuit_value_size_), this->circuit_value_size_);

  for (uint32 k = 0; k < this->num_rings_; k++) {
    BigIntLib* big_int_lib = &circuit_->all_big_int_libs_[k];
    memcpy((p->zs_[iteration])->view_[k], sign_data->views_[k][(e + 1) % this->party_size_], big_int_lib->view_size());
  }
  //sign_data->views_[(e + 1) % this->party_size_] = NULL; // Set to null, so it's not destroyed by destroySignData, but later with destroyProof
}

void ZKBPP::serialize(Proof* p) {
#ifdef USE_FSTREAM
  using namespace std;
  ofstream fout("proof.bin", ios::binary | ios::out);

  fout.write((char*)p->e_, this->hash_size_);
  for (uint32 i = 0; i < this->num_iterations_; i++) {
    uint32 e = p->e_i_[i];
    fout.write((char*)p->y_e2_[i], this->circuit_value_size_);
    fout.write((char*)p->H_k_View_[i], this->hash_size_);
    if (e != 0) {
      fout.write((char*)p->zs_[i]->x_3_, this->circuit_value_size_);
      for (uint32 k = 0; k < this->num_rings_; k++) {
        fout.write((char*)p->zs_[i]->witness_3_[k], circuit_->all_big_int_libs_[k].witness_3_size());
        fout.write((char*)p->zs_[i]->declassify3_[k], circuit_->all_big_int_libs_[k].declassify3_size());
      }
    }
    fout.write((char*)p->zs_[i]->k_1_, this->num_rings_ * this->random_tape_size_);
    fout.write((char*)p->zs_[i]->k_2_, this->num_rings_ * this->random_tape_size_);

    if(e == 0) {
      fout.write((char*)p->zs_[i]->k_1_hash_, this->hash_size_);
      fout.write((char*)p->zs_[i]->k_2_hash_, this->hash_size_);
    } else if(e == 1) {
      fout.write((char*)p->zs_[i]->k_2_hash_, this->hash_size_);
    } else { // e == 2
      fout.write((char*)p->zs_[i]->k_1_hash_, this->hash_size_);
    }

    fout.write((char*)p->zs_[i]->y_share_, this->circuit_value_size_);
    for (uint32 k = 0; k < this->num_rings_; k++) {
      BigIntLib* big_int_lib = &circuit_->all_big_int_libs_[k];
      fout.write((char*)p->zs_[i]->view_[k], big_int_lib->view_size());
    }
  }

  fout.close();
  if (fout.good()) {
#ifdef TESTING
    std::cout << "The proof was written to the file proof.bin" << std::endl;
#endif
  } else {
#ifdef TESTING
    std::cerr << "Failed to write the proof to proof.bin" << std::endl;
#endif
    exit(1);
  }
#endif

  this->destroyProof(p);
}

Proof* ZKBPP::deserialize() {
  Proof* p = this->createProof();

#ifdef USE_FSTREAM
  using namespace std;
  ifstream fin("proof.bin", ios::binary | ios::in);
  if (!fin.good()) {
#ifdef TESTING
    std::cerr << "Failed to open file proof.bin" << std::endl;
#endif
    exit(1);
  }

  fin.read((char*)p->e_, this->hash_size_);
  this->extendChallengeHash(p);

  for (uint32 i = 0; i < this->num_iterations_; i++) {
    uint32 e = p->e_i_[i];
    fin.read((char*)p->y_e2_[i], this->circuit_value_size_);
    fin.read((char*)p->H_k_View_[i], this->hash_size_);
    if(e == 0) {
      p->zs_[i] = this->createZ(false);
    } else {
      p->zs_[i] = this->createZ(true);
      fin.read((char*)p->zs_[i]->x_3_, this->circuit_value_size_);
      for (uint32 k = 0; k < this->num_rings_; k++) {
        fin.read((char*)p->zs_[i]->witness_3_[k], circuit_->all_big_int_libs_[k].witness_3_size());
        fin.read((char*)p->zs_[i]->declassify3_[k], circuit_->all_big_int_libs_[k].declassify3_size());
      }
    }
    fin.read((char*)p->zs_[i]->k_1_, this->num_rings_ * this->random_tape_size_);
    fin.read((char*)p->zs_[i]->k_2_, this->num_rings_ * this->random_tape_size_);

    if(e == 0) {
      (p->zs_[i])->k_1_hash_ = new uchar[this->hash_size_];
      (p->zs_[i])->k_2_hash_ = new uchar[this->hash_size_];
      fin.read((char*)p->zs_[i]->k_1_hash_, this->hash_size_);
      fin.read((char*)p->zs_[i]->k_2_hash_, this->hash_size_);
    } else if(e == 1) {
      (p->zs_[i])->k_2_hash_ = new uchar[this->hash_size_];
      fin.read((char*)p->zs_[i]->k_2_hash_, this->hash_size_);
    } else { // e == 2
      (p->zs_[i])->k_1_hash_ = new uchar[this->hash_size_];
      fin.read((char*)p->zs_[i]->k_1_hash_, this->hash_size_);
    }

    fin.read((char*)p->zs_[i]->y_share_, this->circuit_value_size_);
    for (uint32 k = 0; k < this->num_rings_; k++) {
      BigIntLib* big_int_lib = &circuit_->all_big_int_libs_[k];
      fin.read((char*)p->zs_[i]->view_[k], big_int_lib->view_size());
    }
  }

  if (!fin.good()) {
#ifdef TESTING
    std::cerr << "Error reading file proof.bin, maybe it does not contain the correct proof" << std::endl;
#endif
    exit(1);
  }

  fin.close();
#endif

  return p;
}

void ZKBPP::buildChallengeHash(uchar* destination, ContainerA* ca) {
  uint32 a_size = this->total_a_size_;
  uint32 hash_data_size = this->num_iterations_ * a_size; // This is a lot...
  uchar* hash_data = new uchar[hash_data_size];
  uint32 offset = 0;
  for(uint32 i = 0; i < this->num_iterations_; i++) {
    memcpy(hash_data + offset, ca->as_[i]->ys_C_hashs_, a_size);
    offset += a_size;
  }
  // Build challenge hash and add to proof
  SHA256(destination, hash_data, hash_data_size);
  delete[] hash_data;
}

void ZKBPP::SHA256(uchar* destination, uchar* data, uint32 data_size) {
#ifdef USE_OPENSSL
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, data, data_size);
  SHA256_Final(destination, &sha256);
#endif
}

void ZKBPP::SHA256Prime(uchar* destination, uchar* data, uint32 data_size) { // TODO
  data[data_size - 1] += 1;
  this->SHA256(destination, data, data_size);
  data[data_size - 1] -= 1;
}

void ZKBPP::SHA256Dash(uchar* destination, uchar* data, uint32 data_size) { // TODO
  data[data_size - 1] += 2;
  this->SHA256(destination, data, data_size);
  data[data_size - 1] -= 2;
}

void ZKBPP::SHA256ExtendChallenge(uchar* destination, uchar* data, uint32 data_size) { // TODO
  data[data_size - 1] += 3;
  this->SHA256(destination, data, data_size);
  data[data_size - 1] -= 3;
}

// Compute p->e_i_ from p->e_
void ZKBPP::extendChallengeHash(Proof* p) {
  if (this->num_iterations_ > 320) {
    // Because 3^40 < 2^64 < 3^41, we need 64 bits of randomness for each 40 iterations.
    // We only generate 512 bits of randomness here, which is enough for up to 320 iterations.
#ifdef TESTING
    std::cerr << "extendChallengeHash: support for more than 320 iterations is currently not implemented" << std::endl << std::flush;
#endif
    exit(1);
  }
  if (sizeof(uint64) != 8) {
#ifdef TESTING
    std::cerr << "extendChallengeHash: sizeof(uint64) != 8" << std::endl << std::flush;
#endif
    exit(1);
  }
  if (this->hash_size_ != 32) {
#ifdef TESTING
    std::cerr << "extendChallengeHash: hash_size_ != 32" << std::endl << std::flush;
#endif
    exit(1);
  }
  uint64 t[8];
  memcpy(t, p->e_, 32);
  SHA256ExtendChallenge((uchar *)(t + 4), p->e_, 32);
  //std::cout << "extendChallengeHash: ";
  for (uint32 i = 0; i < this->num_iterations_; i++) {
    uint32 i1 = i%8;
    p->e_i_[i] = t[i1] % 3;
    t[i1] /= 3;
    //std::cout << (uint32)p->e_i_[i];
  }
  //std::cout << std::endl;
}

void ZKBPP::printProof(Proof* p, bool print_view) {
#ifdef TESTING
  bool format = true;
  std::cout << "--- Proof ---" << std::endl;
  std::cout << "Challenge:" << std::endl;
  this->printDataAsHex(p->e_, this->hash_size_, format);
  for(uint32 i = 0; i < p->num_iterations_; i++) {
    std::cout << "--- Iteration " << i << " ---" << std::endl;
    std::cout << "(b) y_e+2:" << std::endl;
    this->printDataAsHex(p->y_e2_[i], this->circuit_value_size_, format);
    std::cout << "(b) H'(k, View):" << std::endl;
    this->printDataAsHex(p->H_k_View_[i], this->hash_size_, format);
    std::cout << "(z) k_e:" << std::endl;
    this->printDataAsHex(p->zs_[i]->k_1_, this->num_rings_ * this->random_tape_size_, format);
    std::cout << "(z) k_e+1:" << std::endl;
    this->printDataAsHex(p->zs_[i]->k_2_, this->num_rings_ * this->random_tape_size_, format);
    std::cout << "(z) x_3:" << std::endl;
    this->printDataAsHex(p->zs_[i]->x_3_, this->circuit_value_size_, format);
    if(print_view == true) {
      std::cout << "(z) View_e+1" << std::endl;
    }
  }
  std::cout << "-------------" << std::endl;
#endif
}

void ZKBPP::printDataAsHex(uchar* data, uint32 data_size, bool format) {
#ifdef TESTING
  for(uint32 i = 0; i < data_size; i++) {
    std::cout << std::setfill('0') << std::setw(2) << std::hex << (uint32)data[i];
    if(format == true)
      std::cout << (((i + 1) % 32 == 0) ? "\n" : " ");
  }
  std::cout << std::dec << std::endl;
#endif
}
