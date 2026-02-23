/*
 * Copyright 2026 Cybernetica AS
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

use crate::builtins::{
    cast_to_pre, get_int, get_wire, is_int_const_or_pre, is_unknown, unslice_helper, SoA,
};
use crate::externs_header::*;
use crate::rep;
use crate::sieve_backend;
use crate::DomainType::Prover;
use crate::ValueView;
use crate::CURR_DOMAIN;
use crate::NEED_REL;
use num_traits::{One, ToPrimitive, Zero};

pub fn array_to_prover_bitwise(
    _ctx: &ContextRef,
    _: &mut Stack,
    _d: DomainType,
    _m: &NatType,
    v: &Value,
) -> Value {
    v.clone()
}

pub fn mulv(ctx: &ContextRef, _: &mut Stack, m: &NatType, x: &Value, c: &Value) -> Value {
    let r = if is_unknown(x) || is_unknown(c) {
        ctx.unknown.clone()
    } else {
        (m.mul)(get_int(x), get_int(c))
    };
    if NEED_REL && !is_int_const_or_pre(x) {
        let w1 = get_wire(x);
        let w = sieve_backend().mulv(m, w1, c);
        rep::Post::new(w, r)
    } else {
        r
    }
}

pub fn shlc_pre(
    ctx: &ContextRef,
    _: &mut Stack,
    _: DomainType,
    r: &NatType,
    x: &Value,
    n: u64,
) -> Value {
    if is_unknown(x) {
        ctx.unknown.clone()
    } else {
        (r.from_bigint)(&((r.to_bigint)(x) << n))
    }
}

pub fn shrc_pre(
    ctx: &ContextRef,
    _: &mut Stack,
    _: DomainType,
    r: &NatType,
    x: &Value,
    n: u64,
) -> Value {
    if is_unknown(x) {
        ctx.unknown.clone()
    } else {
        (r.from_bigint)(&((r.to_bigint)(x) >> n))
    }
}

pub fn shlc_post(ctx: &ContextRef, stack: &mut Stack, m: &NatType, x: &Value, n: u64) -> Value {
    let r = shlc_pre(ctx, stack, Prover, m, cast_to_pre(x), n);
    if NEED_REL && !is_int_const_or_pre(x) {
        let w1 = get_wire(x);
        let w = sieve_backend().shlc(m, w1, n);
        rep::Post::new(w, r)
    } else {
        r
    }
}

pub fn shrc_post(ctx: &ContextRef, stack: &mut Stack, m: &NatType, x: &Value, n: u64) -> Value {
    let r = shrc_pre(ctx, stack, Prover, m, cast_to_pre(x), n);
    if NEED_REL && !is_int_const_or_pre(x) {
        let w1 = get_wire(x);
        let w = sieve_backend().shrc(m, w1, n);
        rep::Post::new(w, r)
    } else {
        r
    }
}

fn transpose_bit_matrix_post_helper(
    ctx: &ContextRef,
    m1: &NatType,
    m2: &NatType,
    nr: u64,
    nc: u64,
    xs: &Vec<Value>,
) -> Vec<Value> {
    let mut any_unknown = false;
    for x in xs {
        if is_unknown(x) {
            any_unknown = true;
            break;
        }
    }
    if any_unknown {
        let mut ys = Vec::new();
        for _ in 0..nc as usize {
            ys.push(ctx.unknown.clone());
        }
        ys
    } else {
        let xs: Vec<u64> = xs
            .iter()
            .map(|x| (m1.to_bigint)(x).to_u64().unwrap())
            .collect();
        let mut ys = Vec::new();
        for i in 0..nc as usize {
            let mut r = 0u64;
            for j in 0..nr as usize {
                r = r ^ (((xs[j] >> i) & 1) << j);
            }
            ys.push(r);
        }
        let ys: Vec<Value> = ys
            .into_iter()
            .map(|y| (m2.from_bigint)(&BigInt::from(y)))
            .collect();
        ys
    }
}

pub fn transpose_bit_matrix_post(
    ctx: &ContextRef,
    _: &mut Stack,
    m1: &NatType,
    m2: &NatType,
    nr: u64,
    nc: u64,
    x: &Value,
) -> Value {
    match x.view() {
        ValueView::PostSoA(SoA::Scalar((wr1, xs))) => {
            let wr = sieve_backend().bit_matrix_transpose(m1, m2, wr1, nr, nc);
            let ys = transpose_bit_matrix_post_helper(ctx, m1, m2, nr, nc, xs);
            rep::PostSoA::new(SoA::Scalar((wr, ys)))
        }
        ValueView::Array(xs) => {
            rep::Array::new(transpose_bit_matrix_post_helper(ctx, m1, m2, nr, nc, xs))
        }
        _ => panic!(),
    }
}

pub fn ecdsa_verification(
    _ctx: &ContextRef,
    _: &mut Stack,
    m: &NatType,
    _q: &NatType,
    bits_per_block: u64,
    fixpowers1: &Vec<Value>,
    scalar1: &Value,
    fixpowers2: &Vec<Value>,
    scalar2: &Value,
    r_x: &Value,
    r_y: &Value,
) {
    for x in fixpowers1 {
        sieve_backend().add_instance(m, x);
    }
    for x in fixpowers2 {
        sieve_backend().add_instance(m, x);
    }
    let zero = (m.from_bigint)(&BigInt::zero());
    sieve_backend().add_witness(m, &zero);
    sieve_backend().add_witness(m, &zero);
    sieve_backend().ecdsa_verification(
        m,
        bits_per_block,
        get_wire(scalar1),
        get_wire(scalar2),
        get_wire(r_x),
        get_wire(r_y),
    );
}

fn bitwise_vec_to_bitwise_vec_helper(
    ctx: &ContextRef,
    m1: &NatType,
    m2: &NatType,
    xs: &Vec<Value>,
    n_from: usize,
) -> Vec<Value> {
    //let n_from = xs.len();
    let from_bits = m1.modulus.as_ref().unwrap().to_u64().unwrap();
    let to_bits = m2.modulus.as_ref().unwrap().to_u64().unwrap();
    let n_to = ((n_from as u64 * from_bits + to_bits - 1) / to_bits) as usize;
    let mut any_unknown = false;
    for x in xs {
        if is_unknown(x) {
            any_unknown = true;
            break;
        }
    }
    if any_unknown {
        let mut ys = Vec::new();
        for _ in 0..n_to {
            ys.push(ctx.unknown.clone());
        }
        ys
    } else {
        let xs: Vec<u64> = xs
            .iter()
            .map(|x| (m1.to_bigint)(x).to_u64().unwrap())
            .collect();
        let mut ys = Vec::new();
        ys.resize(n_to, 0);
        if from_bits % to_bits == 0 {
            let block_len = (from_bits / to_bits) as usize;
            let mut to_i = 0;
            let word_mask = ((1u128 << to_bits) - 1) as u64;
            for i in 0..n_from {
                let mut t = xs[i];
                to_i += block_len;
                for _ in 0..block_len {
                    to_i -= 1;
                    ys[to_i] = t & word_mask;
                    t >>= to_bits;
                }
                to_i += block_len;
            }
        } else if to_bits % from_bits == 0 {
            let block_len = (to_bits / from_bits) as usize;
            let mut from_i = 0;
            let mut i = 0;
            while i + 1 < n_to {
                let mut t = xs[from_i];
                from_i += 1;
                for _ in 1..block_len {
                    t = (t << from_bits) ^ xs[from_i];
                    from_i += 1;
                }
                ys[i] = t;
                i += 1;
            }
            let mut t = xs[from_i];
            from_i += 1;
            for _ in 1..block_len {
                t <<= from_bits;
                if from_i < n_from {
                    t ^= xs[from_i];
                    from_i += 1;
                }
            }
            ys[n_to - 1] = t;
        } else {
            panic!("bitwiseVecToBitwiseVec can be used only if the number of bits of one bitwise ring is a multiple of the number of bits of the other");
        }
        let ys: Vec<Value> = ys
            .into_iter()
            .map(|y| (m2.from_bigint)(&BigInt::from(y)))
            .collect();
        ys
    }
}

pub fn bitwise_vec_to_bitwise_vec(
    ctx: &ContextRef,
    _: &mut Stack,
    m1: &NatType,
    m2: &NatType,
    x: &Value,
) -> Value {
    let x = unslice_helper(x);
    match x.view() {
        ValueView::PostSoA(SoA::Scalar((wr1, xs))) => {
            let wr = sieve_backend().bitwise_vec_to_bitwise_vec(m1, m2, wr1);
            let ys = bitwise_vec_to_bitwise_vec_helper(ctx, m1, m2, xs, wr1.length());
            rep::PostSoA::new(SoA::Scalar((wr, ys)))
        }
        ValueView::Array(xs) => {
            rep::Array::new(bitwise_vec_to_bitwise_vec_helper(ctx, m1, m2, xs, xs.len()))
        }
        _ => panic!(),
    }
}

fn concat_vec_helper(ctx: &ContextRef, xs: &Vec<Value>, ys: &Vec<Value>, n: usize) -> Vec<Value> {
    let mut any_unknown = false;
    for x in xs {
        if is_unknown(x) {
            any_unknown = true;
            break;
        }
    }
    if !any_unknown {
        for y in ys {
            if is_unknown(y) {
                any_unknown = true;
                break;
            }
        }
    }
    if any_unknown {
        let mut zs = Vec::new();
        for _ in 0..n {
            zs.push(ctx.unknown.clone());
        }
        zs
    } else {
        let mut zs = Vec::new();
        for x in xs {
            zs.push(x.clone());
        }
        for y in ys {
            zs.push(y.clone());
        }
        zs
    }
}

pub fn concat_vec(ctx: &ContextRef, _: &mut Stack, m1: &NatType, x: &Value, y: &Value) -> Value {
    let x = unslice_helper(x);
    let y = unslice_helper(y);
    match (x.view(), y.view()) {
        (
            ValueView::PostSoA(SoA::Scalar((wr1, xs))),
            ValueView::PostSoA(SoA::Scalar((wr2, ys))),
        ) => {
            let wr = sieve_backend().concat_vec(m1, wr1, wr2);
            let zs = concat_vec_helper(ctx, xs, ys, wr1.length() + wr2.length());
            rep::PostSoA::new(SoA::Scalar((wr, zs)))
        }
        (ValueView::Array(xs), ValueView::Array(ys)) => {
            rep::Array::new(concat_vec_helper(ctx, xs, ys, xs.len() + ys.len()))
        }
        _ => panic!(),
    }
}

#[allow(unused)]
pub fn transpose_bit_matrix_uint_pre(
    _: &ContextRef,
    _: &mut Stack,
    x: &Vec<Value>,
    n: u64,
) -> Vec<Value> {
    let n = n as usize;
    let m = x.len();
    let mut res: Vec<Value> = Vec::new();
    let one = BigInt::one();
    for i in 0..n {
        let mut r = BigInt::zero();
        for j in 0..m {
            let y: &BigInt = (&x[j]).into();
            r = r ^ (((y >> i) & &one) << j);
        }
        res.push(r.into());
    }
    res
}

const ROUND_K: [u32; 64] = [
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
];

fn transpose_bit_matrix_u32_64(x: &[u32; 64]) -> [u64; 32] {
    let mut res: [u64; 32] = [0; 32];
    for i in 0..32 {
        let mut r = 0u64;
        for j in 0..64 {
            r = r ^ ((((x[j] >> i) & 1) as u64) << j);
        }
        res[i] = r;
    }
    res
}

pub fn string_to_list(_: &ContextRef, _: &mut Stack, x: &String) -> Vec<u8> {
    x.chars().map(|c| c as u8).collect()
}

pub fn print_as_string(_: &ContextRef, _: &mut Stack, x: &Vec<u8>) {
    println!("print_as_string: length = {}", x.len());
    let x: String = x.iter().map(|c| *c as char).collect();
    println!("{}", x);
}

pub fn get_round_k_transposed(_: &ContextRef, _: &mut Stack) -> Vec<u64> {
    transpose_bit_matrix_u32_64(&ROUND_K).into_iter().collect()
}

pub fn sha256_block_compute_witness_u32(
    _: &ContextRef,
    _: &mut Stack,
    h_init: &Vec<u32>,
    msg_block: &Vec<u32>,
) -> (Vec<u64>, Vec<u64>, Vec<u64>) {
    //println!("{}", h_init.len());
    //println!("{}", msg_block.len());
    //println!("{}", h_init[0]);
    if CURR_DOMAIN == Prover {
        let mut w: [u32; 64] = [0; 64];
        for i in 0..16 {
            w[i] = msg_block[i];
        }
        for i in 16..64 {
            let s0 = w[i - 15].rotate_right(7) ^ w[i - 15].rotate_right(18) ^ (w[i - 15] >> 3);
            let s1 = w[i - 2].rotate_right(17) ^ w[i - 2].rotate_right(19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16]
                .wrapping_add(s0)
                .wrapping_add(w[i - 7])
                .wrapping_add(s1);
        }
        let mut a = h_init[0];
        let mut b = h_init[1];
        let mut c = h_init[2];
        let mut d = h_init[3];
        let mut e = h_init[4];
        let mut f = h_init[5];
        let mut g = h_init[6];
        let mut h = h_init[7];

        let mut a_arr: [u32; 64] = [0; 64];
        let mut e_arr: [u32; 64] = [0; 64];

        for i in 0..64 {
            a_arr[i] = a;
            e_arr[i] = e;
            let s1 = e.rotate_right(6) ^ e.rotate_right(11) ^ e.rotate_right(25);
            let ch = (e & f) ^ (!e & g);
            let temp1 = h
                .wrapping_add(s1)
                .wrapping_add(ch)
                .wrapping_add(ROUND_K[i])
                .wrapping_add(w[i]);
            let s0 = a.rotate_right(2) ^ a.rotate_right(13) ^ a.rotate_right(22);
            let maj = (a & b) ^ (a & c) ^ (b & c);
            let temp2 = s0.wrapping_add(maj);
            h = g;
            g = f;
            f = e;
            e = d.wrapping_add(temp1);
            d = c;
            c = b;
            b = a;
            a = temp1.wrapping_add(temp2);
        }

        //let a_to_h_out: [u32; 8] = [a, b, c, d, e, f, g, h];
        //for i in 0 .. 8 {
        //    println!("{}: {:08x}", i, h_init[i].wrapping_add(a_to_h_out[i]));
        //}
        //for i in 0 .. 8 {
        //    println!("{}: {}", i, h_init[i].wrapping_add(a_to_h_out[i]));
        //}

        let w_res = transpose_bit_matrix_u32_64(&w).into_iter().collect();
        let a_res = transpose_bit_matrix_u32_64(&a_arr).into_iter().collect();
        let e_res = transpose_bit_matrix_u32_64(&e_arr).into_iter().collect();
        (w_res, a_res, e_res)
    } else {
        let res: Vec<u64> = [0u64; 32].into_iter().collect();
        (res.clone(), res.clone(), res)
    }
}
