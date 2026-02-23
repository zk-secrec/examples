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
#include "utils.h"
#include "BigIntLib.h"
//#include <cmath> // log2(.)
//#include <chrono>

#ifdef USE_OPENSSL
#include <openssl/evp.h> // For AES
#endif

//#define VERBOSE
//#define VERBOSE_ERROR

#ifndef USE_CSTDLIB
void exit2(int status) {
}
#endif

void BigIntLib::Init(uint32 branch_size_bits, uint32 field_type, uint32 ring_variant) {
  BigIntLib::field_type_ = field_type;
  BigIntLib::field_size_bits_ = branch_size_bits;
  BigIntLib::ring_variant_ = ring_variant;
  //this->gate_size_ = ceil((float)branch_size_bits / 64) * 8;
  this->gate_size_ = (branch_size_bits + WORD_SIZE - 1) / WORD_SIZE * (WORD_SIZE / 8);
  //this->gate_num_words_ = ceil(float(this->gate_size_) / (WORD_SIZE / 8));
  this->gate_num_words_ = (branch_size_bits + WORD_SIZE - 1) / WORD_SIZE;
  //this->value_size_ = ceil(branch_size_bits / 8.0);
  this->value_size_ = (branch_size_bits + 7) / 8;

  //BigIntLib::field_size_bytes_ = ceil(((float)BigIntLib::field_size_bits_ / WORD_SIZE) * 8);
  BigIntLib::field_size_bytes_ = (BigIntLib::field_size_bits_ + 7) / 8;
  //BigIntLib::field_num_words_ = ceil((float)(BigIntLib::field_size_bytes_) / (WORD_SIZE / 8));
  BigIntLib::field_num_words_ = (BigIntLib::field_size_bits_ + WORD_SIZE - 1) / WORD_SIZE;

  uint32 msb_word_unused_bits = (WORD_SIZE - (BigIntLib::field_size_bits_ % WORD_SIZE)) % WORD_SIZE;
  BigIntLib::msb_word_mask_ = (0xFFFFFFFFFFFFFFFF >> msb_word_unused_bits);

  BigIntLib::modulo_ = new word[BigIntLib::field_num_words_];

  // This if construction is bad, change to something better later
  if(BigIntLib::field_type_ == 0) { // Prime field
    BigIntLib::minus_modulo_ = new word[BigIntLib::field_num_words_];
    BigIntLib::minus_modulo_bits_ = new word[BigIntLib::field_size_bits_];
    memset(minus_modulo_, 0, BigIntLib::field_num_words_ * sizeof(word));
    memset(minus_modulo_bits_, 0, BigIntLib::field_size_bits_ * sizeof(word));
    int* t_coeff;
    GREATER_FUNCTION = &BigIntLib::GreaterPF;
    TRY_REDUCE_FUNCTION = &BigIntLib::TryReducePF;
    GET_RANDOM_FIELD_ELEMENTS_FUNCTION = &BigIntLib::GetRandomFieldElementsPF;
    //GET_RANDOM_FIELD_ELEMENTS_NON_ZERO_FUNCTION = &BigIntLib::GetRandomFieldElementsNonZeroPF;
    //TIMES_2_FUNCTION = &BigIntLib::Times2PF;
    //TIMES_3_FUNCTION = &BigIntLib::Times3PF;
    /* if(BigIntLib::field_size_bits_ == 3) {
      // Solinas prime 2^3 - 2 - 1
      BigIntLib::modulo_[0] = 0x5;
      // 2^3 - 2 - 1 = t^3 - t - 1, where t = 2^1
      BigIntLib::fastreduc_num_words_ = 3;
      t_coeff = new int[BigIntLib::fastreduc_num_words_]();
      t_coeff[0] = -1;
      t_coeff[1] = -1;

      ADD_FUNCTION = &BigIntLib::AddPF3;
      SUB_FUNCTION = &BigIntLib::SubPF3;
      MUL_FUNCTION = &BigIntLib::MulPF3Fast;
    }
    else if(BigIntLib::field_size_bits_ == 4) {
      // Solinas prime 2^4 - 2^2 - 1
      // Crandall prime 2^4 - 5 (same as Solinas)
      BigIntLib::modulo_[0] = 0xB;
      // 2^4 - 2^2 - 1 = t^2 - t - 1, where t = 2^2
      BigIntLib::fastreduc_num_words_ = 2;
      t_coeff = new int[BigIntLib::fastreduc_num_words_]();
      t_coeff[0] = -1;
      t_coeff[1] = -1;

      ADD_FUNCTION = &BigIntLib::AddPF4;
      SUB_FUNCTION = &BigIntLib::SubPF4;
      MUL_FUNCTION = &BigIntLib::MulPF4Fast;
      //MUL_FUNCTION = &BigIntLib::MulPF4FastCrandall;
      DOUBLE_ADD_FUNCTION = &BigIntLib::DoubleAddPF4;
    }
    else if(BigIntLib::field_size_bits_ == 16) {
      // Solinas prime 2^16 - 2^4 - 1
      // Crandall prime 2^32 - 17 (same as Solinas)
      BigIntLib::modulo_[0] = 0xFFEF;
      // 2^16 - 2^4 - 1 = t^4 - t - 1, where t = 2^4
      BigIntLib::fastreduc_num_words_ = 4;
      t_coeff = new int[BigIntLib::fastreduc_num_words_]();
      t_coeff[0] = -1;
      t_coeff[1] = -1;

      ADD_FUNCTION = &BigIntLib::AddPF16;
      SUB_FUNCTION = &BigIntLib::SubPF16;
      MUL_FUNCTION = &BigIntLib::MulPF16Fast;
      //MUL_FUNCTION = &BigIntLib::MulPF16FastCrandall;

      // Experimental
      ADD_SPEC_FUNCTION = &BigIntLib::AddSpecPF16;
      MUL_SPEC_FUNCTION = &BigIntLib::MulSpecPF16;
      REDUC_FUNCTION = &BigIntLib::SolinasReducPF16;
    }
    else if(BigIntLib::field_size_bits_ == 32) {
      // Solinas prime 2^32 - 2^4 - 1
      // Crandall prime 2^32 - 5
      BigIntLib::modulo_[0] = 0xFFFFFFEF; // Solinas USE THIS!
      //BigIntLib::modulo_[0] = 0xFEFFFFFF; // 2^32 - 2^24 - 1 = t^4 - t^3 - 1 ONLY FOR PRINTING OUT MATRICES!
      //BigIntLib::modulo_[0] = 0xFFFFFFFB; // Crandall
      // 2^32 - 2^4 - 1 = t^8 - t - 1, where t = 2^4
      BigIntLib::fastreduc_num_words_ = 8; // USE THIS!
      //BigIntLib::fastreduc_num_words_ = 4; // 2^32 - 2^24 - 1 = t^4 - t^3 - 1 ONLY FOR PRINTING OUT MATRICES!
      t_coeff = new int[BigIntLib::fastreduc_num_words_]();
      t_coeff[0] = -1;
      t_coeff[1] = -1; // USE THIS!
      //t_coeff[3] = -1; // 2^32 - 2^24 - 1 = t^4 - t^3 - 1 ONLY FOR PRINTING OUT MATRICES!


      ADD_FUNCTION = &BigIntLib::AddPF32;
      SUB_FUNCTION = &BigIntLib::SubPF32;
      MUL_FUNCTION = &BigIntLib::MulPF32Fast;
      //MUL_FUNCTION = &BigIntLib::MulPF32FastCrandall;
    }
    else if(BigIntLib::field_size_bits_ == 61) {
      // 2^61 - 1
      BigIntLib::modulo_[0] = 0x1FFFFFFFFFFFFFFF;
      BigIntLib::fastreduc_num_words_ = 0;
      t_coeff = new int[BigIntLib::fastreduc_num_words_]();

      ADD_FUNCTION = &BigIntLib::AddPF61;
      SUB_FUNCTION = &BigIntLib::SubPF61;
      MUL_FUNCTION = &BigIntLib::MulPF61Fast;
    }
    else if(BigIntLib::field_size_bits_ == 64) {
      // Solinas prime 2^64 - 2^8 - 1
      // Crandall prime 2^64 - 59
      BigIntLib::modulo_[0] = 0xFFFFFFFFFFFFFEFF; // Solinas
      //BigIntLib::modulo_[0] = 0xFFFFFFFFFFFFFFC5; // Crandall
      // 2^64 - 2^8 - 1 = t^8 - t - 1, where t = 2^8
      BigIntLib::fastreduc_num_words_ = 8;
      t_coeff = new int[BigIntLib::fastreduc_num_words_]();
      t_coeff[0] = -1;
      t_coeff[1] = -1;

      ADD_FUNCTION = &BigIntLib::AddPF64;
      SUB_FUNCTION = &BigIntLib::SubPF64;
      //MUL_FUNCTION = &BigIntLib::MulPF64;
      MUL_FUNCTION = &BigIntLib::MulPF64Fast;
      //MUL_FUNCTION = &BigIntLib::MulPF64FastCrandall;
    }
    else if(BigIntLib::field_size_bits_ == 136) {
      // Solinas prime 2^136 - 2^8 - 1 (alternative with 16-bit-words: 2^144 - 2^128 - 1)
      BigIntLib::modulo_[0] = 0xFFFFFFFFFFFFFEFF;
      BigIntLib::modulo_[1] = 0xFFFFFFFFFFFFFFFF;
      BigIntLib::modulo_[2] = 0x00000000000000FF;
      // 2^136 - 2^8 - 1 = t^17 - t - 1, where t = 2^8
      BigIntLib::fastreduc_num_words_ = 17;
      t_coeff = new int[BigIntLib::fastreduc_num_words_]();
      t_coeff[0] = -1;
      t_coeff[1] = -1;

      ADD_FUNCTION = &BigIntLib::AddPF136;
      SUB_FUNCTION = &BigIntLib::SubPF136;
      MUL_FUNCTION = &BigIntLib::MulPF136Fast;
    }
    else */ if(BigIntLib::field_size_bits_ == 256 && ring_variant == 0) {
      // 2^256 - 2^224 + 2^192 + 2^96 - 1
      BigIntLib::modulo_[0] = 0xffffffffffffffff;
      BigIntLib::modulo_[1] = 0x00000000ffffffff;
      BigIntLib::modulo_[2] = 0x0000000000000000;
      BigIntLib::modulo_[3] = 0xffffffff00000001;
      BigIntLib::fastreduc_num_words_ = 0;
      t_coeff = new int[BigIntLib::fastreduc_num_words_]();

      ADD_FUNCTION = &BigIntLib::AddPF256;
      SUB_FUNCTION = &BigIntLib::SubPF256;
      MUL_FUNCTION = &BigIntLib::MulPF256PFast;
    }
    else if(BigIntLib::field_size_bits_ == 256 && ring_variant == 1) {
      BigIntLib::modulo_[0] = 0xf3b9cac2fc632551;
      BigIntLib::modulo_[1] = 0xbce6faada7179e84;
      BigIntLib::modulo_[2] = 0xffffffffffffffff;
      BigIntLib::modulo_[3] = 0xffffffff00000000;
      BigIntLib::fastreduc_num_words_ = 0;
      t_coeff = new int[BigIntLib::fastreduc_num_words_]();

      ADD_FUNCTION = &BigIntLib::AddPF256;
      SUB_FUNCTION = &BigIntLib::SubPF256;
      MUL_FUNCTION = &BigIntLib::MulPF256QFast;
    }
    /* else if(BigIntLib::field_size_bits_ == 256 && ring_variant == 2) {
      // Implement
      // 2^256 - 2^184 + 2^32 + 1 = t^32 - t^23 + t^4 + 1, where t = 2^8 (Generalized Mersenne Prime)
      BigIntLib::modulo_[0] = 0x0000000100000001;
      BigIntLib::modulo_[1] = 0x0000000000000000;
      BigIntLib::modulo_[2] = 0xFF00000000000000;
      BigIntLib::modulo_[3] = 0xFFFFFFFFFFFFFFFF;
      BigIntLib::fastreduc_num_words_ = 32;
      t_coeff = new int[BigIntLib::fastreduc_num_words_]();
      t_coeff[0] = 1;
      t_coeff[4] = 1;
      t_coeff[23] = -1;
      // TESTING with t^7 - t^3 + 1
      // BigIntLib::field_size_bytes_ = 7;
      //t_coeff[0] = 1;
      //t_coeff[3] = -1;
      // TESTING with t^16 - t - 1
      // BigIntLib::field_size_bytes_ = 16;
      //t_coeff[0] = -1;
      //t_coeff[1] = -1;
      // TESTING with t^14 - t^7 - 1
      //BigIntLib::field_size_bytes_ = 14;
      //t_coeff[0] = -1;
      //t_coeff[7] = -1;

      ADD_FUNCTION = &BigIntLib::AddPF256;
      SUB_FUNCTION = &BigIntLib::SubPF256;
      MUL_FUNCTION = &BigIntLib::MulPF256Fast;
    }
    else if(BigIntLib::field_size_bits_ == 272) {
      // 272-bit gates like MiMC+, write functions for this...
      BigIntLib::modulo_[0] = 0xFFFFFEFFFFFFFFFF;
      BigIntLib::modulo_[1] = 0xFFFFFFFFFFFFFFFF;
      BigIntLib::modulo_[2] = 0xFFFFFFFFFFFFFFFF;
      BigIntLib::modulo_[3] = 0xFFFFFFFFFFFFFFFF;
      BigIntLib::modulo_[4] = 0x000000000000FFFF;
      // 2^272 - 2^40 - 1 = t^34 - t^5 - 1, where t = 2^8 (this is a Solinas prima)
      BigIntLib::fastreduc_num_words_ = 34;
      t_coeff = new int[BigIntLib::fastreduc_num_words_]();
      t_coeff[0] = -1;
      t_coeff[5] = -1;

      ADD_FUNCTION = &BigIntLib::AddPF272;
      SUB_FUNCTION = &BigIntLib::SubPF272;
      MUL_FUNCTION = &BigIntLib::MulPF272Fast;
    } */
    else if(BigIntLib::field_size_bits_ == 384 && ring_variant == 0) {
      BigIntLib::modulo_[0] = 0x00000000FFFFFFFF;
      BigIntLib::modulo_[1] = 0xFFFFFFFF00000000;
      BigIntLib::modulo_[2] = 0xFFFFFFFFFFFFFFFE;
      BigIntLib::modulo_[3] = 0xFFFFFFFFFFFFFFFF;
      BigIntLib::modulo_[4] = 0xFFFFFFFFFFFFFFFF;
      BigIntLib::modulo_[5] = 0xFFFFFFFFFFFFFFFF;
      // 2^384 - 2^128 - 2^96 + 2^32 - 1 = t^12 - t^4 - t^3 + t - 1, where t = 2^32
      BigIntLib::fastreduc_num_words_ = 0;
      t_coeff = new int[BigIntLib::fastreduc_num_words_]();

      ADD_FUNCTION = &BigIntLib::AddPF384;
      SUB_FUNCTION = &BigIntLib::SubPF384;
      MUL_FUNCTION = &BigIntLib::MulPF384PFast1;
    }
    else if(BigIntLib::field_size_bits_ == 384 && ring_variant == 1) {
      BigIntLib::modulo_[0] = 0xECEC196ACCC52973;
      BigIntLib::modulo_[1] = 0x581A0DB248B0A77A;
      BigIntLib::modulo_[2] = 0xC7634D81F4372DDF;
      BigIntLib::modulo_[3] = 0xFFFFFFFFFFFFFFFF;
      BigIntLib::modulo_[4] = 0xFFFFFFFFFFFFFFFF;
      BigIntLib::modulo_[5] = 0xFFFFFFFFFFFFFFFF;
      BigIntLib::fastreduc_num_words_ = 0;
      t_coeff = new int[BigIntLib::fastreduc_num_words_]();

      ADD_FUNCTION = &BigIntLib::AddPF384;
      SUB_FUNCTION = &BigIntLib::SubPF384;
      MUL_FUNCTION = &BigIntLib::MulPF384QFast;
    }
    else {
#ifdef TESTING
      std::cout << "[BigIntLib] Error: Field size " << BigIntLib::field_size_bits_ << " (variant " << ring_variant << ") not supported for this field type." << std::endl;
#endif
      exit(1);
    }

    word carry = 0;
    for (uint32 i = 0; i < gate_num_words_; i++) {
      minus_modulo_[i] = -modulo_[i] - carry;
      if (i + 1 == gate_num_words_) {
        minus_modulo_[i] &= msb_word_mask_;
      }
      if (modulo_[i] != 0) {
        carry = 1;
      }
    }

    uint32 word_size_mask = WORD_SIZE - 1;
    uint32 word_size_shift = 0;
    while ((1 << word_size_shift) < WORD_SIZE)
      word_size_shift++;

    for (uint32 i = 0; i < field_size_bits_; i++) {
      uint32 i1 = i >> word_size_shift;
      uint32 i2 = i & word_size_mask;
      minus_modulo_bits_[i] = -((minus_modulo_[i1] >> i2) & 1);
    }

    //// Compute matrices used for reduction
    //BigIntLib::ComputeReductionMatrix(t_coeff);
    //BigIntLib::ComputeModularAdditionMatrix();
    //BigIntLib::ComputeModularSubtractionMatrix();

    // Clean up
    delete[] t_coeff;
  }
  /*
  else if(BigIntLib::field_type_ == 1) { // Binary field
    GREATER_FUNCTION = &BigIntLib::GreaterBF;
    TRY_REDUCE_FUNCTION = &BigIntLib::TryReduceBF;
    GET_RANDOM_FIELD_ELEMENTS_FUNCTION = &BigIntLib::GetRandomFieldElementsBF;
    GET_RANDOM_FIELD_ELEMENTS_NON_ZERO_FUNCTION = &BigIntLib::GetRandomFieldElementsNonZeroBF;
    if(BigIntLib::field_size_bits_ == 3) {
      // Irreducible polynomial x^3 + x + 1
      BigIntLib::modulo_[0] = 0xB;

      ADD_FUNCTION = &BigIntLib::XorBF3;
      SUB_FUNCTION = &BigIntLib::XorBF3;
      MUL_FUNCTION = &BigIntLib::MulBF3Fast;
      TIMES_2_FUNCTION = &BigIntLib::Times2BF3;
      TIMES_3_FUNCTION = &BigIntLib::Times3BF3;
    }
    else if(BigIntLib::field_size_bits_ == 17) {
      // Irreducible polynomial x^17 + x^3 + 1
      BigIntLib::modulo_[0] = 0x20009;

      ADD_FUNCTION = &BigIntLib::XorBF17;
      SUB_FUNCTION = &BigIntLib::XorBF17;
      MUL_FUNCTION = &BigIntLib::MulBF17Fast;
      TIMES_2_FUNCTION = &BigIntLib::Times2BF17;
      TIMES_3_FUNCTION = &BigIntLib::Times3BF17;
    }
    else if(BigIntLib::field_size_bits_ == 33) {
      // Irreducible polynomial x^33 + x^6 + x^3 + x + 1
      BigIntLib::modulo_[0] = 0x20000004B;

      ADD_FUNCTION = &BigIntLib::XorBF33;
      SUB_FUNCTION = &BigIntLib::XorBF33;
      MUL_FUNCTION = &BigIntLib::MulBF33Fast;
      TIMES_2_FUNCTION = &BigIntLib::Times2BF33;
      TIMES_3_FUNCTION = &BigIntLib::Times3BF33;
    }
    else if(BigIntLib::field_size_bits_ == 65) {
      // Irreducible polynomial x^65 + x^4 + x^3 + x + 1
      BigIntLib::modulo_[0] = 0x1B;
      BigIntLib::modulo_[1] = 0x2;

      ADD_FUNCTION = &BigIntLib::XorBF65;
      SUB_FUNCTION = &BigIntLib::XorBF65;
      MUL_FUNCTION = &BigIntLib::MulBF65Fast;
      TIMES_2_FUNCTION = &BigIntLib::Times2BF65;
      TIMES_3_FUNCTION = &BigIntLib::Times3BF65;
    }
    else {
      std::cout << "[BigIntLib] Error: Field size " << BigIntLib::field_size_bits_ << " not supported for this field type." << std::endl;
      exit(1);
    }
  }
  else if(BigIntLib::field_type_ == 2) { // Ring of integers modulo 2^n
    //GREATER_FUNCTION = &BigIntLib::GreaterPF;
    GET_RANDOM_FIELD_ELEMENTS_FUNCTION = &BigIntLib::GetRandomFieldElementsBF;
    if(BigIntLib::field_size_bits_ == 32) {
      TRY_REDUCE_FUNCTION = &BigIntLib::TryReduceR32;
      ADD_FUNCTION = &BigIntLib::AddR32;
      SUB_FUNCTION = &BigIntLib::SubR32;
      MUL_FUNCTION = &BigIntLib::MulR32;
    } else if(BigIntLib::field_size_bits_ == 64) {
      TRY_REDUCE_FUNCTION = &BigIntLib::TryReduceR64;
      ADD_FUNCTION = &BigIntLib::AddR64;
      SUB_FUNCTION = &BigIntLib::SubR64;
      MUL_FUNCTION = &BigIntLib::MulR64;
    } else {
      std::cout << "[BigIntLib] Error: Field size " << BigIntLib::field_size_bits_ << " not supported for this field type (ring of integers modulo 2^n)." << std::endl;
      exit(1);
    }
  } */
  else if(BigIntLib::field_type_ == 3) { // ring of n-bit vectors with bitwise operations
    //GREATER_FUNCTION = &BigIntLib::GreaterPF;
    GET_RANDOM_FIELD_ELEMENTS_FUNCTION = &BigIntLib::GetRandomFieldElementsBF;
    if(BigIntLib::field_size_bits_ == 32) {
      TRY_REDUCE_FUNCTION = &BigIntLib::TryReduceR32;
      ADD_FUNCTION = &BigIntLib::XorBW64;
      SUB_FUNCTION = &BigIntLib::XorBW64;
      MUL_FUNCTION = &BigIntLib::AndBW64;
      SHLC_FUNCTION = &BigIntLib::ShlcBW32;
      SHRC_FUNCTION = &BigIntLib::ShrcBW64;
    } else if(BigIntLib::field_size_bits_ == 64) {
      TRY_REDUCE_FUNCTION = &BigIntLib::TryReduceR64;
      ADD_FUNCTION = &BigIntLib::XorBW64;
      SUB_FUNCTION = &BigIntLib::XorBW64;
      MUL_FUNCTION = &BigIntLib::AndBW64;
      SHLC_FUNCTION = &BigIntLib::ShlcBW64;
      SHRC_FUNCTION = &BigIntLib::ShrcBW64;
    } else if(BigIntLib::field_size_bits_ <= 64) {
      TRY_REDUCE_FUNCTION = &BigIntLib::TryReduceRAtMost64;
      ADD_FUNCTION = &BigIntLib::XorBW64;
      SUB_FUNCTION = &BigIntLib::XorBW64;
      MUL_FUNCTION = &BigIntLib::AndBW64;
      SHLC_FUNCTION = &BigIntLib::ShlcBWAtMost64;
      SHRC_FUNCTION = &BigIntLib::ShrcBW64;
    } else if(BigIntLib::field_size_bits_ == 256) {
      TRY_REDUCE_FUNCTION = &BigIntLib::TryReduceR64;
      ADD_FUNCTION = &BigIntLib::XorBW256;
      SUB_FUNCTION = &BigIntLib::XorBW256;
      MUL_FUNCTION = &BigIntLib::AndBW256;
      SHLC_FUNCTION = &BigIntLib::ShlcBW256;
      SHRC_FUNCTION = &BigIntLib::ShrcBW256;
    } else if(BigIntLib::field_size_bits_ == 384) {
      TRY_REDUCE_FUNCTION = &BigIntLib::TryReduceR64;
      ADD_FUNCTION = &BigIntLib::XorBW384;
      SUB_FUNCTION = &BigIntLib::XorBW384;
      MUL_FUNCTION = &BigIntLib::AndBW384;
      SHLC_FUNCTION = &BigIntLib::ShlcBW384;
      SHRC_FUNCTION = &BigIntLib::ShrcBW384;
    } else {
#ifdef TESTING
      std::cout << "[BigIntLib] Error: Field size " << BigIntLib::field_size_bits_ << " not supported for this field type (ring of n-bit vectors with bitwise operations)." << std::endl;
#endif
      exit(1);
    }
  }
  else if(BigIntLib::field_type_ == 4) { // elliptic curve additive group
    GET_RANDOM_FIELD_ELEMENTS_FUNCTION = &BigIntLib::GetRandomFieldElementsEC;
    ADD_FUNCTION = &BigIntLib::AddEC;
    SUB_FUNCTION = &BigIntLib::SubEC;
  }
  else {
#ifdef TESTING
    std::cout << "[BigIntLib] Error: Please choose field type 0 (prime field), 1 (binary field), 2 (ring modulo 2^n), 3 (bitwise ring), 4 (elliptic curve)." << std::endl;
#endif
    exit(1);
  }
}

