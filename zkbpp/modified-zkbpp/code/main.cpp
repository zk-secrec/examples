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
#include "CircuitContainer.h"

#define NO_MEMCPY
#define NO_MEMSET
#include "utils.h"

// Creates plaintext value
void write_plaintext(uchar* plaintext, uint32 plaintext_size) {
  //memset(plaintext, 0, plaintext_size);
  for(uint32 i = 0; i < plaintext_size; i++) {
    plaintext[i] = (uchar)(i); // Write some values
  }
}

uint32 atoi2(const char *s) {
  uint32 r = 0;
  while (*s) {
    r *= 10;
    uint32 t = *s - '0';
    if (t < 0 || t > 9) {
#ifdef TESTING
      std::cerr << "atoi2 failed" << std::endl;
#endif
      exit(1);
    }
    r += t;
    s++;
  }
  return r;
}

int main(int argc, char** argv)
{
#ifdef TESTING
  std::cout << "Starting" << std::endl;
#endif

  // No error handling here...
  if(argc != 7) {
#ifdef TESTING
    std::cout << "Usage: ./program <num_field_bits> <num_branches> <field_type> <cipher_type> <zkbpp_print_result> <action>" << std::endl;
    std::cout << "Set <action> = 1 for proving, <action> = 2 for verifying, <action> = 3 for both" << std::endl;
    //std::cout << "Enter ./program help for possible choices." << std::endl;
#endif
    return 1;
  }

  //cpu_set_t set;
  //CPU_ZERO(&set);        // clear cpu mask
  //CPU_SET(0, &set);      // set cpu 0
  //sched_setaffinity(0, sizeof(cpu_set_t), &set);  // 0 is the calling process

  uint32 field_bits = atoi2(argv[1]);
  uint32 num_branches = atoi2(argv[2]);
  //uint32 value_size = ceil((field_bits * num_branches) / 8.0); // For value sizes mod 8 != 0: use uint32 value_size = ceil((branch_bits * num_branches) / 8.0);
  uint32 value_size = (field_bits * num_branches + 7) / 8;
  uint32 random_tape_size = 16;
  uint32 key_size = value_size;
  uint32 field_type = atoi2(argv[3]);
  uint32 cipher_type = atoi2(argv[4]);
  bool print_result = (bool)(atoi2(argv[5]));
  uint32 action = atoi2(argv[6]);
  bool need_prove = (bool)(action & 1);
  bool need_verify = (bool)(action & 2);

  uchar plaintext[value_size];
  uchar y[value_size];
  write_plaintext(plaintext, value_size); // DETERMINISTIC FOR TESTING! Plaintext is not actually used.
  uint32 party_size = 3;
  //uint32 zkbpp_iterations = 438; // This should be 438 later
  uint32 zkbpp_iterations = 219; // This should be 438 later
  
  
  CircuitContainer* c = new CircuitContainer();
  c->init(value_size, random_tape_size, key_size, field_bits, num_branches, field_type, party_size, need_prove);
  c->initCipher(cipher_type);

  if (need_prove) {
#ifdef TESTING
    std::cout << "Direct" << std::endl << std::flush;
#endif
    c->directEncryption(plaintext, y);
  }

  ZKBPP* zkbpp = new ZKBPP();
  zkbpp->init(party_size, zkbpp_iterations, c, print_result);

  //uint32 num_runs = 50;
  uint32 num_runs = 1;
#ifdef USE_CHRONO
  uint64 total_gensign = 0;
  uint64 total_sign = 0;
  uint64 total_genverify = 0;
  uint64 total_verify = 0;
  uint64 total_circuit_sign = 0;
  uint64 total_circuit_verify = 0;
#endif
  Proof* p;
  bool success;
  for(uint32 i = 0; i < num_runs; i++) {
    if (need_prove) {
#ifdef TESTING
      std::cout << "Proving" << std::endl << std::flush;
#endif
      p = zkbpp->sign(plaintext);
#ifdef TESTING
      std::cout << "Serializing" << std::endl << std::flush;
#endif
      zkbpp->serialize(p);
    }
    if (need_verify) {
#ifdef TESTING
      std::cout << "Deserializing" << std::endl << std::flush;
#endif
      p = zkbpp->deserialize();
#ifdef TESTING
      std::cout << "Verifying" << std::endl << std::flush;
#endif
      success = zkbpp->verify(p, plaintext, y);
#ifdef TESTING
      std::cout << "Finished verifying" << std::endl << std::flush;
#endif
    }
#ifdef USE_CHRONO
    total_gensign += zkbpp->getLastGenSignNS();
    total_sign += zkbpp->getLastSignNS();
    total_genverify += zkbpp->getLastGenVerifyNS();
    total_verify += zkbpp->getLastVerifyNS();
    total_circuit_sign += c->getLastCircuitSignNS(); // Last circuit execution of each run
    total_circuit_verify += c->getLastCircuitVerifyNS(); // Last circuit execution of each run
#endif
  }

  // Print out information
#ifdef USE_CHRONO
  float time_gensign = ((float)total_gensign / num_runs / 1000000);
  float time_sign = ((float)total_sign / num_runs / 1000000);
  float time_sign_total =  time_gensign + time_sign;
  float time_genverify = ((float)total_genverify / num_runs / 1000000);
  float time_verify = ((float)total_verify / num_runs / 1000000);
  float time_verify_total = time_genverify + time_verify;
  float time_circuit_sign = ((float)(total_circuit_sign) / num_runs / 1000000);
  float time_circuit_verify = ((float)(total_circuit_verify) / num_runs / 1000000);
  float time_circuits_sign_all = (((float)(total_circuit_sign / num_runs) / 1000000) * zkbpp_iterations);
  float time_circuits_verify_all = (((float)(total_circuit_verify / num_runs) / 1000000) * zkbpp_iterations);
#endif
#ifdef TESTING
  std::cout << "--- CONFIGURATION ---" << std::endl;
  std::cout << "Number of test runs: " << num_runs << std::endl;
  std::cout << "--- CIRCUIT ---" << std::endl;
  std::cout << "Field type: " << ((field_type == 0) ? "Prime field" : ((field_type == 1) ? "Binary field" : ((field_type == 2) ? "Ring modulo 2^n" : "Bitwise ring"))) << std::endl;
  std::cout << "Field size (bits): " << field_bits << std::endl;
  std::cout << "Value size (bytes): " << value_size << std::endl;
  std::cout << "Key size (bytes): " << key_size << std::endl;
#ifdef USE_CHRONO
  std::cout << "--- TIME ---" << std::endl;
  std::cout << "Average time for gensign: " << time_gensign << " ms" << std::endl;
  std::cout << "Average time for sign: " << time_sign << " ms" << std::endl;
  std::cout << "Average time for sign (total): " << time_sign_total << " ms" << std::endl;
  std::cout << "Average time for genverify: " << time_genverify << " ms" << std::endl;
  std::cout << "Average time for verify: " << time_verify << " ms" << std::endl;
  std::cout << "Average time for verify (total): " << time_verify_total << " ms" << std::endl;
  std::cout << "Average time for circuit sign: " << time_circuit_sign << " ms" << std::endl;
  std::cout << "Average time for circuit verify: " << time_circuit_verify << " ms" << std::endl;
  std::cout << "Average time for all circuits sign: " << time_circuits_sign_all << " ms" << std::endl;
  std::cout << "Average time for all circuits verify: " << time_circuits_verify_all << " ms" << std::endl;
#endif
  std::cout << "--- ZKB++ ---" << std::endl;
  std::cout << "Number of ZKB++ iterations: " << zkbpp_iterations << std::endl;
#endif
  if (need_verify) {
    if(success == true) {
#ifdef TESTING
      std::cout << "Result: ACCEPT" << std::endl;
#endif
    } else {
#ifdef TESTING
      std::cout << "Result: !!!!!!!!!! REJECT !!!!!!!!!!"  << std::endl;
#endif
      return 2;
    }
  }

  // Clean up
  delete zkbpp;
  delete c;

  return 0;
}
