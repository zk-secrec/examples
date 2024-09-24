// This Rust file implements the extern functions declared in example_ffi.zksc.

// The module crate::externs_header, which re-exports some essential types,
// needs to be imported in every Rust file implementing ZK-SecreC extern functions:
use crate::externs_header::*;

// Extern function implementations must use the pub keyword, so that they can be used from the ZK-SecreC code.
// The first two parameters must be of types &ContextRef and &mut Stack,
// the variables can be _ as they are usually not needed.
// The next parameters are the type parameters (other than Unqualified) of the ZKSC function,
// the Rust type depends on the kind of the ZKSC type parameter,
// according to the following table:
//   ZKSC         Rust
//   -----------------------------------
//   domain(@)    DomainType
//   stage($)     StageType
//   Nat          &NatType
//   Qualified    QualifiedType
//   Unqualified  (no parameter in Rust)
//
// After that, there will be the ordinary parameters of the ZKSC function.
// Here, we have _d: DomainType corresponding to the @D in ZKSC,
// and x: u64 corresponding to x : u64 $pre @D in ZKSC.
// The variable names do not have to be the same as in ZKSC.
pub fn sqr(_: &ContextRef, _: &mut Stack, _d: DomainType, x: u64) -> u64 {
    x * x
}

pub fn sqr_u128(_: &ContextRef, _: &mut Stack, _d: DomainType, x: u128) -> u128 {
    x * x
}

pub fn cond(_: &ContextRef, _: &mut Stack, _d: DomainType, b: bool, x: u64, y: u64) -> u64 {
    if b { x } else { y }
}

pub fn print_i128(_: &ContextRef, _: &mut Stack, _d: DomainType, x: u128) {
    println!("{}", x as i128);
}

pub fn flist(_: &ContextRef, _: &mut Stack, xs: &Vec<bool>) -> Vec<bool> {
    for x in xs {
        println!("flist: {}", x);
    }
    xs.clone()
}

pub fn farr(_: &ContextRef, _: &mut Stack, xs: &Vec<u128>) -> Vec<u128> {
    for x in xs {
        println!("farr: {}", x);
    }
    xs.clone()
}

// The element lists of a multidimensional list are represented as Value
// but &Value can be converted to a Rust vector using the into() method from the Into trait
// and a Rust vector can be converted to a Value also using the into() method.
pub fn flistlist(_: &ContextRef, _: &mut Stack, xs: &Vec<Value>) -> Vec<Value> {
    for ys in xs {
        println!("flistlist");
        let ys: &Vec<u128> = ys.into();
        for x in ys {
            println!("flistlist: {}", x);
        }
    }
    let mut xs2 = Vec::new();
    for i in 0 .. xs.len() {
        let ys: &Vec<u128> = (&xs[i]).into();
        let mut ys2 = Vec::new();
        for j in 0 .. ys.len() {
            ys2.push(ys[j] * 2);
        }
        xs2.push(ys2.into());
    }
    xs2
}

pub fn ftuple2(_: &ContextRef, _: &mut Stack, xs: (u64, u128)) -> (u64, u128) {
    println!("ftuple2: ({}, {})", xs.0, xs.1);
    xs
}

pub fn ftuple3(_: &ContextRef, _: &mut Stack, xs: (u64, u128, u128)) -> (u64, u128, u128) {
    println!("ftuple3: ({}, {}, {})", xs.0, xs.1, xs.2);
    xs
}

// uint $pre @D is translated to BigInt.
pub fn ftuple4(_: &ContextRef, _: &mut Stack, xs: (&BigInt, u128, bool, ())) -> (BigInt, u128, bool, ()) {
    println!("ftuple4: ({}, {}, {}, ())", xs.0, xs.1, xs.2);
    (xs.0 + 1, xs.1 * 2, !xs.2, ())
}

pub fn ftuple5(_: &ContextRef, _: &mut Stack, xs: (u64, u128, u8, u16, u32)) -> (u64, u128, u8, u16, u32) {
    println!("ftuple5: ({}, {}, {}, {}, {})", xs.0, xs.1, xs.2, xs.3, xs.4);
    xs
}

// Tuples with more than 5 components are translated to Box<[Value]> instead of Rust tuples.
// Box<[Value]> can also be used with tuples with fewer elements to avoid copying the components during conversion,
// e.g. for large nested tuples with at most 5 components for each individual tuple.
pub fn ftuple6(_: &ContextRef, _: &mut Stack, x: &Box<[Value]>) -> Box<[Value]> {
    // Here we also use the into() method to convert from &Value or to Value.
    let x0: u64 = (&x[0]).into();
    let x1: u128 = (&x[1]).into();
    let x2: u8 = (&x[2]).into();
    let x3: u16 = (&x[3]).into();
    let x4: u32 = (&x[4]).into();
    let x5: u64 = (&x[5]).into();
    println!("ftuple6: ({}, {}, {}, {}, {}, {})", x0, x1, x2, x3, x4, x5);
    Box::new([(x0 + 1).into(), (x1 * 2).into(), x2.into(), x3.into(), x[4].clone(), x[5].clone()])
}

pub fn fnestedtuple1(_: &ContextRef, _: &mut Stack, xs: ((u64, u128), u64)) -> ((u64, u128), u64) {
    println!("fnestedtuple1: (({}, {}), {})", xs.0.0, xs.0.1, xs.1);
    xs
}

pub fn fnestedtuple2(_: &ContextRef, _: &mut Stack, xs: ((&Value, &Vec<u128>, bool), u64)) -> ((Value, Vec<u128>, bool), u64) {
    println!("fnestedtuple2: ((?, [{}, {}], {}), {})", xs.0.1[0], xs.0.1[1], xs.0.2, xs.1);
    ((xs.0.0.clone(), xs.0.1.clone(), xs.0.2), xs.1)
}

