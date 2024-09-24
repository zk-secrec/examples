# Log Processing

## Introduction

In this example, we present the statement for solving the following problem. A web service owner needs to prove to their partners that the service has not been impacted by recently discovered vulnerabilities. This could be checked from the web server access logs. But the service owner is reluctant to publish all its web server access logs for privacy reasons. On the other hand, even if the service owner did it, partners could suspect that the owner would have withheld some relevant parts.

Hence the web service owner and their partners agree on the following. The owner signes the access log file daily and publishes that signature to the partners. But the partners make queries at random times to the web service to leave distinctive entries, so-called *canaries*, to the access log. As the web service owner does not know the canaries, the owner may unintentionally remove some of them if they decided to hide some parts of the access log. This would indicate to the partners that the access log provided by the owner is not authentic. After receiving the signature from the service owner, the partners publish their canaries and also the indicators of compromise (IOC) that they are interested in. Then the zero-knowledge proof that the access log contains all canaries and does not contain any IOCs is set up.

## The statement

### Inputs

#### Prover's inputs

In this scenario, the web service owner is the prover. Its daily access log is its private input. The statement that is to be proven in ZK inputs the access log as a list of characters, where each character is a number between 0 and 255.

#### Verifier's inputs

In this scenario, the verifiers are the partners of the web service owner. Their input contains the canaries, the IOCs, and the signature of the access log. The latter is needed for proving that the private access log is authentic. But we have omitted signature verification from the implementation of the current example. (We have implemented it in context of other use cases. In fact, this scenario does not essentially require signing, just time stamping would provide the same level of certainty.)

#### Public inputs

The set of public inputs consists of the lengths of all canaries and IOCs and also the length of the access log.

### Checks and computations

The statement that is to be proven in ZK consists of the following checks:

* Every canary occurs in the access log;
* No IOC occurs in the access log.

So the core of the computation is substring search.

## Implementation

### JSON objects

In the subdirectory `input`, we have included one file for each of the prover's, the verifier's and public inputs, with names `witness00.json`, `instance00.json` and `public00.json`, respectively. In addition to the prover input discussed above, the corresponding input file provides the list of positions where the canaries occur in the access log (in the order of occurrence of canaries in the verifier's input). This input is consumed by solutions that ask the prover to provide the occurrence positions as expanded witness (instead of checking all possibilities).

### ZK-SecreC source programs

In the subdirectory `src`, we have included our implementation of the described relation in ZK-SecreC. There are four types of ZKSC files:

* `log-proc-pos*.zksc` contain different approaches to checking if all canaries occur in the access log;
* `log-proc-neg*.zksc` contain different approaches to checking if no IOC occurs in the access log;
* `log-proc-joint*.zksc` contain implementations of both checking the occurrence of canaries and missing of IOCs;
* `RabinKarp.zksc` contains auxiliary functions used by approaches based on the Rabin-Karp substring search algorithm (more precisely, `log-proc-pos_rk.zksc`, `log-proc-neg_rk.zksc` and both of `log-proc-joint*.zksc`).

All files with name `log-proc-*.zksc` contain function `main`, i.e., each of them contains a program that can be executed with the ZK-SecreC compiler.

### Configuration files

In the subdirectory `config`, we have included two Circuit Configuration Communication (CCC) files. Both specify the same fields, conversions and plugins that the ZK back-end must support. In addition, `log-proc-chal.ccc` declares support for verifier's challenges, while `log-proc.ccc` does not. In the variants `log-proc-pos_stor.zksc` and `log-proc-joint_te.zksc` that make use of stores, the ZK-SecreC compiler automatically chooses between stores implemented with verifier's challenges or without them. Note that `log-proc-pos_rk.zksc` and `log-proc-joint_rk.zksc` do not work without challenges, so these variant must be executed with `log-proc-chal.ccc`. Challenges are used there for randomly choosing the base value at which point to evaluate the substrings as polynomials.

## Helper script

The subdirectory `test-gen` contains a Haskell program `Cybersecurity.hs` (together with its imports `JSON.hs` and `Util.hs`) that is capable of generating inputs (`public.json`, `instance.json`, and `witness.json`) of various sizes for the log processing statement. The "size" parameter for this script determines the length of the randomly generated access log, with the increase of 1 in the parameter corresponding to doubling of the access log.

