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

// Shared implementation of circuits.
#include "CircuitContainer.h"
#include "utils.h"
//#include <vector>

#ifdef USE_OPENSSL
#include <openssl/rand.h> // For RAND_bytes(.)
//#include <openssl/sha.h> // FOR SHA256_DIGEST_LENGTH
#endif

#ifdef USE_FSTREAM
#include <fstream> // used only by readInstance() and readWitness()
#endif

#ifdef USE_CHRONO
#include <stdint.h> // rdtsc
#endif

//#define VERBOSE

CircuitContainer::CircuitContainer() {

}

CircuitContainer::~CircuitContainer() {
  if(this->value_shares_ != NULL) {
    for(uint32 i = 0; i < this->party_size_; i++) {
      delete[] this->value_shares_[i];
    }
    delete[] this->value_shares_;
    this->value_shares_ = NULL;
  }
  if(this->value_ != NULL) delete[] this->value_;
  if(this->key_shares_ != NULL) {
    for(uint32 i = 0; i < this->party_size_; i++) {
      delete[] this->key_shares_[i];
    }
    delete[] this->key_shares_;
    this->key_shares_ = NULL;
  }
  if(this->key_ != NULL) delete[] this->key_;

  if(this->all_instance_shares_ != NULL) {
    for(uint32 i = 0; i < this->num_rings_; i++) {
      for (uint32 j = 0; j < this->party_size_; j++) {
        delete[] this->all_instance_shares_[i][j];
      }
      delete[] this->all_instance_shares_[i];
    }
    delete[] this->all_instance_shares_;
    this->all_instance_shares_ = NULL;
  }

  if (this->all_instance_values_ != NULL) {
    for(uint32 i = 0; i < this->num_rings_; i++) {
      delete[] this->all_instance_values_[i];
    }
    delete[] this->all_instance_values_;
  }

  if(this->all_witness_shares_ != NULL) {
    for(uint32 i = 0; i < this->num_rings_; i++) {
      for (uint32 j = 0; j < this->party_size_; j++) {
        delete[] this->all_witness_shares_[i][j];
      }
      delete[] this->all_witness_shares_[i];
    }
    delete[] this->all_witness_shares_;
    this->all_witness_shares_ = NULL;
  }

  if (this->all_witness_values_ != NULL) {
    for(uint32 i = 0; i < this->num_rings_; i++) {
      delete[] this->all_witness_values_[i];
    }
    delete[] this->all_witness_values_;
  }

  if (this->assert_eq_bw_shares_ != NULL) {
    for(uint32 i = 0; i < this->num_rings_; i++) {
      delete[] this->assert_eq_bw_shares_[i];
      delete[] this->assert_eq_ad_shares_[i];
    }
    delete[] this->assert_eq_bw_shares_;
    delete[] this->assert_eq_ad_shares_;
  }

  if(this->all_intermediate_results_ != NULL) {
    for (uint32 j = 0; j < this->num_rings_; j++) {
      BigIntLib* big_int_lib = &this->all_big_int_libs_[j];
      for(uint32 i = 0; i < big_int_lib->num_intermediate_results_; i++) {
        delete[] this->all_intermediate_results_[j][i];
      }
      delete[] this->all_intermediate_results_[j];
    }
    delete[] this->all_intermediate_results_;
  }

  // Clean up random numbers
  this->destroyRandomNumbers();

  // Clean up numbers for faster squaring
  if(this->squaring_precomp_ != NULL) delete[] this->squaring_precomp_;

  // Clean up BigIntLib
  //this->big_int_lib_->CleanUp();
  for (uint32 i = 0; i < this->num_rings_; i++) {
    this->all_big_int_libs_[i].CleanUp();
  }
  this->big_int_lib_ = NULL;
  delete[] this->all_big_int_libs_;
  BigIntLib::CleanUpStatic();
}

void CircuitContainer::readInstance() {
#ifdef USE_FSTREAM
  std::ifstream f_instance("generated_instance.txt");
  if (!f_instance.good()) {
#ifdef TESTING
    std::cerr << "Failed to open the instance file" << std::endl;
#endif
    exit(1);
  }
  this->all_instance_values_ = new word*[this->num_rings_];
  for(uint32 i = 0; i < this->num_rings_; i++) {
    BigIntLib* big_int_lib = &this->all_big_int_libs_[i];
    uint32 num_instance_words = big_int_lib->num_instance_values_ * big_int_lib->gate_num_words_;
    this->all_instance_values_[i] = new word[num_instance_words];
    for (uint32 j = 0; j < num_instance_words; j++) {
      f_instance >> this->all_instance_values_[i][j];
      //std::cout << "read instance " << this->all_instance_values_[i][j] << std::endl;
    }
  }
  f_instance.close();
#endif
}

void CircuitContainer::readWitness() {
#ifdef USE_FSTREAM
  std::ifstream f_witness("generated_witness.txt");
  if (!f_witness.good()) {
#ifdef TESTING
    std::cerr << "Failed to open the witness file" << std::endl;
#endif
    exit(1);
  }
  this->all_witness_values_ = new word*[this->num_rings_];
  for(uint32 i = 0; i < this->num_rings_; i++) {
    BigIntLib* big_int_lib = &this->all_big_int_libs_[i];
    uint32 num_witness_words = big_int_lib->num_witness_values_ * big_int_lib->gate_num_words_;
    this->all_witness_values_[i] = new word[num_witness_words];
    for (uint32 j = 0; j < num_witness_words; j++) {
      f_witness >> this->all_witness_values_[i][j];
      //std::cout << "read witness " << this->all_witness_values_[i][j] << std::endl;
    }
  }
  f_witness.close();
#endif
}

void CircuitContainer::init(uint32 value_size, uint32 random_tape_size, uint32 key_size, uint32 branch_bits, uint32 num_branches, uint32 field_type, uint32 party_size, bool need_prove) {
  //std::cout << "CircuitContainer::init()" << std::endl;
  this->initConsts();
  this->value_size_ = value_size;
  this->random_tape_size_ = random_tape_size;
  this->key_size_ = key_size;
  //this->branch_size_ = ceil(((float)branch_bits / WORD_SIZE) * 8);
  this->branch_size_ = (branch_bits + 7) / 8;
  //this->gate_size_ = ceil((float)branch_bits / 64) * 8;
  this->gate_size_ = (branch_bits + WORD_SIZE - 1) / WORD_SIZE * (WORD_SIZE / 8);
  //this->gate_num_words_ = ceil(float(this->gate_size_) / (WORD_SIZE / 8));
  this->gate_num_words_ = (branch_bits + WORD_SIZE - 1) / WORD_SIZE;
  this->branch_bits_ = branch_bits;
  //this->hash_size_ = SHA256_DIGEST_LENGTH;
  this->hash_size_ = 32;
  this->min_key_hash_size_ = MIN(this->key_size_, this->hash_size_);
#ifdef TESTING
  std::cout << "field_type: " << field_type << std::endl;
  std::cout << "value_size_: " << this->value_size_ << std::endl;
  std::cout << "key_size_: " << this->key_size_ << std::endl;
  std::cout << "min_key_hash_size_: " << this->min_key_hash_size_ << std::endl;
  std::cout << "hash_size_: " << this->hash_size_ << std::endl;
  std::cout << "gate_size_: " << this->gate_size_ << std::endl;
  std::cout << "gate_num_words_: " << this->gate_num_words_ << std::endl;
  std::cout << "branch_bits_: " << this->branch_bits_ << std::endl;
  std::cout << "sizeof(unsigned short): " << sizeof(unsigned short) << std::endl;
  std::cout << "sizeof(unsigned int): " << sizeof(unsigned int) << std::endl;
  std::cout << "sizeof(unsigned long): " << sizeof(unsigned long) << std::endl;
  std::cout << "sizeof(unsigned long long): " << sizeof(unsigned long long) << std::endl;
#endif
  this->party_size_ = party_size;
  this->value_ = new uchar[value_size];
  memset(this->value_, 0, value_size);
  this->value_shares_ = new uchar*[this->party_size_];
  this->key_shares_ = new uchar*[this->party_size_];
  //std::cout << "Generating key" << std::endl << std::flush;
  if (need_prove) {
    this->key_ = new uchar[key_size];
    this->randomizeKey();
  } else {
    this->key_ = NULL;
  }
  for(uint32 i = 0; i < party_size; i++) {
    this->value_shares_[i] = new uchar[value_size];
    memset(this->value_shares_[i], 0, value_size);
    this->key_shares_[i] = new uchar[key_size];
    memset(this->key_shares_[i], 0, key_size);
  }

  this->intermediate_results_ = NULL;
  this->squaring_precomp_ = NULL;
  this->num_feistel_branches_ = num_branches;
  //this->feistel_branch_indices_.push_back(0); // Push back single index for ciphers without feistel branches

  // Init BigIntLib
  BigIntLib::InitStatic();
  this->initBigIntLib();
  this->ring_no_ = this->num_rings_;
  this->bw_word_ring_no_ = this->num_rings_;
  BigIntLib* bw_word_ring = NULL;
  BigIntLib* ringEC = NULL;
  BigIntLib* ringP = NULL;
  BigIntLib* ringBW_EC = NULL;
  for(uint32 i = 0; i < this->num_rings_; i++) {
    BigIntLib* big_int_lib = &this->all_big_int_libs_[i];
    if (big_int_lib->field_type_ == 3 && big_int_lib->field_size_bits_ == WORD_SIZE) {
      this->bw_word_ring_no_ = i;
      bw_word_ring = big_int_lib;
    }
    if (big_int_lib->field_type_ == 4) {
      if (ringEC != NULL) {
#ifdef TESTING
        std::cerr << "Multiple elliptic curves in a single circuit are not supported" << std::endl;
#endif
        exit(1);
      }
      ringEC = big_int_lib;
#ifdef TESTING
      std::cout << "Elliptic curve with " << ringEC->field_size_bits_ << " bits" << std::endl;
#endif
    }
  }
  if (ringEC != NULL) {
    for(uint32 i = 0; i < this->num_rings_; i++) {
      BigIntLib* big_int_lib = &this->all_big_int_libs_[i];
      if (big_int_lib->field_type_ == 3 && 2*big_int_lib->field_size_bits_ == ringEC->field_size_bits_) {
        ringBW_EC = big_int_lib;
      }
      if (big_int_lib->field_type_ == 0 && 2*big_int_lib->field_size_bits_ == ringEC->field_size_bits_ && big_int_lib->ring_variant_ == 0) {
        ringP = big_int_lib;
      }
    }
    ringEC->ringP_ = ringP;
    ringEC->ringBW_ = ringBW_EC;
  }
  for(uint32 i = 0; i < this->num_rings_; i++) {
    BigIntLib* big_int_lib = &this->all_big_int_libs_[i];
    big_int_lib->num_random_gates_ = big_int_lib->num_mul_gates_;
    big_int_lib->num_intermediate_results_ = big_int_lib->num_mul_gates_;
    big_int_lib->num_declassify3s_ = 0;
    big_int_lib->ring_no_ = i;
  }
  for(uint32 i = 0; i < this->num_rings_; i++) {
    BigIntLib* big_int_lib = &this->all_big_int_libs_[i];
    if (big_int_lib->field_type_ == 4) {
      big_int_lib->num_random_gates_ += big_int_lib->num_ecdsa_verifications_;
      big_int_lib->num_assert_zeros_ += big_int_lib->num_ecdsa_verifications_;
      ringP->num_mul_gates_ += big_int_lib->num_ecdsa_verifications_ * 6;
      ringP->num_random_gates_ += big_int_lib->num_ecdsa_verifications_ * 6;
      ringP->num_assert_zeros_ += big_int_lib->num_ecdsa_verifications_ * 2;
      ringP->num_declassify3s_ += big_int_lib->num_ecdsa_verifications_ * 2;
    }
    if (big_int_lib->num_ring_conversions_ > 0) {
      if (big_int_lib->field_type_ != 2 && big_int_lib->field_type_ != 0) {
#ifdef TESTING
        std::cerr << "ERROR: Second ring in assert_eq_bw_ad must be an additive ring or prime field" << std::endl;
#endif
        exit(1);
      }
      BigIntLib* bw_ring = NULL;
      for (uint32 j = 0; j < this->num_rings_; j++) {
        BigIntLib* bw_ring1 = &this->all_big_int_libs_[j];
        if (bw_ring1->field_type_ == 3 && bw_ring1->field_size_bits_ == big_int_lib->field_size_bits_) {
          big_int_lib->bitwise_ring_no_ = j;
          bw_ring = bw_ring1;
          break;
        }
      }
      uint32 k = (big_int_lib->field_type_ == 2) ? 1 : 2;
      uint32 num_batches = (big_int_lib->num_ring_conversions_ + WORD_SIZE - 1) / WORD_SIZE;
      bw_word_ring->num_mul_gates_ += 2 * (k * big_int_lib->field_size_bits_ - 1) * num_batches;
      bw_word_ring->num_random_gates_ += 2 * (k * big_int_lib->field_size_bits_ - 1) * num_batches;
      bw_ring->num_declassify3s_ += big_int_lib->num_ring_conversions_;
      big_int_lib->num_random_gates_ += big_int_lib->num_ring_conversions_;
      big_int_lib->num_assert_zeros_ += big_int_lib->num_ring_conversions_;
    }
  }

  this->readInstance();

  if (need_prove) {
    this->readWitness();
  } else {
    this->all_witness_values_ = NULL;
  }

  // Allocate space for instance shares
  this->all_instance_shares_ = new word**[this->num_rings_];
  for(uint32 i = 0; i < this->num_rings_; i++) {
    this->all_instance_shares_[i] = new word*[this->party_size_];
    BigIntLib* big_int_lib = &this->all_big_int_libs_[i];
    uint32 num_instance_words = big_int_lib->num_instance_values_ * big_int_lib->gate_num_words_;
    for (uint32 j = 0; j < this->party_size_; j++) {
      this->all_instance_shares_[i][j] = new word[num_instance_words];
      memset(this->all_instance_shares_[i][j], 0, num_instance_words * (WORD_SIZE / 8));
    }
  }

  // Allocate space for witness shares
  this->all_witness_shares_ = new word**[this->num_rings_];
  for(uint32 i = 0; i < this->num_rings_; i++) {
    this->all_witness_shares_[i] = new word*[this->party_size_];
    BigIntLib* big_int_lib = &this->all_big_int_libs_[i];
    uint32 num_witness_words = big_int_lib->num_witness_values_ * big_int_lib->gate_num_words_;
    for (uint32 j = 0; j < this->party_size_; j++) {
      this->all_witness_shares_[i][j] = new word[num_witness_words];
      memset(this->all_witness_shares_[i][j], 0, num_witness_words * (WORD_SIZE / 8));
    }
  }

  // Allocate space for assert_eq_bw_ad shares
  this->assert_eq_bw_shares_ = new word*[this->num_rings_];
  this->assert_eq_ad_shares_ = new word*[this->num_rings_];
  for(uint32 i = 0; i < this->num_rings_; i++) {
    BigIntLib* big_int_lib = &this->all_big_int_libs_[i];
    uint32 num_words = big_int_lib->num_ring_conversions_ * this->party_size_ * big_int_lib->gate_num_words_;
    this->assert_eq_bw_shares_[i] = new word[num_words];
    this->assert_eq_ad_shares_[i] = new word[num_words];
    memset(this->assert_eq_bw_shares_[i], 0, num_words * (WORD_SIZE / 8));
    memset(this->assert_eq_ad_shares_[i], 0, num_words * (WORD_SIZE / 8));
  }

  // Allocate space for random numbers
  this->all_random_numbers_ = new word**[this->num_rings_];
  for (uint32 j = 0; j < this->num_rings_; j++) {
    this->all_random_numbers_[j] = new word*[this->party_size_];
    BigIntLib* big_int_lib = &this->all_big_int_libs_[j];
    uint32 n = big_int_lib->num_random_gates_ + big_int_lib->num_witness_values_;
    for(uint32 i = 0; i < this->party_size_; i++) {
      this->all_random_numbers_[j][i] = new word[n * big_int_lib->gate_num_words_];
      memset(this->all_random_numbers_[j][i], 0, n * big_int_lib->gate_num_words_ * (WORD_SIZE / 8));
    }
  }

  // Allocate space for intermediate results
  this->all_intermediate_results_ = new word**[this->num_rings_];
  for (uint32 j = 0; j < this->num_rings_; j++) {
    BigIntLib* big_int_lib = &this->all_big_int_libs_[j];
    this->all_intermediate_results_[j] = new word*[big_int_lib->num_intermediate_results_];
    for(uint32 i = 0; i < big_int_lib->num_intermediate_results_; i++) {
      this->all_intermediate_results_[j][i] = new word[big_int_lib->gate_num_words_];
      memset(this->all_intermediate_results_[j][i], 0x0, big_int_lib->gate_size_);
    }
  }

  setRing(0);
}

void CircuitContainer::initCipher(uint32 cipher_type) {
  if(cipher_type == 1) {
    this->initMiMC();
  }
  else {
#ifdef TESTING
    std::cout << "Error: Unknown cipher type '" << cipher_type << "'." << std::endl;
#endif
    exit(1);
  }
}

void CircuitContainer::setRing(uint32 ring_no) {
  if (ring_no != this->ring_no_) {
    this->ring_no_ = ring_no;
    this->big_int_lib_ = &this->all_big_int_libs_[ring_no];
    this->random_numbers_ = this->all_random_numbers_[ring_no];
    this->intermediate_results_ = this->all_intermediate_results_[ring_no];
  }
}

