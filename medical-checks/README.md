# Medical check

## Introduction

In this example, we present the statement for solving the following problem. A military recruit must receive credentials from a certain list of doctors about his diseases, to be delivered to the Military Entrance Processing Station (MEPS). In order to make a positive decision, MEPS must get positive answers to the following questions:

* Do all the applicant’s diseases enable him to serve in the army?
* Are all the doctors who provided the documents recognized by authorities?
* Are all the documents valid?

For the first check, MEPS has an official set of disqualifying diagnoses. None of the diagnoses of the recruit may occur there. For the second check, MEPS uses a list of certified health care providers. All issuers of the delivered documents must occur in that list. For the third check to be successful, all documents must belong to the particular applicant and none of them may be out-of-date. As the recruit does not want the MEPS officials to know his particular diagnoses, zero-knowledge proof is used for interaction between the recruit and MEPS.

In practice, signatures of the doctors must also be checked. We have omitted this part as we assume signature checking to be performed outside zero knowledge.

Our solution reckons with the possibility that the MEPS officials can make a positive decision even in the presence of disqualifying diseases on a case-by-case basis. For applying to an exception, a recruit must reveal all his disqualifying diagnoses.

Our solution does not address the recruit’s chance to cheat by not delivering the credentials of disqualifying diagnoses to MEPS or just missing a medical examination. This could be addressed by other means, e.g., by asking all recognized health care providers to deliver the list of recruits whom they have issued credentials that are still valid (without actually sending the credentials to MEPS). By analyzing these data, MEPS can find out if some recruit has missed a required medical examination or denying its results.

Our implementation assumes that credentials issued by doctors have the mobile driver's licence (mDL) format. This format requires the fields to be serialized in [Concise Binary Object Representation (CBOR)](https://www.rfc-editor.org/rfc/rfc8949.html).

## The statement

### Inputs

#### Prover's inputs

In this scenario, the recruit is the prover. Its private input contains data he wants to hide, i.e., the credentials issued by doctors.

#### Verifier's inputs

In this scenario, the verifiers are the MEPS officials. Their input contains information known to MEPS, i.e., the list of disqualifying diagnoses and the list of certified doctors. If a recruit having some disqualifying diagnoses wants to apply to an exception, his revealed diagnoses also belong to the verifier's input. Such diagnoses must be removed from the automatic checklist during the proof.

Due to details of the mDL standard, the verifier's input must also contain hashes of some parts of the prover's input.

#### Public inputs

The public input in our scenario contains the following data:

* The name of the recruit;
* The day of birth of the recruit;
* The lists of lengths of fields of all health reports;
* An upper bound of all lengths of diagnoses;
* An upper bound of all lengths of names of doctors;
* The number of all disqualifying diagnoses;
* The number of all certified doctors;
* The number of all fields in the standard health report;
* The number of health reports the recruit has;
* The number of revealed diagnoses;
* The list of numbers of diagnoses in health reports;
* Today's date.

Besides these, public input contains some technical parameters related to the mDL standard.

### Checks and Computations

The statement that is to be proven in ZK consists of the following checks:

* The credentials are issued to the right person;
* The credentials are currently valid;
* All issuers of health reports are certified doctors;
* Each diagnosis the person has is either confessed by him or not disqualifying;
* All hashes indeed originate from the corresponding parts of the prover's input.

The main complexity lies in parsing the fields in the CBOR format that the health report contains.

In our implementation, we assume that diagnoses are represented by hierarchical codes consisting of up to 7 characters. The small length of the codes enables performing occurs checks more efficiently by interpreting strings as integers with the same bit representation, since such integers do not overflow in the case of standard circuit moduli. Diagnosis codes in the [International Classification of Diseases (ICD)](https://www.who.int/classifications/classification-of-diseases) fit into this limit, hence this assumption is realistic.

Hierarchic structure means that related diagnoses have codes with a common prefix. MEPS can declare all diagnoses whose codes have a particular common prefix to be disqualifying. This way, the list of disqualifying diagnoses can be shorter as not having to mention every particular disease. On the other hand, this requires testing a code being a prefix of another instead of an equality check. Testing if a private string has a prefix among a given list of strings is another relatively complex subtask in the whole implementation besides CBOR parsing.

## Implementation

### JSON objects

In the subdirectory `input`, we have included three kits of inputs for the medical check scenario of different size. They are located in files `1x2x1_witness.json`, `1x2x1_instance.json`, `1x2x1_public.json` and similar files with the second 1 being replaced with 10 or 100. This number refers to the length of the lists of disqualifying diagnoses and certified doctors. The first 1 is the number of health reports whereas the 2 is the number of diagnoses the recruit has.

Besides these, the subdirectory contains files with names `example_cbor_witness??.json`, `example_cbor_instance??.json` and `example_cbor_public??.json`. They are meant for testing the CBOR-related utilities separately. Among these test cases, numbers 10, 11 and 21 are supposed to fail, while all the others should succeed.

### ZK-SecreC source programs

In the subdirectory `src`, we have included our implementation of the described relation in ZK-SecreC. The implementation contains four files:

* `hashes.zksc` contains auxiliary functions related to computing of SHA-256 hashes and checking them;
* `utils.zksc` contains various other auxiliary functions;
* `cbor.zksc` contains a library of functions for operations on data structures represented in the CBOR format;
* `mdchk-cbor.zksc` contains our solution to the medical check use case.

Note that SHA-256 hashing requires external Bristol circuits whose location (directory) must be fed to the compiler with the `-b` flag.

Besides `mdchk-cbor.zksc`, the file `cbor.zksc` also contains a function `main`. This can be used for testing the CBOR-related utilities separately, using the files from the `examples_cbor_*.json` family as inputs.

### Configuration files

In the subdirectory `config`, we have included a Circuit Configuration Communication (CCC) file suitable for our implementation of the medical check scenario. Note that challenges are required because our implementation makes essential use of challenges.

## Helper script

The subdirectory `test-gen` contains a Haskell program `MedicalCheck.hs` (together with its imports `CBOR.hs`, `JSON.hs` and `Util.hs`) that is capable of generating inputs (`public.json`, `instance.json`, and `witness.json`) for the medical check statement. It takes as arguments the output directory and three parameters of the task (the number of health reports, the number of diagnoses in each report, and the common number of bad diagnoses and certified doctors).

There is no program for automatic generation of inputs for testing the CBOR parser separately.