void BigIntLib::InitStatic() {
  // Init buffer for cipher (AES in this case, 16 bytes output)
  BigIntLib::evp_cipher_buffer_ = new uchar[16];

#ifdef USE_OPENSSL
  // Init AES
  BigIntLib::evp_cipher_ctx_ = EVP_CIPHER_CTX_new();
#endif
}

void BigIntLib::CleanUpStatic() {
  // Clear AES and buffer
  delete[] BigIntLib::evp_cipher_buffer_;
#ifdef USE_OPENSSL
  EVP_CIPHER_CTX_free(BigIntLib::evp_cipher_ctx_);
#endif
}

#ifdef USE_OPENSSL
EVP_CIPHER_CTX* BigIntLib::evp_cipher_ctx_;
#endif
uchar* BigIntLib::evp_cipher_buffer_;

void BigIntLib::CleanUp() {
  delete[] BigIntLib::modulo_;

  // Matrices (this can be shortened in a single for loop), do it only for prime fields
  if(BigIntLib::field_type_ == 0) {
    delete[] BigIntLib::minus_modulo_;
    delete[] BigIntLib::minus_modulo_bits_;

    //uint32 n = BigIntLib::fastreduc_num_words_;
    //for(uint32 i = 0; i < n; i++) {
    //  delete[] BigIntLib::reduction_matrix_[i];
    //}
    //delete[] BigIntLib::reduction_matrix_;
    //for(uint32 i = 0; i < n; i++) {
    //  delete[] BigIntLib::reduction_matrix_positive_[i];
    //}
    //delete[] BigIntLib::reduction_matrix_positive_;
    //for(uint32 i = 0; i < n; i++) {
    //  delete[] BigIntLib::reduction_matrix_negative_[i];
    //}
    //delete[] BigIntLib::reduction_matrix_negative_;
    //for(uint32 i = 0; i < BigIntLib::mod_addition_weight_; i++) {
    //  delete[] BigIntLib::mod_addition_matrix_[i];
    //}
    //delete[] BigIntLib::mod_addition_matrix_;
    //for(uint32 i = 0; i < BigIntLib::mod_subtraction_weight_; i++) {
    //  delete[] BigIntLib::mod_subtraction_matrix_[i];
    //}
    //delete[] BigIntLib::mod_subtraction_matrix_;
  }
}

void BigIntLib::ShlC(word* b, word* a, uint32 n) {
  (this->*SHLC_FUNCTION)(b, a, n);
}

void BigIntLib::ShrC(word* b, word* a, uint32 n) {
  (this->*SHRC_FUNCTION)(b, a, n);
}

void BigIntLib::Add(word* c, word* a, word* b) {
  #ifdef VERBOSE
  if(BigIntLib::field_type_ == 0) {
    std::cout << "--- ADD ---" << std::endl;
    std::cout << "Copy for Sage:" << std::endl;
    std::cout << "(0x" << BigIntLib::ToString(a) << " + 0x" << BigIntLib::ToString(b) << ") \% 0x" << BigIntLib::ToString(BigIntLib::modulo_) << std::endl;
  }
  else if(BigIntLib::field_type_ == 1) {
    std::cout << "--- ADD ---" << std::endl;
    std::cout << "Copy for Sage:" << std::endl;
    std::cout << "0x" << BigIntLib::ToString(a) << " ^ 0x" << BigIntLib::ToString(b) << std::endl;
  }
  #endif
  (this->*ADD_FUNCTION)(c, a, b);
  #ifdef VERBOSE
  std::cout << "Result: 0x" << BigIntLib::ToString(c) << std::endl;
  #endif

  #ifdef VERBOSE_ERROR
  if(BigIntLib::field_type_ == 0) {
    if(BigIntLib::Greater(a, BigIntLib::modulo_)) {
      std::cout << "[ADD] ERROR!!! a > modulo" << std::endl;
      exit(1);
    }

    if(BigIntLib::Greater(b, BigIntLib::modulo_)) {
      std::cout << "[ADD] ERROR!!! b > modulo" << std::endl;
      exit(1);
    }

    if(BigIntLib::Greater(c, BigIntLib::modulo_)) {
      std::cout << "[ADD] ERROR!!! Result > modulo" << std::endl;
      std::cout << "--- FAILED ADD ---" << std::endl;
      std::cout << "Copy for Sage:" << std::endl;
      std::cout << "(0x" << BigIntLib::ToString(a) << " + 0x" << BigIntLib::ToString(b) << ") \% 0x" << BigIntLib::ToString(BigIntLib::modulo_) << std::endl;
      std::cout << "Result: 0x" << BigIntLib::ToString(c) << std::endl;
      exit(1);
    }
  }
  #endif
}

void BigIntLib::Sub(word* c, word* a, word* b) {
  #ifdef VERBOSE
  if(BigIntLib::field_type_ == 0) {
    std::cout << "--- SUB ---" << std::endl;
    std::cout << "Copy for Sage:" << std::endl;
    std::cout << "(0x" << BigIntLib::ToString(a) << " - 0x" << BigIntLib::ToString(b) << ") \% 0x" << BigIntLib::ToString(BigIntLib::modulo_) << std::endl;
  }
  else if(BigIntLib::field_type_ == 1) {
    std::cout << "--- SUB ---" << std::endl;
    std::cout << "Copy for Sage:" << std::endl;
    std::cout << "0x" << BigIntLib::ToString(a) << " ^ 0x" << BigIntLib::ToString(b) << std::endl;
  }
  #endif
  (this->*SUB_FUNCTION)(c, a, b);
  #ifdef VERBOSE
  std::cout << "Result: 0x" << BigIntLib::ToString(c) << std::endl;
  #endif

  #ifdef VERBOSE_ERROR
  if(BigIntLib::field_type_ == 0) {
    if(BigIntLib::Greater(a, BigIntLib::modulo_)) {
      std::cout << "[SUB] ERROR!!! a > modulo" << std::endl;
      exit(1);
    }

    if(BigIntLib::Greater(b, BigIntLib::modulo_)) {
      std::cout << "[SUB] ERROR!!! b > modulo" << std::endl;
      exit(1);
    }

    if(BigIntLib::Greater(c, BigIntLib::modulo_)) {
      std::cout << "[SUB] ERROR!!! Result > modulo" << std::endl;
      std::cout << "--- FAILED SUB ---" << std::endl;
      std::cout << "Copy for Sage:" << std::endl;
      std::cout << "(0x" << BigIntLib::ToString(a) << " - 0x" << BigIntLib::ToString(b) << ") \% 0x" << BigIntLib::ToString(BigIntLib::modulo_) << std::endl;
      std::cout << "Result: 0x" << BigIntLib::ToString(c) << std::endl;
      exit(1);
    }
  }
  #endif
  
}

void BigIntLib::Mul(word* c, word* a, word* b) {
  #ifdef VERBOSE
  if(BigIntLib::field_type_ == 0) {
    std::cout << "--- MUL ---" << std::endl;
    std::cout << "Copy for Sage:" << std::endl;
    std::cout << "(0x" << BigIntLib::ToString(a) << " * 0x" << BigIntLib::ToString(b) << ") \% 0x" << BigIntLib::ToString(BigIntLib::modulo_) << std::endl;
  }
  else if(BigIntLib::field_type_ == 1) {
    std::cout << "--- MUL ---" << std::endl;
    std::cout << "Copy for Sage:" << std::endl;
    std::cout << "FIELD_SIZE = " << BigIntLib::field_size_bits_ << std::endl;
    std::cout << "a = 0x" << BigIntLib::ToString(a) << std::endl;
    std::cout << "b = 0x" << BigIntLib::ToString(b) << std::endl;
  }
  #endif
  (this->*MUL_FUNCTION)(c, a, b);
  #ifdef VERBOSE
  std::cout << "Result: 0x" << BigIntLib::ToString(c) << std::endl;
  #endif
}

// a**b, where b has at most n bits
// b and c must not be equal pointers
void BigIntLib::Power(word* c, word* a, word* b, uint32 n) {
  if (b == c) {
#ifdef TESTING
    std::cerr << "Error" << std::endl;
#endif
    exit(1);
  }
  word tmp[field_num_words_];
  memcpy(tmp, a, gate_size_);
  memset(c, 0, gate_size_);
  c[0] = 1;
  for (uint32 i = 0; i < n; i++) {
    word bit = (b[i >> 6] >> (i & 63)) & 1;
    if (bit)
      Mul(c, c, tmp);
    if (i+1 < n)
      Mul(tmp, tmp, tmp);
  }
}

// returns true if the square root exists (c = sqrt(a)) and false otherwise
bool BigIntLib::Sqrt(word* c, word* a) {
  if (field_type_ != 0 || (modulo_[0] & 3) != 3) {
#ifdef TESTING
    std::cerr << "BigIntLib::Sqrt is implemented only for prime fields with p = 3 (mod 4)" << std::endl;
#endif
    exit(1);
  }
  word tmp[field_num_words_];
  memcpy(tmp, modulo_, gate_size_);
  // add 1 to tmp
  for (uint32 i = 0; i < field_num_words_; i++) {
    tmp[i]++;
    if (tmp[i] != 0)
      break;
  }
  if (tmp[field_num_words_ - 1] == 0) {
#ifdef TESTING
    std::cerr << "BigIntLib::Sqrt is not implemented for p in the form 2^(64n) - 1" << std::endl;
#endif
    exit(1);
  }
  // divide by 4
  for (uint32 i = 0; i < field_num_words_; i++) {
    tmp[i] >>= 2;
    if (i+1 < field_num_words_)
      tmp[i] ^= (tmp[i+1] << 62);
  }
  word tmp2[field_num_words_];
  Power(tmp2, a, tmp, field_size_bits_ - 1);
  Mul(tmp, tmp2, tmp2);
  Sub(tmp, tmp, a);
  if (IsZero(tmp)) {
    memcpy(c, tmp2, gate_size_);
    return true;
  } else {
    return false;
  }
}

void BigIntLib::InverseGeneric(BigIntLib* ringBW, word* result, word* x) {
  uint32 nw = gate_num_words_;
  uint32 nb = gate_size_;
  word xi[nw];
  word xj[nw];
  word y1[nw];
  word y2[nw];
  word remainder[nw];
  word y1_times_quotient[nw];
  word y[nw];
  word tmp[nw];
  word xj_times_quotient[nw];
  memcpy(xi, this->modulo_, nb);
  memcpy(xj, x, nb);
  memset(y1, 0, nb);
  y1[0] = 1;
  memset(y2, 0, nb);
  while (!this->IsZero(xj)) {
    memcpy(remainder, xi, nb);
    memcpy(y, y2, nb);
    for (;;) {
      uint32 quotient_num_bits = 0; // current part of quotient is 2^quotient_num_bits
      memcpy(y1_times_quotient, y1, nb);
      memcpy(tmp, xj, nb);
      for (;;) {
        if (tmp[nw-1] >> 63)
          break; // avoid overflow in ShlC
        ringBW->ShlC(tmp, tmp, 1);
        if (!this->Smaller(remainder, tmp)) {
          quotient_num_bits++;
          this->Add(y1_times_quotient, y1_times_quotient, y1_times_quotient);
        } else
          break;
      }
      ringBW->ShlC(xj_times_quotient, xj, quotient_num_bits);
      this->Sub(remainder, remainder, xj_times_quotient);
      this->Sub(y, y, y1_times_quotient);
      if (this->Smaller(remainder, xj))
        break;
    }
    memcpy(y2, y1, nb);
    memcpy(y1, y, nb);
    memcpy(xi, xj, nb);
    memcpy(xj, remainder, nb);
  }
  xi[0]--;
  if (!this->IsZero(xi)) {
#ifdef TESTING
    std::cerr << "ERROR: BigIntLib::Inverse: element is not invertible" << std::endl;
#endif
    exit(1);
  }
  memcpy(result, y2, nb);
}

void BigIntLib::Inverse256(BigIntLib* ringBW, word* result, word* x) {
  word xi[4];
  word xj[4];
  word y1[4];
  word y2[4];
  word remainder[4];
  word y1_times_quotient[4];
  word y[4];
  word tmp[4];
  word xj_times_quotient[4];
  memcpy(xi, this->modulo_, 32);
  memcpy(xj, x, 32);
  memset(y1, 0, 32);
  y1[0] = 1;
  memset(y2, 0, 32);
  while (!this->IsZero(xj)) {
    memcpy(remainder, xi, 32);
    memcpy(y, y2, 32);
    for (;;) {
      uint32 quotient_num_bits = 0; // current part of quotient is 2^quotient_num_bits
      memcpy(y1_times_quotient, y1, 32);
      memcpy(tmp, xj, 32);
      for (;;) {
        if (tmp[3] >> 63)
          break; // avoid overflow in ShlC
        ringBW->ShlC(tmp, tmp, 1);
        if (!this->Smaller(remainder, tmp)) {
          quotient_num_bits++;
          this->Add(y1_times_quotient, y1_times_quotient, y1_times_quotient);
        } else
          break;
      }
      ringBW->ShlC(xj_times_quotient, xj, quotient_num_bits);
      this->Sub(remainder, remainder, xj_times_quotient);
      this->Sub(y, y, y1_times_quotient);
      if (this->Smaller(remainder, xj))
        break;
    }
    memcpy(y2, y1, 32);
    memcpy(y1, y, 32);
    memcpy(xi, xj, 32);
    memcpy(xj, remainder, 32);
  }
  xi[0]--;
  if (!this->IsZero(xi)) {
#ifdef TESTING
    std::cerr << "ERROR: BigIntLib::Inverse: element is not invertible" << std::endl;
#endif
    exit(1);
  }
  memcpy(result, y2, 32);
}

void BigIntLib::Inverse384(BigIntLib* ringBW, word* result, word* x) {
  word xi[6];
  word xj[6];
  word y1[6];
  word y2[6];
  word remainder[6];
  word y1_times_quotient[6];
  word y[6];
  word tmp[6];
  word xj_times_quotient[6];
  memcpy(xi, this->modulo_, 48);
  memcpy(xj, x, 48);
  memset(y1, 0, 48);
  y1[0] = 1;
  memset(y2, 0, 48);
  while (!this->IsZero(xj)) {
    memcpy(remainder, xi, 48);
    memcpy(y, y2, 48);
    for (;;) {
      uint32 quotient_num_bits = 0; // current part of quotient is 2^quotient_num_bits
      memcpy(y1_times_quotient, y1, 48);
      memcpy(tmp, xj, 48);
      for (;;) {
        if (tmp[5] >> 63)
          break; // avoid overflow in ShlC
        ringBW->ShlC(tmp, tmp, 1);
        if (!this->Smaller(remainder, tmp)) {
          quotient_num_bits++;
          this->Add(y1_times_quotient, y1_times_quotient, y1_times_quotient);
        } else
          break;
      }
      ringBW->ShlC(xj_times_quotient, xj, quotient_num_bits);
      this->Sub(remainder, remainder, xj_times_quotient);
      this->Sub(y, y, y1_times_quotient);
      if (this->Smaller(remainder, xj))
        break;
    }
    memcpy(y2, y1, 48);
    memcpy(y1, y, 48);
    memcpy(xi, xj, 48);
    memcpy(xj, remainder, 48);
  }
  xi[0]--;
  if (!this->IsZero(xi)) {
#ifdef TESTING
    std::cerr << "ERROR: BigIntLib::Inverse: element is not invertible" << std::endl;
#endif
    exit(1);
  }
  memcpy(result, y2, 48);
}

void BigIntLib::Inverse(BigIntLib* ringBW, word* result, word* x) {
  if (field_type_ != 0) {
#ifdef TESTING
    std::cerr << "ERROR: BigIntLib::Inverse is only implemented for prime fields" << std::endl;
#endif
    exit(1);
  }
  if (ringBW->field_type_ != 3 || ringBW->field_size_bits_ != field_size_bits_) {
#ifdef TESTING
    std::cerr << "ERROR: BigIntLib::Inverse: incorrect ringBW" << std::endl;
#endif
    exit(1);
  }
  if (this->IsZero(x)) {
#ifdef TESTING
    std::cerr << "ERROR: BigIntLib::Inverse: division by zero" << std::endl;
#endif
    exit(1);
  }

  // The generic version is about 25% slower than the specialized versions
  if (field_size_bits_ == 256) {
    Inverse256(ringBW, result, x);
  } else if (field_size_bits_ == 384) {
    Inverse384(ringBW, result, x);
  } else {
    InverseGeneric(ringBW, result, x);
  }
}

void BigIntLib::Print(void* source, uint32 num_bytes) {
  // TEMP
#ifdef TESTING
  uchar* pointer = (uchar*)source;
  for(uint32 i = 0; i < num_bytes; i++) {
    std::cout << std::setfill('0') << std::setw(2) << std::hex << (uint32)(pointer[num_bytes - i - 1]);
  }
  std::cout << std::dec << std::endl;
#endif
}

bool BigIntLib::IsZero(word* a) {
  // memcmp with test block (all zeros)?
  for(uint32 i = 0; i < BigIntLib::field_num_words_; i++) {
    if(a[i] != 0) return false;
  }
  return true;
}

bool BigIntLib::Smaller(word* a, word* b) {
  for(uint32 i = 0; i < BigIntLib::field_num_words_; i++) {
    if(a[BigIntLib::field_num_words_ - i - 1] < b[BigIntLib::field_num_words_ - i - 1]) return true;
    else if(a[BigIntLib::field_num_words_ - i - 1] > b[BigIntLib::field_num_words_ - i - 1]) return false;
  }
  return false;
}

bool BigIntLib::Greater(word* a, word* b) { // Change and rename to GreaterEqual later!
  return (this->*GREATER_FUNCTION)(a, b);
}

bool BigIntLib::GreaterPF(word* a, word* b) { // Change and rename to GreaterEqualPF later!
  for(uint32 i = 0; i < BigIntLib::field_num_words_; i++) {
    if(a[BigIntLib::field_num_words_ - i - 1] > b[BigIntLib::field_num_words_ - i - 1]) return true;
    else if(a[BigIntLib::field_num_words_ - i - 1] < b[BigIntLib::field_num_words_ - i - 1]) return false;
  }
  return false;
}

bool BigIntLib::GreaterBF(word* a, word* b) { // Change and rename to GreaterEqualBF later!
  return false; // TODO
}

uchar BigIntLib::Compare(word* a, word* b) {
  // 0 .. a < b
  // 1 .. a = b
  // 2 .. a > b
  for(uint32 i = 0; i < BigIntLib::field_num_words_; i++) {
    if(a[BigIntLib::field_num_words_ - i - 1] < b[BigIntLib::field_num_words_ - i - 1]) return 0;
    else if(a[BigIntLib::field_num_words_ - i - 1] > b[BigIntLib::field_num_words_ - i - 1]) return 2;
  }
  return 1;
}

void BigIntLib::TryReduce(word* a) { // Checks if value is in field, reduces if not
  (this->*TRY_REDUCE_FUNCTION)(a);
}

void BigIntLib::TryReducePF(word* a) { // Checks if value is in field, reduces if not
  //if(BigIntLib::GreaterPF(a, BigIntLib::modulo_)) BigIntLib::Sub(a, a, BigIntLib::modulo_);
  if(!BigIntLib::Smaller(a, BigIntLib::modulo_)) BigIntLib::Sub(a, a, BigIntLib::modulo_);
}

void BigIntLib::TryReduceBF(word* a) { // Checks if value is in field, reduces if not TODO
  return;
}

void BigIntLib::GetRandomFieldElements(void* destination, uchar* seed, uint32 num_elements) {
  (this->*GET_RANDOM_FIELD_ELEMENTS_FUNCTION)(destination, seed, num_elements);
}