void CircuitContainer::runSign(uchar* x, SignData* sign_data) {
  // Stack storage for field values
  //word* test = new word[this->num_feistel_branches_ * this->party_size_ * this->gate_num_words_ * 2];
  setRing(0);
  alignas(8) word value_shares_f[this->num_feistel_branches_][this->party_size_][this->gate_num_words_];
  alignas(8) word key_shares_f[this->num_feistel_branches_][this->party_size_][this->gate_num_words_];
  memset(value_shares_f, 0, this->num_feistel_branches_ * this->party_size_ * this->gate_size_);
  memset(key_shares_f, 0, this->num_feistel_branches_ * this->party_size_ * this->gate_size_);
  
  // Prepare (should be the same routine for each circuit!)
  //auto circuit_sign_start = std::chrono::high_resolution_clock::now();
  this->beforeSign(x, sign_data, (word*)value_shares_f, (word*)key_shares_f);
  //auto circuit_sign_stop = std::chrono::high_resolution_clock::now();

  // Run circuit
#ifdef USE_CHRONO
  auto circuit_sign_start = std::chrono::high_resolution_clock::now();
#endif
  (this->*circuit_function_)((word*)value_shares_f, (word*)key_shares_f, this->party_size_);
#ifdef USE_CHRONO
  auto circuit_sign_stop = std::chrono::high_resolution_clock::now();
#endif
  //this->last_circuit_sign_time_ = std::chrono::duration_cast<std::chrono::nanoseconds>(circuit_sign_stop - circuit_sign_start).count();

  // Clean up (should be the same routine for each circuit!)
  //auto circuit_sign_start = std::chrono::high_resolution_clock::now();
  this->afterSign((word*)value_shares_f);
  //auto circuit_sign_stop = std::chrono::high_resolution_clock::now();

#ifdef USE_CHRONO
  this->last_circuit_sign_time_ = std::chrono::duration_cast<std::chrono::nanoseconds>(circuit_sign_stop - circuit_sign_start).count();
#endif
}

void CircuitContainer::beforeSign(uchar* x, SignData* sign_data, word* value_shares_f, word* key_shares_f) {

  this->sign_data_ = sign_data;

  setRing(0);

  // NEW
  // Set this->value_shares_[0] to x, and both this->value_shares_[1] and this->value_shares_[2] to 0
  memcpy(this->value_shares_[0], x, this->value_size_);
  memset(this->value_shares_[1], 0x0, this->value_size_);
  memset(this->value_shares_[2], 0x0, this->value_size_);

  memset(this->key_shares_[0], 0, this->key_size_);
  memset(this->key_shares_[1], 0, this->key_size_);
  memcpy(this->key_shares_[0], (this->sign_data_)->random_tapes_hashs_, this->min_key_hash_size_);
  memcpy(this->key_shares_[1], (this->sign_data_)->random_tapes_hashs_ + this->hash_size_, this->min_key_hash_size_);

  // Calculate final shares for value and key
  (this->*prepare_shares_field_sign_function_)(x, value_shares_f, key_shares_f);

  // Write x_3 (last key share!) to SignData
  memcpy((this->sign_data_)->x_3_, this->key_shares_[2], this->value_size_);

  #ifdef VERBOSE
  std::cout << "[SIGN] Input private x: ";
  this->big_int_lib_->Print(x, this->value_size_);
  //this->big_int_lib_->Print((word*)this->value_shares_[0], this->value_size_);
  //this->big_int_lib_->Print((word*)this->value_shares_[1], this->value_size_);
  for(uint32 i = 0; i < this->party_size_; i++) {
    std::cout << "[SIGN] Input share " << i << ": ";
    this->big_int_lib_->Print(this->value_shares_[i], this->value_size_);
  }
  #endif

  #ifdef VERBOSE
  for(uint32 i = 0; i < this->party_size_; i++) {
    std::cout << "[SIGN] Key share " << i << ": ";
    this->big_int_lib_->Print(this->key_shares_[i], this->key_size_);
  }
  #endif

  uchar* random_tapes[3];
  random_tapes[0] = (this->sign_data_)->random_tapes_;
  random_tapes[1] = random_tapes[0] + this->num_rings_ * this->random_tape_size_;
  random_tapes[2] = random_tapes[1] + this->num_rings_ * this->random_tape_size_;
  this->prepareRandomNumbers(random_tapes, this->party_size_);

  for (uint32 k = 0; k < this->num_rings_; k++) {
    setRing(k);

    // Instance shares
    memcpy(this->all_instance_shares_[k][0], this->all_instance_values_[k], big_int_lib_->num_instance_values_ * big_int_lib_->gate_num_words_ * (WORD_SIZE / 8));
    memset(this->all_instance_shares_[k][1], 0, big_int_lib_->num_instance_values_ * big_int_lib_->gate_num_words_ * (WORD_SIZE / 8));
    memset(this->all_instance_shares_[k][2], 0, big_int_lib_->num_instance_values_ * big_int_lib_->gate_num_words_ * (WORD_SIZE / 8));

    // Witness shares
    for(uint32 i = 0; i < this->party_size_; i++) {
      if (i < 2) {
        memcpy(this->all_witness_shares_[k][i], this->random_numbers_[i] + big_int_lib_->num_random_gates_ * big_int_lib_->gate_num_words_, big_int_lib_->num_witness_values_ * big_int_lib_->gate_num_words_ * (WORD_SIZE / 8));
      } else {
        word* w = this->all_witness_shares_[k][i];
        word* w0 = this->all_witness_shares_[k][0];
        word* w1 = this->all_witness_shares_[k][1];
        word t[big_int_lib_->gate_num_words_];
        for (uint32 j = 0; j < big_int_lib_->num_witness_values_; j++) {
          uint32 j2 = j * big_int_lib_->gate_num_words_;
          this->big_int_lib_->Sub(t, this->all_witness_values_[k] + j2, w0 + j2);
          this->big_int_lib_->Sub(w + j2, t, w1 + j2);
        }
        memcpy(this->sign_data_->witness_3_[k], w, this->big_int_lib_->witness_3_size());
      }
    }

    // Reset current mul gate and current intermediate result
    this->big_int_lib_->current_mul_gate_ = 0;
    this->big_int_lib_->current_random_gate_ = 0;
    this->big_int_lib_->current_intermediate_result_ = 0;
    this->big_int_lib_->current_declassify3_ = 0;
    this->big_int_lib_->current_assert_zero_ = 0;
    this->big_int_lib_->current_assert_eq_bw_ad_ = 0;
    this->big_int_lib_->current_instance_ = 0;
    this->big_int_lib_->current_witness_ = 0;
  }

  this->verify_ = false;

  // Function pointers
  this->get_instance_shared_function_ = &CircuitContainer::getInstanceSharedSign;
  this->get_witness_shared_function_ = &CircuitContainer::getWitnessSharedSign;
  this->assert_zero_shared_function_ = &CircuitContainer::assertZeroSharedSign;
  this->assert_eq_bw_ad_shared_function_ = &CircuitContainer::assertEqBwAdSharedSign;
  this->bitwise_to_bitwise_shared_function_ = &CircuitContainer::bitwiseToBitwiseSharedSign;
  this->bitwise_vec_to_bitwise_vec_shared_function_ = &CircuitContainer::bitwiseVecToBitwiseVecSharedSign;
  this->bitwise_matrix_transpose_shared_function_ = &CircuitContainer::bitwiseMatrixTransposeSharedSign;
  this->ecdsa_verification_shared_function_ = &CircuitContainer::ecdsaVerificationSharedSign;
  this->copy_c_shared_function_ = &CircuitContainer::copyCSharedSign;
  this->add_c_shared_function_ = &CircuitContainer::addCSharedSign;
  this->mul_c_shared_function_ = &CircuitContainer::mulCSharedSign;
  this->shl_c_shared_function_ = &CircuitContainer::shlCSharedSign;
  this->shr_c_shared_function_ = &CircuitContainer::shrCSharedSign;
  this->add_shared_function_ = &CircuitContainer::addSharedSign;
  this->sub_shared_function_ = &CircuitContainer::subSharedSign;
  this->mul_shared_function_ = &CircuitContainer::mulSharedSign;
}

void CircuitContainer::prepareSharesFieldSign(uchar* x, word* value_shares_f, word* key_shares_f) {
  word *(key_shares_tmp[3]) = {key_shares_f, key_shares_f + this->gate_num_words_, key_shares_f + 2 * this->gate_num_words_};
  word temp[this->gate_num_words_];
  memset(temp, 0, this->gate_size_);

  // Value shares (do 1 and 2 outside?)
  memcpy(value_shares_f, this->value_shares_[0], this->branch_size_);
  memset(value_shares_f + this->gate_num_words_, 0x0, this->branch_size_);
  memset(value_shares_f + 2 * this->gate_num_words_, 0x0, this->branch_size_);

  // Key shares
  memcpy(key_shares_tmp[0], this->key_shares_[0], this->branch_size_);
  memcpy(key_shares_tmp[1], this->key_shares_[1], this->branch_size_);
  memcpy(temp, this->key_, this->branch_size_);
  this->big_int_lib_->TryReduce(key_shares_tmp[0]);
  this->big_int_lib_->TryReduce(key_shares_tmp[1]);
  this->big_int_lib_->TryReduce(temp);
  this->big_int_lib_->Sub(key_shares_tmp[2], temp, key_shares_tmp[0]);
  this->big_int_lib_->Sub(key_shares_tmp[2], key_shares_tmp[2], key_shares_tmp[1]);
  memcpy(this->key_shares_[2], key_shares_tmp[2], this->branch_size_);
}

void CircuitContainer::afterSign(word* value_shares_f) {
  // Write shares to bytes
  setRing(0);
  (this->*output_shares_to_bytes_function_)(value_shares_f, this->party_size_);

  // Write last values to SignData
  for(uint32 i = 0; i < this->party_size_; i++) {
    memcpy((this->sign_data_)->y_shares_ + (i * this->value_size_), this->value_shares_[i], this->value_size_);
    #ifdef VERBOSE
    std::cout << "[SIGN] Output share " << i << ": ";
    this->big_int_lib_->Print((this->sign_data_)->y_shares_ + (i * this->value_size_), this->value_size_);
    #endif
  }
  
  // Destroy unneeded data
  //this->destroyRandomNumbers(random_numbers, this->party_size_);
}

void CircuitContainer::runVerify(Proof* p, uchar* x, uchar* y, VerifyData* verify_data, uint32 iteration) {
  // Prepare (should be the same routine for each circuit!)
  // Stack storage for field values
  setRing(0);
  alignas(8) word value_shares_f[this->num_feistel_branches_][this->party_size_ - 1][this->gate_num_words_];
  alignas(8) word key_shares_f[this->num_feistel_branches_][this->party_size_ - 1][this->gate_num_words_];
  memset(value_shares_f, 0, this->num_feistel_branches_ * (this->party_size_ - 1) * this->gate_size_);
  memset(key_shares_f, 0, this->num_feistel_branches_ * (this->party_size_ - 1) * this->gate_size_);

  #ifdef VERBOSE
  std::cout << "[VERIFY] Entering Verify step with y: ";
  this->big_int_lib_->Print(y, this->value_size_);
  #endif

  uchar* key_shares[this->party_size_ - 1];
  this->beforeVerify(p, x, y, verify_data, key_shares, (word*)value_shares_f, (word*)key_shares_f, iteration);

  // Run circuit
#ifdef USE_CHRONO
  auto circuit_verify_start = std::chrono::high_resolution_clock::now();
#endif
  (this->*circuit_function_)((word*)value_shares_f, (word*)key_shares_f, this->party_size_ - 1);
#ifdef USE_CHRONO
  auto circuit_verify_stop = std::chrono::high_resolution_clock::now();
  this->last_circuit_verify_time_ = std::chrono::duration_cast<std::chrono::nanoseconds>(circuit_verify_stop - circuit_verify_start).count();
#endif

  // Clean up (should be the same routine for each circuit!)
  this->afterVerify(y, (word*)value_shares_f);
}

void CircuitContainer::beforeVerify(Proof* p, uchar* x, uchar* y, VerifyData* verify_data, uchar** key_shares, word* value_shares_f, word* key_shares_f, uint32 iteration) {
  this->proof_ = p;
  this->verify_data_ = verify_data;
  this->e_ = p->e_i_[iteration];
  
  this->iteration_ = iteration;

  #ifdef VERBOSE
  std::cout << "[VERIFY] e: " << this->e_ << std::endl;
  #endif

  // Input and the two shares
  // Remark: shares[0] and shares[1] are BOTH needed and should be written throughout the computation! each given View value after a MUL must be written to shares[1]!

  setRing(0);
  memset(this->key_shares_[0], 0, this->key_size_);
  memset(this->key_shares_[1], 0, this->key_size_);
  memset(this->value_shares_[0], 0, this->value_size_);
  memset(this->value_shares_[1], 0, this->value_size_);

  uchar* random_tapes[2];
  random_tapes[0] = p->zs_[iteration]->k_1_;
  random_tapes[1] = p->zs_[iteration]->k_2_;
  this->prepareRandomNumbers(random_tapes, this->party_size_ - 1);

  if(this->e_ == 0) {
    memcpy(this->key_shares_[0], p->zs_[iteration]->k_1_hash_, this->min_key_hash_size_);
    memcpy(this->key_shares_[1], p->zs_[iteration]->k_2_hash_, this->min_key_hash_size_);
    memcpy(this->value_shares_[0], x, this->value_size_);
    /*
    // Generic solution for security < 256 bits (a bit slower due to std::min, could be stored in var at the beginning)
    memcpy(this->value_shares_[0], p->zs_[iteration]->k_1_hash_, std::min(this->value_size_, this->hash_size_));
    memcpy(this->value_shares_[1], p->zs_[iteration]->k_2_hash_, std::min(this->value_size_, this->hash_size_));
    */
  }
  else if(this->e_ == 1) {
    memcpy(this->key_shares_[0], p->zs_[iteration]->k_2_hash_, this->min_key_hash_size_);
    memcpy(this->key_shares_[1], p->zs_[iteration]->x_3_, this->value_size_);
    /*
    // Generic solution for security < 256 bits (a bit slower due to std::min, could be stored in var at the beginning)
    memcpy(this->value_shares_[0], p->zs_[iteration]->k_2_hash_, std::min(this->value_size_, this->hash_size_));
    memcpy(this->value_shares_[1], p->zs_[iteration]->x_3_, this->value_size_);
    */
  }
  else if(this->e_ == 2) {
    memcpy(this->key_shares_[0], p->zs_[iteration]->x_3_, this->value_size_);
    memcpy(this->key_shares_[1], p->zs_[iteration]->k_1_hash_, this->min_key_hash_size_);
    memcpy(this->value_shares_[1], x, this->value_size_);
    /*
    // Generic solution for security < 256 bits (a bit slower due to std::min, could be stored in var at the beginning)
    memcpy(this->value_shares_[0], p->zs_[iteration]->x_3_, this->value_size_);
    memcpy(this->value_shares_[1], p->zs_[iteration]->k_1_hash_, std::min(this->value_size_, this->hash_size_));
    */
  }
  else {
#ifdef TESTING
    std::cout << "Error: e not in {0, 1, 2}" << std::endl;
#endif
  }

  for (uint32 k = 0; k < this->num_rings_; k++) {
    setRing(k);

    if(this->e_ == 0) {
      memcpy(this->all_instance_shares_[k][0], this->all_instance_values_[k], big_int_lib_->num_instance_values_ * big_int_lib_->gate_num_words_ * (WORD_SIZE / 8));
      memset(this->all_instance_shares_[k][1], 0, big_int_lib_->num_instance_values_ * big_int_lib_->gate_num_words_ * (WORD_SIZE / 8));
      memcpy(this->all_witness_shares_[k][0], this->random_numbers_[0] + big_int_lib_->num_random_gates_ * big_int_lib_->gate_num_words_, big_int_lib_->num_witness_values_ * big_int_lib_->gate_num_words_ * (WORD_SIZE / 8));
      memcpy(this->all_witness_shares_[k][1], this->random_numbers_[1] + big_int_lib_->num_random_gates_ * big_int_lib_->gate_num_words_, big_int_lib_->num_witness_values_ * big_int_lib_->gate_num_words_ * (WORD_SIZE / 8));
    }
    else if(this->e_ == 1) {
      memset(this->all_instance_shares_[k][0], 0, big_int_lib_->num_instance_values_ * big_int_lib_->gate_num_words_ * (WORD_SIZE / 8));
      memset(this->all_instance_shares_[k][1], 0, big_int_lib_->num_instance_values_ * big_int_lib_->gate_num_words_ * (WORD_SIZE / 8));
      memcpy(this->all_witness_shares_[k][0], this->random_numbers_[0] + big_int_lib_->num_random_gates_ * big_int_lib_->gate_num_words_, big_int_lib_->num_witness_values_ * big_int_lib_->gate_num_words_ * (WORD_SIZE / 8));
      memcpy(this->all_witness_shares_[k][1], p->zs_[iteration]->witness_3_[k], this->big_int_lib_->witness_3_size());
    }
    else if(this->e_ == 2) {
      memset(this->all_instance_shares_[k][0], 0, big_int_lib_->num_instance_values_ * big_int_lib_->gate_num_words_ * (WORD_SIZE / 8));
      memcpy(this->all_instance_shares_[k][1], this->all_instance_values_[k], big_int_lib_->num_instance_values_ * big_int_lib_->gate_num_words_ * (WORD_SIZE / 8));
      memcpy(this->all_witness_shares_[k][0], p->zs_[iteration]->witness_3_[k], this->big_int_lib_->witness_3_size());
      memcpy(this->all_witness_shares_[k][1], this->random_numbers_[1] + big_int_lib_->num_random_gates_ * big_int_lib_->gate_num_words_, big_int_lib_->num_witness_values_ * big_int_lib_->gate_num_words_ * (WORD_SIZE / 8));
    }
    else {
#ifdef TESTING
      std::cout << "Error: e not in {0, 1, 2}" << std::endl;
#endif
    }

    // Reset current mul gate and current intermediate result
    this->big_int_lib_->current_mul_gate_ = 0;
    this->big_int_lib_->current_random_gate_ = 0;
    this->big_int_lib_->current_intermediate_result_ = 0;
    this->big_int_lib_->current_declassify3_ = 0;
    this->big_int_lib_->current_assert_zero_ = 0;
    this->big_int_lib_->current_assert_eq_bw_ad_ = 0;
    this->big_int_lib_->current_instance_ = 0;
    this->big_int_lib_->current_witness_ = 0;
  }

  setRing(0);
  #ifdef VERBOSE
  for(uint32 i = 0; i < this->party_size_ - 1; i++) {
    std::cout << "[VERIFY] Input share " << i << ": ";
    this->big_int_lib_->Print(this->value_shares_[i], this->value_size_);
  }
  #endif

  #ifdef VERBOSE
  for(uint32 i = 0; i < this->party_size_ - 1; i++) {
    std::cout << "[VERIFY] Key share " << i << ": ";
    this->big_int_lib_->Print(this->key_shares_[i], this->key_size_);
  }
  #endif

  (this->*prepare_shares_field_verify_function_)(value_shares_f, key_shares_f);

  this->verify_ = true;

  // Function pointers
  this->get_instance_shared_function_ = &CircuitContainer::getInstanceSharedVerify;
  this->get_witness_shared_function_ = &CircuitContainer::getWitnessSharedVerify;
  this->assert_zero_shared_function_ = &CircuitContainer::assertZeroSharedVerify;
  this->assert_eq_bw_ad_shared_function_ = &CircuitContainer::assertEqBwAdSharedVerify;
  this->bitwise_to_bitwise_shared_function_ = &CircuitContainer::bitwiseToBitwiseSharedVerify;
  this->bitwise_vec_to_bitwise_vec_shared_function_ = &CircuitContainer::bitwiseVecToBitwiseVecSharedVerify;
  this->bitwise_matrix_transpose_shared_function_ = &CircuitContainer::bitwiseMatrixTransposeSharedVerify;
  this->ecdsa_verification_shared_function_ = &CircuitContainer::ecdsaVerificationSharedVerify;
  this->copy_c_shared_function_ = &CircuitContainer::copyCSharedVerify;
  this->add_c_shared_function_ = &CircuitContainer::addCSharedVerify;
  this->mul_c_shared_function_ = &CircuitContainer::mulCSharedVerify;
  this->shl_c_shared_function_ = &CircuitContainer::shlCSharedVerify;
  this->shr_c_shared_function_ = &CircuitContainer::shrCSharedVerify;
  this->add_shared_function_ = &CircuitContainer::addSharedVerify;
  this->sub_shared_function_ = &CircuitContainer::subSharedVerify;
  this->mul_shared_function_ = &CircuitContainer::mulSharedVerify;
}

