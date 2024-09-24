# ZK-SecreC Examples

## Introduction

ZK-SecreC is a programming language for expressing statements that we may want to prove under zero-knowledge (ZK), using a protocol for ZK proofs. Having gotten its start from DARPA's [SIEVE program](https://www.darpa.mil/program/securing-information-for-encrypted-verification-and-evaluation), the compiler for ZK-SecreC is capable of translating ZK-SecreC programs into the [common intermediate representation](https://github.com/sieve-zk/ir) of SIEVE. The compiler is also able to directly interface with several ZK proof backends. The [compiler](https://github.com/zk-secrec) and its [documentation](https://github.com/zk-secrec) are publicly available.

This repository contains examples of ZK-SecreC. This hopefully complement the documentation, allowing for a shallower learning curve and faster exploitation. The examples consist of several small examples, as well as a few larger use-cases that have been built using ZK-SecreC.

## Included examples

The following examples are in the following folders:

* `examples` folder contains several small examples, each of which demonstrates a single feature and/or solves a single small task.
* `income-source-check` folder contains an example of parsing files in comma-separated value format, reading values from them, and checking that these values obey some "business logic". The parsing, reading, and checking happens under ZK.
* `face-recognition` folder contains an implementation of face recognition under zero-knowledge. Several machine-learning methods have been implemented in ZK-SecreC for this purpose.
* `alg-approx` folder contains a method for evaluating elementary functions under zero-knowledge.
* `log-processing` folder an example of substring checks, making sure that multiple search strings are, or are not a part of a longer text.
* `electric-vehicle` folder contains the source code of our "electric vehicle demo app". It consists of several variations of checking that a geographic trajectory is long enough, while being located mostly in a given geographic area.
* `highway-tax` folder contains a statement stating that a geographic trajectory has followed a certain highway for no more than a given distance. It is a spinoff use case of the electric vehicle demo.
* `medical-checks` folder contains an example of checking [credentials](https://en.wikipedia.org/wiki/Verifiable_credentials) (in the [mDL format](https://en.wikipedia.org/wiki/Mobile_driver%27s_license)) in zero-knowledge.

Each folder contains its own `README.md` file, describing the example or examples in this folder.

## Acknowledgements

ZK-SecreC design and tool development has been funded by the Defense Advanced Research Projects Agency (DARPA) under contract HR0011-20-C-0083. The views, opinions, and/or findings expressed are those of the author(s) and should not be interpreted as representing the official views or policies of the Department of Defense or the U.S. Government. Distribution Statement "A" (Approved for Public Release, Distribution Unlimited).