void BigIntLib::SetSeed(uchar* seed) {
#ifdef USE_OPENSSL
  // 128 bit IV
  static const unsigned char iv[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5'};
  EVP_EncryptInit_ex(BigIntLib::evp_cipher_ctx_, EVP_aes_128_ctr(), NULL, seed, iv);
#endif
}

void BigIntLib::GetNextRandomnessFromSeed(uchar *dest, uint32 num_bytes) {
#ifdef USE_OPENSSL
  static const unsigned char plaintext[16] = {'0'};
  static int length;
  EVP_EncryptUpdate(BigIntLib::evp_cipher_ctx_, dest, &length, plaintext, num_bytes);
#endif
}

void BigIntLib::FillRandom(void* destination, uchar* seed, uint32 num_bytes) {
  uint32 aes_blocksize = 16;
  SetSeed(seed);

  uint32 count = num_bytes;
  uchar* dest = (uchar*)destination;
  for(; count >= aes_blocksize; count -= aes_blocksize, dest += aes_blocksize) {
    BigIntLib::GetNextRandomnessFromSeed(dest);
  }

  if(count) {
    BigIntLib::GetNextRandomnessFromSeed(dest, count);
  }

  //memset(destination, 0, num_bytes); // FOR TESTING, TEMP
}

void BigIntLib::GetRandomFieldElementsEC(void* destination, uchar* seed, uint32 num_elements) {
  uchar* dest_pointer = (uchar*)destination;

  // Init AES
  BigIntLib::SetSeed(seed);

  GetNextRandomnessFromSeed(BigIntLib::evp_cipher_buffer_);
  uint32 buffer_bytes_left = 16;
  uint32 buffer_bytes_used = 0;
  bool current_element_in_field = false;
  uint32 gate_size = BigIntLib::field_num_words_ * 8;
  uint32 msb_word_index = ringP_->field_num_words_ - 1;
  uint32 bytes_still_to_write;
  uint32 bytes_write_now;

  uchar* current_write_pointer_base;
  uchar* current_write_pointer;
  word* current_element_number;

  word C384[6] = {0x2a85c8edd3ec2aef, 0xc656398d8a2ed19d, 0x0314088f5013875a, 0x181d9c6efe814112, 0x988e056be3f82d19, 0xb3312fa7e23ee7e4};
  word C256[6] = {0x3bce3c3e27d2604b, 0x651d06b0cc53b0f6, 0xb3ebbd55769886bc, 0x5ac635d8aa3a93e7};
  word* c;
  if (field_size_bits_ == 2*384)
    c = C384;
  else if (field_size_bits_ == 2*256)
    c = C256;
  else {
#ifdef TESTING
    std::cerr << "GetRandomFieldElementsEC: elliptic curve with " << field_size_bits_ << " bits not implemented" << std::endl;
#endif
    exit(1);
  }
  word zero[ringP_->field_num_words_];
  memset(zero, 0, ringP_->gate_size_);

  // For each element
  for(uint32 i = 0; i < num_elements; i++) {
    current_write_pointer_base = dest_pointer + i * gate_size;
    current_element_number = (word*)current_write_pointer_base;
    // This writes one complete element
    do {
      // First generate the x-coordinate of the EC point, which is an element in ringP_
      // Generate one word more, of which one bit is used to choose the sign of the y-coordinate
      bytes_still_to_write = ringP_->field_size_bytes_ + 8;
      current_write_pointer = current_write_pointer_base;
      while(bytes_still_to_write > 0) {
        //bytes_write_now = std::min(bytes_still_to_write, buffer_bytes_left);
        bytes_write_now = MIN(bytes_still_to_write, buffer_bytes_left);
        memcpy(current_write_pointer, BigIntLib::evp_cipher_buffer_ + buffer_bytes_used, bytes_write_now);
        buffer_bytes_left -= bytes_write_now;
        bytes_still_to_write -= bytes_write_now;
        current_write_pointer += bytes_write_now;
        buffer_bytes_used += bytes_write_now;
        if(buffer_bytes_left == 0) {
          // Buffer is empty, get new AES bytes (maybe write next value already here?)
          GetNextRandomnessFromSeed(BigIntLib::evp_cipher_buffer_);
          buffer_bytes_left = 16;
          buffer_bytes_used = 0;
        }
      }

      // Cancel out last word
      current_element_number[msb_word_index] &= ringP_->msb_word_mask_;
      current_element_in_field = ringP_->Smaller(current_element_number, ringP_->modulo_);
      if (current_element_in_field) {
        word tmp[ringP_->field_num_words_];
        ringP_->Mul(tmp, current_element_number, current_element_number);
        ringP_->Mul(tmp, tmp, current_element_number); // x^3
        ringP_->Sub(tmp, tmp, current_element_number);
        ringP_->Sub(tmp, tmp, current_element_number);
        ringP_->Sub(tmp, tmp, current_element_number); // x^3 - 3x
        ringP_->Add(tmp, tmp, c); // x^3 - 3x + c
        current_element_in_field = ringP_->Sqrt(tmp, tmp); // sqrt(x^3 - 3x + c)
        if (current_element_in_field) {
          // choose the sign of the y-coordinate
          if ((current_element_number[msb_word_index + 1] & 1) != 0) {
            ringP_->Sub(current_element_number + ringP_->field_num_words_, zero, tmp);
          } else {
            memcpy(current_element_number + ringP_->field_num_words_, tmp, ringP_->gate_size_);
          }
        }
      }
    } while(!current_element_in_field);
  }
}

void BigIntLib::GetRandomFieldElementsPF(void* destination, uchar* seed, uint32 num_elements) {
  uchar* dest_pointer = (uchar*)destination;

  // Init AES
  BigIntLib::SetSeed(seed);

  GetNextRandomnessFromSeed(BigIntLib::evp_cipher_buffer_);
  uint32 buffer_bytes_left = 16;
  uint32 buffer_bytes_used = 0;
  bool current_element_in_field = false;
  uint32 gate_size = BigIntLib::field_num_words_ * 8;
  uint32 msb_word_index = BigIntLib::field_num_words_ - 1;
  uint32 bytes_still_to_write;
  uint32 bytes_write_now;

  uchar* current_write_pointer_base;
  uchar* current_write_pointer;
  word* current_element_number;

  // For each element
  for(uint32 i = 0; i < num_elements; i++) {
    current_write_pointer_base = dest_pointer + i * gate_size;
    current_element_number = (word*)current_write_pointer_base;
    // This writes one complete element
    do {
      bytes_still_to_write = BigIntLib::field_size_bytes_;
      current_write_pointer = current_write_pointer_base;
      while(bytes_still_to_write > 0) {
        //bytes_write_now = std::min(bytes_still_to_write, buffer_bytes_left);
        bytes_write_now = MIN(bytes_still_to_write, buffer_bytes_left);
        memcpy(current_write_pointer, BigIntLib::evp_cipher_buffer_ + buffer_bytes_used, bytes_write_now);
        buffer_bytes_left -= bytes_write_now;
        bytes_still_to_write -= bytes_write_now;
        current_write_pointer += bytes_write_now;
        buffer_bytes_used += bytes_write_now;
        if(buffer_bytes_left == 0) {
          // Buffer is empty, get new AES bytes (maybe write next value already here?)
          GetNextRandomnessFromSeed(BigIntLib::evp_cipher_buffer_);
          buffer_bytes_left = 16;
          buffer_bytes_used = 0;
        }
      }

      // Cancel out last word
      current_element_number[msb_word_index] &= BigIntLib::msb_word_mask_;
      current_element_in_field = BigIntLib::Smaller(current_element_number, BigIntLib::modulo_);
    } while(!current_element_in_field);
  }
}

void BigIntLib::GetRandomFieldElementsBF(void* destination, uchar* seed, uint32 num_elements) {
  uchar* dest_pointer = (uchar*)destination;

  // Init AES
  BigIntLib::SetSeed(seed);

  GetNextRandomnessFromSeed(BigIntLib::evp_cipher_buffer_);
  uint32 buffer_bytes_left = 16;
  uint32 buffer_bytes_used = 0;
  uint32 gate_size = BigIntLib::field_num_words_ * 8;
  uint32 msb_word_index = BigIntLib::field_num_words_ - 1;
  uint32 bytes_still_to_write;
  uint32 bytes_write_now;

  uchar* current_write_pointer_base;
  uchar* current_write_pointer;
  word* current_element_number;

  for(uint32 i = 0; i < num_elements; i++) {
    current_write_pointer_base = dest_pointer + i * gate_size;
    current_element_number = (word*)current_write_pointer_base;
    bytes_still_to_write = BigIntLib::field_size_bytes_;
    current_write_pointer = current_write_pointer_base;
    // This writes one complete element
    while(bytes_still_to_write > 0) {
      //bytes_write_now = std::min(bytes_still_to_write, buffer_bytes_left);
      bytes_write_now = MIN(bytes_still_to_write, buffer_bytes_left);
      memcpy_generic(current_write_pointer, BigIntLib::evp_cipher_buffer_ + buffer_bytes_used, bytes_write_now);
      buffer_bytes_left -= bytes_write_now;
      bytes_still_to_write -= bytes_write_now;
      current_write_pointer += bytes_write_now;
      buffer_bytes_used += bytes_write_now;
      if(buffer_bytes_left == 0) {
        // Buffer is empty, get new AES bytes (maybe write next value already here?)
        GetNextRandomnessFromSeed(BigIntLib::evp_cipher_buffer_);
        buffer_bytes_left = 16;
        buffer_bytes_used = 0;
      }

    }

    // Cancel out last word
    current_element_number[msb_word_index] &= BigIntLib::msb_word_mask_;
  }
}

void BigIntLib::TryReduceR32(word* a) {
  *a &= 0xFFFFFFFF;
}

void BigIntLib::TryReduceR64(word* a) {
  return;
}

void BigIntLib::TryReduceRAtMost64(word* a) {
  *a &= msb_word_mask_;
}

void BigIntLib::AddR32(word* c, word* a, word* b) {
  *c = (*a + *b) & 0xFFFFFFFF;
}

void BigIntLib::SubR32(word* c, word* a, word* b) {
  *c = (*a - *b) & 0xFFFFFFFF;
}

void BigIntLib::MulR32(word* c, word* a, word* b) {
  *c = (*a * *b) & 0xFFFFFFFF;
}

void BigIntLib::AddR64(word* c, word* a, word* b) {
  *c = *a + *b;
}

void BigIntLib::SubR64(word* c, word* a, word* b) {
  *c = *a - *b;
}

void BigIntLib::MulR64(word* c, word* a, word* b) {
  *c = *a * *b;
}

void BigIntLib::ShlcBW32(word* b, word* a, uint32 n) {
  *b = (*a << n) & 0xFFFFFFFF;
}

void BigIntLib::XorBW64(word* c, word* a, word* b) {
  *c = *a ^ *b;
}

void BigIntLib::AndBW64(word* c, word* a, word* b) {
  *c = *a & *b;
}

void BigIntLib::ShlcBW64(word* b, word* a, uint32 n) {
  *b = *a << n;
}

void BigIntLib::ShrcBW64(word* b, word* a, uint32 n) {
  *b = *a >> n;
}

void BigIntLib::ShlcBWAtMost64(word* b, word* a, uint32 n) {
  *b = (*a << n) & msb_word_mask_;
}

void BigIntLib::XorBW256(word* c, word* a, word* b) {
  c[0] = a[0] ^ b[0];
  c[1] = a[1] ^ b[1];
  c[2] = a[2] ^ b[2];
  c[3] = a[3] ^ b[3];
}

void BigIntLib::AndBW256(word* c, word* a, word* b) {
  c[0] = a[0] & b[0];
  c[1] = a[1] & b[1];
  c[2] = a[2] & b[2];
  c[3] = a[3] & b[3];
}

void BigIntLib::ShlcBW256(word* b, word* a, uint32 n) {
  if (n == 0) {
    if (b != a)
      memcpy(b, a, 32);
    return;
  }
  if (n >= 256)
    n = 256;
  word* c = a;
  while (n >= 64) {
    b[3] = c[2];
    b[2] = c[1];
    b[1] = c[0];
    b[0] = 0;
    n -= 64;
    c = b;
  }
  if (n > 0) {
    uint32 m = 64 - n;
    b[3] = (c[3] << n) ^ (c[2] >> m);
    b[2] = (c[2] << n) ^ (c[1] >> m);
    b[1] = (c[1] << n) ^ (c[0] >> m);
    b[0] = (c[0] << n);
  }
}

void BigIntLib::ShrcBW256(word* b, word* a, uint32 n) {
#ifdef TESTING
  std::cerr << "ShrcBW256 not yet implemented" << std::endl << std::flush;
#endif
  exit(1);
}

void BigIntLib::XorBW384(word* c, word* a, word* b) {
  c[0] = a[0] ^ b[0];
  c[1] = a[1] ^ b[1];
  c[2] = a[2] ^ b[2];
  c[3] = a[3] ^ b[3];
  c[4] = a[4] ^ b[4];
  c[5] = a[5] ^ b[5];
}

void BigIntLib::AndBW384(word* c, word* a, word* b) {
  c[0] = a[0] & b[0];
  c[1] = a[1] & b[1];
  c[2] = a[2] & b[2];
  c[3] = a[3] & b[3];
  c[4] = a[4] & b[4];
  c[5] = a[5] & b[5];
}

void BigIntLib::ShlcBW384(word* b, word* a, uint32 n) {
  if (n == 0) {
    if (b != a)
      memcpy(b, a, 48);
    return;
  }
  if (n >= 384)
    n = 384;
  word* c = a;
  while (n >= 64) {
    b[5] = c[4];
    b[4] = c[3];
    b[3] = c[2];
    b[2] = c[1];
    b[1] = c[0];
    b[0] = 0;
    n -= 64;
    c = b;
  }
  if (n > 0) {
    uint32 m = 64 - n;
    b[5] = (c[5] << n) ^ (c[4] >> m);
    b[4] = (c[4] << n) ^ (c[3] >> m);
    b[3] = (c[3] << n) ^ (c[2] >> m);
    b[2] = (c[2] << n) ^ (c[1] >> m);
    b[1] = (c[1] << n) ^ (c[0] >> m);
    b[0] = (c[0] << n);
  }
}

void BigIntLib::ShrcBW384(word* b, word* a, uint32 n) {
#ifdef TESTING
  std::cerr << "ShrcBW384 not yet implemented" << std::endl << std::flush;
#endif
  exit(1);
}

void BigIntLib::AddPF256(word* c, word* a, word* b) {
  // Implement static ADD in PF with 256 bits...
  doubleword tmp;
  word carry = 0;
  // for(uint32 i = 0; i < BigIntLib::field_num_words_; i++) {
  //   tmp = (doubleword)a[i] + b[i] + carry;
  //   c[i] = ((tmp << WORD_SIZE) >> WORD_SIZE);
  //   carry = (tmp >> WORD_SIZE);
  // }
  //BigIntLib::Print(a, 40);
  //BigIntLib::Print(b, 40);
  //BigIntLib::Print(c, 40);
  tmp = (doubleword)a[0] + b[0] + carry;
  c[0] = ((tmp << WORD_SIZE) >> WORD_SIZE);
  carry = (tmp >> WORD_SIZE);
  tmp = (doubleword)a[1] + b[1] + carry;
  c[1] = ((tmp << WORD_SIZE) >> WORD_SIZE);
  carry = (tmp >> WORD_SIZE);
  tmp = (doubleword)a[2] + b[2] + carry;
  c[2] = ((tmp << WORD_SIZE) >> WORD_SIZE);
  carry = (tmp >> WORD_SIZE);
  tmp = (doubleword)a[3] + b[3] + carry;
  c[3] = ((tmp << WORD_SIZE) >> WORD_SIZE);
  carry = (tmp >> WORD_SIZE);

  //if(carry == 1 || BigIntLib::Greater(c, BigIntLib::modulo_)) {
  if(carry == 1 || !BigIntLib::Smaller(c, BigIntLib::modulo_)) {
    word borrow = 0;
    // for(uint32 i = 0; i < BigIntLib::field_num_words_; i++) {
    //   tmp = (doubleword)c[i] - BigIntLib::modulo_[i] - borrow;
    //   c[i] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    //   borrow = (tmp >> WORD_SIZE) != 0;
    // }
    tmp = (doubleword)c[0] - BigIntLib::modulo_[0] - borrow;
    c[0] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    borrow = (tmp >> WORD_SIZE) != 0;
    tmp = (doubleword)c[1] - BigIntLib::modulo_[1] - borrow;
    c[1] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    borrow = (tmp >> WORD_SIZE) != 0;
    tmp = (doubleword)c[2] - BigIntLib::modulo_[2] - borrow;
    c[2] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    borrow = (tmp >> WORD_SIZE) != 0;
    tmp = (doubleword)c[3] - BigIntLib::modulo_[3] - borrow;
    c[3] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    borrow = (tmp >> WORD_SIZE) != 0;
  }
}

void BigIntLib::SubPF256(word* c, word* a, word* b) {
  // Implement static SUB in PF with 256 bits...
  doubleword tmp;
  word borrow = 0;
  // for(uint32 i = 0; i < BigIntLib::field_num_words_; i++) {
  //   tmp = (doubleword)a[i] - b[i] - borrow;
  //   c[i] = ((tmp << WORD_SIZE) >> WORD_SIZE);
  //   borrow = (tmp >> WORD_SIZE) != 0;
  // }
  tmp = (doubleword)a[0] - b[0] - borrow;
  c[0] = ((tmp << WORD_SIZE) >> WORD_SIZE);
  borrow = (tmp >> WORD_SIZE) != 0;
  tmp = (doubleword)a[1] - b[1] - borrow;
  c[1] = ((tmp << WORD_SIZE) >> WORD_SIZE);
  borrow = (tmp >> WORD_SIZE) != 0;
  tmp = (doubleword)a[2] - b[2] - borrow;
  c[2] = ((tmp << WORD_SIZE) >> WORD_SIZE);
  borrow = (tmp >> WORD_SIZE) != 0;
  tmp = (doubleword)a[3] - b[3] - borrow;
  c[3] = ((tmp << WORD_SIZE) >> WORD_SIZE);
  borrow = (tmp >> WORD_SIZE) != 0;
  
  if(borrow == 1) {
    word carry = 0;
    // for(uint32 i = 0; i < BigIntLib::field_num_words_; i++) {
    //   tmp = (doubleword)c[i] + BigIntLib::modulo_[i] + carry;
    //   c[i] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    //   carry = (tmp >> WORD_SIZE);
    // }
    tmp = (doubleword)c[0] + BigIntLib::modulo_[0] + carry;
    c[0] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    carry = (tmp >> WORD_SIZE);
    tmp = (doubleword)c[1] + BigIntLib::modulo_[1] + carry;
    c[1] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    carry = (tmp >> WORD_SIZE);
    tmp = (doubleword)c[2] + BigIntLib::modulo_[2] + carry;
    c[2] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    carry = (tmp >> WORD_SIZE);
    tmp = (doubleword)c[3] + BigIntLib::modulo_[3] + carry;
    c[3] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    carry = (tmp >> WORD_SIZE);
  }
}

void BigIntLib::Mul256_512(word* c_temp, word* a, word* b) {
  word U;
  word V;
  doubleword UV;

  memset(c_temp, 0, 64);

  if (a == b) {
    U = 0;
    UV = c_temp[0] + ((doubleword)(a[0]) * (doubleword)(a[0])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[0] = V;
    UV = c_temp[0 + 1] + ((doubleword)(a[0]) * (doubleword)(a[1])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[0 + 1] = V;
    UV = c_temp[0 + 2] + ((doubleword)(a[0]) * (doubleword)(a[2])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[0 + 2] = V;
    UV = c_temp[0 + 3] + ((doubleword)(a[0]) * (doubleword)(a[3])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[0 + 3] = V;
    c_temp[0 + 4] = U;

    U = 0;
    UV = c_temp[1] + ((doubleword)(a[1]) * (doubleword)(a[0])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[1] = V;
    UV = c_temp[1 + 1] + ((doubleword)(a[1]) * (doubleword)(a[1])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[1 + 1] = V;
    UV = c_temp[1 + 2] + ((doubleword)(a[1]) * (doubleword)(a[2])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[1 + 2] = V;
    UV = c_temp[1 + 3] + ((doubleword)(a[1]) * (doubleword)(a[3])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[1 + 3] = V;
    c_temp[1 + 4] = U;

    U = 0;
    UV = c_temp[2] + ((doubleword)(a[2]) * (doubleword)(a[0])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[2] = V;
    UV = c_temp[2 + 1] + ((doubleword)(a[2]) * (doubleword)(a[1])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[2 + 1] = V;
    UV = c_temp[2 + 2] + ((doubleword)(a[2]) * (doubleword)(a[2])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[2 + 2] = V;
    UV = c_temp[2 + 3] + ((doubleword)(a[2]) * (doubleword)(a[3])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[2 + 3] = V;
    c_temp[2 + 4] = U;

    U = 0;
    UV = c_temp[3] + ((doubleword)(a[3]) * (doubleword)(a[0])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[3] = V;
    UV = c_temp[3 + 1] + ((doubleword)(a[3]) * (doubleword)(a[1])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[3 + 1] = V;
    UV = c_temp[3 + 2] + ((doubleword)(a[3]) * (doubleword)(a[2])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[3 + 2] = V;
    UV = c_temp[3 + 3] + ((doubleword)(a[3]) * (doubleword)(a[3])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[3 + 3] = V;
    c_temp[3 + 4] = U;
  } else {
    //for(uint32 i = 0; i < 4; i++) {
    //  U = 0;
    //  for(uint32 j = 0; j < 4; j++) {
    //    UV = c_temp[i + j] + ((doubleword)(a[i]) * (doubleword)(b[j])) + U;
    //    U = UV >> WORD_SIZE;
    //    V = (UV << WORD_SIZE) >> WORD_SIZE;
    //    c_temp[i + j] = V;
    //  }
    //  c_temp[i + 4] = U;
    //}

    U = 0;
    UV = c_temp[0] + ((doubleword)(a[0]) * (doubleword)(b[0])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[0] = V;
    UV = c_temp[0 + 1] + ((doubleword)(a[0]) * (doubleword)(b[1])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[0 + 1] = V;
    UV = c_temp[0 + 2] + ((doubleword)(a[0]) * (doubleword)(b[2])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[0 + 2] = V;
    UV = c_temp[0 + 3] + ((doubleword)(a[0]) * (doubleword)(b[3])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[0 + 3] = V;
    c_temp[0 + 4] = U;

    U = 0;
    UV = c_temp[1] + ((doubleword)(a[1]) * (doubleword)(b[0])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[1] = V;
    UV = c_temp[1 + 1] + ((doubleword)(a[1]) * (doubleword)(b[1])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[1 + 1] = V;
    UV = c_temp[1 + 2] + ((doubleword)(a[1]) * (doubleword)(b[2])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[1 + 2] = V;
    UV = c_temp[1 + 3] + ((doubleword)(a[1]) * (doubleword)(b[3])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[1 + 3] = V;
    c_temp[1 + 4] = U;

    U = 0;
    UV = c_temp[2] + ((doubleword)(a[2]) * (doubleword)(b[0])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[2] = V;
    UV = c_temp[2 + 1] + ((doubleword)(a[2]) * (doubleword)(b[1])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[2 + 1] = V;
    UV = c_temp[2 + 2] + ((doubleword)(a[2]) * (doubleword)(b[2])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[2 + 2] = V;
    UV = c_temp[2 + 3] + ((doubleword)(a[2]) * (doubleword)(b[3])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[2 + 3] = V;
    c_temp[2 + 4] = U;

    U = 0;
    UV = c_temp[3] + ((doubleword)(a[3]) * (doubleword)(b[0])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[3] = V;
    UV = c_temp[3 + 1] + ((doubleword)(a[3]) * (doubleword)(b[1])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[3 + 1] = V;
    UV = c_temp[3 + 2] + ((doubleword)(a[3]) * (doubleword)(b[2])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[3 + 2] = V;
    UV = c_temp[3 + 3] + ((doubleword)(a[3]) * (doubleword)(b[3])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[3 + 3] = V;
    c_temp[3 + 4] = U;
  }
}

void BigIntLib::MulPF256QFast(word* c, word* a, word* b) {
  word t1[8];
  word t2[8];
  Mul256_512(t1, a, b);
  Mul256_512(t2, t1 + 4, minus_modulo_); // highest 32 bits of result are zero
  TryReducePF(t1);
  TryReducePF(t2);
  AddPF256(c, t1, t2);
  Mul256_512(t1, t2 + 4, minus_modulo_); // highest 64 bits of result are zero
  TryReducePF(t1);
  AddPF256(c, c, t1);
  Mul256_512(t2, t1 + 4, minus_modulo_); // highest 96 bits of result are zero
  TryReducePF(t2);
  AddPF256(c, c, t2);
  Mul256_512(t1, t2 + 4, minus_modulo_); // highest 128 bits of result are zero
  TryReducePF(t1);
  AddPF256(c, c, t1);
  Mul256_512(t2, t1 + 4, minus_modulo_); // highest 160 bits of result are zero
  TryReducePF(t2);
  AddPF256(c, c, t2);
  Mul256_512(t1, t2 + 4, minus_modulo_); // highest 192 bits of result are zero
  TryReducePF(t1);
  AddPF256(c, c, t1);
  Mul256_512(t2, t1 + 4, minus_modulo_); // highest 224 bits of result are zero
  TryReducePF(t2);
  AddPF256(c, c, t2);
  Mul256_512(t1, t2 + 4, minus_modulo_); // highest 256 bits of result are zero
  TryReducePF(t1);
  AddPF256(c, c, t1);
}

#define MASK32 0xffffffff

void BigIntLib::MulPF256PFast(word* c, word* a, word* b) {
  // Implement static MUL in PF with 256 bits followed by static reduction using a generalized mersenne prime...
  word c_temp[8];
  Mul256_512(c_temp, a, b);

  // Reduction
  // 2^256 = 2^224 - 2^192 - 2^96 + 1
  // t^8 = t^7 - t^6 - t^3 + 1 where t = 2^32

  word d[16];
  for (uint32 i = 0, j = 0; i < 8; i++) {
    d[j++] = c_temp[i] & MASK32;
    d[j++] = c_temp[i] >> 32;
  }

  word e[8];
  word carry;
  e[7] = d[7] + 2*d[15]           + d[15] + d[8]  - d[10] - d[11] - d[12] - d[13];
  carry = (word)((signedword)e[7] >> 32); // need shift right with sign extension here
  e[7] = (e[7] & MASK32) + carry;
  e[0] = d[0]                     + d[8]  + d[9]  - d[11] - d[12] - d[13] - d[14] + carry;
  e[1] = d[1]                     + d[9]  + d[10] - d[12] - d[13] - d[14] - d[15];
  e[2] = d[2]                     + d[10] + d[11] - d[13] - d[14] - d[15];
  e[3] = d[3] + 2*(d[11] + d[12])         + d[13]         - d[15] - d[8]  - d[9]  - carry;
  e[4] = d[4] + 2*(d[12] + d[13])         + d[14]                 - d[9]  - d[10];
  e[5] = d[5] + 2*(d[13] + d[14])         + d[15]                 - d[10] - d[11];
  e[6] = d[6] + 2*(d[14] + d[15]) + d[14] + d[13] - d[8]  - d[9]                  - carry;

  word t, u;
  carry = 0;
  for (uint32 i = 0, j = 0; i < 4; i++) {
    t = carry + e[j++];
    carry = (word)((signedword)t >> 32); // need shift right with sign extension here
    u = carry + e[j++];
    carry = (word)((signedword)u >> 32); // need shift right with sign extension here
    c[i] = (u << 32) ^ (t & MASK32);
  }
  if (!Smaller(c, BigIntLib::modulo_)) {
    word borrow = 0;
    doubleword tmp;
    tmp = (doubleword)c[0] - BigIntLib::modulo_[0] - borrow;
    c[0] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    borrow = (tmp >> WORD_SIZE) != 0;
    tmp = (doubleword)c[1] - BigIntLib::modulo_[1] - borrow;
    c[1] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    borrow = (tmp >> WORD_SIZE) != 0;
    tmp = (doubleword)c[2] - BigIntLib::modulo_[2] - borrow;
    c[2] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    borrow = (tmp >> WORD_SIZE) != 0;
    tmp = (doubleword)c[3] - BigIntLib::modulo_[3] - borrow;
    c[3] = ((tmp << WORD_SIZE) >> WORD_SIZE);
  }
  while (carry >> 63) {
    SubPF256(c, c, minus_modulo_);
    carry++;
  }
  while (carry > 0) {
    AddPF256(c, c, minus_modulo_);
    carry--;
  }
}

bool BigIntLib::GreaterEq384(word* a, word* b) {
  for(int i = 5; i >= 0; i--) {
    if(a[i] > b[i]) return true;
    else if(a[i] < b[i]) return false;
  }
  return true;

  // For some reason the unrolled version is slower
  //if (a[5] > b[5]) return true;
  //else if (a[5] < b[5]) return false;
  //if (a[4] > b[4]) return true;
  //else if (a[4] < b[4]) return false;
  //if (a[3] > b[3]) return true;
  //else if (a[3] < b[3]) return false;
  //if (a[2] > b[2]) return true;
  //else if (a[2] < b[2]) return false;
  //if (a[1] > b[1]) return true;
  //else if (a[1] < b[1]) return false;
  //if (a[0] > b[0]) return true;
  //else if (a[0] < b[0]) return false;
  //return true;
  ////return (a[0] >= b[0]);
}

void BigIntLib::AddPF384(word* c, word* a, word* b) {
  // Implement static ADD in PF with 384 bits...
  doubleword tmp;
  word carry = 0;
  // for(uint32 i = 0; i < BigIntLib::field_num_words_; i++) {
  //   tmp = (doubleword)a[i] + b[i] + carry;
  //   c[i] = ((tmp << WORD_SIZE) >> WORD_SIZE);
  //   carry = (tmp >> WORD_SIZE);
  // }
  tmp = (doubleword)a[0] + b[0] + carry;
  c[0] = ((tmp << WORD_SIZE) >> WORD_SIZE);
  carry = (tmp >> WORD_SIZE);
  tmp = (doubleword)a[1] + b[1] + carry;
  c[1] = ((tmp << WORD_SIZE) >> WORD_SIZE);
  carry = (tmp >> WORD_SIZE);
  tmp = (doubleword)a[2] + b[2] + carry;
  c[2] = ((tmp << WORD_SIZE) >> WORD_SIZE);
  carry = (tmp >> WORD_SIZE);
  tmp = (doubleword)a[3] + b[3] + carry;
  c[3] = ((tmp << WORD_SIZE) >> WORD_SIZE);
  carry = (tmp >> WORD_SIZE);
  tmp = (doubleword)a[4] + b[4] + carry;
  c[4] = ((tmp << WORD_SIZE) >> WORD_SIZE);
  carry = (tmp >> WORD_SIZE);
  tmp = (doubleword)a[5] + b[5] + carry;
  c[5] = ((tmp << WORD_SIZE) >> WORD_SIZE);
  carry = (tmp >> WORD_SIZE);

  if(carry == 1 || GreaterEq384(c, BigIntLib::modulo_)) {
    word borrow = 0;
    // for(uint32 i = 0; i < BigIntLib::field_num_words_; i++) {
    //   tmp = (doubleword)c[i] - BigIntLib::modulo_[i] - borrow;
    //   c[i] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    //   borrow = (tmp >> WORD_SIZE) != 0;
    // }
    tmp = (doubleword)c[0] - BigIntLib::modulo_[0] - borrow;
    c[0] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    borrow = (tmp >> WORD_SIZE) != 0;
    tmp = (doubleword)c[1] - BigIntLib::modulo_[1] - borrow;
    c[1] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    borrow = (tmp >> WORD_SIZE) != 0;
    tmp = (doubleword)c[2] - BigIntLib::modulo_[2] - borrow;
    c[2] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    borrow = (tmp >> WORD_SIZE) != 0;
    tmp = (doubleword)c[3] - BigIntLib::modulo_[3] - borrow;
    c[3] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    borrow = (tmp >> WORD_SIZE) != 0;
    tmp = (doubleword)c[4] - BigIntLib::modulo_[4] - borrow;
    c[4] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    borrow = (tmp >> WORD_SIZE) != 0;
    tmp = (doubleword)c[5] - BigIntLib::modulo_[5] - borrow;
    c[5] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    // borrow = (tmp >> WORD_SIZE);
  }
}

void BigIntLib::SubPF384(word* c, word* a, word* b) {
  // Implement static SUB in PF with 384 bits...
  doubleword tmp;
  word borrow = 0;
  // for(uint32 i = 0; i < BigIntLib::field_num_words_; i++) {
  //   tmp = (doubleword)a[i] - b[i] - borrow;
  //   c[i] = ((tmp << WORD_SIZE) >> WORD_SIZE);
  //   borrow = (tmp >> WORD_SIZE) != 0;
  // }
  tmp = (doubleword)a[0] - b[0] - borrow;
  c[0] = ((tmp << WORD_SIZE) >> WORD_SIZE);
  borrow = (tmp >> WORD_SIZE) != 0;
  tmp = (doubleword)a[1] - b[1] - borrow;
  c[1] = ((tmp << WORD_SIZE) >> WORD_SIZE);
  borrow = (tmp >> WORD_SIZE) != 0;
  tmp = (doubleword)a[2] - b[2] - borrow;
  c[2] = ((tmp << WORD_SIZE) >> WORD_SIZE);
  borrow = (tmp >> WORD_SIZE) != 0;
  tmp = (doubleword)a[3] - b[3] - borrow;
  c[3] = ((tmp << WORD_SIZE) >> WORD_SIZE);
  borrow = (tmp >> WORD_SIZE) != 0;
  tmp = (doubleword)a[4] - b[4] - borrow;
  c[4] = ((tmp << WORD_SIZE) >> WORD_SIZE);
  borrow = (tmp >> WORD_SIZE) != 0;
  tmp = (doubleword)a[5] - b[5] - borrow;
  c[5] = ((tmp << WORD_SIZE) >> WORD_SIZE);
  borrow = (tmp >> WORD_SIZE) != 0;
  
  if(borrow == 1) {
    word carry = 0;
    // for(uint32 i = 0; i < BigIntLib::field_num_words_; i++) {
    //   tmp = (doubleword)c[i] + BigIntLib::modulo_[i] + carry;
    //   c[i] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    //   carry = (tmp >> WORD_SIZE);
    // }
    tmp = (doubleword)c[0] + BigIntLib::modulo_[0] + carry;
    c[0] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    carry = (tmp >> WORD_SIZE);
    tmp = (doubleword)c[1] + BigIntLib::modulo_[1] + carry;
    c[1] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    carry = (tmp >> WORD_SIZE);
    tmp = (doubleword)c[2] + BigIntLib::modulo_[2] + carry;
    c[2] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    carry = (tmp >> WORD_SIZE);
    tmp = (doubleword)c[3] + BigIntLib::modulo_[3] + carry;
    c[3] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    carry = (tmp >> WORD_SIZE);
    tmp = (doubleword)c[4] + BigIntLib::modulo_[4] + carry;
    c[4] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    carry = (tmp >> WORD_SIZE);
    tmp = (doubleword)c[5] + BigIntLib::modulo_[5] + carry;
    c[5] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    // carry = (tmp >> WORD_SIZE);
  }
}

void BigIntLib::Mul384_768(word* c_temp, word* a, word* b) {
  word U;
  word V;
  doubleword UV;

  //for(uint32 i = 0; i < 12; i++) c_temp[i] = 0;
  memset(c_temp, 0, 96);

  if (a == b) {
    U = 0;
    UV = c_temp[0] + ((doubleword)(a[0]) * (doubleword)(a[0])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[0] = V;
    UV = c_temp[0 + 1] + ((doubleword)(a[0]) * (doubleword)(a[1])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[0 + 1] = V;
    UV = c_temp[0 + 2] + ((doubleword)(a[0]) * (doubleword)(a[2])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[0 + 2] = V;
    UV = c_temp[0 + 3] + ((doubleword)(a[0]) * (doubleword)(a[3])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[0 + 3] = V;
    UV = c_temp[0 + 4] + ((doubleword)(a[0]) * (doubleword)(a[4])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[0 + 4] = V;
    UV = c_temp[0 + 5] + ((doubleword)(a[0]) * (doubleword)(a[5])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[0 + 5] = V;
    c_temp[0 + 6] = U;

    U = 0;
    UV = c_temp[1] + ((doubleword)(a[1]) * (doubleword)(a[0])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[1] = V;
    UV = c_temp[1 + 1] + ((doubleword)(a[1]) * (doubleword)(a[1])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[1 + 1] = V;
    UV = c_temp[1 + 2] + ((doubleword)(a[1]) * (doubleword)(a[2])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[1 + 2] = V;
    UV = c_temp[1 + 3] + ((doubleword)(a[1]) * (doubleword)(a[3])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[1 + 3] = V;
    UV = c_temp[1 + 4] + ((doubleword)(a[1]) * (doubleword)(a[4])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[1 + 4] = V;
    UV = c_temp[1 + 5] + ((doubleword)(a[1]) * (doubleword)(a[5])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[1 + 5] = V;
    c_temp[1 + 6] = U;

    U = 0;
    UV = c_temp[2] + ((doubleword)(a[2]) * (doubleword)(a[0])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[2] = V;
    UV = c_temp[2 + 1] + ((doubleword)(a[2]) * (doubleword)(a[1])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[2 + 1] = V;
    UV = c_temp[2 + 2] + ((doubleword)(a[2]) * (doubleword)(a[2])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[2 + 2] = V;
    UV = c_temp[2 + 3] + ((doubleword)(a[2]) * (doubleword)(a[3])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[2 + 3] = V;
    UV = c_temp[2 + 4] + ((doubleword)(a[2]) * (doubleword)(a[4])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[2 + 4] = V;
    UV = c_temp[2 + 5] + ((doubleword)(a[2]) * (doubleword)(a[5])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[2 + 5] = V;
    c_temp[2 + 6] = U;

    U = 0;
    UV = c_temp[3] + ((doubleword)(a[3]) * (doubleword)(a[0])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[3] = V;
    UV = c_temp[3 + 1] + ((doubleword)(a[3]) * (doubleword)(a[1])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[3 + 1] = V;
    UV = c_temp[3 + 2] + ((doubleword)(a[3]) * (doubleword)(a[2])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[3 + 2] = V;
    UV = c_temp[3 + 3] + ((doubleword)(a[3]) * (doubleword)(a[3])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[3 + 3] = V;
    UV = c_temp[3 + 4] + ((doubleword)(a[3]) * (doubleword)(a[4])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[3 + 4] = V;
    UV = c_temp[3 + 5] + ((doubleword)(a[3]) * (doubleword)(a[5])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[3 + 5] = V;
    c_temp[3 + 6] = U;

    U = 0;
    UV = c_temp[4] + ((doubleword)(a[4]) * (doubleword)(a[0])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[4] = V;
    UV = c_temp[4 + 1] + ((doubleword)(a[4]) * (doubleword)(a[1])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[4 + 1] = V;
    UV = c_temp[4 + 2] + ((doubleword)(a[4]) * (doubleword)(a[2])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[4 + 2] = V;
    UV = c_temp[4 + 3] + ((doubleword)(a[4]) * (doubleword)(a[3])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[4 + 3] = V;
    UV = c_temp[4 + 4] + ((doubleword)(a[4]) * (doubleword)(a[4])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[4 + 4] = V;
    UV = c_temp[4 + 5] + ((doubleword)(a[4]) * (doubleword)(a[5])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[4 + 5] = V;
    c_temp[4 + 6] = U;

    U = 0;
    UV = c_temp[5] + ((doubleword)(a[5]) * (doubleword)(a[0])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[5] = V;
    UV = c_temp[5 + 1] + ((doubleword)(a[5]) * (doubleword)(a[1])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[5 + 1] = V;
    UV = c_temp[5 + 2] + ((doubleword)(a[5]) * (doubleword)(a[2])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[5 + 2] = V;
    UV = c_temp[5 + 3] + ((doubleword)(a[5]) * (doubleword)(a[3])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[5 + 3] = V;
    UV = c_temp[5 + 4] + ((doubleword)(a[5]) * (doubleword)(a[4])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[5 + 4] = V;
    UV = c_temp[5 + 5] + ((doubleword)(a[5]) * (doubleword)(a[5])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[5 + 5] = V;
    c_temp[5 + 6] = U;
  } else {
    //for(uint32 i = 0; i < 6; i++) {
    //  U = 0;
    //  for(uint32 j = 0; j < 6; j++) {
    //    UV = c_temp[i + j] + ((doubleword)(a[i]) * (doubleword)(b[j])) + U;
    //    U = UV >> WORD_SIZE;
    //    V = (UV << WORD_SIZE) >> WORD_SIZE;
    //    c_temp[i + j] = V;
    //  }
    //  c_temp[i + 6] = U;
    //}

    U = 0;
    UV = c_temp[0] + ((doubleword)(a[0]) * (doubleword)(b[0])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[0] = V;
    UV = c_temp[0 + 1] + ((doubleword)(a[0]) * (doubleword)(b[1])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[0 + 1] = V;
    UV = c_temp[0 + 2] + ((doubleword)(a[0]) * (doubleword)(b[2])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[0 + 2] = V;
    UV = c_temp[0 + 3] + ((doubleword)(a[0]) * (doubleword)(b[3])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[0 + 3] = V;
    UV = c_temp[0 + 4] + ((doubleword)(a[0]) * (doubleword)(b[4])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[0 + 4] = V;
    UV = c_temp[0 + 5] + ((doubleword)(a[0]) * (doubleword)(b[5])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[0 + 5] = V;
    c_temp[0 + 6] = U;

    U = 0;
    UV = c_temp[1] + ((doubleword)(a[1]) * (doubleword)(b[0])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[1] = V;
    UV = c_temp[1 + 1] + ((doubleword)(a[1]) * (doubleword)(b[1])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[1 + 1] = V;
    UV = c_temp[1 + 2] + ((doubleword)(a[1]) * (doubleword)(b[2])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[1 + 2] = V;
    UV = c_temp[1 + 3] + ((doubleword)(a[1]) * (doubleword)(b[3])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[1 + 3] = V;
    UV = c_temp[1 + 4] + ((doubleword)(a[1]) * (doubleword)(b[4])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[1 + 4] = V;
    UV = c_temp[1 + 5] + ((doubleword)(a[1]) * (doubleword)(b[5])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[1 + 5] = V;
    c_temp[1 + 6] = U;

    U = 0;
    UV = c_temp[2] + ((doubleword)(a[2]) * (doubleword)(b[0])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[2] = V;
    UV = c_temp[2 + 1] + ((doubleword)(a[2]) * (doubleword)(b[1])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[2 + 1] = V;
    UV = c_temp[2 + 2] + ((doubleword)(a[2]) * (doubleword)(b[2])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[2 + 2] = V;
    UV = c_temp[2 + 3] + ((doubleword)(a[2]) * (doubleword)(b[3])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[2 + 3] = V;
    UV = c_temp[2 + 4] + ((doubleword)(a[2]) * (doubleword)(b[4])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[2 + 4] = V;
    UV = c_temp[2 + 5] + ((doubleword)(a[2]) * (doubleword)(b[5])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[2 + 5] = V;
    c_temp[2 + 6] = U;

    U = 0;
    UV = c_temp[3] + ((doubleword)(a[3]) * (doubleword)(b[0])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[3] = V;
    UV = c_temp[3 + 1] + ((doubleword)(a[3]) * (doubleword)(b[1])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[3 + 1] = V;
    UV = c_temp[3 + 2] + ((doubleword)(a[3]) * (doubleword)(b[2])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[3 + 2] = V;
    UV = c_temp[3 + 3] + ((doubleword)(a[3]) * (doubleword)(b[3])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[3 + 3] = V;
    UV = c_temp[3 + 4] + ((doubleword)(a[3]) * (doubleword)(b[4])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[3 + 4] = V;
    UV = c_temp[3 + 5] + ((doubleword)(a[3]) * (doubleword)(b[5])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[3 + 5] = V;
    c_temp[3 + 6] = U;

    U = 0;
    UV = c_temp[4] + ((doubleword)(a[4]) * (doubleword)(b[0])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[4] = V;
    UV = c_temp[4 + 1] + ((doubleword)(a[4]) * (doubleword)(b[1])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[4 + 1] = V;
    UV = c_temp[4 + 2] + ((doubleword)(a[4]) * (doubleword)(b[2])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[4 + 2] = V;
    UV = c_temp[4 + 3] + ((doubleword)(a[4]) * (doubleword)(b[3])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[4 + 3] = V;
    UV = c_temp[4 + 4] + ((doubleword)(a[4]) * (doubleword)(b[4])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[4 + 4] = V;
    UV = c_temp[4 + 5] + ((doubleword)(a[4]) * (doubleword)(b[5])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[4 + 5] = V;
    c_temp[4 + 6] = U;

    U = 0;
    UV = c_temp[5] + ((doubleword)(a[5]) * (doubleword)(b[0])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[5] = V;
    UV = c_temp[5 + 1] + ((doubleword)(a[5]) * (doubleword)(b[1])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[5 + 1] = V;
    UV = c_temp[5 + 2] + ((doubleword)(a[5]) * (doubleword)(b[2])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[5 + 2] = V;
    UV = c_temp[5 + 3] + ((doubleword)(a[5]) * (doubleword)(b[3])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[5 + 3] = V;
    UV = c_temp[5 + 4] + ((doubleword)(a[5]) * (doubleword)(b[4])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[5 + 4] = V;
    UV = c_temp[5 + 5] + ((doubleword)(a[5]) * (doubleword)(b[5])) + U;
    U = UV >> WORD_SIZE;
    V = (UV << WORD_SIZE) >> WORD_SIZE;
    c_temp[5 + 5] = V;
    c_temp[5 + 6] = U;
  }
}

void BigIntLib::MulPF384PFast(word* c, word* a, word* b) {
  // Implement static MUL in PF with 384 bits followed by static reduction using a generalized mersenne prime...
  //memset(c, 0, BigIntLib::field_num_words_ * (WORD_SIZE / 8)); // Probably not needed
  word c_temp[12];
  Mul384_768(c_temp, a, b);

  // Reduction
  uchar* pointer = (uchar*)c_temp;
  word temp_var[6];
  uchar* temp_var_pointer = (uchar*)temp_var;
  for(uint32 i = 0; i < 6; i++) { // better with memset
    c[i] = 0;
    temp_var[i] = 0;
  }

  // 2^384 = 2^128 + 2^96 - 2^32 + 1
  // t^12 = t^4 + t^3 - t + 1 where t = 2^32
  // Modular Additions and Subtractions
  // + 12 13 14 15 16 17 18 19 20 21 22 23
  // -    12 13 14 15 16 17 18 19 20 21 22 23
  // +          12 13 14 15 16 17 18 19 20 21 22 23
  // +             12 13 14 15 16 17 18 19 20 21 22 23
  //
  // + 12 13 14 15 16 17 18 19 20 21 22 23
  // - 23 12 13 14 15 16 17 18 19 20 21 22
  // + 21 22 23 12 13 14 15 16 17 18 19 20
  // + 20 21 22 23 12 13 14 15 16 17 18 19
  // +    23
  // -    21 22 23
  // -    20 21 22 23
  // -          23
  // +          21 22 23
  // +          20 21 22 23
  // -             23
  // +             21 22 23
  // +             20 21 22 23
  //
  // + 12 13 14 15 16 17 18 19 20 21 22 23
  // - 23 12 13 14 15 16 17 18 19 20 21 22
  // + 21 22 23 12 13 14 15 16 17 18 19 20
  // + 20 21 22 23 12 13 14 15 16 17 18 19
  // +             21 22 23
  // +             20 21 22 23
  // +          20 21 22 23
  // +    23    21 22 23
  // -    21 22 23
  // -    20 21 22 23
  // -          23 23
  //
  // c <- s0
  memcpy(c, pointer, 48);

  // First addition
  memcpy(temp_var_pointer, pointer + 48, 48);
  BigIntLib::AddPF384(c, c, temp_var);

  // First subtraction
  memcpy(temp_var_pointer, pointer + 92, 4);
  memcpy(temp_var_pointer + 4, pointer + 48, 44);
  BigIntLib::SubPF384(c, c, temp_var);

  // Second addition
  memcpy(temp_var_pointer, pointer + 84, 12);
  memcpy(temp_var_pointer + 12, pointer + 48, 36);
  BigIntLib::AddPF384(c, c, temp_var);

  // Third addition
  memcpy(temp_var_pointer, pointer + 80, 16);
  memcpy(temp_var_pointer + 16, pointer + 48, 32);
  BigIntLib::AddPF384(c, c, temp_var);

  // 4th addition
  memset(temp_var_pointer, 0, 48);
  memcpy(temp_var_pointer + 16, pointer + 84, 12);
  BigIntLib::AddPF384(c, c, temp_var);

  // 5th addition
  memcpy(temp_var_pointer + 16, pointer + 80, 16);
  BigIntLib::AddPF384(c, c, temp_var);

  // 6th addition
  memcpy(temp_var_pointer + 12, pointer + 80, 16);
  memset(temp_var_pointer + 28, 0, 4);
  BigIntLib::AddPF384(c, c, temp_var);

  // 7th addition
  memcpy(temp_var_pointer + 4, pointer + 92, 4);
  memcpy(temp_var_pointer + 12, pointer + 84, 12);
  memset(temp_var_pointer + 24, 0, 4);
  BigIntLib::AddPF384(c, c, temp_var);

  // Second subtraction
  memcpy(temp_var_pointer + 4, pointer + 84, 12);
  memset(temp_var_pointer + 16, 0, 8);
  BigIntLib::SubPF384(c, c, temp_var);

  // Third subtraction
  memcpy(temp_var_pointer + 4, pointer + 80, 16);
  BigIntLib::SubPF384(c, c, temp_var);

  // 4th subtraction
  memset(temp_var_pointer + 4, 0, 8);
  memcpy(temp_var_pointer + 12, pointer + 92, 4);
  BigIntLib::SubPF384(c, c, temp_var);
}

void BigIntLib::MulPF384PFast1(word* c, word* a, word* b) {
  // Implement static MUL in PF with 384 bits followed by static reduction using a generalized mersenne prime...
  //memset(c, 0, BigIntLib::field_num_words_ * (WORD_SIZE / 8)); // Probably not needed
  word c_temp[12];
  Mul384_768(c_temp, a, b);

  // Reduction
  // 2^384 = 2^128 + 2^96 - 2^32 + 1
  // t^12 = t^4 + t^3 - t + 1 where t = 2^32

  word d[24];
  for (uint32 i = 0, j = 0; i < 12; i++) {
    d[j++] = c_temp[i] & MASK32;
    d[j++] = c_temp[i] >> 32;
  }

  word e[12];
  word carry;
  e[11] = d[11]           + d[23] + d[20] + d[19]                 - d[22];
  carry = (word)((signedword)e[11] >> 32); // need shift right with sign extension here
  e[11] &= MASK32;
  e[0]  = d[0]            + d[12] + d[21]                 + d[20] - d[23]                 + carry;
  e[1]  = d[1]            + d[13] + d[22] + d[23]                 - d[12] - d[20]         - carry;
  e[2]  = d[2]            + d[14] + d[23]                         - d[13] - d[21];
  e[3]  = d[3]            + d[15] + d[12] + d[20]         + d[21] - d[14] - d[22] - d[23] + carry;
  e[4]  = d[4]  + 2*d[21] + d[16] + d[13] + d[12] + d[20] + d[22] - d[15] - 2*d[23]       + carry;
  e[5]  = d[5]  + 2*d[22] + d[17] + d[14] + d[13] + d[21] + d[23] - d[16];
  e[6]  = d[6]  + 2*d[23] + d[18] + d[15] + d[14] + d[22]         - d[17];
  e[7]  = d[7]            + d[19] + d[16] + d[15] + d[23]         - d[18];
  e[8]  = d[8]            + d[20] + d[17] + d[16]                 - d[19];
  e[9]  = d[9]            + d[21] + d[18] + d[17]                 - d[20];
  e[10] = d[10]           + d[22] + d[19] + d[18]                 - d[21];

  word t, u;
  carry = 0;
  for (uint32 i = 0, j = 0; i < 6; i++) {
    t = carry + e[j++];
    carry = (word)((signedword)t >> 32); // need shift right with sign extension here
    u = carry + e[j++];
    carry = (word)((signedword)u >> 32); // need shift right with sign extension here
    c[i] = (u << 32) ^ (t & MASK32);
  }
  if (GreaterEq384(c, BigIntLib::modulo_)) {
    word borrow = 0;
    doubleword tmp;
    tmp = (doubleword)c[0] - BigIntLib::modulo_[0] - borrow;
    c[0] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    borrow = (tmp >> WORD_SIZE) != 0;
    tmp = (doubleword)c[1] - BigIntLib::modulo_[1] - borrow;
    c[1] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    borrow = (tmp >> WORD_SIZE) != 0;
    tmp = (doubleword)c[2] - BigIntLib::modulo_[2] - borrow;
    c[2] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    borrow = (tmp >> WORD_SIZE) != 0;
    tmp = (doubleword)c[3] - BigIntLib::modulo_[3] - borrow;
    c[3] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    borrow = (tmp >> WORD_SIZE) != 0;
    tmp = (doubleword)c[4] - BigIntLib::modulo_[4] - borrow;
    c[4] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    borrow = (tmp >> WORD_SIZE) != 0;
    tmp = (doubleword)c[5] - BigIntLib::modulo_[5] - borrow;
    c[5] = ((tmp << WORD_SIZE) >> WORD_SIZE);
    // borrow = (tmp >> WORD_SIZE);
  }
  while (carry >> 63) {
    SubPF384(c, c, minus_modulo_);
    carry++;
  }
  while (carry > 0) {
    AddPF384(c, c, minus_modulo_);
    carry--;
  }
}

#define MINMOD384P0 0xffffffff00000001
#define MINMOD384P1 0x00000000ffffffff
#define MINMOD384P2 1

#define MINMOD384Q0 0x1313e695333ad68d
#define MINMOD384Q1 0xa7e5f24db74f5885
#define MINMOD384Q2 0x389cb27e0bc8d220

void BigIntLib::MulPF384PFast2(word* c, word* a, word* b) {
  // Implement static MUL in PF with 384 bits followed by static reduction using a generalized mersenne prime...
  //memset(c, 0, BigIntLib::field_num_words_ * (WORD_SIZE / 8)); // Probably not needed
  word U;
  word V;
  doubleword UV;
  word c_temp[12];
  Mul384_768(c_temp, a, b);

  word c_temp2[9];
  word* c_temp_upper = c_temp + 6;
  // Compute c_temp2 as the product of the lower half of minus_modulo_ and the upper half of c_temp.
  // minus_modulo_ is 2^384 - P and only the lower 3 words can be nonzero.
  memset(c_temp2, 0, sizeof(c_temp2));

  //for(uint32 i = 0; i < 3; i++) {
  //  U = 0;
  //  for(uint32 j = 0; j < 6; j++) {
  //    UV = c_temp2[i + j] + ((doubleword)(minus_modulo_[i]) * (doubleword)(c_temp_upper[j])) + U;
  //    U = UV >> WORD_SIZE;
  //    V = (UV << WORD_SIZE) >> WORD_SIZE;
  //    c_temp2[i + j] = V;
  //  }
  //  c_temp2[i + 6] = U;
  //}

  U = 0;
  UV = c_temp2[0] + (MINMOD384P0 * (doubleword)(c_temp_upper[0])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[0] = V;
  UV = c_temp2[0 + 1] + (MINMOD384P0 * (doubleword)(c_temp_upper[1])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[0 + 1] = V;
  UV = c_temp2[0 + 2] + (MINMOD384P0 * (doubleword)(c_temp_upper[2])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[0 + 2] = V;
  UV = c_temp2[0 + 3] + (MINMOD384P0 * (doubleword)(c_temp_upper[3])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[0 + 3] = V;
  UV = c_temp2[0 + 4] + (MINMOD384P0 * (doubleword)(c_temp_upper[4])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[0 + 4] = V;
  UV = c_temp2[0 + 5] + (MINMOD384P0 * (doubleword)(c_temp_upper[5])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[0 + 5] = V;
  c_temp2[0 + 6] = U;

  U = 0;
  UV = c_temp2[1] + (MINMOD384P1 * (doubleword)(c_temp_upper[0])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[1] = V;
  UV = c_temp2[1 + 1] + (MINMOD384P1 * (doubleword)(c_temp_upper[1])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[1 + 1] = V;
  UV = c_temp2[1 + 2] + (MINMOD384P1 * (doubleword)(c_temp_upper[2])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[1 + 2] = V;
  UV = c_temp2[1 + 3] + (MINMOD384P1 * (doubleword)(c_temp_upper[3])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[1 + 3] = V;
  UV = c_temp2[1 + 4] + (MINMOD384P1 * (doubleword)(c_temp_upper[4])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[1 + 4] = V;
  UV = c_temp2[1 + 5] + (MINMOD384P1 * (doubleword)(c_temp_upper[5])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[1 + 5] = V;
  c_temp2[1 + 6] = U;

  U = 0;
  UV = c_temp2[2] + ((doubleword)(c_temp_upper[0])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[2] = V;
  UV = c_temp2[2 + 1] + ((doubleword)(c_temp_upper[1])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[2 + 1] = V;
  UV = c_temp2[2 + 2] + ((doubleword)(c_temp_upper[2])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[2 + 2] = V;
  UV = c_temp2[2 + 3] + ((doubleword)(c_temp_upper[3])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[2 + 3] = V;
  UV = c_temp2[2 + 4] + ((doubleword)(c_temp_upper[4])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[2 + 4] = V;
  UV = c_temp2[2 + 5] + ((doubleword)(c_temp_upper[5])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[2 + 5] = V;
  c_temp2[2 + 6] = U;

  word c_temp3[6];
  word* c_temp2_upper = c_temp2 + 6;
  // Compute c_temp3 as the product of the lower half of minus_modulo_ and the upper 3 words of c_temp2.
  memset(c_temp3, 0, sizeof(c_temp3));

  //for(uint32 i = 0; i < 3; i++) {
  //  U = 0;
  //  for(uint32 j = 0; j < 3; j++) {
  //    UV = c_temp3[i + j] + ((doubleword)(minus_modulo_[i]) * (doubleword)(c_temp2_upper[j])) + U;
  //    U = UV >> WORD_SIZE;
  //    V = (UV << WORD_SIZE) >> WORD_SIZE;
  //    c_temp3[i + j] = V;
  //  }
  //  c_temp3[i + 3] = U;
  //}

  U = 0;
  UV = c_temp3[0] + (MINMOD384P0 * (doubleword)(c_temp2_upper[0])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp3[0] = V;
  UV = c_temp3[0 + 1] + (MINMOD384P0 * (doubleword)(c_temp2_upper[1])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp3[0 + 1] = V;
  UV = c_temp3[0 + 2] + (MINMOD384P0 * (doubleword)(c_temp2_upper[2])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp3[0 + 2] = V;
  c_temp3[0 + 3] = U;

  U = 0;
  UV = c_temp3[1] + (MINMOD384P1 * (doubleword)(c_temp2_upper[0])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp3[1] = V;
  UV = c_temp3[1 + 1] + (MINMOD384P1 * (doubleword)(c_temp2_upper[1])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp3[1 + 1] = V;
  UV = c_temp3[1 + 2] + (MINMOD384P1 * (doubleword)(c_temp2_upper[2])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp3[1 + 2] = V;
  c_temp3[1 + 3] = U;

  U = 0;
  UV = c_temp3[2] + ((doubleword)(c_temp2_upper[0])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp3[2] = V;
  UV = c_temp3[2 + 1] + ((doubleword)(c_temp2_upper[1])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp3[2 + 1] = V;
  UV = c_temp3[2 + 2] + ((doubleword)(c_temp2_upper[2])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp3[2 + 2] = V;
  c_temp3[2 + 3] = U;

  // Add together the lower field_num_words_ of c_temp, c_temp2, and c_temp3.
  // Add c_temp3 first, as c_temp + c_temp2 can be >= 2^384 + P and then P would need to be subtracted twice but is subtracted only once, giving a wrong result.
  // The highest bit of c_temp3 is always 0, so c_temp + c_temp3 is always less than 2^384 + P and c_temp + c_temp3 + c_temp2 is always less than 3P.
  BigIntLib::AddPF384(c, c_temp, c_temp3);
  BigIntLib::AddPF384(c, c, c_temp2);
}

void BigIntLib::MulPF384QFast(word* c, word* a, word* b) {
  // Implement static MUL in PF with 384 bits followed by static reduction using a generalized mersenne prime...
  //memset(c, 0, BigIntLib::field_num_words_ * (WORD_SIZE / 8)); // Probably not needed
  word U;
  word V;
  doubleword UV;
  word c_temp[12];
  Mul384_768(c_temp, a, b);

  word c_temp2[9];
  word* c_temp_upper = c_temp + 6;
  // Compute c_temp2 as the product of the lower half of minus_modulo_ and the upper half of c_temp.
  // minus_modulo_ is 2^384 - Q and only the lower 3 words can be nonzero.
  memset(c_temp2, 0, sizeof(c_temp2));

  //for(uint32 i = 0; i < 3; i++) {
  //  U = 0;
  //  for(uint32 j = 0; j < 6; j++) {
  //    UV = c_temp2[i + j] + ((doubleword)(minus_modulo_[i]) * (doubleword)(c_temp_upper[j])) + U;
  //    U = UV >> WORD_SIZE;
  //    V = (UV << WORD_SIZE) >> WORD_SIZE;
  //    c_temp2[i + j] = V;
  //  }
  //  c_temp2[i + 6] = U;
  //}

  U = 0;
  UV = c_temp2[0] + (MINMOD384Q0 * (doubleword)(c_temp_upper[0])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[0] = V;
  UV = c_temp2[0 + 1] + (MINMOD384Q0 * (doubleword)(c_temp_upper[1])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[0 + 1] = V;
  UV = c_temp2[0 + 2] + (MINMOD384Q0 * (doubleword)(c_temp_upper[2])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[0 + 2] = V;
  UV = c_temp2[0 + 3] + (MINMOD384Q0 * (doubleword)(c_temp_upper[3])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[0 + 3] = V;
  UV = c_temp2[0 + 4] + (MINMOD384Q0 * (doubleword)(c_temp_upper[4])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[0 + 4] = V;
  UV = c_temp2[0 + 5] + (MINMOD384Q0 * (doubleword)(c_temp_upper[5])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[0 + 5] = V;
  c_temp2[0 + 6] = U;

  U = 0;
  UV = c_temp2[1] + (MINMOD384Q1 * (doubleword)(c_temp_upper[0])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[1] = V;
  UV = c_temp2[1 + 1] + (MINMOD384Q1 * (doubleword)(c_temp_upper[1])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[1 + 1] = V;
  UV = c_temp2[1 + 2] + (MINMOD384Q1 * (doubleword)(c_temp_upper[2])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[1 + 2] = V;
  UV = c_temp2[1 + 3] + (MINMOD384Q1 * (doubleword)(c_temp_upper[3])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[1 + 3] = V;
  UV = c_temp2[1 + 4] + (MINMOD384Q1 * (doubleword)(c_temp_upper[4])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[1 + 4] = V;
  UV = c_temp2[1 + 5] + (MINMOD384Q1 * (doubleword)(c_temp_upper[5])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[1 + 5] = V;
  c_temp2[1 + 6] = U;

  U = 0;
  UV = c_temp2[2] + (MINMOD384Q2 * (doubleword)(c_temp_upper[0])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[2] = V;
  UV = c_temp2[2 + 1] + (MINMOD384Q2 * (doubleword)(c_temp_upper[1])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[2 + 1] = V;
  UV = c_temp2[2 + 2] + (MINMOD384Q2 * (doubleword)(c_temp_upper[2])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[2 + 2] = V;
  UV = c_temp2[2 + 3] + (MINMOD384Q2 * (doubleword)(c_temp_upper[3])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[2 + 3] = V;
  UV = c_temp2[2 + 4] + (MINMOD384Q2 * (doubleword)(c_temp_upper[4])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[2 + 4] = V;
  UV = c_temp2[2 + 5] + (MINMOD384Q2 * (doubleword)(c_temp_upper[5])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp2[2 + 5] = V;
  c_temp2[2 + 6] = U;

  word c_temp3[6];
  word* c_temp2_upper = c_temp2 + 6;
  // Compute c_temp3 as the product of the lower half of minus_modulo_ and the upper 3 words of c_temp2.
  memset(c_temp3, 0, sizeof(c_temp3));

  //for(uint32 i = 0; i < 3; i++) {
  //  U = 0;
  //  for(uint32 j = 0; j < 3; j++) {
  //    UV = c_temp3[i + j] + ((doubleword)(minus_modulo_[i]) * (doubleword)(c_temp2_upper[j])) + U;
  //    U = UV >> WORD_SIZE;
  //    V = (UV << WORD_SIZE) >> WORD_SIZE;
  //    c_temp3[i + j] = V;
  //  }
  //  c_temp3[i + 3] = U;
  //}

  U = 0;
  UV = c_temp3[0] + (MINMOD384Q0 * (doubleword)(c_temp2_upper[0])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp3[0] = V;
  UV = c_temp3[0 + 1] + (MINMOD384Q0 * (doubleword)(c_temp2_upper[1])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp3[0 + 1] = V;
  UV = c_temp3[0 + 2] + (MINMOD384Q0 * (doubleword)(c_temp2_upper[2])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp3[0 + 2] = V;
  c_temp3[0 + 3] = U;

  U = 0;
  UV = c_temp3[1] + (MINMOD384Q1 * (doubleword)(c_temp2_upper[0])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp3[1] = V;
  UV = c_temp3[1 + 1] + (MINMOD384Q1 * (doubleword)(c_temp2_upper[1])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp3[1 + 1] = V;
  UV = c_temp3[1 + 2] + (MINMOD384Q1 * (doubleword)(c_temp2_upper[2])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp3[1 + 2] = V;
  c_temp3[1 + 3] = U;

  U = 0;
  UV = c_temp3[2] + (MINMOD384Q2 * (doubleword)(c_temp2_upper[0])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp3[2] = V;
  UV = c_temp3[2 + 1] + (MINMOD384Q2 * (doubleword)(c_temp2_upper[1])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp3[2 + 1] = V;
  UV = c_temp3[2 + 2] + (MINMOD384Q2 * (doubleword)(c_temp2_upper[2])) + U;
  U = UV >> WORD_SIZE;
  V = (UV << WORD_SIZE) >> WORD_SIZE;
  c_temp3[2 + 2] = V;
  c_temp3[2 + 3] = U;

  // Add together the lower field_num_words_ of c_temp, c_temp2, and c_temp3.
  // Add c_temp3 first, as c_temp + c_temp2 can be >= 2^384 + Q and then Q would need to be subtracted twice but is subtracted only once, giving a wrong result.
  // The highest bit of c_temp3 is always 0, so c_temp + c_temp3 is always less than 2^384 + Q and c_temp + c_temp3 + c_temp2 is always less than 3Q.
  BigIntLib::AddPF384(c, c_temp, c_temp3);
  BigIntLib::AddPF384(c, c, c_temp2);
}

void BigIntLib::AddEC(word* result, word* p1, word* p2) {
  uint32 nw = ringP_->gate_num_words_;
  uint32 nb = ringP_->gate_size_;
  word *(x[2]) = {p1, p1 + nw};
  word *(y[2]) = {p2, p2 + nw};
  word *(r[2]) = {result, result + nw};
  // the zero EC point is represented as (0,0)
  if (ringP_->IsZero(x[0]) && ringP_->IsZero(x[1])) {
    if (result != p2) {
      memcpy(result, p2, 2*nb);
    }
    return;
  }
  if (ringP_->IsZero(y[0]) && ringP_->IsZero(y[1])) {
    if (result != p1)
      memcpy(result, p1, 2*nb);
    return;
  }
  word x0_minus_y0[nw];
  ringP_->Sub(x0_minus_y0, x[0], y[0]);
  word s[nw];
  if (ringP_->IsZero(x0_minus_y0)) {
    word x1_plus_y1[nw];
    ringP_->Add(x1_plus_y1, x[1], y[1]);
    if (ringP_->IsZero(x1_plus_y1)) {
      memset(result, 0, 2*nb);
      return;
    }
    word inverse_x1_plus_y1[nw];
    ringP_->Inverse(ringBW_, inverse_x1_plus_y1, x1_plus_y1);
    word x02[nw];
    ringP_->Mul(x02, x[0], x[0]);
    word t[nw];
    ringP_->Add(t, x02, x02);
    ringP_->Add(t, t, x02);
    word three[nw];
    memset(three, 0, nb);
    three[0] = 3;
    ringP_->Sub(t, t, three);
    ringP_->Mul(s, t, inverse_x1_plus_y1);
  } else {
    word inverse_x0_minus_y0[nw];
    ringP_->Inverse(ringBW_, inverse_x0_minus_y0, x0_minus_y0);
    word x1_minus_y1[nw];
    ringP_->Sub(x1_minus_y1, x[1], y[1]);
    ringP_->Mul(s, x1_minus_y1, inverse_x0_minus_y0);
  }
  word s2[nw];
  ringP_->Mul(s2, s, s);
  word x0_plus_y0[nw];
  ringP_->Add(x0_plus_y0, x[0], y[0]);
  word r0[nw];
  ringP_->Sub(r0, s2, x0_plus_y0);
  word t1[nw];
  ringP_->Sub(t1, x[0], r0);
  ringP_->Mul(t1, t1, s);
  ringP_->Sub(r[1], t1, x[1]);
  memcpy(r[0], r0, nb);
}

void BigIntLib::NegateEC(word* result, word* p) {
  word zero[ringP_->gate_num_words_];
  memset(zero, 0, ringP_->gate_size_);
  memcpy(result, p, ringP_->gate_size_);
  ringP_->Sub(result + ringP_->gate_num_words_, zero, p + ringP_->gate_num_words_);
}

void BigIntLib::SubEC(word* result, word* p1, word* p2) {
  word minus_p2[2*ringP_->gate_num_words_];
  NegateEC(minus_p2, p2);
  AddEC(result, p1, minus_p2);
}

//void BigIntLib::ComputeReductionMatrix(int* t_coeff) {
//  // Method due to Jerome A. Solinas, "Generalized Mersenne Numbers", 1999
//  // The result is a n x n matrix of int numbers, where n is the number of terms (depends on word size)
//  // E.g.: 2^64 - 2^8 - 1 = t^8 - t - 1, where t = 2^8
//  uint32 n = BigIntLib::fastreduc_num_words_;
//  //int t_coeff[n]; // TODO Hard-coded
//  //memset(t_coeff, 0, sizeof(int) * n); // TODO Hard-coded
//  //t_coeff[0] = -1; // TODO Hard-coded
//  //t_coeff[1] = -1; // TODO Hard-coded
//
//  // Allocate memory
//  BigIntLib::reduction_matrix_ = new int*[n];
//  for(uint32 i = 0; i < n; i++) {
//    BigIntLib::reduction_matrix_[i] = new int[n];
//  }
//  BigIntLib::reduction_matrix_positive_ = new int*[n];
//  for(uint32 i = 0; i < n; i++) {
//    BigIntLib::reduction_matrix_positive_[i] = new int[n];
//  }
//  BigIntLib::reduction_matrix_negative_ = new int*[n];
//  for(uint32 i = 0; i < n; i++) {
//    BigIntLib::reduction_matrix_negative_[i] = new int[n];
//  }
//
//  for(uint32 j = 0; j < n; j++) {
//    BigIntLib::reduction_matrix_[0][j] = -(t_coeff[j]);
//  }
//  for(uint32 i = 1; i < n; i++) {
//    for(uint32 j = 0; j < n; j++) {
//      if(j == 0) {
//        BigIntLib::reduction_matrix_[i][j] = -(t_coeff[0]) * BigIntLib::reduction_matrix_[i - 1][n - 1];
//      }
//      else {
//        BigIntLib::reduction_matrix_[i][j] = BigIntLib::reduction_matrix_[i - 1][j - 1] - (t_coeff[j] * BigIntLib::reduction_matrix_[i - 1][n - 1]);
//      }
//    }
//  }
//
//  // Split in positive and negative part
//  int val_temp;
//  for(uint32 i = 0; i < n; i++) {
//    for(uint32 j = 0; j < n; j++) {
//      val_temp = BigIntLib::reduction_matrix_[i][j];
//      if(val_temp > 0) {
//        BigIntLib::reduction_matrix_positive_[i][j] = val_temp;
//        BigIntLib::reduction_matrix_negative_[i][j] = 0;
//      }
//      else if(val_temp < 0) {
//        BigIntLib::reduction_matrix_positive_[i][j] = 0;
//        BigIntLib::reduction_matrix_negative_[i][j] = val_temp;
//      }
//      else {
//        BigIntLib::reduction_matrix_positive_[i][j] = 0;
//        BigIntLib::reduction_matrix_negative_[i][j] = 0;
//      }
//      //if(BigIntLib::reduction_matrix_positive_[i][j] == 2) BigIntLib::reduction_matrix_positive_[i][j] = 1; // TEMP TESTING
//    }
//  }
//
//  // Get modular addition weight and modular subtraction weight
//  int max_addition = 0;
//  int current_addition;
//  int max_subtraction = 0;
//  int current_subtraction;
//  for(uint32 j = 0; j < n; j++) {
//    current_addition = 0;
//    current_subtraction = 0;
//    for(uint32 i = 0; i < n; i++) {
//      current_addition += BigIntLib::reduction_matrix_positive_[i][j];
//      current_subtraction += BigIntLib::reduction_matrix_negative_[i][j];
//    }
//    if(current_addition > max_addition) max_addition = current_addition;
//    if(current_subtraction < max_subtraction) max_subtraction = current_subtraction;
//  }
//  BigIntLib::mod_addition_weight_ = max_addition;
//  BigIntLib::mod_subtraction_weight_ = max_subtraction * (-1);
//
//  #ifdef VERBOSE
//  // Print reduction matrix
//  std::cout << "Computed reduction matrix X:" << std::endl;
//  for(uint32 i = 0; i < n; i++) {
//    for(uint32 j = 0; j < n; j++) {
//       std::cout << BigIntLib::reduction_matrix_[i][j] << "  ";
//    }
//    std::cout << std::endl;
//  }
//
//  // Print positive part
//  std::cout << "Positive part X+ of reduction matrix X:" << std::endl;
//  for(uint32 i = 0; i < n; i++) {
//    for(uint32 j = 0; j < n; j++) {
//       std::cout << BigIntLib::reduction_matrix_positive_[i][j] << "  ";
//    }
//    std::cout << std::endl;
//  }
//
//  // Print Negative part
//  std::cout << "Negative part X- of reduction matrix X:" << std::endl;
//  for(uint32 i = 0; i < n; i++) {
//    for(uint32 j = 0; j < n; j++) {
//       std::cout << BigIntLib::reduction_matrix_negative_[i][j] << "  ";
//    }
//    std::cout << std::endl;
//  }
//
//  std::cout << "Modular addition weight: " << BigIntLib::mod_addition_weight_ << std::endl;
//  std::cout << "Modular subtraction weight: " << BigIntLib::mod_subtraction_weight_ << std::endl;
//  #endif
//
//}
//
//void BigIntLib::ComputeModularAdditionMatrix() {
//  // Method due to Jerome A. Solinas, "Generalized Mersenne Numbers", 1999
//  // Specific algorithm due to Mario Taschwer, "Modular Multiplication Using Special Prime Moduli", ‎2001
//  //uint32 mod_addition_weight = 3; // TODO Hard-coded
//  uint32 mod_weight = BigIntLib::mod_addition_weight_;
//  uint32 n = BigIntLib::fastreduc_num_words_;
//
//  // Allocate memory
//  BigIntLib::mod_addition_matrix_ = new int*[mod_weight];
//  for(uint32 i = 0; i < mod_weight; i++) {
//    BigIntLib::mod_addition_matrix_[i] = new int[n];
//  }
//  
//  int i;
//  int val_temp;
//  for(uint32 k = 0; k < mod_weight; k++) {
//    for(uint32 j = 0; j < n; j++) {
//      i = 0;
//      while(i < n && BigIntLib::reduction_matrix_positive_[i][j] == 0) {
//        i++;
//      }
//      if(i < n) {
//        val_temp = n + i;
//        if(val_temp == (n * 2)) val_temp = -1; // TEMP
//        BigIntLib::mod_addition_matrix_[k][j] = val_temp;
//        BigIntLib::reduction_matrix_positive_[i][j]--;
//      }
//      else {
//        val_temp = 2 * n;
//        if(val_temp == (n * 2)) val_temp = -1; // TEMP
//        BigIntLib::mod_addition_matrix_[k][j] = val_temp;
//      }
//    }
//  }
//
//  #ifdef VERBOSE
//  // Print modular addition matrix
//  std::cout << "Computed modular addition matrix A+:" << std::endl;
//  for(uint32 i = 0; i < mod_weight; i++) {
//    for(uint32 j = 0; j < n; j++) {
//      std::cout << BigIntLib::mod_addition_matrix_[i][j] << "  ";
//    }
//    std::cout << std::endl;
//  }
//  #endif
//}
//
//void BigIntLib::ComputeModularSubtractionMatrix() {
//  // Method due to Jerome A. Solinas, "Generalized Mersenne Numbers", 1999
//  //uint32 mod_addition_weight = 3; // TODO Hard-coded
//  uint32 mod_weight = BigIntLib::mod_subtraction_weight_;
//  uint32 n = BigIntLib::fastreduc_num_words_;
//
//  // Allocate memory
//  BigIntLib::mod_subtraction_matrix_ = new int*[mod_weight];
//  for(uint32 i = 0; i < mod_weight; i++) {
//    BigIntLib::mod_subtraction_matrix_[i] = new int[n];
//  }
//  
//  int i;
//  int val_temp;
//  for(uint32 k = 0; k < mod_weight; k++) {
//    for(uint32 j = 0; j < n; j++) {
//      i = 0;
//      while(i < n && BigIntLib::reduction_matrix_negative_[i][j] == 0) {
//        i++;
//      }
//      if(i < n) {
//        val_temp = n + i;
//        if(val_temp == (n * 2)) val_temp = -1; // TEMP
//        BigIntLib::mod_subtraction_matrix_[k][j] = val_temp;
//        BigIntLib::reduction_matrix_negative_[i][j]++; // Difference to the calculation of the modular addition matrix
//      }
//      else {
//        val_temp = 2 * n;
//        if(val_temp == (n * 2)) val_temp = -1; // TEMP
//        BigIntLib::mod_subtraction_matrix_[k][j] = val_temp;
//      }
//    }
//  }
//
//  #ifdef VERBOSE
//  // Print modular addition matrix
//  std::cout << "Computed modular subtraction matrix A-:" << std::endl;
//  for(uint32 i = 0; i < mod_weight; i++) {
//    for(uint32 j = 0; j < n; j++) {
//       std::cout << BigIntLib::mod_subtraction_matrix_[i][j] << "  ";
//    }
//    std::cout << std::endl;
//  }
//  #endif
//}
//
//void BigIntLib::XorBF3(word* c, word* a, word* b) {
//  *c = *a ^ *b;
//}
//
//void BigIntLib::MulBF3Fast(word* c, word* a, word* b) {
//  /*
//  doubleword r = b[0];
//  asm("pclmulqdq %2, %1, %0;"
//    : "+x"(r)
//    : "x"(a[0]), "i"(0)
//    );
//  */
//  doubleword r = 0;
//  asm("pclmulqdq %2, %1, %0;"
//    : "=x"(r)
//    : "x"(a[0]), "i"(0), "0"(b[0])
//    );
//  
//  word c0 = r & 0x7; // LS 3 bits
//  word c1 = (r & 0x18) >> 3; // MS 2 bits
//
//  // Add c1 to bits 1 and 0 of c0 (c1 doesn't neet to be done first, because it's not affected)
//  *c = c0 ^ (c1 << 1) ^ c1;
//}
//
//void BigIntLib::Times2BF3(word* c, word* a) {
//  word r = 0;
//  memcpy(&r, a, BigIntLib::field_size_bytes_);
//  r <<= 1;
//  // Bits 1, 0
//  word T = r >> 3;
//  *c = (r ^ (T << 1) ^ T) & 0x7;
//}
//
//void BigIntLib::Times3BF3(word* c, word* a) {
//  word r = 0;
//  memcpy(&r, a, BigIntLib::field_size_bytes_);
//  r <<= 2;
//  // Bits 1, 0
//  word T = r >> 3;
//  *c = (r ^ (T << 1) ^ T) & 0x7;
//}
//
//void BigIntLib::XorBF17(word* c, word* a, word* b) {
//  *c = *a ^ *b;
//}
//
//void BigIntLib::MulBF17Fast(word* c, word* a, word* b) {
//  
//  doubleword r = 0;
//  asm("pclmulqdq %2, %1, %0;"
//    : "=x"(r)
//    : "x"(a[0]), "i"(0), "0"(b[0])
//    );
//  
//  // Reduction
//  // p(x) = x^17 + x^3 + 1
//  /*
//  word r_0_n_1 = r & 0x1FFFF;
//  word r_n_deg_r = r >> 17;
//  word x_m_1 = 0x9; // = 2^3 + 1
//  doubleword t = 0;
//  asm("pclmulqdq %2, %1, %0;"
//    : "=x"(t)
//    : "x"(r_n_deg_r), "i"(0), "0"(x_m_1)
//    );
//  r = r_0_n_1 ^ t;
//  t = r >> 17;
//  r = r ^ (t << 3) ^ t;
//  *c = r & 0x1FFFF;
//  */
//  word c0 = r & 0x1FFFF; // LS 17 bits
//  word c1 = r >> 17; // MS 16 bits
//  
//  c1 = c1 ^ (c1 >> 14);
//  // Add c1 to bits 3 and 0 of c0
//  c0 = c0 ^ (c1 << 3) ^ c1;
//  
//  // Build result
//  *c = c0 & 0x1FFFF;
//}
//
//void BigIntLib::Times2BF17(word* c, word* a) {
//  word r = 0;
//  memcpy(&r, a, BigIntLib::field_size_bytes_);
//  r <<= 1;
//  // Bits 3, 0
//  word T = r >> 17;
//  *c = (r ^ (T << 3) ^ T) & 0x1FFFF;
//}
//
//void BigIntLib::Times3BF17(word* c, word* a) {
//  word r = 0;
//  memcpy(&r, a, BigIntLib::field_size_bytes_);
//  r <<= 2;
//  // Bits 3, 0
//  word T = r >> 17;
//  *c = (r ^ (T << 3) ^ T) & 0x1FFFF;
//}
//
//void BigIntLib::XorBF33(word* c, word* a, word* b) {
//  *c = *a ^ *b;
//}
//
//void BigIntLib::MulBF33Fast(word* c, word* a, word* b) {
//  /*
//  doubleword r = 0;
//  word b_temp = *b;
//
//  // Fast multiplication (Left-to-right comb method with windows of width w)
//  // Precompute u(x) * b(x) for all possible values of u(x) (w = 4, 16 possible values)
//  // This takes (2 * 16) - 15 = 17 XOR operations and (3 * 16) / 2 = 24 shifts
//  // Note: This can be implemented faster by reusing already computed results! E.g. B_u[3] = b_temp ^ (b_temp << 1) = b_temp ^ B_u[2]
//  doubleword B_u[16];
//  // 0: 0b0000
//  B_u[0] = 0x0;
//  // 1: 0b0001
//  B_u[1] = b_temp;
//  // 2: 0b0010
//  B_u[2] = (b_temp << 1);
//  // 3: 0b0011
//  B_u[3] = b_temp ^ (b_temp << 1);
//  // 4: 0b0100
//  B_u[4] = (b_temp << 2);
//  // 5: 0b0101
//  B_u[5] = b_temp ^ (b_temp << 2);
//  // 6: 0b0110
//  B_u[6] = (b_temp << 1) ^ (b_temp << 2);
//  // 7: 0b0111
//  B_u[7] = b_temp ^ (b_temp << 1) ^ (b_temp << 2);
//  // 8: 0b1000
//  B_u[8] = (b_temp << 3);
//  // 9: 0b1001
//  B_u[9] = b_temp ^ (b_temp << 3);
//  // 10: 0b1010
//  B_u[10] = (b_temp << 1) ^ (b_temp << 3);
//  // 11: 0b1011
//  B_u[11] = b_temp ^ (b_temp << 1) ^ (b_temp << 3);
//  // 12: 0b1100
//  B_u[12] = (b_temp << 2) ^ (b_temp << 3);
//  // 13: 0b1101
//  B_u[13] = b_temp ^ (b_temp << 2) ^ (b_temp << 3);
//  // 14: 0b1110
//  B_u[14] = (b_temp << 1) ^ (b_temp << 2) ^ (b_temp << 3);
//  // 15: 0b1111
//  B_u[15] = b_temp ^ (b_temp << 1) ^ (b_temp << 2) ^ (b_temp << 3);
//  */
//
//  // Unrolled version of the multiplication loop
//  /*
//  r = r ^ B_u[(*a >> 32) & 0xF];
//  r = r << 4;
//  r = r ^ B_u[(*a >> 28) & 0xF];
//  r = r << 4;
//  r = r ^ B_u[(*a >> 24) & 0xF];
//  r = r << 4;
//  r = r ^ B_u[(*a >> 20) & 0xF];
//  r = r << 4;
//  r = r ^ B_u[(*a >> 16) & 0xF];
//  r = r << 4;
//  r = r ^ B_u[(*a >> 12) & 0xF];
//  r = r << 4;
//  r = r ^ B_u[(*a >> 8) & 0xF];
//  r = r << 4;
//  r = r ^ B_u[(*a >> 4) & 0xF];
//  r = r << 4;
//  r = r ^ B_u[(*a) & 0xF];
//  */
//  
//  /*
//  // Optimized version
//  r = ((((((((((((((((B_u[(*a >> 32) & 0xF]) << 4) ^
//    B_u[(*a >> 28) & 0xF]) << 4) ^
//    B_u[(*a >> 24) & 0xF]) << 4) ^
//    B_u[(*a >> 20) & 0xF]) << 4) ^
//    B_u[(*a >> 16) & 0xF]) << 4) ^
//    B_u[(*a >> 12) & 0xF]) << 4) ^
//    B_u[(*a >> 8) & 0xF]) << 4) ^
//    B_u[(*a >> 4) & 0xF]) << 4) ^
//    B_u[(*a) & 0xF];
//  */
//
//  // (Experimental) Carry-Less multiplication using CPU instruction PCLMULQDQ
//  /*
//  doubleword r = b[0];
//  asm("pclmulqdq %2, %1, %0;"
//    : "+x"(r)
//    : "x"(a[0]), "i"(0)
//    );
//  */
//  doubleword r = 0;
//  asm("pclmulqdq %2, %1, %0;"
//    : "=x"(r)
//    : "x"(a[0]), "i"(0), "0"(b[0])
//    );
//
//  
//  
//  // Reduction
//  // x^33 = x^6 + x^3 + x + 1 mod p(x)
//  // x^64 = x^37 + x^34 + x^32 + x^31 mod p(x)
//  // Manipulate from higher words to lower words
//  
//  word c0 = r & 0x1FFFFFFFF; // LS 33 bits
//  word c1 = r >> 33; // MS 32 bits
//
//  word T = c1;
//  c1 = c1 ^ (T >> 27) ^ (T >> 30); // 27 = 33 - 6, 30 = 33 - 3, 32 = 33 - 1 (omitted, all zeros), x^0 does not affect c1
//  T = c1;
//  *c = c0 ^ ((T << 6) & 0x1FFFFFFFF) ^ ((T << 3) & 0x1FFFFFFFF) ^ ((T << 1) & 0x1FFFFFFFF) ^ T; // = c0, for x^6, x^3, x^1, x^0
//
//  // p(x) = x^33 + x^10 + 1
//  /*
//  word r_0_n_1 = r & 0x1FFFFFFFF;
//  word r_n_deg_r = r >> 33;
//  word x_m_1 = 0x401; // = 2^10 + 1
//  doubleword t = 0;
//  asm("pclmulqdq %2, %1, %0;"
//    : "=x"(t)
//    : "x"(r_n_deg_r), "i"(0), "0"(x_m_1)
//    );
//  r = r_0_n_1 ^ t;
//  t = r >> 33;
//  r = r ^ (t << 10) ^ t;
//  *c = r & 0x1FFFFFFFF;
//  */
//
//  /*
//  uint32 c0 = r & 0xFFFFFFFF; // first 32 bits
//  uint32 c1 = (r >> 32) & 0xFFFFFFFF; // second 32 bits
//  uint32 c2 = (r >> 64) & 0xFFFFFFFF; // third 32 bits (only 1 bit)
//
//  // Reduce LSB of c2 and add to bits 37, 34, 32 and 31 of c (because x^64 = x^37 + x^34 + x^32 + x^31 mod p(x))
//  uint32 T = c2;
//  c1 = c1 ^ (T << 5);
//  c1 = c1 ^ (T << 2);
//  c1 = c1 ^ T;
//  c0 = c0 ^ (T << 31);
//
//  // Reduce MS 31 bits of c1 and add to bits 6, 3, 1 and 0 of c (because x^33 = x^6 + x^3 + x + 1 mod p(x))
//  // Parts relevant for c1
//  T = c1 >> 1;
//  c1 = c1 ^ (T >> 26);
//  c1 = c1 ^ (T >> 29);
//  c1 = c1 ^ (T >> 31);
//  // Parts relevant for c0
//  T = c1 >> 1;
//  c0 = c0 ^ (T << 6);
//  c0 = c0 ^ (T << 3);
//  c0 = c0 ^ (T << 1);
//  c0 = c0 ^ T;
//
//  // Build result = (LSB of c1 || c0)
//  *c = ((word)(c1 & 0x1) << 32) | c0;
//  */
//}
//
//void BigIntLib::Times2BF33(word* c, word* a) {
//  word r = 0;
//  memcpy(&r, a, BigIntLib::field_size_bytes_);
//  r <<= 1;
//  // Bits 6, 3, 1, 0
//  word T = r >> 33;
//  *c = (r ^ (T << 6) ^ (T << 3) ^ (T << 1) ^ T) & 0x1FFFFFFFF;
//}
//
//void BigIntLib::Times3BF33(word* c, word* a) {
//  word r = 0;
//  memcpy(&r, a, BigIntLib::field_size_bytes_);
//  r <<= 2;
//  // Bits 6, 3, 1, 0
//  word T = r >> 33;
//  *c = (r ^ (T << 6) ^ (T << 3) ^ (T << 1) ^ T) & 0x1FFFFFFFF;
//}
//
//void BigIntLib::XorBF65(word* c, word* a, word* b) {
//  c[0] = a[0] ^ b[0];
//  c[1] = a[1] ^ b[1];
//}
//
//void BigIntLib::MulBF65Fast(word* c, word* a, word* b) {
//  //word r[3] = {0};
//  //doubleword b_temp = *((doubleword*)b);
//
//  // Slow multiplication (Right-to-left comb method)
//  /*
//  for(uint32 i = 0; i < 64; i++) { // Make this operation faster with window-based multiplication
//    if((((*a) & ((word)0x1 << i)) >> i) == 0x1)
//      r = r ^ b_temp;
//    if(i != 63)
//      b_temp = b_temp << 1;
//  }
//  */
//  
//
//  // Fast multiplication (Left-to-right comb method with windows of width w)
//  // Precompute u(x) * b(x) for all possible values of u(x) (w = 4, 16 possible values)
//  // This takes (2 * 16) - 15 = 17 XOR operations and (3 * 16) / 2 = 24 shifts
//  // Note: This can be implemented faster by reusing already computed results! E.g. B_u[3] = b_temp ^ (b_temp << 1) = b_temp ^ B_u[2]
//  /*
//  doubleword B_u[16];
//  // 0: 0b0000
//  B_u[0] = 0x0;
//  // 1: 0b0001
//  B_u[1] = b_temp;
//  // 2: 0b0010
//  B_u[2] = (b_temp << 1);
//  // 3: 0b0011
//  B_u[3] = b_temp ^ (b_temp << 1);
//  // 4: 0b0100
//  B_u[4] = (b_temp << 2);
//  // 5: 0b0101
//  B_u[5] = b_temp ^ (b_temp << 2);
//  // 6: 0b0110
//  B_u[6] = (b_temp << 1) ^ (b_temp << 2);
//  // 7: 0b0111
//  B_u[7] = b_temp ^ (b_temp << 1) ^ (b_temp << 2);
//  // 8: 0b1000
//  B_u[8] = (b_temp << 3);
//  // 9: 0b1001
//  B_u[9] = b_temp ^ (b_temp << 3);
//  // 10: 0b1010
//  B_u[10] = (b_temp << 1) ^ (b_temp << 3);
//  // 11: 0b1011
//  B_u[11] = b_temp ^ (b_temp << 1) ^ (b_temp << 3);
//  // 12: 0b1100
//  B_u[12] = (b_temp << 2) ^ (b_temp << 3);
//  // 13: 0b1101
//  B_u[13] = b_temp ^ (b_temp << 2) ^ (b_temp << 3);
//  // 14: 0b1110
//  B_u[14] = (b_temp << 1) ^ (b_temp << 2) ^ (b_temp << 3);
//  // 15: 0b1111
//  B_u[15] = b_temp ^ (b_temp << 1) ^ (b_temp << 2) ^ (b_temp << 3);
//  */
//
//  // Compute for each w-bit window, where w = 4
//  /*
//  uint32 u = 0;
//  for(int i = 15; i >= 0; i--) { // 16 = W / w = 64 / 4
//    u = (*a >> (i * 4)) & 0xF;
//    r = r ^ B_u[u];
//    if(i != 0)
//      r = r << 4;
//  }
//  */
//  // Unrolled version of the multiplication loop
//  //doubleword* r_dw_pointer = (doubleword*)r;
//  /*
//  *r_dw_pointer = *r_dw_pointer ^ B_u[(a[1]) & 0xF];
//  *r_dw_pointer = *r_dw_pointer << 4;
//  *r_dw_pointer = *r_dw_pointer ^ B_u[(a[0] >> 60) & 0xF];
//  *r_dw_pointer = *r_dw_pointer << 4;
//  *r_dw_pointer = *r_dw_pointer ^ B_u[(a[0] >> 56) & 0xF];
//  *r_dw_pointer = *r_dw_pointer << 4;
//  *r_dw_pointer = *r_dw_pointer ^ B_u[(a[0] >> 52) & 0xF];
//  *r_dw_pointer = *r_dw_pointer << 4;
//  *r_dw_pointer = *r_dw_pointer ^ B_u[(a[0] >> 48) & 0xF];
//  *r_dw_pointer = *r_dw_pointer << 4;
//  *r_dw_pointer = *r_dw_pointer ^ B_u[(a[0] >> 44) & 0xF];
//  *r_dw_pointer = *r_dw_pointer << 4;
//  *r_dw_pointer = *r_dw_pointer ^ B_u[(a[0] >> 40) & 0xF];
//  *r_dw_pointer = *r_dw_pointer << 4;
//  *r_dw_pointer = *r_dw_pointer ^ B_u[(a[0] >> 36) & 0xF];
//  *r_dw_pointer = *r_dw_pointer << 4;
//  *r_dw_pointer = *r_dw_pointer ^ B_u[(a[0] >> 32) & 0xF];
//  *r_dw_pointer = *r_dw_pointer << 4;
//  *r_dw_pointer = *r_dw_pointer ^ B_u[(a[0] >> 28) & 0xF];
//  *r_dw_pointer = *r_dw_pointer << 4;
//  *r_dw_pointer = *r_dw_pointer ^ B_u[(a[0] >> 24) & 0xF];
//  *r_dw_pointer = *r_dw_pointer << 4;
//  *r_dw_pointer = *r_dw_pointer ^ B_u[(a[0] >> 20) & 0xF];
//  *r_dw_pointer = *r_dw_pointer << 4;
//  *r_dw_pointer = *r_dw_pointer ^ B_u[(a[0] >> 16) & 0xF];
//  *r_dw_pointer = *r_dw_pointer << 4;
//  *r_dw_pointer = *r_dw_pointer ^ B_u[(a[0] >> 12) & 0xF];
//  *r_dw_pointer = *r_dw_pointer << 4;
//  *r_dw_pointer = *r_dw_pointer ^ B_u[(a[0] >> 8) & 0xF];
//  *r_dw_pointer = *r_dw_pointer << 4;
//  *r_dw_pointer = *r_dw_pointer ^ B_u[(a[0] >> 4) & 0xF];
//  */
//
//  // Optimized version
//  /*
//  doubleword* r_dw_pointer = (doubleword*)r;
//  *r_dw_pointer = ((((((((((((((((((((((((((((((B_u[(a[1]) & 0xF]) << 4) ^
//    B_u[(a[0] >> 60) & 0xF]) << 4) ^
//    B_u[(a[0] >> 56) & 0xF]) << 4) ^
//    B_u[(a[0] >> 52) & 0xF]) << 4) ^
//    B_u[(a[0] >> 48) & 0xF]) << 4) ^
//    B_u[(a[0] >> 44) & 0xF]) << 4) ^
//    B_u[(a[0] >> 40) & 0xF]) << 4) ^
//    B_u[(a[0] >> 36) & 0xF]) << 4) ^
//    B_u[(a[0] >> 32) & 0xF]) << 4) ^
//    B_u[(a[0] >> 28) & 0xF]) << 4) ^
//    B_u[(a[0] >> 24) & 0xF]) << 4) ^
//    B_u[(a[0] >> 20) & 0xF]) << 4) ^
//    B_u[(a[0] >> 16) & 0xF]) << 4) ^
//    B_u[(a[0] >> 12) & 0xF]) << 4) ^
//    B_u[(a[0] >> 8) & 0xF]) << 4) ^
//    B_u[(a[0] >> 4) & 0xF];
//
//  // Modified << 4 (store MS 4 bits of current value to r[2])
//  r[2] = (*r_dw_pointer >> 124) & 0xF;
//  *r_dw_pointer = *r_dw_pointer << 4;
//  *r_dw_pointer = *r_dw_pointer ^ B_u[(a[0]) & 0xF];
//  */
//
//  // Word-wise polynomial multiplication using the CLMUL instruction
//  word r[3];
//  //memset(r, 0x0, 3 * 8);
//  /*
//  doubleword temp_res = a[0];
//  asm("pclmulqdq %2, %1, %0;"
//    : "+x"(temp_res)
//    : "x"(b[0]), "i"(0)
//    );
//  */
//
//  doubleword temp_res = 0;
//  asm("pclmulqdq %2, %1, %0;"
//    : "=x"(temp_res)
//    : "x"(a[0]), "i"(0), "0"(b[0])
//    );
//
//
//  r[0] = ((temp_res << 64) >> 64);
//  r[1] = (temp_res >> 64);
//  //temp_res = a[0] * b[1];
//  //r[1] = r[1] ^ ((temp_res << 64) >> 64); // or: r[1] = r[1] ^ (a[0] * b[1]), since b[1] is either 0 or 1
//  r[1] = r[1] ^ (a[0] * b[1]);
//  //temp_res = a[1] * b[0];
//  //r[1] = r[1] ^ ((temp_res << 64) >> 64); // or: r[1] = r[1] ^ (a[1] * b[0]), since a[1] is either 0 or 1
//  r[1] = r[1] ^ (a[1] * b[0]);
//  //temp_res = a[1] * b[1];
//  //r[2] = ((temp_res << 64) >> 64); // or: r[2] = r[2] ^ (a[1] * b[1]), since both a[1] and b[1] are either 0 or 1
//  r[2] = a[1] * b[1];
//  //std::cout << "a = 0x" << BigIntLib::ToString(a, 9) << std::endl;
//  //std::cout << "b = 0x" << BigIntLib::ToString(b, 9) << std::endl;
//  //std::cout << "r = " << BigIntLib::ToString(r, 17) << std::endl;
//
//  // Reduction
//  // x^65 = x^4 + x^3 + x + 1 mod p(x)
//  // x^128 = x^67 + x^66 + x^64 + x^63 mod p(x)
//  // Manipulate from higher words to lower words
//
//  //std::cout << "r: " << BigIntLib::ToString(&r, BigIntLib::field_size_bytes_ * 2) << std::endl;
//  
//  /*
//  doubleword c0 = ((doubleword)(r[1] & 0x1) << 64) | r[0]; // LS 65 bits
//  word c1 = ((r[2] & 0x1) << 63) | (r[1] >> 1); // MS 64 bits
//
//  doubleword T = c1;
//  c1 = c1 ^ (T >> 61) ^ (T >> 62); // 61 = 65 - 4, 62 = 65 - 3, 64 = 65 - 1 (omitted, all zeros), x^0 does not affect c1
//  T = c1;
//  c0 = c0 ^ ((T << 63) >> 59) ^ ((T << 63) >> 60) ^ ((T << 63) >> 62) ^ T; // for x^4, x^3, x^1, x^0
//  c[0] = c0 & 0xFFFFFFFFFFFFFFFF; // LS 64 bits of c0
//  c[1] = (c0 >> 64) & 0x1; // 64th bit of c0 (x^64)
//  */
//  
//  word c0 = r[0]; // first 64 bits
//  word c1 = r[1]; // second 64 bits
//  word c2 = r[2]; // third 64 bits (only 1 bit)
//
//  // Reduce LSB of c2 and add to bits 67, 66, 64 and 63 of c (because x^128 = x^67 + x^66 + x^64 + x^63 mod p(x))
//  word T = c2;
//  c1 = c1 ^ (T << 3);
//  c1 = c1 ^ (T << 2);
//  c1 = c1 ^ T;
//  c0 = c0 ^ (T << 63);
//
//  // Reduce MS 63 bits of c1 and add to bits 4, 3, 1 and 0 of c (because x^65 = x^4 + x^3 + x + 1 mod p(x))
//  // Parts relevant for c0
//  T = c1 >> 1;
//  c1 = c1 ^ (T >> 60);
//  c1 = c1 ^ (T >> 61);
//  c1 = c1 ^ (T >> 63);
//  // Parts relevant for c0
//  T = c1 >> 1;
//  c0 = c0 ^ (T << 4);
//  c0 = c0 ^ (T << 3);
//  c0 = c0 ^ (T << 1);
//  c0 = c0 ^ T;
//
//  // Build result = (LSB of c1 || c0)
//  c[1] = c1 & 0x1;
//  c[0] = c0;
//
//  /*
//  // Slower anyway, small bug inside
//  doubleword red_bits = (r[1] >> 1) | (r[2] << 63);
//  doubleword* r_val = (doubleword*)r;
//  *r_val = *r_val ^ (red_bits << 4);
//  red_bits = (r[1] >> 1) | (r[2] << 63);
//  *r_val = *r_val ^ (red_bits << 3);
//  red_bits = (r[1] >> 1) | (r[2] << 63);
//  *r_val = *r_val ^ (red_bits << 1);
//  red_bits = (r[1] >> 1) | (r[2] << 63);
//  *r_val = *r_val ^ (red_bits);
//
//  c[1] = r[1] & 0x1;
//  c[0] = r[0];
//  */
//  
//}
//
//void BigIntLib::Times2BF65(word* c, word* a) {
//  doubleword r = 0;
//  memcpy(&r, a, BigIntLib::field_size_bytes_);
//  r <<= 1;
//  // Bits 4, 3, 1, 0
//  doubleword T = r >> 65;
//  *c = ((r ^ (T << 4) ^ (T << 3) ^ (T << 1) ^ T) << 63) >> 63;
//}
//
//void BigIntLib::Times3BF65(word* c, word* a) {
//  doubleword r = 0;
//  memcpy(&r, a, BigIntLib::field_size_bytes_);
//  r <<= 2;
//  // Bits 4, 3, 1, 0
//  doubleword T = r >> 65;
//  *c = ((r ^ (T << 4) ^ (T << 3) ^ (T << 1) ^ T) << 63) >> 63;
//}
//
//void BigIntLib::AddPF3(word* c, word* a, word* b) {
//  word r = *a + *b;
//  if(r >= BigIntLib::modulo_[0]) r -= BigIntLib::modulo_[0];
//  *c = r;
//}
//
//void BigIntLib::SubPF3(word* c, word* a, word* b) {
//  word r = (*a > *b) ? (*a - *b) : (*a + BigIntLib::modulo_[0] - *b);
//  if(r >= BigIntLib::modulo_[0]) r -= BigIntLib::modulo_[0];
//  *c = r;
//}
//
//void BigIntLib::MulPF3Fast(word* c, word* a, word* b) {
//  uchar r = (*a * *b);
//
//  // Modular additions
//  // 3  3  4
//  // 5  4  5
//  // -1  5  -1
//
//  //uchar temp_var = 0x0;
//  *c = r & 0x7;
//  //temp_var = ((r & 0x8) >> 3) | ((r & 0x18) >> 2);
//  *c = *c + (((r & 0x8) >> 3) | ((r & 0x18) >> 2));
//  if(*c >= BigIntLib::modulo_[0]) *c -= BigIntLib::modulo_[0];
//  //temp_var = ((r & 0x20) >> 5) | ((r & 0x30) >> 3);
//  *c = *c + (((r & 0x20) >> 5) | ((r & 0x30) >> 3));
//  if(*c >= BigIntLib::modulo_[0]) *c -= BigIntLib::modulo_[0];
//  //temp_var = ((r & 0x20) >> 4);
//  *c = *c + (((r & 0x20) >> 4));
//  if(*c >= BigIntLib::modulo_[0]) *c -= BigIntLib::modulo_[0];
//  
//}
//
//void BigIntLib::AddPF4(word* c, word* a, word* b) {
//  word r = *a + *b;
//  if(r >= BigIntLib::modulo_[0]) r -= BigIntLib::modulo_[0];
//  *c = r;
//}
//
//void BigIntLib::SubPF4(word* c, word* a, word* b) {
//  word r = (*a > *b) ? (*a - *b) : (*a + BigIntLib::modulo_[0] - *b);
//  if(r >= BigIntLib::modulo_[0]) r -= BigIntLib::modulo_[0];
//  *c = r;
//}
//
//void BigIntLib::MulPF4Fast(word* c, word* a, word* b) {
//  uchar r = (*a * *b);
//
//  // Modular additions
//  // 2  2
//  // 3  3
//  // -1  3
//
//  uchar temp_var = 0x0;
//  *c = r & 0xF;
//  temp_var = (((uchar)(r << 2) >> 6) << 2) | ((uchar)(r << 2) >> 6);
//  *c = *c + temp_var;
//  if(*c >= BigIntLib::modulo_[0]) *c -= BigIntLib::modulo_[0];
//  temp_var = ((r >> 6) << 2) | (r >> 6);
//  *c = *c + temp_var;
//  if(*c >= BigIntLib::modulo_[0]) *c -= BigIntLib::modulo_[0];
//  temp_var = (r >> 6) << 2;
//  *c = *c + temp_var;
//  if(*c >= BigIntLib::modulo_[0]) *c -= BigIntLib::modulo_[0];
//}
//
//void BigIntLib::MulPF4FastCrandall(word* c, word* a, word* b) {
//  word r = (*a * *b);
//  word c_small = 5;
//  word q_i = r >> 4;
//  r = r & 0xF;
//  word t;
//
//  // WHILE 1.1
//  t = c_small * q_i;
//  r = r + (t & 0xF) * (q_i > 0);
//  q_i = t >> 4;
//
//  // WHILE 1.2
//  t = c_small * q_i;
//  r = r + (t & 0xF) * (q_i > 0);
//
//  // WHILE 2.1
//  r -= BigIntLib::modulo_[0] * (r >= BigIntLib::modulo_[0]);
//
//  // WHILE 2.2
//  r -= BigIntLib::modulo_[0] * (r >= BigIntLib::modulo_[0]);
//  
//  *c = r;
//}
//
//void BigIntLib::DoubleAddPF4(word* c, word* a, word* b) {
//  // Adds both 4-bit sides of the bytes separately
//  // Least significant 4 bits
//  word r1 = (*a & 0xF) + (*b & 0xF);
//  if(r1 >= BigIntLib::modulo_[0]) r1 -= BigIntLib::modulo_[0];
//  // Most significant 4 bits
//  word r2 = ((*a & 0xF0) >> 4) + ((*b & 0xF0) >> 4);
//  if(r2 >= BigIntLib::modulo_[0]) r2 -= BigIntLib::modulo_[0];
//  // Assign
//  *c = (r2 << 4) | r1;
//}
//
//void BigIntLib::AddPF16(word* c, word* a, word* b) {
//  word r = *a + *b;
//  if(r >= BigIntLib::modulo_[0]) r -= BigIntLib::modulo_[0];
//  *c = r;
//}
//
//void BigIntLib::AddSpecPF16(word* c, word* a, word* b) {
//  // No reduction
//  *c = *a + *b;
//}
//
//void BigIntLib::SubPF16(word* c, word* a, word* b) {
//  word r = (*a > *b) ? (*a - *b) : (*a + BigIntLib::modulo_[0] - *b);
//  if(r >= BigIntLib::modulo_[0]) r -= BigIntLib::modulo_[0];
//  *c = r;
//}
//
//void BigIntLib::MulPF16Fast(word* c, word* a, word* b) {
//  word r = (*a * *b);
//
//  // Modular additions
//  // 4  4  5  6
//  // 7  5  6  7
//  // -1  7  -1  -1
//
//  
//  word temp_var;
//  word t = r & 0xFFFF;
//  temp_var = ((r & 0x000F0000) >> 16) | ((r & 0x0FFF0000) >> 12);
//  t = t + temp_var;
//  if(t >= BigIntLib::modulo_[0]) t -= BigIntLib::modulo_[0];
//  temp_var = ((r & 0xF0000000) >> 28) | ((r & 0xFFF00000) >> 16);
//  t = t + temp_var;
//  if(t >= BigIntLib::modulo_[0]) t -= BigIntLib::modulo_[0];
//  temp_var = ((r & 0xF0000000) >> 24);
//  t = t + temp_var;
//  if(t >= BigIntLib::modulo_[0]) t -= BigIntLib::modulo_[0];
//  *c = t;
//}
//
//void BigIntLib::MulSpecPF16(word* c, word* a) {
//  // Constant modular multiplication with 16
//  word r = (*a << 4);
//
//  // r is something like 0x000xxxxx (MS 12 bits are 0)
//
//  word temp_var;
//  word t = r & 0xFFFF;
//  temp_var = ((r & 0x000F0000) >> 16) | ((r & 0x000F0000) >> 12);
//  t = t + temp_var;
//  if(t >= BigIntLib::modulo_[0]) t -= BigIntLib::modulo_[0];
//  // Since the MS 12 bits of r are 0, the following steps of the Solinas reduction are not necessary
//  *c = t;
//}
//
//void BigIntLib::SolinasReducPF16(word* out, word* in) {
//  word r = *in;
//  word temp_var;
//  word t = r & 0xFFFF;
//  temp_var = ((r & 0x000F0000) >> 16) | ((r & 0x0FFF0000) >> 12);
//  t = t + temp_var;
//  if(t >= BigIntLib::modulo_[0]) t -= BigIntLib::modulo_[0];
//  temp_var = ((r & 0xF0000000) >> 28) | ((r & 0xFFF00000) >> 16);
//  t = t + temp_var;
//  if(t >= BigIntLib::modulo_[0]) t -= BigIntLib::modulo_[0];
//  temp_var = ((r & 0xF0000000) >> 24);
//  t = t + temp_var;
//  if(t >= BigIntLib::modulo_[0]) t -= BigIntLib::modulo_[0];
//  *out = t;
//}
//
//void BigIntLib::MulPF16FastCrandall(word* c, word* a, word* b) {
//  word r = (*a * *b);
//  word c_small = 17;
//  word q_i = r >> 16;
//  r = r & 0xFFFF;
//  word t;
//
//  // WHILE 1.1
//  t = c_small * q_i;
//  r = r + (t & 0xFFFF) * (q_i > 0);
//  q_i = t >> 16;
//
//  // WHILE 1.2
//  t = c_small * q_i;
//  r = r + (t & 0xFFFF) * (q_i > 0);
//
//  // WHILE 2.1
//  r -= BigIntLib::modulo_[0] * (r >= BigIntLib::modulo_[0]);
//
//  // WHILE 2.2
//  r -= BigIntLib::modulo_[0] * (r >= BigIntLib::modulo_[0]);
//
//  *c = r;
//}
//
//void BigIntLib::AddPF32(word* c, word* a, word* b) {
//  word r = (*a) + (*b);
//  if(r >= BigIntLib::modulo_[0]) r -= BigIntLib::modulo_[0];
//  *c = r;
//}
//
//void BigIntLib::SubPF32(word* c, word* a, word* b) {
//  word r = (*a > *b) ? (*a - *b) : (*a + BigIntLib::modulo_[0] - *b);
//  if(r >= BigIntLib::modulo_[0]) r -= BigIntLib::modulo_[0];
//  *c = r;
//}
//
//void BigIntLib::MulPF32Fast(word* c, word* a, word* b) {
//  word r = (*a * *b);
//
//  // Modular additions
//  // 8  8  9  10  11  12  13  14
//  // 15  9  10  11  12  13  14  15
//  // -1  15  -1  -1  -1  -1  -1  -1
//
//  /*
//  word temp_var;
//  word x_tmp;
//  x_tmp = r & 0xFFFFFFFF;
//  
//  temp_var = ((r >> 32) & 0xF) | (((r >> 32) & 0x0FFFFFFF) << 4);
//  x_tmp = x_tmp + temp_var;
//  if(x_tmp >= BigIntLib::modulo_[0]) x_tmp -= BigIntLib::modulo_[0];
//  temp_var = ((r >> 60) & 0xF) | (((r >> 36) & 0x0FFFFFFF) << 4);
//  x_tmp = x_tmp + temp_var;
//  if(x_tmp >= BigIntLib::modulo_[0]) x_tmp -= BigIntLib::modulo_[0];
//  temp_var = (((r >> 60) & 0xF) << 4);
//  x_tmp = x_tmp + temp_var;
//  if(x_tmp >= BigIntLib::modulo_[0]) x_tmp -= BigIntLib::modulo_[0];
//  *c = (word)x_tmp;
//  */
//
//  // If computation is wrong, check first commented (old) lines!!!
//  word temp_var;
//  word t;
//  t = r & 0xFFFFFFFF;
//  //temp_var = ((r >> 32) & 0xF) | (((r >> 32) & 0x0FFFFFFF) << 4);
//  temp_var = ((r >> 32) & 0xF) | ((r & 0xFFFFFFF00000000) >> 28);
//  t = t + temp_var;
//  if(t >= BigIntLib::modulo_[0]) t -= BigIntLib::modulo_[0];
//  //t -= BigIntLib::modulo_[0] * (t >= BigIntLib::modulo_[0]);
//  //temp_var = ((r >> 60) & 0xF) | (((r >> 36) & 0x0FFFFFFF) << 4);
//  temp_var = (r >> 60) | ((r & 0xFFFFFFF000000000) >> 32);
//  //temp_var = (r >> 60) | ((r >> 36) << 4);
//  t = t + temp_var;
//  if(t >= BigIntLib::modulo_[0]) t -= BigIntLib::modulo_[0];
//  //t -= BigIntLib::modulo_[0] * (t >= BigIntLib::modulo_[0]);
//  //temp_var = (((r >> 60) & 0xF) << 4);
//  temp_var = ((r >> 60) << 4);
//  t = t + temp_var;
//  if(t >= BigIntLib::modulo_[0]) t -= BigIntLib::modulo_[0];
//  //t -= BigIntLib::modulo_[0] * (t >= BigIntLib::modulo_[0]);
//  *c = t;
//}
//
//void BigIntLib::MulPF32FastCrandall(word* c, word* a, word* b) {
//  word r = (*a * *b);
//  word c_small = 5; // 2^2 + 1 from p_32 = 2^32 - 2^2 - 1 = 2^32 - c
//  word q_i = r >> 32;
//  r = r & 0xFFFFFFFF;
//  word t;
//
//  // c * x = (c - 1) * x + x
//
//  // WHILE 1.1
//  //if(q_i > 0) { // This if-clause could be omitted with very high probability
//    t = c_small * q_i;
//    //t = (q_i << 2) + q_i;
//    //r = r + (t & 0xFFFFFFFF);
//    r = r + (t & 0xFFFFFFFF) * (q_i > 0);
//    q_i = t >> 32;
//  //}
//
//  // WHILE 1.2
//  //if(q_i > 0) { // This if-clause cannot be omitted
//    t = c_small * q_i;
//    //t = (q_i << 2) + q_i;
//    //r = r + (t & 0xFFFFFFFF);
//    r = r + (t & 0xFFFFFFFF) * (q_i > 0);
//    //q_i = t >> 32;
//  //}
//
//  // WHILE 2.1
//  /*
//  if(r >= BigIntLib::modulo_[0]) // This if-clause cannot be omitted
//    r -= BigIntLib::modulo_[0];
//  // WHILE 2.2
//  if(r >= BigIntLib::modulo_[0]) // This if-clause cannot be omitted
//    r -= BigIntLib::modulo_[0];
//  */
//  // WHILE 2.1
//  r -= BigIntLib::modulo_[0] * (r >= BigIntLib::modulo_[0]);
//
//  // WHILE 2.2
//  r -= BigIntLib::modulo_[0] * (r >= BigIntLib::modulo_[0]);
//  
//  *c = r;
//}
//
//void BigIntLib::AddPF61(word* c, word* a, word* b) {
//  *c = *a + *b;
//  if (*c >= 0x1FFFFFFFFFFFFFFF) {
//    *c -= 0x1FFFFFFFFFFFFFFF;
//  }
//}
//
//void BigIntLib::SubPF61(word* c, word* a, word* b) {
//  *c = *a - *b;
//  if (*c >= 0x8000000000000000) {
//    *c += 0x1FFFFFFFFFFFFFFF;
//  }
//}
//
//void BigIntLib::MulPF61Fast(word* c, word* a, word* b) {
//  doubleword r = (doubleword)(*a) * (doubleword)(*b);
//  word t = (word)(r & 0x1FFFFFFFFFFFFFFF) + (word)(r >> 61);
//  t = (t & 0x1FFFFFFFFFFFFFFF) + (t >> 61);
//  *c = (t < 0x1FFFFFFFFFFFFFFF) ? t : (t - 0x1FFFFFFFFFFFFFFF);
//}
//
//void BigIntLib::AddPF64(word* c, word* a, word* b) {
//  doubleword r = (doubleword)(*a) + *b;
//  if(r >= BigIntLib::modulo_[0]) r -= BigIntLib::modulo_[0];
//  *c = (word)r;
//}
//
//void BigIntLib::SubPF64(word* c, word* a, word* b) {
//  doubleword r = (*a > *b) ? (*a - *b) : (*a + BigIntLib::modulo_[0] - *b);
//  if(r >= BigIntLib::modulo_[0]) r -= BigIntLib::modulo_[0];
//  *c = (word)r;
//}
//
//void BigIntLib::MulPF64(word* c, word* a, word* b) {
//  *c = ((doubleword)(*a) * (doubleword)(*b)) % BigIntLib::modulo_[0];
//}
//
//void BigIntLib::MulPF64Fast(word* c, word* a, word* b) {
//  // TODO Everything is hard-coded here, change it
//  // Method due to Jerome A. Solinas, "Generalized Mersenne Numbers", 1999
//  // Inputs are matrices X and A, which are called the reduction matrix and the modular addition matrix, respectively.
//  // These are calculated in the beginning. The additions below can easily be derived from the values in the modular addition matrix,
//  // where values contained in this matrix define the i-th t-base word in the integer to be reduced.
//  // The following hard-coded method works for an arbitrary integer a and prime p = 2^64 - 2^8 - 1, where a < p^2.
//  // Both a and p are represented as a collection of t-base words, where t is 2^8:
//  // (a * b) = (ab_15, ab_14, ... , ab_1, ab_0) [16 bytes], p = (p_7, p_6, ... , p_1, p_0) [8 bytes]
//  doubleword r = ((doubleword)(*a) * (doubleword)(*b));
//
//  // Values (indices in value r, see modular addition matrix):
//  // s0: 00 01 02 03 04 05 06 07
//  // s1: 08 08 09 10 11 12 13 14
//  // s2: 15 09 10 11 12 13 14 15
//  // s3: 00 15 00 00 00 00 00 00
//
//  doubleword x_tmp;
//  x_tmp = r & 0xFFFFFFFFFFFFFFFF;
//  x_tmp = x_tmp + (((r >> 64) & 0xFF) | (((r >> 64) & 0x00FFFFFFFFFFFFFF) << 8)) +
//    (((r >> 120) & 0xFF) | (((r >> 72) & 0x00FFFFFFFFFFFFFF) << 8)) +
//    (((r >> 120) & 0xFF) << 8);
//  if(x_tmp >= BigIntLib::modulo_[0]) x_tmp -= BigIntLib::modulo_[0];
//  if(x_tmp >= BigIntLib::modulo_[0]) x_tmp -= BigIntLib::modulo_[0];
//  if(x_tmp >= BigIntLib::modulo_[0]) x_tmp -= BigIntLib::modulo_[0];
//  *c = (word)x_tmp;
//}
//
//void BigIntLib::MulPF64FastCrandall(word* c, word* a, word* b) {
//  doubleword r = ((doubleword)(*a) * (doubleword)(*b));
//  word c_small = 59;
//  doubleword q_i = r >> 64;
//  r = r & 0xFFFFFFFFFFFFFFFF;
//  doubleword t;
//
//  // WHILE 1.1
//  t = c_small * q_i;
//  r = r + (t & 0xFFFFFFFFFFFFFFFF) * (q_i > 0);
//  q_i = t >> 64;
//
//  // WHILE 1.2
//  t = c_small * q_i;
//  r = r + (t & 0xFFFFFFFFFFFFFFFF) * (q_i > 0);
//
//  // WHILE 2.1
//  if(r >= BigIntLib::modulo_[0]) r -= BigIntLib::modulo_[0];
//
//  // WHILE 2.2
//  if(r >= BigIntLib::modulo_[0]) r -= BigIntLib::modulo_[0];
//
//  *c = r;
//}
//
//void BigIntLib::AddPF136(word* c, word* a, word* b) {
//  // Implement static ADD in PF with 136 bits...
//  doubleword tmp;
//  word carry = 0;
//  // for(uint32 i = 0; i < BigIntLib::field_num_words_; i++) {
//  //   tmp = (doubleword)a[i] + b[i] + carry;
//  //   c[i] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//  //   carry = (tmp >> WORD_SIZE);
//  // }
//  tmp = (doubleword)a[0] + b[0] + carry;
//  c[0] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//  carry = (tmp >> WORD_SIZE);
//  tmp = (doubleword)a[1] + b[1] + carry;
//  c[1] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//  carry = (tmp >> WORD_SIZE);
//  tmp = (doubleword)a[2] + b[2] + carry;
//  c[2] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//  carry = (tmp >> WORD_SIZE);
//
//  //if(carry == 1 || BigIntLib::Greater(c, BigIntLib::modulo_)) {
//  if(carry == 1 || !BigIntLib::Smaller(c, BigIntLib::modulo_)) {
//    word borrow = 0;
//    tmp = (doubleword)c[0] - BigIntLib::modulo_[0] - borrow;
//    c[0] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//    borrow = (tmp >> WORD_SIZE) != 0;
//    tmp = (doubleword)c[1] - BigIntLib::modulo_[1] - borrow;
//    c[1] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//    borrow = (tmp >> WORD_SIZE) != 0;
//    tmp = (doubleword)c[2] - BigIntLib::modulo_[2] - borrow;
//    c[2] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//    borrow = (tmp >> WORD_SIZE) != 0;
//  }
//}
//
//void BigIntLib::SubPF136(word* c, word* a, word* b) {
//  // Implement static SUB in PF with 136 bits...
//  doubleword tmp;
//  word borrow = 0;
//  // for(uint32 i = 0; i < BigIntLib::field_num_words_; i++) {
//  //   tmp = (doubleword)a[i] - b[i] - borrow;
//  //   c[i] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//  //   borrow = (tmp >> WORD_SIZE) != 0;
//  // }
//  tmp = (doubleword)a[0] - b[0] - borrow;
//  c[0] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//  borrow = (tmp >> WORD_SIZE) != 0;
//  tmp = (doubleword)a[1] - b[1] - borrow;
//  c[1] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//  borrow = (tmp >> WORD_SIZE) != 0;
//  tmp = (doubleword)a[2] - b[2] - borrow;
//  c[2] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//  borrow = (tmp >> WORD_SIZE) != 0;
//  
//  if(borrow == 1) {
//    word carry = 0;
//    // for(uint32 i = 0; i < BigIntLib::field_num_words_; i++) {
//    //   tmp = (doubleword)c[i] + BigIntLib::modulo_[i] + carry;
//    //   c[i] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//    //   carry = (tmp >> WORD_SIZE);
//    // }
//    tmp = (doubleword)c[0] + BigIntLib::modulo_[0] + carry;
//    c[0] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//    carry = (tmp >> WORD_SIZE);
//    tmp = (doubleword)c[1] + BigIntLib::modulo_[1] + carry;
//    c[1] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//    carry = (tmp >> WORD_SIZE);
//    tmp = (doubleword)c[2] + BigIntLib::modulo_[2] + carry;
//    c[2] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//    carry = (tmp >> WORD_SIZE);
//  }
//}
//
//void BigIntLib::MulPF136Fast(word* c, word* a, word* b) {
//  // Implement static MUL in PF with 136 bits followed by static reduction using a generalized mersenne prime...
//  //memset(c, 0, BigIntLib::field_num_words_ * (WORD_SIZE / 8)); // Probably not needed
//  word U;
//  word V;
//  doubleword UV;
//  word c_temp[BigIntLib::field_num_words_ * 2];
//  // for(uint32 i = 0; i < BigIntLib::field_num_words_; i++) c_temp[i] = 0;
//  // for(uint32 i = 0; i < BigIntLib::field_num_words_; i++) {
//  //   U = 0;
//  //   for(uint32 j = 0; j < BigIntLib::field_num_words_; j++) {
//  //     UV = c_temp[i + j] + ((doubleword)(a[i]) * (doubleword)(b[j])) + U;
//  //     U = UV >> WORD_SIZE;
//  //     V = (UV << WORD_SIZE) >> WORD_SIZE;
//  //     c_temp[i + j] = V;
//  //   }
//  //   c_temp[i + BigIntLib::field_num_words_] = U;
//  // }
//
//  // STATIC
//  memset(c_temp, 0, (BigIntLib::field_num_words_ * 2) * (WORD_SIZE / 8));
//  // LOOP 1
//  U = 0;
//  UV = c_temp[0] + ((doubleword)(a[0]) * (doubleword)(b[0])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[0] = V;
//  UV = c_temp[1] + ((doubleword)(a[0]) * (doubleword)(b[1])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[1] = V;
//  UV = c_temp[2] + ((doubleword)(a[0]) * (doubleword)(b[2])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[2] = V;
//  c_temp[BigIntLib::field_num_words_] = U;
//  // LOOP 2
//  U = 0;
//  UV = c_temp[1] + ((doubleword)(a[1]) * (doubleword)(b[0])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[1] = V;
//  UV = c_temp[2] + ((doubleword)(a[1]) * (doubleword)(b[1])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[2] = V;
//  UV = c_temp[3] + ((doubleword)(a[1]) * (doubleword)(b[2])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[3] = V;
//  c_temp[BigIntLib::field_num_words_ + 1] = U;
//  // LOOP 3
//  U = 0;
//  UV = c_temp[2] + ((doubleword)(a[2]) * (doubleword)(b[0])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[2] = V;
//  UV = c_temp[3] + ((doubleword)(a[2]) * (doubleword)(b[1])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[3] = V;
//  UV = c_temp[4] + ((doubleword)(a[2]) * (doubleword)(b[2])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[4] = V;
//  c_temp[BigIntLib::field_num_words_ + 2] = U;
//
//  // Modular additions
//  // 17  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31  32
//  // 33  18  19  20  21  22  23  24  25  26  27  28  29  30  31  32  33
//  // -1  33  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1
//
//  uchar temp_var[24];
//  uchar* pointer = (uchar*)c_temp;
//  memset(temp_var, 0, 24);
//  memset(c, 0, 24);
//  memcpy(c, pointer, 17);
//  memcpy(temp_var + 1, pointer + 17, 16);
//  temp_var[0] = pointer[17];
//  BigIntLib::Add(c, c, (word*)temp_var);
//  memcpy(temp_var + 1, pointer + 18, 16);
//  temp_var[0] = pointer[33];
//  BigIntLib::Add(c, c, (word*)temp_var);
//  memset(temp_var, 0, 24);
//  temp_var[1] = pointer[33];
//  BigIntLib::Add(c, c, (word*)temp_var);
//}
//
//void BigIntLib::MulPF256Fast(word* c, word* a, word* b) {
//  // Implement static MUL in PF with 256 bits followed by static reduction using a generalized mersenne prime...
//  //memset(c, 0, BigIntLib::field_num_words_ * (WORD_SIZE / 8)); // Probably not needed
//  word U;
//  word V;
//  doubleword UV;
//  word c_temp[BigIntLib::field_num_words_ * 2];
//  // for(uint32 i = 0; i < BigIntLib::field_num_words_; i++) c_temp[i] = 0;
//  // for(uint32 i = 0; i < BigIntLib::field_num_words_; i++) {
//  //   U = 0;
//  //   for(uint32 j = 0; j < BigIntLib::field_num_words_; j++) {
//  //     UV = c_temp[i + j] + ((doubleword)(a[i]) * (doubleword)(b[j])) + U;
//  //     U = UV >> WORD_SIZE;
//  //     V = (UV << WORD_SIZE) >> WORD_SIZE;
//  //     c_temp[i + j] = V;
//  //   }
//  //   c_temp[i + BigIntLib::field_num_words_] = U;
//  // }
//
//  // STATIC
//  memset(c_temp, 0, (BigIntLib::field_num_words_ * 2) * (WORD_SIZE / 8));
//  // LOOP 1
//  U = 0;
//  UV = c_temp[0] + ((doubleword)(a[0]) * (doubleword)(b[0])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[0] = V;
//  UV = c_temp[1] + ((doubleword)(a[0]) * (doubleword)(b[1])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[1] = V;
//  UV = c_temp[2] + ((doubleword)(a[0]) * (doubleword)(b[2])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[2] = V;
//  UV = c_temp[3] + ((doubleword)(a[0]) * (doubleword)(b[3])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[3] = V;
//  c_temp[BigIntLib::field_num_words_] = U;
//  // LOOP 2
//  U = 0;
//  UV = c_temp[1] + ((doubleword)(a[1]) * (doubleword)(b[0])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[1] = V;
//  UV = c_temp[2] + ((doubleword)(a[1]) * (doubleword)(b[1])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[2] = V;
//  UV = c_temp[3] + ((doubleword)(a[1]) * (doubleword)(b[2])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[3] = V;
//  UV = c_temp[4] + ((doubleword)(a[1]) * (doubleword)(b[3])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[4] = V;
//  c_temp[BigIntLib::field_num_words_ + 1] = U;
//  // LOOP 3
//  U = 0;
//  UV = c_temp[2] + ((doubleword)(a[2]) * (doubleword)(b[0])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[2] = V;
//  UV = c_temp[3] + ((doubleword)(a[2]) * (doubleword)(b[1])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[3] = V;
//  UV = c_temp[4] + ((doubleword)(a[2]) * (doubleword)(b[2])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[4] = V;
//  UV = c_temp[5] + ((doubleword)(a[2]) * (doubleword)(b[3])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[5] = V;
//  c_temp[BigIntLib::field_num_words_ + 2] = U;
//  // LOOP 4
//  U = 0;
//  UV = c_temp[3] + ((doubleword)(a[3]) * (doubleword)(b[0])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[3] = V;
//  UV = c_temp[4] + ((doubleword)(a[3]) * (doubleword)(b[1])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[4] = V;
//  UV = c_temp[5] + ((doubleword)(a[3]) * (doubleword)(b[2])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[5] = V;
//  UV = c_temp[6] + ((doubleword)(a[3]) * (doubleword)(b[3])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[6] = V;
//  c_temp[BigIntLib::field_num_words_ + 3] = U;
//
//  // Modular additions
//  // 60  61  62  63  60  61  62  63  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  32  33  34  35  36  37  38  39  40
//  // -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  41  42  43  44  45  46  47  48  49
//  // -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  50  51  52  53  54  55  56  57  58
//  // -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  59  60  61  62  63  -1  -1  -1  -1
//
//  // Modular subtractions
//  // 32  33  34  35  32  33  34  35  36  37  38  39  40  41  42  43  44  45  46  47  48  49  50  51  52  53  54  55  56  57  58  59
//  // 41  42  43  44  36  37  38  39  40  41  42  43  44  45  46  47  48  49  50  51  52  53  54  55  56  57  58  59  60  61  62  63
//  // 50  51  52  53  41  42  43  44  45  46  47  48  49  50  51  52  53  54  55  56  57  58  59  60  61  62  63  -1  -1  -1  -1  -1
//  // 59  60  61  62  45  46  47  48  49  50  51  52  53  54  55  56  57  58  59  60  61  62  63  60  61  62  63  -1  -1  -1  -1  -1
//  // -1  -1  -1  -1  50  51  52  53  54  55  56  57  58  59  60  61  62  63  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1
//  // -1  -1  -1  -1  54  55  56  57  58  59  60  61  62  63  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1
//  // -1  -1  -1  -1  59  60  61  62  63  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1
//  // -1  -1  -1  -1  63  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1
//
//  // Following additions
//  // This is slower than the optimized memcpy-versions found in the other field sizes,
//  // but due to the comparatively high number of modular additions and subtractions it shouldn't make a huge difference.
//  uchar temp_var[32];
//  memcpy(c, c_temp, 32);
//  uchar* pointer = (uchar*)c_temp;
//  for(uint32 i = 0; i < BigIntLib::mod_addition_weight_; i++) {
//    for(uint32 j = 0; j < 32; j++) { // 32 = 256 / 8, which is the word size for the reduction using the specified Generalized Mersenne Prime
//      temp_var[j] = ((BigIntLib::mod_addition_matrix_[i][j]) == -1) ? 0 : pointer[(BigIntLib::mod_addition_matrix_[i][j])];
//    }
//    BigIntLib::Add(c, c, (word*)temp_var);
//  }
//
//  // Following subtractions
//  for(uint32 i = 0; i < BigIntLib::mod_subtraction_weight_; i++) {
//    for(uint32 j = 0; j < 32; j++) { // 32 = 256 / 8, which is the word size for the reduction using the specified Generalized Mersenne Prime
//      temp_var[j] = ((BigIntLib::mod_subtraction_matrix_[i][j]) == -1) ? 0 : pointer[(BigIntLib::mod_subtraction_matrix_[i][j])];
//    }
//    BigIntLib::Sub(c, c, (word*)temp_var);
//  }
//  
//  //memcpy(c, c_temp, 32);
//}
//
//void BigIntLib::AddPF272(word* c, word* a, word* b) {
//  // Implement static ADD in PF with 272 bits...
//  a[4] &= 0xFFFF; // Keep only first 16 bits of last word (16 + 4 * 64 = 272)
//  b[4] &= 0xFFFF; // Keep only first 16 bits of last word (16 + 4 * 64 = 272)
//  doubleword tmp;
//  word carry = 0;
//  // for(uint32 i = 0; i < BigIntLib::field_num_words_; i++) {
//  //   tmp = (doubleword)a[i] + b[i] + carry;
//  //   c[i] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//  //   carry = (tmp >> WORD_SIZE);
//  // }
//  //BigIntLib::Print(a, 40);
//  //BigIntLib::Print(b, 40);
//  //BigIntLib::Print(c, 40);
//  tmp = (doubleword)a[0] + b[0] + carry;
//  c[0] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//  carry = (tmp >> WORD_SIZE);
//  tmp = (doubleword)a[1] + b[1] + carry;
//  c[1] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//  carry = (tmp >> WORD_SIZE);
//  tmp = (doubleword)a[2] + b[2] + carry;
//  c[2] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//  carry = (tmp >> WORD_SIZE);
//  tmp = (doubleword)a[3] + b[3] + carry;
//  c[3] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//  carry = (tmp >> WORD_SIZE);
//  tmp = (doubleword)a[4] + b[4] + carry;
//  c[4] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//  carry = (tmp >> WORD_SIZE);
//
//  //if(carry == 1 || BigIntLib::Greater(c, BigIntLib::modulo_)) { // Carry should never be 1 with an only partially used last word...
//  if(carry == 1 || !BigIntLib::Smaller(c, BigIntLib::modulo_)) { // Carry should never be 1 with an only partially used last word...
//    word borrow = 0;
//    // for(uint32 i = 0; i < BigIntLib::field_num_words_; i++) {
//    //   tmp = (doubleword)c[i] - BigIntLib::modulo_[i] - borrow;
//    //   c[i] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//    //   borrow = (tmp >> WORD_SIZE) != 0;
//    // }
//    tmp = (doubleword)c[0] - BigIntLib::modulo_[0] - borrow;
//    c[0] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//    borrow = (tmp >> WORD_SIZE) != 0;
//    tmp = (doubleword)c[1] - BigIntLib::modulo_[1] - borrow;
//    c[1] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//    borrow = (tmp >> WORD_SIZE) != 0;
//    tmp = (doubleword)c[2] - BigIntLib::modulo_[2] - borrow;
//    c[2] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//    borrow = (tmp >> WORD_SIZE) != 0;
//    tmp = (doubleword)c[3] - BigIntLib::modulo_[3] - borrow;
//    c[3] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//    borrow = (tmp >> WORD_SIZE) != 0;
//    tmp = (doubleword)c[4] - BigIntLib::modulo_[4] - borrow;
//    c[4] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//    //borrow = (tmp >> WORD_SIZE) != 0;
//  }
//}
//
//void BigIntLib::SubPF272(word* c, word* a, word* b) {
//  // Implement static SUB in PF with 272 bits...
//  a[4] &= 0xFFFF; // Keep only first 16 bits of last word (16 + 4 * 64 = 272)
//  b[4] &= 0xFFFF; // Keep only first 16 bits of last word (16 + 4 * 64 = 272)
//  doubleword tmp;
//  word borrow = 0;
//  // for(uint32 i = 0; i < BigIntLib::field_num_words_; i++) {
//  //   tmp = (doubleword)a[i] - b[i] - borrow;
//  //   c[i] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//  //   borrow = (tmp >> WORD_SIZE) != 0;
//  // }
//  tmp = (doubleword)a[0] - b[0] - borrow;
//  c[0] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//  borrow = (tmp >> WORD_SIZE) != 0;
//  tmp = (doubleword)a[1] - b[1] - borrow;
//  c[1] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//  borrow = (tmp >> WORD_SIZE) != 0;
//  tmp = (doubleword)a[2] - b[2] - borrow;
//  c[2] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//  borrow = (tmp >> WORD_SIZE) != 0;
//  tmp = (doubleword)a[3] - b[3] - borrow;
//  c[3] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//  borrow = (tmp >> WORD_SIZE) != 0;
//  tmp = (doubleword)a[4] - b[4] - borrow;
//  c[4] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//  borrow = (tmp >> WORD_SIZE) != 0;
//  
//  if(borrow == 1) {
//    word carry = 0;
//    // for(uint32 i = 0; i < BigIntLib::field_num_words_; i++) {
//    //   tmp = (doubleword)c[i] + BigIntLib::modulo_[i] + carry;
//    //   c[i] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//    //   carry = (tmp >> WORD_SIZE);
//    // }
//    tmp = (doubleword)c[0] + BigIntLib::modulo_[0] + carry;
//    c[0] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//    carry = (tmp >> WORD_SIZE);
//    tmp = (doubleword)c[1] + BigIntLib::modulo_[1] + carry;
//    c[1] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//    carry = (tmp >> WORD_SIZE);
//    tmp = (doubleword)c[2] + BigIntLib::modulo_[2] + carry;
//    c[2] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//    carry = (tmp >> WORD_SIZE);
//    tmp = (doubleword)c[3] + BigIntLib::modulo_[3] + carry;
//    c[3] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//    carry = (tmp >> WORD_SIZE);
//    tmp = (doubleword)c[4] + BigIntLib::modulo_[4] + carry;
//    c[4] = ((tmp << WORD_SIZE) >> WORD_SIZE);
//    carry = (tmp >> WORD_SIZE);
//  }
//}
//
//void BigIntLib::MulPF272Fast(word* c, word* a, word* b) {
//  // Implement static MUL in PF with 272 bits followed by static reduction using a generalized mersenne prime...
//  //memset(c, 0, BigIntLib::field_num_words_ * (WORD_SIZE / 8)); // Probably not needed
//  a[4] &= 0xFFFF; // Keep only first 16 bits of last word (16 + 4 * 64 = 272)
//  b[4] &= 0xFFFF; // Keep only first 16 bits of last word (16 + 4 * 64 = 272)
//  word U;
//  word V;
//  doubleword UV;
//  word c_temp[BigIntLib::field_num_words_ * 2];
//  // for(uint32 i = 0; i < BigIntLib::field_num_words_; i++) c_temp[i] = 0;
//  // for(uint32 i = 0; i < BigIntLib::field_num_words_; i++) {
//  //   U = 0;
//  //   for(uint32 j = 0; j < BigIntLib::field_num_words_; j++) {
//  //     UV = c_temp[i + j] + ((doubleword)(a[i]) * (doubleword)(b[j])) + U;
//  //     U = UV >> WORD_SIZE;
//  //     V = (UV << WORD_SIZE) >> WORD_SIZE;
//  //     c_temp[i + j] = V;
//  //   }
//  //   c_temp[i + BigIntLib::field_num_words_] = U;
//  // }
//
//  // STATIC
//  memset(c_temp, 0, (BigIntLib::field_num_words_ * 2) * (WORD_SIZE / 8));
//  // LOOP 1
//  U = 0;
//  UV = c_temp[0] + ((doubleword)(a[0]) * (doubleword)(b[0])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[0] = V;
//  UV = c_temp[1] + ((doubleword)(a[0]) * (doubleword)(b[1])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[1] = V;
//  UV = c_temp[2] + ((doubleword)(a[0]) * (doubleword)(b[2])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[2] = V;
//  UV = c_temp[3] + ((doubleword)(a[0]) * (doubleword)(b[3])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[3] = V;
//  UV = c_temp[4] + ((doubleword)(a[0]) * (doubleword)(b[4])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[4] = V;
//  c_temp[BigIntLib::field_num_words_] = U;
//  // LOOP 2
//  U = 0;
//  UV = c_temp[1] + ((doubleword)(a[1]) * (doubleword)(b[0])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[1] = V;
//  UV = c_temp[2] + ((doubleword)(a[1]) * (doubleword)(b[1])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[2] = V;
//  UV = c_temp[3] + ((doubleword)(a[1]) * (doubleword)(b[2])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[3] = V;
//  UV = c_temp[4] + ((doubleword)(a[1]) * (doubleword)(b[3])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[4] = V;
//  UV = c_temp[5] + ((doubleword)(a[1]) * (doubleword)(b[4])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[5] = V;
//  c_temp[BigIntLib::field_num_words_ + 1] = U;
//  // LOOP 3
//  U = 0;
//  UV = c_temp[2] + ((doubleword)(a[2]) * (doubleword)(b[0])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[2] = V;
//  UV = c_temp[3] + ((doubleword)(a[2]) * (doubleword)(b[1])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[3] = V;
//  UV = c_temp[4] + ((doubleword)(a[2]) * (doubleword)(b[2])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[4] = V;
//  UV = c_temp[5] + ((doubleword)(a[2]) * (doubleword)(b[3])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[5] = V;
//  UV = c_temp[6] + ((doubleword)(a[2]) * (doubleword)(b[4])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[6] = V;
//  c_temp[BigIntLib::field_num_words_ + 2] = U;
//  // LOOP 4
//  U = 0;
//  UV = c_temp[3] + ((doubleword)(a[3]) * (doubleword)(b[0])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[3] = V;
//  UV = c_temp[4] + ((doubleword)(a[3]) * (doubleword)(b[1])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[4] = V;
//  UV = c_temp[5] + ((doubleword)(a[3]) * (doubleword)(b[2])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[5] = V;
//  UV = c_temp[6] + ((doubleword)(a[3]) * (doubleword)(b[3])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[6] = V;
//  UV = c_temp[7] + ((doubleword)(a[3]) * (doubleword)(b[4])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[7] = V;
//  c_temp[BigIntLib::field_num_words_ + 3] = U;
//  // LOOP 5
//  U = 0;
//  UV = c_temp[4] + ((doubleword)(a[4]) * (doubleword)(b[0])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[4] = V;
//  UV = c_temp[5] + ((doubleword)(a[4]) * (doubleword)(b[1])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[5] = V;
//  UV = c_temp[6] + ((doubleword)(a[4]) * (doubleword)(b[2])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[6] = V;
//  UV = c_temp[7] + ((doubleword)(a[4]) * (doubleword)(b[3])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[7] = V;
//  UV = c_temp[8] + ((doubleword)(a[4]) * (doubleword)(b[4])) + U;
//  U = UV >> WORD_SIZE;
//  V = (UV << WORD_SIZE) >> WORD_SIZE;
//  c_temp[8] = V;
//  c_temp[BigIntLib::field_num_words_ + 4] = U;
//
//  // Reduction
//  uchar* pointer = (uchar*)c_temp;
//  word temp_var[5] = {0}; // 34 = 272 / 8, which is the word size for the reduction using the specified Generalized Mersenne Prime
//  uchar* temp_var_pointer = (uchar*)temp_var;
//  for(uint32 i = 0; i < BigIntLib::field_num_words_; i++) { // better with memset
//    c[i] = 0;
//    temp_var[i] = 0;
//  }
//
//  // Modular Additions
//  // 34  35  36  37  38  34  35  36  37  38  39  40  41  42  43  44  45  46  47  48  49  50  51  52  53  54  55  56  57  58  59  60  61  62
//  // 63  64  65  66  67  39  40  41  42  43  44  45  46  47  48  49  50  51  52  53  54  55  56  57  58  59  60  61  62  63  64  65  66  67
//  // -1  -1  -1  -1  -1  63  64  65  66  67  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1  -1
//  
//  // c <- s0
//  memcpy(c, pointer, 34);
//
//  // First addition
//  memcpy(temp_var_pointer, pointer + 34, 5);
//  memcpy(temp_var_pointer + 5, pointer + 34, 29);
//  BigIntLib::Add(c, c, temp_var);
//
//  // Second addition
//  memcpy(temp_var_pointer, pointer + 63, 5);
//  memcpy(temp_var_pointer + 5, pointer + 39, 29);
//  BigIntLib::Add(c, c, temp_var);
//
//  // Third addition
//  memset(temp_var_pointer, 0, 34);
//  memcpy(temp_var_pointer + 5, pointer + 63, 5);
//  BigIntLib::Add(c, c, temp_var);
//
//  // Following additions
//  // uchar temp_v[34];
//  // memcpy(c, c_temp, BigIntLib::field_size_bytes_);
//  // uchar* ptr = (uchar*)c_temp;
//  // for(uint32 i = 0; i < BigIntLib::mod_addition_weight_; i++) {
//  //   for(uint32 j = 0; j < 34; j++) { // 34 = 272 / 8, which is the word size for the reduction using the specified Generalized Mersenne Prime
//  //     temp_v[j] = ((BigIntLib::mod_addition_matrix_[i][j]) == -1) ? 0 : ptr[(BigIntLib::mod_addition_matrix_[i][j])];
//  //   }
//  //   BigIntLib::Add(c, c, (word*)temp_v);
//  // }
//
//  //memcpy(c, c_temp, BigIntLib::field_size_bytes_);
//}
//
//void BigIntLib::GetRandomFieldElementsNonZero(void* destination, uchar* seed, uint32 num_elements) {
//  (this->*GET_RANDOM_FIELD_ELEMENTS_NON_ZERO_FUNCTION)(destination, seed, num_elements);
//}
//
//void BigIntLib::GetRandomFieldElementsNonZeroPF(void* destination, uchar* seed, uint32 num_elements) {
//  uchar* dest_pointer = (uchar*)destination;
//
//  // Init AES
//  int length;
//  static const unsigned char iv[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5'};
//  EVP_EncryptInit_ex(BigIntLib::evp_cipher_ctx_, EVP_aes_128_ctr(), NULL, seed, iv);
//  static const unsigned char plaintext[16] = {'0'};
//
//  EVP_EncryptUpdate(BigIntLib::evp_cipher_ctx_, BigIntLib::evp_cipher_buffer_, &length, plaintext, sizeof(plaintext));
//  uint32 buffer_bytes_left = 16;
//  uint32 buffer_bytes_used = 0;
//  bool current_element_in_field = false;
//  uint32 gate_size = BigIntLib::field_num_words_ * 8;
//  uint32 msb_word_index = BigIntLib::field_num_words_ - 1;
//  uint32 bytes_still_to_write;
//  uint32 bytes_write_now;
//
//  uchar* current_write_pointer_base;
//  uchar* current_write_pointer;
//  word* current_element_number;
//
//  // For each element
//  for(uint32 i = 0; i < num_elements; i++) {
//    current_write_pointer_base = dest_pointer + i * gate_size;
//    current_element_number = (word*)current_write_pointer_base;
//    // This writes one complete element
//    do {
//      bytes_still_to_write = BigIntLib::field_size_bytes_;
//      current_write_pointer = current_write_pointer_base;
//      while(bytes_still_to_write > 0) {
//        bytes_write_now = std::min(bytes_still_to_write, buffer_bytes_left);
//        memcpy(current_write_pointer, BigIntLib::evp_cipher_buffer_ + buffer_bytes_used, bytes_write_now);
//        buffer_bytes_left -= bytes_write_now;
//        bytes_still_to_write -= bytes_write_now;
//        current_write_pointer += bytes_write_now;
//        buffer_bytes_used += bytes_write_now;
//
//        if(buffer_bytes_left == 0) {
//          // Buffer is empty, get new AES bytes (maybe write next value already here?)
//          EVP_EncryptUpdate(BigIntLib::evp_cipher_ctx_, BigIntLib::evp_cipher_buffer_, &length, plaintext, sizeof(plaintext));
//          buffer_bytes_left = 16;
//          buffer_bytes_used = 0;
//        }
//
//      }
//
//      // Cancel out last word
//      current_element_number[msb_word_index] &= BigIntLib::msb_word_mask_;
//      current_element_in_field = BigIntLib::Smaller(current_element_number, BigIntLib::modulo_);
//      
//    } while(!current_element_in_field || BigIntLib::IsZero(current_element_number));
//  }
//}
//
//void BigIntLib::GetRandomFieldElementsNonZeroBF(void* destination, uchar* seed, uint32 num_elements) {
//  uchar* dest_pointer = (uchar*)destination;
//
//  // Init AES
//  int length;
//  static const unsigned char iv[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5'};
//  EVP_EncryptInit_ex(BigIntLib::evp_cipher_ctx_, EVP_aes_128_ctr(), NULL, seed, iv);
//  static const unsigned char plaintext[16] = {'0'};
//
//  EVP_EncryptUpdate(BigIntLib::evp_cipher_ctx_, BigIntLib::evp_cipher_buffer_, &length, plaintext, sizeof(plaintext));
//  uint32 buffer_bytes_left = 16;
//  uint32 buffer_bytes_used = 0;
//  uint32 gate_size = BigIntLib::field_num_words_ * 8;
//  uint32 msb_word_index = BigIntLib::field_num_words_ - 1;
//  //uint32 bytes_for_element = BigIntLib::field_size_bytes_; // rounded up to the next larger byte value for BigIntLib::field_size_bits_ mod 8 != 0
//  uint32 bytes_still_to_write;
//  uint32 bytes_write_now;
//
//  uchar* current_write_pointer_base;
//  uchar* current_write_pointer;
//  word* current_element_number;
//
//  for(uint32 i = 0; i < num_elements; i++) {
//    current_write_pointer_base = dest_pointer + i * gate_size;
//    current_element_number = (word*)current_write_pointer_base;
//    // This writes one complete element
//    do {
//      bytes_still_to_write = BigIntLib::field_size_bytes_;
//      current_write_pointer = current_write_pointer_base;
//      while(bytes_still_to_write > 0) {
//        bytes_write_now = std::min(bytes_still_to_write, buffer_bytes_left);
//        memcpy(current_write_pointer, BigIntLib::evp_cipher_buffer_ + buffer_bytes_used, bytes_write_now);
//        buffer_bytes_left -= bytes_write_now;
//        bytes_still_to_write -= bytes_write_now;
//        current_write_pointer += bytes_write_now;
//        buffer_bytes_used += bytes_write_now;
//
//        if(buffer_bytes_left == 0) {
//          // Buffer is empty, get new AES bytes (maybe write next value already here?)
//          EVP_EncryptUpdate(BigIntLib::evp_cipher_ctx_, BigIntLib::evp_cipher_buffer_, &length, plaintext, sizeof(plaintext));
//          buffer_bytes_left = 16;
//          buffer_bytes_used = 0;
//        }
//
//      }
//
//      // Cancel out last word
//      current_element_number[msb_word_index] &= BigIntLib::msb_word_mask_;
//    } while(BigIntLib::IsZero(current_element_number));
//  }
//}
//
//void BigIntLib::MulSpec(word* c, word* a) {
//  (this->*MUL_SPEC_FUNCTION)(c, a);
//}
//
//void BigIntLib::SolinasReduc(word* out, word* in) {
//  (this->*REDUC_FUNCTION)(out, in);
//}
//
//void BigIntLib::AddSpec(word* c, word* a, word* b) {
//  (this->*ADD_SPEC_FUNCTION)(c, a, b);
//}
//
//void BigIntLib::DoubleAdd(word* c, word* a, word* b) {
//  #ifdef VERBOSE
//  std::cout << "--- DOUBLEADD ---" << std::endl;
//  std::cout << "Copy for Sage:" << std::endl;
//  std::cout << "(0x" << BigIntLib::ToString(a) << " * 0x" << BigIntLib::ToString(b) << ") \% 0x" << BigIntLib::ToString(BigIntLib::modulo_) << std::endl;
//  #endif
//  (this->*DOUBLE_ADD_FUNCTION)(c, a, b);
//  #ifdef VERBOSE
//  std::cout << "Result: 0x" << BigIntLib::ToString(c) << std::endl;
//  #endif
//}
//
//void BigIntLib::Times2PF(word* c, word* a) {
//  BigIntLib::Add(c, a, a);
//}
//
//void BigIntLib::Times3PF(word* c, word* a) {
//  BigIntLib::Add(c, a, a);
//  BigIntLib::Add(c, c, a);
//}
//
//void BigIntLib::Times2(word* c, word* a) {
//  (this->*TIMES_2_FUNCTION)(c, a);
//}
//
//void BigIntLib::Times3(word* c, word* a) {
//  (this->*TIMES_3_FUNCTION)(c, a);
//}
//
//std::string BigIntLib::ToString(void* source, uint32 num_bytes) {
//  if(num_bytes == 0) num_bytes = BigIntLib::field_size_bytes_;
//  uchar* pointer = (uchar*)source;
//  std::ostringstream string_stream;
//  for(uint32 i = 0; i < num_bytes; i++) {
//    string_stream << std::setfill('0') << std::setw(2) << std::hex << (uint32)(pointer[num_bytes - i - 1]);
//  }
//  std::string ret_string = string_stream.str();
//  return ret_string;
//}