void CircuitContainer::prepareSharesFieldVerify(word* value_shares_f, word* key_shares_f) {
  word *(value_shares_tmp[2]) = {value_shares_f, value_shares_f + this->gate_num_words_};
  word *(key_shares_tmp[2]) = {key_shares_f, key_shares_f + this->gate_num_words_};

  // Value shares
  memcpy(value_shares_tmp[0], this->value_shares_[0], this->branch_size_);
  memcpy(value_shares_tmp[1], this->value_shares_[1], this->branch_size_);
  this->big_int_lib_->TryReduce(value_shares_tmp[0]);
  this->big_int_lib_->TryReduce(value_shares_tmp[1]);

  // Key shares
  memcpy(key_shares_tmp[0], this->key_shares_[0], this->branch_size_);
  memcpy(key_shares_tmp[1], this->key_shares_[1], this->branch_size_);
  this->big_int_lib_->TryReduce(key_shares_tmp[0]);
  this->big_int_lib_->TryReduce(key_shares_tmp[1]);
}

void CircuitContainer::afterVerify(uchar* y, word* value_shares_f) {
  // Write shares to bytes
  setRing(0);
  (this->*output_shares_to_bytes_function_)(value_shares_f, this->party_size_ - 1);

  memcpy((this->verify_data_)->y_share_, this->value_shares_[0], this->value_size_);

  #ifdef VERBOSE
  std::cout << "[VERIFY] Output: ";
  this->big_int_lib_->Print(y, this->value_size_);
  std::cout << "[VERIFY] Calculated Output share: ";
  this->big_int_lib_->Print((this->verify_data_)->y_share_, this->value_size_);
  std::cout << "[VERIFY] Given Output share: ";
  this->big_int_lib_->Print(((this->proof_)->zs_[this->iteration_])->y_share_, this->value_size_);
  #endif

  // Calculate last share
  (this->*verify_calc_last_share_function_)(y, value_shares_f);

  #ifdef VERBOSE
  std::cout << "[VERIFY] Calculated Final Output share: ";
  this->big_int_lib_->Print((this->verify_data_)->y_e2_, this->value_size_);
  #endif

  // Destroy unneeded data
  //this->destroyRandomNumbers(random_numbers, this->party_size_ - 1);
}

void CircuitContainer::directEncryption(uchar* x, uchar* y) {
#ifdef USE_CHRONO
  auto direct_call_start = std::chrono::high_resolution_clock::now();
#endif
  (this->*direct_function_)(x, y);
#ifdef USE_CHRONO
  auto direct_call_stop = std::chrono::high_resolution_clock::now();
  this->last_direct_call_time_ = std::chrono::duration_cast<std::chrono::nanoseconds>(direct_call_stop - direct_call_start).count();
#endif
}

void CircuitContainer::getParams(uint32* value_size, uint32* random_tape_size, uint32* key_size, uint32* gate_size) {
  *value_size = this->value_size_;
  *random_tape_size = this->random_tape_size_;
  *key_size = this->key_size_;
  *gate_size = this->gate_size_;
}

void CircuitContainer::prepareRandomNumbers(uchar** random_tapes, uint32 party_size) {
  for (uint32 j = 0; j < this->num_rings_; j++) {
    BigIntLib* big_int_lib = &this->all_big_int_libs_[j];
    uint32 n = big_int_lib->num_random_gates_ + big_int_lib->num_witness_values_;
    for(uint32 i = 0; i < party_size; i++) {
      // Use random tape of this party as seed and set random numbers for mul gates
      big_int_lib->GetRandomFieldElements(this->all_random_numbers_[j][i], random_tapes[i] + j * this->random_tape_size_, n);
    }
  }
}

void CircuitContainer::destroyRandomNumbers() {
  for (uint32 j = 0; j < this->num_rings_; j++) {
    for(uint32 i = 0; i < this->party_size_; i++) {
      delete[] this->all_random_numbers_[j][i];
    }
    delete[] this->all_random_numbers_[j];
  }
  delete[] this->all_random_numbers_;
}

void CircuitContainer::randomizeKey() {
#ifdef USE_OPENSSL
  RAND_bytes(this->key_, this->key_size_);
#endif
  //memset(this->key_, 0xAB, this->key_size_); // DETERMINISTIC FOR TESTING!
}

void CircuitContainer::outputSharesToBytes(word* output_shares, uint32 party_size) {
  for(uint32 i = 0; i < 3; i++) {
    memcpy(this->value_shares_[i], output_shares + i * this->gate_num_words_, this->branch_size_);
  }
}

void CircuitContainer::verifyCalcLastShare(uchar* y, word* value_shares_f) {
  word words_temp[this->num_feistel_branches_][this->gate_num_words_];
  memset(words_temp, 0, this->num_feistel_branches_ * this->gate_size_);
  word value_temp[this->num_feistel_branches_][this->gate_num_words_];
  memset(value_temp, 0, this->num_feistel_branches_ * this->gate_size_);
  word given_temp[this->num_feistel_branches_][this->gate_num_words_];
  memset(given_temp, 0, this->num_feistel_branches_ * this->gate_size_);
  memcpy(value_temp[0], y, this->branch_size_);
  memcpy(given_temp[0], ((this->proof_)->zs_[this->iteration_])->y_share_, this->branch_size_);
  this->big_int_lib_->Sub(words_temp[0], value_temp[0], value_shares_f); // First party is always the calculated party in the verify step
  this->big_int_lib_->Sub(words_temp[0], words_temp[0], given_temp[0]);
  memcpy((this->verify_data_)->y_e2_, words_temp[0], this->branch_size_);
}

uint64 CircuitContainer::rdtsc() {
#ifdef USE_CHRONO
  uint32 high, low;
  __asm__ __volatile__ ("rdtsc" : "=a" (low), "=d" (high));
  return (((uint64)high << 32) | low);
#else
  return 0;
#endif
}

uchar* CircuitContainer::getKey() {
  return this->key_;
}

uint32 CircuitContainer::getLastCircuitSignNS() {
  return this->last_circuit_sign_time_;
}

uint32 CircuitContainer::getLastCircuitVerifyNS() {
  return this->last_circuit_verify_time_;
}

uint64 CircuitContainer::getLastDirectCallCycles() {
  return this->last_direct_call_cycles_;
}

uint32 CircuitContainer::getLastDirectCallNS() {
  return this->last_direct_call_time_;
}

uint32 CircuitContainer::getCipherNumBranches() {
  return this->num_feistel_branches_;
}

void CircuitContainer::getInstanceDirect(word* a) {
    //std::cout << "getInstanceDirect " << this->current_instance_ << std::endl;
    if (this->big_int_lib_->current_instance_ >= big_int_lib_->num_instance_values_) {
#ifdef TESTING
        std::cerr << "ERROR: number of get_instances is too high" << std::endl;
#endif
        exit(1);
    }

    uint32 k2 = this->big_int_lib_->current_instance_ * big_int_lib_->gate_num_words_;
    for (uint32 j = 0; j < big_int_lib_->gate_num_words_; j++) {
        a[j] = this->all_instance_values_[ring_no_][k2 + j];
    }

    this->big_int_lib_->current_instance_++;
}

void CircuitContainer::getWitnessDirect(word* a) {
    //std::cout << "getWitnessDirect " << this->current_witness_ << std::endl;
    if (this->big_int_lib_->current_witness_ >= big_int_lib_->num_witness_values_) {
#ifdef TESTING
        std::cerr << "ERROR: number of get_witnesses is too high" << std::endl;
#endif
        exit(1);
    }

    uint32 k2 = this->big_int_lib_->current_witness_ * big_int_lib_->gate_num_words_;
    for (uint32 j = 0; j < big_int_lib_->gate_num_words_; j++) {
        a[j] = this->all_witness_values_[ring_no_][k2 + j];
    }

    this->big_int_lib_->current_witness_++;
}

void CircuitContainer::assertZeroDirect(word* a) {
    //std::cout << "assertZeroDirect" << std::endl;
    if (!this->big_int_lib_->IsZero(a)) {
#ifdef TESTING
        std::cerr << "ERROR (assertZeroDirect): assertion failed" << std::endl << std::flush;
        this->big_int_lib_->Print(a, big_int_lib_->value_size_);
#endif
        exit(1);
    }
}

// Conversion from one bitwise ring to another
void CircuitContainer::bitwiseToBitwiseDirect(uint32 to_ring_no, uint32 from_ring_no, word* b, word* a) {
  BigIntLib* to_ring = &this->all_big_int_libs_[to_ring_no];
  BigIntLib* from_ring = &this->all_big_int_libs_[from_ring_no];
  if (to_ring->field_type_ != 3 || from_ring->field_type_ != 3) {
#ifdef TESTING
    std::cerr << "bitwiseToBitwiseDirect can be used only for conversion from one bitwise ring to another" << std::endl << std::flush;
#endif
    exit(1);
  }
  if (to_ring->field_size_bits_ > from_ring->field_size_bits_) {
    memcpy(b, a, from_ring->gate_size_);
    memset(b + from_ring->gate_num_words_, 0, to_ring->gate_size_ - from_ring->gate_size_);
  } else if (to_ring->field_size_bits_ < from_ring->field_size_bits_) {
    memcpy(b, a, to_ring->gate_size_);
    b[to_ring->gate_num_words_ - 1] &= to_ring->msb_word_mask_;
  } else {
    memcpy(b, a, from_ring->gate_size_);
  }
}

// word* shares contains the words of ALL shares (share_party_1_word_1, share_party_1_word_2, ... ,share_party_n_word_m-1, share_party_n_word_m) for n parties and m words
void CircuitContainer::getInstanceSharedSign(word* a_shares) {
    if (this->iteration_ == 1 && this->big_int_lib_->current_instance_ >= big_int_lib_->num_instance_values_) {
#ifdef TESTING
        std::cerr << "ERROR: number of get_instances is too high" << std::endl;
#endif
        exit(1);
    }

    word *a_shares_ptr = a_shares;
    uint32 k2 = this->big_int_lib_->current_instance_ * big_int_lib_->gate_num_words_;
    for (uint32 i = 0; i < 3; i++) {
        for (uint32 j = 0; j < big_int_lib_->gate_num_words_; j++) {
            *a_shares_ptr++ = this->all_instance_shares_[ring_no_][i][k2 + j];
        }
    }

    this->big_int_lib_->current_instance_++;
}

// word* shares contains the words of ALL shares (share_party_1_word_1, share_party_1_word_2, ... ,share_party_n_word_m-1, share_party_n_word_m) for n parties and m words
void CircuitContainer::getInstanceSharedVerify(word* a_shares) {
    //std::cout << "getInstanceSharedVerify " << this->current_instance_ << std::endl;
    if (this->iteration_ == 1 && this->big_int_lib_->current_instance_ >= big_int_lib_->num_instance_values_) {
#ifdef TESTING
        std::cerr << "ERROR: number of get_instances is too high" << std::endl;
#endif
        exit(1);
    }

    word *a_shares_ptr = a_shares;
    uint32 k2 = this->big_int_lib_->current_instance_ * big_int_lib_->gate_num_words_;
    for (uint32 i = 0; i < 2; i++) {
        for (uint32 j = 0; j < big_int_lib_->gate_num_words_; j++) {
            *a_shares_ptr++ = this->all_instance_shares_[ring_no_][i][k2 + j];
        }
    }

    this->big_int_lib_->current_instance_++;
}

// word* shares contains the words of ALL shares (share_party_1_word_1, share_party_1_word_2, ... ,share_party_n_word_m-1, share_party_n_word_m) for n parties and m words
void CircuitContainer::getWitnessSharedSign(word* a_shares) {
    //std::cout << "getWitnessSharedSign " << this->current_witness_ << std::endl;
    if (this->iteration_ == 1 && this->big_int_lib_->current_witness_ >= big_int_lib_->num_witness_values_) {
#ifdef TESTING
        std::cerr << "ERROR: number of get_witnesses is too high" << std::endl;
#endif
        exit(1);
    }

    word *a_shares_ptr = a_shares;
    uint32 k2 = this->big_int_lib_->current_witness_ * big_int_lib_->gate_num_words_;
    for (uint32 i = 0; i < 3; i++) {
        for (uint32 j = 0; j < big_int_lib_->gate_num_words_; j++) {
            *a_shares_ptr++ = this->all_witness_shares_[ring_no_][i][k2 + j];
        }
    }

    this->big_int_lib_->current_witness_++;
}

// This allows prover to set the value of a witness that was not known during witness file generation
// It must be 0 in the witness file
// The shares of this value are written to a_shares
void CircuitContainer::setWitnessSharedSign(word* witness_value, word* a_shares) {
    if (this->iteration_ == 1 && this->big_int_lib_->current_witness_ >= big_int_lib_->num_witness_values_) {
#ifdef TESTING
        std::cerr << "ERROR: number of get_witnesses is too high" << std::endl;
#endif
        exit(1);
    }

    word *a_shares_ptr = a_shares;
    uint32 k2 = this->big_int_lib_->current_witness_ * big_int_lib_->gate_num_words_;
    uint32 k2byte = this->big_int_lib_->current_witness_ * big_int_lib_->gate_size_;
    if (!big_int_lib_->IsZero(this->all_witness_values_[ring_no_] + k2)) {
#ifdef TESTING
        std::cerr << "setWitnessSharedSign: witness value in witness file is not zero" << std::endl;
#endif
        exit(1);
    }
    word *w = this->all_witness_shares_[ring_no_][2] + k2;
    word t[big_int_lib_->gate_num_words_];
    big_int_lib_->Sub(t, witness_value, this->all_witness_shares_[ring_no_][0] + k2);
    big_int_lib_->Sub(w, t, this->all_witness_shares_[ring_no_][1] + k2);
    memcpy(this->sign_data_->witness_3_[ring_no_] + k2byte, w, big_int_lib_->gate_size_);
    for (uint32 i = 0; i < this->party_size_; i++) {
        for (uint32 j = 0; j < big_int_lib_->gate_num_words_; j++) {
            *a_shares_ptr++ = this->all_witness_shares_[ring_no_][i][k2 + j];
        }
    }

    this->big_int_lib_->current_witness_++;
}

