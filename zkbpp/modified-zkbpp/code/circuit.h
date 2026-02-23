// For testing, run the following:
//   cp instance.txt generated_instance.txt
//   cp witness.txt generated_witness.txt
//   cp circuit.h generated_circuit.h
//   echo > generated_header.h
//   ./_compile_zkbpp.sh
//   ./zkbpp_test 272 1 0 1 1

void CircuitContainer::initConsts() {
  this->num_instance_values_ = 2;
  this->num_witness_values_ = 2;
  this->num_mul_gates_ = 1;
  this->num_assert_zeros_ = 4;
}

void CircuitContainer::directImpl() {
  word x1[this->gate_num_words_];
  getWitnessDirect(x1);
  word x2[this->gate_num_words_];
  mulDirect(x2, x1, x1);
  word x3[this->gate_num_words_];
  BigIntLib::Sub(x3, x2, x1);
  assertZeroDirect(x3);

  word x4[this->gate_num_words_];
  getWitnessDirect(x4);
  word x5[this->gate_num_words_];
  BigIntLib::Add(x5, x1, x1);
  word x6[this->gate_num_words_];
  BigIntLib::Sub(x6, x5, x4);
  assertZeroDirect(x6);

  word x7[this->gate_num_words_];
  getInstanceDirect(x7);
  word x8[this->gate_num_words_];
  getInstanceDirect(x8);
  word x9[this->gate_num_words_];
  BigIntLib::Sub(x9, x7, x4);
  assertZeroDirect(x9);

  word x10[this->gate_num_words_];
  word c1[5] = { 19, 0, 0, 0, 0 };
  BigIntLib::Add(x10, x1, c1);
  word x11[this->gate_num_words_];
  word c2[5] = { 4, 0, 0, 0, 0 };
  BigIntLib::Mul(x11, x8, c2);
  word x12[this->gate_num_words_];
  BigIntLib::Sub(x12, x11, x10);
  assertZeroDirect(x12);
}

void CircuitContainer::circuitImpl(uint32 party_size) {
  word x1[party_size][this->gate_num_words_];
  (this->*get_witness_shared_function_)((word*)x1);
  word x2[party_size][this->gate_num_words_];
  (this->*mul_shared_function_)((word*)x1, (word*)x1, (word*)x2);
  word x3[party_size][this->gate_num_words_];
  (this->*sub_shared_function_)((word*)x2, (word*)x1, (word*)x3);
  (this->*assert_zero_shared_function_)((word*)x3);

  word x4[party_size][this->gate_num_words_];
  (this->*get_witness_shared_function_)((word*)x4);
  word x5[party_size][this->gate_num_words_];
  (this->*add_shared_function_)((word*)x1, (word*)x1, (word*)x5);
  word x6[party_size][this->gate_num_words_];
  (this->*sub_shared_function_)((word*)x5, (word*)x4, (word*)x6);
  (this->*assert_zero_shared_function_)((word*)x6);

  word x7[party_size][this->gate_num_words_];
  (this->*get_instance_shared_function_)((word*)x7);
  word x8[party_size][this->gate_num_words_];
  (this->*get_instance_shared_function_)((word*)x8);
  word x9[party_size][this->gate_num_words_];
  (this->*sub_shared_function_)((word*)x7, (word*)x4, (word*)x9);
  (this->*assert_zero_shared_function_)((word*)x9);

  word x10[party_size][this->gate_num_words_];
  word c1[5] = { 19, 0, 0, 0, 0 };
  (this->*add_c_shared_function_)((word*)x1, c1, (word*)x10);
  word x11[party_size][this->gate_num_words_];
  word c2[5] = { 4, 0, 0, 0, 0 };
  (this->*mul_c_shared_function_)((word*)x8, c2, (word*)x11);
  word x12[party_size][this->gate_num_words_];
  (this->*sub_shared_function_)((word*)x11, (word*)x10, (word*)x12);
  (this->*assert_zero_shared_function_)((word*)x12);
}