// Tuples that are elements of lists are represented as Value and need to be converted using into().
pub fn flisttuple(_: &ContextRef, _: &mut Stack, xs: &Vec<Value>) -> Vec<Value> {
    let mut xs2 = Vec::new();
    for i in 0 .. xs.len() {
        let pair: (u64, u128) = (&xs[i]).into();
        println!("flisttuple: ({}, {})", pair.0, pair.1);
        xs2.push((pair.0 * 3, pair.1 * 4).into());
    }
    xs2
}

// Structs of any size are translated to Box<[Value]> like large tuples.
pub fn fstruct(_: &ContextRef, _: &mut Stack, x: &Box<[Value]>) -> Box<[Value]> {
    let x0: u64 = (&x[0]).into();
    let x1: u128 = (&x[1]).into();
    println!("fstruct: ({}, {})", x0, x1);
    Box::new([(x0 + 1).into(), (x1 * 2).into()])
}

pub fn fstring(_: &ContextRef, _: &mut Stack, x: &String) -> String {
    println!("fstring: {}", x);
    x.clone() + "!"
}

// Parametrically polymorphic functions can be implemented without knowing how the compiler works internally.
// The elements are of type Value, which can represent any ZKSC type, but the only thing that is done
// with Value is cloning it (to convert &Value to Value), which just increases the reference count and takes constant time.
pub fn reverse_list(_: &ContextRef, _: &mut Stack, _: QualifiedType, xs: &Vec<Value>) -> Vec<Value> {
    let mut ys = Vec::new();
    for i in 0 .. xs.len() {
        ys.push(xs[xs.len() - 1 - i].clone());
    }
    ys
}

// Functions with &mut arguments (corresponding to ref arguments in ZKSC):
pub fn fmutlist(_: &ContextRef, _: &mut Stack, xs: &mut Vec<u128>) {
    for i in 0 .. xs.len() {
        println!("fmutlist: {}", xs[i]);
    }
    xs[0] = xs[0] + 1;
    xs[1] = xs[1] * 2;
}

pub fn fmutlistb(_: &ContextRef, _: &mut Stack, _: &NatType, _: DomainType, _: DomainType, xs: &mut Vec<bool>) {
    for i in 0 .. xs.len() {
        println!("fmutlist: {}", xs[i]);
    }
    xs[0] = !xs[0];
    xs[1] = !xs[1];
}

pub fn fmutlistlist(_: &ContextRef, _: &mut Stack, xs: &mut Vec<Value>) {
    for ys in &mut *xs {
        println!("fmutlistlist");
        let ys: &mut Vec<u128> = ys.into();
        for x in ys {
            println!("fmutlistlist: {}", x);
            *x = *x + 1;
        }
    }
    for i in 0 .. xs.len() {
        let ys: &mut Vec<u128> = (&mut xs[i]).into();
        for j in 0 .. ys.len() {
            ys[j] = ys[j] * 2;
        }
    }
}

// Mutable tuples can only be translated to &mut Box<[Value]>, not to (&mut T1, ..., &mut Tn).
pub fn fmuttuple6(_: &ContextRef, _: &mut Stack, x: &mut Box<[Value]>) {
    let x0: u64 = (&x[0]).into();
    let x1: u128 = (&x[1]).into();
    let x2: u8 = (&x[2]).into();
    let x3: u16 = (&x[3]).into();
    let x4: u32 = (&x[4]).into();
    let x5: u64 = (&x[5]).into();
    // Cannot take several mutable borrows of x at the same time:
    //let x0: &mut u64 = (&mut x[0]).into();
    //let x1: &mut u128 = (&mut x[1]).into();
    //let x2: &mut u8 = (&mut x[2]).into();
    //let x3: &mut u16 = (&mut x[3]).into();
    //let x4: &mut u32 = (&mut x[4]).into();
    //let x5: &mut u64 = (&mut x[5]).into();

    println!("fmuttuple6: ({}, {}, {}, {}, {}, {})", x0, x1, x2, x3, x4, x5);
    x[0] = (x5 * 2).into();
    x[5] = (x0 + 2).into();

    // An alternative:
    //let x0m: &mut u64 = (&mut x[0]).into();
    //*x0m = x5 * 2;
    //let x5m: &mut u64 = (&mut x[5]).into();
    //*x5m = x0 + 2;
}

pub fn fmutlisttuple(_: &ContextRef, _: &mut Stack, xs: &mut Vec<Value>) {
    for i in 0 .. xs.len() {
        let x: (u64, u128) = (&xs[i]).into();
        let x_mut: &mut Box<[Value]> = (&mut xs[i]).into();
        println!("fmutlisttuple: ({}, {})", x.0, x.1);
        x_mut[0] = (x.0 + 1).into();
        x_mut[1] = (x.1 * 2).into();

        // An alternative:
        //let x: &mut Box<[Value]> = (&mut xs[i]).into();
        //let x0: u64 = (&x[0]).into();
        //let x1: u128 = (&x[1]).into();
        //println!("fmutlisttuple: ({}, {})", x0, x1);
        //x[0] = (x0 + 1).into();
        //x[1] = (x1 * 2).into();
    }
}

pub fn fmutbigint(_: &ContextRef, _: &mut Stack, x: &mut BigInt) {
    println!("fmutbigint: {}", x);
    *x = &*x + 100;
}

pub fn fmutstring(_: &ContextRef, _: &mut Stack, x: &mut String) {
    println!("fmutstring: {}", x);
    *x = x.clone() + "!";
}

pub fn fmutu64(_: &ContextRef, _: &mut Stack, x: &mut u64) {
    println!("fmutu64: {}", x);
    *x = *x + 1;
}