// word* shares contains the words of ALL shares (share_party_1_word_1, share_party_1_word_2, ... ,share_party_n_word_m-1, share_party_n_word_m) for n parties and m words
void CircuitContainer::getWitnessSharedVerify(word* a_shares) {
    //std::cout << "getWitnessSharedVerify " << this->current_witness_ << std::endl;
    if (this->iteration_ == 1 && this->big_int_lib_->current_witness_ >= big_int_lib_->num_witness_values_) {
#ifdef TESTING
        std::cerr << "ERROR: number of get_witnesses is too high" << std::endl;
#endif
        exit(1);
    }

    word *a_shares_ptr = a_shares;
    uint32 k2 = this->big_int_lib_->current_witness_ * big_int_lib_->gate_num_words_;
    for (uint32 i = 0; i < 2; i++) {
        for (uint32 j = 0; j < big_int_lib_->gate_num_words_; j++) {
            *a_shares_ptr++ = this->all_witness_shares_[ring_no_][i][k2 + j];
        }
    }

    this->big_int_lib_->current_witness_++;
}

// word* shares contains the words of ALL shares (share_party_1_word_1, share_party_1_word_2, ... ,share_party_n_word_m-1, share_party_n_word_m) for n parties and m words
void CircuitContainer::assertZeroSharedSign(word* a_shares) {
    //std::cout << "assertZeroSharedSign " << this->current_assert_zero_ << std::endl;
    if (this->big_int_lib_->current_assert_zero_ >= big_int_lib_->num_assert_zeros_) {
#ifdef TESTING
        std::cerr << "ERROR: number of assert_zeros is too high" << std::endl;
#endif
        exit(1);
    }

    word *(a_shares_p[3]) = {a_shares, a_shares + big_int_lib_->gate_num_words_, a_shares + 2 * big_int_lib_->gate_num_words_};
    uint32 size_per_party = big_int_lib_->num_assert_zeros_ * big_int_lib_->value_size_;
    uchar *curr_assert_zero_ptr = this->sign_data_->assert_zero_shares_[ring_no_] + (this->big_int_lib_->current_assert_zero_ * big_int_lib_->value_size_);
    // only the first 2 shares of each assert_zero argument need to be hashed
    memcpy(curr_assert_zero_ptr, a_shares_p[0], big_int_lib_->value_size_);
    memcpy(curr_assert_zero_ptr + size_per_party, a_shares_p[1], big_int_lib_->value_size_);

    word a[big_int_lib_->gate_num_words_];
    this->big_int_lib_->Add(a, a_shares_p[0], a_shares_p[1]);
    this->big_int_lib_->Add(a, a, a_shares_p[2]);
    if (!this->big_int_lib_->IsZero(a)) {
#ifdef TESTING
        std::cerr << "ERROR: assertion failed" << std::endl;
        this->big_int_lib_->Print(a, big_int_lib_->value_size_);
#endif
        exit(1);
    }

    this->big_int_lib_->current_assert_zero_++;
}

// word* shares contains the words of ALL shares (share_party_1_word_1, share_party_1_word_2, ... ,share_party_n_word_m-1, share_party_n_word_m) for n parties and m words
void CircuitContainer::assertZeroSharedVerify(word* a_shares) {
    //std::cout << "assertZeroSharedVerify " << this->big_int_lib_->current_assert_zero_ << std::endl;

    word *(a_shares_p[2]) = {a_shares, a_shares + big_int_lib_->gate_num_words_};
    uint32 size_per_party = big_int_lib_->num_assert_zeros_ * big_int_lib_->value_size_;
    uchar *curr_assert_zero_ptr = this->verify_data_->assert_zero_shares_[ring_no_] + (this->big_int_lib_->current_assert_zero_ * big_int_lib_->value_size_);
    word a2[big_int_lib_->gate_num_words_];
    memset(a2, 0, sizeof(a2));

    for (uint32 i = 0; i < this->party_size_; i++) {
        if (i < 2) {
            memcpy(curr_assert_zero_ptr + i * size_per_party, a_shares_p[i], big_int_lib_->value_size_);
            this->big_int_lib_->Sub(a2, a2, a_shares_p[i]);
        } else {
            memcpy(curr_assert_zero_ptr + i * size_per_party, a2, big_int_lib_->value_size_);
        }
    }

    this->big_int_lib_->current_assert_zero_++;
}

void CircuitContainer::bitwiseToBitwiseSharedSign(uint32 from_ring_no, uint32 to_ring_no, word* a_shares, word* b_shares) {
  BigIntLib* to_ring = &this->all_big_int_libs_[to_ring_no];
  BigIntLib* from_ring = &this->all_big_int_libs_[from_ring_no];
  if (to_ring->field_type_ != 3 || from_ring->field_type_ != 3) {
#ifdef TESTING
    std::cerr << "bitwiseToBitwiseSharedSign can be used only for conversion from one bitwise ring to another" << std::endl << std::flush;
#endif
    exit(1);
  }
  word *(a_shares_p[3]) = {a_shares, a_shares + from_ring->gate_num_words_, a_shares + 2 * from_ring->gate_num_words_};
  word *(b_shares_p[3]) = {b_shares, b_shares + to_ring->gate_num_words_, b_shares + 2 * to_ring->gate_num_words_};
  bitwiseToBitwiseDirect(to_ring_no, from_ring_no, b_shares_p[0], a_shares_p[0]);
  bitwiseToBitwiseDirect(to_ring_no, from_ring_no, b_shares_p[1], a_shares_p[1]);
  bitwiseToBitwiseDirect(to_ring_no, from_ring_no, b_shares_p[2], a_shares_p[2]);
}

void CircuitContainer::bitwiseToBitwiseSharedVerify(uint32 from_ring_no, uint32 to_ring_no, word* a_shares, word* b_shares) {
  BigIntLib* to_ring = &this->all_big_int_libs_[to_ring_no];
  BigIntLib* from_ring = &this->all_big_int_libs_[from_ring_no];
  if (to_ring->field_type_ != 3 || from_ring->field_type_ != 3) {
#ifdef TESTING
    std::cerr << "bitwiseToBitwiseSharedVerify can be used only for conversion from one bitwise ring to another" << std::endl << std::flush;
#endif
    exit(1);
  }
  word *(a_shares_p[2]) = {a_shares, a_shares + from_ring->gate_num_words_};
  word *(b_shares_p[2]) = {b_shares, b_shares + to_ring->gate_num_words_};
  bitwiseToBitwiseDirect(to_ring_no, from_ring_no, b_shares_p[0], a_shares_p[0]);
  bitwiseToBitwiseDirect(to_ring_no, from_ring_no, b_shares_p[1], a_shares_p[1]);
}

// word* shares contains the words of ALL shares (share_party_1_word_1, share_party_1_word_2, ... ,share_party_n_word_m-1, share_party_n_word_m) for n parties and m words
void CircuitContainer::addSharedSign(word* a_shares, word* b_shares, word* c_shares) {
  word *(a_shares_p[3]) = {a_shares, a_shares + big_int_lib_->gate_num_words_, a_shares + 2 * big_int_lib_->gate_num_words_};
  word *(b_shares_p[3]) = {b_shares, b_shares + big_int_lib_->gate_num_words_, b_shares + 2 * big_int_lib_->gate_num_words_};
  word *(c_shares_p[3]) = {c_shares, c_shares + big_int_lib_->gate_num_words_, c_shares + 2 * big_int_lib_->gate_num_words_};
  this->big_int_lib_->Add(c_shares_p[0], a_shares_p[0], b_shares_p[0]);
  this->big_int_lib_->Add(c_shares_p[1], a_shares_p[1], b_shares_p[1]);
  this->big_int_lib_->Add(c_shares_p[2], a_shares_p[2], b_shares_p[2]);
}

// word* shares contains the words of ALL shares (share_party_1_word_1, share_party_1_word_2, ... ,share_party_n_word_m-1, share_party_n_word_m) for n parties and m words
void CircuitContainer::addSharedVerify(word* a_shares, word* b_shares, word* c_shares) {
  word *(a_shares_p[2]) = {a_shares, a_shares + big_int_lib_->gate_num_words_};
  word *(b_shares_p[2]) = {b_shares, b_shares + big_int_lib_->gate_num_words_};
  word *(c_shares_p[2]) = {c_shares, c_shares + big_int_lib_->gate_num_words_};
  this->big_int_lib_->Add(c_shares_p[0], a_shares_p[0], b_shares_p[0]);
  this->big_int_lib_->Add(c_shares_p[1], a_shares_p[1], b_shares_p[1]);
}

void CircuitContainer::subSharedSign(word* a_shares, word* b_shares, word* c_shares) {
  word *(a_shares_p[3]) = {a_shares, a_shares + big_int_lib_->gate_num_words_, a_shares + 2 * big_int_lib_->gate_num_words_};
  word *(b_shares_p[3]) = {b_shares, b_shares + big_int_lib_->gate_num_words_, b_shares + 2 * big_int_lib_->gate_num_words_};
  word *(c_shares_p[3]) = {c_shares, c_shares + big_int_lib_->gate_num_words_, c_shares + 2 * big_int_lib_->gate_num_words_};
  this->big_int_lib_->Sub(c_shares_p[0], a_shares_p[0], b_shares_p[0]);
  this->big_int_lib_->Sub(c_shares_p[1], a_shares_p[1], b_shares_p[1]);
  this->big_int_lib_->Sub(c_shares_p[2], a_shares_p[2], b_shares_p[2]);
}

void CircuitContainer::subSharedVerify(word* a_shares, word* b_shares, word* c_shares) {
  word *(a_shares_p[2]) = {a_shares, a_shares + big_int_lib_->gate_num_words_};
  word *(b_shares_p[2]) = {b_shares, b_shares + big_int_lib_->gate_num_words_};
  word *(c_shares_p[2]) = {c_shares, c_shares + big_int_lib_->gate_num_words_};
  this->big_int_lib_->Sub(c_shares_p[0], a_shares_p[0], b_shares_p[0]);
  this->big_int_lib_->Sub(c_shares_p[1], a_shares_p[1], b_shares_p[1]);
}

// word* shares contains the words of ALL shares (share_party_1_word_1, share_party_1_word_2, ... ,share_party_n_word_m-1, share_party_n_word_m) for n parties and m words
void CircuitContainer::copyCSharedSign(word* b, word* c_shares) {
  memcpy(c_shares, b, big_int_lib_->gate_size_);
  memset(c_shares + big_int_lib_->gate_num_words_, 0, 2 * big_int_lib_->gate_size_);
}

// word* shares contains the words of ALL shares (share_party_1_word_1, share_party_1_word_2, ... ,share_party_n_word_m-1, share_party_n_word_m) for n parties and m words
void CircuitContainer::copyCSharedVerify(word* b, word* c_shares) {
  if(this->e_ == 0) {
    memcpy(c_shares, b, big_int_lib_->gate_size_);
    memset(c_shares + big_int_lib_->gate_num_words_, 0, big_int_lib_->gate_size_);
  }
  else if(this->e_ == 1) {
    memset(c_shares, 0, 2 * big_int_lib_->gate_size_);
  }
  else if(this->e_ == 2) {
    memset(c_shares, 0, big_int_lib_->gate_size_);
    memcpy(c_shares + big_int_lib_->gate_num_words_, b, big_int_lib_->gate_size_);
  }
}

void CircuitContainer::shlCSharedSign(word* a_shares, uint32 n, word* b_shares) {
  word *(a_shares_p[3]) = {a_shares, a_shares + big_int_lib_->gate_num_words_, a_shares + 2 * big_int_lib_->gate_num_words_};
  word *(b_shares_p[3]) = {b_shares, b_shares + big_int_lib_->gate_num_words_, b_shares + 2 * big_int_lib_->gate_num_words_};
  this->big_int_lib_->ShlC(b_shares_p[0], a_shares_p[0], n);
  this->big_int_lib_->ShlC(b_shares_p[1], a_shares_p[1], n);
  this->big_int_lib_->ShlC(b_shares_p[2], a_shares_p[2], n);
}

void CircuitContainer::shlCSharedVerify(word* a_shares, uint32 n, word* b_shares) {
  word *(a_shares_p[2]) = {a_shares, a_shares + big_int_lib_->gate_num_words_};
  word *(b_shares_p[2]) = {b_shares, b_shares + big_int_lib_->gate_num_words_};
  this->big_int_lib_->ShlC(b_shares_p[0], a_shares_p[0], n);
  this->big_int_lib_->ShlC(b_shares_p[1], a_shares_p[1], n);
}

void CircuitContainer::shrCSharedSign(word* a_shares, uint32 n, word* b_shares) {
  word *(a_shares_p[3]) = {a_shares, a_shares + big_int_lib_->gate_num_words_, a_shares + 2 * big_int_lib_->gate_num_words_};
  word *(b_shares_p[3]) = {b_shares, b_shares + big_int_lib_->gate_num_words_, b_shares + 2 * big_int_lib_->gate_num_words_};
  this->big_int_lib_->ShrC(b_shares_p[0], a_shares_p[0], n);
  this->big_int_lib_->ShrC(b_shares_p[1], a_shares_p[1], n);
  this->big_int_lib_->ShrC(b_shares_p[2], a_shares_p[2], n);
}

void CircuitContainer::shrCSharedVerify(word* a_shares, uint32 n, word* b_shares) {
  word *(a_shares_p[2]) = {a_shares, a_shares + big_int_lib_->gate_num_words_};
  word *(b_shares_p[2]) = {b_shares, b_shares + big_int_lib_->gate_num_words_};
  this->big_int_lib_->ShrC(b_shares_p[0], a_shares_p[0], n);
  this->big_int_lib_->ShrC(b_shares_p[1], a_shares_p[1], n);
}

// word* shares contains the words of ALL shares (share_party_1_word_1, share_party_1_word_2, ... ,share_party_n_word_m-1, share_party_n_word_m) for n parties and m words
void CircuitContainer::addCSharedSign(word* a_shares, word* b, word* c_shares) {
  this->big_int_lib_->Add(c_shares, a_shares, b);
  memcpy(c_shares + big_int_lib_->gate_num_words_, a_shares + big_int_lib_->gate_num_words_, 2 * big_int_lib_->gate_size_);
}

// word* shares contains the words of ALL shares (share_party_1_word_1, share_party_1_word_2, ... ,share_party_n_word_m-1, share_party_n_word_m) for n parties and m words
void CircuitContainer::addCSharedVerify(word* a_shares, word* b, word* c_shares) {
  if(this->e_ == 0) {
    this->big_int_lib_->Add(c_shares, a_shares, b);
    memcpy(c_shares + big_int_lib_->gate_num_words_, a_shares + big_int_lib_->gate_num_words_, big_int_lib_->gate_size_);
  }
  else if(this->e_ == 1) {
    memcpy(c_shares, a_shares, 2 * big_int_lib_->gate_size_);
  }
  else if(this->e_ == 2) {
    memcpy(c_shares, a_shares, big_int_lib_->gate_size_);
    this->big_int_lib_->Add(c_shares + big_int_lib_->gate_num_words_, a_shares + big_int_lib_->gate_num_words_, b);
  }
}

// word* shares contains the words of ALL shares (share_party_1_word_1, share_party_1_word_2, ... ,share_party_n_word_m-1, share_party_n_word_m) for n parties and m words
void CircuitContainer::mulCSharedSign(word* a_shares, word* b, word* c_shares) {
  word *(a_shares_p[3]) = {a_shares, a_shares + big_int_lib_->gate_num_words_, a_shares + 2 * big_int_lib_->gate_num_words_};
  word *(c_shares_p[3]) = {c_shares, c_shares + big_int_lib_->gate_num_words_, c_shares + 2 * big_int_lib_->gate_num_words_};
  this->big_int_lib_->Mul(c_shares_p[0], a_shares_p[0], b);
  this->big_int_lib_->Mul(c_shares_p[1], a_shares_p[1], b);
  this->big_int_lib_->Mul(c_shares_p[2], a_shares_p[2], b);
}

// word* shares contains the words of ALL shares (share_party_1_word_1, share_party_1_word_2, ... ,share_party_n_word_m-1, share_party_n_word_m) for n parties and m words
void CircuitContainer::mulCSharedVerify(word* a_shares, word* b, word* c_shares) {
  word *(a_shares_p[2]) = {a_shares, a_shares + big_int_lib_->gate_num_words_};
  word *(c_shares_p[2]) = {c_shares, c_shares + big_int_lib_->gate_num_words_};
  this->big_int_lib_->Mul(c_shares_p[0], a_shares_p[0], b);
  this->big_int_lib_->Mul(c_shares_p[1], a_shares_p[1], b);
}

void CircuitContainer::mulDirect(word* c, word* a, word* b) {
  this->big_int_lib_->Mul(c, a, b);
  memcpy(this->intermediate_results_[this->big_int_lib_->current_intermediate_result_++], c, big_int_lib_->gate_size_);
}

// ring_no must correspond to the bitwise ring with WORD_SIZE bits.
// For field_size_bits_ triples (b_i,r_i,s_i) of n-bit values in parallel.
// The ith bits of the n values in inputs (b,r) and outputs (s) form the ith triple of n-bit values (b_i,r_i,s_i).
// It computes s_i = b_i - r_i where s_i, b_i, r_i are bitwise-shared values.
// Uses n - 1 multiplications (which use n - 1 random values) but no intermediate results.
// If compute_carry then one extra multiplication (and random value) is used.
void CircuitContainer::subXorSharedSign(uint32 n, uint32 ring_no, bool compute_carry, word* b_shares, word* r_shares, word* s_shares, word* carry_shares) {
  word (*b)[3] = (word (*)[3]) b_shares; // length in the 1st dimension is n
  word (*r)[3] = (word (*)[3]) r_shares; // length in the 1st dimension is n
  word (*s)[3] = (word (*)[3]) s_shares; // length in the 1st dimension is n
  uint32 c_len = compute_carry ? (n + 1) : n;
  word (*c)[3] = (word (*)[3]) new word[c_len * 3]; // length in the 1st dimension is c_len
  BigIntLib* bw_ring = &this->all_big_int_libs_[ring_no];
  if (bw_ring->field_size_bits_ != WORD_SIZE || bw_ring->field_type_ != 3) {
#ifdef TESTING
    std::cerr << "ERROR" << std::endl;
#endif
    exit(1);
  }

  setRing(ring_no);
  memset(c[0], 0, 3 * sizeof(word));
  for (uint32 k = 0; k < n; k++) {
    word t[3], t2[3], t3[3];
    addSharedSign(c[k], r[k], t);
    addSharedSign(t, b[k], s[k]);
    if (compute_carry || k + 1 < n) {
      addSharedSign(c[k], s[k], t2);
      mulNoIntermediateResultSharedSign(t, t2, t3);
      addSharedSign(t3, c[k], c[k+1]);
    }
  }
  if (compute_carry) {
    memcpy(carry_shares, c[n], 3 * sizeof(word));
  }

  delete[] (word*) c;
}

