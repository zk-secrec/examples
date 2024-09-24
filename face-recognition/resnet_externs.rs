use crate::externs_header::*;
use crate::CURR_DOMAIN;
use num_traits::ToPrimitive;

// The extern for this is declared in ResidualNetwork.zksc
#[allow(unused)]
pub fn uint_n_pre_matrix_to_i128(_: &ContextRef, _: &mut Stack, m: &NatType, d: DomainType, xss: &Vec<Value>) -> Vec<Value> {
    if d <= CURR_DOMAIN {
        let m_i128: i128 = m.modulus.as_ref().unwrap().to_i128().unwrap();
        let half_m: i128 = m_i128/2;
        let f = |x| if x >= half_m { x - m_i128 } else { x };
        let yss: Vec<Value> = xss.iter().map(|row| {
            let row: &Vec<Value> = row.into();
            let ys: Vec<u128> = row.iter().map(|x| f((m.to_bigint)(x).to_i128().unwrap()) as u128).collect();
            ys.into()
        }).collect();
        yss
    } else {
        let yss: Vec<Value> = xss.iter().map(|row| {
            let row: &Vec<Value> = row.into();
            let nc = row.len();
            let mut ys: Vec<u128> = Vec::with_capacity(nc);
            ys.resize(nc, 0u128);
            ys.into()
        }).collect();
        yss
    }
}

// The extern for this is declared in ResidualNetwork.zksc
#[allow(unused)]
pub fn i128_pre_matrix_to_uint_n(ctx: &ContextRef, _: &mut Stack, m: &NatType, d: DomainType, xss: &Vec<Value>) -> Vec<Value> {
    if d <= CURR_DOMAIN {
        let yss: Vec<Value> = xss.iter().map(|row| {
            let row: &Vec<u128> = row.into();
            let ys: Vec<Value> = row.iter().map(|x| (m.from_bigint)(&BigInt::from(*x as i128))).collect();
            ys.into()
        }).collect();
        yss
    } else {
        let yss: Vec<Value> = xss.iter().map(|row| {
            let row: &Vec<u128> = row.into();
            let nc = row.len();
            let mut ys: Vec<Value> = Vec::with_capacity(nc);
            ys.resize(nc, ctx.unknown.clone());
            ys.into()
        }).collect();
        yss
    }
}