void CircuitContainer::subXorSharedVerify(uint32 n, uint32 ring_no, bool compute_carry, word* b_shares, word* r_shares, word* s_shares, word* carry_shares) {
  word (*b)[3] = (word (*)[3]) b_shares; // length in the 1st dimension is n
  word (*r)[3] = (word (*)[3]) r_shares; // length in the 1st dimension is n
  word (*s)[3] = (word (*)[3]) s_shares; // length in the 1st dimension is n
  uint32 c_len = compute_carry ? (n + 1) : n;
  word (*c)[3] = (word (*)[3]) new word[c_len * 3]; // length in the 1st dimension is c_len
  BigIntLib* bw_ring = &this->all_big_int_libs_[ring_no];
  if (bw_ring->field_size_bits_ != WORD_SIZE || bw_ring->field_type_ != 3) {
#ifdef TESTING
    std::cerr << "ERROR" << std::endl;
#endif
    exit(1);
  }

  setRing(ring_no);
  memset(c[0], 0, 3 * sizeof(word));
  for (uint32 k = 0; k < n; k++) {
    word t[3], t2[3], t3[3];
    addSharedVerify(c[k], r[k], t);
    addSharedVerify(t, b[k], s[k]);
    if (compute_carry || k + 1 < n) {
      addSharedVerify(c[k], s[k], t2);
      mulSharedVerify(t, t2, t3);
      addSharedVerify(t3, c[k], c[k+1]);
    }
  }
  if (compute_carry) {
    memcpy(carry_shares, c[n], 2 * sizeof(word));
  }

  delete[] (word*) c;
}

// Do the subtraction modulo the modulus of the ring modulus_ring_no rather than modulo 2^n.
// If modulus is a power of two then uses n - 1 multiplications (which use n - 1 random values) but no intermediate results.
// If modulus is not a power of two then uses 2n - 1 multiplications (which use 2n - 1 random values) but no intermediate results.
void CircuitContainer::subXorModSharedSign(uint32 n, uint32 ring_no, uint32 modulus_ring_no, word* b_shares, word* r_shares, word* s_shares) {
  BigIntLib* modulus_ring = &this->all_big_int_libs_[modulus_ring_no];
  if (modulus_ring->field_type_ == 2) { // modulo 2^n
    subXorSharedSign(n, ring_no, false, b_shares, r_shares, s_shares, NULL);
  } else {
    if (modulus_ring->minus_modulo_ == NULL || modulus_ring->minus_modulo_[0] == 0) {
#ifdef TESTING
      std::cerr << "ERROR: subXorSharedSignMod not supported for this ring" << std::endl << std::flush;
#endif
      exit(1);
    }
    word carry_shares[3];
    word s1_shares[n][3];
    subXorSharedSign(n, ring_no, true, b_shares, r_shares, (word*)s1_shares, carry_shares);
    // if carry is 1 we need to add the modulus but instead we subtract 2^n - modulus
    word to_subtract[n][3];
    for (uint32 i = 0; i < n; i++) {
      for (uint32 j = 0; j < 3; j++) {
        to_subtract[i][j] = carry_shares[j] & modulus_ring->minus_modulo_bits_[i];
      }
    }
    subXorSharedSign(n, ring_no, false, (word*)s1_shares, (word*)to_subtract, s_shares, NULL);
  }
}

// Do the subtraction modulo the modulus of the ring modulus_ring_no rather than modulo 2^n.
void CircuitContainer::subXorModSharedVerify(uint32 n, uint32 ring_no, uint32 modulus_ring_no, word* b_shares, word* r_shares, word* s_shares) {
  BigIntLib* modulus_ring = &this->all_big_int_libs_[modulus_ring_no];
  if (modulus_ring->field_type_ == 2) { // modulo 2^n
    subXorSharedVerify(n, ring_no, false, b_shares, r_shares, s_shares, NULL);
  } else {
    if (modulus_ring->minus_modulo_ == NULL || modulus_ring->minus_modulo_[0] == 0) {
#ifdef TESTING
      std::cerr << "ERROR: subXorSharedSignMod not supported for this ring" << std::endl << std::flush;
#endif
      exit(1);
    }
    word carry_shares[3];
    word s1_shares[n][3];
    subXorSharedVerify(n, ring_no, true, b_shares, r_shares, (word*)s1_shares, carry_shares);
    // if carry is 1 we need to add the modulus but instead we subtract 2^n - modulus
    word to_subtract[n][3];
    for (uint32 i = 0; i < n; i++) {
      for (uint32 j = 0; j < 2; j++) {
        to_subtract[i][j] = carry_shares[j] & modulus_ring->minus_modulo_bits_[i];
      }
    }
    subXorSharedVerify(n, ring_no, false, (word*)s1_shares, (word*)to_subtract, s_shares, NULL);
  }
}

void CircuitContainer::declassify3SharedSign(uint32 ring_no, word* a_shares, word* b_shares) {
  setRing(ring_no);
  if (this->iteration_ == 1 && this->big_int_lib_->current_declassify3_ >= big_int_lib_->num_declassify3s_) {
#ifdef TESTING
      std::cerr << "ERROR: number of declassify3s is too high" << std::endl;
#endif
      exit(1);
  }

  word *(a_shares_p[3]) = {a_shares, a_shares + big_int_lib_->gate_num_words_, a_shares + 2 * big_int_lib_->gate_num_words_};
  word *b_shares_p_2 = b_shares + 2 * big_int_lib_->gate_num_words_;
  memcpy(this->sign_data_->declassify3_1_[ring_no] + big_int_lib_->current_declassify3_ * big_int_lib_->gate_size_, a_shares_p[0], big_int_lib_->gate_size_);
  memcpy(this->sign_data_->declassify3_2_[ring_no] + big_int_lib_->current_declassify3_ * big_int_lib_->gate_size_, a_shares_p[1], big_int_lib_->gate_size_);
  this->big_int_lib_->Add(b_shares_p_2, a_shares_p[0], a_shares_p[1]);
  this->big_int_lib_->Add(b_shares_p_2, b_shares_p_2, a_shares_p[2]);
  memset(b_shares, 0, 2 * big_int_lib_->gate_size_);
  big_int_lib_->current_declassify3_++;
}

void CircuitContainer::declassify3SharedVerify(uint32 ring_no, word* a_shares, word* b_shares) {
  setRing(ring_no);
  word *(a_shares_p[2]) = {a_shares, a_shares + big_int_lib_->gate_num_words_};
  word *(b_shares_p[2]) = {b_shares, b_shares + big_int_lib_->gate_num_words_};

  if (this->e_ == 0) {
    memcpy(this->verify_data_->declassify3_1_[ring_no] + big_int_lib_->current_declassify3_ * big_int_lib_->gate_size_, a_shares_p[0], big_int_lib_->gate_size_);
    memcpy(this->verify_data_->declassify3_2_[ring_no] + big_int_lib_->current_declassify3_ * big_int_lib_->gate_size_, a_shares_p[1], big_int_lib_->gate_size_);
    memset(b_shares, 0, 2 * big_int_lib_->gate_size_);
  } else if (this->e_ == 1) {
    memcpy(this->verify_data_->declassify3_2_[ring_no] + big_int_lib_->current_declassify3_ * big_int_lib_->gate_size_, a_shares_p[0], big_int_lib_->gate_size_);
    memset(b_shares_p[0], 0, big_int_lib_->gate_size_);
    memcpy(b_shares_p[1], this->proof_->zs_[this->iteration_]->declassify3_[ring_no_] + big_int_lib_->current_declassify3_ * big_int_lib_->gate_size_, big_int_lib_->gate_size_);
    memcpy(this->verify_data_->declassify3_1_[ring_no] + big_int_lib_->current_declassify3_ * big_int_lib_->gate_size_, b_shares_p[1], big_int_lib_->gate_size_);
    this->big_int_lib_->Add(b_shares_p[1], b_shares_p[1], a_shares_p[0]);
    this->big_int_lib_->Add(b_shares_p[1], b_shares_p[1], a_shares_p[1]);
  } else if (this->e_ == 2) {
    memcpy(this->verify_data_->declassify3_1_[ring_no] + big_int_lib_->current_declassify3_ * big_int_lib_->gate_size_, a_shares_p[1], big_int_lib_->gate_size_);
    memset(b_shares_p[1], 0, big_int_lib_->gate_size_);
    memcpy(b_shares_p[0], this->proof_->zs_[this->iteration_]->declassify3_[ring_no_] + big_int_lib_->current_declassify3_ * big_int_lib_->gate_size_, big_int_lib_->gate_size_);
    memcpy(this->verify_data_->declassify3_2_[ring_no] + big_int_lib_->current_declassify3_ * big_int_lib_->gate_size_, b_shares_p[0], big_int_lib_->gate_size_);
    this->big_int_lib_->Add(b_shares_p[0], b_shares_p[0], a_shares_p[0]);
    this->big_int_lib_->Add(b_shares_p[0], b_shares_p[0], a_shares_p[1]);
  }
  big_int_lib_->current_declassify3_++;
}

void CircuitContainer::concatVecDirect(uint32 ring_no, uint32 n1, uint32 n2, word* c, word* a, word* b) {
  BigIntLib* ring = &this->all_big_int_libs_[ring_no];
  uint32 a_size = n1 * ring->gate_size_;
  uint32 a_num_words = n1 * ring->gate_num_words_;
  uint32 b_size = n2 * ring->gate_size_;
  memcpy(c, a, a_size);
  memcpy(c + a_num_words, b, b_size);
}

void CircuitContainer::concatVecShared(uint32 ring_no, uint32 n1, uint32 n2, word* a_shares, word* b_shares, word* c_shares) {
  BigIntLib* ring = &this->all_big_int_libs_[ring_no];
  uint32 a_num_words = n1 * 3 * ring->gate_num_words_;
  uint32 b_num_words = n2 * 3 * ring->gate_num_words_;
  //uint32 a_size = n1 * 3 * ring->gate_size_;
  //uint32 b_size = n2 * 3 * ring->gate_size_;
  //memcpy(c_shares, a_shares, a_size);
  //memcpy(c_shares + a_num_words, b_shares, b_size);
  // memcpy is slower than the for loops
  for (uint32 i = 0; i < a_num_words; i++)
    c_shares[i] = a_shares[i];
  word* c_shares2 = c_shares + a_num_words;
  for (uint32 i = 0; i < b_num_words; i++)
    c_shares2[i] = b_shares[i];
}

// a contains n_from elements
void CircuitContainer::bitwiseVecToBitwiseVecDirect(uint32 from_ring_no, uint32 to_ring_no, uint32 n_from, word* b, word* a) {
  BigIntLib* to_ring = &this->all_big_int_libs_[to_ring_no];
  BigIntLib* from_ring = &this->all_big_int_libs_[from_ring_no];
  if (to_ring->field_type_ != 3 || from_ring->field_type_ != 3) {
#ifdef TESTING
    std::cerr << "bitwiseVecToBitwiseVec can be used only for converting from one bitwise ring to another" << std::endl << std::flush;
#endif
    exit(1);
  }
  if (to_ring->gate_num_words_ > 1 || from_ring->gate_num_words_ > 1) {
#ifdef TESTING
    std::cerr << "bitwiseVecToBitwiseVec can be used only for bitwise rings with at most WORD_SIZE bits" << std::endl << std::flush;
#endif
    exit(1);
  }
  uint32 from_bits = from_ring->field_size_bits_;
  uint32 to_bits = to_ring->field_size_bits_;
  uint32 n_to = ((uint64)n_from * from_bits + to_bits - 1) / to_bits;
  if (from_bits % to_bits == 0) {
    uint32 block_len = from_bits / to_bits;
    uint32 to_i = 0;
    word word_mask = to_ring->msb_word_mask_;
    for (uint32 i = 0; i < n_from; i++) {
      word t = a[i];
      to_i += block_len;
      for (uint32 j = 0; j < block_len; j++) {
        b[--to_i] = t & word_mask;
        t >>= to_bits;
      }
      to_i += block_len;
    }
  } else if (to_bits % from_bits == 0) {
    uint32 block_len = to_bits / from_bits;
    uint32 from_i = 0;
    for (uint32 i = 0; i + 1 < n_to; i++) {
      word t = a[from_i++];
      for (uint32 j = 1; j < block_len; j++) {
        t = (t << from_bits) ^ a[from_i++];
      }
      b[i] = t;
    }
    word t = a[from_i++];
    for (uint32 j = 1; j < block_len; j++) {
      t <<= from_bits;
      if (from_i < n_from)
        t ^= a[from_i++];
    }
    b[n_to - 1] = t;
  } else {
#ifdef TESTING
    std::cerr << "bitwiseVecToBitwiseVec can be used only if the number of bits of one bitwise ring is a multiple of the number of bits of the other" << std::endl << std::flush;
#endif
    exit(1);
  }
}

void CircuitContainer::bitwiseVecToBitwiseVecSharedSign(uint32 from_ring_no, uint32 to_ring_no, uint32 n_from, word* a_shares, word* b_shares) {
  word (*a_shares_p)[3] = (word (*)[3]) a_shares;
  word (*b_shares_p)[3] = (word (*)[3]) b_shares;
  BigIntLib* to_ring = &this->all_big_int_libs_[to_ring_no];
  BigIntLib* from_ring = &this->all_big_int_libs_[from_ring_no];
  if (to_ring->field_type_ != 3 || from_ring->field_type_ != 3) {
#ifdef TESTING
    std::cerr << "bitwiseVecToBitwiseVec can be used only for converting from one bitwise ring to another" << std::endl << std::flush;
#endif
    exit(1);
  }
  if (to_ring->gate_num_words_ > 1 || from_ring->gate_num_words_ > 1) {
#ifdef TESTING
    std::cerr << "bitwiseVecToBitwiseVec can be used only for bitwise rings with at most WORD_SIZE bits" << std::endl << std::flush;
#endif
    exit(1);
  }
  uint32 from_bits = from_ring->field_size_bits_;
  uint32 to_bits = to_ring->field_size_bits_;
  uint32 n_to = ((uint64)n_from * from_bits + to_bits - 1) / to_bits;
  if (from_bits % to_bits == 0) {
    uint32 block_len = from_bits / to_bits;
    word word_mask = to_ring->msb_word_mask_;

    uint32 to_i = 0;
    for (uint32 i = 0; i < n_from; i++) {
      word t = a_shares_p[i][0];
      to_i += block_len;
      for (uint32 j = 0; j < block_len; j++) {
        b_shares_p[--to_i][0] = t & word_mask;
        t >>= to_bits;
      }
      to_i += block_len;
    }

    to_i = 0;
    for (uint32 i = 0; i < n_from; i++) {
      word t = a_shares_p[i][1];
      to_i += block_len;
      for (uint32 j = 0; j < block_len; j++) {
        b_shares_p[--to_i][1] = t & word_mask;
        t >>= to_bits;
      }
      to_i += block_len;
    }

    to_i = 0;
    for (uint32 i = 0; i < n_from; i++) {
      word t = a_shares_p[i][2];
      to_i += block_len;
      for (uint32 j = 0; j < block_len; j++) {
        b_shares_p[--to_i][2] = t & word_mask;
        t >>= to_bits;
      }
      to_i += block_len;
    }
  } else if (to_bits % from_bits == 0) {
    uint32 block_len = to_bits / from_bits;

    uint32 from_i = 0;
    for (uint32 i = 0; i + 1 < n_to; i++) {
      word t = a_shares_p[from_i++][0];
      for (uint32 j = 1; j < block_len; j++) {
        t = (t << from_bits) ^ a_shares_p[from_i++][0];
      }
      b_shares_p[i][0] = t;
    }
    word t = a_shares_p[from_i++][0];
    for (uint32 j = 1; j < block_len; j++) {
      t <<= from_bits;
      if (from_i < n_from)
        t ^= a_shares_p[from_i++][0];
    }
    b_shares_p[n_to - 1][0] = t;

    from_i = 0;
    for (uint32 i = 0; i + 1 < n_to; i++) {
      word t = a_shares_p[from_i++][1];
      for (uint32 j = 1; j < block_len; j++) {
        t = (t << from_bits) ^ a_shares_p[from_i++][1];
      }
      b_shares_p[i][1] = t;
    }
    t = a_shares_p[from_i++][1];
    for (uint32 j = 1; j < block_len; j++) {
      t <<= from_bits;
      if (from_i < n_from)
        t ^= a_shares_p[from_i++][1];
    }
    b_shares_p[n_to - 1][1] = t;

    from_i = 0;
    for (uint32 i = 0; i + 1 < n_to; i++) {
      word t = a_shares_p[from_i++][2];
      for (uint32 j = 1; j < block_len; j++) {
        t = (t << from_bits) ^ a_shares_p[from_i++][2];
      }
      b_shares_p[i][2] = t;
    }
    t = a_shares_p[from_i++][2];
    for (uint32 j = 1; j < block_len; j++) {
      t <<= from_bits;
      if (from_i < n_from)
        t ^= a_shares_p[from_i++][2];
    }
    b_shares_p[n_to - 1][2] = t;
  } else {
#ifdef TESTING
    std::cerr << "bitwiseVecToBitwiseVec can be used only if the number of bits of one bitwise ring is a multiple of the number of bits of the other" << std::endl << std::flush;
#endif
    exit(1);
  }
}

void CircuitContainer::bitwiseVecToBitwiseVecSharedVerify(uint32 from_ring_no, uint32 to_ring_no, uint32 n_from, word* a_shares, word* b_shares) {
  word (*a_shares_p)[3] = (word (*)[3]) a_shares;
  word (*b_shares_p)[3] = (word (*)[3]) b_shares;
  BigIntLib* to_ring = &this->all_big_int_libs_[to_ring_no];
  BigIntLib* from_ring = &this->all_big_int_libs_[from_ring_no];
  if (to_ring->field_type_ != 3 || from_ring->field_type_ != 3) {
#ifdef TESTING
    std::cerr << "bitwiseVecToBitwiseVec can be used only for converting from one bitwise ring to another" << std::endl << std::flush;
#endif
    exit(1);
  }
  if (to_ring->gate_num_words_ > 1 || from_ring->gate_num_words_ > 1) {
#ifdef TESTING
    std::cerr << "bitwiseVecToBitwiseVec can be used only for bitwise rings with at most WORD_SIZE bits" << std::endl << std::flush;
#endif
    exit(1);
  }
  uint32 from_bits = from_ring->field_size_bits_;
  uint32 to_bits = to_ring->field_size_bits_;
  uint32 n_to = ((uint64)n_from * from_bits + to_bits - 1) / to_bits;
  if (from_bits % to_bits == 0) {
    uint32 block_len = from_bits / to_bits;

    word word_mask = to_ring->msb_word_mask_;
    uint32 to_i = 0;
    for (uint32 i = 0; i < n_from; i++) {
      word t = a_shares_p[i][0];
      to_i += block_len;
      for (uint32 j = 0; j < block_len; j++) {
        b_shares_p[--to_i][0] = t & word_mask;
        t >>= to_bits;
      }
      to_i += block_len;
    }

    to_i = 0;
    for (uint32 i = 0; i < n_from; i++) {
      word t = a_shares_p[i][1];
      to_i += block_len;
      for (uint32 j = 0; j < block_len; j++) {
        b_shares_p[--to_i][1] = t & word_mask;
        t >>= to_bits;
      }
      to_i += block_len;
    }
  } else if (to_bits % from_bits == 0) {
    uint32 block_len = to_bits / from_bits;

    uint32 from_i = 0;
    for (uint32 i = 0; i + 1 < n_to; i++) {
      word t = a_shares_p[from_i++][0];
      for (uint32 j = 1; j < block_len; j++) {
        t = (t << from_bits) ^ a_shares_p[from_i++][0];
      }
      b_shares_p[i][0] = t;
    }
    word t = a_shares_p[from_i++][0];
    for (uint32 j = 1; j < block_len; j++) {
      t <<= from_bits;
      if (from_i < n_from)
        t ^= a_shares_p[from_i++][0];
    }
    b_shares_p[n_to - 1][0] = t;

    from_i = 0;
    for (uint32 i = 0; i + 1 < n_to; i++) {
      word t = a_shares_p[from_i++][1];
      for (uint32 j = 1; j < block_len; j++) {
        t = (t << from_bits) ^ a_shares_p[from_i++][1];
      }
      b_shares_p[i][1] = t;
    }
    t = a_shares_p[from_i++][1];
    for (uint32 j = 1; j < block_len; j++) {
      t <<= from_bits;
      if (from_i < n_from)
        t ^= a_shares_p[from_i++][1];
    }
    b_shares_p[n_to - 1][1] = t;
  } else {
#ifdef TESTING
    std::cerr << "bitwiseVecToBitwiseVec can be used only if the number of bits of one bitwise ring is a multiple of the number of bits of the other" << std::endl << std::flush;
#endif
    exit(1);
  }
}

// a contains nr elements of nc bits each
// b will contain nc elements of nr bits each and is the transpose of the bit matrix a
void CircuitContainer::bitwiseMatrixTransposeDirect(uint32 from_ring_no, uint32 to_ring_no, uint32 nr, uint32 nc, word* b, word* a) {
  BigIntLib* to_ring = &this->all_big_int_libs_[to_ring_no];
  BigIntLib* from_ring = &this->all_big_int_libs_[from_ring_no];
  if (to_ring->field_type_ != 3 || from_ring->field_type_ != 3) {
#ifdef TESTING
    std::cerr << "bitwiseMatrixTranspose can be used only for transposing from one bitwise ring to another" << std::endl << std::flush;
#endif
    exit(1);
  }
  if (to_ring->gate_num_words_ > 1 || from_ring->gate_num_words_ > 1) {
#ifdef TESTING
    std::cerr << "bitwiseMatrixTranspose can be used only for bitwise rings with at most WORD_SIZE bits" << std::endl << std::flush;
#endif
    exit(1);
  }
  if (nr > to_ring->field_size_bits_ || nc > from_ring->field_size_bits_) {
#ifdef TESTING
    std::cerr << "bitwiseMatrixTranspose: bit matrix does not fit into the input or output ring" << std::endl << std::flush;
#endif
    exit(1);
  }

  for (uint32 i = 0; i < nc; i++) {
    word t = 0;
    for (uint32 j = 0; j < nr; j++) {
      t ^= ((a[j] >> i) & 1) << j;
    }
    b[i] = t;
  }
}

void CircuitContainer::bitwiseMatrixTransposeSharedSign(uint32 from_ring_no, uint32 to_ring_no, uint32 nr, uint32 nc, word* a_shares, word* b_shares) {
  BigIntLib* to_ring = &this->all_big_int_libs_[to_ring_no];
  BigIntLib* from_ring = &this->all_big_int_libs_[from_ring_no];
  if (to_ring->field_type_ != 3 || from_ring->field_type_ != 3) {
#ifdef TESTING
    std::cerr << "bitwiseMatrixTranspose can be used only for transposing from one bitwise ring to another" << std::endl << std::flush;
#endif
    exit(1);
  }
  if (to_ring->gate_num_words_ > 1 || from_ring->gate_num_words_ > 1) {
#ifdef TESTING
    std::cerr << "bitwiseMatrixTranspose can be used only for bitwise rings with at most WORD_SIZE bits" << std::endl << std::flush;
#endif
    exit(1);
  }
  if (nr > to_ring->field_size_bits_ || nc > from_ring->field_size_bits_) {
#ifdef TESTING
    std::cerr << "bitwiseMatrixTranspose: bit matrix does not fit into the input or output ring" << std::endl << std::flush;
#endif
    exit(1);
  }
  word (*a_shares_p)[3] = (word (*)[3]) a_shares;
  word (*b_shares_p)[3] = (word (*)[3]) b_shares;

  for (uint32 i = 0; i < nc; i++) {
    word t = 0;
    for (uint32 j = 0; j < nr; j++) {
      t ^= ((a_shares_p[j][0] >> i) & 1) << j;
    }
    b_shares_p[i][0] = t;
  }

  for (uint32 i = 0; i < nc; i++) {
    word t = 0;
    for (uint32 j = 0; j < nr; j++) {
      t ^= ((a_shares_p[j][1] >> i) & 1) << j;
    }
    b_shares_p[i][1] = t;
  }

  for (uint32 i = 0; i < nc; i++) {
    word t = 0;
    for (uint32 j = 0; j < nr; j++) {
      t ^= ((a_shares_p[j][2] >> i) & 1) << j;
    }
    b_shares_p[i][2] = t;
  }
}

void CircuitContainer::bitwiseMatrixTransposeSharedVerify(uint32 from_ring_no, uint32 to_ring_no, uint32 nr, uint32 nc, word* a_shares, word* b_shares) {
  BigIntLib* to_ring = &this->all_big_int_libs_[to_ring_no];
  BigIntLib* from_ring = &this->all_big_int_libs_[from_ring_no];
  if (to_ring->field_type_ != 3 || from_ring->field_type_ != 3) {
#ifdef TESTING
    std::cerr << "bitwiseMatrixTranspose can be used only for transposing from one bitwise ring to another" << std::endl << std::flush;
#endif
    exit(1);
  }
  if (to_ring->gate_num_words_ > 1 || from_ring->gate_num_words_ > 1) {
#ifdef TESTING
    std::cerr << "bitwiseMatrixTranspose can be used only for bitwise rings with at most WORD_SIZE bits" << std::endl << std::flush;
#endif
    exit(1);
  }
  if (nr > to_ring->field_size_bits_ || nc > from_ring->field_size_bits_) {
#ifdef TESTING
    std::cerr << "bitwiseMatrixTranspose: bit matrix does not fit into the input or output ring" << std::endl << std::flush;
#endif
    exit(1);
  }
  word (*a_shares_p)[3] = (word (*)[3]) a_shares;
  word (*b_shares_p)[3] = (word (*)[3]) b_shares;

  for (uint32 i = 0; i < nc; i++) {
    word t = 0;
    for (uint32 j = 0; j < nr; j++) {
      t ^= ((a_shares_p[j][0] >> i) & 1) << j;
    }
    b_shares_p[i][0] = t;
  }

  for (uint32 i = 0; i < nc; i++) {
    word t = 0;
    for (uint32 j = 0; j < nr; j++) {
      t ^= ((a_shares_p[j][1] >> i) & 1) << j;
    }
    b_shares_p[i][1] = t;
  }
}

// For n values in parallel.
// Uses 2 * (k * bw_ring->field_size_bits_ - 1) multiplications
// (which use the same amount of random values but no intermediate results)
// in the bitwise WORD_SIZE-bit ring
// where k is 1 if the additive modulus is a power of two and 2 otherwise.
// Also uses n random values in ad_ring and n declassify3's in bw_ring.
void CircuitContainer::bitwiseToAdditiveShared(bool verify, uint32 n, uint32 bw_ring_no, uint32 ad_ring_no, word* bw_shares, word* ad_shares) {
  // verify: true = verify, false = prove
  uint32 party_size = verify ? 2 : 3;
  BigIntLib* bw_ring = &this->all_big_int_libs_[bw_ring_no];
  BigIntLib* ad_ring = &this->all_big_int_libs_[ad_ring_no];
  if (bw_ring->field_size_bits_ != ad_ring->field_size_bits_ || bw_ring->field_type_ != 3 || ad_ring->field_type_ != 2 && ad_ring->field_type_ != 0 || n > WORD_SIZE) {
#ifdef TESTING
    std::cerr << "ERROR" << std::endl;
#endif
    exit(1);
  }
  word *(bw_shares_p[n][3]);
  word *ptr = bw_shares;
  for (uint32 i = 0; i < n; i++) {
    for (uint32 j = 0; j < 3; j++) {
      bw_shares_p[i][j] = ptr;
      ptr += bw_ring->gate_num_words_;
    }
  }
  word *(ad_shares_p[n][3]);
  ptr = ad_shares;
  for (uint32 i = 0; i < n; i++) {
    for (uint32 j = 0; j < 3; j++) {
      ad_shares_p[i][j] = ptr;
      ptr += ad_ring->gate_num_words_;
    }
  }

  uint32 r_party = 0, s_party = 1;
  if (verify) {
    if (this->e_ == 1) {
      r_party = 2;
      s_party = 0;
    } else if (this->e_ == 2) {
      r_party = 1;
      s_party = 2;
    }
  }
  uint32 gate_random_pointer = ad_ring->current_random_gate_ * ad_ring->gate_num_words_;
  word r_shares[n][ad_ring->gate_num_words_]; // only shares of 1st party, the rest are 0
  if (r_party != 2)
    memcpy(r_shares, all_random_numbers_[ad_ring_no][r_party] + gate_random_pointer, n * ad_ring->gate_size_);
  word s_shares[n][ad_ring->gate_num_words_]; // only shares of 2nd party, the rest are 0
  if (s_party != 2)
    memcpy(s_shares, all_random_numbers_[ad_ring_no][s_party] + gate_random_pointer, n * ad_ring->gate_size_);
  ad_ring->current_random_gate_ += n;

  word (*v)[3] = (word (*)[3]) new word[bw_ring->field_size_bits_ * 3]; // length in the 1st dimension is bw_ring->field_size_bits_
  word (*r)[3] = (word (*)[3]) new word[bw_ring->field_size_bits_ * 3];
  word (*s)[3] = (word (*)[3]) new word[bw_ring->field_size_bits_ * 3];
  word (*v_minus_r)[3] = (word (*)[3]) new word[bw_ring->field_size_bits_ * 3];
  word (*v_minus_r_minus_s)[3] = (word (*)[3]) new word[bw_ring->field_size_bits_ * 3];
  memset(v, 0, bw_ring->field_size_bits_ * 3 * sizeof(word));
  memset(r, 0, bw_ring->field_size_bits_ * 3 * sizeof(word));
  memset(s, 0, bw_ring->field_size_bits_ * 3 * sizeof(word));

  uint32 word_size_mask = WORD_SIZE - 1;
  uint32 word_size_shift = 0;
  while ((1 << word_size_shift) < WORD_SIZE)
    word_size_shift++;

  for (uint32 i = 0; i < bw_ring->field_size_bits_; i++) {
    uint32 i1 = i >> word_size_shift;
    uint32 i2 = i & word_size_mask;
    for (uint32 j = 0; j < party_size; j++) {
      word t = 0;
      for (uint32 k = 0; k < n; k++) {
        t ^= ((bw_shares_p[k][j][i1] >> i2) & 1) << k;
      }
      v[i][j] = t;
    }
    word tr = 0, ts = 0;
    for (uint32 k = 0; k < n; k++) {
      tr ^= ((r_shares[k][i1] >> i2) & 1) << k;
      ts ^= ((s_shares[k][i1] >> i2) & 1) << k;
    }
    r[i][r_party] = tr;
    s[i][s_party] = ts;
  }

  uint32 bw_word_ring_no = this->bw_word_ring_no_;
  if (verify) {
    subXorModSharedVerify(bw_ring->field_size_bits_, bw_word_ring_no, ad_ring_no, (word*)v, (word*)r, (word*)v_minus_r);
    subXorModSharedVerify(bw_ring->field_size_bits_, bw_word_ring_no, ad_ring_no, (word*)v_minus_r, (word*)s, (word*)v_minus_r_minus_s);
  } else {
    subXorModSharedSign(bw_ring->field_size_bits_, bw_word_ring_no, ad_ring_no, (word*)v, (word*)r, (word*)v_minus_r);
    subXorModSharedSign(bw_ring->field_size_bits_, bw_word_ring_no, ad_ring_no, (word*)v_minus_r, (word*)s, (word*)v_minus_r_minus_s);
  }

  word v_minus_r_minus_s_shares_p[n][3][bw_ring->gate_num_words_];
  memset(v_minus_r_minus_s_shares_p, 0, n * 3 * bw_ring->gate_size_);
  for (uint32 k = 0; k < n; k++) {
    for (uint32 j = 0; j < party_size; j++) {
      uint32 i = 0;
      for (uint32 i1 = 0; i1 < bw_ring->gate_num_words_; i1++) {
        word t = 0;
        for (uint32 i2 = 0; i2 < WORD_SIZE && i < bw_ring->field_size_bits_; i2++, i++) {
          t ^= ((v_minus_r_minus_s[i][j] >> k) & 1) << i2;
        }
        v_minus_r_minus_s_shares_p[k][j][i1] = t;
      }
    }
  }

  if (verify) {
    for (uint32 k = 0; k < n; k++) {
      declassify3SharedVerify(bw_ring_no, (word*)v_minus_r_minus_s_shares_p[k], (word*)ad_shares_p[k][0]); // here we assume that [k][0], [k][1], and [k][2] are consecutive in memory
      if (r_party != 2)
        memcpy(ad_shares_p[k][r_party], r_shares[k], ad_ring->gate_size_);
      if (s_party != 2)
        memcpy(ad_shares_p[k][s_party], s_shares[k], ad_ring->gate_size_);
    }
  } else {
    for (uint32 k = 0; k < n; k++) {
      declassify3SharedSign(bw_ring_no, (word*)v_minus_r_minus_s_shares_p[k], (word*)ad_shares_p[k][0]); // here we assume that [k][0] and [k][1] are consecutive in memory
      memcpy(ad_shares_p[k][0], r_shares[k], ad_ring->gate_size_);
      memcpy(ad_shares_p[k][1], s_shares[k], ad_ring->gate_size_);
    }
  }

  delete[] (word*) v;
  delete[] (word*) r;
  delete[] (word*) s;
  delete[] (word*) v_minus_r;
  delete[] (word*) v_minus_r_minus_s;
}

void CircuitContainer::performAssertEqBwAds(bool verify) {
  for(uint32 i = 0; i < this->num_rings_; i++) {
    BigIntLib* ad_ring = &this->all_big_int_libs_[i];
    if (ad_ring->current_assert_eq_bw_ad_ > 0) {
      BigIntLib* bw_ring = &this->all_big_int_libs_[ad_ring->bitwise_ring_no_];
      if (bw_ring->field_size_bits_ != ad_ring->field_size_bits_ || bw_ring->field_type_ != 3 || ad_ring->field_type_ != 2 && ad_ring->field_type_ != 0) {
#ifdef TESTING
        std::cerr << "ERROR: First ring of assert_eq_bw_ad must be a bitwise ring and the second ring an additive ring or prime field with the same number of bits" << std::endl;
#endif
        exit(1);
      }
      word* bw_shares_ptr = this->assert_eq_bw_shares_[i];
      word* ad_shares_ptr = this->assert_eq_ad_shares_[i];
      word ad_shares[MIN(ad_ring->current_assert_eq_bw_ad_, (uint32)WORD_SIZE) * 3 * ad_ring->gate_num_words_];
      word diff_shares[3 * ad_ring->gate_num_words_];
      while (ad_ring->current_assert_eq_bw_ad_ > 0) {
        uint32 n = MIN(ad_ring->current_assert_eq_bw_ad_, (uint32)WORD_SIZE);
        bitwiseToAdditiveShared(verify, n, ad_ring->bitwise_ring_no_, i, bw_shares_ptr, ad_shares);
        setRing(i);
        for (uint32 i = 0; i < n; i++) {
          uint32 k = i * 3 * ad_ring->gate_num_words_;
          (this->*sub_shared_function_)(ad_shares + k, ad_shares_ptr + k, diff_shares);
          (this->*assert_zero_shared_function_)(diff_shares);
        }
        ad_ring->current_assert_eq_bw_ad_ -= n;
        bw_shares_ptr += n * 3 * bw_ring->gate_num_words_;
        ad_shares_ptr += n * 3 * ad_ring->gate_num_words_;
      }
    }
  }
}

void CircuitContainer::assertEqBwAdSharedSign(uint32 bw_ring_no, uint32 ad_ring_no, word* bw_shares, word* ad_shares) {
  setRing(ad_ring_no);

  if (this->iteration_ == 1 && this->big_int_lib_->current_assert_eq_bw_ad_ >= big_int_lib_->num_ring_conversions_) {
#ifdef TESTING
    std::cerr << "ERROR: number of ring conversions is too high" << std::endl;
#endif
    exit(1);
  }

  word *bw_shares_ptr = bw_shares;
  word *ad_shares_ptr = ad_shares;
  uint32 k2 = this->big_int_lib_->current_assert_eq_bw_ad_ * this->party_size_ * big_int_lib_->gate_num_words_;
  for (uint32 i = 0; i < 3; i++) {
    uint32 k3 = k2 + i * big_int_lib_->gate_num_words_;
    for (uint32 j = 0; j < big_int_lib_->gate_num_words_; j++) {
      this->assert_eq_bw_shares_[ad_ring_no][k3 + j] = *bw_shares_ptr++;
      this->assert_eq_ad_shares_[ad_ring_no][k3 + j] = *ad_shares_ptr++;
    }
  }

  this->big_int_lib_->current_assert_eq_bw_ad_++;
}

void CircuitContainer::assertEqBwAdSharedVerify(uint32 bw_ring_no, uint32 ad_ring_no, word* bw_shares, word* ad_shares) {
  setRing(ad_ring_no);

  if (this->iteration_ == 1 && this->big_int_lib_->current_assert_eq_bw_ad_ >= big_int_lib_->num_ring_conversions_) {
#ifdef TESTING
    std::cerr << "ERROR: number of ring conversions is too high" << std::endl;
#endif
    exit(1);
  }

  word *bw_shares_ptr = bw_shares;
  word *ad_shares_ptr = ad_shares;
  uint32 k2 = this->big_int_lib_->current_assert_eq_bw_ad_ * this->party_size_ * big_int_lib_->gate_num_words_;
  for (uint32 i = 0; i < 2; i++) {
    uint32 k3 = k2 + i * big_int_lib_->gate_num_words_;
    for (uint32 j = 0; j < big_int_lib_->gate_num_words_; j++) {
      this->assert_eq_bw_shares_[ad_ring_no][k3 + j] = *bw_shares_ptr++;
      this->assert_eq_ad_shares_[ad_ring_no][k3 + j] = *ad_shares_ptr++;
    }
  }

  this->big_int_lib_->current_assert_eq_bw_ad_++;
}

void CircuitContainer::ec_add_shared(bool verify, BigIntLib* ringEC, word* result, word* p1, word* p2) {
  ringEC->Add(result, p1, p2);
  ringEC->Add(result + ringEC->gate_num_words_, p1 + ringEC->gate_num_words_, p2 + ringEC->gate_num_words_);
  if (!verify)
    ringEC->Add(result + 2 * ringEC->gate_num_words_, p1 + 2 * ringEC->gate_num_words_, p2 + 2 * ringEC->gate_num_words_);
}

void CircuitContainer::ec_sub_shared(bool verify, BigIntLib* ringEC, word* result, word* p1, word* p2) {
  ringEC->Sub(result, p1, p2);
  ringEC->Sub(result + ringEC->gate_num_words_, p1 + ringEC->gate_num_words_, p2 + ringEC->gate_num_words_);
  if (!verify)
    ringEC->Sub(result + 2 * ringEC->gate_num_words_, p1 + 2 * ringEC->gate_num_words_, p2 + 2 * ringEC->gate_num_words_);
}

void CircuitContainer::ec_scmult_fixbase_local(BigIntLib* ringEC, uint32 bits_per_block, word* fixpowers, word* scalar, word* result) {
  BigIntLib* ringP = ringEC->ringP_;
  uint32 num_bits = ringP->field_size_bits_;
  uint32 num_blocks = (num_bits + bits_per_block - 1) / bits_per_block;
  uint32 num_bits_in_highest_block = num_bits - (num_blocks - 1) * bits_per_block;
  uint32 block_size = (1 << bits_per_block) - 1;
  word tmp[ringP->gate_num_words_];
  memcpy(tmp, scalar, ringP->gate_size_);
  memset(result, 0, ringEC->gate_size_);
  if (bits_per_block > 64) {
#ifdef TESTING
    std::cerr << "ERROR: ec_scmult_fixbase_local: the case of bits_per_block larger than 64 is not implemented" << std::endl;
#endif
    exit(1);
  }
  if (num_bits % 64 != 0) {
#ifdef TESTING
    std::cerr << "ERROR: ec_scmult_fixbase_local: the case of the number of bits in P not a multiple of 64 is not implemented" << std::endl;
#endif
    exit(1);
  }
  uint32 highest_i = ringP->gate_num_words_ - 1;
  uint32 num_bits_in_curr_block = num_bits_in_highest_block;
  for (uint32 j = 0; j < num_blocks; j++) {
    uint32 i = num_blocks - 1 - j;
    word block_bits = tmp[highest_i] >> (64 - num_bits_in_curr_block);
    if (block_bits > 0) {
      ringEC->Add(result, result, fixpowers + ringEC->gate_num_words_ * (i * block_size + block_bits - 1));
    }
    ringEC->ringBW_->ShlC(tmp, tmp, num_bits_in_curr_block);
    num_bits_in_curr_block = bits_per_block;
  }
}

void CircuitContainer::ec_scmult_fixbase_shared(BigIntLib* ringEC, bool verify, uint32 bits_per_block, word* fixpowers, word* scalar, word* result) {
  BigIntLib* ringP = ringEC->ringP_;
  word *(scalar_shares_p[3]) = {scalar, scalar + ringP->gate_num_words_, scalar + 2 * ringP->gate_num_words_}; // shared modulo Q
  word *(result_shares_p[3]) = {result, result + ringEC->gate_num_words_, result + 2 * ringEC->gate_num_words_}; // shared over EC addition, each party has an EC point as 2 values modulo P
  ec_scmult_fixbase_local(ringEC, bits_per_block, fixpowers, scalar_shares_p[0], result_shares_p[0]);
  ec_scmult_fixbase_local(ringEC, bits_per_block, fixpowers, scalar_shares_p[1], result_shares_p[1]);
  if (!verify)
    ec_scmult_fixbase_local(ringEC, bits_per_block, fixpowers, scalar_shares_p[2], result_shares_p[2]);
}

// r = (x / y) mod p
// Uses 1 witness value + 1 multiplication (without intermediate value) + 1 assert_zero
void CircuitContainer::divide_mod_P_shared(BigIntLib* ringP, BigIntLib* ringBW, bool verify, word* x, word* y, word* r) {
  word x_local[ringP->gate_num_words_];
  ringP->Add(x_local, x, x + ringP->gate_num_words_);
  ringP->Add(x_local, x_local, x + 2 * ringP->gate_num_words_);
  word y_local[ringP->gate_num_words_];
  ringP->Add(y_local, y, y + ringP->gate_num_words_);
  ringP->Add(y_local, y_local, y + 2 * ringP->gate_num_words_);
  word y_inv_local[ringP->gate_num_words_];
  ringP->Inverse(ringBW, y_inv_local, y_local);
  word x_div_y_local[ringP->gate_num_words_];
  ringP->Mul(x_div_y_local, x_local, y_inv_local);
  word tmp[3 * ringP->gate_num_words_];
  setRing(ringP->ring_no_);
  if (verify) {
    getWitnessSharedVerify(r);
    mulSharedVerify(r, y, tmp);
  } else {
    setWitnessSharedSign(x_div_y_local, r);
    mulNoIntermediateResultSharedSign(r, y, tmp);
  }
  (this->*sub_shared_function_)(tmp, x, tmp);
  (this->*assert_zero_shared_function_)(tmp);
}

// Subtract EC points (x0,x1) and (y0,y1), result (r0,r1), where each of x0, x1, y0, y1, r0, r1 is shared modulo P
// Uses 1 witness value + 3 multiplications (without intermediate value) + 1 assert_zero
void CircuitContainer::ec_sub_modPshared(BigIntLib* ringP, BigIntLib* ringBW, bool verify, word* x0, word* x1, word* y0, word* y1, word* r0, word* r1) {
  word x0_minus_y0[3 * ringP->field_num_words_];
  word x0_plus_y0[3 * ringP->field_num_words_];
  word x1_plus_y1[3 * ringP->field_num_words_];
  (this->*sub_shared_function_)(x0, y0, x0_minus_y0);
  (this->*add_shared_function_)(x0, y0, x0_plus_y0);
  (this->*add_shared_function_)(x1, y1, x1_plus_y1);
  word s[3 * ringP->field_num_words_];
  divide_mod_P_shared(ringP, ringBW, verify, x1_plus_y1, x0_minus_y0, s);
  word s2[3 * ringP->field_num_words_];
  setRing(ringP->ring_no_);
  if (verify)
    mulSharedVerify(s, s, s2);
  else
    mulNoIntermediateResultSharedSign(s, s, s2);
  (this->*sub_shared_function_)(s2, x0_plus_y0, r0);
  word x0_minus_r0[3 * ringP->field_num_words_];
  (this->*sub_shared_function_)(x0, r0, x0_minus_r0);
  if (verify)
    mulSharedVerify(s, x0_minus_r0, r1);
  else
    mulNoIntermediateResultSharedSign(s, x0_minus_r0, r1);
  (this->*sub_shared_function_)(r1, x1, r1);
}

// convert an EC point (x,y) where x and y are shared mod P, to a point p shared over EC addition
// Uses 2 witness values + 6 multiplications (without intermediate value) + 2 assert_zeros + 2 declassify3s (mod P) + 1 random value (EC)
void CircuitContainer::convert_modP_to_EC_shared(BigIntLib* ringBW, bool verify, word* x, word* y, word* p) {
  BigIntLib* ringEC;
  uint32 ringEC_no;
  for(uint32 i = 0; i < this->num_rings_; i++) {
    BigIntLib* big_int_lib = &this->all_big_int_libs_[i];
    if (big_int_lib->field_type_ == 4) {
      ringEC = big_int_lib;
      ringEC_no = i;
    }
  }
  BigIntLib* ringP = ringEC->ringP_;
  uint32 r_party = 0, s_party = 1;
  if (verify) {
    if (this->e_ == 1) {
      r_party = 2;
      s_party = 0;
    } else if (this->e_ == 2) {
      r_party = 1;
      s_party = 2;
    }
  }
  uint32 gate_random_pointer = ringEC->current_random_gate_ * ringEC->gate_num_words_;
  word r_share[ringEC->gate_num_words_]; // only share of 1st party, the rest are 0
  word r_x[3][ringP->gate_num_words_];
  word r_y[3][ringP->gate_num_words_];
  memset(r_x, 0, sizeof(r_x));
  memset(r_y, 0, sizeof(r_y));
  if (r_party != 2) {
    memcpy(r_share, all_random_numbers_[ringEC_no][r_party] + gate_random_pointer, ringEC->gate_size_);
    memcpy(r_x[r_party], r_share, ringP->gate_size_);
    memcpy(r_y[r_party], r_share + ringP->gate_num_words_, ringP->gate_size_);
  }
  word s_share[ringEC->gate_num_words_]; // only share of 2nd party, the rest are 0
  word s_x[3][ringP->gate_num_words_];
  word s_y[3][ringP->gate_num_words_];
  memset(s_x, 0, sizeof(s_x));
  memset(s_y, 0, sizeof(s_y));
  if (s_party != 2) {
    memcpy(s_share, all_random_numbers_[ringEC_no][s_party] + gate_random_pointer, ringEC->gate_size_);
    memcpy(s_x[s_party], s_share, ringP->gate_size_);
    memcpy(s_y[s_party], s_share + ringP->gate_num_words_, ringP->gate_size_);
  }
  ringEC->current_random_gate_++;
  word v_minus_r_x[3 * ringP->field_num_words_];
  word v_minus_r_y[3 * ringP->field_num_words_];
  ec_sub_modPshared(ringP, ringBW, verify, x, y, (word*)r_x, (word*)r_y, v_minus_r_x, v_minus_r_y);
  word v_minus_r_minus_s_x[3 * ringP->field_num_words_];
  word v_minus_r_minus_s_y[3 * ringP->field_num_words_];
  ec_sub_modPshared(ringP, ringBW, verify, v_minus_r_x, v_minus_r_y, (word*)s_x, (word*)s_y, v_minus_r_minus_s_x, v_minus_r_minus_s_y);
  word p_x_shares[3][ringP->gate_num_words_];
  word p_y_shares[3][ringP->gate_num_words_];
  if (verify) {
    declassify3SharedVerify(ringP->ring_no_, v_minus_r_minus_s_x, (word*)p_x_shares);
    declassify3SharedVerify(ringP->ring_no_, v_minus_r_minus_s_y, (word*)p_y_shares);
    memcpy(p, p_x_shares[0], ringP->gate_size_);
    memcpy(p + ringEC->gate_num_words_, p_x_shares[1], ringP->gate_size_);
    memcpy(p + ringP->gate_num_words_, p_y_shares[0], ringP->gate_size_);
    memcpy(p + ringP->gate_num_words_ + ringEC->gate_num_words_, p_y_shares[1], ringP->gate_size_);
    if (r_party != 2)
      memcpy(p + r_party * ringEC->gate_num_words_, r_share, ringEC->gate_size_);
    if (s_party != 2)
      memcpy(p + s_party * ringEC->gate_num_words_, s_share, ringEC->gate_size_);
  } else {
    declassify3SharedSign(ringP->ring_no_, v_minus_r_minus_s_x, (word*)p_x_shares);
    declassify3SharedSign(ringP->ring_no_, v_minus_r_minus_s_y, (word*)p_y_shares);
    memcpy(p, r_share, ringEC->gate_size_);
    memcpy(p + ringEC->gate_num_words_, s_share, ringEC->gate_size_);
    memcpy(p + 2*ringEC->gate_num_words_, p_x_shares[2], ringP->gate_size_);
    memcpy(p + ringP->gate_num_words_ + 2*ringEC->gate_num_words_, p_y_shares[2], ringP->gate_size_);
  }
}

// Uses 2 witness values + 6 multiplications (without intermediate value) + 2 assert_zeros + 2 declassify3s (mod P)
// + 1 random value + 1 assert_zero (EC)
void CircuitContainer::ecdsaVerification(bool verify, uint32 bits_per_block, word* fixpowers1, word* scalar1, word* fixpowers2, word* scalar2, word* r_x, word* r_y) {
  BigIntLib* ringBW;
  BigIntLib* ringEC;
  for(uint32 i = 0; i < this->num_rings_; i++) {
    BigIntLib* big_int_lib = &this->all_big_int_libs_[i];
    if (big_int_lib->field_type_ == 4) {
      ringEC = big_int_lib;
    }
  }
  ringBW = ringEC->ringBW_;

  word result[3 * ringEC->gate_num_words_];
  word result2[3 * ringEC->gate_num_words_];
  ec_scmult_fixbase_shared(ringEC, verify, bits_per_block, fixpowers1, scalar1, result);
  ec_scmult_fixbase_shared(ringEC, verify, bits_per_block, fixpowers2, scalar2, result2);
  ec_add_shared(verify, ringEC, result, result, result2);
  word r_ec[3 * ringEC->gate_num_words_];
  convert_modP_to_EC_shared(ringBW, verify, r_x, r_y, r_ec);
  word diff[3 * ringEC->gate_num_words_];
  ec_sub_shared(verify, ringEC, diff, result, r_ec);
  setRing(ringEC->ring_no_);

  (this->*assert_zero_shared_function_)(diff);
}

void CircuitContainer::ecdsaVerificationSharedSign(uint32 bits_per_block, word* fixpowers1, word* scalar1, word* fixpowers2, word* scalar2, word* r_x, word* r_y) {
  ecdsaVerification(false, bits_per_block, fixpowers1, scalar1, fixpowers2, scalar2, r_x, r_y);
}

void CircuitContainer::ecdsaVerificationSharedVerify(uint32 bits_per_block, word* fixpowers1, word* scalar1, word* fixpowers2, word* scalar2, word* r_x, word* r_y) {
  ecdsaVerification(true, bits_per_block, fixpowers1, scalar1, fixpowers2, scalar2, r_x, r_y);
}

// Version of mulSharedSign that does not use intermediate values.
// word* shares contains the words of ALL shares (share_party_1_word_1, share_party_1_word_2, ... ,share_party_n_word_m-1, share_party_n_word_m) for n parties and m words
void CircuitContainer::mulNoIntermediateResultSharedSign(word* a_shares, word* b_shares, word* c_shares) {
  if (this->iteration_ == 1 && this->big_int_lib_->current_mul_gate_ >= big_int_lib_->num_mul_gates_) {
#ifdef TESTING
    std::cerr << "ERROR: number of multiplications is too high: " << this->big_int_lib_->current_mul_gate_ << " >= " << big_int_lib_->num_mul_gates_ << std::endl;
#endif
    exit(1);
  }

  word *(a_shares_p[3]) = {a_shares, a_shares + big_int_lib_->gate_num_words_, a_shares + 2 * big_int_lib_->gate_num_words_};
  word *(b_shares_p[3]) = {b_shares, b_shares + big_int_lib_->gate_num_words_, b_shares + 2 * big_int_lib_->gate_num_words_};
  word *(c_shares_p[3]) = {c_shares, c_shares + big_int_lib_->gate_num_words_, c_shares + 2 * big_int_lib_->gate_num_words_};

  word temp[big_int_lib_->gate_num_words_];
  uint32 mul_gate_random_pointer = this->big_int_lib_->current_random_gate_ * big_int_lib_->gate_num_words_;
  uint32 mul_gate_pointer_uchar = this->big_int_lib_->current_mul_gate_ * big_int_lib_->gate_size_;
  
  // Share 1
  this->big_int_lib_->Mul(temp, a_shares_p[0], b_shares_p[0]); // a0 * b0
  this->big_int_lib_->Mul(c_shares_p[0], a_shares_p[1], b_shares_p[0]); // a1 * b0
  this->big_int_lib_->Add(temp, temp, c_shares_p[0]); // a0 * b0 + a1 * b0
  this->big_int_lib_->Mul(c_shares_p[0], a_shares_p[0], b_shares_p[1]); // a0 * b1
  this->big_int_lib_->Add(temp, temp, c_shares_p[0]); // a0 * b0 + a1 * b0 + a0 * b1
  this->big_int_lib_->Add(temp, temp, this->random_numbers_[0] + mul_gate_random_pointer); // a0 * b0 + a1 * b0 + a0 * b1 + r0
  this->big_int_lib_->Sub(c_shares_p[0], temp, this->random_numbers_[1] + mul_gate_random_pointer); // a0 * b0 + (a1 * b0 + a0 * b1 - r1) + r0
  // Write c share to SignData
  memcpy((this->sign_data_)->views_[ring_no_][0] + mul_gate_pointer_uchar, c_shares_p[0], this->big_int_lib_->field_size_bytes_); // Maybe use this->branch_size_ instead of this->gate_size_

  // Share 2
  this->big_int_lib_->Mul(temp, a_shares_p[1], b_shares_p[1]); // a1 * b1
  this->big_int_lib_->Mul(c_shares_p[1], a_shares_p[2], b_shares_p[1]); // a2 * b1
  this->big_int_lib_->Add(temp, temp, c_shares_p[1]);
  this->big_int_lib_->Mul(c_shares_p[1], a_shares_p[1], b_shares_p[2]);
  this->big_int_lib_->Add(temp, temp, c_shares_p[1]);
  this->big_int_lib_->Add(temp, temp, this->random_numbers_[1] + mul_gate_random_pointer);
  this->big_int_lib_->Sub(c_shares_p[1], temp, this->random_numbers_[2] + mul_gate_random_pointer); // a1 * b1 + (a2 * b1 + a1 * b2 - r2) + r1
  // Write c share to SignData
  memcpy((this->sign_data_)->views_[ring_no_][1] + mul_gate_pointer_uchar, c_shares_p[1], this->big_int_lib_->field_size_bytes_); // Maybe use this->branch_size_ instead of this->gate_size_

  // Share 3
  this->big_int_lib_->Mul(temp, a_shares_p[2], b_shares_p[2]); // a2 * b2
  this->big_int_lib_->Mul(c_shares_p[2], a_shares_p[0], b_shares_p[2]); // a0 * b2
  this->big_int_lib_->Add(temp, temp, c_shares_p[2]);
  this->big_int_lib_->Mul(c_shares_p[2], a_shares_p[2], b_shares_p[0]);
  this->big_int_lib_->Add(temp, temp, c_shares_p[2]);
  this->big_int_lib_->Add(temp, temp, this->random_numbers_[2] + mul_gate_random_pointer);
  this->big_int_lib_->Sub(c_shares_p[2], temp, this->random_numbers_[0] + mul_gate_random_pointer); // a2 * b2 + (a0 * b2 + a2 * b0 - r0) + r2
  // Write c share to SignData
  memcpy((this->sign_data_)->views_[ring_no_][2] + mul_gate_pointer_uchar, c_shares_p[2], this->big_int_lib_->field_size_bytes_); // Maybe use this->branch_size_ instead of this->gate_size_

  // ---
  this->big_int_lib_->current_mul_gate_++;
  this->big_int_lib_->current_random_gate_++;
}

// word* shares contains the words of ALL shares (share_party_1_word_1, share_party_1_word_2, ... ,share_party_n_word_m-1, share_party_n_word_m) for n parties and m words
void CircuitContainer::squSharedSign(word* a_shares, word* c_shares) {
  word *(a_shares_p[3]) = {a_shares, a_shares + big_int_lib_->gate_num_words_, a_shares + 2 * big_int_lib_->gate_num_words_};
  word *(c_shares_p[3]) = {c_shares, c_shares + big_int_lib_->gate_num_words_, c_shares + 2 * big_int_lib_->gate_num_words_};

  word temp[big_int_lib_->gate_num_words_];
  uint32 mul_gate_random_pointer = this->big_int_lib_->current_random_gate_ * big_int_lib_->gate_num_words_;
  uint32 mul_gate_pointer_uchar = this->big_int_lib_->current_mul_gate_ * big_int_lib_->gate_size_;
  
  // Share 1
  this->big_int_lib_->Mul(temp, a_shares_p[0], a_shares_p[0]); // a0^2
  this->big_int_lib_->Mul(c_shares_p[0], a_shares_p[1], a_shares_p[0]); // a1 * a0
  this->big_int_lib_->Add(temp, temp, c_shares_p[0]); // a0^2 + a1 * a0
  this->big_int_lib_->Add(temp, temp, c_shares_p[0]); // a0^2 + 2 * a1 * a0
  this->big_int_lib_->Add(temp, temp, this->random_numbers_[0] + mul_gate_random_pointer); // a0^2 + 2 * a1 * a0 + r0
  this->big_int_lib_->Sub(c_shares_p[0], temp, this->random_numbers_[1] + mul_gate_random_pointer); // a0^2 + 2 * a1 * a0 + (r0 - r1)
  // Write c share to SignData
  memcpy((this->sign_data_)->views_[ring_no_][0] + mul_gate_pointer_uchar, c_shares_p[0], this->big_int_lib_->field_size_bytes_); // Maybe use this->branch_size_ instead of this->gate_size_

  // Share 2
  this->big_int_lib_->Mul(temp, a_shares_p[1], a_shares_p[1]); // a1^2
  this->big_int_lib_->Mul(c_shares_p[1], a_shares_p[2], a_shares_p[1]); // a2 * a1
  this->big_int_lib_->Add(temp, temp, c_shares_p[1]);
  this->big_int_lib_->Add(temp, temp, c_shares_p[1]);
  this->big_int_lib_->Add(temp, temp, this->random_numbers_[1] + mul_gate_random_pointer);
  this->big_int_lib_->Sub(c_shares_p[1], temp, this->random_numbers_[2] + mul_gate_random_pointer); // a1^2 + 2 * a2 * a1 + (r1 - r2)
  // Write c share to SignData
  memcpy((this->sign_data_)->views_[ring_no_][1] + mul_gate_pointer_uchar, c_shares_p[1], this->big_int_lib_->field_size_bytes_); // Maybe use this->branch_size_ instead of this->gate_size_

  // --- New ZKB++ optimization
  this->big_int_lib_->Sub(temp, this->intermediate_results_[this->big_int_lib_->current_intermediate_result_], c_shares_p[0]);
  this->big_int_lib_->Sub(c_shares_p[2], temp, c_shares_p[1]);
  memcpy((this->sign_data_)->views_[ring_no_][2] + mul_gate_pointer_uchar, c_shares_p[2], this->big_int_lib_->field_size_bytes_); // Maybe use this->branch_size_ instead of this->gate_size_
  this->big_int_lib_->current_intermediate_result_++;
  // ---

  this->big_int_lib_->current_mul_gate_++;
  this->big_int_lib_->current_random_gate_++;
}

// word* shares contains the words of ALL shares (share_party_1_word_1, share_party_1_word_2, ... ,share_party_n_word_m-1, share_party_n_word_m) for n parties and m words
void CircuitContainer::mulSharedSign(word* a_shares, word* b_shares, word* c_shares) {
  if (a_shares == b_shares) {
    return squSharedSign(a_shares, c_shares);
  }

  word *(a_shares_p[3]) = {a_shares, a_shares + big_int_lib_->gate_num_words_, a_shares + 2 * big_int_lib_->gate_num_words_};
  word *(b_shares_p[3]) = {b_shares, b_shares + big_int_lib_->gate_num_words_, b_shares + 2 * big_int_lib_->gate_num_words_};
  word *(c_shares_p[3]) = {c_shares, c_shares + big_int_lib_->gate_num_words_, c_shares + 2 * big_int_lib_->gate_num_words_};

  word temp[big_int_lib_->gate_num_words_];
  uint32 mul_gate_random_pointer = this->big_int_lib_->current_random_gate_ * big_int_lib_->gate_num_words_;
  uint32 mul_gate_pointer_uchar = this->big_int_lib_->current_mul_gate_ * big_int_lib_->gate_size_;
  
  // Share 1
  this->big_int_lib_->Mul(temp, a_shares_p[0], b_shares_p[0]); // a0 * b0
  this->big_int_lib_->Mul(c_shares_p[0], a_shares_p[1], b_shares_p[0]); // a1 * b0
  this->big_int_lib_->Add(temp, temp, c_shares_p[0]); // a0 * b0 + a1 * b0
  this->big_int_lib_->Mul(c_shares_p[0], a_shares_p[0], b_shares_p[1]); // a0 * b1
  this->big_int_lib_->Add(temp, temp, c_shares_p[0]); // a0 * b0 + a1 * b0 + a0 * b1
  this->big_int_lib_->Add(temp, temp, this->random_numbers_[0] + mul_gate_random_pointer); // a0 * b0 + a1 * b0 + a0 * b1 + r0
  this->big_int_lib_->Sub(c_shares_p[0], temp, this->random_numbers_[1] + mul_gate_random_pointer); // a0 * b0 + a1 * b0 + a0 * b1 + (r0 - r1)
  // Write c share to SignData
  memcpy((this->sign_data_)->views_[ring_no_][0] + mul_gate_pointer_uchar, c_shares_p[0], this->big_int_lib_->field_size_bytes_); // Maybe use this->branch_size_ instead of this->gate_size_

  // Share 2
  this->big_int_lib_->Mul(temp, a_shares_p[1], b_shares_p[1]); // a1 * b1
  this->big_int_lib_->Mul(c_shares_p[1], a_shares_p[2], b_shares_p[1]); // a2 * b1
  this->big_int_lib_->Add(temp, temp, c_shares_p[1]);
  this->big_int_lib_->Mul(c_shares_p[1], a_shares_p[1], b_shares_p[2]);
  this->big_int_lib_->Add(temp, temp, c_shares_p[1]);
  this->big_int_lib_->Add(temp, temp, this->random_numbers_[1] + mul_gate_random_pointer);
  this->big_int_lib_->Sub(c_shares_p[1], temp, this->random_numbers_[2] + mul_gate_random_pointer); // a1 * b1 + a2 * b1 + a1 * b2 + (r1 - r2)
  // Write c share to SignData
  memcpy((this->sign_data_)->views_[ring_no_][1] + mul_gate_pointer_uchar, c_shares_p[1], this->big_int_lib_->field_size_bytes_); // Maybe use this->branch_size_ instead of this->gate_size_

  // --- New ZKB++ optimization
  this->big_int_lib_->Sub(temp, this->intermediate_results_[this->big_int_lib_->current_intermediate_result_], c_shares_p[0]);
  this->big_int_lib_->Sub(c_shares_p[2], temp, c_shares_p[1]);
  memcpy((this->sign_data_)->views_[ring_no_][2] + mul_gate_pointer_uchar, c_shares_p[2], this->big_int_lib_->field_size_bytes_); // Maybe use this->branch_size_ instead of this->gate_size_
  this->big_int_lib_->current_intermediate_result_++;
  // ---

  this->big_int_lib_->current_mul_gate_++;
  this->big_int_lib_->current_random_gate_++;
}

// word* shares contains the words of ALL shares (share_party_1_word_1, share_party_1_word_2, ... ,share_party_n_word_m-1, share_party_n_word_m) for n parties and m words
void CircuitContainer::mulSharedVerify(word* a_shares, word* b_shares, word* c_shares) {
  // How to:
  // 1. Compute value c_shares[0] using a_shares[0], a_shares[1], b_shares[0] and b_shares[1] (normally like above)
  // 2. Store value in View to c_shares[1] (this is the output that can't be computed here, so it has to be taken from the View)
  // All in all: c_shares[0] is computed like always, c_shares[1] gets the View values
  // Compute c_shares[0]
  word temp[big_int_lib_->gate_num_words_];
  uint32 offset = big_int_lib_->gate_num_words_;
  uint32 mul_gate_random_pointer = this->big_int_lib_->current_random_gate_ * big_int_lib_->gate_num_words_;
  uint32 mul_gate_pointer_uchar = this->big_int_lib_->current_mul_gate_ * big_int_lib_->gate_size_;
  this->big_int_lib_->Mul(temp, a_shares, b_shares);
  this->big_int_lib_->Mul(c_shares, a_shares + offset, b_shares);
  this->big_int_lib_->Add(temp, temp, c_shares);
  this->big_int_lib_->Mul(c_shares, a_shares, b_shares + offset);
  this->big_int_lib_->Add(temp, temp, c_shares);
  this->big_int_lib_->Add(temp, temp, this->random_numbers_[0] + mul_gate_random_pointer);
  this->big_int_lib_->Sub(c_shares, temp, this->random_numbers_[1] + mul_gate_random_pointer);
  // Get value from view and store in c_shares[1]
  memcpy(c_shares + offset, this->proof_->zs_[this->iteration_]->view_[ring_no_] + mul_gate_pointer_uchar, this->big_int_lib_->field_size_bytes_); // Maybe use this->branch_size_ instead of this->gate_size_
  // Write c_shares[0] to VerifyData (this is part of the View computed now)
  memcpy((this->verify_data_)->view_[ring_no_] + mul_gate_pointer_uchar, c_shares, this->big_int_lib_->field_size_bytes_); // Maybe use this->branch_size_ instead of this->gate_size_
  this->big_int_lib_->current_mul_gate_++;
  this->big_int_lib_->current_random_gate_++;
}

// word* shares contains the words of ALL shares (share_party_1_word_1, share_party_1_word_2, ... ,share_party_n_word_m-1, share_party_n_word_m) for n parties and m words
void CircuitContainer::copyShares(word* from_shares, word* to_shares, uint32 party_size) {
  uint32 offset;
  for(uint32 i = 0; i < party_size; i++) {
    offset = i * big_int_lib_->gate_num_words_;
    memcpy(to_shares + offset, from_shares + offset, big_int_lib_->gate_size_); // Maybe use this->branch_size_ instead of this->gate_size_
  }
}

void CircuitContainer::initMiMC() {
  // Function pointers
  this->direct_function_ = &CircuitContainer::directMiMC;
  this->circuit_function_ = &CircuitContainer::circuitMiMC;
  this->prepare_shares_field_sign_function_ = &CircuitContainer::prepareSharesFieldSign;
  this->prepare_shares_field_verify_function_ = &CircuitContainer::prepareSharesFieldVerify;
  this->output_shares_to_bytes_function_ = &CircuitContainer::outputSharesToBytes;
  this->verify_calc_last_share_function_ = &CircuitContainer::verifyCalcLastShare;
}

void CircuitContainer::directMiMC(uchar* x, uchar* y) {
  for (uint32 k = 0; k < this->num_rings_; k++) {
    setRing(k);
    this->big_int_lib_->current_intermediate_result_ = 0;
    this->big_int_lib_->current_instance_ = 0;
    this->big_int_lib_->current_witness_ = 0;
  }
  uint64 cycles_begin = this->rdtsc();
  //std::cout << "directMiMC" << std::endl;

  this->directImpl();

  setRing(0);
  // Make output zero
  word output[this->gate_num_words_];
  memset(output, 0, sizeof(output));

  this->last_direct_call_cycles_ = this->rdtsc() - cycles_begin;
  //std::cout << "Direct output: " << this->big_int_lib_->ToString(output) << std::endl;
  memcpy(y, output, this->value_size_);
}

void CircuitContainer::circuitMiMC(word* value_shares_f, word* key_shares_f, uint32 party_size) {
  this->circuitImpl(party_size);
  this->performAssertEqBwAds(this->verify_); // All assert_eq_bw_ads are performed at the end in batches of WORD_SIZE conversions.

  setRing(0);
  // Use index 0 ([0]) to indicate first (and only) branch

  // Make output zero
  for(uint32 i = 0; i < party_size; i++) {
    memset(this->value_shares_[i], 0, this->value_size_);
    memset(value_shares_f + i * this->gate_num_words_, 0, this->value_size_);
  }
}
